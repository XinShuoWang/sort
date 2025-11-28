#pragma once

#include "Conf.h"
#include <mutex>
#include <vector>
#include <stdexcept>

  class MemRegions {
  public:
    void add(char *addr, memSize size) {
      std::lock_guard<std::mutex> guard(mutex_);
      regions_.emplace_back(addr, size);
    }

    char *findStart(char *addr) {
      std::lock_guard<std::mutex> guard(mutex_);
      for (const auto &region : regions_) {
        if (addr >= region.first && addr < region.first + region.second) {
          return region.first;
        }
      }
      throw std::runtime_error("Can't find start address");
    }

    bool remove(char *addr) {
      std::lock_guard<std::mutex> guard(mutex_);
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
