/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime API.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_STAT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_STAT_H_

#if defined(NACL_IN_TOOLCHAIN_HEADERS)
#include <sys/types.h>
#include <stdint.h>
#else
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"
#endif

/*
 * nacl_abi_mode_t is uint32_t, so we have more bits to play with:
 *
 * 3 b/octal digit, 30 bits:   1234567890
 */
#define NACL_ABI_S_IFMT        0000370000  /* for now */
/* Removed NACL_ABI_S_IFSHM_SYSV, which was 0000300000. */
#define NACL_ABI_S_IFSEMA      0000270000
#define NACL_ABI_S_IFCOND      0000260000
#define NACL_ABI_S_IFMUTEX     0000250000
#define NACL_ABI_S_IFSHM       0000240000
#define NACL_ABI_S_IFBOUNDSOCK 0000230000  /* bound socket*/
#define NACL_ABI_S_IFSOCKADDR  0000220000  /* socket address */
#define NACL_ABI_S_IFDSOCK     0000210000  /* data-only, transferable socket*/

#define NACL_ABI_S_IFSOCK      0000140000  /* data-and-descriptor socket*/
#define NACL_ABI_S_IFLNK       0000120000  /* symbolic link */
#define NACL_ABI_S_IFREG       0000100000  /* regular file */
#define NACL_ABI_S_IFBLK       0000060000  /* block device */
#define NACL_ABI_S_IFDIR       0000040000  /* directory */
#define NACL_ABI_S_IFCHR       0000020000  /* character device */
#define NACL_ABI_S_IFIFO       0000010000  /* fifo */

#define NACL_ABI_S_UNSUP       0000370000  /* unsupported file type */
/*
 * NaCl does not support file system objects other than regular files
 * and directories, and objects of other types will appear in the
 * directory namespace but will be mapped to NACL_ABI_S_UNSUP when
 * these objects are stat(2)ed.  Opening these kinds of objects will
 * fail.
 *
 * The ABI includes these bits so (library) code that use these
 * preprocessor symbols will compile.  The semantics of having a new
 * "unsupported" file type should enable code to run in a reasonably
 * sane way, but YMMV.
 */

#define NACL_ABI_S_ISUID      0004000
#define NACL_ABI_S_ISGID      0002000
#define NACL_ABI_S_ISVTX      0001000

#define NACL_ABI_S_IREAD      0400
#define NACL_ABI_S_IWRITE     0200
#define NACL_ABI_S_IEXEC      0100

#define NACL_ABI_S_IRWXU  (NACL_ABI_S_IREAD|NACL_ABI_S_IWRITE|NACL_ABI_S_IEXEC)
#define NACL_ABI_S_IRUSR  (NACL_ABI_S_IREAD)
#define NACL_ABI_S_IWUSR  (NACL_ABI_S_IWRITE)
#define NACL_ABI_S_IXUSR  (NACL_ABI_S_IEXEC)

#define NACL_ABI_S_IRWXG  (NACL_ABI_S_IRWXU >> 3)
#define NACL_ABI_S_IRGRP  (NACL_ABI_S_IREAD >> 3)
#define NACL_ABI_S_IWGRP  (NACL_ABI_S_IWRITE >> 3)
#define NACL_ABI_S_IXGRP  (NACL_ABI_S_IEXEC >> 3)

#define NACL_ABI_S_IRWXO  (NACL_ABI_S_IRWXU >> 6)
#define NACL_ABI_S_IROTH  (NACL_ABI_S_IREAD >> 6)
#define NACL_ABI_S_IWOTH  (NACL_ABI_S_IWRITE >> 6)
#define NACL_ABI_S_IXOTH  (NACL_ABI_S_IEXEC >> 6)
/*
 * only user access bits are supported; the rest are cleared when set
 * (effectively, umask of 077) and cleared when read.
 */

#define NACL_ABI_S_ISSOCK(m)  (((m) & NACL_ABI_S_IFMT) == NACL_ABI_S_IFSOCK)
#define NACL_ABI_S_ISLNK(m)   (((m) & NACL_ABI_S_IFMT) == NACL_ABI_S_IFLNK)
#define NACL_ABI_S_ISREG(m)   (((m) & NACL_ABI_S_IFMT) == NACL_ABI_S_IFREG)
#define NACL_ABI_S_ISBLK(m)   (((m) & NACL_ABI_S_IFMT) == NACL_ABI_S_IFBLK)
#define NACL_ABI_S_ISDIR(m)   (((m) & NACL_ABI_S_IFMT) == NACL_ABI_S_IFDIR)
#define NACL_ABI_S_ISSOCKADDR(m) \
                              (((m) & NACL_ABI_S_IFMT) == NACL_ABI_S_IFSOCKADDR)
#define NACL_ABI_S_ISCHR(m)   (((m) & NACL_ABI_S_IFMT) == NACL_ABI_S_IFCHR)
#define NACL_ABI_S_ISFIFO(m)  (((m) & NACL_ABI_S_IFMT) == NACL_ABI_S_IFIFO)
#define NACL_ABI_S_ISSHM(m)   (((m) & NACL_ABI_S_IFMT) == NACL_ABI_S_IFSHM)


/*
 * Linux <bit/stat.h> uses preprocessor to define st_atime to
 * st_atim.tv_sec etc to widen the ABI to use a struct timespec rather
 * than just have a time_t access/modification/inode-change times.
 * this is unfortunate, since that symbol cannot be used as a struct
 * member elsewhere (!).
 *
 * just like with type name conflicts, we avoid it by using nacl_abi_
 * as a prefix for struct members too.  sigh.
 */

struct nacl_abi_stat {  /* must be renamed when ABI is exported */
  nacl_abi_dev_t     nacl_abi_st_dev;       /* not implemented */
  nacl_abi_ino_t     nacl_abi_st_ino;       /* not implemented */
  nacl_abi_mode_t    nacl_abi_st_mode;      /* partially implemented. */
  nacl_abi_nlink_t   nacl_abi_st_nlink;     /* link count */
  nacl_abi_uid_t     nacl_abi_st_uid;       /* not implemented */
  nacl_abi_gid_t     nacl_abi_st_gid;       /* not implemented */
  nacl_abi_dev_t     nacl_abi_st_rdev;      /* not implemented */
  nacl_abi_off_t     nacl_abi_st_size;      /* object size */
  nacl_abi_blksize_t nacl_abi_st_blksize;   /* not implemented */
  nacl_abi_blkcnt_t  nacl_abi_st_blocks;    /* not implemented */
  nacl_abi_time_t    nacl_abi_st_atime;     /* access time */
  int64_t            nacl_abi_st_atimensec; /* possibly just pad */
  nacl_abi_time_t    nacl_abi_st_mtime;     /* modification time */
  int64_t            nacl_abi_st_mtimensec; /* possibly just pad */
  nacl_abi_time_t    nacl_abi_st_ctime;     /* inode change time */
  int64_t            nacl_abi_st_ctimensec; /* possibly just pad */
};

#endif
