#pragma once

#include <atomic>
#include <winsock.h>

namespace SCK {
class Socket {
  std::atomic<int> sockfd;

public:
  explicit Socket(int sock) noexcept : sockfd(sock) {}
  Socket() : sockfd(-1) {}
  ~Socket() {
    if (sockfd.load() >= 0)
      closesocket(sockfd.load());
  }
  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;
  Socket(Socket &&other) noexcept : sockfd(other.sockfd.exchange(-1)) {}
  Socket &operator=(Socket &&other) noexcept {
    if (this != &other) {
      if (sockfd.load() >= 0)
        closesocket(sockfd.load());
      sockfd.store(other.sockfd.exchange(-1));
    }
    return *this;
  }
  int get_sockfd() const noexcept { return sockfd.load(); }
};
} // namespace SCK