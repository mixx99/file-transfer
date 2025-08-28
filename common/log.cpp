#include "log.hpp"

#include <iostream>
#include <mutex>

namespace {
std::mutex print_mutex;
}

namespace LOG {
void safe_print(const std::string &str) {
  const std::lock_guard<std::mutex> lock(print_mutex);
  std::cout << str << std::endl;
}
} // namespace LOG