/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sys_imc.h"

#include <string.h>

#include "native_client/src/trusted/desc/nacl_desc_imc.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_shm.h"
#include "native_client/src/trusted/desc/nacl_desc_invalid.h"
#include "native_client/src/trusted/desc/nrd_xfer.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


static int const kKnownInvalidDescNumber = -1;

int32_t NaClSysImcMakeBoundSock(struct NaClAppThread *natp,
                                uint32_t             descs_addr) {
  /*
   * Create a bound socket descriptor and a socket address descriptor.
   */
  struct NaClApp              *nap = natp->nap;
  int32_t                     retval = -NACL_ABI_EINVAL;
  struct NaClDesc             *pair[2];
  int32_t                     usr_pair[2];

  /* This syscall is not used in Chromium so is disabled by default. */
  if (!NaClAclBypassChecks) {
    return -NACL_ABI_EACCES;
  }

  NaClLog(3,
          ("Entered NaClSysImcMakeBoundSock(0x%08"NACL_PRIxPTR","
           " 0x%08"NACL_PRIx32")\n"),
          (uintptr_t) natp, descs_addr);

  retval = NaClCommonDescMakeBoundSock(pair);
  if (0 != retval) {
    goto cleanup;
  }

  usr_pair[0] = NaClAppSetDescAvail(nap, pair[0]);
  usr_pair[1] = NaClAppSetDescAvail(nap, pair[1]);
  if (!NaClCopyOutToUser(nap, descs_addr, usr_pair, sizeof usr_pair)) {
    /*
     * NB: The descriptors were briefly observable to untrusted code
     * in this window, even though the syscall had not returned yet,
     * and another thread which guesses their numbers could actually
     * use them, so the NaClDescSafeUnref inside NaClAppSetDesc below
     * might not actually deallocate right away.  To avoid this, we
     * could grab the descriptor lock and hold it until after the
     * copyout is done, but that imposes an ordering between the
     * descriptor lock and the VM lock which can cause problems
     * elsewhere.
     */
    NaClAppSetDesc(nap, usr_pair[0], NULL);
    NaClAppSetDesc(nap, usr_pair[1], NULL);
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  retval = 0;

cleanup:
  return retval;
}

int32_t NaClSysImcAccept(struct NaClAppThread  *natp,
                         int                   d) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *ndp;

  NaClLog(3, "Entered NaClSysImcAccept(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t) natp, d);

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
  } else {
    struct NaClDesc *result_desc;
    retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
              AcceptConn)(ndp, &result_desc);
    if (retval == 0) {
      retval = NaClAppSetDescAvail(nap, result_desc);
    }
    NaClDescUnref(ndp);
  }

  return retval;
}

int32_t NaClSysImcConnect(struct NaClAppThread *natp,
                          int                  d) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *ndp;

  NaClLog(3, "Entered NaClSysImcConnectAddr(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t) natp, d);

  /* This syscall is not used in Chromium so is disabled by default. */
  if (!NaClAclBypassChecks) {
    return -NACL_ABI_EACCES;
  }

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
  } else {
    struct NaClDesc *result;
    retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
              ConnectAddr)(ndp, &result);
    if (retval == 0) {
      retval = NaClAppSetDescAvail(nap, result);
    }
    NaClDescUnref(ndp);
  }

  return retval;
}

/*
 * This function converts addresses from user addresses to system
 * addresses, copying into kernel space as needed to avoid TOCvTOU
 * races, then invokes the descriptor's SendMsg() method.
 */
