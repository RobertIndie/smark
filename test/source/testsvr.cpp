#include "testsvr.h"

#include "debug.h"

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

  TestServer::TestServer() {
    on_msg = [this](uv_stream_t *cli, const char *data, int len) {
      char *send_data = (char *)malloc(len);
      memcpy(send_data, data, len);
      this->Send(cli, send_data, len);
    };
    loop = new uv_loop_t();
    uv_loop_init(loop);
  }
  TestServer::~TestServer() {}
  typedef struct {
    uv_write_t req;
    uv_buf_t buf;
  } write_req_t;
  void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t *)req;
    free(wr->buf.base);
    free(wr);
  }

  void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    (void)handle;
    buf->base = (char *)malloc(suggested_size);
    buf->len = suggested_size;
  }

  void on_close(uv_handle_t *handle) { free(handle); }
  void echo_write(uv_write_t *req, int status) {
    if (status) {
      // ERR("Write error: " << uv_strerror(status));
    }
    free_write_req(req);
  }

  void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    auto test_svr = reinterpret_cast<TestServer *>(client->data);
    if (nread > 0) {
      test_svr->on_msg((uv_stream_t *)client, buf->base, nread);
      free(buf->base);
      return;
    }
    if (nread < 0) {
      if (nread != UV_EOF) ERR("Read error: " << uv_err_name(nread));
      uv_close((uv_handle_t *)client, on_close);
    }

    free(buf->base);
  }

  void on_new_connection(uv_stream_t *server, int status) {
    DLOG("New connection");
    auto test_svr = reinterpret_cast<TestServer *>(server->data);

    if (status < 0) {
      ERR("New Connection error:" << uv_strerror(status));
    }

    uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(test_svr->loop, client);
    client->data = test_svr;
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
      uv_read_start((uv_stream_t *)client, alloc_buffer, on_read);
    } else {
      uv_close((uv_handle_t *)client, on_close);
    }
  }

  uint16_t TestServer::Connect(uint16_t listen_port) {
    // sock_fd_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    // if (sock_fd_ < 0) {
    //   throw new std::runtime_error("socket create fail.");
    // }

    uv_tcp_init(loop, &server);
    server.data = this;

    // int flags = fcntl(sock_fd_, F_GETFL, 0);
    // fcntl(sock_fd_, F_SETFL, flags | O_NONBLOCK);

    // sockaddr_in myserver;
    // memset(&myserver, 0, sizeof(myserver));
    // myserver.sin_family = AF_INET;
    // myserver.sin_addr.s_addr = htonl(INADDR_ANY);
    // myserver.sin_port = htons(listen_port);

#define IP "0.0.0.0"

    auto ip = IP;

    sockaddr_in addr;
    uv_ip4_addr(ip, listen_port, &addr);

    // while (bind(sock_fd_, (sockaddr *)&myserver, sizeof(myserver)) < 0) {
    //   if (errno != EADDRINUSE) ERR("bind fail.");
    //   listen_port++;
    //   myserver.sin_port = htons(listen_port);
    // }

    while (uv_tcp_bind(&server, (const sockaddr *)&addr, 0) == UV_EADDRINUSE) {
      listen_port++;
      uv_ip4_addr(ip, listen_port, &addr);
    }

    // if (listen(sock_fd_, kMaxConns) < 0) {
    //   ERR("listen fail.");
    // }

    if (uv_listen((uv_stream_t *)(&server), kMaxConns, on_new_connection)) {
      ERR("listen fail.");
    }

    // flags = 1;
    // setsockopt(sock_fd_, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
    this->port = listen_port;
    return listen_port;
  }
  void TestServer::Run() {
    uv_run(loop, UV_RUN_DEFAULT);
    //     epoll_event ev;
    //     int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    //     if (epoll_fd == -1) {
    //       ERR("epoll create fail.")
    //     }

    //     ev.events = EPOLLIN;
    //     ev.data.fd = sock_fd_;
    //     if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd_, &ev) == -1) {
    //       ERR("epoll_ctl sock_fd_ fail.");
    //     }
    // #define MAX_EVENT 100
    //     epoll_event events[MAX_EVENT];

    //     while (true) {
    //       int nfds = epoll_wait(epoll_fd, events, MAX_EVENT, -1);
    //       if (nfds == -1) {
    //         if (errno == EINTR) continue;
    //         ERR("error epoll_wait.");
    //       }
    //       for (int i = 0; i < nfds; i++) {
    //         if (events[i].data.fd == sock_fd_) {
    //           // new connection
    //           sockaddr_in remote_addr;
    //           socklen_t addr_len = sizeof(sockaddr_in);
    //           int conn_sock = accept(sock_fd_, (sockaddr *)&remote_addr, &addr_len);
    //           if (conn_sock == -1) {
    //             ERR("accept error.");
    //           }

    //           setnonblocking(conn_sock);
    //           ev.events = EPOLLIN;
    //           ev.data.fd = conn_sock;
    //           if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
    //             ERR("epoll_ctl conn_sock fail.");
    //           }
    //         } else {
    //           char buffer[1024];
    //           int fd = events[i].data.fd;
    //           auto rsize = read(fd, buffer, sizeof(buffer));
    //           if (rsize == -1) {
    //             ERR("read err.");
    //           }
    //           if (rsize == 0) {
    // #if defined(__linux__)
    // #  if (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 9))
    //             epoll_event _e;
    //             epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &_e);
    // #  else
    //             epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    // #  endif
    // #endif
    //           }
    //           if (rsize > 0) {
    //             // int curr_w = 0;
    //             // while (curr_w != rsize) curr_w += write(fd, buffer, rsize);
    //             on_msg(fd, buffer, rsize);
    //           }
    //         }
    //       }
    //     }
  }

  void TestServer::Send(uv_stream_t *client, char *data, int len) {
    // const char *curr_wr = data;
    // while (curr_wr - data != len) {
    //   curr_wr += write(fd, data, len);
    // }
    write_req_t *req = (write_req_t *)malloc(sizeof(write_req_t));
    req->buf = uv_buf_init(data, len);
    uv_write((uv_write_t *)req, client, &req->buf, 1, echo_write);
  }

  SimpleHttpServer::SimpleHttpServer() {
    this->on_msg = [this](uv_stream_t *cli, auto, auto) {
      char res[]
          = "HTTP/1.1 200 OK\r\n"
            "test-header: test_value\r\n"
            "\r\n"
            "This is a response";
      char *data = (char *)malloc(sizeof(res));
      memcpy(data, res, sizeof(res));
      this->Send(cli, data, sizeof(res));
    };
  }

}  // namespace smark_tests
