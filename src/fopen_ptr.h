#pragma once

#include <cstdio>
#include <memory>

namespace misc {

auto fopen_ptr(const char* filename, const char* mode) {
  return std::unique_ptr<FILE, decltype(&std::fclose)>{
      std::fopen(filename, mode), &std::fclose};
}

}  // namespace misc
