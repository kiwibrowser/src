/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_LINUX_SYSCALLS_INCLUDE_SYS_SOCKET_H_
#define NATIVE_CLIENT_SRC_PUBLIC_LINUX_SYSCALLS_INCLUDE_SYS_SOCKET_H_

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AF_UNIX 1
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_SEQPACKET 5

#define SOL_SOCKET 1

#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

#define SCM_RIGHTS 1

#define MSG_CTRUNC 0x8
#define MSG_TRUNC 0x20
#define MSG_DONTWAIT 0x40
#define MSG_NOSIGNAL 0x4000

typedef uint32_t socklen_t;

struct iovec {
  void *iov_base;
  size_t iov_len;
};

struct msghdr {
  void *msg_name;
  socklen_t msg_namelen;

  struct iovec *msg_iov;
  size_t msg_iovlen;

  void *msg_control;
  size_t msg_controllen;

  int msg_flags;
};

struct cmsghdr {
  socklen_t cmsg_len;
  int cmsg_level;
  int cmsg_type;
};

ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
int shutdown(int sockfd, int how);
int socketpair(int domain, int type, int protocol, int sv[2]);

#define CMSG_ALIGN(len) (((len) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1))
#define CMSG_SPACE(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(len))
#define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#define CMSG_DATA(cmsg) \
    ((unsigned char *) ((uintptr_t) (cmsg) + sizeof(struct cmsghdr)))

#define CMSG_FIRSTHDR(mhdr) __cmsg_firsthdr(mhdr)
static __inline__ struct cmsghdr *__cmsg_firsthdr(struct msghdr *__mhdr) {
  if (__mhdr->msg_controllen < sizeof(struct cmsghdr)) {
    return (struct cmsghdr *) 0;
  }
  return (struct cmsghdr *) __mhdr->msg_control;
}

#define CMSG_NXTHDR(mhdr, cmsg) __cmsg_nxthdr(mhdr, cmsg)
static __inline__ struct cmsghdr *__cmsg_nxthdr(struct msghdr *__mhdr,
                                                struct cmsghdr *__cmsg) {
  struct cmsghdr *__next =
      (struct cmsghdr *) ((uintptr_t) __cmsg + CMSG_ALIGN(__cmsg->cmsg_len));
  if ((uintptr_t) (__next + 1) >
      (uintptr_t) __mhdr->msg_control + __mhdr->msg_controllen) {
    return (struct cmsghdr *) 0;
  }
  return __next;
}

#ifdef __cplusplus
}
#endif

#endif
