#include "MemRegions.h"
#include <gtest/gtest.h>
#include <stdexcept>

TEST(MemRegionsTest, AddFindRemove) {
  MemRegions regions;
  char buffer[1024];
  regions.add(buffer, 512);
  regions.add(buffer + 512, 256);

  EXPECT_EQ(regions.findStart(buffer + 400), buffer);
  EXPECT_EQ(regions.findStart(buffer + 600), buffer + 512);

  EXPECT_TRUE(regions.remove(buffer));
  EXPECT_FALSE(regions.remove(buffer));
}

TEST(MemRegionsTest, UnsortedInsertFind) {
  MemRegions regions;
  char buffer[1024];
  regions.add(buffer + 300, 50);
  regions.add(buffer, 100);
  regions.add(buffer + 500, 200);

  EXPECT_EQ(regions.findStart(buffer + 20), buffer);
  EXPECT_EQ(regions.findStart(buffer + 320), buffer + 300);
  EXPECT_EQ(regions.findStart(buffer + 600), buffer + 500);
}

TEST(MemRegionsTest, BoundaryAndException) {
  MemRegions regions;
  char buffer[256];
  regions.add(buffer, 128);

  EXPECT_EQ(regions.findStart(buffer), buffer);
  EXPECT_EQ(regions.findStart(buffer + 127), buffer);
  EXPECT_THROW(regions.findStart(buffer + 128), std::runtime_error);
  EXPECT_THROW(regions.findStart(buffer + 200), std::runtime_error);
}

TEST(MemRegionsTest, RemoveSemantics) {
  MemRegions regions;
  char buffer[1024];
  regions.add(buffer, 256);
  regions.add(buffer + 300, 100);

  EXPECT_TRUE(regions.remove(buffer));
  EXPECT_FALSE(regions.remove(buffer));
  EXPECT_THROW(regions.findStart(buffer + 10), std::runtime_error);

  EXPECT_EQ(regions.findStart(buffer + 350), buffer + 300);
  EXPECT_TRUE(regions.remove(buffer + 300));
  EXPECT_THROW(regions.findStart(buffer + 350), std::runtime_error);
}

TEST(MemRegionsTest, RemoveWithInnerAddress) {
  MemRegions regions;
  char buffer[1024];
  regions.add(buffer + 128, 128);
  EXPECT_EQ(regions.findStart(buffer + 200), buffer + 128);
  EXPECT_TRUE(regions.remove(buffer + 200));
  EXPECT_THROW(regions.findStart(buffer + 200), std::runtime_error);
}

TEST(MemRegionsTest, ManyRegionsCorrectness) {
  MemRegions regions;
  char buffer[1024];
  for (int i = 0; i < 256; ++i) {
    regions.add(buffer + i * 4, 4);
  }
  for (int i = 0; i < 256; ++i) {
    char* addr = buffer + i * 4 + 2;
    EXPECT_EQ(regions.findStart(addr), buffer + i * 4);
  }
  EXPECT_THROW(regions.findStart(buffer + 1024), std::runtime_error);
}
