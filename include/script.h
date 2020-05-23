#pragma once

#include <map>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include "LuaBridge/LuaBridge.h"
#include "LuaBridge/Map.h"
#include "debug.h"
#include "util.h"

namespace smark {
  class LuaThread {
  public:
    std::string ip;
    uint16_t port;
    void Set(std::string name, std::string value);
    std::string Get(std::string name);
    void Stop();
    std::map<std::string, std::string> env;
  };
  class Script {
  public:
    Script();
    void Init();
    void Run(std::string codes);
    void CallSetup(LuaThread *thread);
    void CallInit();
    util::HttpRequest *CallRequest();
    void CallReponse(util::HttpResponse *response);
    void CallDone();

    // private:
    // std::unique_ptr<lua_State> state = std::make_unique<lua_State>(luaL_newstate());
    lua_State *state = luaL_newstate();
  };
}  // namespace smark
