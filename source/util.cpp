#include "util.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <memory>
#include <tuple>

#include "debug.h"

namespace smark::util {
  EventLoop::EventLoop(int set_size) {
#ifdef SUPPORT_AE
    ae_el_ = aeCreateEventLoop(set_size);
#endif
  }

  void EventLoop::writable_proc(aeEventLoop *loop, int fd, void *data, int mask) {
    (void)loop;  // prevent unused parameter error
    (void)mask;
    auto el = reinterpret_cast<EventLoop *>(data);
    auto obj = el->obj_map_[fd];
    if (obj->writable_event) obj->writable_event(el);
  }
  void EventLoop::readable_proc(aeEventLoop *loop, int fd, void *data, int mask) {
    (void)loop;
    (void)mask;
    auto el = reinterpret_cast<EventLoop *>(data);
    auto obj = el->obj_map_[fd];
    if (obj->readable_event) obj->writable_event(el);
  }
  void EventLoop::SetEvent(const IEventObj *obj, EventFlag flag) {
    obj_map_[obj->GetFD()] = obj;
    if (flag & EventFlag::kWriteable
        && aeCreateFileEvent(ae_el_, obj->GetFD(), AE_WRITABLE, writable_proc, this) != AE_OK)
      ERR("Create file event error");
    if (flag & EventFlag::kReadable
        && aeCreateFileEvent(ae_el_, obj->GetFD(), AE_READABLE, readable_proc, this) != AE_OK)
      ERR("Create file event error");
  }

  void EventLoop::DelEvent(const IEventObj *obj, EventFlag flag) {
    int mask = AE_NONE;
    if (flag & EventFlag::kWriteable) mask |= AE_WRITABLE;
    if (flag & EventFlag::kReadable) mask |= AE_READABLE;
    aeDeleteFileEvent(ae_el_, obj->GetFD(), mask);
  }

  void EventLoop::Wait() { aeMain(ae_el_); }

  Socket::Socket() {
    fd_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ < 0) {
      ERR("Create socket fail.");
    }
  }

  void Socket::Connect(std::string ip, int16_t port) {
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
      ERR("Invalid address:" << LOG_VALUE(ip));
    }

    if (connect(fd_, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      ERR("Connect fail.");
    }
  }

  int Socket::Write(const char *data, int len) {
    auto ret = write(fd_, data, len);
    if (ret == -1) {
      ERR("Write err");
    }
    return ret;
  }

  int Socket::Read(char *buff, int len) {
    auto ret = read(fd_, buff, len);
    if (ret == -1) {
      ERR("Read err.");
    }
    return ret;
  }

  int Socket::GetFD() const { return fd_; }
}  // namespace smark::util