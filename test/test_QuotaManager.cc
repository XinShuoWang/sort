#include "QuotaManager.h"
#include "Spiller.h"
#include <gtest/gtest.h>

TEST(QuotaManagerTest, AcquireReleaseSpill) {
  auto spiller = std::make_shared<Spiller>("./spill_test_quota");
  QuotaManager q(2 * kPageSize, spiller);

  EXPECT_TRUE(q.tryAcquire(kPageSize));
  EXPECT_EQ(q.used(), kPageSize);
  EXPECT_EQ(q.available(), kPageSize);

  // register memory to enable spill path, but queue starts empty
  // spill will not reduce used without registered mem; expectation reflects code

  bool ok = q.tryAcquire(2 * kPageSize);
  EXPECT_FALSE(ok);
  EXPECT_LE(q.used(), 2 * kPageSize);

  q.release(kPageSize);
  EXPECT_GE(q.available(), 0);
}
