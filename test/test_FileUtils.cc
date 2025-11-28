#include "FileUtils.h"
#include <gtest/gtest.h>
#include <cstring>
#include <filesystem>

TEST(FileUtilsTest, WriteReadRemove) {
  const char *content = "abcdef";
  char buf[16];
  std::memset(buf, 0, sizeof(buf));
  std::string file = "./test_fileutils.bin";

  std::string out = FileUtils::write(file, (char *)content, 6);
  EXPECT_EQ(out, file);
  FileUtils::read(file, 0, buf, 6);
  EXPECT_EQ(std::memcmp(buf, content, 6), 0);

  std::memset(buf, 0, sizeof(buf));
  FileUtils::read(file, 2, buf, 3);
  EXPECT_EQ(std::memcmp(buf, "cde", 3), 0);

  FileUtils::remove(file);
  EXPECT_FALSE(std::filesystem::exists(file));
}
