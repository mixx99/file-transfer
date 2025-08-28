#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace MESG {
enum MESSAGE_TYPE : uint32_t {
  MESSAGE_TYPE_START,   // Info about file.
  MESSAGE_TYPE_FILE,    // Binary file data.
  MESSAGE_TYPE_CONFIRM, // Confirm receiving packet.
  MESSAGE_TYPE_FINAL,   // Final message.
};
enum MESSAGE_STATUS : uint32_t {
  MESSAGE_SUCCESS,
  MESSAGE_FAILURE,
};

void serialize_uint32(std::vector<uint8_t> &buffer, uint32_t &offset,
                      uint32_t number);
void serialize_str(std::vector<uint8_t> &buffer, uint32_t &offset,
                   const std::vector<uint8_t> &str);
uint32_t deserialize_uint32(const std::vector<uint8_t> &buffer,
                            uint32_t &offset);
std::vector<uint8_t> deserialize_str(const std::vector<uint8_t> &buffer,
                                     uint32_t &offset, uint32_t length);

MESSAGE_TYPE get_type(std::vector<uint8_t> raw_data);

class BaseMessage {
protected:
  MESSAGE_TYPE type;

public:
  virtual ~BaseMessage() = default;
  virtual std::vector<uint8_t> serialize_message() const = 0;
  virtual void deserialize_message(const std::vector<uint8_t> &buffer) = 0;
  MESSAGE_TYPE get_type() const noexcept { return type; }
  void set_type(MESSAGE_TYPE m_type) noexcept { type = m_type; }
};
class FileMessage : public BaseMessage {
  uint32_t packet_number;
  uint32_t data_length;

public:
  std::vector<uint8_t> data;
  std::vector<uint8_t> serialize_message() const override;
  void deserialize_message(const std::vector<uint8_t> &buffer) override;
  uint32_t get_packet_number() const noexcept { return packet_number; }
  void set_packet_number(uint32_t new_packet_number) noexcept {
    packet_number = new_packet_number;
  }
};
class StartMessage : public BaseMessage {
  uint32_t port;
  uint32_t name_length;
  std::vector<uint8_t> filename;

public:
  std::vector<uint8_t> serialize_message() const override;
  void deserialize_message(const std::vector<uint8_t> &buffer) override;
  uint32_t get_port() const noexcept { return port; }
  void set_port(uint32_t new_port) noexcept { port = new_port; }
  std::string get_filename() const;
  void set_filename(const std::string &name);
};
class ConfirmMessage : public BaseMessage {
  uint32_t packet_number;
  MESSAGE_STATUS status;

public:
  std::vector<uint8_t> serialize_message() const override;
  void deserialize_message(const std::vector<uint8_t> &buffer) override;
  uint32_t get_packet_number() const noexcept { return packet_number; }
  void set_packet_number(uint32_t new_packet_number) noexcept {
    packet_number = new_packet_number;
  }
  MESSAGE_STATUS get_message_status() const noexcept { return status; }
  void set_message_status(MESSAGE_STATUS new_status) { status = new_status; }
};
class FinalMessage : public BaseMessage {
  uint32_t crc_code;

public:
  std::vector<uint8_t> serialize_message() const override;
  void deserialize_message(const std::vector<uint8_t> &buffer) override;
  uint32_t get_crc_code() const noexcept { return crc_code; }
  void set_crc_code(uint32_t new_crc_code) noexcept { crc_code = new_crc_code; }
};
} // namespace MESG