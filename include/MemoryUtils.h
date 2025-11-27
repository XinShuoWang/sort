#pragma once

#include <cstdint>
#include <fstream>
#include <unistd.h>

class MemoryUtils {
public:
  static constexpr int64_t kInvalidRssSize = 0;

  static int64_t getProcessRss() {
    static const int64_t kPageSize = [] {
      const long pageSize = sysconf(_SC_PAGESIZE);
      return pageSize > 0 ? pageSize : 4096; // Typically 4096 bytes
    }();

    std::ifstream statmFile("/proc/self/statm");
    if (!statmFile) {
      return kInvalidRssSize;
    }

    int64_t rssPages = 0;
    // The first value is the total virtual memory size
    statmFile >> rssPages;
    // The second value is the number of pages in the RSS
    statmFile >> rssPages;
    return rssPages * kPageSize;
  }
};
