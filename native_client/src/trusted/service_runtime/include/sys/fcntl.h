/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime API.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_FCNTL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_FCNTL_H_

#if defined(NACL_IN_TOOLCHAIN_HEADERS)
#include <sys/types.h>
#endif

/* from bits/fcntl.h */
#define NACL_ABI_O_ACCMODE       0003
#define NACL_ABI_O_RDONLY          00
#define NACL_ABI_O_WRONLY          01
#define NACL_ABI_O_RDWR            02

#define NACL_ABI_O_CREAT         0100 /* not fcntl */
#define NACL_ABI_O_TRUNC        01000 /* not fcntl */
#define NACL_ABI_O_APPEND       02000
#define NACL_ABI_O_DIRECTORY  0200000 /* not fcntl */

/*
 * Features not implemented by NaCl, but required by the newlib build.
 */
#define NACL_ABI_O_EXCL          0200
#define NACL_ABI_O_NONBLOCK     04000
#define NACL_ABI_O_NDELAY      NACL_ABI_O_NONBLOCK
#define NACL_ABI_O_SYNC        010000
#define NACL_ABI_O_FSYNC       NACL_ABI_O_SYNC
#define NACL_ABI_O_ASYNC       020000

/*
 * Features not implemented by NaCl, but required by nacl_helper_nonsfi.
 */
#define NACL_ABI_O_CLOEXEC   02000000

/* XXX close on exec request; must match UF_EXCLOSE in user.h */
#define FD_CLOEXEC  1 /* posix */

/* fcntl(2) requests */
#define NACL_ABI_F_DUPFD   0 /* Duplicate fildes */
#define NACL_ABI_F_GETFD   1 /* Get fildes flags (close on exec) */
#define NACL_ABI_F_SETFD   2 /* Set fildes flags (close on exec) */
#define NACL_ABI_F_GETFL   3 /* Get file flags */
#define NACL_ABI_F_SETFL   4 /* Set file flags */
#ifndef _POSIX_SOURCE
#define NACL_ABI_F_GETOWN  5 /* Get owner - for ASYNC */
#define NACL_ABI_F_SETOWN  6 /* Set owner - for ASYNC */
#endif  /* !_POSIX_SOURCE */
#define NACL_ABI_F_GETLK   7 /* Get record-locking information */
#define NACL_ABI_F_SETLK   8 /* Set or Clear a record-lock (Non-Blocking) */
#define NACL_ABI_F_SETLKW  9 /* Set or Clear a record-lock (Blocking) */
#ifndef _POSIX_SOURCE
#define NACL_ABI_F_RGETLK  10  /* Test a remote lock to see if it is blocked */
#define NACL_ABI_F_RSETLK  11  /* Set or unlock a remote lock */
#define NACL_ABI_F_CNVT    12  /* Convert a fhandle to an open fd */
#define NACL_ABI_F_RSETLKW   13  /* Set or Clear remote record-lock(Blocking) */
#endif  /* !_POSIX_SOURCE */

/* fcntl(2) flags (l_type field of flock structure) */
#define NACL_ABI_F_RDLCK   1 /* read lock */
#define NACL_ABI_F_WRLCK   2 /* write lock */
#define NACL_ABI_F_UNLCK   3 /* remove lock(s) */
#ifndef _POSIX_SOURCE
#define NACL_ABI_F_UNLKSYS 4 /* remove remote locks for a given system */
#endif  /* !_POSIX_SOURCE */

/* For openat(2) used by nacl_helper_nonsfi. */
#define NACL_ABI_AT_FDCWD (-2)

#if defined(NACL_IN_TOOLCHAIN_HEADERS)
/* file segment locking set data type - information passed to system by user */
struct flock {
  short l_type;
  short l_whence;
  off_t l_start;
  off_t l_len;
  pid_t l_pid;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

extern int open(const char *file, int oflag, ...);
extern int creat(const char *file, mode_t mode);
extern int fcntl(int, int, ...);
extern int openat(int dirfd, const char *pathname, int oflag, ...);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */
#endif  /* defined(NACL_IN_TOOLCHAIN_HEADERS) */

#endif