int32_t NaClSysImcSendmsg(struct NaClAppThread *natp,
                          int                  d,
                          uint32_t             nanimhp,
                          int                  flags) {
  struct NaClApp                *nap = natp->nap;
  int32_t                       retval = -NACL_ABI_EINVAL;
  ssize_t                       ssize_retval;
  uintptr_t                     sysaddr;
  /* copy of user-space data for validation */
  struct NaClAbiNaClImcMsgHdr   kern_nanimh;
  struct NaClAbiNaClImcMsgIoVec kern_naiov[NACL_ABI_IMC_IOVEC_MAX];
  struct NaClImcMsgIoVec        kern_iov[NACL_ABI_IMC_IOVEC_MAX];
  int32_t                       usr_desc[NACL_ABI_IMC_USER_DESC_MAX];
  /* kernel-side representatin of descriptors */
  struct NaClDesc               *kern_desc[NACL_ABI_IMC_USER_DESC_MAX];
  struct NaClImcTypedMsgHdr     kern_msg_hdr;
  struct NaClDesc               *ndp;
  size_t                        i;

  NaClLog(3,
          ("Entered NaClSysImcSendmsg(0x%08"NACL_PRIxPTR", %d,"
           " 0x%08"NACL_PRIx32", 0x%x)\n"),
          (uintptr_t) natp, d, nanimhp, flags);

  if (!NaClCopyInFromUser(nap, &kern_nanimh, nanimhp, sizeof kern_nanimh)) {
    NaClLog(4, "NaClImcMsgHdr not in user address space\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup_leave;
  }
  /* copy before validating contents */

  /*
   * Some of these checks duplicate checks that will be done in the
   * nrd xfer library, but it is better to check before doing the
   * address translation of memory/descriptor vectors if those vectors
   * might be too long.  Plus, we need to copy and validate vectors
   * for TOCvTOU race protection, and we must prevent overflows.  The
   * nrd xfer library's checks should never fire when called from the
   * service runtime, but the nrd xfer library might be called from
   * other code.
   */
  if (kern_nanimh.iov_length > NACL_ABI_IMC_IOVEC_MAX) {
    NaClLog(4, "gather/scatter array too large\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_leave;
  }
  if (kern_nanimh.desc_length > NACL_ABI_IMC_USER_DESC_MAX) {
    NaClLog(4, "handle vector too long\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_leave;
  }

  if (kern_nanimh.iov_length > 0) {
    if (!NaClCopyInFromUser(nap, kern_naiov, (uintptr_t) kern_nanimh.iov,
                            (kern_nanimh.iov_length * sizeof kern_naiov[0]))) {
      NaClLog(4, "gather/scatter array not in user address space\n");
      retval = -NACL_ABI_EFAULT;
      goto cleanup_leave;
    }

    for (i = 0; i < kern_nanimh.iov_length; ++i) {
      sysaddr = NaClUserToSysAddrRange(nap,
                                       (uintptr_t) kern_naiov[i].base,
                                       kern_naiov[i].length);
      if (kNaClBadAddress == sysaddr) {
        retval = -NACL_ABI_EFAULT;
        goto cleanup_leave;
      }
      kern_iov[i].base = (void *) sysaddr;
      kern_iov[i].length = kern_naiov[i].length;
    }
  }

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup_leave;
  }

  /*
   * make things easier for cleaup exit processing
   */
  memset(kern_desc, 0, sizeof kern_desc);
  retval = -NACL_ABI_EINVAL;

  kern_msg_hdr.iov = kern_iov;
  kern_msg_hdr.iov_length = kern_nanimh.iov_length;

  if (0 == kern_nanimh.desc_length) {
    kern_msg_hdr.ndescv = 0;
    kern_msg_hdr.ndesc_length = 0;
  } else {
    if (!NaClCopyInFromUser(nap, usr_desc, kern_nanimh.descv,
                            kern_nanimh.desc_length * sizeof usr_desc[0])) {
      retval = -NACL_ABI_EFAULT;
      goto cleanup;
    }

    for (i = 0; i < kern_nanimh.desc_length; ++i) {
      if (kKnownInvalidDescNumber == usr_desc[i]) {
        kern_desc[i] = (struct NaClDesc *) NaClDescInvalidMake();
      } else {
        /* NaCl modules are ILP32, so this works on ILP32 and LP64 systems */
        kern_desc[i] = NaClAppGetDesc(nap, usr_desc[i]);
      }
      if (NULL == kern_desc[i]) {
        retval = -NACL_ABI_EBADF;
        goto cleanup;
      }
    }
    kern_msg_hdr.ndescv = kern_desc;
    kern_msg_hdr.ndesc_length = kern_nanimh.desc_length;
  }
  kern_msg_hdr.flags = kern_nanimh.flags;

  /* lock user memory ranges in kern_naiov */
  for (i = 0; i < kern_nanimh.iov_length; ++i) {
    NaClVmIoWillStart(nap,
                      kern_naiov[i].base,
                      kern_naiov[i].base + kern_naiov[i].length - 1);
  }
  ssize_retval = NACL_VTBL(NaClDesc, ndp)->SendMsg(ndp, &kern_msg_hdr, flags);
  /* unlock user memory ranges in kern_naiov */
  for (i = 0; i < kern_nanimh.iov_length; ++i) {
    NaClVmIoHasEnded(nap,
                     kern_naiov[i].base,
                     kern_naiov[i].base + kern_naiov[i].length - 1);
  }

  if (NaClSSizeIsNegErrno(&ssize_retval)) {
    /*
     * NaClWouldBlock uses TSD (for both the errno-based and
     * GetLastError()-based implementations), so this is threadsafe.
     */
    if (0 != (flags & NACL_DONT_WAIT) && NaClWouldBlock()) {
      retval = -NACL_ABI_EAGAIN;
    } else if (-NACL_ABI_EMSGSIZE == ssize_retval) {
      /*
       * Allow the caller to handle the case when imc_sendmsg fails because
       * the message is too large for the system to send in one piece.
       */
      retval = -NACL_ABI_EMSGSIZE;
    } else {
      /*
       * TODO(bsy): the else case is some mysterious internal error.
       * Should we destroy the ndp or otherwise mark it as bad?  Was
       * the failure atomic?  Did it send some partial data?  Linux
       * implementation appears okay.
       */
      retval = -NACL_ABI_EIO;
    }
  } else if (ssize_retval > INT32_MAX || ssize_retval < INT32_MIN) {
    retval = -NACL_ABI_EOVERFLOW;
  } else {
    /* cast is safe due to range checks above */
    retval = (int32_t)ssize_retval;
  }

cleanup:
  for (i = 0; i < kern_nanimh.desc_length; ++i) {
    if (NULL != kern_desc[i]) {
      NaClDescUnref(kern_desc[i]);
      kern_desc[i] = NULL;
    }
  }
  NaClDescUnref(ndp);
cleanup_leave:
  NaClLog(3, "NaClSysImcSendmsg: returning %d\n", retval);
  return retval;
}

int32_t NaClSysImcRecvmsg(struct NaClAppThread *natp,
                          int                  d,
                          uint32_t             nanimhp,
                          int                  flags) {
  struct NaClApp                        *nap = natp->nap;
  int32_t                               retval = -NACL_ABI_EINVAL;
  ssize_t                               ssize_retval;
  uintptr_t                             sysaddr;
  size_t                                i;
  struct NaClDesc                       *ndp;
  struct NaClAbiNaClImcMsgHdr           kern_nanimh;
  struct NaClAbiNaClImcMsgIoVec         kern_naiov[NACL_ABI_IMC_IOVEC_MAX];
  struct NaClImcMsgIoVec                kern_iov[NACL_ABI_IMC_IOVEC_MAX];
  int32_t                               usr_desc[NACL_ABI_IMC_USER_DESC_MAX];
  struct NaClImcTypedMsgHdr             recv_hdr;
  struct NaClDesc                       *new_desc[NACL_ABI_IMC_DESC_MAX];
  nacl_abi_size_t                       num_user_desc;
  struct NaClDesc                       *invalid_desc = NULL;

  NaClLog(3,
          ("Entered NaClSysImcRecvMsg(0x%08"NACL_PRIxPTR", %d,"
           " 0x%08"NACL_PRIx32")\n"),
          (uintptr_t) natp, d, nanimhp);

  /*
   * First, we validate user-supplied message headers before
   * allocating a receive buffer.
   */
  if (!NaClCopyInFromUser(nap, &kern_nanimh, nanimhp, sizeof kern_nanimh)) {
    NaClLog(4, "NaClImcMsgHdr not in user address space\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup_leave;
  }
  /* copy before validating */

  if (kern_nanimh.iov_length > NACL_ABI_IMC_IOVEC_MAX) {
    NaClLog(4, "gather/scatter array too large: %"NACL_PRIdNACL_SIZE"\n",
            kern_nanimh.iov_length);
    retval = -NACL_ABI_EINVAL;
    goto cleanup_leave;
  }
  if (kern_nanimh.desc_length > NACL_ABI_IMC_USER_DESC_MAX) {
    NaClLog(4, "handle vector too long: %"NACL_PRIdNACL_SIZE"\n",
            kern_nanimh.desc_length);
    retval = -NACL_ABI_EINVAL;
    goto cleanup_leave;
  }

  if (kern_nanimh.iov_length > 0) {
    /*
     * Copy IOV array into kernel space.  Validate this snapshot and do
     * user->kernel address conversions on this snapshot.
     */
    if (!NaClCopyInFromUser(nap, kern_naiov, (uintptr_t) kern_nanimh.iov,
                            (kern_nanimh.iov_length * sizeof kern_naiov[0]))) {
      NaClLog(4, "gather/scatter array not in user address space\n");
      retval = -NACL_ABI_EFAULT;
      goto cleanup_leave;
    }
    /*
     * Convert every IOV base from user to system address, validate
     * range of bytes are really in user address space.
     */

    for (i = 0; i < kern_nanimh.iov_length; ++i) {
      sysaddr = NaClUserToSysAddrRange(nap,
                                       (uintptr_t) kern_naiov[i].base,
                                       kern_naiov[i].length);
      if (kNaClBadAddress == sysaddr) {
        NaClLog(4, "iov number %"NACL_PRIuS" not entirely in user space\n", i);
        retval = -NACL_ABI_EFAULT;
        goto cleanup_leave;
      }
      kern_iov[i].base = (void *) sysaddr;
      kern_iov[i].length = kern_naiov[i].length;
    }
  }

  if (kern_nanimh.desc_length > 0) {
    sysaddr = NaClUserToSysAddrRange(nap,
                                     (uintptr_t) kern_nanimh.descv,
                                     kern_nanimh.desc_length * sizeof(int32_t));
    if (kNaClBadAddress == sysaddr) {
      retval = -NACL_ABI_EFAULT;
      goto cleanup_leave;
    }
  }

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    NaClLog(4, "receiving descriptor invalid\n");
    retval = -NACL_ABI_EBADF;
    goto cleanup_leave;
  }

  recv_hdr.iov = kern_iov;
  recv_hdr.iov_length = kern_nanimh.iov_length;

  recv_hdr.ndescv = new_desc;
  recv_hdr.ndesc_length = NACL_ARRAY_SIZE(new_desc);
  memset(new_desc, 0, sizeof new_desc);

  recv_hdr.flags = 0;  /* just to make it obvious; IMC will clear it for us */

  /* lock user memory ranges in kern_naiov */
  for (i = 0; i < kern_nanimh.iov_length; ++i) {
    NaClVmIoWillStart(nap,
                      kern_naiov[i].base,
                      kern_naiov[i].base + kern_naiov[i].length - 1);
  }
  ssize_retval = NACL_VTBL(NaClDesc, ndp)->RecvMsg(ndp, &recv_hdr, flags);
  /* unlock user memory ranges in kern_naiov */
  for (i = 0; i < kern_nanimh.iov_length; ++i) {
    NaClVmIoHasEnded(nap,
                     kern_naiov[i].base,
                     kern_naiov[i].base + kern_naiov[i].length - 1);
  }
  /*
   * retval is number of user payload bytes received and excludes the
   * header bytes.
   */
  NaClLog(3, "NaClSysImcRecvMsg: RecvMsg() returned %"NACL_PRIdS"\n",
          ssize_retval);
  if (NaClSSizeIsNegErrno(&ssize_retval)) {
    /* negative error numbers all have valid 32-bit representations,
     * so this cast is safe. */
    retval = (int32_t) ssize_retval;
    goto cleanup;
  } else if (ssize_retval > INT32_MAX || ssize_retval < INT32_MIN) {
    retval = -NACL_ABI_EOVERFLOW;
    goto cleanup;
  } else {
    /* cast is safe due to range check above */
    retval = (int32_t) ssize_retval;
  }

  /*
   * NB: recv_hdr.flags may contain NACL_ABI_MESSAGE_TRUNCATED and/or
   * NACL_ABI_HANDLES_TRUNCATED.
   */

  kern_nanimh.flags = recv_hdr.flags;

  /*
   * Now internalize the NaClHandles as NaClDesc objects.
   */
  num_user_desc = recv_hdr.ndesc_length;

  if (kern_nanimh.desc_length < num_user_desc) {
    kern_nanimh.flags |= NACL_ABI_RECVMSG_DESC_TRUNCATED;
    for (i = kern_nanimh.desc_length; i < num_user_desc; ++i) {
      NaClDescUnref(new_desc[i]);
      new_desc[i] = NULL;
    }
    num_user_desc = kern_nanimh.desc_length;
  }

  invalid_desc = (struct NaClDesc *) NaClDescInvalidMake();
  /* prepare to write out to user space the descriptor numbers */
  for (i = 0; i < num_user_desc; ++i) {
    if (invalid_desc == new_desc[i]) {
      usr_desc[i] = kKnownInvalidDescNumber;
      NaClDescUnref(new_desc[i]);
    } else {
      usr_desc[i] = NaClAppSetDescAvail(nap, new_desc[i]);
    }
    new_desc[i] = NULL;
  }
  if (0 != num_user_desc &&
      !NaClCopyOutToUser(nap, (uintptr_t) kern_nanimh.descv, usr_desc,
                         num_user_desc * sizeof usr_desc[0])) {
    NaClLog(LOG_FATAL,
            ("NaClSysImcRecvMsg: in/out ptr (descv %"NACL_PRIxPTR
             ") became invalid at copyout?\n"),
            (uintptr_t) kern_nanimh.descv);
  }

  kern_nanimh.desc_length = num_user_desc;
  if (!NaClCopyOutToUser(nap, nanimhp, &kern_nanimh, sizeof kern_nanimh)) {
    NaClLog(LOG_FATAL,
            "NaClSysImcRecvMsg: in/out ptr (iov) became"
            " invalid at copyout?\n");
  }
  /* copy out updated desc count, flags */
 cleanup:
  if (retval < 0) {
    for (i = 0; i < NACL_ARRAY_SIZE(new_desc); ++i) {
      if (NULL != new_desc[i]) {
        NaClDescUnref(new_desc[i]);
        new_desc[i] = NULL;
      }
    }
  }
  NaClDescUnref(ndp);
  NaClDescSafeUnref(invalid_desc);
  NaClLog(3, "NaClSysImcRecvMsg: returning %d\n", retval);
cleanup_leave:
  return retval;
}

int32_t NaClSysImcMemObjCreate(struct NaClAppThread  *natp,
                               size_t                size) {
  struct NaClApp        *nap = natp->nap;
  int32_t               retval = -NACL_ABI_EINVAL;
  struct NaClDescImcShm *shmp;
  off_t                 size_as_off;

  NaClLog(3,
          ("Entered NaClSysImcMemObjCreate(0x%08"NACL_PRIxPTR
           " 0x%08"NACL_PRIxS")\n"),
          (uintptr_t) natp, size);

  /* This syscall is not used in Chromium so is disabled by default. */
  if (!NaClAclBypassChecks) {
    return -NACL_ABI_EACCES;
  }

  if (0 != (size & (NACL_MAP_PAGESIZE - 1))) {
    return -NACL_ABI_EINVAL;
  }
  /*
   * TODO(bsy): policy about maximum shm object size should be
   * enforced here.
   */
  size_as_off = (off_t) size;
  if (size_as_off < 0) {
    return -NACL_ABI_EINVAL;
  }

  shmp = NULL;

  shmp = malloc(sizeof *shmp);
  if (NULL == shmp) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  if (!NaClDescImcShmAllocCtor(shmp, size_as_off, /* executable= */ 0)) {
    retval = -NACL_ABI_ENOMEM;  /* is this reasonable? */
    goto cleanup;
  }

  retval = NaClAppSetDescAvail(nap, (struct NaClDesc *) shmp);
  shmp = NULL;

cleanup:
  free(shmp);

  return retval;
}

int32_t NaClSysImcSocketPair(struct NaClAppThread *natp,
                             uint32_t             descs_out) {
  struct NaClApp          *nap = natp->nap;
  int32_t                 usr_pair[2];
  struct NaClDesc         *pair[2];
  int32_t                 retval;

  NaClLog(3,
          ("Entered NaClSysImcSocketPair(0x%08"NACL_PRIxPTR
           " 0x%08"NACL_PRIx32")\n"),
          (uintptr_t) natp, descs_out);

  /* This syscall is not used in Chromium so is disabled by default. */
  if (!NaClAclBypassChecks) {
    return -NACL_ABI_EACCES;
  }

  retval = NaClCommonDescSocketPair(pair);
  if (0 != retval) {
    goto cleanup;
  }

  usr_pair[0] = NaClAppSetDescAvail(nap, pair[0]);
  usr_pair[1] = NaClAppSetDescAvail(nap, pair[1]);

  if (!NaClCopyOutToUser(nap, (uintptr_t) descs_out, usr_pair,
                         sizeof usr_pair)) {
    NaClAppSetDesc(nap, usr_pair[0], NULL);
    NaClAppSetDesc(nap, usr_pair[1], NULL);
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  retval = 0;

cleanup:
  return retval;
}
