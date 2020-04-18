/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file defines various POSIX-like functions directly using NaCl
 * syscall trampolines.  For normal application use, these are defined
 * instead using the IRT function tables.  Here we're defining the versions
 * to be used inside the IRT itself, and in various local tests that do not
 * use the IRT.
 *
 * We define these all in one file so that we can be sure that we get
 * them all defined here and won't have any stragglers brought in from
 * the normal C libraries, where we'd get the IRT-based versions instead.
 * Since the first thing in the link (../stubs/crt1.x) forces a reference
 * to _exit, we can be sure that this file will be brought in first.
 */

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/untrusted/nacl/getcwd.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

struct dirent;
struct stat;
struct timeval;
struct timespec;

static int errno_call(int error) {
  if (error) {
    errno = -error;
    return -1;
  }
  return 0;
}

static int errno_value_call(int rv) {
  if (rv < 0) {
    errno = -rv;
    return -1;
  }
  return rv;
}

void _exit(int status) {
  NACL_SYSCALL(exit)(status);
  __builtin_trap();
}

int gettimeofday(struct timeval *tv, void *tz) {
  return errno_call(NACL_SYSCALL(gettimeofday)(tv));
}

clock_t clock(void) {
  return NACL_SYSCALL(clock)();
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
  return errno_call(NACL_GC_WRAP_SYSCALL(NACL_SYSCALL(nanosleep)(req, rem)));
}

int clock_getres(clockid_t clk_id, struct timespec *res) {
  return errno_call(NACL_SYSCALL(clock_getres)(clk_id, res));
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  return errno_call(NACL_SYSCALL(clock_gettime)(clk_id, tp));
}

int sched_yield(void) {
  return errno_call(NACL_GC_WRAP_SYSCALL(NACL_SYSCALL(sched_yield)()));
}

long int sysconf(int name) {
  int value;
  int error = NACL_SYSCALL(sysconf)(name, &value);
  if (error < 0) {
    errno = -error;
    return -1L;
  }
  return value;
}

void *mmap(void *start, size_t length, int prot, int flags,
           int fd, off_t offset) {
  uint32_t rv = (uintptr_t) NACL_SYSCALL(mmap)(start, length, prot, flags,
                                               fd, &offset);
  if ((uint32_t) rv > 0xffff0000u) {
    errno = -(int32_t) rv;
    return MAP_FAILED;
  }
  return (void *) (uintptr_t) rv;
}

int munmap(void *start, size_t length) {
  return errno_call(NACL_SYSCALL(munmap)(start, length));
}

int mprotect(void *start, size_t length, int prot) {
  return errno_call(NACL_SYSCALL(mprotect)(start, length, prot));
}

int open(char const *pathname, int oflag, ...) {
  mode_t cmode;
  va_list ap;

  if (oflag & O_CREAT) {
    va_start(ap, oflag);
    cmode = va_arg(ap, mode_t);
    va_end(ap);
  } else {
    cmode = 0;
  }

  return errno_value_call(NACL_GC_WRAP_SYSCALL(
      NACL_SYSCALL(open)(pathname, oflag, cmode)));
}

int close(int fd) {
  return errno_call(NACL_SYSCALL(close)(fd));
}

int read(int fd, void *buf, size_t count) {
  return errno_value_call(NACL_GC_WRAP_SYSCALL(
      NACL_SYSCALL(read)(fd, buf, count)));
}

int write(int fd, const void *buf, size_t count) {
  return errno_value_call(NACL_GC_WRAP_SYSCALL(
      NACL_SYSCALL(write)(fd, buf, count)));
}

int pread(int fd, void *buf, size_t count, off_t offset) {
  return errno_value_call(NACL_GC_WRAP_SYSCALL(
      NACL_SYSCALL(pread)(fd, buf, count, &offset)));
}

int pwrite(int fd, const void *buf, size_t count, off_t offset) {
  return errno_value_call(NACL_GC_WRAP_SYSCALL(
      NACL_SYSCALL(pwrite)(fd, buf, count, &offset)));
}

