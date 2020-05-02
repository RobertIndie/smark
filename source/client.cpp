#include "client.h"

namespace smark {
  TCPClient::TCPClient() {}

  void TCPClient::Connect(std::string ip, int16_t port) { socket_.Connect(ip, port); }

  void TCPClient::Send(const char* data, int len) {
    const char* curr_wr = data;
    while (curr_wr - data != len) {
      curr_wr += socket_.Write(data, len);
    }
  }

  void TCPClient::Recv(char* buff, int len) { socket_.Read(buff, len); }
}  // namespace smark
