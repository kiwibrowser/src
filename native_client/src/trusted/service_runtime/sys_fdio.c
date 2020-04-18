/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sys_fdio.h"

#include <string.h>

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


static size_t const kdefault_io_buffer_bytes_to_log = 64;

int32_t NaClSysDup(struct NaClAppThread *natp,
                   int                  oldfd) {
  struct NaClApp  *nap = natp->nap;
  int             retval;
  struct NaClDesc *old_nd;

  NaClLog(3, "NaClSysDup(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t) natp, oldfd);
  old_nd = NaClAppGetDesc(nap, oldfd);
  if (NULL == old_nd) {
    retval = -NACL_ABI_EBADF;
    goto done;
  }
  retval = NaClAppSetDescAvail(nap, old_nd);
done:
  return retval;
}

int32_t NaClSysDup2(struct NaClAppThread  *natp,
                    int                   oldfd,
                    int                   newfd) {
  struct NaClApp  *nap = natp->nap;
  int             retval;
  struct NaClDesc *old_nd;

  NaClLog(3, "NaClSysDup(0x%08"NACL_PRIxPTR", %d, %d)\n",
          (uintptr_t) natp, oldfd, newfd);
  if (newfd < 0) {
    retval = -NACL_ABI_EINVAL;
    goto done;
  }
  /*
   * TODO(bsy): is this a reasonable largest sane value?  The
   * descriptor array shouldn't get too large.
   */
  if (newfd >= NACL_MAX_FD) {
    retval = -NACL_ABI_EINVAL;
    goto done;
  }
  old_nd = NaClAppGetDesc(nap, oldfd);
  if (NULL == old_nd) {
    retval = -NACL_ABI_EBADF;
    goto done;
  }
  NaClAppSetDesc(nap, newfd, old_nd);
  retval = newfd;
done:
  return retval;
}

int32_t NaClSysClose(struct NaClAppThread *natp,
                     int                  d) {
  struct NaClApp  *nap = natp->nap;
  int             retval = -NACL_ABI_EBADF;
  struct NaClDesc *ndp;

  NaClLog(3, "Entered NaClSysClose(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t) natp, d);

  NaClFastMutexLock(&nap->desc_mu);
  ndp = NaClAppGetDescMu(nap, d);
  if (NULL != ndp) {
    NaClAppSetDescMu(nap, d, NULL);  /* Unref the desc_tbl */
  }
  NaClFastMutexUnlock(&nap->desc_mu);
  NaClLog(5, "Invoking Close virtual function of object 0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) ndp);
  if (NULL != ndp) {
    NaClDescUnref(ndp);
    retval = 0;
  }

  return retval;
}

int32_t NaClSysIsatty(struct NaClAppThread *natp,
                      int                  d) {
  struct NaClApp  *nap = natp->nap;
  int             retval = -NACL_ABI_EBADF;
  struct NaClDesc *ndp;

  NaClLog(3, "Entered NaClSysIsatty(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t) natp, d);

  if (!NaClAclBypassChecks) {
    return -NACL_ABI_EACCES;
  }

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    NaClLog(4, "bad desc\n");
    return -NACL_ABI_EBADF;
  }

  retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->Isatty)(ndp);
  NaClDescUnref(ndp);
  return retval;
}

int32_t NaClSysGetdents(struct NaClAppThread *natp,
                        int                  d,
                        uint32_t             dirp,
                        size_t               count) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  ssize_t         getdents_ret;
  uintptr_t       sysaddr;
  struct NaClDesc *ndp;

  NaClLog(3,
          ("Entered NaClSysGetdents(0x%08"NACL_PRIxPTR", "
           "%d, 0x%08"NACL_PRIx32", "
           "%"NACL_PRIuS"[0x%"NACL_PRIxS"])\n"),
          (uintptr_t) natp, d, dirp, count, count);

  if (!NaClFileAccessEnabled()) {
    /*
     * Filesystem access is disabled, so disable the getdents() syscall.
     * We do this for security hardening, though it should be redundant,
     * because untrusted code should not be able to open any directory
     * descriptors (i.e. descriptors with a non-trivial Getdents()
     * implementation).
     */
    return -NACL_ABI_EACCES;
  }

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  /*
   * Generic NaClCopyOutToUser is not sufficient, since buffer size
   * |count| is arbitrary and we wouldn't want to have to allocate
   * memory in trusted address space to match.
   */
  sysaddr = NaClUserToSysAddrRange(nap, dirp, count);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(4, " illegal address for directory data\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup_unref;
  }

  /*
   * Clamp count to INT32_MAX to avoid the possibility of Getdents returning
   * a value that is outside the range of an int32.
   */
  if (count > INT32_MAX) {
    count = INT32_MAX;
  }
  /*
   * Grab addr space lock; getdents should not normally block, though
   * if the directory is on a networked filesystem this could, and
   * cause mmap to be slower on Windows.
   */
  NaClXMutexLock(&nap->mu);
  getdents_ret = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
                  Getdents)(ndp,
                            (void *) sysaddr,
                            count);
  NaClXMutexUnlock(&nap->mu);
  /* drop addr space lock */
  if ((getdents_ret < INT32_MIN && !NaClSSizeIsNegErrno(&getdents_ret))
      || INT32_MAX < getdents_ret) {
    /* This should never happen, because we already clamped the input count */
    NaClLog(LOG_FATAL, "Overflow in Getdents: return value is %"NACL_PRIxS,
            (size_t) getdents_ret);
  } else {
    retval = (int32_t) getdents_ret;
  }
  if (retval > 0) {
    NaClLog(4, "getdents returned %d bytes\n", retval);
    NaClLog(8, "getdents result: %.*s\n", retval, (char *) sysaddr);
  } else {
    NaClLog(4, "getdents returned %d\n", retval);
  }

cleanup_unref:
  NaClDescUnref(ndp);

cleanup:
  return retval;
}

