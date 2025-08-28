#include "client.hpp"
#include "crc.hpp"
#include "log.hpp"
#include "typedef.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

#include <winsock.h>

namespace {
constexpr uint32_t TIMEOUT_IN_SECONDS = 1;
} // namespace

namespace CLN {

void Client::init_winsock() {
  WSADATA wsaData;
  int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (result != 0)
    LOG::safe_print("WSAStartup failed");
}

void Client::run() {
  init_winsock();
  listen_tcp_worker = std::thread(&Client::listen_tcp, this);

  if (listen_tcp_worker.joinable())
    listen_tcp_worker.join();
  if (send_message_worker.joinable())
    send_message_worker.join();
}

void Client::send_file() {
  send_start_message();
  std::this_thread::sleep_for(std::chrono::milliseconds(
      500)); /* Waiting for server start listening UDP. */
  send_file_data();
  send_final_message();
  stop();
}

void Client::send_start_message() {
  if (tcp_socket.get_sockfd() < 0) {
    LOG::safe_print("TCP socket is not connected.");
    stop();
    return;
  }
  MESG::StartMessage message;
  message.set_type(MESG::MESSAGE_TYPE_START);
  message.set_port(udp_port);
  message.set_filename(filename);
  std::vector<uint8_t> serialized_message = message.serialize_message();
  int sent = send(tcp_socket.get_sockfd(),
                  reinterpret_cast<const char *>(serialized_message.data()),
                  serialized_message.size(), 0);
  if (sent < 0) {
    LOG::safe_print("Failed to send start message.");
    stop();
  } else {
    LOG::safe_print("Start message sent.");
  }
}

void Client::send_file_data() {
  fill_file_data(filename);
  SCK::Socket sockfd(socket(AF_INET, SOCK_DGRAM, 0));
  if (sockfd.get_sockfd() < 0) {
    LOG::safe_print("Failed to create a udp socket.");
    stop();
    return;
  }
  struct sockaddr_in sockaddr = {0};
  std::memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(udp_port);
  sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());

