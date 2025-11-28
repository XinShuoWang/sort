#pragma once

#include <cstdint>
#include <string>

using memSize = uint64_t;

constexpr memSize kPageSize = 16 * 1024 * 1024L;

struct Config {
  std::string spillDir;
  memSize quota;
};
