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
    uint conn_per_thread = setting.connection_count / setting.thread_count;
    for (uint i = 0; i < setting.thread_count; i++) {
      auto thr_p = std::make_shared<std::thread>([conn_per_thread, this]() {  // thread_main
        util::EventLoop el(10 + setting.connection_count * 3);
        Status status;
        std::vector<std::shared_ptr<TCPClient>> clients;
        const char data[] = "Hello world";
        for (uint i = 0; i < conn_per_thread; i++) {
          auto clip = std::make_shared<TCPClient>();
          clients.push_back(clip);
          clip->writable_event = [clip, &data, this, &status, &conn_per_thread](auto) {  // write
            clip->writable_event = [](auto) {};
            clip->readable_event = [clip, &data, this, &status,
                                    &conn_per_thread](util::EventLoop* el) {  // read
              clip->readable_event = [](auto) {};
              char buff[1024];
              clip->Recv(buff, sizeof(buff));
              if (strcmp(buff, data) != 0) ERR("Recv error.");
              status.finish_count++;
              if (status.finish_count >= conn_per_thread) el->Stop();
              std::lock_guard<std::mutex> guard(status_mutex_);
              this->status.finish_count++;
            };
            clip->Send(data, sizeof(data));
            status.request_count++;
            std::lock_guard<std::mutex> guard(status_mutex_);
            this->status.request_count++;
          };
          clip->Connect(this->setting.ip, this->setting.port);
          el.SetEvent(clip.get());
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
