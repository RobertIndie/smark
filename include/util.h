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

#include <functional>
#include <map>
#include <memory>

namespace smark::util {
  class EventLoop;
  class IEventObj {
  public:
    virtual int GetFD() const = 0;
    std::function<void(EventLoop* el)> writable_event;
    std::function<void(EventLoop* el)> readable_event;
  };
  class EventLoop {
  public:
    EventLoop(int set_size);
    enum EventFlag { kNone = 0, kWriteable = 1, kReadable = 2, kBoth = 3 };
    void SetEvent(const IEventObj* obj, EventFlag flag = EventFlag::kBoth);
    void DelEvent(const IEventObj* obj, EventFlag flag = EventFlag::kBoth);
    void Wait();
    void Stop();
#ifdef SUPPORT_AE
  private:
    aeEventLoop* ae_el_;
    std::map<int, const IEventObj*> obj_map_;  // fd -> obj
    static void writable_proc(aeEventLoop* loop, int fd, void* data, int mask);
    static void readable_proc(aeEventLoop* loop, int fd, void* data, int mask);
#endif
  };
  class Socket : public IEventObj {
  public:
    Socket();
    void Connect(std::string ip, int16_t port);
    int Write(const char* data, int len);
    int Read(char* buff, int len);
    int GetFD() const;
#ifdef SUPPORT_AE
  private:
    int fd_ = -1;
#endif
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
  class HttpReponseParser {
  public:
    void Init();
    void Feed(const char* data, size_t len);
    std::function<void(std::shared_ptr<HttpResponse>)> on_complete;
  };
}  // namespace smark::util