#include "FileUtils.h"
#include <gtest/gtest.h>
#include <cstring>
#include <filesystem>
#include <vector>

TEST(FileUtilsTest, WriteReadRemove) {
  const char *content = "abcdef";
  char buf[16];
  std::memset(buf, 0, sizeof(buf));
  std::string file = "./test_fileutils.bin";

  std::string out = FileUtils::write(file, (char *)content, 6, CompressionType::None);
  EXPECT_EQ(out, file);
  FileUtils::read(file, 0, buf, 6);
  EXPECT_EQ(std::memcmp(buf, content, 6), 0);

  std::memset(buf, 0, sizeof(buf));
  FileUtils::read(file, 2, buf, 3);
  EXPECT_EQ(std::memcmp(buf, "cde", 3), 0);

  FileUtils::remove(file);
  EXPECT_FALSE(std::filesystem::exists(file));
}

TEST(FileUtilsTest, WriteReadCompressedZstd) {
  std::string file = "./test_fileutils_zstd.bin";
  std::vector<char> data(1024);
  for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>(i % 256);
  std::string out = FileUtils::write(file, data.data(), data.size(), CompressionType::Zstd);
  EXPECT_EQ(out, file);
  std::vector<char> buf(data.size());
  FileUtils::read(file, 0, buf.data(), buf.size());
  EXPECT_EQ(std::memcmp(buf.data(), data.data(), buf.size()), 0);
  std::vector<char> part(100);
  FileUtils::read(file, 100, part.data(), part.size());
  EXPECT_EQ(std::memcmp(part.data(), data.data() + 100, part.size()), 0);
  FileUtils::remove(file);
  EXPECT_FALSE(std::filesystem::exists(file));
}

TEST(FileUtilsTest, WriteReadCompressedLz4) {
  std::string file = "./test_fileutils_lz4.bin";
  std::vector<char> data(2048);
  for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>((i * 7) % 256);
  std::string out = FileUtils::write(file, data.data(), data.size(), CompressionType::Lz4);
  EXPECT_EQ(out, file);
  std::vector<char> buf(data.size());
  FileUtils::read(file, 0, buf.data(), buf.size());
  EXPECT_EQ(std::memcmp(buf.data(), data.data(), buf.size()), 0);
  std::vector<char> part(256);
  FileUtils::read(file, 256, part.data(), part.size());
  EXPECT_EQ(std::memcmp(part.data(), data.data() + 256, part.size()), 0);
  FileUtils::remove(file);
  EXPECT_FALSE(std::filesystem::exists(file));
}
