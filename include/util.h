#pragma onece
#include <functional>

namespace smark::util {
  class EventLoop;
  class IEventObj {
  public:
    virtual int GetFD() = 0;
    std::function<void(EventLoop* el)> write_event;
    std::function<void(EventLoop* el)> read_event;
  };
  class EventLoop {
  public:
    void SetEvent(IEventObj* obj, int flag);
    void DelEvent(IEventObj* obj);
    void Wait();
  };
  class Socket {
  public:
    void Connect(std::string ip, int16_t port);
    int Write(const char* data, int len);
    int Read(char* buff, int len);
  };
}  // namespace smark::util