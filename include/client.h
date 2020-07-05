#pragma once
#include <functional>
#include <memory>
#include <queue>
#include <vector>

#include "tasks.h"
#include "util.h"

namespace smark {
  class TCPClient : public smark::util::Socket {
  public:
    explicit TCPClient(smark::util::EventLoop* el);
  };

  class HttpClient : public TCPClient {
  public:
    explicit HttpClient(smark::util::EventLoop* el);
    void Request(util::HttpRequest* request);
    std::function<void(HttpClient*, std::shared_ptr<util::HttpResponse>)> on_response;

  private:
    util::HttpReponseParser parser_;
  };

  class HttpAsyncClient : protected HttpClient {
    // use protected inherit to prevent user changes on_complete during requesting.
  public:
    explicit HttpAsyncClient(smark::util::EventLoop* el);
    std::shared_ptr<tasks::ValueTask<int>> ConnectAsync(std::string ip, int16_t port);
    std::shared_ptr<tasks::ValueTask<std::shared_ptr<util::HttpResponse>>> RequestAsync(
        util::HttpRequest* request);
    void Close();
  };
}  // namespace smark