#pragma once

#include "DirectoryUtils.h"
#include "FileUtils.h"
#include "conf.h"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
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
    {
      std::lock_guard<std::mutex> guard(mutex_);
      addrToFileName_[addr] = fileName;
    }
  }

  void recoverMem(char *startAddr, int64_t offset, char *dst, memSize size) {
    std::string fileName;
    {
      std::lock_guard<std::mutex> guard(mutex_);
      auto it = addrToFileName_.find(startAddr);
      if (it == addrToFileName_.end()) {
        throw std::runtime_error("Can't find file name mapping for address: " +
                                 std::to_string((uint64_t)startAddr));
      }
      fileName = it->second;
    }
    FileUtils::read(fileName, offset, dst, size);
  }

private:
  std::string nextFileName() {
    int64_t id = fileId_.fetch_add(1);
    return spillPath_ + "/" + std::to_string(id) + kFileSuffix;
  }

  std::atomic<int64_t> fileId_;
  inline static const std::string kFileSuffix = ".bin";

  std::mutex mutex_;
  std::unordered_map<char *, std::string> addrToFileName_;

  std::string spillPath_;
};

using SpillerPtr = std::shared_ptr<Spiller>;