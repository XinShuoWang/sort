#pragma once

#include "Conf.h"
#include <mutex>
#include <stdexcept>
#include <vector>

// Can be optimzied to O(logn)
class MemRegions {
public:
  void add(char *addr, memSize size) {
    std::unique_lock<std::mutex> guard(mutex_);
    regions_.emplace_back(addr, size);
  }

  char *findStart(char *addr) {
    std::unique_lock<std::mutex> guard(mutex_);
    for (const auto &region : regions_) {
      if (addr >= region.first && addr < region.first + region.second) {
        return region.first;
      }
    }
    throw std::runtime_error("Can't find start address");
  }

  bool remove(char *addr) {
    std::unique_lock<std::mutex> guard(mutex_);
    for (auto it = regions_.begin(); it != regions_.end(); ++it) {
      if (it->first == addr) {
        regions_.erase(it);
        return true;
      }
    }
    return false;
  }

private:
  std::mutex mutex_;
  std::vector<std::pair<char *, memSize>> regions_;
};
