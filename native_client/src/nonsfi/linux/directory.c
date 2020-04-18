/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file provides DIR related functions: fdopendir(), closedir() and
 * readdir_r().
 * Unfortunately dirent is slightly different from linux_abi_dirent64, which
 * is getdents64's result type. So, instead of reusing the implementation
 * in newlib, we implement those functions by ourselves.
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "native_client/src/nonsfi/linux/abi_conversion.h"
#include "native_client/src/nonsfi/linux/linux_sys_private.h"
#include "native_client/src/nonsfi/linux/linux_syscall_structs.h"

/*
 * The amount of space reserved for getdents64() usage.
 */
#define _DIRBLKSIZ 512

struct buffers {
  char getdents_buf[_DIRBLKSIZ];
  struct dirent converted_entry;
};

DIR *fdopendir(int fd) {
  /*
   * We check if fd is valid or not by fstat. We just discard the result.
   * On failure, we use its errno, expecting it is EBADF.
   */
  struct stat st;
  if (fstat(fd, &st) != 0) {
    return NULL;
  }

  if (!S_ISDIR(st.st_mode)) {
    errno = ENOTDIR;
    return NULL;
  }

  DIR *dirp = (DIR *) malloc(sizeof(DIR));
  if (dirp == NULL) {
    return NULL;
  }

  dirp->dd_buf = malloc(sizeof(struct buffers));
  if (dirp->dd_buf == NULL) {
    free(dirp);
    return NULL;
  }

  /*
   * We reuses newlib's DIR definition, so updating the original code may
   * cause a trouble.
   */
  dirp->dd_len = _DIRBLKSIZ;
  dirp->dd_fd = fd;
  dirp->dd_loc = 0;
  dirp->dd_seek = 0;
  dirp->dd_size = 0;
  return dirp;
}

int closedir(DIR *dirp) {
  int rc = close(dirp->dd_fd);
  free(dirp->dd_buf);
  free(dirp);
  return rc;
}

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result) {
  for (;;) {
    if (dirp->dd_loc >= dirp->dd_size) {
      dirp->dd_size = linux_getdents64(
          dirp->dd_fd,
          (struct linux_abi_dirent64 *) dirp->dd_buf, dirp->dd_len);
      if (dirp->dd_size <= 0) {
        *result = NULL;
        return dirp->dd_size == 0 ? 0 : errno;
      }
      dirp->dd_loc = 0;
    }

    struct linux_abi_dirent64 *linux_entry =
        (struct linux_abi_dirent64 *) (dirp->dd_buf + dirp->dd_loc);
    if (linux_entry->d_reclen <= 0 ||
        linux_entry->d_reclen > dirp->dd_len + 1 - dirp->dd_loc) {
      *result = NULL;
      /*
       * Read dirent record looks broken. No corresponding errno, here.
       * Return EIO to keep consistent with newlib's semantics.
       */
      return EIO;
    }

    /* Move to the next entry */
    dirp->dd_loc += linux_entry->d_reclen;

    /* Skip deleted entry. */
    if (linux_entry->d_ino == 0)
      continue;

    /* Got a valid entry. Convert its ABI and return it. */
    linux_dirent_to_nacl_dirent(linux_entry, entry);
    *result = entry;
    return 0;
  }
}

struct dirent *readdir(DIR *dirp) {
  struct dirent *de;
  int error =
      readdir_r(dirp, &((struct buffers *)dirp->dd_buf)->converted_entry, &de);
  if (error)
    errno = error;
  return de;
}
