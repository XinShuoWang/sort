#pragma once

#include "Buffer.h"
#include "Conf.h"
#include "MemRegions.h"
#include "MmapMemory.h"
#include "Spiller.h"
#include "Statistics.h"

#include <thread>

class PageFaultHandler {
public:
  explicit PageFaultHandler(SpillerPtr spiller);

  ~PageFaultHandler();

  void registerMemory(MmapMemoryPtr &mem);

  bool unregisterMemory(char *addr, memSize size);

  Statistics stats() const;

private:
  void loop();
  void handleEvent();

private:
  int userFaultFd_, stopEventFd_;
  std::thread handlerThread_;
  MemRegions regions_;
  SpillerPtr spiller_;
  Statistics stats_;
  BufferPtr buffer_;
};
using PageFaultHandlerPtr = std::shared_ptr<PageFaultHandler>;
