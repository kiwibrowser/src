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
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "native_client/src/public/imc_types.h"
#include "native_client/src/shared/platform/nacl_log.h"


/*
 * TODO(bsy,bradnelson): remove SIGPIPE_FIX.  It is needed for future
 * testing because our test framework appears to not see the SIGPIPE
 * on OSX when the fix is not in place.  We've tracked it down to the
 * Python subprocess module, where if we manually run
 * subprocess.Popen('...path-to-sigpipe_test...') the SIGPIPE doesn't
 * actually occur(!); however, when running the same sigpipe_test
 * executable from the shell it's apparent that the SIGPIPE *does*
 * occur.  Presumably it's some weird code path in subprocess that is
 * leaving the signal handler for SIGPIPE as SIG_IGN rather than
 * SIG_DFL.  Unfortunately, we could not create a simpler test of our
 * test infrastructure (writing to a pipe that's closed) -- perhaps
 * the multithreaded nature of sigpipe_test is involved.
 *
 * In production code, SIGPIPE_FIX should be 1.  The old behavior is
 * only needed to help us track down the problem in python.
 */
#define SIGPIPE_FIX           1

/*
 * The code guarded by SIGPIPE_FIX has been found to still raise
 * SIGPIPE in certain situations. Until we can boil this down to a
 * small test case and, possibly, file a bug against the OS, we need
 * to forcibly suppress these signals.
 */
#define SIGPIPE_ALT_FIX       1

#if SIGPIPE_ALT_FIX
# include <signal.h>
#endif  /* SIGPIPE_ALT_FIX */

#include <algorithm>


/*
 * The number of recvmsg retries to perform to determine --
 * heuristically, unfortunately -- if the remote end of the socketpair
 * had actually closed.  This is a (new) hacky workaround for an OSX
 * blemish that replaces the older, buggier workaround.
 */
static const int kRecvMsgRetries = 8;

/*
 * The maximum number of NaClIOVec elements sent by SendDatagram(). Plus one for
 * NaClInternalHeader with the descriptor data bytes.
 */
static const size_t kIovLengthMax = NACL_ABI_IMC_IOVEC_MAX + 1;

/*
 * The IMC datagram header followed by a message_bytes of data sent over the
 * a stream-oriented socket. We need to use stream-oriented socket for OS X
 * since it doesn't support file descriptor transfer over SOCK_DGRAM socket
 * like Linux.
 */
struct Header {
  /*
   * The total bytes of data in the IMC datagram excluding the size of
   * Header.
   */
  size_t message_bytes;
  /* The total number of handles to be transferred with IMC datagram. */
  size_t handle_count;
};


/*
 * Gets an array of file descriptors stored in msg.
 * The fdv parameter must be an int array of kHandleCountMax elements.
 * GetRights() returns the number of file descriptors copied into fdv.
 */
