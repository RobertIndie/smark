#pragma once

#include <map>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include "LuaBridge/LuaBridge.h"
#include "debug.h"
#include "util.h"

namespace smark {
  class Thread {
  public:
    std::string ip;
    uint16_t port;
    void Set(std::string name, luabridge::LuaRef value);
    luabridge::LuaRef Get(std::string name);
    void Stop();

  private:
    std::map<std::string, luabridge::LuaRef> env;
  };
  class Script {
  public:
    Script();
    void Init();
    void CallSetup(Thread *thread);
    void CallInit();
    util::HttpRequest *CallRequest();
    void CallReponse(util::HttpResponse *response);
    void CallDone();
  };
}  // namespace smark
