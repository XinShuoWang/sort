#include "FileUtils.h"

#include <filesystem>
#include <fstream>

std::string FileUtils::write(const std::string &fileName, char *addr,
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

void FileUtils::read(std::string &fileName, int64_t offset, char *addr,
                     memSize size) {
  std::ifstream file(fileName, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Can't open " + fileName + " for read.");
  }
  file.seekg(offset);
  file.read(static_cast<char *>(addr), size);
  if (!file.good()) {
    throw std::runtime_error("Encounter error for reading file.");
  }

  file.close();
}

void FileUtils::remove(const std::string &fileName) {
  if (!std::filesystem::remove(fileName)) {
    throw std::runtime_error("Can't remove file " + fileName);
  }
}
