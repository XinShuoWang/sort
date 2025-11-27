#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Record {
public:
  Record() {}
  ~Record() {}

  Record(const Record &) = delete;
  Record(Record &&) = delete;
  Record &operator=(const Record &) = delete;
  Record &operator=(Record &&) = delete;

  int64_t size() { return size_; }

  void serialize(char *dst) { int64_t cols = data_.size();
    
  }

  void deserialize() {}

private:
  std::vector<std::string> data_;
  int64_t size_;
};

class RandomRecordGenerator {
public:
  RandomRecordGenerator(int64_t size) : size_(size), count_(0) {}
  ~RandomRecordGenerator() {}

  bool hasNext() { return count_ < size_; }

  int64_t next() { return rand(); }

private:
  int64_t size_, count_;
};
