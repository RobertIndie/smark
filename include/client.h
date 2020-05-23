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
  };

  class HttpClient : public TCPClient {
  public:
    HttpClient(smark::util::EventLoop* el);
    void Request(util::HttpRequest* request);
    std::function<void(HttpClient*, std::shared_ptr<util::HttpResponse>)> on_response;

  private:
    util::HttpReponseParser parser_;
  };
}  // namespace smark