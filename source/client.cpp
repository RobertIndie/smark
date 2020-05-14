#include "client.h"

#include "debug.h"

namespace smark {
  TCPClient::TCPClient() {}

  void TCPClient::Connect(std::string ip, int16_t port) { socket_.Connect(ip, port); }

  void TCPClient::Send(const char* data, int len) {
    const char* curr_wr = data;
    while (curr_wr - data != len) {
      curr_wr += socket_.Write(data, len);
    }
  }

  size_t TCPClient::Recv(char* buff, int len) { return socket_.Read(buff, len); }

  int TCPClient::GetFD() const { return socket_.GetFD(); }

  void TCPClient::Close() { socket_.Close(); }

  HttpClient::HttpClient() {
    writable_event = [this](auto) {
      if (!this->request_queue_.empty()) {
        auto req = this->request_queue_.front();
        auto data = req->ToString();
        this->cli_.Send(data.c_str(), data.length());
        request_queue_.pop();
      }
    };
    readable_event = [this](auto) {
      char buffer[1024];
      int recved = this->cli_.Recv(buffer, sizeof(buffer));
      parser_.Feed(buffer, recved);
      if (recved != 0 && buffer[recved - 1] == '\0') parser_.Feed(buffer, 0);  // end of response
    };
    parser_.on_complete = [this](std::shared_ptr<util::HttpResponse> res) {
      if (this->on_response) this->on_response(this, res);
    };
    parser_.Init();
  }

  void HttpClient::Connect(std::string ip, int16_t port) {
    cli_.Connect(ip, port);
    DLOG("HttpClient connect to: " << LOG_VALUE(ip) << LOG_VALUE(port));
  }

  void HttpClient::Request(std::shared_ptr<util::HttpRequest> request) {
    request_queue_.push(request);
  }

  int HttpClient::GetFD() const { return cli_.GetFD(); }

  void HttpClient::Close() { cli_.Close(); }
}  // namespace smark