off_t lseek(int fd, off_t offset, int whence) {
  int error = NACL_SYSCALL(lseek)(fd, &offset, whence);
  if (error) {
    errno = -error;
    return -1;
  }
  return offset;
}

int fchdir(int fd) {
  return errno_value_call(NACL_SYSCALL(fchdir)(fd));
}

int fchmod(int fd, mode_t mode) {
  return errno_value_call(NACL_SYSCALL(fchmod)(fd, mode));
}

int fsync(int fd) {
  return errno_value_call(NACL_SYSCALL(fsync)(fd));
}

int fdatasync(int fd) {
  return errno_value_call(NACL_SYSCALL(fdatasync)(fd));
}

int ftruncate(int fd, off_t length) {
  return errno_value_call(NACL_SYSCALL(ftruncate)(fd, &length));
}

int dup(int fd) {
  return errno_value_call(NACL_SYSCALL(dup)(fd));
}

int dup2(int oldfd, int newfd) {
  return errno_value_call(NACL_SYSCALL(dup2)(oldfd, newfd));
}

int fstat(int fd, struct stat *st) {
  return errno_call(NACL_SYSCALL(fstat)(fd, st));
}

int stat(const char *file, struct stat *st) {
  return errno_call(NACL_SYSCALL(stat)(file, st));
}

int getdents(int fd, struct dirent *buf, size_t count) {
  return errno_value_call(NACL_GC_WRAP_SYSCALL(
      NACL_SYSCALL(getdents)(fd, buf, count)));
}

int isatty(int fd) {
  int result = NACL_SYSCALL(isatty)(fd);
  if (result < 0) {
    errno = -result;
    return 0;
  }
  return result;
}

int getpid(void) {
  return errno_value_call(NACL_SYSCALL(getpid)());
}

int mkdir(const char *path, int mode) {
  return errno_call(NACL_SYSCALL(mkdir)(path, mode));
}

int rmdir(const char *path) {
  return errno_call(NACL_SYSCALL(rmdir)(path));
}

int chdir(const char *path) {
  return errno_call(NACL_SYSCALL(chdir)(path));
}

char *__getcwd_without_malloc(char *buffer, size_t len) {
  int retval = NACL_SYSCALL(getcwd)(buffer, len);
  if (retval != 0) {
    errno = -retval;
    return NULL;
  }

  return buffer;
}

int unlink(const char *path) {
  return errno_call(NACL_SYSCALL(unlink)(path));
}

int truncate(const char *path, off_t length) {
  return errno_call(NACL_SYSCALL(truncate)(path, &length));
}

int lstat(const char *file, struct stat *st) {
  return errno_call(NACL_SYSCALL(lstat)(file, st));
}

int link(const char *oldpath, const char *newpath) {
  return errno_call(NACL_SYSCALL(link)(oldpath, newpath));
}

int rename(const char *oldpath, const char* newpath) {
  return errno_call(NACL_SYSCALL(rename)(oldpath, newpath));
}

int symlink(const char *oldpath, const char* newpath) {
  return errno_call(NACL_SYSCALL(symlink)(oldpath, newpath));
}

int chmod(const char *path, mode_t mode) {
  return errno_call(NACL_SYSCALL(chmod)(path, mode));
}

int access(const char *path, int amode) {
  return errno_call(NACL_SYSCALL(access)(path, amode));
}

int readlink(const char *path, char *buf, int bufsize) {
  return errno_value_call(NACL_SYSCALL(readlink)(path, buf, bufsize));
}

int utimes(const char *path, const struct timeval *times) {
  return errno_value_call(NACL_SYSCALL(utimes)(path, times));
}

/*
 * This is a stub since _start will call it but we don't want to
 * do the normal initialization.
 */
void __libnacl_irt_init(Elf32_auxv_t *auxv) {
}

/*
 * These have to be weak because the IRT defines its own versions.
 */
__attribute__((weak)) int nacl_tls_init(void *thread_ptr) {
  return -NACL_SYSCALL(tls_init)(thread_ptr);
}

__attribute__((weak)) void *nacl_tls_get(void) {
  return NACL_SYSCALL(tls_get)();
}
