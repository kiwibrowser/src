/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_NONSFI_LINUX_ABI_CONVERSION_H_
#define NATIVE_CLIENT_SRC_NONSFI_LINUX_ABI_CONVERSION_H_ 1

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#include "native_client/src/nonsfi/linux/linux_syscall_defines.h"
#include "native_client/src/nonsfi/linux/linux_syscall_structs.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

#if defined(__i386__) || defined(__arm__)

struct linux_abi_timeval {
  int32_t tv_sec;
  int32_t tv_usec;
};

struct linux_abi_timespec {
  int32_t tv_sec;
  int32_t tv_nsec;
};

struct linux_abi_stat64 {
  uint64_t st_dev;
  uint32_t pad1;
  uint32_t st_ino_32;
  uint32_t st_mode;
  uint32_t st_nlink;
  uint32_t st_uid;
  uint32_t st_gid;
  uint64_t st_rdev;
  uint32_t pad2;
  /*
   * ARM, PNaCl, and other ABIs require a 32bit padding for alignment
   * here, while i386 does not.
   */
#if defined(__arm__)
  uint32_t pad3;
#endif
  int64_t st_size;
  uint32_t st_blksize;
#if defined(__arm__)
  uint32_t pad4;
#endif
  uint64_t st_blocks;
  int32_t st_atime;
  uint32_t st_atime_nsec;
  int32_t st_mtime;
  uint32_t st_mtime_nsec;
  int32_t st_ctime;
  uint32_t st_ctime_nsec;
  uint64_t st_ino;

  /*
   * We control the layout of this struct explicitly because PNaCl
   * clang inserts extra paddings even for the i386 target without
   * this.
   */
} __attribute__((packed));

#else
# error Unsupported architecture
#endif

/* We do not use O_RDONLY for abi conversion because it is 0. */
#define LINUX_O_WRONLY 01
#define LINUX_O_RDWR 02
#define LINUX_O_CREAT 0100
#define LINUX_O_TRUNC 01000
#define LINUX_O_APPEND 02000
#define LINUX_O_EXCL 0200
#define LINUX_O_NONBLOCK 04000
#define LINUX_O_SYNC 04010000
#define LINUX_O_ASYNC 020000

#if defined(__i386__)
# define LINUX_O_DIRECTORY 0200000
#elif defined(__arm__)
# define LINUX_O_DIRECTORY 040000
#else
# error Unsupported architecture
#endif

#define LINUX_O_CLOEXEC 02000000

#define NACL_KNOWN_O_FLAGS (O_ACCMODE | O_CREAT | O_TRUNC | O_APPEND | \
                            O_EXCL | O_NONBLOCK | O_SYNC | O_ASYNC | \
                            O_DIRECTORY | O_CLOEXEC)

/* Converts open flags from NaCl's to Linux's ABI. */
static inline int nacl_oflags_to_linux_oflags(int nacl_oflags) {
  if (nacl_oflags & ~NACL_KNOWN_O_FLAGS) {
    /* Unknown bit is found. */
    return -1;
  }
#define NACL_TO_LINUX(name) ((nacl_oflags & name) ? LINUX_ ## name : 0)
  return (NACL_TO_LINUX(O_WRONLY) |
          NACL_TO_LINUX(O_RDWR) |
          NACL_TO_LINUX(O_CREAT) |
          NACL_TO_LINUX(O_TRUNC) |
          NACL_TO_LINUX(O_APPEND) |
          NACL_TO_LINUX(O_EXCL) |
          NACL_TO_LINUX(O_NONBLOCK) |
          NACL_TO_LINUX(O_SYNC) |
          NACL_TO_LINUX(O_ASYNC) |
          NACL_TO_LINUX(O_DIRECTORY) |
          NACL_TO_LINUX(O_CLOEXEC));
#undef NACL_TO_LINUX
}