int32_t NaClSysRead(struct NaClAppThread  *natp,
                    int                   d,
                    uint32_t              buf,
                    uint32_t              count) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  ssize_t         read_result = -NACL_ABI_EINVAL;
  uintptr_t       sysaddr;
  struct NaClDesc *ndp;
  size_t          log_bytes;
  char const      *ellipsis = "";

  NaClLog(3,
          ("Entered NaClSysRead(0x%08"NACL_PRIxPTR", "
           "%d, 0x%08"NACL_PRIx32", "
           "%"NACL_PRIu32"[0x%"NACL_PRIx32"])\n"),
          (uintptr_t) natp, d, buf, count, count);

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  sysaddr = NaClUserToSysAddrRange(nap, buf, count);
  if (kNaClBadAddress == sysaddr) {
    NaClDescUnref(ndp);
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  /*
   * The maximum length for read and write is INT32_MAX--anything larger and
   * the return value would overflow. Passing larger values isn't an error--
   * we'll just clamp the request size if it's too large.
   */
  if (count > INT32_MAX) {
    count = INT32_MAX;
  }

  NaClVmIoWillStart(nap, buf, buf + count - 1);
  read_result = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
                 Read)(ndp, (void *) sysaddr, count);
  NaClVmIoHasEnded(nap, buf, buf + count - 1);
  if (read_result > 0) {
    NaClLog(4, "read returned %"NACL_PRIdS" bytes\n", read_result);
    log_bytes = (size_t) read_result;
    if (log_bytes > INT32_MAX) {
      log_bytes = INT32_MAX;
      ellipsis = "...";
    }
    if (NaClLogGetVerbosity() < 10) {
      if (log_bytes > kdefault_io_buffer_bytes_to_log) {
        log_bytes = kdefault_io_buffer_bytes_to_log;
        ellipsis = "...";
      }
    }
    NaClLog(8, "read result: %.*s%s\n",
            (int) log_bytes, (char *) sysaddr, ellipsis);
  } else {
    NaClLog(4, "read returned %"NACL_PRIdS"\n", read_result);
  }
  NaClDescUnref(ndp);

  /* This cast is safe because we clamped count above.*/
  retval = (int32_t) read_result;
