#pragma once

#include "MmapMemory.h"
#include "Spiller.h"
#include "conf.h"

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
#include <utility>

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
      memRegions_.emplace_back(std::make_pair(addr, size));
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

      std::cout << "Page fault! address: " << (void *)msg.arg.pagefault.address
                << ", size: " << kPageSize << std::endl;

      // fill by origin content
      char s[kPageSize];
      auto startAddr = findStartAddr(addr);
      spiller_->pin(startAddr, addr - startAddr, s, kPageSize);

      // tell kernel, page is ready
      uffdio_copy copy = {.dst = msg.arg.pagefault.address,
                          .src = (uint64_t)s,
                          .len = kPageSize,
                          .mode = 0};
      ioctl(fd_, UFFDIO_COPY, &copy);
    }
  }

  // TODO: can optimize
  char *findStartAddr(char *addr) {
    std::lock_guard<std::mutex> guard(mutex_);
    for (const auto &region : memRegions_) {
      if (addr >= region.first && addr < region.first + region.second) {
        return region.first;
      }
    }
    throw std::runtime_error("Can't find start address");
  }

  void stop() {
    isRunning_ = false;
    // waiting for stop
    usleep(1000000);
    if (handlerThread_.joinable()) {
      handlerThread_.join();
    }
  }

  int fd_;
  std::atomic<bool> isRunning_;
  std::thread handlerThread_;

  std::mutex mutex_;
  std::vector<std::pair<char *, int64_t>> memRegions_;

  SpillerPtr spiller_;
};