#pragma once

#include "MmapMemory.h"
#include "PageFaultHandler.h"
#include "QuotaManager.h"
#include <glog/logging.h>


class BufferManager {
public:
  BufferManager(const std::string &spillPath, const memSize quotaSize) {
    quotaManager_ = std::make_unique<QuotaManager>(quotaSize);
    handler_ = std::make_unique<PageFaultHandler>(spillPath);
    handler_->start();
  }

  BufferManager(const BufferManager &) = delete;
  BufferManager(BufferManager &&) = delete;
  BufferManager &operator=(const BufferManager &) = delete;
  BufferManager &operator=(BufferManager &&) = delete;

  ~BufferManager() {
    if (handler_) {
      handler_->stop();
    }
  }

  void invalidMemory(MmapMemoryPtr &mem) {
    quotaManager_->release(mem->size());
    handler_->invalidMemory(mem);
  }

  void invalidMemoryWithoutSave(char *addr, memSize size) {
    quotaManager_->release(size);
    handler_->invalidMemoryWithoutSave(addr, size);
  }

  static MmapMemoryPtr accquireMemory(int64_t size) {
    // if (!quotaManager_->tryAcquire(size)) {
    //   handler_->work();
    // }
    return std::make_shared<MmapMemory>(size);
  }

private:
  std::unique_ptr<QuotaManager> quotaManager_;
  std::unique_ptr<PageFaultHandler> handler_;
};