  uint32_t packet_number = 0;
  auto last_send = std::chrono::steady_clock::now();
  while (should_run) {
    MESG::FileMessage message;
    message.data.resize(BUFFER_MESSAGE_SIZE);
    message.set_packet_number(packet_number);
    message.set_type(MESG::MESSAGE_TYPE_FILE);
    uint32_t offset = packet_number * BUFFER_MESSAGE_SIZE;
    if (offset >= file_data.size()) {
      LOG::safe_print("All packets sent.");
      break;
    }
    uint32_t chunk_size = BUFFER_MESSAGE_SIZE < file_data.size() - offset
                              ? BUFFER_MESSAGE_SIZE
                              : file_data.size() - offset;
    message.set_packet_number(packet_number);
    message.data.assign(file_data.begin() + offset,
                        file_data.begin() + offset + chunk_size);
    std::vector<uint8_t> serialized_message = message.serialize_message();
    int sent = sendto(sockfd.get_sockfd(),
                      reinterpret_cast<char *>(serialized_message.data()),
                      serialized_message.size(), 0,
                      (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (sent < 0) {
      LOG::safe_print("Failed to send a message");
      continue;
    }
    can_send_file.store(false);
    last_send = std::chrono::steady_clock::now();
    while (should_run && !can_send_file.load()) {
      auto now = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = now - last_send;
      if (elapsed.count() * 1000 >= delay) {
        break; // Resending a packet.
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (can_send_file.load())
      packet_number++;
  }
}

void Client::send_final_message() {
  if (tcp_socket.get_sockfd() < 0) {
    LOG::safe_print("TCP socket is not connected.");
    return;
  }
  uint8_t crc_code = CRC::get_crc(filename);
  MESG::FinalMessage message;
  message.set_type(MESG::MESSAGE_TYPE_FINAL);
  message.set_crc_code(crc_code);
  std::vector<uint8_t> serialized_message = message.serialize_message();
  int sent = send(tcp_socket.get_sockfd(),
                  reinterpret_cast<const char *>(serialized_message.data()),
                  serialized_message.size(), 0);
  if (sent < 0) {
    LOG::safe_print("Failed to send final message TCP.");
  } else {
    LOG::safe_print("Final message sent. File transfer complete.");
  }
}

void Client::stop() { should_run.store(false); }

void Client::fill_file_data(const std::string &path_to_file) {
  std::ifstream file(path_to_file, std::ios::binary | std::ios::ate);
  if (!file) {
    LOG::safe_print("Failed to open a file.");
    stop();
  }
  std::streamsize size = file.tellg();
  if (size > MAX_FILE_SIZE) {
    LOG::safe_print("File size is too large.");
    stop();
  }
  file.seekg(0, std::ios::beg);
  file_data.resize(size);
  if (!file.read(reinterpret_cast<char *>(file_data.data()), size)) {
    LOG::safe_print("Failed to read a file.");
    stop();
  }
}

void Client::parse_message(const std::vector<uint8_t> &data) {
  MESG::MESSAGE_TYPE type = MESG::get_type(data);
  std::unique_ptr<MESG::BaseMessage> message = create_message(type);
  if (message == nullptr) {
    LOG::safe_print("Failed to parse a message. Wrong type.");
    return;
  }
  message->deserialize_message(data);
  switch (type) {
  case MESG::MESSAGE_TYPE_CONFIRM: {
    auto confirm_message = dynamic_cast<MESG::ConfirmMessage *>(message.get());
    if (confirm_message->get_message_status() == MESG::MESSAGE_SUCCESS)
      can_send_file.store(true);
    else {
      LOG::safe_print("Something went wrong on the server.");
      stop();
    }
    break;
  }
  default: {
    LOG::safe_print("Failed to match a message type.");
    break;
  }
  }
}

std::unique_ptr<MESG::BaseMessage>
Client::create_message(MESG::MESSAGE_TYPE type) {
  switch (type) {
  case MESG::MESSAGE_TYPE_START:
    return std::make_unique<MESG::StartMessage>();
  case MESG::MESSAGE_TYPE_FILE:
    return std::make_unique<MESG::FileMessage>();
  case MESG::MESSAGE_TYPE_CONFIRM:
    return std::make_unique<MESG::ConfirmMessage>();
  case MESG::MESSAGE_TYPE_FINAL:
    return std::make_unique<MESG::FinalMessage>();
  default:
    return nullptr;
  }
}

void Client::listen_tcp() {
  tcp_socket = SCK::Socket((socket(AF_INET, SOCK_STREAM, 0)));
  if (tcp_socket.get_sockfd() < 0) {
    LOG::safe_print("Failed to create a tcp socket.");
    stop();
    return;
  }
  struct sockaddr_in sockaddr;
  std::memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(tcp_port);
  sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
  LOG::safe_print("Trying to connect to the server.");
  if (connect(tcp_socket.get_sockfd(), (struct sockaddr *)&sockaddr,
              sizeof(sockaddr)) < 0) {
    LOG::safe_print("Failed to connect to server.");
    stop();
    return;
  }
  LOG::safe_print("Connected to server.");
  send_message_worker = std::thread(&Client::send_file, this);
  DWORD timeout = TIMEOUT_IN_SECONDS * 1000;
  setsockopt(tcp_socket.get_sockfd(), SOL_SOCKET, SO_RCVTIMEO,
             (const char *)&timeout, sizeof timeout);
  int result = 0;
  std::vector<uint8_t> message(BUFFER_MESSAGE_SIZE);
  while (should_run || result > 0) {
    result =
        recv(tcp_socket.get_sockfd(), reinterpret_cast<char *>(message.data()),
             BUFFER_MESSAGE_SIZE, 0);
    if (result < 0) {
      int err = WSAGetLastError();
      if (err == WSAETIMEDOUT)
        continue; // Timeout.
      else {
        LOG::safe_print("Something went wrong.");
        stop();
        return;
      }
    }
    if (result == 0) {
      LOG::safe_print("Server closed a connection.");
      stop();
      return;
    }
    message.resize(result);
    parse_message(message);
    message.resize(BUFFER_MESSAGE_SIZE);
  }
}
} // namespace CLN