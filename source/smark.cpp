#include "smark.h"

#include <cstring>

#include "client.h"
#include "debug.h"
#include "util.h"
using namespace smark;


namespace smark {
  void Smark::Run() {
    uint32_t conn_per_thread = setting.connection_count / setting.thread_count;
    for (uint32_t i = 0; i < setting.thread_count; i++) {
      auto lua_thr_p = std::make_shared<LuaThread>();
      bool isStop = false;
      lua_thr_p->on_stop = [&isStop]() { isStop = true; };
      Script main_script;
      main_script.Init();
      main_script.SetThread(lua_thr_p.get());
      main_script.Run(setting.lua_code);
      main_script.CallSetup();

      if (isStop) continue;

      auto thr_p
          = std::make_shared<std::thread>([conn_per_thread, this, lua_thr_p]() {  // thread_main
              util::EventLoop el;
              Status status;
              std::vector<std::shared_ptr<HttpClient>> clients;

              Script thr_script;
              lua_thr_p->on_stop = [&el]() { el.Stop(); };
              thr_script.Init();
              thr_script.SetThread(lua_thr_p.get());
              thr_script.Run(this->setting.lua_code);

              thr_script.CallInit();

              // Build request

              auto req = thr_script.CallRequest();
              if (!req) {
                req = new util::HttpRequest();
                req->method = "Get";
                req->request_uri = "/test";
                auto test_header = std::make_shared<util::HttpPacket::Header>();
                test_header->name = "test-header";
                test_header->value = "test_value";
                req->headers.push_back(test_header);
                req->body = "This is a request";
              }

              for (uint32_t i = 0; i < conn_per_thread; i++) {
                auto clip = std::make_shared<HttpClient>(&el);
                clients.push_back(clip);

                status.request_count++;  // TODO: move to request callback
                {
                  std::lock_guard<std::mutex> guard(status_mutex_);
                  this->status.request_count++;
                }
                clip->on_response = [this, &status, &conn_per_thread, &el, &thr_script](
                                        HttpClient* cli, std::shared_ptr<util::HttpResponse> res) {
                  (void)res;
                  thr_script.CallReponse(res.get());
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
              thr_script.CallDone();
            });
      thread_pool_[thr_p] = lua_thr_p;
    }
    for (auto iter = thread_pool_.begin(); iter != thread_pool_.end(); iter++) {
      iter->first->join();
    }
  }
}  // namespace smark
