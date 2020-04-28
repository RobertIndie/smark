#include "testsvr.h"

namespace smark_tests {
  void setnonblocking(int sock) {
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
      perror("fcntl(sock,GETFL)");
      exit(1);
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
      perror("fcntl(sock,SETFL,opts)");
      exit(1);
    }
  }

  TestServer::TestServer() {}
  TestServer::~TestServer() {}

  void TestServer::Connect(uint16_t listen_port) {
    sock_fd_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd_ < 0) {
      throw new std::runtime_error("socket create fail.");
    }

    int flags = fcntl(sock_fd_, F_GETFL, 0);
    fcntl(sock_fd_, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in myserver;
    memset(&myserver, 0, sizeof(myserver));
    myserver.sin_family = AF_INET;
    myserver.sin_addr.s_addr = htonl(INADDR_ANY);
    myserver.sin_port = htons(listen_port);

    if (bind(sock_fd_, (sockaddr *)&myserver, sizeof(myserver)) < 0) {
      throw "bind() failed";
    }

    if (listen(sock_fd_, kMaxConns) < 0) {
      throw "listen() failed";
    }

    flags = 1;
    setsockopt(sock_fd_, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
  }
  void TestServer::Run() {
    epoll_event ev;
#define ERR(err) \
  perror(err);   \
  throw err;
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
      ERR("epoll create fail.")
    }

    ev.events = EPOLLIN;
    ev.data.fd = sock_fd_;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd_, &ev) == -1) {
      ERR("epoll_ctl sock_fd_ fail.");
    }
#define MAX_EVENT 100
    epoll_event events[MAX_EVENT];

    while (true) {
      int nfds = epoll_wait(epoll_fd, events, MAX_EVENT, -1);
      if (nfds == -1) {
        ERR("error epoll_wait.");
      }
      for (int i = 0; i < nfds; i++) {
        if (events[i].data.fd == sock_fd_) {
          // new connection
          sockaddr_in remote_addr;
          socklen_t addr_len = sizeof(sockaddr_in);
          int conn_sock = accept(sock_fd_, (sockaddr *)&remote_addr, &addr_len);
          if (conn_sock == -1) {
            ERR("accept error.");
          }

          setnonblocking(conn_sock);
          ev.events = EPOLLIN;
          ev.data.fd = conn_sock;
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
            ERR("epoll_ctl conn_sock fail.");
          }
        } else {
          char buffer[1024];
          int fd = events[i].data.fd;
          auto rsize = read(fd, buffer, sizeof(buffer));
          if (rsize == -1) {
            ERR("read err.");
          }
          if (rsize == 0) {
#if defined(__linux__)
#  if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9))
            epoll_event _e;
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &_e);
#  else
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
#  endif
#endif
          }
          if (rsize > 0) {
            write(fd, buffer, rsize);
          }
        }
      }
    }
  }

}  // namespace smark_tests
