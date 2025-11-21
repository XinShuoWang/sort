#pragma once

#include "conf.h"

#include <atomic>
#include <cstring>
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
  Spiller() { fileId_ = 0; }
  ~Spiller() {}

  Spiller(const Spiller &) = delete;
  Spiller(Spiller &&) = delete;
  Spiller &operator=(const Spiller &) = delete;
  Spiller &operator=(Spiller &&) = delete;

  void unpin(char *addr, memSize size) {
    std::string fileName = write(addr, size);
    {
      std::lock_guard<std::mutex> guard(mutex_);
      addrToFileName_[addr] = fileName;
    }
  }

  void pin(char *startAddr, int64_t offset, char *dst, memSize size) {
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
    read(fileName, offset, dst, size);
  }

private:
  std::string write(char *addr, memSize size) {
    std::string fileName = nextFileName();
    std::ofstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("Can't open " + fileName + " for write.");
    }

    file.write(static_cast<const char *>(addr), size);
    if (!file.good()) {
      throw std::runtime_error("Encounter error for writing file.");
    }

    file.close();
    return fileName;
  }

  void read(std::string &fileName, int64_t offset, char *addr, memSize size) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("Can't open " + fileName + " for read.");
    }
    // seek to offset
    file.seekg(offset);
    // read 1 page
    file.read(static_cast<char *>(addr), size);
    if (!file.good()) {
      throw std::runtime_error("Encounter error for reading file.");
    }

    file.close();
  }

  std::string nextFileName() {
    int64_t id = fileId_.fetch_add(1);
    return std::to_string(id) + ".txt";
  }

  std::atomic<int64_t> fileId_;

  std::mutex mutex_;
  std::unordered_map<char *, std::string> addrToFileName_;
};

using SpillerPtr = std::shared_ptr<Spiller>;