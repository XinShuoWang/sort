#include "BufferManager.h"
#include "PageFaultHandler.h"
#include "Spiller.h"
#include <gtest/gtest.h>
#include <cstring>

TEST(PageFaultHandlerTest, RegisterAndFault) {
  auto spiller = std::make_shared<Spiller>("./spill_pf", CompressionType::Zstd);
  PageFaultHandler handler(spiller);
  auto mem = std::make_shared<MmapMemory>(kPageSize);
  char *addr = mem->address();
  for (memSize i = 0; i < mem->size(); ++i) addr[i] = (char)(i % 251);
  spiller->registerMem(mem);
  spiller->spill(mem->size());

  handler.registerMemory(mem);
  volatile char x = addr[0];
  (void)x;
  handler.unregisterMemory(mem->address(), mem->size());
}
