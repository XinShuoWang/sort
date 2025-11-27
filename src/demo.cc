#include "BufferManager.h"
#include "conf.h"

#include <cstring>
#include <glog/logging.h>

int main() {
  google::InitGoogleLogging("min_demo");
  FLAGS_logtostderr = 1;
  try {
    BufferManager manager("./spill");

    memSize size = kPageSize * 2 + 3;

    // allocate mem
    auto memory = BufferManager::accquireMemory(size);
    size = memory->size();

    // write content to mem
    for (memSize i = 0; i < memory->size(); ++i) {
      memory->address()[i] = (rand() % 26) + 'A';
    }

    // mark this memory can be flush to disk
    manager.invalidMemory(memory);

    // Page Fault!!!!
    char *addr = memory->address();
    std::string s(addr + 9000, size - 9000);
    LOG(INFO) << "mem content is: " << s;

  } catch (const std::exception &e) {
    LOG(ERROR) << "Encounter error: " << e.what();
    return 1;
  }

  return 0;
}
