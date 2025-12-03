#pragma once

#include <cstdint>
#include <string>

struct Statistics {
  uint64_t pageFaultCount{0};

  std::string toString() const {
    return "pageFaultCount: " + std::to_string(pageFaultCount);
  }
};
