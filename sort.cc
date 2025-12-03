#include "BufferManager.h"
#include "MemoryUtils.h"
#include "MmapMemory.h"
#include "conf.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <execution>
#include <glog/logging.h>
#include <queue>
#include <vector>

struct Element {
  int64_t value;
  size_t arrayIdx;
  size_t elementIdx;
};

struct ElementCompare {
  bool operator()(const Element &a, const Element &b) {
    return a.value > b.value;
  }
};

int main() {
  google::InitGoogleLogging("sort");
  FLAGS_logtostderr = 1;
  BufferManager manager("./spill");
  memSize perBlocksize = kPageSize * 1024 * 1024; // 4GB
  int64_t epoch = 4;

  std::vector<MmapMemoryPtr> stores;
  std::vector<int64_t *> arrays;
  std::vector<size_t> sizes;

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
    LOG(INFO) << "Sorting " << numElements << " int64 numbers";
    // partial sort
    std::sort(std::execution::par, ptr, ptr + numElements);
    // mark this memory can be flush to disk
    manager.invalidMemory(memory);
    LOG(INFO) << "Block flushed to disk";
    stores.emplace_back(memory);
    arrays.emplace_back(ptr);
    sizes.emplace_back(numElements);
  }

  const int64_t kElementsPerPage = kPageSize / sizeof(int64_t);
  std::priority_queue<Element, std::vector<Element>, ElementCompare> minHeap;
  // init heap
  for (size_t i = 0; i < arrays.size(); ++i) {
    if (sizes[i] > 0 && arrays[i] != nullptr) {
      minHeap.emplace(
          Element{.value = arrays[i][0], .arrayIdx = i, .elementIdx = 0});
    }
  }
  while (!minHeap.empty()) {
    auto e = minHeap.top();
    minHeap.pop();
    // LOG(INFO) << e.value << " ";
    if (e.elementIdx + 1 < sizes[e.arrayIdx]) {
      minHeap.emplace(Element{.value = arrays[e.arrayIdx][e.elementIdx + 1],
                              .arrayIdx = e.arrayIdx,
                              .elementIdx = e.elementIdx + 1});

      // switch to next page, so this page is useless, invalid it
      if (e.elementIdx / kElementsPerPage !=
          (e.elementIdx + 1) / kElementsPerPage) {
        auto pageStartAddr = reinterpret_cast<char *>(arrays[e.arrayIdx]) +
                             (e.elementIdx / kElementsPerPage) * kPageSize;
        manager.invalidMemoryWithoutSave(pageStartAddr, kPageSize);
      }
    }
  }
  LOG(INFO) << "Merge complete";
  return 0;
}
