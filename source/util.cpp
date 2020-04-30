#include "util.h"

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
    auto el = reinterpret_cast<EventLoop *>(data);
    auto obj = el->obj_map_[fd];
    if (obj->writable_event) obj->writable_event(el);
  }
  void EventLoop::readable_proc(aeEventLoop *loop, int fd, void *data, int mask) {
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
}  // namespace smark::util