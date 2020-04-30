#pragma once
#ifdef __linux__
#  define SUPPORT_AE
#endif
#ifdef SUPPORT_AE
#  include <ae.h>
#endif

#include <functional>
#include <map>

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
#ifdef SUPPORT_AE
  private:
    aeEventLoop* ae_el_;
    std::map<int, const IEventObj*> obj_map_;  // fd -> obj
    static void writable_proc(aeEventLoop* loop, int fd, void* data, int mask);
    static void readable_proc(aeEventLoop* loop, int fd, void* data, int mask)
#endif
  };
  class Socket {
  public:
    void Connect(std::string ip, int16_t port);
    int Write(const char* data, int len);
    int Read(char* buff, int len);
  };
}  // namespace smark::util