#include "platform.h"
DISABLE_SOME_WARNINGS
#include <client.h>
#include <doctest/doctest.h>
#include <smark.h>

#include <cstring>
#include <future>
#include <memory>

#include "debug.h"
#include "script.h"
#include "util.h"

#if defined(_WIN32) || defined(WIN32)
// apparently this is required to compile in MSVC++
#  include <sstream>
#endif

#include "testsvr.h"

#define INIT_TASK int __task_count = __COUNTER__
#define SUB_TASK(task) \
  (void)__COUNTER__;   \
  task++
#define END_TASK __task_count = __COUNTER__ - __task_count - 1

// do not use '==' to compare string
// do not use string.compare: fail on "This is a response"
#define STR_COMPARE(str, value) strcmp(str.c_str(), value) == 0

using namespace smark_tests;

uint16_t port = SVR_PORT;
std::mutex port_mutex;

uint16_t RunServer(TestServer* svr, std::thread** thread) {
  port_mutex.lock();
  port++;
  port = svr->Connect(port);
  port_mutex.unlock();
  *thread = new std::thread([svr]() { svr->Run(); });
  return port;
}

using namespace smark;
TEST_CASE("TCPClient") {
  auto svr = new TestServer();
  DEFER(delete svr;)
  std::thread* thread = nullptr;
  uint16_t svr_port = RunServer(svr, &thread);
  // DEFER(delete thread;)
  DLOG("Run TCP server on port:" << svr_port);
  int task = 0;
  INIT_TASK;

  util::EventLoop el;
  TCPClient cli(&el);

  const char data[] = "Hello world";
  cli.on_read = [&cli, &task, &data](const char* recv_data, ssize_t nread) {
    SUB_TASK(task);
    CHECK(nread == sizeof(data));
    CHECK(strcmp(recv_data, data) == 0);
    cli.Close();
  };
  cli.Connect("127.0.0.1", port, [&cli, &task, &data](int status) {
    if (status) {
      ERR("Connect error:" << util::EventLoop::GetErrorStr(status));
    }
    cli.Write(data, sizeof(data), [](int status) {
      if (status) {
        ERR("Write error:" << util::EventLoop::GetErrorStr(status));
      }
    });
  });
  el.Wait();
  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("FailConnect") {
  port_mutex.lock();
  port++;
  port_mutex.unlock();

  int task = 0;
  INIT_TASK;

  util::EventLoop el;
  TCPClient cli(&el);
  cli.Connect("127.0.0.1", port, [&task](int status) {
    SUB_TASK(task);
    if (status) {
      DLOG("Test fail connect:" << util::EventLoop::GetErrorStr(status));
    }

    // Use macro instead of actual status code.
    CHECK(status == UV_ECONNREFUSED);  // In windows platform, the status of error "connection
                                       // refused" is -4078. But in linux, it is -111.
  });

  el.Wait();
  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("BasicBenchmark") {
  DLOG("Test: BasicBenchmark");
  auto svr = new SimpleHttpServer();
  DEFER(delete svr;)
  std::thread* thread = nullptr;
  uint16_t p = RunServer(svr, &thread);
  // DEFER(delete thread;)
  DLOG("Run Http server on port:" << p);
  Smark smark;
  smark.setting.connection_count = 1;
  smark.setting.thread_count = 1;
  smark.setting.ip = "127.0.0.1";
  smark.setting.port = p;
  // smark.setting.timeout_us = -1;
  smark.Run();
  CHECK(smark.status.finish_count == smark.setting.connection_count);
}

TEST_CASE("HttpClient") {
  auto svr = new SimpleHttpServer();
  DEFER(delete svr;)
  std::thread* thread = nullptr;
  uint16_t p = RunServer(svr, &thread);
  // DEFER(delete thread;)
  DLOG("Run Http server on port:" << p);
  INIT_TASK;
  int task = 0;
  auto req = std::make_shared<util::HttpRequest>();
  req->method = "Get";
  req->request_uri = "/test";
  auto test_header = std::make_shared<util::HttpPacket::Header>();
  test_header->name = "test-header";
  test_header->value = "test_value";
  req->headers.push_back(test_header);
  req->body = "This is a request";

  util::EventLoop el;
  HttpClient cli(&el);
  cli.on_response = [&task, &el, &cli](auto, std::shared_ptr<util::HttpResponse> res) {
    SUB_TASK(task);
    CHECK(STR_COMPARE(res->status_code, "OK"));
    int header_count = res->headers.size();
    CHECK(header_count == 1);
    auto test_header = res->headers[0];
    CHECK(STR_COMPARE(test_header->name, "test-header"));
    CHECK(STR_COMPARE(test_header->value, "test_value"));
    CHECK(STR_COMPARE(res->body, "This is a response"));
    cli.Close();
  };
  cli.Connect("127.0.0.1", p, [&cli, &req](int status) {
    if (status) {
      ERR("Connect error:" << util::EventLoop::GetErrorStr(status));
    }
    cli.Request(req.get());
  });

  el.Wait();
  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("Script_Setup") {
  LuaThread thread;
  Script script;
  script.Init();
  auto code
      = "function setup()\n"
        " thread.ip='127.0.0.1'\n"
        " thread.port=12138\n"
        " thread:set('testStr','str')\n"
        // " thread:set('testInt',100)\n"
        " thread:set('Str2',thread:get('testStr'))\n"
        "end";
  script.Run(code);
  script.SetThread(&thread);
  script.CallSetup();
  CHECK(STR_COMPARE(thread.ip, "127.0.0.1"));
  CHECK(thread.port == 12138);
  CHECK(STR_COMPARE(thread.env["testStr"], "str"));
  // CHECK(thread.env["testInt"] == 100);
  CHECK(STR_COMPARE(thread.env["testStr"], "str"));
}

TEST_CASE("Script_Init") {
  LuaThread thread;
  Script script;
  script.Init();
  auto code
      = "function init()\n"
        " pass=true\n"
        "end";
  script.Run(code);
  script.CallInit();
  CHECK(luabridge::getGlobal(script.state, "pass"));
}

TEST_CASE("Script_Request") {
  LuaThread thread;
  Script script;
  script.Init();
  auto code
      = "function request()\n"
        " local req = smark.Request()\n"
        " req.body='content'\n"
        " local headers = {}\n"
        " headers['test']='value'\n"
        " req:set_headers(headers)\n"
        " req.method='GET'\n"
        " req.uri='/test'\n"
        " return req\n"
        "end";
  script.Run(code);
  auto req = script.CallRequest();
  CHECK(STR_COMPARE(req->headers[0]->name, "test"));
  CHECK(STR_COMPARE(req->headers[0]->value, "value"));
  CHECK(STR_COMPARE(req->method, "GET"));
  CHECK(STR_COMPARE(req->request_uri, "/test"));
  CHECK(STR_COMPARE(req->body, "content"));
}

TEST_CASE("Script_Response") {
  LuaThread thread;
  Script script;
  script.Init();
  auto code
      = "function check(b,mes)\n"
        " if(b~=true)\n"
        " then\n"
        "   pass=false\n"
        "   print(mes)\n"
        "   error(mes)\n"
        " end\n"
        "end\n"
        "function response(res)\n"
        " check(res.body=='content','response body incorrect.')\n"
        " check(res.status=='200','response status incorrect.')\n"
        " headers = res:get_headers()\n"
        " check(headers['test']=='value','response headers incorrect.')\n"
        " pass=true\n"
        "end";
  script.Run(code);
  util::HttpResponse res;
  res.body = "content";
  res.status_code = "200";
  auto header = std::make_shared<util::HttpPacket::Header>();
  header->name = "test";
  header->value = "value";
  res.headers.push_back(header);

  script.CallReponse(&res);
  CHECK(luabridge::getGlobal(script.state, "pass"));
}

TEST_CASE("Script_Done") {
  LuaThread thread;
  Script script;
  script.Init();
  auto code
      = "function done()\n"
        " pass=true\n"
        "end";
  script.Run(code);
  script.CallDone();
  CHECK(luabridge::getGlobal(script.state, "pass"));
}

TEST_CASE("Script_Smark") {
  DLOG("Test: BasicBenchmark");
  auto svr = new SimpleHttpServer();
  DEFER(delete svr;)
  std::thread* thread = nullptr;
  uint16_t p = RunServer(svr, &thread);
  // DEFER(delete thread;)
  DLOG("Run Http server on port:" << p);
  Smark smark;
  smark.setting.connection_count = 1;
  smark.setting.thread_count = 1;
  smark.setting.ip = "127.0.0.1";
  smark.setting.port = p;
  // smark.setting.timeout_us = -1;
  smark.setting.lua_code
      = "function check(b,mes)\n"
        " if(b~=true)\n"
        " then\n"
        "   pass=false\n"
        "   print(mes)\n"
        "   error(mes)\n"
        " end\n"
        "end\n"
        "function setup()\n"
        " thread:set('test_env','env_value')\n"
        "end\n"
        "function request()\n"
        " call_request=true\n"
        " local req = smark.Request()\n"
        " req.body='This is a request'\n"
        " local headers = {}\n"
        " headers['test-header']='test_value'\n"
        " req:set_headers(headers)\n"
        " req.method='GET'\n"
        " req.uri='/test'\n"
        " return req\n"
        "end\n"
        "function response(res)\n"
        " call_response=true\n"
        "end\n"
        "function done()\n"
        " check(call_request,'Did not call request.')\n"
        " check(call_response,'Did not call response.')\n"
        " check(thread:get('test_env')=='env_value','Thread env test fail.')"
        "end\n";
  smark.Run();
}
