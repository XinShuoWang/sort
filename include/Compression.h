#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <vector>

enum class CompressionType : uint16_t {
  None = 0,
  Zstd = 1,
  Lz4 = 2,
};

std::vector<char> compressBuffer(const char *src, size_t size,
                                 CompressionType type);

void decompressBuffer(const char *src, size_t csize, char *dst, size_t dsize,
                      CompressionType type);

void compressToStream(const char *src, size_t size, CompressionType type,
                      std::ostream &out);

void decompressFromStreamToRange(std::istream &in, CompressionType type,
                                 size_t originalSize, size_t offset, char *dst,
                                 size_t size);
