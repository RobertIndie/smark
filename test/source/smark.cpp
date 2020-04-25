#include <doctest/doctest.h>
#include <smark.h>

#if defined(_WIN32) || defined(WIN32)
// apparently this is required to compile in MSVC++
#  include <sstream>
#endif

TEST_CASE("Smark") {
  using namespace smark;

  Smark smark("World");

  CHECK(smark.greet(LanguageCode::EN) == "Hello, World!");
  CHECK(smark.greet(LanguageCode::DE) == "Hallo World!");
  CHECK(smark.greet(LanguageCode::ES) == "¡Hola World!");
  CHECK(smark.greet(LanguageCode::FR) == "Bonjour World!");
}
