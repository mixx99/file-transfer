#include "client.hpp"
#include <cstdint>
#include <iostream>

int main(int argc, char **argv) {
  if (argc < 6 || argc > 6) {
    std::cout << "Invalid argument" << std::endl;
    return EXIT_FAILURE;
  }
  std::string ip, filename;
  uint32_t tcp_port = 0, udp_port = 0;
  double delay = 0;
  ip = argv[1];
  tcp_port = std::stoi(argv[2]);
  udp_port = std::stoi(argv[3]);
  filename = argv[4];
  delay = std::stoi(argv[5]);
  CLN::Client client(ip, tcp_port, udp_port, filename, delay);
  client.run();
}