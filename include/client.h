#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "util.h"

namespace smark {
  class TCPClient : public smark::util::IEventObj {
  public:
    TCPClient();
    void Connect(std::string ip, int16_t port);
    void Send(const char* data, int len);
    void Recv(char* buff, int len);
    virtual int GetFD() const;

  private:
    smark::util::Socket socket_;
  };

  class HttpPacket {
  public:
    class Header {
    public:
      std::string name;
      std::string value;
    };
    std::vector<Header> headers;
    virtual std::string ToString() const = 0;
  };

  class HttpRequest : public HttpPacket {
  public:
    std::string method;
    std::string request_uri;
  };

  class HttpResponse : public HttpPacket {
  public:
    std::string status_code;
  };

  class HttpClient : public smark::util::IEventObj {
  public:
    HttpClient();
    void Request(const HttpRequest* request);
    std::function<void(HttpClient*, std::shared_ptr<HttpResponse>)> on_response;
  };
}  // namespace smark