/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/* NaCl inter-module communication primitives. */

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "native_client/src/shared/platform/nacl_log.h"

/*
 * Gets an array of file descriptors stored in msg.
 * The fdv parameter must be an int array of kHandleCountMax elements.
 * GetRights() returns the number of file descriptors copied into fdv.
 */
static size_t GetRights(struct msghdr* msg, int* fdv) {
  struct cmsghdr* cmsg;
  size_t count = 0;
  for (cmsg = CMSG_FIRSTHDR(msg);
       cmsg != 0;
       cmsg = CMSG_NXTHDR(msg, cmsg)) {
    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
      while (CMSG_LEN((1 + count) * sizeof(int)) <= cmsg->cmsg_len) {
        *fdv++ = ((int *) CMSG_DATA(cmsg))[count];
        ++count;
      }
    }
  }
  return count;
}

/*
 * We keep these no-op implementations of SocketAddress-based
 * functions so that sigpipe_test continues to link.
 */
NaClHandle NaClBoundSocket(const NaClSocketAddress* address) {
  UNREFERENCED_PARAMETER(address);
  NaClLog(LOG_FATAL, "BoundSocket(): Not used on Linux\n");
  return -1;
}

int NaClSendDatagramTo(const NaClMessageHeader* message, int flags,
                       const NaClSocketAddress* name) {
  UNREFERENCED_PARAMETER(message);
  UNREFERENCED_PARAMETER(flags);
  UNREFERENCED_PARAMETER(name);
  NaClLog(LOG_FATAL, "SendDatagramTo(): Not used on Linux\n");
  return -1;
}

int NaClSocketPair(NaClHandle pair[2]) {
  /*
   * The read operation for a SOCK_SEQPACKET socket returns zero when the
   * remote peer closed the connection unlike a SOCK_DGRAM socket. Note
   * SOCK_SEQPACKET was introduced with Linux 2.6.4.
   */
  int rv = socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pair);
  if (rv != 0) {
    NaClLog(LOG_ERROR, "SocketPair: socketpair failed, errno %d\n", errno);
  }
  return rv;
}

int NaClClose(NaClHandle handle) {
  return close(handle);
}

int NaClSendDatagram(NaClHandle handle, const NaClMessageHeader* message,
                     int flags) {
  struct msghdr msg;
  unsigned char buf[CMSG_SPACE(NACL_HANDLE_COUNT_MAX * sizeof(int))];

  if (NACL_HANDLE_COUNT_MAX < message->handle_count) {
    errno = EMSGSIZE;
    return -1;
  }
  /*
   * The following assert was an earlier attempt to remember/check the
   * assumption that our struct IOVec -- which we must define to be
   * cross platform -- is compatible with struct iovec on *x systems.
   * The length field of IOVec was switched to be uint32_t at one point
   * to use concrete types, which introduced a problem on 64-bit systems.
   *
   * Clearly, the assert does not check a strong-enough condition,
   * since structure padding would make the two sizes the same.
   *
  assert(sizeof(struct iovec) == sizeof(NaClIOVec));
   *
   * Don't do this again!
   */

  if (!NaClMessageSizeIsValid(message)) {
    errno = EMSGSIZE;
    return -1;
  }

  msg.msg_iov = (struct iovec *) message->iov;
  msg.msg_iovlen = message->iov_length;
  msg.msg_name = 0;
  msg.msg_namelen = 0;

  if (0 < message->handle_count && message->handles != NULL) {
    struct cmsghdr* cmsg;
    int size = message->handle_count * sizeof(int);
    msg.msg_control = buf;
    msg.msg_controllen = CMSG_SPACE(size);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(size);
    memcpy(CMSG_DATA(cmsg), message->handles, size);
    msg.msg_controllen = cmsg->cmsg_len;
  } else {
    msg.msg_control = 0;
    msg.msg_controllen = 0;
  }
  msg.msg_flags = 0;
  return sendmsg(handle, &msg,
                 MSG_NOSIGNAL | ((flags & NACL_DONT_WAIT) ? MSG_DONTWAIT : 0));
}

int NaClReceiveDatagram(NaClHandle handle, NaClMessageHeader* message,
                        int flags) {
  struct msghdr msg;
  unsigned char buf[CMSG_SPACE(NACL_HANDLE_COUNT_MAX * sizeof(int))];
  int count;

  if (NACL_HANDLE_COUNT_MAX < message->handle_count) {
    errno = EMSGSIZE;
    return -1;
  }
  msg.msg_name = 0;
  msg.msg_namelen = 0;

  /*
   * Make sure we cannot receive more than 2**32-1 bytes.
   */
  if (!NaClMessageSizeIsValid(message)) {
    errno = EMSGSIZE;
    return -1;
  }

  msg.msg_iov = (struct iovec *) message->iov;
  msg.msg_iovlen = message->iov_length;
  if (0 < message->handle_count && message->handles != NULL) {
    msg.msg_control = buf;
    msg.msg_controllen = CMSG_SPACE(message->handle_count * sizeof(int));
  } else {
    msg.msg_control = 0;
    msg.msg_controllen = 0;
  }
  msg.msg_flags = 0;
  message->flags = 0;
  count = recvmsg(handle, &msg, (flags & NACL_DONT_WAIT) ? MSG_DONTWAIT : 0);
  if (0 <= count) {
    message->handle_count = GetRights(&msg, message->handles);
    if (msg.msg_flags & MSG_TRUNC) {
      message->flags |= NACL_MESSAGE_TRUNCATED;
    }
    if (msg.msg_flags & MSG_CTRUNC) {
      message->flags |= NACL_HANDLES_TRUNCATED;
    }
  }
  return count;
}
