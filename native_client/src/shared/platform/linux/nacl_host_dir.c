/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Directory descriptor / Handle abstraction.
 *
 * Note that we avoid using the thread-specific data / thread local
 * storage access to the "errno" variable, and instead use the raw
 * system call return interface of small negative numbers as errors.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_host_dir.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/service_runtime/nacl_config.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/dirent.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"

#ifdef _syscall3
_syscall3(int, getdents, uint, fd, struct dirent *, dirp, uint, count)

int getdents(unsigned int fd, struct dirent* dirp, unsigned int count);
#else
# include <sys/syscall.h>
int getdents(unsigned int fd, struct dirent* dirp, unsigned int count) {
  return syscall(__NR_getdents, fd, dirp, count);
}
#endif

struct linux_dirent {  /* offsets, ILP32 and LP64 */
  unsigned long  d_ino;             /*  0,  0 */
  unsigned long  d_off;             /*  4,  8 */
  unsigned short d_reclen;          /*  8, 16 */
  char           d_name[1];         /* 10, 18 */
  /* actual length is d_reclen - 2 - offsetof(struct linux_dirent, d_name) */
  /*
   * char pad;    / Zero padding byte
   * char d_type; / File type (only since Linux 2.6.4; offset is d_reclen - 1)
   */
};


/*
 * from native_client/src/trusted/service_runtime/include/sys/dirent.h:

struct nacl_abi_dirent {                                 offsets, NaCl
  nacl_abi_ino_t nacl_abi_d_ino;                                0
  nacl_abi_off_t nacl_abi_d_off;                                8
  uint16_t       nacl_abi_d_reclen;                            16
  char           nacl_abi_d_name[NACL_ABI_MAXNAMLEN + 1];      18
};

 * It would be nice if we didn't have to buffer dirent data.  Is it
 * possible, when the untrusted NaCl module invokes getdent, to
 * determine a (different) buffer size value to use with the host OS
 * (Linux)?
 *
 * Since the actual length is d_reclen, do we know if d_reclen is
 * padded out to a multiple of the alignment restriction for longs?
 * Best to assume that either may hold. On x86 where unaligned
 * accesses are okay, this might be fine; on other architectures where
 * it isn't, then presumably d_reclen will include the padding, and
 * d_type (which we don't use) is on the far end of the padding.
 *
 * Given a user-supplied buffer of N bytes to hold nacl_abi_dirent
 * entries, what is the size of the buffer that should be supplied to
 * the Linux kernel so that we will never end up with more information
 * than we can copy / return back to the user?
 *
 * For LP64, the answer is relatively simple: everything is the same
 * size, except that the kernel is going to use one more byte for the
 * d_type entry.  Since copying to the nacl_abi_dirent omits that, the
 * transfer shrinks the space needed.
 *
 * For ILP32, the answer is a little harder.  The linux_dirent use 8
 * fewer bytes for the entries before d_name, but one more at the end
 * for d_type.  So, when does the worst case expansion occur?  The
 * number of dirent entries is multiplied by the expansion, so we need
 * to determine the smallest dirent entry.  Assuming single character
 * file names, a user's buffer of size N can hold int(N/20) dirents.
 * The linux_dirent, with no alignment pads, but with d_type (we
 * assume newer kernels only), will take 13 bytes per entry, so we had
 * better not claim to have more than 13*int(N/20) bytes of space
 * available to the Linux kernel.  (We don't need to check in our
 * platform qualification check since "older" kernels are pre 2.6.4,
 * and all reasonable Linux distributions use newer kernels.)
 *
 * Suppose the user gave us a buffer of 40 bytes.  This is enough for
 * two dirent structures containing information about files each with
 * a single character name.  So, we invoke Linux's getdents with a 26
 * byte buffer.  What happens if the next actual directory entry's
 * name is 40-18-1=21 bytes long?  It would have fit in the user's
 * buffer, but the linux_dirent buffer required for that entry is
 * 10+21+2 bytes in length, or 33 bytes.  We supplied linux with a 26
 * byte buffer, so it should respond with EINVAL since the buffer
 * space is too small.
 *
 * This argues against a simple scheme that avoids buffering.
 *
 * We could, when we encounter EINVAL, increase the buffer size used
 * with the host OS.  How do we expand and could that might result in
 * too much being read in?
 */

