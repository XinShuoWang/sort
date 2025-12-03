#pragma once

#include "Conf.h"
#include "Spiller.h"

#include <mutex>

class QuotaManager {
public:
  QuotaManager(memSize size, SpillerPtr &spiller);
  ~QuotaManager();

  QuotaManager(const QuotaManager &) = delete;
  QuotaManager(QuotaManager &&) = delete;
  QuotaManager &operator=(const QuotaManager &) = delete;
  QuotaManager &operator=(QuotaManager &&) = delete;

  bool tryAcquire(memSize size);

  void release(memSize size);

  memSize used();

  memSize available();

private:
  std::mutex mutex_;
  const memSize size_;
  memSize used_;
  SpillerPtr spiller_;
};
