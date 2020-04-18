/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_DIRENT_H
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_DIRENT_H

#if defined(NACL_IN_TOOLCHAIN_HEADERS)
#include <sys/types.h>
#include <stdint.h>
#else
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"
#endif

#if defined(NACL_IN_TOOLCHAIN_HEADERS)
/* check the compiler toolchain */
# ifdef NACL_ABI_MAXNAMLEN
#  if NACL_ABI_MAXNAMLEN != 255
#   error "MAXNAMLEN inconsistent"
#  endif
#  ifdef NAME_MAX
#   if NAME_MAX != 255
#    error "NAME_MAX inconsistent"
#   endif
#  endif
# else
#  define NACL_ABI_MAXNAMLEN 255
# endif
#else /* defined(NACL_IN_TOOLCHAIN_HEADERS) */
# define NACL_ABI_MAXNAMLEN 255
#endif

/*
 * _dirdesc contains the state used by opendir/readdir/closedir.
 */
typedef struct nacl_abi__dirdesc {
  int   nacl_abi_dd_fd;
  long  nacl_abi_dd_loc;
  long  nacl_abi_dd_size;
  char  *nacl_abi_dd_buf;
  int   nacl_abi_dd_len;
  long  nacl_abi_dd_seek;
} nacl_abi_DIR;

/*
 * dirent represents a single directory entry.
 */
struct nacl_abi_dirent {
  nacl_abi_ino_t nacl_abi_d_ino;
  nacl_abi_off_t nacl_abi_d_off;
  uint16_t       nacl_abi_d_reclen;
  char           nacl_abi_d_name[NACL_ABI_MAXNAMLEN + 1];
};

/*
 * external function declarations
 */
extern nacl_abi_DIR           *nacl_abi_opendir(const char *dirpath);
extern struct nacl_abi_dirent *nacl_abi_readdir(nacl_abi_DIR *direntry);
extern int                    nacl_abi_closedir(nacl_abi_DIR *direntry);
extern void                   nacl_abi_rewinddir(nacl_abi_DIR *direntry);
extern int                    nacl_abi_readdir_r(nacl_abi_DIR *direntry,
                                                 struct nacl_abi_dirent *entry,
                                                 struct nacl_abi_dirent **res);
extern nacl_abi_DIR           *nacl_abi_fdopendir(int fd);

#endif