int NaClHostDirCtor(struct NaClHostDir  *d,
                    int                 dir_desc) {
  if (!NaClMutexCtor(&d->mu)) {
    return -NACL_ABI_ENOMEM;
  }
  d->fd = dir_desc;
  d->cur_byte = 0;
  d->nbytes = 0;
  NaClLog(3, "NaClHostDirCtor: success.\n");
  return 0;
}

int NaClHostDirOpen(struct NaClHostDir  *d,
                    char                *path) {
  int         fd;
  struct stat stbuf;
  int         rv;

  NaClLog(3, "NaClHostDirOpen(0x%08"NACL_PRIxPTR", %s)\n", (uintptr_t) d, path);
  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDirOpen: 'this' is NULL\n");
  }

  NaClLog(3, "NaClHostDirOpen: invoking open(%s)\n", path);
  fd = open(path, O_RDONLY);
  NaClLog(3, "NaClHostDirOpen: got DIR* %d\n", fd);
  if (-1 == fd) {
    NaClLog(LOG_ERROR,
            "NaClHostDirOpen: open returned -1, errno %d\n", errno);
    return -NaClXlateErrno(errno);
  }
  /* check that it is really a directory */
  if (-1 == fstat(fd, &stbuf)) {
    NaClLog(LOG_ERROR,
            "NaClHostDirOpen: fstat failed?!?  errno %d\n", errno);
    (void) close(fd);
    return -NaClXlateErrno(errno);
  }
  if (!S_ISDIR(stbuf.st_mode)) {
    (void) close(fd);
    return -NACL_ABI_ENOTDIR;
  }
  rv = NaClHostDirCtor(d, fd);
  return rv;
}

/*
 * Copy and translate a single linux_dirent to nacl_abi_dirent.
 * Returns number of bytes consumed (includes alignment adjustment for
 * next entry).
 *
 * TODO(bsy): add filesystem info argument to specify which
 * directories are "root" inodes, to rewrite the inode number of '..'
 * as appropriate.
 */
static ssize_t NaClCopyDirent(struct NaClHostDir *d,
                              void               *buf,
                              size_t             len) {
  struct linux_dirent             *ldp = (struct linux_dirent *) (
      d->dirent_buf
      + d->cur_byte);
  struct nacl_abi_dirent volatile *nadp;
  size_t                          adjusted_size;

  /* make sure the buffer is aligned */
  CHECK(0 == ((sizeof(nacl_abi_ino_t) - 1) & (uintptr_t) buf));

  if (d->cur_byte == d->nbytes) {
    return 0;  /* none available */
  }
  CHECK(d->cur_byte < d->nbytes);
  CHECK(ldp->d_reclen <= d->nbytes - d->cur_byte);
  /* no partial record transferred. */

  nadp = (struct nacl_abi_dirent volatile *) buf;

  /*
   * is there enough space? assume Linux is sane, so no ssize_t
   * overflow in the adjusted_size computation.  (NAME_MAX is small.)
   */
  CHECK(NAME_MAX < 256);
  adjusted_size = offsetof(struct nacl_abi_dirent, nacl_abi_d_name)
      + strlen(ldp->d_name) + 1;  /* NUL termination */
  /* pad for alignment for access to d_ino */
  adjusted_size = (adjusted_size + (sizeof(nacl_abi_ino_t) - 1))
      & ~(sizeof(nacl_abi_ino_t) - 1);
  if (len < adjusted_size) {
    return -NACL_ABI_EINVAL;  /* result buffer is too small */
  }

  nadp->nacl_abi_d_ino = ldp->d_ino;
  nadp->nacl_abi_d_off = ldp->d_off;
  nadp->nacl_abi_d_reclen = adjusted_size;
  NaClLog(4, "NaClCopyDirent: %s\n", ldp->d_name);
  strcpy((char *) nadp->nacl_abi_d_name, ldp->d_name);
  /* NB: some padding bytes may not get overwritten */

  d->cur_byte += ldp->d_reclen;

  NaClLog(4, "NaClCopyDirent: returning %"NACL_PRIuS"\n", adjusted_size);
  return (ssize_t) adjusted_size;
}

