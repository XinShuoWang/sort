#include "MmapMemory.h"
#include <cstring>
#include <stdexcept>
#include <sys/mman.h>

MmapMemory::MmapMemory(memSize size) {
  requestSize_ = size;
  size_ = ((size / kPageSize) + (size % kPageSize == 0 ? 0 : 1)) * kPageSize;
  ptr_ = nullptr;
}

MmapMemory::MmapMemory(char *addr, memSize size)
    : ptr_(addr), size_(size), requestSize_(size) {}

char *MmapMemory::address() {
  if (ptr_ == nullptr) {
    auto memory = mmap(nullptr, size_, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (memory == MAP_FAILED) {
      throw std::runtime_error("mmap memory allocation failed!");
    }
    ptr_ = reinterpret_cast<char *>(memory);
  }
  return ptr_;
}

memSize MmapMemory::size() { return size_; }

memSize MmapMemory::requestSize() { return requestSize_; }

MmapMemory::~MmapMemory() {
  if (ptr_ != nullptr) {
    munmap(ptr_, size_);
  }
}
