/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.
 * Connection capabilities.
 */

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_conn_cap.h"
#include "native_client/src/trusted/desc/nacl_desc_imc.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

static struct NaClDescVtbl const kNaClDescConnCapFdVtbl;  /* fwd */

static int NaClDescConnCapFdSubclassCtor(struct NaClDescConnCapFd  *self,
                                         NaClHandle                endpt) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  self->connect_fd = endpt;
  basep->base.vtbl = (struct NaClRefCountVtbl const *) &kNaClDescConnCapFdVtbl;
  return 1;
}

int NaClDescConnCapFdCtor(struct NaClDescConnCapFd  *self,
                          NaClHandle                endpt) {
  struct NaClDesc *basep = (struct NaClDesc *) self;
  int rv;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;
  if (!NaClDescCtor(basep)) {
    return 0;
  }
  rv = NaClDescConnCapFdSubclassCtor(self, endpt);
  if (!rv) {
    (*NACL_VTBL(NaClRefCount, basep)->Dtor)((struct NaClRefCount *) self);
  }
  return rv;
}

static void NaClDescConnCapFdDtor(struct NaClRefCount *vself) {
  struct NaClDescConnCapFd *self = (struct NaClDescConnCapFd *) vself;

  (void) NaClClose(self->connect_fd);
  self->connect_fd = NACL_INVALID_HANDLE;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
  return;
}

static int NaClDescConnCapFdFstat(struct NaClDesc       *vself,
                                  struct nacl_abi_stat  *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_mode = NACL_ABI_S_IFSOCKADDR | NACL_ABI_S_IRWXU;
  return 0;
}

static int NaClDescConnCapFdExternalizeSize(struct NaClDesc *vself,
                                            size_t          *nbytes,
                                            size_t          *nhandles) {
  int rv;

  rv = NaClDescExternalizeSize(vself, nbytes, nhandles);
  if (0 != rv) {
    return rv;
  }

  *nhandles += 1;
  return 0;
}

static int NaClDescConnCapFdExternalize(struct NaClDesc          *vself,
                                        struct NaClDescXferState *xfer) {
  struct NaClDescConnCapFd    *self;
  int rv;

  rv = NaClDescExternalize(vself, xfer);
  if (0 != rv) {
    return rv;
  }
  self = (struct NaClDescConnCapFd *) vself;
  *xfer->next_handle++ = self->connect_fd;

  return 0;
}

static int NaClDescConnCapFdConnectAddr(struct NaClDesc *vself,
                                        struct NaClDesc **out_desc) {
  struct NaClDescConnCapFd  *self = (struct NaClDescConnCapFd *) vself;
  NaClHandle                sock_pair[2];
  struct NaClDescImcDesc    *connected_socket;
  char                      control_buf[CMSG_SPACE_KHANDLE_COUNT_MAX_INTS];
  struct iovec              iovec;
  struct msghdr             connect_msg;
  struct cmsghdr            *cmsg;
  int                       sent;
  int                       retval;

  assert(CMSG_SPACE(sizeof(int)) <= CMSG_SPACE_KHANDLE_COUNT_MAX_INTS);

  sock_pair[0] = NACL_INVALID_HANDLE;
  sock_pair[1] = NACL_INVALID_HANDLE;
  connected_socket = (struct NaClDescImcDesc *) NULL;

  retval = -NACL_ABI_EINVAL;

  if (0 != NaClSocketPair(sock_pair)) {
    retval = -NACL_ABI_EMFILE;
    goto cleanup;
  }

  iovec.iov_base = "c";
  iovec.iov_len = 1;
  connect_msg.msg_iov = &iovec;
  connect_msg.msg_iovlen = 1;
  connect_msg.msg_name = NULL;
  connect_msg.msg_namelen = 0;
  connect_msg.msg_control = control_buf;
  connect_msg.msg_controllen = sizeof(control_buf);
  connect_msg.msg_flags = 0;

  cmsg = CMSG_FIRSTHDR(&connect_msg);
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  /*
   * We use memcpy() rather than assignment through a cast to avoid
   * strict-aliasing warnings
   */
  memcpy(CMSG_DATA(cmsg), &sock_pair[0], sizeof(int));
  /* Set msg_controllen to the actual size of the cmsg. */
  connect_msg.msg_controllen = cmsg->cmsg_len;

  sent = sendmsg(self->connect_fd, &connect_msg, 0);
  if (1 != sent) {
    retval = -NACL_ABI_EIO;
    goto cleanup;
  }

  if (NACL_OSX) {
    /*
     * Mac OS X has a kernel bug in which a socket descriptor that is
     * referenced only from the message queue of another socket can
     * get garbage collected.  This causes the socket descriptor not
     * to work properly.  To work around this, we don't close our
     * reference to the socket until we receive an acknowledgement
     * that it has been successfully received.
     *
     * We cannot receive the acknowledgement through self->connect_fd
     * because this FD could be shared between multiple processes.  So
     * we receive the acknowledgement through the socket pair that we
     * have just created.
     *
     * However, this creates a risk that we are left hanging if the
     * other process dies after our sendmsg() call, because we are
     * holding on to the socket that it would use to send the ack.  To
     * avoid this problem, we use poll() so that we will be notified
     * if self->connect_fd becomes unwritable.
     * TODO(mseaborn): Add a test case to simulate that scenario.
     *
     * See http://code.google.com/p/nativeclient/issues/detail?id=1796
     *
     * Note that we are relying on a corner case of poll() here.
     * Using POLLHUP in "events" is not meaningful on Linux, which is
     * documented as ignoring POLLHUP as an input argument and will
     * return POLLHUP in "revents" even if it not present in "events".
     * On Mac OS X, however, passing events == 0 does not work if we
     * want to get POLLHUP.  We are in the unusual situation of
     * waiting for a socket to become *un*writable.
     */
    struct pollfd poll_fds[2];
    poll_fds[0].fd = self->connect_fd;
    poll_fds[0].events = POLLHUP;
    poll_fds[1].fd = sock_pair[1];
    poll_fds[1].events = POLLIN;
    if (poll(poll_fds, 2, -1) < 0) {
      NaClLog(LOG_ERROR,
              "NaClDescConnCapFdConnectAddr: poll() failed, errno %d\n", errno);
      retval = -NACL_ABI_EIO;
      goto cleanup;
    }
    /*
     * It is not necessarily an error if POLLHUP fires on
     * self->connect_fd: The other process could have done
     * imc_accept(S) and then closed S.  This means it will have
     * received sock_pair[0] successfully, so we can close our
     * reference to sock_pair[0] and then receive the ack.
     * TODO(mseaborn): Add a test case to cover this scenario.
     */
  }

  (void) NaClClose(sock_pair[0]);
  sock_pair[0] = NACL_INVALID_HANDLE;

  if (NACL_OSX) {
    /* Receive the acknowledgement.  We do not expect this to block. */
    char ack_buffer[1];
    ssize_t received = recv(sock_pair[1], ack_buffer, sizeof(ack_buffer), 0);
    if (received != 1 || ack_buffer[0] != 'a') {
      retval = -NACL_ABI_EIO;
      goto cleanup;
    }
  }

  connected_socket = malloc(sizeof(*connected_socket));
  if (NULL == connected_socket ||
      !NaClDescImcDescCtor(connected_socket, sock_pair[1])) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  sock_pair[1] = NACL_INVALID_HANDLE;

  *out_desc = (struct NaClDesc *) connected_socket;
  connected_socket = NULL;
  retval = 0;

cleanup:
  NaClSafeCloseNaClHandle(sock_pair[0]);
  NaClSafeCloseNaClHandle(sock_pair[1]);
  free(connected_socket);

  return retval;
}

