#pragma once

#include "Conf.h"

#include <fstream>
#include <stdexcept>
#include <string>

class FileUtils {
public:
  static std::string write(const std::string &fileName, char *addr,
                           memSize size);

  static void read(std::string &fileName, int64_t offset, char *addr,
                   memSize size);

  static void remove(const std::string &fileName);
};