static size_t GetRights(struct msghdr* msg, int* fdv) {
  struct cmsghdr* cmsg;
  size_t count = 0;
  if (msg->msg_controllen == 0) {
    return 0;
  }
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
 * Skips the specified length of octets when reading from a handle. Skipped
 * octets are discarded.
 * On success, true is returned. On error, false is returned.
 */
static bool SkipFile(int handle, size_t length) {
  while (0 < length) {
    char scratch[1024];
    size_t count = std::min(sizeof scratch, length);
    count = read(handle, scratch, count);
    if ((ssize_t) count == -1 || count == 0) {
      return false;
    }
    length -= count;
  }
  return true;
}

#if SIGPIPE_ALT_FIX
/*
 * TODO(kbr): move this to an Init() function so it isn't called all
 * the time.
 */
static bool IgnoreSIGPIPE() {
  struct sigaction sa;
  sigset_t mask;
  sigemptyset(&mask);
  sa.sa_handler = SIG_IGN;
  sa.sa_mask = mask;
  sa.sa_flags = 0;
  return sigaction(SIGPIPE, &sa, NULL) == 0;
}
#endif

/*
 * We keep these no-op implementations of SocketAddress-based
 * functions so that sigpipe_test continues to link.
 */
NaClHandle NaClBoundSocket(const NaClSocketAddress* address) {
  UNREFERENCED_PARAMETER(address);
  NaClLog(LOG_FATAL, "BoundSocket(): Not used on OSX\n");
  return -1;
}

int NaClSendDatagramTo(const NaClMessageHeader* message, int flags,
                       const NaClSocketAddress* name) {
  UNREFERENCED_PARAMETER(message);
  UNREFERENCED_PARAMETER(flags);
  UNREFERENCED_PARAMETER(name);
  NaClLog(LOG_FATAL, "SendDatagramTo(): Not used on OSX\n");
  return -1;
}

int NaClSocketPair(NaClHandle pair[2]) {
  int result = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
  if (result == 0) {
#if SIGPIPE_FIX
    int nosigpipe = 1;
#endif
#if SIGPIPE_ALT_FIX
    if (!IgnoreSIGPIPE()) {
      close(pair[0]);
      close(pair[1]);
      return -1;
    }
#endif
#if SIGPIPE_FIX
    if (0 != setsockopt(pair[0], SOL_SOCKET, SO_NOSIGPIPE,
                        &nosigpipe, sizeof nosigpipe) ||
        0 != setsockopt(pair[1], SOL_SOCKET, SO_NOSIGPIPE,
                        &nosigpipe, sizeof nosigpipe)) {
      close(pair[0]);
      close(pair[1]);
      return -1;
    }
#endif
  }
  return result;
}

int NaClClose(NaClHandle handle) {
  return close(handle);
}

int NaClSendDatagram(NaClHandle handle, const NaClMessageHeader* message,
                     int flags) {
  struct msghdr msg;
  struct iovec vec[kIovLengthMax + 1];
  unsigned char buf[CMSG_SPACE_KHANDLE_COUNT_MAX_INTS];
  Header header = { 0, 0 };
  int result;
  size_t i;
  UNREFERENCED_PARAMETER(flags);

  assert(CMSG_SPACE(NACL_HANDLE_COUNT_MAX * sizeof(int))
         <= CMSG_SPACE_KHANDLE_COUNT_MAX_INTS);

  /*
   * The following assert was an earlier attempt to remember/check the
   * assumption that our struct NaClIOVec -- which we must define to be
   * cross platform -- is compatible with struct iovec on *x systems.
   * The length field of NaClIOVec was switched to be uint32_t at oen point
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

  if (NACL_HANDLE_COUNT_MAX < message->handle_count ||
      kIovLengthMax < message->iov_length) {
    errno = EMSGSIZE;
    return -1;
  }

  memmove(&vec[1], message->iov, sizeof(NaClIOVec) * message->iov_length);

  msg.msg_name = 0;
  msg.msg_namelen = 0;
  msg.msg_iov = vec;
  msg.msg_iovlen = 1 + message->iov_length;
  if (0 < message->handle_count && message->handles != NULL) {
    struct cmsghdr *cmsg;
    int size = message->handle_count * sizeof(int);
    msg.msg_control = buf;
    msg.msg_controllen = CMSG_SPACE(size);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(size);
    memcpy(CMSG_DATA(cmsg), message->handles, size);
    msg.msg_controllen = cmsg->cmsg_len;
    header.handle_count = message->handle_count;
  } else {
    msg.msg_control = 0;
    msg.msg_controllen = 0;
  }
  msg.msg_flags = 0;

  /*
   * Send data with the header atomically. Note to send file descriptors we need
   * to send at least one byte of data.
   */
  for (i = 0; i < message->iov_length; ++i) {
    header.message_bytes += message->iov[i].length;
  }
  vec[0].iov_base = &header;
  vec[0].iov_len = sizeof header;
  result = sendmsg(handle, &msg, 0);
  if (result == -1) {
    return -1;
  }
  if ((size_t) result < sizeof header) {
    errno = EMSGSIZE;
    return -1;
  }
  return result - sizeof header;
}

int NaClReceiveDatagram(NaClHandle handle, NaClMessageHeader* message,
                        int flags) {
  struct msghdr msg;
  struct iovec vec[kIovLengthMax];
  unsigned char buf[CMSG_SPACE_KHANDLE_COUNT_MAX_INTS];
  struct Header header;
  struct iovec header_vec = { &header, sizeof header };
  int count;
  int retry_count;
  size_t handle_count = 0;
  size_t buffer_bytes = 0;
  size_t i;

  assert(CMSG_SPACE(NACL_HANDLE_COUNT_MAX * sizeof(int))
         <= CMSG_SPACE_KHANDLE_COUNT_MAX_INTS);

  if (NACL_HANDLE_COUNT_MAX < message->handle_count ||
      kIovLengthMax < message->iov_length) {
    errno = EMSGSIZE;
    return -1;
  }

  /*
   * The following assert was an earlier attempt to remember/check the
   * assumption that our struct NaClIOVec -- which we must define to be
   * cross platform -- is compatible with struct iovec on *x systems.
   * The length field of NaClIOVec was switched to be uint32_t at oen point
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

  message->flags = 0;
  /* Receive the header of the message and handles first. */
  msg.msg_iov = &header_vec;
  msg.msg_iovlen = 1;
  msg.msg_name = 0;
  msg.msg_namelen = 0;
  if (0 < message->handle_count && message->handles != NULL) {
    msg.msg_control = buf;
    msg.msg_controllen = CMSG_SPACE(message->handle_count * sizeof(int));
  } else {
    msg.msg_control = 0;
    msg.msg_controllen = 0;
  }
  msg.msg_flags = 0;
  for (retry_count = 0; retry_count < kRecvMsgRetries; ++retry_count) {
    if (0 != (count = recvmsg(handle, &msg,
                              (flags & NACL_DONT_WAIT) ? MSG_DONTWAIT : 0))) {
      break;
    }
  }
  if (0 != retry_count && kRecvMsgRetries != retry_count) {
    printf("OSX_BLEMISH_HEURISTIC: retry_count = %d, count = %d\n",
           retry_count, count);
  }
  if (0 < count) {
    handle_count = GetRights(&msg, message->handles);
  }
  if (count != sizeof header) {
    while (0 < handle_count) {
      /*
       * Note if the sender has sent one end of a socket pair here,
       * ReceiveDatagram() for that socket will result in a zero length read
       * return henceforth.
       */
      close(message->handles[--handle_count]);
    }
    if (count == 0) {
      message->handle_count = 0;
      return 0;
    }
    if (count != -1) {
      /*
       * TODO(shiki): We should call recvmsg() again here since it could get to
       * wake up with a partial header since the SOCK_STREAM socket does not
       * required to maintain message boundaries.
       */
      errno = EMSGSIZE;
    }
    return -1;
  }

  message->handle_count = handle_count;

  /*
   * OS X seems not to set the MSG_CTRUNC flag in msg.msg_flags as we expect,
   * and we don't rely on it.
   */
  if (message->handle_count < header.handle_count) {
    message->flags |= NACL_HANDLES_TRUNCATED;
  }

  if (header.message_bytes == 0) {
    return 0;
  }

  /* Update message->iov to receive just message_bytes. */
  memmove(vec, message->iov, sizeof(NaClIOVec) * message->iov_length);
  msg.msg_iov = vec;
  msg.msg_iovlen = message->iov_length;
  for (i = 0; i < message->iov_length; ++i) {
    buffer_bytes += vec[i].iov_len;
    if (header.message_bytes <= buffer_bytes) {
      vec[i].iov_len -= buffer_bytes - header.message_bytes;
      buffer_bytes = header.message_bytes;
      msg.msg_iovlen = i + 1;
      break;
    }
  }
  if (buffer_bytes < header.message_bytes) {
    message->flags |= NACL_MESSAGE_TRUNCATED;
  }

  /* Receive the sent data. */
  msg.msg_name = 0;
  msg.msg_namelen = 0;

  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;
  for (retry_count = 0; retry_count < kRecvMsgRetries; ++retry_count) {
    /*
     * We have to pass MSG_WAITALL here, because we have already consumed
     * the header.  If we returned EAGAIN here, subsequent calls would read
     * data as a header, and much hilarity would ensue.
     */
    if (0 != (count = recvmsg(handle, &msg, MSG_WAITALL))) {
      break;
    }
  }
  if (0 != retry_count && kRecvMsgRetries != retry_count) {
    printf("OSX_BLEMISH_HEURISTIC (2): retry_count = %d, count = %d\n",
           retry_count, count);
  }
  if (0 < count) {
    /*
     * If the caller requested fewer bytes than the message contained, we need
     * to read the remaining bytes, discard them, and report message truncated.
     */
    if ((size_t) count < header.message_bytes) {
      if (!SkipFile(handle, header.message_bytes - count)) {
        return -1;
      }
      message->flags |= NACL_MESSAGE_TRUNCATED;
    }
  }
  return count;
}
