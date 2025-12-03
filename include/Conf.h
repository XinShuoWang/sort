#pragma once

#include <cstdint>
#include <string>

using memSize = uint64_t;

constexpr memSize kPageSize = 16 * 1024 * 1024L;

enum CompressionType {
  None = 0,
  Zstd = 1,
  Lz4 = 2,
};

struct Config {
  std::string spillDir;
  memSize quota;
  CompressionType compressionType;
};
