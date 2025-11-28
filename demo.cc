#include "BufferManager.h"
#include "Conf.h"

#include <cstring>
#include <glog/logging.h>

int main() {
  google::InitGoogleLogging("min_demo");
  FLAGS_logtostderr = 1;
  LOG(INFO) << "demo start";
  try {

    const memSize quota = 4 * kPageSize;
    Config conf{.spillDir = "./spill", .quota = quota};
    BufferManager manager(conf);
    std::vector<MmapMemoryPtr> mems;

    const memSize perBlockSize = kPageSize;
    for (int i = 0; i < quota / perBlockSize + 2; ++i) {
      auto mem = manager.accquireMemory(perBlockSize);
      for (memSize i = 0; i < mem->size(); ++i) {
        mem->address()[i] = (rand() % 26) + 'A';
      }
      mems.push_back(mem);
    }

    // Page Fault!!!!
    auto memory = mems[0];
    memSize size = memory->size();
    char *addr = memory->address();
    std::string s(addr, 100);
    LOG(INFO) << "mem content is: " << s;
    LOG(INFO) << "demo end";

  } catch (const std::exception &e) {
    LOG(ERROR) << "Encounter error: " << e.what();
    return 1;
  }

  return 0;
}
