#include "BufferManager.h"
#include "conf.h"

#include <cstring>

int main() {
  try {
    BufferManager manager;

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
    std::string s(addr, size);
    std::cout << "mem content is: " << s << std::endl;

  } catch (const std::exception &e) {
    std::cout << "Encounter error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}