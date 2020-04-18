/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * Here we are testing that return values and errno's are correctly received
 * from intercepted syscalls and parameter gets passed to syscall correctly.
 */
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <irt_syscalls.h>

int test_var;
struct sockaddr_in s_in;

#define REGISTER_WRAPPER_IN_GLIBC(name) do { \
    __nacl_irt_##name = __nacl_irt_##name##_wrap; \
  } while (0)
#define WRAP(name) __nacl_irt_##name##_wrap

  int WRAP(socket) (int domain, int type, int protocol, int* ret) {
    ++test_var;
    *ret = 51;
    return 0;
  }

  int WRAP(accept)(int sockfd, struct sockaddr* addr, socklen_t* addrlen,
                   int* ret) {
    ++test_var;
    *addrlen = 52;
    *ret = 23;
    return 0;
  }

  int WRAP(bind) (int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    ++test_var;
    return sockfd > 0 ? 0 : -53;
  }

  int WRAP(listen) (int sockfd, int backlog) {
    ++test_var;
    return sockfd > 0 ? 0 : -54;
  }

  int WRAP(connect) (int sockfd, const struct sockaddr* addr,
                     socklen_t addrlen) {
    ++test_var;
    assert((void*)&s_in == (void*)addr);
    return sockfd > 0 ? 0 : -55;
  }

  int WRAP(send) (int sockfd, const void *buf, size_t len, int flags,
                  int* ret) {
    ++test_var;
    *ret  = 56;
    return 0;
  }

  int WRAP(sendto) (int sockfd, const void *buf, size_t len, int flags,
                    const struct sockaddr *dest_addr, socklen_t addrlen,
                    int* ret) {
    ++test_var;
    *ret = 57;
    return 0;
  }

  int WRAP(sendmsg) (int sockfd, const struct msghdr *msg, int flags,
                     int* ret) {
    ++test_var;
    *ret = 58;
    return 0;
  }

  int WRAP(recv) (int sockfd, void *buf, size_t len, int flags, int* ret) {
    ++test_var;
    *ret = 59;
    return 0;
  }

  int WRAP(recvfrom) (int sockfd, void *buf, size_t len, int flags,
                      struct sockaddr *src_addr, socklen_t* addrlen,
                      int* ret) {
    ++test_var;
    *ret = 60;
    return 0;
  }

  int WRAP(recvmsg) (int sockfd, struct msghdr *msg, int flags, int* ret) {
    ++test_var;
    *ret = 61;
    return 0;
  }

  int WRAP(getsockname) (int sockfd, struct sockaddr *addr,
                         socklen_t* addrlen) {
    ++test_var;
    *addrlen = 62;
    return 0;
  }

  int WRAP(getpeername) (int sockfd, struct sockaddr *addr,
                         socklen_t* addrlen) {
    ++test_var;
    *addrlen = 63;
    return 0;
  }

  int WRAP(setsockopt) (int sockfd, int level, int optname, const void *optval,
                            socklen_t optlen) {
    ++test_var;
    return -64;
  }

  int WRAP(getsockopt) (int sockfd, int level, int optname, void *optval,
                            socklen_t* optlen) {
    ++test_var;
    return -65;
  }

  int WRAP(epoll_create) (int size, int *sd) {
    ++test_var;
    *sd = 5;
    return -66;
  }

  int WRAP(epoll_ctl) (int epfd, int op, int fd, struct epoll_event *event) {
    ++test_var;
    return -67;
  }

  int WRAP(epoll_wait) (int epfd, struct epoll_event *events,
                        int maxevents, int timeout, int* count) {
    ++test_var;
    *count = 0;
    return -68;
  }

  int WRAP(epoll_pwait) (int epfd, struct epoll_event *events,
                         int maxevents, int timeout, const sigset_t *sigmask,
                         size_t sigset_size, int *count) {
    ++test_var;
    *count = 0;
    return -69;
  }

  int WRAP(poll) (struct pollfd *fds, nfds_t nfds, int timeout, int *count) {
    ++test_var;
    *count = 0;
    return -70;
  }

  int WRAP(ppoll) (struct pollfd *fds, nfds_t nfds,
                   const struct timespec *timeout, const sigset_t *sigmask,
                   size_t sigset_size, int *count) {
    ++test_var;
    *count = 0;
    return -71;
  }

