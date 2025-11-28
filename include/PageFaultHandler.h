#pragma once

#include "Buffer.h"
#include "Conf.h"
#include "MemRegions.h"
#include "MmapMemory.h"
#include "Statistics.h"
#include "Spiller.h"

#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <glog/logging.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>

class PageFaultHandler {
public:
  explicit PageFaultHandler(SpillerPtr spiller);

  ~PageFaultHandler();

  void registerMemory(MmapMemoryPtr &mem);

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
