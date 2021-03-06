#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "script.h"

namespace smark {

  class SmarkSetting {
  public:
    uint32_t connection_count = 0;
    uint32_t thread_count = 0;
    int timeout_us = -1;
    std::string ip;
    uint16_t port;
    std::string lua_code;
  };

  class Status {
  public:
    uint32_t request_count = 0;
    uint32_t finish_count = 0;
  };

  class Smark {
    std::string name;

  public:
    Smark() {}
    void Run();
    SmarkSetting setting;
    Status status;

  private:
    std::map<std::shared_ptr<std::thread>, std::shared_ptr<LuaThread>> thread_pool_;
    std::mutex status_mutex_;
  };

}  // namespace smark
