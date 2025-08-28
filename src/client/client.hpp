#pragma once

#include "message.hpp"
#include "socket.hpp"

#include <atomic>
#include <string>
#include <thread>

namespace CLN {
class Client {
  std::vector<uint8_t> file_data;
  std::atomic<uint32_t> tcp_port;
  std::atomic<uint32_t> udp_port;
  std::atomic<bool> should_run = true;
  std::atomic<bool> can_send_file = true;
  std::atomic<uint32_t> delay; // Miliseconds.
  std::thread send_message_worker;
  std::thread listen_tcp_worker;
  SCK::Socket tcp_socket;
  std::string ip;
  std::string filename;

  void init_winsock();
  void fill_file_data(const std::string &path_to_file);
  void listen_tcp();
  void parse_message(const std::vector<uint8_t> &data);
  void stop();
  void send_file();
  void send_start_message(); // TCP
  void send_file_data();     // UDP
  void send_final_message(); // TCP
  std::unique_ptr<MESG::BaseMessage> create_message(MESG::MESSAGE_TYPE type);

public:
  void run();
  Client(std::string ip, uint32_t tcp_port, uint32_t udp_port,
         std::string filename, uint32_t delay)
      : ip(ip), tcp_port(tcp_port), udp_port(udp_port), filename(filename),
        delay(delay) {}
  ~Client() {
    if (listen_tcp_worker.joinable())
      listen_tcp_worker.join();
    if (send_message_worker.joinable())
      send_message_worker.join();
  }
};
} // namespace CLN