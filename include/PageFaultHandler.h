#pragma once

#include "MemRegions.h"
#include "Spiller.h"
#include "Statistics.h"
#include "conf.h"
#include "MmapMemory.h"

#include <atomic>
#include <poll.h>
#include <thread>
#include <unistd.h>
#include <linux/userfaultfd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/eventfd.h>
#include <glog/logging.h>
#include <fcntl.h>

class PageFaultHandler {
public:
  explicit PageFaultHandler(const std::string &spillPath)
      : userFaultFd_(-1), stopEventFd_(-1), spiller_(std::make_shared<Spiller>(spillPath)) {
    init();
  }

  ~PageFaultHandler() {
    stop();
    if (userFaultFd_ >= 0) {
      close(userFaultFd_);
    }
    if (stopEventFd_ >= 0) {
      close(stopEventFd_);
    }
    LOG(INFO) << "PageFaultHandler statistics: " << stats_.toString();
  }

  void start() {
    std::atomic<bool> hasStarted{false};
    th_ = std::thread([this, &hasStarted]() {
      hasStarted.store(true, std::memory_order_release);
      loop();
    });
    while (!hasStarted.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  }

  void stop() {
    uint64_t v = 1;
    if (stopEventFd_ >= 0) {
      [[maybe_unused]] ssize_t w = write(stopEventFd_, &v, sizeof(v));
    }
    if (th_.joinable()) {
      th_.join();
    }
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

private:
  void init() {
    userFaultFd_ = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (userFaultFd_ < 0) {
      throw std::runtime_error("create userfaultfd failed!");
    }
    stopEventFd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (stopEventFd_ < 0) {
      throw std::runtime_error("create eventfd failed!");
    }
    uffdio_api api = {.api = UFFD_API, .features = 0};
    if (ioctl(userFaultFd_, UFFDIO_API, &api) < 0) {
      throw std::runtime_error("setting API failed!");
    }
  }

  void loop() {
    while (true) {
      pollfd pfds[2] = {{.fd = userFaultFd_, .events = POLLIN, .revents = 0},
                        {.fd = stopEventFd_, .events = POLLIN, .revents = 0}};
      int ret = poll(pfds, 2, -1);
      if (ret > 0) {
        if (pfds[1].revents & POLLIN) {
          uint64_t v;
          [[maybe_unused]] ssize_t r = read(stopEventFd_, &v, sizeof(v));
          break;
        }
        if (pfds[0].revents & POLLIN) {
          handleEvent();
        }
      }
    }
  }

  void handleEvent() {
    uffd_msg msg;
    if (read(userFaultFd_, &msg, sizeof(msg)) != sizeof(msg)) {
      return;
    }
    if (msg.event == UFFD_EVENT_PAGEFAULT) {
      char *addr = reinterpret_cast<char *>(msg.arg.pagefault.address);
      stats_.pageFaultCount++;
      char s[kPageSize];
      auto startAddr = regions_.findStart(addr);
      spiller_->recoverMem(startAddr, addr - startAddr, s, kPageSize);
      uffdio_copy copy = {.dst = msg.arg.pagefault.address,
                          .src = (uint64_t)s,
                          .len = kPageSize,
                          .mode = 0};
      ioctl(userFaultFd_, UFFDIO_COPY, &copy);
    }
  }

private:
  int userFaultFd_, stopEventFd_;
  std::thread th_;
  MemRegions regions_;
  SpillerPtr spiller_;
  Statistics stats_;
};
