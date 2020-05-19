#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

extern "C" {
#include <uv.h>
}

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#if defined(__linux__)
#  include <linux/version.h>
#endif

#define SVR_PORT 12238

namespace smark_tests {
  class TestServer {
  public:
    const int kMaxConns = 20;
    TestServer();
    ~TestServer();
    uint16_t Connect(uint16_t listen_port = SVR_PORT);
    void Run();
    std::function<void(uv_stream_t* client, const char* data, int len)> on_msg;
    void Send(uv_stream_t* client, char* data, int len);

    int sock_fd_;
    uint16_t port;
    uv_loop_t* loop;
    uv_tcp_t server;
  };

  class SimpleHttpServer : public TestServer {
  public:
    SimpleHttpServer();
  };
}  // namespace smark_tests
