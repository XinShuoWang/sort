#include "BufferManager.h"
#include <glog/logging.h>

BufferManager::BufferManager(const Config &conf) {
  spiller_ = std::make_shared<Spiller>(conf.spillDir);
  pageFaultHandler_ = std::make_shared<PageFaultHandler>(spiller_);
  quotaManager_ = std::make_unique<QuotaManager>(conf.quota, spiller_);
  LOG(INFO) << "buffer manager init quota=" << conf.quota
            << " spillDir=" << conf.spillDir;
}

BufferManager::~BufferManager() {}

MmapMemoryPtr BufferManager::accquireMemory(int64_t size) {
  if (!quotaManager_->tryAcquire(size)) {
    throw std::runtime_error("quota not enough! OOM error!");
  }
  auto mem = std::make_shared<MmapMemory>(size);
  spiller_->registerMem(mem);
  pageFaultHandler_->registerMemory(mem);
  LOG(INFO) << "buffer manager acquired memory size=" << size
            << " addr=" << (uint64_t)mem->address();
  return mem;
}
