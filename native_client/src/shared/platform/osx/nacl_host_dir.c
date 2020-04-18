/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dirent.h>

#include "native_client/src/include/nacl_platform.h"

#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_host_dir.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/dirent.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"


#ifndef SSIZE_T_MAX
# define SSIZE_T_MAX ((ssize_t) ((~(size_t) 0) >> 1))
#endif


int NaClHostDirOpen(struct NaClHostDir  *d,
                    char                *path) {
  DIR  *dirp;

  NaClLog(3, "NaClHostDirOpen(0x%08"NACL_PRIxPTR", %s)\n", (uintptr_t) d, path);
  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDirOpen: 'this' is NULL\n");
  }

  if (!NaClMutexCtor(&d->mu)) {
    NaClLog(LOG_ERROR,
            "NaClHostDirOpen: could not initialize mutex\n");
    return -NACL_ABI_ENOMEM;
  }

  NaClLog(3, "NaClHostDirOpen: invoking POSIX opendir(%s)\n", path);
  dirp = opendir(path);
  NaClLog(3, "NaClHostDirOpen: got DIR* 0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) dirp);
  if (NULL == dirp) {
    NaClLog(LOG_ERROR,
            "NaClHostDirOpen: open returned NULL, errno %d\n", errno);
    return -NaClXlateErrno(errno);
  }
  d->dirp = dirp;
  d->dp = readdir(d->dirp);
  d->off = 0;
  NaClLog(3, "NaClHostDirOpen: success.\n");
  return 0;
}

ssize_t NaClHostDirGetdents(struct NaClHostDir  *d,
                            void                *buf,
                            size_t              len) {
  struct nacl_abi_dirent *p;
  int                    rec_length;
  ssize_t                i;

  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDirGetdents: 'this' is NULL\n");
  }
  NaClLog(3, "NaClHostDirGetdents(0x%08"NACL_PRIxPTR", %"NACL_PRIuS"):\n",
          (uintptr_t) buf, len);

  NaClXMutexLock(&d->mu);

  i = 0;
  while ((size_t) i < len) {
    if (NULL == d->dp) {
      goto done;
    }
    if (i > SSIZE_T_MAX - d->dp->d_reclen) {
      NaClLog(LOG_FATAL, "NaClHostDirGetdents: buffer impossibly large\n");
    }
    if ((size_t) i + d->dp->d_reclen > len) {
      if (0 == i) {
        i = (size_t) -NACL_ABI_EINVAL;
      }
      goto done;
    }
    p = (struct nacl_abi_dirent *) (((char *) buf) + i);
    p->nacl_abi_d_off = ++d->off;
    p->nacl_abi_d_ino = d->dp->d_ino;
    memcpy(p->nacl_abi_d_name,
           d->dp->d_name,
           d->dp->d_namlen + 1);
    /*
     * Newlib expects entries to start on 0mod4 boundaries.
     * Round reclen to the next multiple of four.
     */
    rec_length = (offsetof(struct nacl_abi_dirent, nacl_abi_d_name) +
                  (d->dp->d_namlen + 1 + 3)) & ~3;
    /*
     * We cast to a volatile pointer so that the compiler won't ever
     * pick up the rec_length value from that user-accessible memory
     * location, rather than actually using the value in a register or
     * in the local frame.
     */
    ((volatile struct nacl_abi_dirent *) p)->nacl_abi_d_reclen = rec_length;
    if ((size_t) i > SIZE_T_MAX - rec_length) {
      NaClLog(LOG_FATAL, "NaClHostDirGetdents: buffer offset overflow\n");
    }
    i += rec_length;
    d->dp = readdir(d->dirp);
  }
 done:
  NaClXMutexUnlock(&d->mu);
  return (ssize_t) i;
}

int NaClHostDirRewind(struct NaClHostDir *d) {
  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDirRewind: 'this' is NULL\n");
  }
  rewinddir(d->dirp);
  d->dp = readdir(d->dirp);
  return 0;
}

int NaClHostDirClose(struct NaClHostDir *d) {
  int retval;

  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDirClose: 'this' is NULL\n");
  }
  NaClLog(3, "NaClHostDirClose(0x%08"NACL_PRIxPTR")\n", (uintptr_t) d->dirp);
  retval = closedir(d->dirp);
  if (-1 != retval) {
    d->dirp = NULL;
  }
  NaClMutexDtor(&d->mu);
  return (-1 == retval) ? -NaClXlateErrno(errno) : retval;
}

/*
 * In the OSX 10.10 SDK, dirfd is a macro that uses a GNU statement expression.
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-statement-expression"

int NaClHostDirFchdir(struct NaClHostDir *d) {
  int fd = dirfd(d->dirp);
  if (-1 == fd) {
    return -NaClXlateErrno(errno);
  }
  if (-1 == fchdir(fd)) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDirFchmod(struct NaClHostDir *d, int mode) {
  int fd = dirfd(d->dirp);
  if (-1 == fd) {
    return -NaClXlateErrno(errno);
  }
  if (-1 == fchmod(fd, mode)) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDirFsync(struct NaClHostDir *d) {
  int fd = dirfd(d->dirp);
  if (-1 == fd) {
    return -NaClXlateErrno(errno);
  }
  if (-1 == fsync(fd)) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

int NaClHostDirFdatasync(struct NaClHostDir *d) {
  int fd = dirfd(d->dirp);
  if (-1 == fd) {
    return -NaClXlateErrno(errno);
  }
  /* fdatasync does not exist for osx. */
  if (-1 == fsync(fd)) {
    return -NaClXlateErrno(errno);
  }
  return 0;
}

#pragma clang diagnostic pop
