#include "util.h"

#include <arpa/inet.h>
#include <fcntl.h>
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
    if (obj->readable_event) obj->readable_event(el);
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

  void EventLoop::Stop() { aeStop(ae_el_); }

  Socket::Socket() {
    fd_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ < 0) {
      ERR("Create socket fail.");
    }
  }

  void set_nonblocking(int sock) {
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
      ERR("set_nonblocking: get file status flags fail.");
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
      ERR("set_nonblocking: set file status flags fail.");
    }
  }

  void Socket::Connect(std::string ip, int16_t port) {
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    set_nonblocking(fd_);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
      ERR("Invalid address:" << LOG_VALUE(ip));
    }

    if (connect(fd_, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0
        && errno != EINPROGRESS) {
      ERR("Connect fail.");
    }
  }

  size_t Socket::Write(const char *data, int len) {
    auto ret = write(fd_, data, len);
    if (ret == -1) {
      ERR("Write err");
    }
    return ret;
  }

  size_t Socket::Read(char *buff, int len) {
    auto ret = read(fd_, buff, len);
    if (ret == -1) {
      ERR("Read err.");
    }
    return ret;
  }

  int Socket::GetFD() const { return fd_; }

  std::string HttpRequest::ToString() const {
    std::ostringstream oss;
    oss << method << " " << request_uri << " "
        << "HTTP/1.1"
        << "\r\n";
    for (auto iter = headers.begin(); iter != headers.end(); iter++) {
      oss << (**iter).name << ": " << (**iter).value << "\r\n";
    }
    oss << "\r\n";
    if (!body.empty()) oss << body << "\r\n";
    return oss.str();
  }

  std::string HttpResponse::ToString() const {
    ERR("do not implement.");  // TODO
  }

  void HttpReponseParser::Init() {
    parser_ = std::make_shared<http_parser>();
    http_parser_init(parser_.get(), HTTP_RESPONSE);
    parser_->data = this;

    http_parser_settings_init(&settings_);
    settings_.on_message_begin = OnMessageBegin;
    settings_.on_status = OnStatus;
    settings_.on_header_field = OnHeaderField;
    settings_.on_header_value = OnHeaderValue;
    settings_.on_body = OnBody;
    settings_.on_message_complete = OnMessageComplete;
  }

  int OnMessageBegin(http_parser *p) {
    auto parser = reinterpret_cast<HttpReponseParser *>(p->data);
    parser->res_ = std::make_shared<HttpResponse>();
    return 0;
  }
  int OnStatus(http_parser *p, const char *at, size_t length) {
    auto parser = reinterpret_cast<HttpReponseParser *>(p->data);
    parser->res_->status_code = std::string(at, length);
    return 0;
  }
  int OnHeaderField(http_parser *p, const char *at, size_t length) {
    auto parser = reinterpret_cast<HttpReponseParser *>(p->data);
    auto new_header = std::make_shared<HttpPacket::Header>();
    new_header->name = std::string(at, length);
    parser->res_->headers.push_back(new_header);
    return 0;
  }
  int OnHeaderValue(http_parser *p, const char *at, size_t length) {
    auto parser = reinterpret_cast<HttpReponseParser *>(p->data);
    auto header = parser->res_->headers.back();
    header->value = std::string(at, length);
    return 0;
  }
  int OnBody(http_parser *p, const char *at, size_t length) {
    auto parser = reinterpret_cast<HttpReponseParser *>(p->data);
    parser->res_->body = std::string(at, length);
    return 0;
  }
  int OnMessageComplete(http_parser *p) {
    auto parser = reinterpret_cast<HttpReponseParser *>(p->data);
    parser->on_complete(parser->res_);
    return 0;
  }

  void HttpReponseParser::Feed(const char *data, size_t len) {
    size_t nparsed = http_parser_execute(parser_.get(), &settings_, data, len);
    if (nparsed != len) ERR("http_parser_execute error." << LOG_VALUE(parser_->http_errno));
  }
}  // namespace smark::util