#include "PageFaultHandler.h"
#include "Spiller.h"

#include <chrono>

PageFaultHandler::PageFaultHandler(SpillerPtr spiller)
    : userFaultFd_(-1), stopEventFd_(-1), spiller_(spiller),
      buffer_(std::make_shared<Buffer>(kPageSize)) {
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
  std::atomic<bool> hasStarted{false};
  handlerThread_ = std::thread([this, &hasStarted]() {
    hasStarted.store(true, std::memory_order_release);
    LOG(INFO) << "pagefault handler thread started";
    loop();
  });
  while (!hasStarted.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }
  LOG(INFO) << "pagefault handler init userfaultfd=" << userFaultFd_
            << " stopEventFd=" << stopEventFd_;
}

PageFaultHandler::~PageFaultHandler() {
  uint64_t v = 1;
  if (stopEventFd_ >= 0) {
    [[maybe_unused]] ssize_t w = write(stopEventFd_, &v, sizeof(v));
  }
  if (handlerThread_.joinable()) {
    handlerThread_.join();
  }
  if (userFaultFd_ >= 0) {
    close(userFaultFd_);
  }
  if (stopEventFd_ >= 0) {
    close(stopEventFd_);
  }
  LOG(INFO) << "PageFaultHandler statistics: " << stats_.toString();
}

void PageFaultHandler::registerMemory(MmapMemoryPtr &mem) {
  char *addr = mem->address();
  memSize size = mem->size();
  uffdio_register reg = {.range = {.start = (uint64_t)addr, .len = size},
                         .mode = UFFDIO_REGISTER_MODE_MISSING};
  if (ioctl(userFaultFd_, UFFDIO_REGISTER, &reg) < 0) {
    throw std::runtime_error("register memory address failed!");
  }
  regions_.add(addr, size);
  LOG(INFO) << "pagefault register range start=" << (uint64_t)addr
            << " size=" << size;
}

bool PageFaultHandler::unregisterMemory(char *addr, memSize size) {
  // After unmmap, the memory is not registered anymore, so we don't need to
  // unregister it, but we still need to remove it from regions_.
  // uffdio_range unreg{.start = (uint64_t)addr, .len = size};
  // if (ioctl(userFaultFd_, UFFDIO_UNREGISTER, &unreg) < 0) {
  //   throw std::runtime_error("unregister memory address failed!");
  // }
  bool removed = regions_.remove(addr);
  LOG(INFO) << "pagefault unregister range start=" << (uint64_t)addr
            << " size=" << size << " removed=" << removed;
  return true;
}

void PageFaultHandler::loop() {
  while (true) {
    pollfd pfds[2] = {{.fd = userFaultFd_, .events = POLLIN, .revents = 0},
                      {.fd = stopEventFd_, .events = POLLIN, .revents = 0}};
    int ret = poll(pfds, 2, -1);
    if (ret > 0) {
      if (pfds[1].revents & POLLIN) {
        uint64_t v;
        [[maybe_unused]] ssize_t r = read(stopEventFd_, &v, sizeof(v));
        LOG(INFO) << "pagefault handler stop signal received";
        break;
      }
      if (pfds[0].revents & POLLIN) {
        handleEvent();
      }
    }
  }
}

void PageFaultHandler::handleEvent() {
  uffd_msg msg;
  if (read(userFaultFd_, &msg, sizeof(msg)) != sizeof(msg)) {
    return;
  }
  if (msg.event == UFFD_EVENT_PAGEFAULT) {
    char *addr = reinterpret_cast<char *>(msg.arg.pagefault.address);
    stats_.pageFaultCount++;
    LOG(INFO) << "pagefault event addr=" << (uint64_t)addr;
    auto startAddr = regions_.findStart(addr);
    spiller_->recoverMem(startAddr, addr - startAddr, buffer_->data(),
                         kPageSize);
    uffdio_copy copy = {.dst = msg.arg.pagefault.address,
                        .src = (uint64_t)buffer_->data(),
                        .len = kPageSize,
                        .mode = 0};
    ioctl(userFaultFd_, UFFDIO_COPY, &copy);
    LOG(INFO) << "pagefault copied page size=" << kPageSize;
  }
}
