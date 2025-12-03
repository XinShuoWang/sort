#include "QuotaManager.h"
#include <glog/logging.h>

QuotaManager::QuotaManager(memSize size, SpillerPtr &spiller)
    : size_(size), used_(0), spiller_(spiller) {}

QuotaManager::~QuotaManager() {}

bool QuotaManager::tryAcquire(memSize size) {
  std::lock_guard<std::mutex> lock(mutex_);
  int tryTimes = 3;
  do {
    if (used_ + size <= size_) {
      used_ += size;
      return true;
    }
    auto spilled = spiller_->spill(used_ + size - size_);
    used_ -= spilled;
  } while (--tryTimes > 0);
  LOG(ERROR) << "quota acquire failed size=" << size << " used=" << used_
             << " total=" << size_;
  return false;
}

void QuotaManager::release(memSize size) {
  std::lock_guard<std::mutex> lock(mutex_);
  used_ -= size;
}

memSize QuotaManager::used() {
  std::lock_guard<std::mutex> lock(mutex_);
  return used_;
}

memSize QuotaManager::available() {
  std::lock_guard<std::mutex> lock(mutex_);
  return size_ - used_;
}