static ssize_t NaClStreamDirents(struct NaClHostDir *d,
                                 void               *buf,
                                 size_t             len) {
  ssize_t retval;
  size_t  xferred = 0;
  ssize_t entry_size;

  NaClXMutexLock(&d->mu);
  while (len > 0) {
    NaClLog(4, "NaClStreamDirents: loop, xferred = %"NACL_PRIuS"\n", xferred);
    entry_size = NaClCopyDirent(d, buf, len);
    if (0 == entry_size) {
      CHECK(d->cur_byte == d->nbytes);
      retval = getdents(d->fd,
                        (struct dirent *) d->dirent_buf,
                        sizeof d->dirent_buf);
      if (-1 == retval) {
        if (xferred > 0) {
          /* next time through, we'll pick up the error again */
          goto cleanup;
        } else {
          xferred = -NaClXlateErrno(errno);
          goto cleanup;
        }
      } else if (0 == retval) {
        goto cleanup;
      }
      d->cur_byte = 0;
      d->nbytes = retval;
    } else if (entry_size < 0) {
      /*
       * The only error return from NaClCopyDirent is NACL_ABI_EINVAL
       * due to destinaton buffer too small for the current entry.  If
       * we had copied some entries before, we were successful;
       * otherwise report that the buffer is too small for the next
       * directory entry.
       */
      if (xferred > 0) {
        goto cleanup;
      } else {
        xferred = entry_size;
        goto cleanup;
      }
    }
    /* entry_size > 0, maybe copy another */
    buf = (void *) ((char *) buf + entry_size);
    CHECK(len >= (size_t) entry_size);
    len -= entry_size;
    xferred += entry_size;
  }
  /* perfect fit! */
 cleanup:
  NaClXMutexUnlock(&d->mu);
  return xferred;
}

ssize_t NaClHostDirGetdents(struct NaClHostDir  *d,
                            void                *buf,
                            size_t              len) {
  int retval;

  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDirGetdents: 'this' is NULL\n");
  }
  NaClLog(3, "NaClHostDirGetdents(0x%08"NACL_PRIxPTR", %"NACL_PRIuS"):\n",
          (uintptr_t) buf, len);

  if (0 != ((__alignof__(struct nacl_abi_dirent) - 1) & (uintptr_t) buf)) {
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }

  retval = NaClStreamDirents(d, buf, len);
 cleanup:
  NaClLog(3, "NaClHostDirGetdents: returned %d\n", retval);
  return retval;
}

int NaClHostDirRewind(struct NaClHostDir *d) {
  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDirRewind: 'this' is NULL\n");
  }
  return -NaClXlateErrno(lseek64(d->fd, 0, SEEK_SET));
}

int NaClHostDirClose(struct NaClHostDir *d) {
  int retval;

  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDirClose: 'this' is NULL\n");
  }
  NaClLog(3, "NaClHostDirClose(%d)\n", d->fd);
  retval = close(d->fd);
  d->fd = -1;
  NaClMutexDtor(&d->mu);
  return (-1 == retval) ? -NaClXlateErrno(errno) : retval;
}

int NaClHostDirFchdir(struct NaClHostDir *d) {
  if (-1 == fchdir(d->fd)) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDirFchmod(struct NaClHostDir *d, int mode) {
  if (-1 == fchmod(d->fd, mode)) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDirFsync(struct NaClHostDir *d) {
  if (-1 == fsync(d->fd)) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDirFdatasync(struct NaClHostDir *d) {
  if (-1 == fdatasync(d->fd)) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}
