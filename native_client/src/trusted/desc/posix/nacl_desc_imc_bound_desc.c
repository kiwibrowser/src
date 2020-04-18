/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.
 * Bound IMC descriptors.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_imc.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_bound_desc.h"

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"


static struct NaClDescVtbl const kNaClDescImcBoundDescVtbl;  /* fwd */

int NaClDescImcBoundDescCtor(struct NaClDescImcBoundDesc  *self,
                             NaClHandle                   h) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;
  if (!NaClDescCtor(basep)) {
    return 0;
  }
  self->h = h;
  basep->base.vtbl =
      (struct NaClRefCountVtbl const *) &kNaClDescImcBoundDescVtbl;
  return 1;
}

void NaClDescImcBoundDescDtor(struct NaClRefCount *vself) {
  struct NaClDescImcBoundDesc *self = (struct NaClDescImcBoundDesc *) vself;

  NaClClose(self->h);
  self->h = NACL_INVALID_HANDLE;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

int NaClDescImcBoundDescFstat(struct NaClDesc          *vself,
                              struct nacl_abi_stat     *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_mode = NACL_ABI_S_IFBOUNDSOCK;
  return 0;
}

int NaClDescImcBoundDescAcceptConn(struct NaClDesc *vself,
                                   struct NaClDesc **result) {
  /*
   * See NaClDescConnCapConnectAddr code in nacl_desc_conn_cap.c
   */
  struct NaClDescImcBoundDesc *self = (struct NaClDescImcBoundDesc *) vself;
  int                         retval;
  NaClHandle                  received_fd;
  struct NaClDescImcDesc      *peer;
  struct iovec                iovec;
  struct msghdr               accept_msg;
  struct cmsghdr              *cmsg;
  char                        data_buf[1];
  char                        control_buf[CMSG_SPACE_KHANDLE_COUNT_MAX_INTS];
  int                         received;

  assert(CMSG_SPACE(sizeof(int)) <= CMSG_SPACE_KHANDLE_COUNT_MAX_INTS);

  peer = malloc(sizeof(*peer));
  if (peer == NULL) {
    return -NACL_ABI_ENOMEM;
  }

  iovec.iov_base = data_buf;
  iovec.iov_len = sizeof(data_buf);
  accept_msg.msg_iov = &iovec;
  accept_msg.msg_iovlen = 1;
  accept_msg.msg_name = NULL;
  accept_msg.msg_namelen = 0;
  accept_msg.msg_control = control_buf;
  accept_msg.msg_controllen = sizeof(control_buf);

  NaClLog(3,
          ("NaClDescImcBoundDescAcceptConn(0x%08"NACL_PRIxPTR", "
           " h = %d\n"),
          (uintptr_t) vself,
          self->h);

  received_fd = NACL_INVALID_HANDLE;

  received = recvmsg(self->h, &accept_msg, 0);
  if (received != 1 || data_buf[0] != 'c') {
    NaClLog(LOG_ERROR,
            ("NaClDescImcBoundDescAcceptConn:"
             " could not receive connection message, errno %d\n"),
            errno);
    retval = -NACL_ABI_EIO;
    goto cleanup;
  }

  retval = 0;
  /*
   * If we got more than one fd in the message, we must clean up by
   * closing those extra descriptors.  Given that control_buf is
   * CMSG_SPACE(sizeof(int)) in size, this shouldn't actualy happen.
   *
   * We follow the principle of being strict in what we send and
   * tolerant in what we accept, and simply discard the extra
   * descriptors without signaling an error (other than a log
   * message).  This makes it easier to rev the protocol w/o
   * necessarily requiring a protocol version number bump.
   */
  for (cmsg = CMSG_FIRSTHDR(&accept_msg);
       cmsg != NULL;
       cmsg = CMSG_NXTHDR(&accept_msg, cmsg)) {
    if (cmsg->cmsg_level == SOL_SOCKET &&
        cmsg->cmsg_type == SCM_RIGHTS &&
        cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
      if (NACL_INVALID_HANDLE != received_fd) {
        int bad_fd;

        /* can only be true on 2nd time through */
        NaClLog(LOG_ERROR, ("NaClDescImcBoundDescAcceptConn: connection"
                            " message contains more than 1 descriptors?!?\n"));
        memcpy(&bad_fd, CMSG_DATA(cmsg), sizeof bad_fd);
        (void) close(bad_fd);
        /*
         * If we want to NOT be tolerant about what we receive, we could
         * uncomment this:
         *
         * retval = -NACL_ABI_EIO;
         */
        continue;
      }
      /*
       * We use memcpy() rather than assignment through a cast to avoid
       * strict-aliasing warnings
       */
      memcpy(&received_fd, CMSG_DATA(cmsg), sizeof received_fd);
    }
  }
  if (0 != retval) {
    goto cleanup;
  }
  if (NACL_INVALID_HANDLE == received_fd) {
    NaClLog(LOG_ERROR, ("NaClDescImcBoundDescAcceptConn: connection"
                        " message contains NO descriptors?!?\n"));
    retval = -NACL_ABI_EIO;
    goto cleanup;
  }

  if (NACL_OSX) {
    /*
     * Send a message to acknowledge that we received the socket FD
     * successfully.  This is part of a workaround for a Mac OS X
     * kernel bug.
     * See http://code.google.com/p/nativeclient/issues/detail?id=1796
     */
    ssize_t sent = send(received_fd, "a", 1, 0);
    if (sent != 1) {
      retval = -NACL_ABI_EIO;
      goto cleanup;
    }
  }

  if (!NaClDescImcDescCtor(peer, received_fd)) {
    retval = -NACL_ABI_EMFILE;
    goto cleanup;
  }
  received_fd = NACL_INVALID_HANDLE;

  *result = (struct NaClDesc *) peer;
  retval = 0;

cleanup:
  if (retval < 0) {
    if (received_fd != NACL_INVALID_HANDLE) {
      (void) close(received_fd);
    }
    free(peer);
  }
  return retval;
}

static struct NaClDescVtbl const kNaClDescImcBoundDescVtbl = {
  {
    NaClDescImcBoundDescDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescPReadNotImplemented,
  NaClDescPWriteNotImplemented,
  NaClDescImcBoundDescFstat,
  NaClDescFchdirNotImplemented,
  NaClDescFchmodNotImplemented,
  NaClDescFsyncNotImplemented,
  NaClDescFdatasyncNotImplemented,
  NaClDescFtruncateNotImplemented,
  NaClDescGetdentsNotImplemented,
  NaClDescExternalizeSizeNotImplemented,
  NaClDescExternalizeNotImplemented,
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
  NaClDescConnectAddrNotImplemented,
  NaClDescImcBoundDescAcceptConn,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
  NaClDescSetMetadata,
  NaClDescGetMetadata,
  NaClDescSetFlags,
  NaClDescGetFlags,
  NaClDescIsattyNotImplemented,
  NACL_DESC_BOUND_SOCKET,
};
