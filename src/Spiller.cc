#include "Spiller.h"
#include "DirectoryUtils.h"
#include "FileUtils.h"

#include <atomic>
#include <glog/logging.h>
#include <sys/mman.h>

Spiller::Spiller(const std::string &path) : spillPath_(path) {
  DirectoryUtils::createDir(spillPath_);
  LOG(INFO) << "spiller init path=" << spillPath_;
}

Spiller::~Spiller() {
  LOG(INFO) << "spiller cleanup path=" << spillPath_;
  DirectoryUtils::removeAll(spillPath_);
}

void Spiller::recoverMem(char *startAddr, int64_t offset, char *dst,
                         memSize size) {
  auto fileNameOpt = addrToFileMap_.get(startAddr);
  if (!fileNameOpt) {
    throw std::runtime_error("Can't find file name mapping for address: " +
                             std::to_string((uint64_t)startAddr));
  }
  FileUtils::read(*fileNameOpt, offset, dst, size);
}

void Spiller::registerMem(MmapMemoryPtr &mem) { queue_.push(mem); }

memSize Spiller::spill(memSize targetSize) {
  memSize spilledSize = 0;
  auto elementSize = queue_.size();
  while (elementSize > 0 && spilledSize < targetSize) {
    auto mem = queue_.front();
    queue_.pop();
    if (mem.use_count() == 1) {
      LOG(INFO) << "<Release> address=" << (uint64_t)mem->address()
                << " size=" << mem->size() << " use_count=" << mem.use_count();
      spilledSize += mem->size();
      addrToFileMap_.erase(mem->address());
    } else {
      LOG(INFO) << "<Spill> mem address=" << (uint64_t)mem->address()
                << " size=" << mem->size() << " use_count=" << mem.use_count();
      spilledSize += mem->size();
      queue_.push(mem);
      eraseMem(mem);
    }
    elementSize--;
  }
  LOG(INFO) << "spiller spill done target=" << targetSize
            << " spilled=" << spilledSize;
  return spilledSize;
}

void Spiller::eraseMem(MmapMemoryPtr &mem) {
  char *addr = mem->address();
  memSize size = mem->size();
  if (addrToFileMap_.get(addr).has_value()) {
    return;
  }
  std::string fileName = FileUtils::write(nextFileName(), addr, size);
  addrToFileMap_.set(addr, fileName);
  madvise(addr, size, MADV_DONTNEED);
}

std::string Spiller::nextFileName() {
  static std::atomic<int64_t> fileId_{0};
  static const std::string kFileSuffix = ".bin";
  int64_t id = fileId_.fetch_add(1);
  return spillPath_ + "/" + std::to_string(id) + kFileSuffix;
}
