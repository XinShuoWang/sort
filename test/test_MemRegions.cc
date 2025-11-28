#include "MemRegions.h"
#include <gtest/gtest.h>

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