cleanup:
  return retval;
}

int32_t NaClSysWrite(struct NaClAppThread *natp,
                     int                  d,
                     uint32_t             buf,
                     uint32_t             count) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  ssize_t         write_result = -NACL_ABI_EINVAL;
  uintptr_t       sysaddr;
  char const      *ellipsis = "";
  struct NaClDesc *ndp;
  size_t          log_bytes;

  NaClLog(3,
          "Entered NaClSysWrite(0x%08"NACL_PRIxPTR", "
          "%d, 0x%08"NACL_PRIx32", "
          "%"NACL_PRIu32"[0x%"NACL_PRIx32"])\n",
          (uintptr_t) natp, d, buf, count, count);

  ndp = NaClAppGetDesc(nap, d);
  NaClLog(4, " ndp = %"NACL_PRIxPTR"\n", (uintptr_t) ndp);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  sysaddr = NaClUserToSysAddrRange(nap, buf, count);
  if (kNaClBadAddress == sysaddr) {
    NaClDescUnref(ndp);
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  log_bytes = count;
  if (log_bytes > INT32_MAX) {
    log_bytes = INT32_MAX;
    ellipsis = "...";
  }
  if (NaClLogGetVerbosity() < 10) {
    if (log_bytes > kdefault_io_buffer_bytes_to_log) {
      log_bytes = kdefault_io_buffer_bytes_to_log;
      ellipsis = "...";
    }
  }
  NaClLog(8, "In NaClSysWrite(%d, %.*s%s, %"NACL_PRIu32")\n",
          d, (int) log_bytes, (char *) sysaddr, ellipsis, count);

  /*
   * The maximum length for read and write is INT32_MAX--anything larger and
   * the return value would overflow. Passing larger values isn't an error--
   * we'll just clamp the request size if it's too large.
   */
  if (count > INT32_MAX) {
    count = INT32_MAX;
  }

  NaClVmIoWillStart(nap, buf, buf + count - 1);
  write_result = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
                  Write)(ndp, (void *) sysaddr, count);
  NaClVmIoHasEnded(nap, buf, buf + count - 1);

  NaClDescUnref(ndp);

  /* This cast is safe because we clamped count above.*/
  retval = (int32_t) write_result;

cleanup:
  return retval;
}

/*
 * This implements 64-bit offsets, so we use |offp| as an in/out
 * address so we can have a 64 bit return value.
 */
int32_t NaClSysLseek(struct NaClAppThread *natp,
                     int                  d,
                     uint32_t             offp,
                     int                  whence) {
  struct NaClApp  *nap = natp->nap;
  nacl_abi_off_t  offset;
  nacl_off64_t    retval64;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *ndp;

  NaClLog(3,
          ("Entered NaClSysLseek(0x%08"NACL_PRIxPTR", %d,"
           " 0x%08"NACL_PRIx32", %d)\n"),
          (uintptr_t) natp, d, offp, whence);

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  if (!NaClCopyInFromUser(nap, &offset, offp, sizeof offset)) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup_unref;
  }
  NaClLog(4, "offset 0x%08"NACL_PRIx64"\n", (uint64_t) offset);

  retval64 = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
              Seek)(ndp, (nacl_off64_t) offset, whence);
  if (NaClOff64IsNegErrno(&retval64)) {
    retval = (int32_t) retval64;
  } else {
    if (NaClCopyOutToUser(nap, offp, &retval64, sizeof retval64)) {
      retval = 0;
    } else {
      NaClLog(LOG_FATAL,
              "NaClSysLseek: in/out ptr became invalid at copyout?\n");
    }
  }
cleanup_unref:
  NaClDescUnref(ndp);
