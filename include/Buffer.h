#pragma once

#include "Conf.h"

#include <memory>
#include <string>

struct Buffer {
private:
  std::string payload_;
  const memSize size_;

public:
  Buffer(memSize size) : size_(size) { payload_.resize(size); }

  char *data() { return payload_.data(); }

  memSize size() const { return size_; }
};

using BufferPtr = std::shared_ptr<Buffer>;
