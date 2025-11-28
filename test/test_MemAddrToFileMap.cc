#include "MemAddrToFileMap.h"
#include <gtest/gtest.h>

TEST(MemAddrToFileMapTest, SetGetErase) {
  MemAddrToFileMap map;
  char a;
  char *addr = &a;

  EXPECT_FALSE(map.get(addr).has_value());
  map.set(addr, "x.bin");
  EXPECT_EQ(map.get(addr).value(), "x.bin");

  {
    std::ofstream ofs("x.bin", std::ios::binary);
    ofs << "data";
  }
  map.erase(addr);
  EXPECT_FALSE(map.get(addr).has_value());
}
