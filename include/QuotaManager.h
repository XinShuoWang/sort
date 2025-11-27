#pragma once

#include "conf.h"

class QuotaManager {
public:
    QuotaManager(memSize size):size_(size), used_(0) {}
    ~QuotaManager() {}

        QuotaManager(const QuotaManager &) = delete;
    QuotaManager(QuotaManager &&) = delete;
    QuotaManager &operator=(const QuotaManager &) = delete;
    QuotaManager &operator=(QuotaManager &&) = delete;

    bool tryAcquire(memSize size) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (used_ + size > size_) {
            return false;
        }
        used_ += size;
        return true;
    }

    void release(memSize size) {
        std::lock_guard<std::mutex> lock(mutex_);
        used_ -= size;
    }

    private:
    std::mutex mutex_;
    const memSize size_;
    memSize used_;
};