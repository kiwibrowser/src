/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * TODO(bradchen): figure out where to move this include file and then
 * move it.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_NACL_SYSCALL_BINDINGS_TRAMPOLINE_H
#define NATIVE_CLIENT_SRC_UNTRUSTED_NACL_SYSCALL_BINDINGS_TRAMPOLINE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

struct NaClExceptionContext;
struct NaClAbiNaClImcMsgHdr;
struct NaClMemMappingInfo;
struct stat;
struct timespec;
struct timeval;

#define NACL_SYSCALL(s) ((TYPE_nacl_ ## s) NACL_SYSCALL_ADDR(NACL_sys_ ## s))

/*
 * These hook functions and the GC_WRAP macro are for wrapping a subset of
 * syscalls that are likely to take a long time, which will interfere with
 * thread parking for garbage collection. In particular, we don't want to
 * wrap all of the syscalls because some of them are used within the thread
 * parking instrumentation (tls related calls).
 */

extern void IRT_pre_irtcall_hook(void);
extern void IRT_post_irtcall_hook(void);

#if defined(__GLIBC__)
/*
 * GC instrumentation is not supported when using nacl-glibc with direct
 * NaCl syscalls.
 */
# define NACL_GC_WRAP_SYSCALL(_expr) (_expr)
#else
# define NACL_GC_WRAP_SYSCALL(_expr) \
  ({                                \
    __typeof__(_expr) __sysret;     \
    IRT_pre_irtcall_hook();        \
    __sysret = _expr;               \
    IRT_post_irtcall_hook();       \
    __sysret;                       \
  })
#endif

/* ============================================================ */
/* files */
/* ============================================================ */

typedef int (*TYPE_nacl_dup)(int oldfd);

typedef int (*TYPE_nacl_dup2)(int oldfd, int newfd);

typedef int (*TYPE_nacl_read) (int desc, void *buf, size_t count);

typedef int (*TYPE_nacl_close) (int desc);

typedef int (*TYPE_nacl_fstat) (int fd, struct stat *stbp);

typedef int (*TYPE_nacl_write) (int desc, void const *buf, size_t count);

typedef int (*TYPE_nacl_open) (char const *pathname, int flags, mode_t mode);

typedef int (*TYPE_nacl_lseek) (int desc,
                                off_t *offset, /* 64 bit value */
                                int whence);

typedef int (*TYPE_nacl_stat) (const char *file, struct stat *st);

typedef int (*TYPE_nacl_fchdir) (int fd);

typedef int (*TYPE_nacl_fchmod) (int fd, mode_t mode);

typedef int (*TYPE_nacl_fsync) (int fd);

typedef int (*TYPE_nacl_fdatasync) (int fd);

typedef int (*TYPE_nacl_ftruncate) (int fd,
                                    off_t *length); /* 64 bit value */

typedef int (*TYPE_nacl_pread) (int fd, void *buf, size_t count, off_t *offset);

typedef int (*TYPE_nacl_pwrite) (int fd,
                                 const void *buf, size_t count,
                                 off_t *offset);

typedef int (*TYPE_nacl_isatty) (int fd);

/* ============================================================ */
/* imc */
/* ============================================================ */

typedef int (*TYPE_nacl_imc_recvmsg) (int desc,
                                      struct NaClAbiNaClImcMsgHdr *nmhp,
                                      int flags);
typedef int (*TYPE_nacl_imc_sendmsg) (int desc,
                                      struct NaClAbiNaClImcMsgHdr const *nmhp,
                                      int flags);
typedef int (*TYPE_nacl_imc_accept) (int d);

typedef int (*TYPE_nacl_imc_connect) (int d);

typedef int (*TYPE_nacl_imc_makeboundsock) (int *dp);

typedef int (*TYPE_nacl_imc_socketpair) (int *d2);

typedef int (*TYPE_nacl_imc_mem_obj_create) (size_t nbytes);

/* ============================================================ */
/* mmap */
/* ============================================================ */

typedef void *(*TYPE_nacl_mmap) (void *start,
                                  size_t length,
                                  int prot,
                                  int flags,
                                  int desc,
                                  off_t *offset);

typedef int (*TYPE_nacl_munmap) (void *start, size_t length);

typedef int (*TYPE_nacl_mprotect) (void *start, size_t length, int prot);

typedef int (*TYPE_nacl_list_mappings) (struct NaClMemMappingInfo *region,
                                        size_t count);

/* ============================================================ */
/* threads */
/* ============================================================ */

typedef void (*TYPE_nacl_thread_exit) (int32_t *stack_flag);
typedef int (*TYPE_nacl_thread_create) (void *start_user_address,
                                        void *stack,
                                        void *thread_ptr,
                                        void *second_thread_ptr);
typedef int (*TYPE_nacl_thread_nice) (const int nice);

/* ============================================================ */
/* mutex */
/* ============================================================ */

