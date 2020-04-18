/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_open(const char *pathname, int oflag, mode_t cmode,
                         int *newfd) {
  int rv = NACL_GC_WRAP_SYSCALL(NACL_SYSCALL(open)(pathname, oflag, cmode));
  if (rv < 0)
    return -rv;
  *newfd = rv;
  return 0;
}

static int nacl_irt_stat(const char *pathname, struct stat *st) {
  return -NACL_SYSCALL(stat)(pathname, st);
}

static int nacl_irt_mkdir(const char *pathname, mode_t mode) {
  return -NACL_SYSCALL(mkdir)(pathname, mode);
}

static int nacl_irt_rmdir(const char *pathname) {
  return -NACL_SYSCALL(rmdir)(pathname);
}

static int nacl_irt_chdir(const char *pathname) {
  return -NACL_SYSCALL(chdir)(pathname);
}

static int nacl_irt_getcwd(char *pathname, size_t len) {
  return -NACL_SYSCALL(getcwd)(pathname, len);
}

static int nacl_irt_unlink(const char *pathname) {
  return -NACL_SYSCALL(unlink)(pathname);
}

static int nacl_irt_truncate(const char *pathname, off_t length) {
  return -NACL_SYSCALL(truncate)(pathname, &length);
}

static int nacl_irt_lstat(const char *pathname, struct stat *st) {
  return -NACL_SYSCALL(lstat)(pathname, st);
}

static int nacl_irt_link(const char *oldpath, const char *newpath) {
  return -NACL_SYSCALL(link)(oldpath, newpath);
}

static int nacl_irt_rename(const char *oldpath, const char *newpath) {
  return -NACL_SYSCALL(rename)(oldpath, newpath);
}

static int nacl_irt_symlink(const char *oldpath, const char *newpath) {
  return -NACL_SYSCALL(symlink)(oldpath, newpath);
}

static int nacl_irt_chmod(const char *path, mode_t mode) {
  return -NACL_SYSCALL(chmod)(path, mode);
}

static int nacl_irt_access(const char *path, int amode) {
  return -NACL_SYSCALL(access)(path, amode);
}

static int nacl_irt_readlink(const char *path, char *buf, size_t bufsize,
                             size_t *nread) {
  int rv = NACL_SYSCALL(readlink)(path, buf, bufsize);
  if (rv < 0)
    return -rv;
  *nread = rv;
  return 0;
}

static int nacl_irt_utimes(const char *filename, const struct timeval *times) {
  return -NACL_SYSCALL(utimes)(filename, times);
}

const struct nacl_irt_filename nacl_irt_filename = {
  nacl_irt_open,
  nacl_irt_stat,
};

const struct nacl_irt_dev_filename_v0_2 nacl_irt_dev_filename_v0_2 = {
  nacl_irt_open,
  nacl_irt_stat,
  nacl_irt_mkdir,
  nacl_irt_rmdir,
  nacl_irt_chdir,
  nacl_irt_getcwd,
  nacl_irt_unlink,
};

const struct nacl_irt_dev_filename nacl_irt_dev_filename = {
  nacl_irt_open,
  nacl_irt_stat,
  nacl_irt_mkdir,
  nacl_irt_rmdir,
  nacl_irt_chdir,
  nacl_irt_getcwd,
  nacl_irt_unlink,
  nacl_irt_truncate,
  nacl_irt_lstat,
  nacl_irt_link,
  nacl_irt_rename,
  nacl_irt_symlink,
  nacl_irt_chmod,
  nacl_irt_access,
  nacl_irt_readlink,
  nacl_irt_utimes,
};
