#pragma once
#include <ae.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

#if defined(__linux__)
#  include <linux/version.h>
#endif

#define SVR_PORT 12138

namespace smark_tests {
  class TestServer {
  public:
    const int kMaxConns = 20;
    TestServer();
    ~TestServer();
    uint16_t Connect(uint16_t listen_port = SVR_PORT);
    void Run();
    std::function<void(int fd, const char* data, int len)> on_msg;
    void Send(int fd, const char* data, int len);

  private:
    int sock_fd_;
  };
}  // namespace smark_tests
