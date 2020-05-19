#pragma once
#include <functional>
#include <memory>
#include <queue>
#include <vector>

#include "util.h"

namespace smark {
  class TCPClient : public smark::util::Socket {
  public:
    TCPClient(smark::util::EventLoop* el);
    // void Connect(std::string ip, int16_t port);
    // void Send(const char* data, int len);
    // size_t Recv(char* buff, int len);
    // std::function<void(const char*, ssize_t)> on_recv;
    // void Close();
  };

  class HttpClient : public TCPClient {
  public:
    HttpClient(smark::util::EventLoop* el);
    // void Connect(std::string ip, int16_t port);
    void Request(std::shared_ptr<util::HttpRequest> request);
    std::function<void(HttpClient*, std::shared_ptr<util::HttpResponse>)> on_response;
    // virtual int GetFD() const;
    // void Close();

  private:
    util::HttpReponseParser parser_;
    // std::queue<std::shared_ptr<util::HttpRequest>> request_queue_;
  };
}  // namespace smark