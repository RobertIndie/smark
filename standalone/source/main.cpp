#include <smark.h>

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
using namespace smark;

int main(int argc, char** argv) {
  cxxopts::Options options(argv[0], "A program to welcome the world!");

  std::string ip;
  uint16_t port;
  uint32_t connection_count, thread_count;

  // clang-format off
  options.add_options()
    ("p,port","set remote port",cxxopts::value<uint16_t>(port)->default_value("80"))
    ("h,host","set remote ip",cxxopts::value<std::string>(ip)->default_value("127.0.0.1"))
    ("t,thread","set the number of threads",cxxopts::value<uint32_t>(thread_count)->default_value("1"))
    ("c,connection","set the number of connection",cxxopts::value<uint32_t>(connection_count)->default_value("1"))

  ;
  // clang-format on
  
 //exception handing
  try {
    auto result=options.parse(argc,argv);
    }
   
    catch(cxxopts::OptionParseException & ex1){
      std::string str=ex1.what();
      std::cout<<str<<std::endl;
      exit(0);
    }
 
    catch(cxxopts::OptionSpecException & ex2){
      std::string str=ex2.what();
      std::cout<<str<<std::endl;
      exit(0);
    }
    
  
  
//smark parameter initialization and its call

  Smark smark;
  smark.setting.connection_count =connection_count; 
  smark.setting.thread_count = thread_count; 
  smark.setting.ip = ip;
  smark.setting.port = port;
  smark.Run();
  std::cout<<smark.status.finish_count<<std::endl;
  return 0;
}
