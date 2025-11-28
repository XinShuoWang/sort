#pragma once

#include "FileUtils.h"

#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

class MemAddrToFileMap {
public:
  void set(char *addr, const std::string &fileName) {
    std::unique_lock<std::shared_mutex> g(mutex_);
    index_[addr] = fileName;
  }

  std::optional<std::string> get(char *addr) {
    std::shared_lock<std::shared_mutex> g(mutex_);
    auto it = index_.find(addr);
    if (it == index_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  void erase(char *addr) {
    std::unique_lock<std::shared_mutex> g(mutex_);
    auto it = index_.find(addr);
    if (it == index_.end()) {
      throw std::runtime_error("addr not found!");
    }
    FileUtils::remove(it->second);
    index_.erase(addr);
  }

private:
  std::shared_mutex mutex_;
  std::unordered_map<char *, std::string> index_;
};
