#pragma once
#include <functional>
#include <memory>
#include <queue>
#include <vector>

#include "util.h"

namespace smark {
  class TCPClient : public smark::util::IEventObj {
  public:
    TCPClient();
    void Connect(std::string ip, int16_t port);
    void Send(const char* data, int len);
    size_t Recv(char* buff, int len);
    virtual int GetFD() const;
    void Close();

  private:
    smark::util::Socket socket_;
  };

  class HttpClient : public smark::util::IEventObj {
  public:
    HttpClient();
    void Connect(std::string ip, int16_t port);
    void Request(std::shared_ptr<util::HttpRequest> request);
    std::function<void(HttpClient*, std::shared_ptr<util::HttpResponse>)> on_response;
    virtual int GetFD() const;
    void Close();

  private:
    TCPClient cli_;
    util::HttpReponseParser parser_;
    std::queue<std::shared_ptr<util::HttpRequest>> request_queue_;
  };
}  // namespace smark