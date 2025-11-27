#pragma once

#include <shared_mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>

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

private:
  std::shared_mutex mutex_;
  std::unordered_map<char *, std::string> index_;
};
