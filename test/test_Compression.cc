#include "Compression.h"
#include <gtest/gtest.h>
#include <vector>
#include <cstring>
#include <sstream>

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

TEST(CompressionTest, StreamZstdRange) {
  std::vector<char> data(200000);
  for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>((i * 7) % 256);
  std::ostringstream oss;
  compressToStream(data.data(), data.size(), CompressionType::Zstd, oss);
  std::string comp = oss.str();
  std::istringstream iss(comp);
  std::vector<char> out(5000);
  decompressFromStreamToRange(iss, CompressionType::Zstd, data.size(), 10000, out.data(), out.size());
  EXPECT_EQ(std::memcmp(out.data(), data.data() + 10000, out.size()), 0);
}

TEST(CompressionTest, StreamLz4Range) {
  std::vector<char> data(250000);
  for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>((i * 5) % 256);
  std::ostringstream oss;
  compressToStream(data.data(), data.size(), CompressionType::Lz4, oss);
  std::string comp = oss.str();
  std::istringstream iss(comp);
  std::vector<char> out(8000);
  decompressFromStreamToRange(iss, CompressionType::Lz4, data.size(), 12345, out.data(), out.size());
  EXPECT_EQ(std::memcmp(out.data(), data.data() + 12345, out.size()), 0);
}

TEST(CompressionTest, StreamNoneRange) {
  std::vector<char> data(10000);
  for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>(i % 256);
  std::string comp(data.begin(), data.end());
  std::istringstream iss(comp);
  std::vector<char> out(3000);
  decompressFromStreamToRange(iss, CompressionType::None, data.size(), 2000, out.data(), out.size());
  EXPECT_EQ(std::memcmp(out.data(), data.data() + 2000, out.size()), 0);
}

TEST(CompressionTest, StreamInvalidRangeZstd) {
  std::vector<char> data(10000);
  for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>((i * 7) % 256);
  std::ostringstream oss;
  compressToStream(data.data(), data.size(), CompressionType::Zstd, oss);
  std::string comp = oss.str();
  std::istringstream iss(comp);
  std::vector<char> out(3000);
  EXPECT_THROW(decompressFromStreamToRange(iss, CompressionType::Zstd, data.size(), 9000, out.data(), 2000), std::runtime_error);
}

TEST(CompressionTest, StreamInvalidRangeLz4) {
  std::vector<char> data(10000);
  for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>((i * 5) % 256);
  std::ostringstream oss;
  compressToStream(data.data(), data.size(), CompressionType::Lz4, oss);
  std::string comp = oss.str();
  std::istringstream iss(comp);
  std::vector<char> out(3000);
  EXPECT_THROW(decompressFromStreamToRange(iss, CompressionType::Lz4, data.size(), 9000, out.data(), 2000), std::runtime_error);
}

TEST(CompressionTest, StreamInvalidRangeNone) {
  std::vector<char> data(1000);
  std::string comp(data.begin(), data.end());
  std::istringstream iss(comp);
  std::vector<char> out(100);
  EXPECT_THROW(decompressFromStreamToRange(iss, CompressionType::None, data.size(), 950, out.data(), 100), std::runtime_error);
}
