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
using namespace smark_tests;
TestServer* test_svr = nullptr;
pthread_t svr_thread;

void* _ServerThread(void* arg) {
  auto test_svr = (TestServer*)arg;
  test_svr->Run();
  return nullptr;
}

void RunServer() {
  if (test_svr == nullptr) {
    test_svr = new TestServer();
    test_svr->Connect();
    int ret = pthread_create(&svr_thread, nullptr, &_ServerThread, test_svr);
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

  TCPClient cli;
  util::EventLoop el(13);
  el.SetEvent(&cli);
  cli.Connect("127.0.0.1", 3000);
  DLOG("Connected");
  const char data[] = "Hello world";
  cli.writable_event = [&cli, &task, &data](util::EventLoop* el) {
    (void)el;
    cli.Send(data, sizeof(data));
    cli.writable_event = [](auto) {};
  };
  cli.readable_event = [&cli, &task, &data](util::EventLoop* el) {
    (void)el;
    task++;
    char buff[1024];
    cli.Recv(buff, sizeof(buff));
    CHECK(strcmp(buff, data) == 0);
    el->Stop();
    cli.readable_event = [](auto) {};
  };
  el.Wait();
  CHECK(task == 1);
}

TEST_CASE("BasicBenchmark") {
  RunServer();
  Smark smark;
  smark.setting.connection_count = 4;
  smark.setting.thread_count = 2;
  smark.setting.ip = "127.0.0.1";
  smark.setting.port = 3000;
  // smark.setting.timeout_us = -1;
  smark.Run();
  CHECK(smark.status.finish_count == 4);
}

TEST_CASE("Smark") {
  using namespace smark;

  Smark smark("World");

  CHECK(smark.greet(LanguageCode::EN) == "Hello, World!");
  CHECK(smark.greet(LanguageCode::DE) == "Hallo World!");
  CHECK(smark.greet(LanguageCode::ES) == "¡Hola World!");
  CHECK(smark.greet(LanguageCode::FR) == "Bonjour World!");
}