#define DIRSIZE 8192
#define PORT 0x1234
int main(int argc, char** argv) {
  REGISTER_WRAPPER_IN_GLIBC(socket);
  REGISTER_WRAPPER_IN_GLIBC(accept);
  REGISTER_WRAPPER_IN_GLIBC(bind);
  REGISTER_WRAPPER_IN_GLIBC(connect);
  REGISTER_WRAPPER_IN_GLIBC(listen);
  REGISTER_WRAPPER_IN_GLIBC(recv);
  REGISTER_WRAPPER_IN_GLIBC(recvmsg);
  REGISTER_WRAPPER_IN_GLIBC(recvfrom);
  REGISTER_WRAPPER_IN_GLIBC(send);
  REGISTER_WRAPPER_IN_GLIBC(sendto);
  REGISTER_WRAPPER_IN_GLIBC(sendmsg);
  REGISTER_WRAPPER_IN_GLIBC(setsockopt);
  REGISTER_WRAPPER_IN_GLIBC(getsockopt);
  REGISTER_WRAPPER_IN_GLIBC(getpeername);
  REGISTER_WRAPPER_IN_GLIBC(getsockname);
  REGISTER_WRAPPER_IN_GLIBC(epoll_create);
  REGISTER_WRAPPER_IN_GLIBC(epoll_ctl);
  REGISTER_WRAPPER_IN_GLIBC(epoll_wait);
  REGISTER_WRAPPER_IN_GLIBC(epoll_pwait);
  REGISTER_WRAPPER_IN_GLIBC(poll);
  REGISTER_WRAPPER_IN_GLIBC(ppoll);

  char dir[DIRSIZE];
  int sd, sd_current, ret;
  socklen_t addrlen;
  struct sockaddr_in pin;
  struct msghdr msg;
  struct epoll_event event;
  struct pollfd pollfds;
  struct timespec timeout;
  sigset_t sigmask;

  int prev_var = test_var;
  sd = socket(AF_INET, SOCK_STREAM, 0);
  assert(sd == 51);
  assert(test_var - prev_var == 1);

  memset(&s_in, 0, sizeof(s_in));
  s_in.sin_family = AF_INET;
  s_in.sin_addr.s_addr = INADDR_ANY;
  s_in.sin_port = htons(PORT);

  prev_var = test_var;
  ret = bind(sd, (struct sockaddr *) &s_in, sizeof(s_in));
  assert(ret == 0);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  errno = 0;
  bind(-1, (struct sockaddr *) &s_in, sizeof(s_in));
  ret = errno;
  assert(ret == -53);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  ret = listen(-1, 5);
  assert(errno == -54);
  assert(test_var - prev_var == 1);

  addrlen = sizeof(pin);
  prev_var = test_var;
  sd_current = accept(sd, (struct sockaddr *)  &pin, &addrlen);
  assert(addrlen == 52 && sd_current == 23);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  errno = 0;
  connect(sd_current, &s_in, addrlen);
  ret = errno;
  assert(ret == 0);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  ret = recv(sd_current, dir, sizeof(dir), 0);
  assert(ret == 59);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  ret = send(sd_current, dir, sizeof(dir), 0);
  assert(ret == 56);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  ret = sendto(sd_current, dir, sizeof(dir), 0, &s_in, 0);
  assert(ret == 57);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  ret = recvfrom(sd_current, dir, sizeof(dir), 0, &s_in, &addrlen);
  assert(ret == 60);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  ret = recvmsg(sd_current, &msg, 0);
  assert(ret == 61);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  ret = sendmsg(sd_current, &msg, 0);
  assert(ret == 58);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  errno = 0;
  getsockopt(sd, 0, 0, dir, &addrlen);
  ret = errno;
  assert(ret == -65);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  errno = 0;
  setsockopt(sd, 0, 0, dir, addrlen);
  ret = errno;
  assert(ret == -64);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  getpeername(sd, &s_in, &addrlen);
  assert(addrlen == 63);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  getsockname(sd, &s_in, &addrlen);
  assert(addrlen == 62);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  errno = 0;
  epoll_create(0);
  ret = errno;
  assert(ret == -66);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  errno = 0;
  epoll_ctl(0, 0, 0, &event);
  ret = errno;
  assert(ret == -67);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  errno = 0;
  epoll_wait(0, &event, 0, 0);
  ret = errno;
  assert(ret == -68);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  errno = 0;
  epoll_pwait(0, &event, 0, 0, &sigmask);
  ret = errno;
  assert(ret == -69);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  errno = 0;
  poll(&pollfds, 0, 0);
  ret = errno;
  assert(ret == -70);
  assert(test_var - prev_var == 1);

  prev_var = test_var;
  errno = 0;
  ppoll(&pollfds, 0, &timeout, &sigmask);
  ret = errno;
  assert(ret == -71);
  assert(test_var - prev_var == 1);

  return 0;
}

