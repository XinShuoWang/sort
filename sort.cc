#include "BufferManager.h"
#include "MemoryUtils.h"
#include "MmapMemory.h"
#include "conf.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

int main() {
  BufferManager manager;
  memSize perBlocksize = kPageSize * 1024 * 1024;
  int64_t epoch = 4;

  std::vector<MmapMemoryPtr> stores;

  for (int i = 0; i < epoch; ++i) {
    // allocate mem
    auto memory = BufferManager::accquireMemory(perBlocksize);
    auto actualBlockSize = memory->size();
    auto numElements = actualBlockSize / sizeof(int64_t);
    auto ptr = reinterpret_cast<int64_t *>(memory->address());

    // write content to mem
    for (memSize j = 0; j < numElements; ++j) {
      ptr[j] = rand();
    }

    std::cout << "Sorting " << numElements << " int64 numbers. Process RSS is: "
              << MemoryUtils::getProcessRss() << std::endl;

    // partial sort
    std::sort(ptr, ptr + numElements);

    std::cout << "Sort is done." << std::endl;

    // mark this memory can be flush to disk
    manager.invalidMemory(memory);

    std::cout << "After invalid, process RSS is:"
              << MemoryUtils::getProcessRss() << std::endl;

    stores.emplace_back(memory);
  }

  for (int i = 0; i < epoch; ++i) {
    auto memory = stores[i];
    auto actualBlockSize = memory->size();
    auto numElements = actualBlockSize / sizeof(int64_t);
    auto ptr = reinterpret_cast<int64_t *>(memory->address());

    for (int j = 0; j < numElements; ++j) {
      if (rand() % 1000 == 1) {
        std::cout << ptr[j] << ",";
      }
      std::cout << std::endl;
    }
  }

  return 0;
}