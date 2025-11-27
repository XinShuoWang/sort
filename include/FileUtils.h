#pragma once

#include "conf.h"

#include <fstream>
#include <stdexcept>
#include <string>

class FileUtils {
public:
  static std::string write(const std::string &fileName, char *addr,
                           memSize size) {
    std::ofstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("Can't open " + fileName + " for write.");
    }

    file.write(static_cast<const char *>(addr), size);
    if (!file.good()) {
      throw std::runtime_error("Encounter error for writing file.");
    }

    file.close();
    return fileName;
  }

  static void read(std::string &fileName, int64_t offset, char *addr,
                   memSize size) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("Can't open " + fileName + " for read.");
    }
    // seek to offset
    file.seekg(offset);
    // read size bytes
    file.read(static_cast<char *>(addr), size);
    if (!file.good()) {
      throw std::runtime_error("Encounter error for reading file.");
    }

    file.close();
  }
};
