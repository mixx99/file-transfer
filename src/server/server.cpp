#include "server.hpp"
#include "crc.hpp"
#include "log.hpp"
#include "message.hpp"
#include "socket.hpp"
#include "typedef.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
#include <winsock.h>

namespace {
constexpr int TIMEOUT_IN_SECONDS = 1;
constexpr uint32_t RECEIVE_FILE_SIZE =
    sizeof(MESG::FileMessage) + BUFFER_MESSAGE_SIZE;
std::atomic<bool> should_run = 1;
} // namespace

namespace SRV {

void Server::init_winsock() {
  WSADATA wsaData;
  int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (result != 0)
    LOG::safe_print("WSAStartup failed");
}

void Server::run() {
  init_winsock();
  listen_tcp_worker = std::thread(&Server::listen_tcp, this);
  if (listen_tcp_worker.joinable())
    listen_tcp_worker.join();
  if (listen_udp_worker.joinable())
    listen_udp_worker.join();
  stop();
  if (!should_run)
    return;
  fill_file_data();
  save_file();
  uint8_t crc_result = CRC::get_crc(directory + "/" + filename);
  if (crc_result != received_crc)
    LOG::safe_print("Something went wrong with file. CRC code isn't correct");
  else
    LOG::safe_print("File was downloaded successfully!");
}

void Server::stop() {
  should_run_tcp.store(false);
  should_run_udp.store(false);
}

void Server::listen_tcp() {
  SCK::Socket tcp_socket(socket(AF_INET, SOCK_STREAM, 0));
  if (tcp_socket.get_sockfd() < 0) {
    LOG::safe_print("Failed to create a tcp socket.");
    stop();
    should_run.store(0);
    return;
  }
  struct sockaddr_in sockaddr;
  std::memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(tcp_port);
  sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());

  if (bind(tcp_socket.get_sockfd(), (struct sockaddr *)&sockaddr,
           sizeof(sockaddr)) < 0) {
    LOG::safe_print("Failed to bind a socket.");
    stop();
    should_run.store(0);
    return;
  }
  if (listen(tcp_socket.get_sockfd(), 10) < 0) {
    LOG::safe_print("failed to listen a socket.");
    stop();
    should_run.store(0);
    return;
  }
  LOG::safe_print("Waiting for client.");
  client_socket =
      SCK::Socket((accept(tcp_socket.get_sockfd(), nullptr, nullptr)));
  if (client_socket.get_sockfd() < 0) {
    LOG::safe_print("Failed to accept client socket.");
    stop();
    should_run.store(0);
    return;
  }
  LOG::safe_print("Client connected.");
  DWORD timeout = TIMEOUT_IN_SECONDS * 1000;
  setsockopt(client_socket.get_sockfd(), SOL_SOCKET, SO_RCVTIMEO,
             (const char *)&timeout, sizeof timeout);
  int result = 0;
  std::vector<uint8_t> message(BUFFER_MESSAGE_SIZE);
  while (should_run_tcp || result > 0) {
    result =
        recv(client_socket.get_sockfd(),
             reinterpret_cast<char *>(message.data()), BUFFER_MESSAGE_SIZE, 0);
    if (result < 0) {
      int err = WSAGetLastError();
      if (err == WSAETIMEDOUT)
        continue; // Timeout.
      else {
        LOG::safe_print("Something went wrong.");
        stop();
        should_run.store(0);
        return;
      }
    }
    if (result == 0) {
      LOG::safe_print("Client closed a connection.");
      stop();
      return;
    }
    message.resize(result);
    parse_message(message);
    message.resize(BUFFER_MESSAGE_SIZE);
  }
}
void Server::listen_udp() {
  SCK::Socket udp_socket(socket(AF_INET, SOCK_DGRAM, 0));
  if (udp_socket.get_sockfd() < 0) {
    LOG::safe_print("Failed to create a udp socket.");
    stop();
    should_run.store(0);
    return;
  }
  DWORD timeout = TIMEOUT_IN_SECONDS * 1000;
  setsockopt(udp_socket.get_sockfd(), SOL_SOCKET, SO_RCVTIMEO,
             (const char *)&timeout, sizeof timeout);
  struct sockaddr_in sockaddr;
  std::memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(udp_port);
  sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
  if (bind(udp_socket.get_sockfd(), (struct sockaddr *)&sockaddr,
           sizeof(sockaddr)) < 0) {
    LOG::safe_print("Failed to bind a socket to listen other users");
    stop();
    should_run.store(0);
    return;
  }
  int result = 0;
  std::vector<uint8_t> message(RECEIVE_FILE_SIZE);
  while (should_run_udp || result > 0) {
    result =
        recv(udp_socket.get_sockfd(), reinterpret_cast<char *>(message.data()),
             RECEIVE_FILE_SIZE, 0);
    if (result <= 0) {
      int err = WSAGetLastError();
      if (err == WSAETIMEDOUT)
        continue; // Timeout.
      else {
        LOG::safe_print("Something went wrong. " + std::to_string(err));
        stop();
        should_run.store(0);
        return;
      }
    }
    parse_message(message);
  }
}

