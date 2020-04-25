#pragma once

#include <string>

namespace smark {

  enum class LanguageCode { EN, DE, ES, FR };

  class Smark {
    std::string name;

  public:
    Smark(std::string name);
    std::string greet(LanguageCode lang = LanguageCode::EN) const;
  };

}  // namespace smark
