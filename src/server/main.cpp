#include "server.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv) {
  std::string ip, directory;
  int port_number = 0;
  if (argc < 4 || argc > 4) {
    std::cerr << "Invalid argument." << std::endl;
    return EXIT_FAILURE;
  }
  ip = argv[1];
  port_number = std::stoi(argv[2]);
  directory = argv[3];
  if (ip == "" || directory == " " || !port_number) {
    std::cerr << "Invalid argument." << std::endl;
    return EXIT_FAILURE;
  }
  SRV::Server server(ip, port_number, directory);
  server.run();
}