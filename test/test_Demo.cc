#include "BufferManager.h"
#include "Conf.h"
#include <gtest/gtest.h>
#include <vector>

TEST(DemoScenarioTest, PageFaultCountIsOne) {
  const memSize quota = 4 * kPageSize;
  Config conf{.spillDir = "./spill", .quota = quota, .compressionType = CompressionType::Zstd};
  BufferManager manager(conf);

  std::vector<MmapMemoryPtr> mems;
  const memSize perBlockSize = kPageSize;
  for (int i = 0; i < quota / perBlockSize + 2; ++i) {
    auto mem = manager.accquireMemory(perBlockSize);
    for (memSize j = 0; j < mem->size(); ++j) {
      mem->address()[j] = static_cast<char>((j % 26) + 'A');
    }
    mems.push_back(mem);
  }

  auto memory = mems[0];
  char *addr = memory->address();
  volatile char x = addr[0];
  (void)x;
  volatile char y = addr[1];
  (void)y;
  auto s = manager.pageFaultStats();
  EXPECT_EQ(s.pageFaultCount, 1);
}