typedef int (*TYPE_nacl_mutex_create) (void);
typedef int (*TYPE_nacl_mutex_lock) (int mutex);
typedef int (*TYPE_nacl_mutex_unlock) (int mutex);
typedef int (*TYPE_nacl_mutex_trylock) (int mutex);

/* ============================================================ */
/* condvar */
/* ============================================================ */

typedef int (*TYPE_nacl_cond_create) (void);
typedef int (*TYPE_nacl_cond_wait) (int cv, int mutex);
typedef int (*TYPE_nacl_cond_signal) (int cv);
typedef int (*TYPE_nacl_cond_broadcast) (int cv);
typedef int (*TYPE_nacl_cond_timed_wait_abs) (int condvar,
                                              int mutex,
                                              const struct timespec *abstime);

/* ============================================================ */
/* semaphore */
/* ============================================================ */

typedef int (*TYPE_nacl_sem_create) (int32_t value);
typedef int (*TYPE_nacl_sem_wait) (int sem);
typedef int (*TYPE_nacl_sem_post) (int sem);

/* ============================================================ */
/* misc */
/* ============================================================ */

typedef int (*TYPE_nacl_getdents) (int desc, void *dirp, size_t count);

typedef int (*TYPE_nacl_gettimeofday) (struct timeval *tv);

typedef int (*TYPE_nacl_sched_yield) (void);

typedef int (*TYPE_nacl_sysconf) (int name, int *res);

typedef void *(*TYPE_nacl_brk) (void *p);

typedef pid_t (*TYPE_nacl_getpid) (void);

typedef clock_t (*TYPE_nacl_clock) (void);

typedef int (*TYPE_nacl_nanosleep) (const struct timespec *req,
                                    struct timespec *rem);

typedef int (*TYPE_nacl_clock_getres) (clockid_t clk_id,
                                       struct timespec *res);

typedef int (*TYPE_nacl_clock_gettime) (clockid_t clk_id,
                                        struct timespec *tp);

typedef int (*TYPE_nacl_mkdir) (const char *path, int mode);

typedef int (*TYPE_nacl_rmdir) (const char *path);

typedef int (*TYPE_nacl_chdir) (const char *path);

typedef int (*TYPE_nacl_getcwd) (char *path, int len);

typedef int (*TYPE_nacl_unlink) (const char *path);

typedef int (*TYPE_nacl_truncate) (const char *file, off_t *length);

typedef int (*TYPE_nacl_lstat) (const char *file, struct stat *st);

typedef int (*TYPE_nacl_link) (const char *oldpath, const char *newpath);

typedef int (*TYPE_nacl_rename) (const char *oldpath, const char *newpath);

typedef int (*TYPE_nacl_symlink) (const char *oldpath, const char *newpath);

typedef int (*TYPE_nacl_chmod) (const char *path, mode_t mode);

typedef int (*TYPE_nacl_access) (const char *path, int amode);

typedef int (*TYPE_nacl_readlink) (const char *path, char *buf, size_t bufsize);

typedef int (*TYPE_nacl_utimes) (const char *path, const struct timeval *times);

#ifdef __GNUC__
typedef void (*TYPE_nacl_exit) (int status) __attribute__((noreturn));
#else
typedef void (*TYPE_nacl_exit) (int status);
#endif

typedef void (*TYPE_nacl_null) (void);

typedef int (*TYPE_nacl_tls_init) (void *thread_ptr);

typedef void *(*TYPE_nacl_tls_get) (void);

typedef int (*TYPE_nacl_second_tls_set) (void *new_value);

typedef void *(*TYPE_nacl_second_tls_get) (void);

typedef int (*TYPE_nacl_dyncode_create) (void *dest, const void *src,
                                       size_t size);

typedef int (*TYPE_nacl_dyncode_modify) (void *dest, const void *src,
                                       size_t size);

typedef int (*TYPE_nacl_dyncode_delete) (void *dest, size_t size);

typedef int (*TYPE_nacl_exception_handler) (
    void (*handler)(struct NaClExceptionContext *context),
    void (**old_handler)(struct NaClExceptionContext *context));

typedef int (*TYPE_nacl_exception_stack) (void *stack, size_t size);

typedef int (*TYPE_nacl_exception_clear_flag) (void);

typedef int (*TYPE_nacl_test_infoleak) (void);

typedef int (*TYPE_nacl_test_crash) (int crash_type);

typedef int (*TYPE_nacl_futex_wait_abs) (volatile int *addr, int value,
                                         const struct timespec *abstime);

typedef int (*TYPE_nacl_futex_wake) (volatile int *addr, int nwake);

typedef int (*TYPE_nacl_get_random_bytes) (void *buf, size_t buf_size);

#if defined(__cplusplus)
}
#endif

#endif  /*  NATIVE_CLIENT_SRC_UNTRUSTED_NACL_SYSCALL_BINDINGS_TRAMPOLINE_H */
