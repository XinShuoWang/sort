#pragma once

#include "conf.h"

#include <memory>
#include <stdexcept>
#include <sys/mman.h>

class MmapMemory {
public:
  MmapMemory(memSize size) {
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

  memSize size() { return size_; }

  memSize requestSize() { return requestSize_; }

  ~MmapMemory() {
    if (ptr_ != nullptr) {
      munmap(ptr_, size_);
    }
  }

private:
  memSize size_, requestSize_;
  char *ptr_;
};

using MmapMemoryPtr = std::shared_ptr<MmapMemory>;
