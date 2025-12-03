#pragma once

#include "Conf.h"

#include <memory>

class MmapMemory {
public:
  explicit MmapMemory(memSize size);
  MmapMemory(char *addr, memSize size);

  MmapMemory(const MmapMemory &) = delete;
  MmapMemory(MmapMemory &&) = delete;
  MmapMemory &operator=(const MmapMemory &) = delete;
  MmapMemory &operator=(MmapMemory &&) = delete;

  char *address();
  memSize size();
  memSize requestSize();
  ~MmapMemory();

private:
  memSize size_, requestSize_;
  char *ptr_;
};

using MmapMemoryPtr = std::shared_ptr<MmapMemory>;
