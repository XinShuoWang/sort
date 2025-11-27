#pragma once

#include "MmapMemory.h"
#include "Spiller.h"
#include "conf.h"
#include "MemRegions.h"
#include "Statistics.h"

#include <fcntl.h>
#include <glog/logging.h>
#include <linux/userfaultfd.h>
#include <mutex>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/eventfd.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <utility>


class BufferManager {
public:
  BufferManager(const std::string &spillPath)
      : userFaultFd_(-1), stopEventFd_(-1) {
    spiller_ = std::make_shared<Spiller>(spillPath);
    init();
  }

  BufferManager(const BufferManager &) = delete;
  BufferManager(BufferManager &&) = delete;
  BufferManager &operator=(const BufferManager &) = delete;
  BufferManager &operator=(BufferManager &&) = delete;

  ~BufferManager() {
    LOG(INFO) << "In the end, " << statistics_.toString();
    stop();
    if (userFaultFd_ >= 0) {
      close(userFaultFd_);
    }
    if (stopEventFd_ >= 0) {
      close(stopEventFd_);
    }
  }

  void invalidMemory(MmapMemoryPtr &mem) {
    char *addr = mem->address();
    memSize size = mem->size();
    spiller_->eraseMem(addr, size);
    invalidMemoryWithoutSave(addr, size);
    registerMemory(mem);
  }

  void invalidMemoryWithoutSave(char *addr, memSize size) {
    madvise(addr, size, MADV_DONTNEED);
  }

  static MmapMemoryPtr accquireMemory(int64_t size) {
    return std::make_shared<MmapMemory>(size);
  }

private:
  void init() {
    // create userfaultfd
    userFaultFd_ = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (userFaultFd_ < 0) {
      throw std::runtime_error("create userfaultfd failed!");
    }

    stopEventFd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (stopEventFd_ < 0) {
      throw std::runtime_error("create eventfd failed!");
    }

    // setting API
    uffdio_api api = {.api = UFFD_API, .features = 0};
    if (ioctl(userFaultFd_, UFFDIO_API, &api) < 0) {
      throw std::runtime_error("setting API failed!");
    }

    startPageFaultHandlerThread();
  }

  void registerMemory(MmapMemoryPtr &mem) {
    char *addr = mem->address();
    memSize size = mem->size();
    uffdio_register reg = {.range = {.start = (uint64_t)addr, .len = size},
                           .mode = UFFDIO_REGISTER_MODE_MISSING};
    if (ioctl(userFaultFd_, UFFDIO_REGISTER, &reg) < 0) {
      throw std::runtime_error("register memory address failed!");
    }
    regions_.add(addr, size);
  }

  void startPageFaultHandlerThread() {
    std::atomic<bool> hasStarted{false};
    handlerThread_ = std::thread([this, &hasStarted]() {
      hasStarted.store(true, std::memory_order_release);
      while (true) {
        pollfd pfds[2] = {{.fd = userFaultFd_, .events = POLLIN, .revents = 0},
                          {.fd = stopEventFd_, .events = POLLIN, .revents = 0}};
        int ret = poll(pfds, 2, -1);
        if (ret > 0) {
          if (pfds[1].revents & POLLIN) {
            uint64_t v;
            ssize_t r = read(stopEventFd_, &v, sizeof(v));
            (void)r;
            break;
          }
          if (pfds[0].revents & POLLIN) {
            handlePageFaultEvent();
          }
        }
      }
    });
    while(!hasStarted.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  }

  void handlePageFaultEvent() {
    uffd_msg msg;
    if (read(userFaultFd_, &msg, sizeof(msg)) != sizeof(msg)) {
      return;
    }

    if (msg.event == UFFD_EVENT_PAGEFAULT) {
      char *addr = reinterpret_cast<char *>(msg.arg.pagefault.address);

      statistics_.pageFaultCount++;

      // fill by origin content
      char s[kPageSize];
      auto startAddr = regions_.findStart(addr);
      spiller_->recoverMem(startAddr, addr - startAddr, s, kPageSize);

      // tell kernel, page is ready
      uffdio_copy copy = {.dst = msg.arg.pagefault.address,
                          .src = (uint64_t)s,
                          .len = kPageSize,
                          .mode = 0};
      ioctl(userFaultFd_, UFFDIO_COPY, &copy);
    }
  }

  void stop() {
    uint64_t v = 1;
    if (stopEventFd_ >= 0) {
      ssize_t w = write(stopEventFd_, &v, sizeof(v));
      (void)w;
    }
    if (handlerThread_.joinable()) {
      handlerThread_.join();
    }
  }

  int userFaultFd_, stopEventFd_;
  std::thread handlerThread_;
  MemRegions regions_;
  SpillerPtr spiller_;
  Statistics statistics_;
};
