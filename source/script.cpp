#include "script.h"

namespace smark {
  void LuaThread::Set(std::string name, std::string value) {  // TDOO: Support value type: LuaRef
    // env[name] = value;
    // env.insert(std::make_pair(name, luabridge::LuaRef(value)));
    env[name] = value;
  }
  std::string LuaThread::Get(std::string name) { /*return env[name];*/
    return env[name];
  }
  void LuaThread::Stop() {
    if (on_stop) on_stop();
  }

  Script::Script() { luaL_openlibs(state); }

  void Script::Init() {
    luabridge::getGlobalNamespace(state)

        .beginNamespace("smark")

        .beginClass<util::HttpPacket>("Packet")  // Register util::HttpPacket
        .addProperty("body", &util::HttpPacket::body)
        .addFunction("set_headers",
                     std::function<void(util::HttpPacket*, luabridge::LuaRef)>(
                         [](util::HttpPacket* packet, luabridge::LuaRef headers) {
                           if (headers.isTable()) {
                             auto headersMap = headers.cast<std::map<std::string, std::string>>();
                             for (auto iter = headersMap.begin(); iter != headersMap.end();
                                  iter++) {
                               auto header = std::make_shared<util::HttpPacket::Header>();
                               header->name = iter->first;
                               header->value = iter->second;
                               packet->headers.push_back(header);
                             }
                           }
                         }))
        .addFunction("get_headers",
                     std::function<std::map<std::string, std::string>(util::HttpPacket*)>(
                         [](util::HttpPacket* packet) {
                           std::map<std::string, std::string> result;
                           for (auto iter = packet->headers.begin(); iter != packet->headers.end();
                                iter++) {
                             result[iter->get()->name] = iter->get()->value;
                           }
                           return result;
                         }))
        .endClass()

        .deriveClass<util::HttpRequest, util::HttpPacket>("Request")  // Register util::HttpRequest
        .addConstructor<void (*)(void)>()
        .addProperty("method", &util::HttpRequest::method)
        .addProperty("uri", &util::HttpRequest::request_uri)
        .endClass()

        .deriveClass<util::HttpResponse, util::HttpPacket>(
            "Response")  // Register util::HttpResponse
        .addConstructor<void (*)(void)>()
        .addProperty("status", &util::HttpResponse::status_code)
        .endClass()

        .beginClass<LuaThread>("thread")  // Register LuaThread
        .addProperty("ip", &LuaThread::ip)
        .addProperty("port", &LuaThread::port)
        .addFunction("set", &LuaThread::Set)
        .addFunction("get", &LuaThread::Get)
        .addFunction("stop", &LuaThread::Stop)
        .endClass()

        .endNamespace();
  }

  void Script::Run(std::string codes) { luaL_dostring(state, codes.c_str()); }

  void Script::SetThread(LuaThread* thread) { luabridge::setGlobal(state, thread, "thread"); }

  void Script::CallSetup() {
    auto setup = luabridge::getGlobal(state, "setup");
    if (setup.isNil()) return;
    if (!setup.isFunction()) {
      ERR("lua: setup is not a function.");
    }
    setup();
  }

  void Script::CallInit() {
    auto init = luabridge::getGlobal(state, "init");
    if (init.isNil()) return;
    if (!init.isFunction()) {
      ERR("lua: init is not a function.");
    }
    init();
  }

  util::HttpRequest* Script::CallRequest() {
    auto request = luabridge::getGlobal(state, "request");
    if (request.isNil()) return nullptr;
    if (!request.isFunction()) {
      ERR("lua: request is not a function.");
    }
    auto result = request();
    return result;
  }

  void Script::CallReponse(util::HttpResponse* res) {
    auto response = luabridge::getGlobal(state, "response");
    if (response.isNil()) return;
    if (!response.isFunction()) {
      ERR("lua: response is not a function.");
    }
    response(res);
  }

  void Script::CallDone() {
    auto done = luabridge::getGlobal(state, "done");
    if (done.isNil()) return;
    if (!done.isFunction()) {
      ERR("lua: done is not a function.");
    }
    done();
  }

}  // namespace smark
