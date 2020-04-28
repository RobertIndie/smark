#include <doctest/doctest.h>
#include <pthread.h>
#include <smark.h>

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

TEST_CASE("Client") {
  RunServer();
  pthread_join(svr_thread, nullptr);
}

TEST_CASE("Smark") {
  using namespace smark;

  Smark smark("World");

  CHECK(smark.greet(LanguageCode::EN) == "Hello, World!");
  CHECK(smark.greet(LanguageCode::DE) == "Hallo World!");
  CHECK(smark.greet(LanguageCode::ES) == "¡Hola World!");
  CHECK(smark.greet(LanguageCode::FR) == "Bonjour World!");
}
