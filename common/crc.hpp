#pragma once

#include <cstdint>
#include <string>

namespace CRC {
uint8_t get_crc(const std::string &path_to_file);
}