#include <iostream>
#include <sys/mman.h>
#include <cstring>

constexpr int kPageSize = 4096;

void visitMem(const char* mem, const int size) {
  for(int i = 1; i <= kPageSize; ++i) {
    std::cout << (int)mem[i] << ",";
    if (i % 256 == 0) {
      std::cout << std::endl;
    }
  }
}

int main() {
    char* memory = static_cast<char*>(mmap(nullptr, kPageSize, 
                                         PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    
    // 写入数据
    strcpy(memory, "Hello, World!");
    std::cout << "写入后: " << std::endl;
    visitMem(memory, kPageSize);
    
    // 使用 MADV_DONTNEED
    madvise(memory, kPageSize, MADV_DONTNEED);
    
    // 再次读取 - 数据已丢失！
    std::cout << "MADV_DONTNEED 后: " << std::endl;
    // 输出可能是乱码或空字符串
    visitMem(memory, kPageSize);
    
    munmap(memory, kPageSize);
    return 0;
}