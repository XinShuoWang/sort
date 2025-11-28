#include "BufferManager.h"
#include "Conf.h"
#include <gtest/gtest.h>
#include <cstring>

TEST(BufferManagerTest, AcquireMemory) {
  Config conf{.spillDir = "./spill_bufmgr", .quota = 2 * kPageSize};
  BufferManager mgr(conf);
  auto m1 = mgr.accquireMemory(kPageSize);
  auto m2 = mgr.accquireMemory(kPageSize);
  EXPECT_EQ(m1->size(), m2->size());
  for (memSize i = 0; i < m1->size(); ++i) m1->address()[i] = (char)(i % 127);
  std::string s(m1->address(), 16);
  EXPECT_FALSE(s.empty());
}
