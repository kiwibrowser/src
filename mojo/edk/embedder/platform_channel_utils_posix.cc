// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/platform_channel_utils_posix.h"

#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>

#include <utility>

#include "base/containers/queue.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "build/build_config.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

#if !defined(OS_NACL)
#include <sys/uio.h>
#endif

#if !defined(SO_PEEK_OFF)
#define SO_PEEK_OFF 42
#endif

namespace mojo {
namespace edk {
namespace {

#if !defined(OS_NACL)
bool IsRecoverableError() {
  return errno == ECONNABORTED || errno == EMFILE || errno == ENFILE ||
         errno == ENOMEM || errno == ENOBUFS;
}

bool GetPeerEuid(InternalPlatformHandle handle, uid_t* peer_euid) {
  DCHECK(peer_euid);
#if defined(OS_MACOSX) || defined(OS_OPENBSD) || defined(OS_FREEBSD)
  uid_t socket_euid;
  gid_t socket_gid;
  if (getpeereid(handle.handle, &socket_euid, &socket_gid) < 0) {
    PLOG(ERROR) << "getpeereid " << handle.handle;
    return false;
  }
  *peer_euid = socket_euid;
  return true;
#else
  struct ucred cred;
  socklen_t cred_len = sizeof(cred);
  if (getsockopt(handle.handle, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) <
      0) {
    PLOG(ERROR) << "getsockopt " << handle.handle;
    return false;
  }
  if (static_cast<unsigned>(cred_len) < sizeof(cred)) {
    NOTREACHED() << "Truncated ucred from SO_PEERCRED?";
    return false;
  }
  *peer_euid = cred.uid;
  return true;
#endif
}

bool IsPeerAuthorized(InternalPlatformHandle peer_handle) {
  uid_t peer_euid;
  if (!GetPeerEuid(peer_handle, &peer_euid))
    return false;
  if (peer_euid != geteuid()) {
    DLOG(ERROR) << "Client euid is not authorised";
    return false;
  }
  return true;
}
#endif  // !defined(OS_NACL)

}  // namespace

// On Linux, |SIGPIPE| is suppressed by passing |MSG_NOSIGNAL| to
// |send()|/|sendmsg()|. (There is no way of suppressing |SIGPIPE| on
// |write()|/|writev().) On Mac, |SIGPIPE| is suppressed by setting the
// |SO_NOSIGPIPE| option on the socket.
//
// Performance notes:
//  - On Linux, we have to use |send()|/|sendmsg()| rather than
//    |write()|/|writev()| in order to suppress |SIGPIPE|. This is okay, since
//    |send()| is (slightly) faster than |write()| (!), while |sendmsg()| is
//    quite comparable to |writev()|.
//  - On Mac, we may use |write()|/|writev()|. Here, |write()| is considerably
//    faster than |send()|, whereas |sendmsg()| is quite comparable to
//    |writev()|.
//  - On both platforms, an appropriate |sendmsg()|/|writev()| is considerably
//    faster than two |send()|s/|write()|s.
//  - Relative numbers (minimum real times from 10 runs) for one |write()| of
//    1032 bytes, one |send()| of 1032 bytes, one |writev()| of 32+1000 bytes,
//    one |sendmsg()| of 32+1000 bytes, two |write()|s of 32 and 1000 bytes, two
//    |send()|s of 32 and 1000 bytes:
//    - Linux: 0.81 s, 0.77 s, 0.87 s, 0.89 s, 1.31 s, 1.22 s
//    - Mac: 2.21 s, 2.91 s, 2.98 s, 3.08 s, 3.59 s, 4.74 s

// Flags to use with calling |send()| or |sendmsg()| (see above).
#if defined(OS_MACOSX) || defined(OS_FUCHSIA)
const int kSendFlags = 0;
#else
const int kSendFlags = MSG_NOSIGNAL;
#endif

ssize_t PlatformChannelWrite(const ScopedInternalPlatformHandle& h,
                             const void* bytes,
                             size_t num_bytes) {
  DCHECK(h.is_valid());
  DCHECK(bytes);
  DCHECK_GT(num_bytes, 0u);

#if defined(OS_MACOSX) || defined(OS_NACL_NONSFI)
  // send() is not available under NaCl-nonsfi.
  return HANDLE_EINTR(write(h.get().handle, bytes, num_bytes));
#else
  return send(h.get().handle, bytes, num_bytes, kSendFlags);
#endif
}

ssize_t PlatformChannelWritev(const ScopedInternalPlatformHandle& h,
                              struct iovec* iov,
                              size_t num_iov) {
  DCHECK(h.is_valid());
  DCHECK(iov);
  DCHECK_GT(num_iov, 0u);

#if defined(OS_MACOSX)
  return HANDLE_EINTR(writev(h.get().handle, iov, static_cast<int>(num_iov)));
#else
  struct msghdr msg = {};
  msg.msg_iov = iov;
  msg.msg_iovlen = num_iov;
  return HANDLE_EINTR(sendmsg(h.get().handle, &msg, kSendFlags));
#endif
}

ssize_t PlatformChannelSendmsgWithHandles(
    const ScopedInternalPlatformHandle& h,
    struct iovec* iov,
    size_t num_iov,
    const std::vector<ScopedInternalPlatformHandle>& platform_handles) {
  DCHECK(iov);
  DCHECK_GT(num_iov, 0u);
  DCHECK(!platform_handles.empty());
  DCHECK_LE(platform_handles.size(), kPlatformChannelMaxNumHandles);

  char cmsg_buf[CMSG_SPACE(kPlatformChannelMaxNumHandles * sizeof(int))];
  struct msghdr msg = {};
  msg.msg_iov = iov;
  msg.msg_iovlen = num_iov;
  msg.msg_control = cmsg_buf;
  msg.msg_controllen = CMSG_LEN(platform_handles.size() * sizeof(int));
  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(platform_handles.size() * sizeof(int));
  for (size_t i = 0; i < platform_handles.size(); i++) {
    DCHECK(platform_handles[i].is_valid());
    reinterpret_cast<int*>(CMSG_DATA(cmsg))[i] =
        platform_handles[i].get().handle;
  }

  return HANDLE_EINTR(sendmsg(h.get().handle, &msg, kSendFlags));
}

ssize_t PlatformChannelRecvmsg(
    const ScopedInternalPlatformHandle& h,
    void* buf,
    size_t num_bytes,
    base::circular_deque<ScopedInternalPlatformHandle>* platform_handles,
    bool block) {
  DCHECK(buf);
  DCHECK_GT(num_bytes, 0u);
  DCHECK(platform_handles);

  struct iovec iov = {buf, num_bytes};
  char cmsg_buf[CMSG_SPACE(kPlatformChannelMaxNumHandles * sizeof(int))];
  struct msghdr msg = {};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof(cmsg_buf);

  ssize_t result =
      HANDLE_EINTR(recvmsg(h.get().handle, &msg, block ? 0 : MSG_DONTWAIT));
  if (result < 0)
    return result;

  // Success; no control messages.
  if (msg.msg_controllen == 0)
    return result;

  DCHECK(!(msg.msg_flags & MSG_CTRUNC));

  for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg;
       cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
      size_t payload_length = cmsg->cmsg_len - CMSG_LEN(0);
      DCHECK_EQ(payload_length % sizeof(int), 0u);
      size_t num_fds = payload_length / sizeof(int);
      const int* fds = reinterpret_cast<int*>(CMSG_DATA(cmsg));
      for (size_t i = 0; i < num_fds; i++) {
        platform_handles->push_back(
            ScopedInternalPlatformHandle(InternalPlatformHandle(fds[i])));
        DCHECK(platform_handles->back().is_valid());
      }
    }
  }

  return result;
}

bool ServerAcceptConnection(const ScopedInternalPlatformHandle& server_handle,
                            ScopedInternalPlatformHandle* connection_handle,
                            bool check_peer_user) {
  DCHECK(server_handle.is_valid());
  connection_handle->reset();
#if defined(OS_NACL)
  NOTREACHED();
  return false;
#else
  ScopedInternalPlatformHandle accept_handle(InternalPlatformHandle(
      HANDLE_EINTR(accept(server_handle.get().handle, NULL, 0))));
  if (!accept_handle.is_valid())
    return IsRecoverableError();

  // Verify that the IPC channel peer is running as the same user.
  if (check_peer_user && !IsPeerAuthorized(accept_handle.get())) {
    return true;
  }

  if (!base::SetNonBlocking(accept_handle.get().handle)) {
    PLOG(ERROR) << "base::SetNonBlocking() failed "
                << accept_handle.get().handle;
    // It's safe to keep listening on |server_handle| even if the attempt to set
    // O_NONBLOCK failed on the client fd.
    return true;
  }

  *connection_handle = std::move(accept_handle);
  return true;
#endif  // defined(OS_NACL)
}

}  // namespace edk
}  // namespace mojo
