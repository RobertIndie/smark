#pragma once
#ifdef __linux__
#  define SUPPORT_AE
#  define SUPPORT_PTHREAD
#endif
#ifdef SUPPORT_AE
extern "C" {
#  include <ae.h>
}
#endif
#ifdef SUPPORT_PTHREAD
#  include <pthread.h>
#endif

extern "C" {
#include <http_parser.h>
}

#include <uv.h>

#include <functional>
#include <map>
#include <memory>

#define _MACRO_CONTACT_IMPL(x, y) x##y
#define _MACRO_CONTACT(x, y) _MACRO_CONTACT_IMPL(x, y)

#define DEFER(X)                                                                      \
  auto _MACRO_CONTACT(_cpp_defer_obj_, __LINE__)                                      \
      = std::unique_ptr<void, std::function<void(void*)>>{reinterpret_cast<void*>(1), \
                                                          [&](void*) { X }};

namespace smark::util {
  typedef std::function<void(int)> CallbackType;  // void(int status)
  class EventLoop;
  class IEventObj {
  public:
    virtual int GetFD() const = 0;
    std::function<void(EventLoop* el)> writable_event;
    std::function<void(EventLoop* el)> readable_event;
  };
  class EventLoop {
  public:
    EventLoop();
    // enum EventFlag { kNone = 0, kWriteable = 1, kReadable = 2, kBoth = 3 };
    // void SetEvent(const IEventObj* obj, EventFlag flag = EventFlag::kBoth);
    // void DelEvent(const IEventObj* obj, EventFlag flag = EventFlag::kBoth);
    void Wait();
    void Stop();
    static std::string GetErrorStr(int status) { return uv_strerror(status); }
    friend class Socket;
#ifdef SUPPORT_AE
  private:
    // aeEventLoop* ae_el_;
    std::unique_ptr<uv_loop_t> loop_ = std::make_unique<uv_loop_t>();
    // std::map<int, const IEventObj*> obj_map_;  // fd -> obj
    // static void writable_proc(aeEventLoop* loop, int fd, void* data, int mask);
    // static void readable_proc(aeEventLoop* loop, int fd, void* data, int mask);
#endif
  };
  class Socket {
  public:
    Socket(EventLoop* el);
    void Connect(
        std::string ip, int16_t port, CallbackType cb = [](auto) {});
    void Write(
        const char* data, int len, CallbackType cb = [](auto) {});
    void ReadStart();
    std::function<void(const char*, ssize_t)> on_read;
    void Close();

  private:
    std::unique_ptr<uv_tcp_t> socket_ = std::make_unique<uv_tcp_t>();
  };
  class HttpPacket {
  public:
    class Header {
    public:
      std::string name;
      std::string value;
    };
    std::vector<std::shared_ptr<Header>> headers;
    std::string body;
    virtual std::string ToString() const = 0;
  };

  class HttpRequest : public HttpPacket {
  public:
    std::string method;
    std::string request_uri;
    virtual std::string ToString() const;
  };

  class HttpResponse : public HttpPacket {
  public:
    std::string status_code;
    virtual std::string ToString() const;
  };

  int OnMessageBegin(http_parser* p);
  int OnStatus(http_parser* p, const char* at, size_t length);
  int OnHeaderField(http_parser* p, const char* at, size_t length);
  int OnHeaderValue(http_parser* p, const char* at, size_t length);
  int OnBody(http_parser* p, const char* at, size_t length);
  int OnMessageComplete(http_parser* p);

  class HttpReponseParser {
  public:
    void Init();
    void Feed(const char* data, size_t len);
    std::function<void(std::shared_ptr<HttpResponse>)> on_complete;

  private:
    std::shared_ptr<http_parser> parser_;
    std::shared_ptr<HttpResponse> res_;
    http_parser_settings settings_;
    friend int OnMessageBegin(http_parser* p);
    friend int OnStatus(http_parser* p, const char* at, size_t length);
    friend int OnHeaderField(http_parser* p, const char* at, size_t length);
    friend int OnHeaderValue(http_parser* p, const char* at, size_t length);
    friend int OnBody(http_parser* p, const char* at, size_t length);
    friend int OnMessageComplete(http_parser* p);
  };
}  // namespace smark::util