void Server::save_file() {
  if (filename.empty()) {
    LOG::safe_print("No filename specified. Can't save file.");
    return;
  }
  if (file_data.empty()) {
    LOG::safe_print("No data to write.");
    return;
  }
  std::ofstream myfile;
  system(std::string("mkdir " + directory).c_str());
  myfile.open(directory + "\\" + filename, std::ios::binary | std::ios::out);
  if (!myfile) {
    LOG::safe_print("Failed to create a file: " + directory + "\\" + filename);
    return;
  }
  myfile.write(reinterpret_cast<char *>(file_data.data()), file_data.size());
  myfile.close();
  LOG::safe_print("File save: " + directory + "\\" + filename);
}

std::unique_ptr<MESG::BaseMessage>
Server::create_message(MESG::MESSAGE_TYPE type) {
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

void Server::send_confirm_message(uint32_t packet_number) {
  if (client_socket.get_sockfd() < 0) {
    LOG::safe_print(
        "Failed to send confirm message to client. Socket is unknown.");
    return;
  }
  MESG::ConfirmMessage confirm_msg;
  confirm_msg.set_packet_number(packet_number);
  confirm_msg.set_type(MESG::MESSAGE_TYPE_CONFIRM);
  confirm_msg.set_message_status(MESG::MESSAGE_SUCCESS);
  std::vector<uint8_t> raw = confirm_msg.serialize_message();

  int sent = send(client_socket.get_sockfd(),
                  reinterpret_cast<const char *>(raw.data()),
                  static_cast<int>(raw.size()), 0);

  if (sent < 0) {
    LOG::safe_print("Failed to send confirm message.");
  }
}

void Server::fill_file_data() {
  std::sort(message_files.begin(), message_files.end(),
            [](const MESG::FileMessage &m1, const MESG::FileMessage &m2) {
              return m1.get_packet_number() < m2.get_packet_number();
            });
  file_data.clear();
  for (int i = 0; i < message_files.size() - 1; ++i) {
    if (message_files[i].get_packet_number() ==
        message_files[i + 1].get_packet_number()) {
      message_files.erase(message_files.begin() + i + 1);
      i--;
    }
  }
  for (int i = 0; i < message_files.size(); ++i) {
    for (int j = 0; j < message_files[i].data.size(); ++j) {
      file_data.push_back(message_files[i].data[j]);
    }
  }
}

void Server::parse_message(const std::vector<uint8_t> &data) {
  MESG::MESSAGE_TYPE type = MESG::get_type(data);
  std::unique_ptr<MESG::BaseMessage> message = create_message(type);
  if (message == nullptr) {
    LOG::safe_print("Failed to parse a message. Wrong type.");
    return;
  }
  message->deserialize_message(data);
  switch (type) {
  case MESG::MESSAGE_TYPE_START: {
    auto start_message = dynamic_cast<MESG::StartMessage *>(message.get());
    if (start_message == nullptr)
      return;
    udp_port = start_message->get_port();
    should_run_udp = true;
    filename = start_message->get_filename();
    listen_udp_worker = std::thread(&Server::listen_udp, this);
    LOG::safe_print("Starting receiving the file:" +
                    start_message->get_filename());
    break;
  }
  case MESG::MESSAGE_TYPE_FILE: {
    auto file_message = dynamic_cast<MESG::FileMessage *>(message.get());
    if (file_message == nullptr)
      return;
    message_files.push_back(*file_message);
    send_confirm_message(file_message->get_packet_number());
    break;
  }
  case MESG::MESSAGE_TYPE_FINAL: {
    auto final_message = dynamic_cast<MESG::FinalMessage *>(message.get());
    received_crc.store(final_message->get_crc_code());
    LOG::safe_print("File downloaded.");
    stop();
    break;
  }
  default: {
    LOG::safe_print("Failed to match a message type.");
    break;
  }
  }
}
} // namespace SRV