/* Converts open flags from Linux's to NaCl's ABI. */
static inline int linux_oflags_to_nacl_oflags(int linux_oflags) {
  /*
   * Because O_SYNC has two 1-bits. So we cannot use the same validation
   * check. We ignore unknown flags in this case.
   */
#define LINUX_TO_NACL(name) \
    (((linux_oflags & LINUX_ ## name) == LINUX_ ## name) ? name : 0)
  return (LINUX_TO_NACL(O_WRONLY) |
          LINUX_TO_NACL(O_RDWR) |
          LINUX_TO_NACL(O_CREAT) |
          LINUX_TO_NACL(O_TRUNC) |
          LINUX_TO_NACL(O_APPEND) |
          LINUX_TO_NACL(O_EXCL) |
          LINUX_TO_NACL(O_NONBLOCK) |
          LINUX_TO_NACL(O_SYNC) |
          LINUX_TO_NACL(O_ASYNC) |
          LINUX_TO_NACL(O_DIRECTORY) |
          LINUX_TO_NACL(O_CLOEXEC));
#undef LINUX_TO_NACL
}

/* Converts the timespec struct from NaCl's to Linux's ABI. */
static inline void nacl_timespec_to_linux_timespec(
    const struct timespec *nacl_timespec,
    struct linux_abi_timespec *linux_timespec) {
  linux_timespec->tv_sec = nacl_timespec->tv_sec;
  linux_timespec->tv_nsec = nacl_timespec->tv_nsec;
}

/* Converts the timespec struct from Linux's to NaCl's ABI. */
static inline void linux_timespec_to_nacl_timespec(
    const struct linux_abi_timespec *linux_timespec,
    struct timespec *nacl_timespec) {
  nacl_timespec->tv_sec = linux_timespec->tv_sec;
  nacl_timespec->tv_nsec = linux_timespec->tv_nsec;
}

/* Converts the timeval struct from Linux's to NaCl's ABI. */
static inline void linux_timeval_to_nacl_timeval(
    const struct linux_abi_timeval *linux_timeval,
    struct timeval *nacl_timeval) {
  nacl_timeval->tv_sec = linux_timeval->tv_sec;
  nacl_timeval->tv_usec = linux_timeval->tv_usec;
}

/* Converts the stat64 struct from Linux's to NaCl's ABI. */
static inline void linux_stat_to_nacl_stat(
    const struct linux_abi_stat64 *linux_stat,
    struct stat *nacl_stat) {
  /*
   * Some fields in linux_stat, such as st_dev, group/other bits of mode
   * and (a,m,c)timensec, are ignored to sync with the NaCl's original
   * implementation. Please see also NaClAbiStatHostDescStatXlateCtor
   * in native_client/src/trusted/desc/posix/nacl_desc.c.
   */
  memset(nacl_stat, 0, sizeof(*nacl_stat));
  nacl_stat->st_dev = 0;
  nacl_stat->st_ino = NACL_FAKE_INODE_NUM;
  nacl_stat->st_mode = linux_stat->st_mode;
  nacl_stat->st_nlink = linux_stat->st_nlink;
  nacl_stat->st_uid = -1;  /* Not root */
  nacl_stat->st_gid = -1;  /* Not wheel */
  nacl_stat->st_rdev = 0;
  nacl_stat->st_size = linux_stat->st_size;
  nacl_stat->st_blksize = 0;
  nacl_stat->st_blocks = 0;
  nacl_stat->st_atime = linux_stat->st_atime;
  nacl_stat->st_mtime = linux_stat->st_mtime;
  nacl_stat->st_ctime = linux_stat->st_ctime;
}

/* Converts the linux_abi_dirent64 struct to NaCl's dirent. */
static inline void linux_dirent_to_nacl_dirent(
    const struct linux_abi_dirent64 *linux_dirent,
    struct dirent *nacl_dirent) {
  nacl_dirent->d_ino = linux_dirent->d_ino;
  nacl_dirent->d_off = linux_dirent->d_off;
  /*
   * linux_abi_dirent64 has one byte d_type between d_off and d_name,
   * while NaCl's dirent has no d_type field at all. We remove it here,
   * so that the d_reclen should be decreased.
   */
  nacl_dirent->d_reclen =
      linux_dirent->d_reclen
      - offsetof(struct linux_abi_dirent64, d_name)
      + offsetof(struct dirent, d_name);

  assert(sizeof(nacl_dirent->d_name) == sizeof(linux_dirent->d_name));
  size_t copy_length =
      linux_dirent->d_reclen - offsetof(struct linux_abi_dirent64, d_name);
  assert(copy_length <= sizeof(nacl_dirent->d_name));
  memcpy(nacl_dirent->d_name, linux_dirent->d_name, copy_length);
}

#endif
