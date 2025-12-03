#pragma once

#include "Conf.h"
#include "MmapMemory.h"
#include "PageFaultHandler.h"
#include "QuotaManager.h"
#include "Spiller.h"

class BufferManager {
public:
  BufferManager(const Config &conf);

  ~BufferManager();

  BufferManager(const BufferManager &) = delete;
  BufferManager(BufferManager &&) = delete;
  BufferManager &operator=(const BufferManager &) = delete;
  BufferManager &operator=(BufferManager &&) = delete;

  MmapMemoryPtr accquireMemory(int64_t size);

  Statistics pageFaultStats() const;

private:
  SpillerPtr spiller_;
  std::unique_ptr<QuotaManager> quotaManager_;
  PageFaultHandlerPtr pageFaultHandler_;
};
