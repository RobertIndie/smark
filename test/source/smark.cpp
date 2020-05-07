#include <client.h>
#include <doctest/doctest.h>
#include <pthread.h>
#include <smark.h>

#include <cstring>
#include <future>

#include "debug.h"
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
TestServer* test_svr = nullptr;
SimpleHttpServer* simple_http_svr = nullptr;
pthread_t svr_thread;
uint16_t port = SVR_PORT;
uint16_t simple_http_port = SVR_PORT;

void* _ServerThread(void* arg) {
  auto test_svr = (TestServer*)arg;
  test_svr->Run();
  return nullptr;
}

void RunServer() {
  if (test_svr == nullptr) {
    test_svr = new TestServer();
    port = test_svr->Connect(port);
    int ret = pthread_create(&svr_thread, nullptr, &_ServerThread, test_svr);
    if (ret == -1) {
      printf("pthread create fail.");
      exit(EXIT_FAILURE);
    }
  }
}

void RunSimpleHttpServer() {
  if (simple_http_svr == nullptr) {
    simple_http_svr = new SimpleHttpServer();
    simple_http_port = simple_http_svr->Connect(port);
    int ret = pthread_create(&svr_thread, nullptr, &_ServerThread, simple_http_svr);
    if (ret == -1) {
      printf("pthread create fail.");
      exit(EXIT_FAILURE);
    }
  }
}
using namespace smark;
TEST_CASE("TCPClient") {
  RunServer();
  int task = 0;
  INIT_TASK;

  TCPClient cli;
  util::EventLoop el(13);
  el.SetEvent(&cli);
  cli.Connect("127.0.0.1", port);
  DLOG("Connected");
  const char data[] = "Hello world";
  cli.writable_event = [&cli, &task, &data](util::EventLoop* el) {
    (void)el;
    cli.Send(data, sizeof(data));
    cli.writable_event = [](auto) {};
  };
  cli.readable_event = [&cli, &task, &data](util::EventLoop* el) {
    (void)el;
    SUB_TASK(task);
    char buff[1024];
    cli.Recv(buff, sizeof(buff));
    CHECK(strcmp(buff, data) == 0);
    el->Stop();
    cli.readable_event = [](auto) {};
  };
  el.Wait();
  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("BasicBenchmark") {
  RunServer();
  Smark smark;
  smark.setting.connection_count = 4;
  smark.setting.thread_count = 2;
  smark.setting.ip = "127.0.0.1";
  smark.setting.port = port;
  // smark.setting.timeout_us = -1;
  smark.Run();
  CHECK(smark.status.finish_count == smark.setting.connection_count);
}

TEST_CASE("HttpClient") {
  RunSimpleHttpServer();
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

  HttpClient cli;
  util::EventLoop el(1024);
  el.SetEvent(&cli);
  cli.on_response = [&task, &el, &cli](auto, std::shared_ptr<util::HttpResponse> res) {
    SUB_TASK(task);
    CHECK(STR_COMPARE(res->status_code, "OK"));
    int header_count = res->headers.size();
    CHECK(header_count == 1);
    auto test_header = res->headers[0];
    CHECK(STR_COMPARE(test_header->name, "test-header"));
    CHECK(STR_COMPARE(test_header->value, "test_value"));
    CHECK(STR_COMPARE(res->body, "This is a response"));
    el.Stop();
  };
  cli.Connect("127.0.0.1", simple_http_port);
  cli.Request(req);

  el.Wait();
  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("Smark") {
  using namespace smark;

  Smark smark("World");

  CHECK(smark.greet(LanguageCode::EN) == "Hello, World!");
  CHECK(smark.greet(LanguageCode::DE) == "Hallo World!");
  CHECK(smark.greet(LanguageCode::ES) == "¡Hola World!");
  CHECK(smark.greet(LanguageCode::FR) == "Bonjour World!");
}
