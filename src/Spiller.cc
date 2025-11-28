#include "Spiller.h"
#include "DirectoryUtils.h"
#include "FileUtils.h"

#include <glog/logging.h>
#include <sys/mman.h>

Spiller::Spiller(const std::string &path) : spillPath_(path) {
  fileId_ = 0;
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
  LOG(INFO) << "spiller recover start_addr=" << (uint64_t)startAddr
            << " offset=" << offset << " size=" << size;
  FileUtils::read(*fileNameOpt, offset, dst, size);
}

void Spiller::registerMem(MmapMemoryPtr &mem) {
  allMems_.push(mem);
  LOG(INFO) << "spiller register addr=" << (uint64_t)mem->address()
            << " size=" << mem->size();
}

memSize Spiller::spill(memSize targetSize) {
  memSize spilledSize = 0;
  auto elementSize = allMems_.size();
  LOG(INFO) << "spiller spill start target=" << targetSize
            << " queue_size=" << elementSize;
  while (elementSize > 0 && spilledSize < targetSize) {
    auto mem = allMems_.front();
    allMems_.pop();
    if (mem.use_count() == 1) {
      addrToFileMap_.erase(mem->address());
      continue;
    } else {
      allMems_.push(mem);
    }
    spilledSize += mem->size();
    eraseMem(mem);
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
  LOG(INFO) << "spiller erase addr=" << (uint64_t)addr << " size=" << size
            << " file=" << fileName;
  madvise(addr, size, MADV_DONTNEED);
}

std::string Spiller::nextFileName() {
  int64_t id = fileId_.fetch_add(1);
  return spillPath_ + "/" + std::to_string(id) + kFileSuffix;
}
