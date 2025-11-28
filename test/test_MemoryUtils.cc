#include "MemoryUtils.h"
#include <gtest/gtest.h>

TEST(MemoryUtilsTest, GetProcessRss) {
  auto rss = MemoryUtils::getProcessRss();
  EXPECT_GE(rss, 0);
}
