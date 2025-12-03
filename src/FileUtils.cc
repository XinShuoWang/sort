#include "FileUtils.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Compression.h"

static constexpr uint32_t kMagic = 0x53554C46;
static constexpr uint16_t kVersion = 1;

std::string FileUtils::write(const std::string &fileName, char *addr,
                             memSize size, CompressionType type) {
  std::ofstream file(fileName, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Can't open " + fileName + " for write.");
  }

  FileMeta meta;
  meta.magic = kMagic;
  meta.version = kVersion;
  meta.method = static_cast<uint16_t>(type);
  meta.originalSize = size;

  if (type == CompressionType::None) {
    meta.compressedSize = size;
    file.write(reinterpret_cast<const char *>(&meta), sizeof(meta));
    if (!file.good()) {
      throw std::runtime_error("Encounter error for writing file.");
    }
    file.write(static_cast<const char *>(addr), size);
    if (!file.good()) {
      throw std::runtime_error("Encounter error for writing file.");
    }
  } else if (type == CompressionType::Zstd || type == CompressionType::Lz4) {
    // Write placeholder meta, then stream-compress to file, then update meta with compressed size
    meta.compressedSize = 0;
    file.write(reinterpret_cast<const char *>(&meta), sizeof(meta));
    if (!file.good()) {
      throw std::runtime_error("Encounter error for writing file.");
    }
    auto startPos = file.tellp();
    compressToStream(addr, size, type, file);
    auto endPos = file.tellp();
    if (startPos == std::ostream::pos_type(-1) || endPos == std::ostream::pos_type(-1)) {
      throw std::runtime_error("Encounter error for computing compressed size.");
    }
    meta.compressedSize = static_cast<uint64_t>(endPos - startPos);
    file.seekp(0);
    file.write(reinterpret_cast<const char *>(&meta), sizeof(meta));
    if (!file.good()) {
      throw std::runtime_error("Encounter error for writing file meta.");
    }
  } else {
    throw std::runtime_error("Unsupported compression type");
  }

  file.close();
  return fileName;
}

void FileUtils::read(std::string &fileName, int64_t offset, char *addr,
                     memSize size) {
  std::ifstream file(fileName, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Can't open " + fileName + " for read.");
  }
  FileMeta meta;
  file.read(reinterpret_cast<char *>(&meta), sizeof(meta));
  if (!file.good()) {
    throw std::runtime_error("Encounter error for reading file.");
  }

  if (meta.magic != kMagic || meta.version == 0) {
    throw std::runtime_error("Encounter bad spill file when reading.");
  }

  if (static_cast<uint64_t>(offset) + static_cast<uint64_t>(size) >
      meta.originalSize) {
    throw std::runtime_error("Read range exceeds original size");
  }

  auto method = static_cast<CompressionType>(meta.method);
  if (method == CompressionType::None) {
    file.seekg(static_cast<std::streamoff>(sizeof(meta)) + offset);
    file.read(static_cast<char *>(addr), size);
    if (!file.good()) {
      throw std::runtime_error("Encounter error for reading file.");
    }
  } else {
    file.seekg(static_cast<std::streamoff>(sizeof(meta)));
    decompressFromStreamToRange(file, method, meta.originalSize, offset, addr,
                                size);
  }

  file.close();
}

void FileUtils::remove(const std::string &fileName) {
  if (!std::filesystem::remove(fileName)) {
    throw std::runtime_error("Can't remove file " + fileName);
  }
}
