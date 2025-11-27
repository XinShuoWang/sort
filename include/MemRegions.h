#pragma once

#include "conf.h"
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

  private:
    std::mutex mutex_;
    std::vector<std::pair<char *, memSize>> regions_;
  };