cleanup:
  return retval;
}

int32_t NaClSysFstat(struct NaClAppThread *natp,
                     int                  d,
                     uint32_t             nasp) {
  struct NaClApp        *nap = natp->nap;
  int32_t               retval = -NACL_ABI_EINVAL;
  struct NaClDesc       *ndp;
  struct nacl_abi_stat  result;

  NaClLog(3,
          ("Entered NaClSysFstat(0x%08"NACL_PRIxPTR
           ", %d, 0x%08"NACL_PRIx32")\n"),
          (uintptr_t) natp, d, nasp);

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    NaClLog(4, "bad desc\n");
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
            Fstat)(ndp, &result);
  if (0 == retval) {
    if (!NaClFileAccessEnabled()) {
      result.nacl_abi_st_ino = NACL_FAKE_INODE_NUM;
    }
    if (!NaClCopyOutToUser(nap, nasp, &result, sizeof result)) {
      retval = -NACL_ABI_EFAULT;
    }
  }

  NaClDescUnref(ndp);
cleanup:
  return retval;
}

int32_t NaClSysFchdir(struct NaClAppThread *natp,
                      int                  d) {
  struct NaClApp  *nap = natp->nap;
  struct NaClDesc *ndp;
  int32_t         retval = -NACL_ABI_EINVAL;

  NaClLog(3,
          ("Entered NaClSysFchdir(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, d);

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
            Fchdir)(ndp);

  NaClDescUnref(ndp);
cleanup:
  return retval;
}

int32_t NaClSysFchmod(struct NaClAppThread *natp,
                      int                  d,
                      int                  mode) {
  struct NaClApp  *nap = natp->nap;
  struct NaClDesc *ndp;
  int32_t         retval = -NACL_ABI_EINVAL;

  NaClLog(3,
          ("Entered NaClSysFchmod(0x%08"NACL_PRIxPTR", %d, 0x%x)\n"),
          (uintptr_t) natp, d, mode);

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
            Fchmod)(ndp, mode);

  NaClDescUnref(ndp);
cleanup:
  return retval;
}

int32_t NaClSysFsync(struct NaClAppThread *natp,
                     int                  d) {
  struct NaClApp  *nap = natp->nap;
  struct NaClDesc *ndp;
  int32_t         retval = -NACL_ABI_EINVAL;

  NaClLog(3,
          ("Entered NaClSysFsync(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, d);

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
            Fsync)(ndp);

  NaClDescUnref(ndp);
cleanup:
  return retval;
}

int32_t NaClSysFdatasync(struct NaClAppThread *natp,
                         int                  d) {
  struct NaClApp  *nap = natp->nap;
  struct NaClDesc *ndp;
  int32_t         retval = -NACL_ABI_EINVAL;

  NaClLog(3,
          ("Entered NaClSysFdatasync(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, d);

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
            Fdatasync)(ndp);

  NaClDescUnref(ndp);
cleanup:
  return retval;
}

int32_t NaClSysFtruncate(struct NaClAppThread *natp,
                         int                  d,
                         uint32_t             lengthp) {
  struct NaClApp  *nap = natp->nap;
  struct NaClDesc *ndp;
  nacl_abi_off_t  length;
  int32_t         retval = -NACL_ABI_EINVAL;

  NaClLog(3,
          ("Entered NaClSysFtruncate(0x%08"NACL_PRIxPTR", %d,"
           " 0x%"NACL_PRIx32")\n"),
          (uintptr_t) natp, d, lengthp);

  ndp = NaClAppGetDesc(nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }
  if (!NaClCopyInFromUser(nap, &length, lengthp, sizeof length)) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup_unref;
  }
  NaClLog(4, "length 0x%08"NACL_PRIx64"\n", (uint64_t) length);


  retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
            Ftruncate)(ndp, length);

cleanup_unref:
  NaClDescUnref(ndp);
cleanup:
  return retval;
}
