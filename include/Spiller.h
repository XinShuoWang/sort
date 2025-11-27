#pragma once

#include "DirectoryUtils.h"
#include "FileUtils.h"
#include "conf.h"
#include "MemAddrToFileMap.h"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class Spiller {
public:
  Spiller(const std::string &path) : spillPath_(path) {
    fileId_ = 0;
    DirectoryUtils::createDir(spillPath_);
  }

  ~Spiller() { DirectoryUtils::removeAll(spillPath_); }

  Spiller(const Spiller &) = delete;
  Spiller(Spiller &&) = delete;
  Spiller &operator=(const Spiller &) = delete;
  Spiller &operator=(Spiller &&) = delete;

  void eraseMem(char *addr, memSize size) {
    std::string fileName = FileUtils::write(nextFileName(), addr, size);
    index_.set(addr, fileName);
  }

  void recoverMem(char *startAddr, int64_t offset, char *dst, memSize size) {
    auto fileNameOpt = index_.get(startAddr);
    if (!fileNameOpt) {
      throw std::runtime_error("Can't find file name mapping for address: " +
                               std::to_string((uint64_t)startAddr));
    }
    FileUtils::read(*fileNameOpt, offset, dst, size);
  }

private:
  std::string nextFileName() {
    int64_t id = fileId_.fetch_add(1);
    return spillPath_ + "/" + std::to_string(id) + kFileSuffix;
  }

  std::atomic<int64_t> fileId_;
  MemAddrToFileMap index_;
  std::string spillPath_;

  inline static const std::string kFileSuffix = ".bin";
};

using SpillerPtr = std::shared_ptr<Spiller>;
