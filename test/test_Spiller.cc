#include "Spiller.h"
#include "MmapMemory.h"
#include "DirectoryUtils.h"
#include <gtest/gtest.h>
#include <cstring>
#include <filesystem>

TEST(SpillerTest, RegisterSpillRecover) {
  std::filesystem::path dir = "./spill_test";
  {
    Spiller s(dir.string(), CompressionType::Zstd);
    auto mem = std::make_shared<MmapMemory>(kPageSize);
    char *addr = mem->address();
    std::memset(addr, 0xCD, mem->size());
    s.registerMem(mem);

    s.spill(mem->size());

    auto page = std::unique_ptr<char[]>(new char[kPageSize]);
    s.recoverMem(addr, 0, page.get(), kPageSize);
    EXPECT_EQ(page.get()[0], (char)0xCD);
  }
  EXPECT_FALSE(DirectoryUtils::exists(dir));
}
