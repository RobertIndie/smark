#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "util.h"

namespace smark {
  class TCPClient : public smark::util::IEventObj {
  public:
    TCPClient();
    void Connect(std::string ip, int16_t port);
    void Send(const char* data, int len);
    void Recv(char* buff, int len);
    virtual int GetFD() const;

  private:
    smark::util::Socket socket_;
  };

  class HttpClient : public smark::util::IEventObj {
  public:
    HttpClient();
    void Request(const util::HttpRequest* request);
    std::function<void(HttpClient*, std::shared_ptr<util::HttpResponse>)> on_response;
    virtual int GetFD() const;
  };
}  // namespace smark