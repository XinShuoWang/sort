#pragma once

#include "Conf.h"
#include "MemAddrToFileMap.h"
#include "MmapMemory.h"

#include <atomic>
#include <glog/logging.h>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>

class Spiller {
public:
  explicit Spiller(const std::string &path);

  ~Spiller();

  Spiller(const Spiller &) = delete;
  Spiller(Spiller &&) = delete;
  Spiller &operator=(const Spiller &) = delete;
  Spiller &operator=(Spiller &&) = delete;

  void recoverMem(char *startAddr, int64_t offset, char *dst, memSize size);

  void registerMem(MmapMemoryPtr &mem);

  memSize spill(memSize targetSize);

private:
  void eraseMem(MmapMemoryPtr &mem);
  std::string nextFileName();

  inline static const std::string kFileSuffix = ".bin";

  std::atomic<int64_t> fileId_;
  MemAddrToFileMap addrToFileMap_;
  std::string spillPath_;

  std::queue<MmapMemoryPtr> allMems_;
};

using SpillerPtr = std::shared_ptr<Spiller>;
using SpillerWeakPtr = std::weak_ptr<Spiller>;
