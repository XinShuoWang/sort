#include "MmapMemory.h"
#include <gtest/gtest.h>
#include <cstring>

TEST(MmapMemoryTest, AllocationAlignment) {
  memSize req = kPageSize + 123;
  MmapMemory mem(req);
  EXPECT_EQ(mem.requestSize(), req);
  EXPECT_GE(mem.size(), req);
  char *addr = mem.address();
  std::memset(addr, 0xAB, mem.size());
  EXPECT_NE(addr, nullptr);
}
