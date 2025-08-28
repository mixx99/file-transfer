#pragma once

#include "message.hpp"
#include "socket.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace SRV {
class Server {
  std::vector<uint8_t> file_data;
  std::vector<MESG::FileMessage> message_files;
  std::string ip;
  std::string directory;
  std::string filename;
  std::atomic<uint32_t> tcp_port;
  std::atomic<uint32_t> udp_port;
  std::atomic<bool> should_run_tcp = true;
  std::atomic<bool> should_run_udp = false;
  std::atomic<uint8_t> received_crc;
  std::thread listen_tcp_worker;
  std::thread listen_udp_worker;
  SCK::Socket client_socket;
  void save_file();
  void listen_tcp();
  void listen_udp();
  void parse_message(const std::vector<uint8_t> &data);
  void fill_file_data();
  void send_confirm_message(uint32_t packet_number);
  std::unique_ptr<MESG::BaseMessage> create_message(MESG::MESSAGE_TYPE type);
  void stop();

public:
  void run();
  void init_winsock();
  Server(std::string new_ip, uint32_t new_tcp_port, std::string new_directory)
      : ip(new_ip), tcp_port(new_tcp_port), directory(new_directory) {}
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;
  Server(Server &&) = default;
  Server &operator=(Server &&) = default;
  ~Server() {
    stop();
    if (listen_tcp_worker.joinable())
      listen_tcp_worker.join();
    if (listen_udp_worker.joinable())
      listen_udp_worker.join();
  }
};
} // namespace SRV