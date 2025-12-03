#pragma once

#include "Conf.h"

#include <cstdint>
#include <istream>
#include <ostream>
#include <vector>

std::vector<char> compressBuffer(const char *src, size_t size,
                                 CompressionType type);

void decompressBuffer(const char *src, size_t csize, char *dst, size_t dsize,
                      CompressionType type);

void compressToStream(const char *src, size_t size, CompressionType type,
                      std::ostream &out);

void decompressFromStreamToRange(std::istream &in, CompressionType type,
                                 size_t originalSize, size_t offset, char *dst,
                                 size_t size);
