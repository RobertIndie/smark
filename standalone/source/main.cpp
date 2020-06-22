#include <smark.h>

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
using namespace smark;

const std::unordered_map<std::string, smark::LanguageCode> languages{
    {"en", smark::LanguageCode::EN},
    {"de", smark::LanguageCode::DE},
    {"es", smark::LanguageCode::ES},
    {"fr", smark::LanguageCode::FR},
};

int main(int argc, char** argv) {
  cxxopts::Options options(argv[0], "A program to welcome the world!");

  std::string ip;
  uint16_t port;
  uint32_t connection_count,thread_count;

  // clang-format off
  options.add_options()
    ("p,port","set remote port",cxxopts::value<uint16_t>(port)->default_value("80"))
    ("ip","set remote ip",cxxopts::value<std::string>(ip)->default_value("127.0.0.1"))
    ("t,thread","set the number of threads",cxxopts::value<uint32_t>(thread_count)->default_value("1"))
    ("c,connection","set the number of connection",cxxopts::value<uint32_t>(connection_count)->default_value("1"))
  ;
  // clang-format on
  // auto result=options.parse(argc,argv);
  // if(!(result.count("port"))){
  //   std::cout<<"set remote port error"<<std::endl;
  //   port=result["port"].as<uint16_t>();
  // }
  // if(!(result.count("ip"))){
  //   std::cout<<"set remote ip error"<<std::endl;
  //   ip=result["ip"].as<std::string>();
  // }

  // if(!(result.count("thread"))){
  //   std::cout<<"set the number of thread error"<<std::endl;
  //   thread_count=result["thread"].as<uint32_t>();
  // }

  //  if(!(result.count("connection"))){
  //   std::cout<<"set the number of connection error"<<std::endl;
  //   connection_count=result["thread"].as<uint32_t>();
  // }
  Smark smark;
  smark.Run();


  return 0;
}
