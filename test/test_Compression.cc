#include "Compression.h"
#include <gtest/gtest.h>
#include <vector>
#include <cstring>

TEST(CompressionTest, NoneRoundTrip) {
  std::vector<char> data(1024);
  for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>(i % 256);
  auto comp = compressBuffer(data.data(), data.size(), CompressionType::None);
  EXPECT_EQ(comp.size(), data.size());
  std::vector<char> out(data.size());
  decompressBuffer(comp.data(), comp.size(), out.data(), out.size(), CompressionType::None);
  EXPECT_EQ(std::memcmp(out.data(), data.data(), data.size()), 0);
}

TEST(CompressionTest, ZstdRoundTrip) {
  std::vector<char> data(4096);
  for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>((i * 7) % 256);
  auto comp = compressBuffer(data.data(), data.size(), CompressionType::Zstd);
  EXPECT_LT(comp.size(), data.size());
  std::vector<char> out(data.size());
  decompressBuffer(comp.data(), comp.size(), out.data(), out.size(), CompressionType::Zstd);
  EXPECT_EQ(std::memcmp(out.data(), data.data(), data.size()), 0);
}

TEST(CompressionTest, Lz4RoundTrip) {
  std::vector<char> data(4096);
  for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>((i * 13) % 256);
  auto comp = compressBuffer(data.data(), data.size(), CompressionType::Lz4);
  EXPECT_LT(comp.size(), data.size());
  std::vector<char> out(data.size());
  decompressBuffer(comp.data(), comp.size(), out.data(), out.size(), CompressionType::Lz4);
  EXPECT_EQ(std::memcmp(out.data(), data.data(), data.size()), 0);
}

