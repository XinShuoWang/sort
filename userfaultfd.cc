#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/userfaultfd.h>
#include <map>
#include <memory>
#include <mutex>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

constexpr int64_t kPageSize = 4096;

class MmapMemory {
public:
  MmapMemory(int64_t size) {
    requestSize_ = size;
    size_ = ((size / kPageSize) + (size % kPageSize == 0 ? 0 : 1)) * kPageSize;
    ptr_ = nullptr;
  }

  MmapMemory(const MmapMemory &) = delete;
  MmapMemory(MmapMemory &&) = delete;
  MmapMemory &operator=(const MmapMemory &) = delete;
  MmapMemory &operator=(MmapMemory &&) = delete;

  char *address() {
    if (ptr_ == nullptr) {
      auto memory = mmap(nullptr, size_, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (memory == MAP_FAILED) {
        throw std::runtime_error("mmap memory allocation failed!");
      }
      ptr_ = reinterpret_cast<char *>(memory);
    }
    return ptr_;
  }

  int64_t size() { return size_; }

  ~MmapMemory() {
    if (ptr_ != nullptr) {
      munmap(ptr_, size_);
    }
  }

private:
  int64_t size_, requestSize_;
  char *ptr_;
};

using MmapMemoryPtr = std::shared_ptr<MmapMemory>;

class MmapMemoryManager {
public:
  static MmapMemoryPtr AccquireMemory(int64_t size) {
    return std::make_shared<MmapMemory>(size);
  }
};

class Userfaultfd {
public:
  Userfaultfd() {
    fd_ = -1;
    isRunning_ = true;
  }

  ~Userfaultfd() {
    if (fd_ >= 0) {
      close(fd_);
    }
  }

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
  }

  void registerMemory(MmapMemoryPtr &mem) {
    char *addr = mem->address();
    uint64_t size = mem->size();
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
  }

  void stop() {
    isRunning_ = false;
    if (handlerThread_.joinable()) {
      handlerThread_.join();
    }
  }

private:
  void handlePageFaultEvent() {
    uffd_msg msg;
    if (read(fd_, &msg, sizeof(msg)) != sizeof(msg)) {
      return;
    }

    if (msg.event == UFFD_EVENT_PAGEFAULT) {
      uint64_t size;
      {
        std::lock_guard<std::mutex> guard(mutex_);
        char *addr = reinterpret_cast<char *>(msg.arg.pagefault.address);
        size = memorySize_[addr];
        memorySize_.erase(addr);
      }

      std::cout << "Page fault! address: " << (void *)msg.arg.pagefault.address
                << ", size: " << size << std::endl;

      // 分配一个页面
      std::string s(size, 'A');

      // 告诉内核页面就绪
      uffdio_copy copy = {.dst = msg.arg.pagefault.address,
                          .src = (uint64_t)s.c_str(),
                          .len = size,
                          .mode = 0};
      ioctl(fd_, UFFDIO_COPY, &copy);
    }
  }

  int fd_;
  bool isRunning_;
  std::thread handlerThread_;

  std::mutex mutex_;
  std::unordered_map<char *, uint64_t> memorySize_;
};

int main() {
  try {
    Userfaultfd demo;

    demo.init();

    // start handler thread
    demo.startPageFaultHandlerThread();
    // waiting for handler thread start
    usleep(100000);

    // test memory access
    {
      std::cout << "TESTING MEM ACCESS" << std::endl;

      int64_t memSize = kPageSize * 2 + 3;

      // allocate mem
      auto memory = MmapMemoryManager::AccquireMemory(memSize);

      // register to userfaultfd
      demo.registerMemory(memory);

      char *addr = memory->address();

      std::cout << "First access mem!" << std::endl;
      // Page Fault!!!!
      std::string s(addr, memSize);
      std::cout << "mem content is: " << s << std::endl;
    }

    // 等待一下让处理线程完成
    sleep(1);
    demo.stop();

    std::cout << "Finish success!" << std::endl;

  } catch (const std::exception &e) {
    std::cout << "Encounter error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}