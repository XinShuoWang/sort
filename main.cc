#include "BufferManager.h"

#include <cstring>

int main() {
  try {
    BufferManager demo;

    demo.init();

    // start handler thread
    demo.startPageFaultHandlerThread();
    // waiting for handler thread start
    usleep(100000);

    // test memory access
    {
      std::cout << "TESTING MEM ACCESS" << std::endl;

      memSize size = kPageSize * 2 + 3;

      // allocate mem
      auto memory = BufferManager::accquireMemory(size);

      // write content to mem
      std::memset(memory->address(), 'x', memory->size());

      // write to disk
      demo.invalidMemory(memory);

      // register to userfaultfd
      demo.registerMemory(memory);

      char *addr = memory->address();
      std::cout << "Access mem!" << std::endl;
      // Page Fault!!!!
      std::string s(addr, size);
      std::cout << "mem content is: " << s << std::endl;
    }

    // 等待一下让处理线程完成
    sleep(1);
    demo.stop();

    std::cout << "Finish success!" << std::endl;

  } catch (const std::exception &e) {
    std::cout << "Encounter error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}