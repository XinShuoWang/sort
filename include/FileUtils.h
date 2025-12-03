#pragma once

#include "Conf.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <cstdint>
#include "Compression.h"

struct FileMeta {
  uint32_t magic;
  uint16_t version;
  uint16_t method;
  uint64_t originalSize;
  uint64_t compressedSize;
};

class FileUtils {
public:
  static std::string write(const std::string &fileName, char *addr,
                           memSize size,
                           CompressionType type = CompressionType::None);

  static void read(std::string &fileName, int64_t offset, char *addr,
                   memSize size);

  static void remove(const std::string &fileName);
};
