#include "client.h"

#include "debug.h"

namespace smark {
  TCPClient::TCPClient(smark::util::EventLoop* el) : Socket(el) {}

  HttpClient::HttpClient(smark::util::EventLoop* el) : TCPClient(el) {
    on_read = [this](const char* data, ssize_t recved) {
      parser_.Feed(data, recved);
      if (recved != 0 && data[recved - 1] == '\0') parser_.Feed(data, 0);  // end of response
    };
    parser_.on_complete = [this](std::shared_ptr<util::HttpResponse> res) {
      if (this->on_response) this->on_response(this, res);
    };
    parser_.Init();
  }

  void HttpClient::Request(util::HttpRequest* request) {
    auto data = request->ToString();
    this->Write(data.c_str(), data.length(), [](int status) {
      if (status) {
        ERR("Http request error:" << util::EventLoop::GetErrorStr(status));
      }
    });
  }

  HttpAsyncClient::HttpAsyncClient(smark::util::EventLoop* el) : HttpClient(el) {}

  std::shared_ptr<tasks::ValueTask<int>> HttpAsyncClient::ConnectAsync(std::string ip,
                                                                       int16_t port) {
    auto task = _async(int, [=](std::shared_ptr<tasks::ValueTask<int>> this_task) {
      DLOG("Try to connect:" << LOG_VALUE(ip) << LOG_VALUE(port));
      Connect(ip, port, [=](int status) {
        DLOG("Connected result:" << LOG_VALUE(status));
        this_task->Complete(status);
      });
    });
    return task;
  }

  std::shared_ptr<tasks::ValueTask<std::shared_ptr<util::HttpResponse>>>
  HttpAsyncClient::RequestAsync(util::HttpRequest* request) {
    auto task = _async(
        std::shared_ptr<util::HttpResponse>,
        [=](std::shared_ptr<tasks::ValueTask<std::shared_ptr<util::HttpResponse>>> this_task) {
          on_response = [this_task](auto, std::shared_ptr<util::HttpResponse> res) {
            this_task->Complete(res);
          };
          HttpClient::Request(request);
        });
    return task;
  }

  void HttpAsyncClient::Close() { Socket::Close(); }

}  // namespace smark
