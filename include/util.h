#pragma once

extern "C" {
#include <http_parser.h>
}

#include <uv.h>

#include <functional>
#include <map>
#include <memory>
#include <thread>

#define _MACRO_CONTACT_IMPL(x, y) x##y
#define _MACRO_CONTACT(x, y) _MACRO_CONTACT_IMPL(x, y)

#define DEFER(X)                                                                      \
  auto _MACRO_CONTACT(_cpp_defer_obj_, __LINE__)                                      \
      = std::unique_ptr<void, std::function<void(void*)>>{reinterpret_cast<void*>(1), \
                                                          [&](void*) { X }};

// macro overload util
#define GET_MACRO_V2(_1, _2, NAME, ...) NAME

namespace smark::util {
  // enabe shared_from_this in constructor
  template <class T, class PtrT> void deleter(PtrT* __ptr) {
    auto t = static_cast<T*>(__ptr);
    t->~T();
    free(t);
  }

  /**
   * Base class allowing use of member function shared_from_this in constructor function,member
   * function.
   */
  template <class T> class enable_shared_from_this {
  public:
    std::shared_ptr<T>* _construct_pself;
    std::weak_ptr<T> _construct_self;

    template <class RT = T> std::shared_ptr<RT> shared_from_this() {
      if (_construct_pself) {
        return std::static_pointer_cast<RT>(*_construct_pself);  // in constructor
      } else {
        return std::static_pointer_cast<RT>(_construct_self.lock());
      }
    }
  };

  template <class BaseT, class T, typename... Params>
  std::shared_ptr<T> make_shared(Params&&... args) {
    std::shared_ptr<BaseT> rtn;
    T* t = (T*)calloc(1, sizeof(T));
    rtn.reset(t, deleter<BaseT, T>);
    t->_construct_pself = &rtn;
    t = new (t) T(std::forward<Params>(args)...);
    t->_construct_pself = NULL;
    t->_construct_self = rtn;

    return std::static_pointer_cast<T>(rtn);
  }

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
    void Wait();
    void Stop();
    static std::string GetErrorStr(int status) { return uv_strerror(status); }
    friend class Socket;

  private:
    std::unique_ptr<uv_loop_t> loop_ = std::make_unique<uv_loop_t>();
  };
  class Socket {
  public:
    Socket(EventLoop* el);
    void Connect(
        std::string ip, int16_t port, CallbackType cb = [](auto) {});
    void Write(
        const char* data, size_t len, CallbackType cb = [](auto) {});
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
    HttpPacket() {}
    std::vector<std::shared_ptr<Header>> headers;  // TODO: use map.
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