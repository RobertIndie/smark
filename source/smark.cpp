#include "smark.h"

#include <cstring>

#include "client.h"
#include "debug.h"
#include "util.h"
using namespace smark;

Smark::Smark(std::string _name) : name(_name) {}

std::string Smark::greet(LanguageCode lang) const {
  switch (lang) {
    default:
    case LanguageCode::EN:
      return "Hello, " + name + "!";
    case LanguageCode::DE:
      return "Hallo " + name + "!";
    case LanguageCode::ES:
      return "¡Hola " + name + "!";
    case LanguageCode::FR:
      return "Bonjour " + name + "!";
  }
}

namespace smark {
  void Smark::Run() {
    uint32_t conn_per_thread = setting.connection_count / setting.thread_count;
    for (uint32_t i = 0; i < setting.thread_count; i++) {
      auto thr_p = std::make_shared<std::thread>([conn_per_thread, this]() {  // thread_main
        util::EventLoop el;
        Status status;
        std::vector<std::shared_ptr<HttpClient>> clients;

        // Build request
        auto req = std::make_shared<util::HttpRequest>();
        req->method = "Get";
        req->request_uri = "/test";
        auto test_header = std::make_shared<util::HttpPacket::Header>();
        test_header->name = "test-header";
        test_header->value = "test_value";
        req->headers.push_back(test_header);
        req->body = "This is a request";

        for (uint32_t i = 0; i < conn_per_thread; i++) {
          auto clip = std::make_shared<HttpClient>(&el);
          clients.push_back(clip);

          status.request_count++;  // TODO: move to request callback
          {
            std::lock_guard<std::mutex> guard(status_mutex_);
            this->status.request_count++;
          }
          clip->on_response = [this, &status, &conn_per_thread, &el](
                                  HttpClient* cli, std::shared_ptr<util::HttpResponse> res) {
            (void)res;
            cli->Close();
            status.finish_count++;
            std::lock_guard<std::mutex> guard(status_mutex_);
            this->status.finish_count++;
          };
          clip->Connect(this->setting.ip, this->setting.port, [clip, req](int status) {
            if (status) {
              ERR("Connect error:" << util::EventLoop::GetErrorStr(status));
            }
            clip->Request(req);
          });
        }
        el.Wait();
      });
      thread_pool_.push_back(thr_p);
    }
    for (auto iter = thread_pool_.begin(); iter != thread_pool_.end(); iter++) {
      (**iter).join();
    }
  }
}  // namespace smark
