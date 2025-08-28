#include "message.hpp"

namespace MESG {
void serialize_uint32(std::vector<uint8_t> &buffer, uint32_t &offset,
                      uint32_t number) {
  if (buffer.size() < offset + sizeof(number))
    buffer.resize(offset + sizeof(number));
  buffer[offset] = number >> 24;
  buffer[offset + 1] = number >> 16;
  buffer[offset + 2] = number >> 8;
  buffer[offset + 3] = number;
  offset += 4;
}
void serialize_str(std::vector<uint8_t> &buffer, uint32_t &offset,
                   const std::vector<uint8_t> &str) {
  if (buffer.size() < offset + str.size())
    buffer.resize(offset + str.size());
  for (int i = 0; i < str.size(); ++i) {
    buffer[offset++] = str[i];
  }
}
uint32_t deserialize_uint32(const std::vector<uint8_t> &buffer,
                            uint32_t &offset) {
  uint32_t result = 0;
  result |= (uint32_t)buffer[offset] << 24;
  result |= (uint32_t)buffer[offset + 1] << 16;
  result |= (uint32_t)buffer[offset + 2] << 8;
  result |= (uint32_t)buffer[offset + 3];
  offset += 4;
  return result;
}
std::vector<uint8_t> deserialize_str(const std::vector<uint8_t> &buffer,
                                     uint32_t &offset, uint32_t length) {
  std::vector<uint8_t> result(buffer.begin() + offset,
                              buffer.begin() + offset + length);
  offset += length;
  return result;
}

std::vector<uint8_t> FileMessage::serialize_message() const {
  std::vector<uint8_t> result;
  uint32_t offset = 0;
  serialize_uint32(result, offset, type);
  serialize_uint32(result, offset, packet_number);
  serialize_uint32(result, offset, data.size());
  serialize_str(result, offset, data);
  return result;
}
void FileMessage::deserialize_message(const std::vector<uint8_t> &buffer) {
  uint32_t offset = 0;
  type = static_cast<MESSAGE_TYPE>(deserialize_uint32(buffer, offset));
  packet_number = deserialize_uint32(buffer, offset);
  data_length = deserialize_uint32(buffer, offset);
  data = deserialize_str(buffer, offset, data_length);
}
std::vector<uint8_t> StartMessage::serialize_message() const {
  std::vector<uint8_t> result;
  uint32_t offset = 0;
  serialize_uint32(result, offset, type);
  serialize_uint32(result, offset, port);
  serialize_uint32(result, offset, filename.size());
  serialize_str(result, offset, filename);
  return result;
}
void StartMessage::deserialize_message(const std::vector<uint8_t> &buffer) {
  uint32_t offset = 0;
  type = static_cast<MESSAGE_TYPE>(deserialize_uint32(buffer, offset));
  port = deserialize_uint32(buffer, offset);
  name_length = deserialize_uint32(buffer, offset);
  filename = deserialize_str(buffer, offset, name_length);
}
std::vector<uint8_t> ConfirmMessage::serialize_message() const {
  std::vector<uint8_t> result;
  uint32_t offset = 0;
  serialize_uint32(result, offset, type);
  serialize_uint32(result, offset, packet_number);
  serialize_uint32(result, offset, status);
  return result;
}
void ConfirmMessage::deserialize_message(const std::vector<uint8_t> &buffer) {
  uint32_t offset = 0;
  type = static_cast<MESSAGE_TYPE>(deserialize_uint32(buffer, offset));
  packet_number = deserialize_uint32(buffer, offset);
  status = static_cast<MESSAGE_STATUS>(deserialize_uint32(buffer, offset));
}
std::vector<uint8_t> FinalMessage::serialize_message() const {
  uint32_t offset = 0;
  std::vector<uint8_t> result;
  serialize_uint32(result, offset, type);
  serialize_uint32(result, offset, crc_code);
  return result;
}
void FinalMessage::deserialize_message(const std::vector<uint8_t> &buffer) {
  uint32_t offset = 0;
  type = static_cast<MESSAGE_TYPE>(deserialize_uint32(buffer, offset));
  crc_code = deserialize_uint32(buffer, offset);
}
MESSAGE_TYPE get_type(std::vector<uint8_t> raw_data) {
  uint32_t dummy_offset = 0;
  MESSAGE_TYPE result =
      static_cast<MESSAGE_TYPE>(deserialize_uint32(raw_data, dummy_offset));
  return result;
}
std::string StartMessage::get_filename() const {
  std::string name;
  name.resize(filename.size());
  for (int i = 0; i < filename.size(); ++i)
    name[i] = filename[i];
  return name;
}
void StartMessage::set_filename(const std::string &name) {
  filename.resize(name.size());
  for (int i = 0; i < filename.size(); ++i)
    filename[i] = name[i];
}
} // namespace MESG