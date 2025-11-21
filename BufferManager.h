#pragma once

#include "MmapMemory.h"
#include "Spiller.h"

#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

class BufferManager {
public:
  BufferManager() {
    fd_ = -1;
    isRunning_ = true;
    spiller_ = std::make_shared<Spiller>();
    init();
  }

  ~BufferManager() {
    stop();
    if (fd_ >= 0) {
      close(fd_);
    }
  }

  void invalidMemory(MmapMemoryPtr &mem) {
    char *addr = mem->address();
    memSize size = mem->size();
    spiller_->unpin(addr, size);
    madvise(addr, size, MADV_DONTNEED);
    registerMemory(mem);
  }

  static MmapMemoryPtr accquireMemory(int64_t size) {
    return std::make_shared<MmapMemory>(size);
  }

private:
  void init() {
    // create userfaultfd
    fd_ = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (fd_ < 0) {
      throw std::runtime_error("create userfaultfd failed!");
    }

    // setting API
    uffdio_api api = {.api = UFFD_API, .features = 0};
    if (ioctl(fd_, UFFDIO_API, &api) < 0) {
      throw std::runtime_error("setting API failed!");
    }

    startPageFaultHandlerThread();
  }

  void registerMemory(MmapMemoryPtr &mem) {
    char *addr = mem->address();
    memSize size = mem->size();
    uffdio_register reg = {.range = {.start = (uint64_t)addr, .len = size},
                           .mode = UFFDIO_REGISTER_MODE_MISSING};
    if (ioctl(fd_, UFFDIO_REGISTER, &reg) < 0) {
      throw std::runtime_error("register memory address failed!");
    }
    {
      std::lock_guard<std::mutex> guard(mutex_);
      memorySize_[addr] = size;
    }
  }

  void startPageFaultHandlerThread() {
    handlerThread_ = std::thread([this]() {
      std::cout << "Page fault handler thread start!" << std::endl;

      while (isRunning_) {
        // waiting for PageFault event...
        pollfd pfd = {.fd = fd_, .events = POLLIN};
        // 100ms timeout
        int ret = poll(&pfd, 1, 100);

        if (ret > 0 && (pfd.revents & POLLIN)) {
          handlePageFaultEvent();
        }
      }
    });
    // waiting for thread start
    usleep(100000);
  }

  void handlePageFaultEvent() {
    uffd_msg msg;
    if (read(fd_, &msg, sizeof(msg)) != sizeof(msg)) {
      return;
    }

    if (msg.event == UFFD_EVENT_PAGEFAULT) {
      char *addr = reinterpret_cast<char *>(msg.arg.pagefault.address);
      memSize size;
      {
        std::lock_guard<std::mutex> guard(mutex_);
        size = memorySize_[addr];
        memorySize_.erase(addr);
      }

      std::cout << "Page fault! address: " << (void *)msg.arg.pagefault.address
                << ", size: " << size << std::endl;

      // 分配一个页面
      std::string s(size, 'A');

      spiller_->pin(addr, const_cast<char *>(s.c_str()), size);

      // 告诉内核页面就绪
      uffdio_copy copy = {.dst = msg.arg.pagefault.address,
                          .src = (uint64_t)s.c_str(),
                          .len = size,
                          .mode = 0};
      ioctl(fd_, UFFDIO_COPY, &copy);
    }
  }

  void stop() {
    isRunning_ = false;
    usleep(1000000);
    if (handlerThread_.joinable()) {
      handlerThread_.join();
    }
  }

  int fd_;
  std::atomic<bool> isRunning_;
  std::thread handlerThread_;

  std::mutex mutex_;
  std::unordered_map<char *, uint64_t> memorySize_;

  SpillerPtr spiller_;
};