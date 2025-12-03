#include "BufferManager.h"
#include <glog/logging.h>

BufferManager::BufferManager(const Config &conf) {
  spiller_ = std::make_shared<Spiller>(conf.spillDir);
  pageFaultHandler_ = std::make_shared<PageFaultHandler>(spiller_);
  quotaManager_ = std::make_unique<QuotaManager>(conf.quota, spiller_);
}

BufferManager::~BufferManager() {}

MmapMemoryPtr BufferManager::accquireMemory(int64_t size) {
  if (!quotaManager_->tryAcquire(size)) {
    throw std::runtime_error("quota not enough! OOM error!");
  }
  auto mem = std::shared_ptr<MmapMemory>(
      new MmapMemory(size), [this](MmapMemory *mem) {
        pageFaultHandler_->unregisterMemory(mem->address(), mem->size());
        delete mem;
      });
  // auto mem = std::make_shared<MmapMemory>(size);
  spiller_->registerMem(mem);
  pageFaultHandler_->registerMemory(mem);
  return mem;
}
