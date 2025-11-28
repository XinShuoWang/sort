#include "Buffer.h"
#include <gtest/gtest.h>
#include <cstring>

TEST(BufferTest, SizeAndData) {
  Buffer buf(1024);
  EXPECT_EQ(buf.size(), 1024);
  std::memset(buf.data(), 0xEF, buf.size());
  EXPECT_EQ(buf.data()[0], (char)0xEF);
}