static int NaClDescConnCapFdAcceptConn(struct NaClDesc  *vself,
                                       struct NaClDesc  **out_desc) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(out_desc);

  NaClLog(LOG_ERROR, "NaClDescConnCapFdAcceptConn: not IMC\n");
  return -NACL_ABI_EINVAL;
}

static struct NaClDescVtbl const kNaClDescConnCapFdVtbl = {
  {
    NaClDescConnCapFdDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescPReadNotImplemented,
  NaClDescPWriteNotImplemented,
  NaClDescConnCapFdFstat,
  NaClDescFchdirNotImplemented,
  NaClDescFchmodNotImplemented,
  NaClDescFsyncNotImplemented,
  NaClDescFdatasyncNotImplemented,
  NaClDescFtruncateNotImplemented,
  NaClDescGetdentsNotImplemented,
  NaClDescConnCapFdExternalizeSize,
  NaClDescConnCapFdExternalize,
  NaClDescLockNotImplemented,
  NaClDescTryLockNotImplemented,
  NaClDescUnlockNotImplemented,
  NaClDescWaitNotImplemented,
  NaClDescTimedWaitAbsNotImplemented,
  NaClDescSignalNotImplemented,
  NaClDescBroadcastNotImplemented,
  NaClDescSendMsgNotImplemented,
  NaClDescRecvMsgNotImplemented,
  NaClDescLowLevelSendMsgNotImplemented,
  NaClDescLowLevelRecvMsgNotImplemented,
  NaClDescConnCapFdConnectAddr,
  NaClDescConnCapFdAcceptConn,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
  NaClDescSetMetadata,
  NaClDescGetMetadata,
  NaClDescSetFlags,
  NaClDescGetFlags,
  NaClDescIsattyNotImplemented,
  NACL_DESC_CONN_CAP_FD,
};

int NaClDescConnCapFdInternalize(
    struct NaClDesc               **out_desc,
    struct NaClDescXferState      *xfer) {
  struct NaClDescConnCapFd *conn_cap;
  int rv;

  conn_cap = malloc(sizeof(*conn_cap));
  if (NULL == conn_cap) {
    return -NACL_ABI_ENOMEM;
  }
  if (!NaClDescInternalizeCtor(&conn_cap->base, xfer)) {
    free(conn_cap);
    conn_cap = NULL;
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  if (xfer->next_handle == xfer->handle_buffer_end) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }
  rv = NaClDescConnCapFdSubclassCtor(conn_cap, *xfer->next_handle);
  if (!rv) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  *xfer->next_handle++ = NACL_INVALID_HANDLE;
  *out_desc = &conn_cap->base;
  rv = 0;
 cleanup:
  if (rv < 0) {
    NaClDescSafeUnref((struct NaClDesc *) conn_cap);
  }
  return rv;
}
