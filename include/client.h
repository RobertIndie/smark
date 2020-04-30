#pragma once
#include "util.h"

namespace smark {
  class TCPClient : public smark::util::IEventObj {
  public:
    void Connect(std::string ip, int16_t port);
    void Send(const char* data, int len);
    void Recv(char* buff, int len);
    virtual int GetFD() const;

  private:
    smark::util::Socket socket_;
  };
}  // namespace smark