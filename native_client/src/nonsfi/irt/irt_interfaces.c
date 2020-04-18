/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__linux__)
# include <linux/futex.h>
# include <sys/syscall.h>
#endif

#include "native_client/src/include/elf32.h"
#include "native_client/src/include/elf_auxv.h"
#include "native_client/src/include/nacl/nacl_exception.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/public/irt_core.h"
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"
#include "native_client/src/trusted/service_runtime/include/sys/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"
#include "native_client/src/trusted/service_runtime/include/sys/unistd.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/nacl/nacl_random.h"

#if defined(__native_client__) && defined(__arm__)
#include "native_client/src/nonsfi/irt/irt_icache.h"
#endif

#if defined(__native_client__)
# include "native_client/src/nonsfi/linux/irt_signal_handling.h"
# include "native_client/src/nonsfi/linux/linux_pthread_private.h"
#endif

/*
 * This is an implementation of NaCl's IRT interfaces that runs
 * outside of the NaCl sandbox.
 *
 * This allows PNaCl to be used as a portability layer without the
 * SFI-based sandboxing.  PNaCl pexes can be translated to
 * non-SFI-sandboxed native code and linked against this IRT
 * implementation.
 */


#if defined(__ANDROID__) && !defined(FUTEX_PRIVATE_FLAG)
/* Android's Linux headers currently don't define this flag. */
# define FUTEX_PRIVATE_FLAG 128
#endif

#if defined(__GLIBC__)
/*
 * glibc's headers will define st_atimensec etc. fields, but only if
 * _POSIX_SOURCE is defined, which disables many other declarations,
 * such as nanosleep(), getpagesize(), MAP_ANON and clock_gettime().
 */
# define st_atimensec st_atim.tv_nsec
# define st_mtimensec st_mtim.tv_nsec
# define st_ctimensec st_ctim.tv_nsec
#elif defined(__APPLE__)
/*
 * Similarly, Mac OS X's headers will define st_atimensec etc. fields,
 * but only if _POSIX_SOURCE is defined, which disables declarations
 * such as _SC_NPROCESSORS_ONLN.
 */
# define st_atimensec st_atimespec.tv_nsec
# define st_mtimensec st_mtimespec.tv_nsec
# define st_ctimensec st_ctimespec.tv_nsec
#endif

/*
 * On Linux, we rename _start() to _user_start() to avoid a clash
 * with the "_start" routine in the host toolchain.  On Mac OS X,
 * lacking objcopy, doing the symbol renaming is trickier, but also
 * unnecessary, because the host toolchain doesn't have a "_start"
 * routine.
 */
#if defined(__APPLE__)
# define USER_START _start
#else
# define USER_START _user_start
#endif
void USER_START(uint32_t *info);

/* TODO(mseaborn): Make threads work on Mac OS X. */
#if defined(__APPLE__)
# define __thread /* nothing */
#endif
static __thread void *g_tls_value;

/*
 * TODO(smklein): Once these decls are added to unistd.h, remove these externs.
 */
extern int fchdir(int fd);
extern int fchmod(int fd, mode_t mode);
extern int fsync(int fd);
extern int fdatasync(int fd);
extern int ftruncate(int fd, off_t length);
extern int utimes(const char *filename, const struct timeval *times);

/*
 * The IRT functions in irt.h are declared as taking "struct timespec"
 * and "struct timeval" pointers, but these are really "struct
 * nacl_abi_timespec" and "struct nacl_abi_timeval" pointers in this
 * unsandboxed context.
 *
 * To avoid changing irt.h for now and also avoid casting function
 * pointers, we use the same type signatures as in irt.h and do the
 * casting here.
 */
static void convert_from_nacl_timespec(struct timespec *dest,
                                       const struct timespec *src_nacl) {
  const struct nacl_abi_timespec *src =
      (const struct nacl_abi_timespec *) src_nacl;
  dest->tv_sec = src->tv_sec;
  dest->tv_nsec = src->tv_nsec;
}

static void convert_to_nacl_timespec(struct timespec *dest_nacl,
                                     const struct timespec *src) {
  struct nacl_abi_timespec *dest = (struct nacl_abi_timespec *) dest_nacl;
  dest->tv_sec = src->tv_sec;
  dest->tv_nsec = src->tv_nsec;
}

static void convert_to_nacl_timeval(struct timeval *dest_nacl,
                                    const struct timeval *src) {
  struct nacl_abi_timeval *dest = (struct nacl_abi_timeval *) dest_nacl;
  dest->nacl_abi_tv_sec = src->tv_sec;
  dest->nacl_abi_tv_usec = src->tv_usec;
}

static void convert_to_nacl_stat(struct stat *dest_nacl,
                                 const struct stat *src) {
  struct nacl_abi_stat *dest = (struct nacl_abi_stat *) dest_nacl;
  dest->nacl_abi_st_dev = src->st_dev;
  dest->nacl_abi_st_ino = src->st_ino;
  dest->nacl_abi_st_mode = src->st_mode;
  dest->nacl_abi_st_nlink = src->st_nlink;
  dest->nacl_abi_st_uid = src->st_uid;
  dest->nacl_abi_st_gid = src->st_gid;
  dest->nacl_abi_st_rdev = src->st_rdev;
  dest->nacl_abi_st_size = src->st_size;
  dest->nacl_abi_st_blksize = src->st_blksize;
  dest->nacl_abi_st_blocks = src->st_blocks;
  dest->nacl_abi_st_atime = src->st_atime;
  dest->nacl_abi_st_atimensec = src->st_atimensec;
  dest->nacl_abi_st_mtime = src->st_mtime;
  dest->nacl_abi_st_mtimensec = src->st_mtimensec;
  dest->nacl_abi_st_ctime = src->st_ctime;
  dest->nacl_abi_st_ctimensec = src->st_ctimensec;
}

static void copy_flag(int *dest, int src, int new_flag, int old_flag) {
  if ((src & old_flag) != 0)
    *dest |= new_flag;
}

/* Returns whether the conversion was successful. */
static int convert_from_nacl_mmap_prot(int *prot, int prot_nacl) {
  if ((prot_nacl & ~NACL_ABI_PROT_MASK) != 0)
    return 0;
  *prot = 0;
  copy_flag(prot, prot_nacl, PROT_READ, NACL_ABI_PROT_READ);
  copy_flag(prot, prot_nacl, PROT_WRITE, NACL_ABI_PROT_WRITE);
  copy_flag(prot, prot_nacl, PROT_EXEC, NACL_ABI_PROT_EXEC);
  return 1;
}

/* Returns whether the conversion was successful. */
static int convert_from_nacl_mmap_flags(int *flags, int flags_nacl) {
  int allowed = NACL_ABI_MAP_SHARED |
                NACL_ABI_MAP_PRIVATE |
                NACL_ABI_MAP_FIXED |
                NACL_ABI_MAP_ANON;
  if ((flags_nacl & ~allowed) != 0)
    return 0;
  *flags = 0;
  copy_flag(flags, flags_nacl, MAP_SHARED, NACL_ABI_MAP_SHARED);
  copy_flag(flags, flags_nacl, MAP_PRIVATE, NACL_ABI_MAP_PRIVATE);
  copy_flag(flags, flags_nacl, MAP_FIXED, NACL_ABI_MAP_FIXED);
  copy_flag(flags, flags_nacl, MAP_ANON, NACL_ABI_MAP_ANON);
  return 1;
}

static int check_error(int result) {
  if (result != 0) {
    /*
     * Check that we really have an error and don't indicate success
     * mistakenly.
     */
    assert(errno != 0);
    return errno;
  }
  return 0;
}

static int irt_close(int fd) {
  return check_error(close(fd));
}

static int irt_dup(int fd, int *new_fd) {
  int result = dup(fd);
  if (result < 0)
    return errno;
  *new_fd = result;
  return 0;
}

static int irt_dup2(int fd, int new_fd) {
  int result = dup2(fd, new_fd);
  if (result < 0)
    return errno;
  assert(result == new_fd);
  return 0;
}

static int irt_read(int fd, void *buf, size_t count, size_t *nread) {
  int result = read(fd, buf, count);
  if (result < 0)
    return errno;
  *nread = result;
  return 0;
}

static int irt_write(int fd, const void *buf, size_t count, size_t *nwrote) {
  int result = write(fd, buf, count);
  if (result < 0)
    return errno;
  *nwrote = result;
  return 0;
}

static int irt_seek(int fd, nacl_irt_off_t offset, int whence,
                    nacl_irt_off_t *new_offset) {
  off_t result = lseek(fd, offset, whence);
  if (result < 0)
    return errno;
  *new_offset = result;
  return 0;
}

static int irt_fstat(int fd, struct stat *st_nacl) {
  struct stat st;
  if (fstat(fd, &st) != 0)
    return errno;
  convert_to_nacl_stat(st_nacl, &st);
  return 0;
}

static void irt_exit(int status) {
  _exit(status);
}

static int irt_clock_func(nacl_irt_clock_t *ticks) {
  nacl_irt_clock_t result = clock();
  if (result == (nacl_irt_clock_t) -1)
    return errno;
  *ticks = result;
  return 0;
}

static int irt_gettod(struct timeval *time_nacl) {
  struct timeval time;
  int result = check_error(gettimeofday(&time, NULL));
  convert_to_nacl_timeval(time_nacl, &time);
  return result;
}

static int irt_sched_yield(void) {
  return check_error(sched_yield());
}

static int irt_nanosleep(const struct timespec *requested_nacl,
                         struct timespec *remaining_nacl) {
  struct timespec requested;
  struct timespec remaining;
  convert_from_nacl_timespec(&requested, requested_nacl);
  int result = check_error(nanosleep(&requested, &remaining));
  if (remaining_nacl != NULL)
    convert_to_nacl_timespec(remaining_nacl, &remaining);
  return result;
}

static int irt_sysconf(int name, int *value) {
  switch (name) {
    case NACL_ABI__SC_PAGESIZE:
      /*
       * For now, return the host's page size (typically 4k) rather
       * than 64k (NaCl's usual page size), which pexes will usually
       * be tested with.  We could change this to 64k, but then the
       * mmap() we define here should round up requested sizes to
       * multiples of 64k.
       */
      *value = sysconf(_SC_PAGESIZE);
      return 0;
    case NACL_ABI__SC_NPROCESSORS_ONLN: {
      int result = sysconf(_SC_NPROCESSORS_ONLN);
      if (result == 0)
        return errno;
      *value = result;
      return 0;
    }
    default:
      return EINVAL;
  }
}

static int irt_mmap(void **addr, size_t len, int prot, int flags,
                    int fd, nacl_irt_off_t off) {
  int host_prot;
  int host_flags;
  if (!convert_from_nacl_mmap_prot(&host_prot, prot) ||
      !convert_from_nacl_mmap_flags(&host_flags, flags)) {
    return EINVAL;
  }
  void *result = mmap(*addr, len, host_prot, host_flags, fd, off);
  if (result == MAP_FAILED)
    return errno;
  *addr = result;
  return 0;
}

static int irt_munmap(void *addr, size_t len) {
  return check_error(munmap(addr, len));
}

static int irt_mprotect(void *addr, size_t len, int prot) {
  int host_prot;
  if (!convert_from_nacl_mmap_prot(&host_prot, prot)) {
    return EINVAL;
  }
  return check_error(mprotect(addr, len, host_prot));
}

static int tls_init(void *ptr) {
  g_tls_value = ptr;
  return 0;
}

static void *tls_get(void) {
  return g_tls_value;
}

/* For newlib based nonsfi_loader, we use the one defined in pnacl_irt.c. */
#if !defined(__native_client__)
void *__nacl_read_tp(void) {
  return g_tls_value;
}
#endif

#if defined(__arm_nonsfi_linux__)

__asm__(".pushsection .text, \"ax\", %progbits\n"
        ".global __aeabi_read_tp\n"
        ".type __aeabi_read_tp, %function\n"
        ".arm\n"
        "__aeabi_read_tp:\n"
        "push {r1-r3, lr}\n"
        "vpush {d0-d7}\n"
        "blx aeabi_read_tp_impl\n"
        "vpop {d0-d7}\n"
        "pop {r1-r3, pc}\n"
        ".popsection\n");

void *aeabi_read_tp_impl(void) {
  return g_tls_value;
}

#endif


struct thread_args {
  void (*start_func)(void);
  void *thread_ptr;
};

static void *start_thread(void *arg) {
  struct thread_args args = *(struct thread_args *) arg;
  free(arg);
  g_tls_value = args.thread_ptr;
  args.start_func();
  abort();
}

#if defined(__native_client__)
static int thread_create_nonsfi(void (*start_func)(void), void *stack,
                                void *thread_ptr, nacl_irt_tid_t *child_tid) {
  struct thread_args *args = malloc(sizeof(struct thread_args));
  if (args == NULL) {
    return ENOMEM;
  }
  args->start_func = start_func;
  args->thread_ptr = thread_ptr;
  /* In Linux, it is possible to use the provided stack directly. */
  int error = nacl_user_thread_create(start_thread, stack, args, child_tid);
  if (error != 0)
    free(args);
  return error;
}

static void thread_exit_nonsfi(int32_t *stack_flag) {
  nacl_user_thread_exit(stack_flag);
}
#endif

static int thread_create(void (*start_func)(void), void *stack,
                         void *thread_ptr) {
#if defined(__native_client__)
  /*
   * When available, use the nonsfi version that does allow the |stack| to be
   * set in the new thread.
   */
  return thread_create_nonsfi(start_func, stack, thread_ptr, NULL);
#else
  /*
   * For now, we ignore the stack that user code provides and just use
   * the stack that the host libpthread allocates.
   */
  pthread_attr_t attr;
  int error = pthread_attr_init(&attr);
  if (error != 0)
    return error;
  error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (error != 0)
    return error;
  struct thread_args *args = malloc(sizeof(struct thread_args));
  if (args == NULL) {
    error = ENOMEM;
    goto cleanup;
  }
  args->start_func = start_func;
  args->thread_ptr = thread_ptr;
  pthread_t tid;
  error = pthread_create(&tid, &attr, start_thread, args);
  if (error != 0)
    free(args);
 cleanup:
  pthread_attr_destroy(&attr);
  return error;
#endif
}

static void thread_exit(int32_t *stack_flag) {
#if defined(__native_client__)
  /*
   * Since we used the nonsfi version of thread_create, we must also call the
   * nonsfi version of thread_exit to correctly clean it up.
   */
  thread_exit_nonsfi(stack_flag);
#else
  *stack_flag = 0;  /* Indicate that the user code's stack can be freed. */
  pthread_exit(NULL);
#endif
}

static int thread_nice(const int nice) {
  return 0;
}

/*
 * Mac OS X does not provide futexes or clock_gettime()/getres() natively.
 * TODO(mseaborn):  Make threads and clock_gettime() work on Mac OS X.
 */
#if defined(__linux__)
static int futex_wait_abs(volatile int *addr, int value,
                          const struct timespec *abstime_nacl) {
  struct timespec reltime;
  struct timespec *reltime_ptr = NULL;
  if (abstime_nacl != NULL) {
    struct timespec time_now;
    if (clock_gettime(CLOCK_REALTIME, &time_now) != 0)
      return errno;

    /* Convert the absolute time to a relative time. */
    const struct nacl_abi_timespec *abstime =
        (const struct nacl_abi_timespec *) abstime_nacl;
    reltime.tv_sec = abstime->tv_sec - time_now.tv_sec;
    reltime.tv_nsec = abstime->tv_nsec - time_now.tv_nsec;
    if (reltime.tv_nsec < 0) {
      reltime.tv_sec -= 1;
      reltime.tv_nsec += 1000000000;
    }
    /*
     * Linux's FUTEX_WAIT returns EINVAL if given a negative relative
     * time.  But an absolute time that's in the past is a valid
     * argument, for which we need to return ETIMEDOUT instead.
     */
    if (reltime.tv_sec < 0)
      return ETIMEDOUT;
    reltime_ptr = &reltime;
  }
  return check_error(syscall(__NR_futex, addr, FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
                             value, reltime_ptr, 0, 0));
}

static int futex_wake(volatile int *addr, int nwake, int *count) {
  int result = syscall(__NR_futex, addr, FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
                       nwake, 0, 0, 0);
  if (result < 0)
    return errno;
  *count = result;
  return 0;
}
#endif

#if defined(__linux__) || defined(__native_client__)
static int irt_clock_getres(nacl_irt_clockid_t clk_id,
                            struct timespec *time_nacl) {
  struct timespec time;
  int result = check_error(clock_getres(clk_id, &time));
  /*
   * The timespec pointer is allowed to be NULL for clock_getres() though
   * not for clock_gettime().
   */
  if (time_nacl != NULL)
    convert_to_nacl_timespec(time_nacl, &time);
  return result;
}

static int irt_clock_gettime(nacl_irt_clockid_t clk_id,
                             struct timespec *time_nacl) {
  struct timespec time;
  int result = check_error(clock_gettime(clk_id, &time));
  convert_to_nacl_timespec(time_nacl, &time);
  return result;
}
#endif

static int irt_open(const char *pathname, int flags, mode_t mode, int *new_fd) {
  int fd = open(pathname, flags, mode);
  if (fd < 0)
    return errno;
  *new_fd = fd;
  return 0;
}

static int irt_stat(const char *pathname, struct stat *stat_info_nacl) {
  struct stat stat_info;
  if (stat(pathname, &stat_info) != 0)
    return errno;
  convert_to_nacl_stat(stat_info_nacl, &stat_info);
  return 0;
}

static int irt_lstat(const char *pathname, struct stat *stat_info_nacl) {
  struct stat stat_info;
  if (lstat(pathname, &stat_info) != 0)
    return errno;
  convert_to_nacl_stat(stat_info_nacl, &stat_info);
  return 0;
}

static int irt_mkdir(const char *pathname, mode_t mode) {
  return check_error(mkdir(pathname, mode));
}

static int irt_rmdir(const char *pathname) {
  return check_error(rmdir(pathname));
}

static int irt_chdir(const char *pathname) {
  return check_error(chdir(pathname));
}

static int irt_fchdir(int fd) {
  return check_error(fchdir(fd));
}

static int irt_fchmod(int fd, mode_t mode) {
  return check_error(fchmod(fd, mode));
}

static int irt_fsync(int fd) {
  return check_error(fsync(fd));
}

static int irt_fdatasync(int fd) {
  return check_error(fdatasync(fd));
}

static int irt_ftruncate(int fd, nacl_irt_off_t length) {
  return check_error(ftruncate(fd, length));
}

static int irt_utimes(const char *filename, const struct timeval *times) {
  return check_error(utimes(filename, times));
}

static int irt_getcwd(char *pathname, size_t len) {
  if (getcwd(pathname, len) == NULL)
    return errno;
  return 0;
}

static int irt_unlink(const char *pathname) {
  return check_error(unlink(pathname));
}

static int irt_truncate(const char *pathname, nacl_irt_off_t length) {
  return check_error(truncate(pathname, length));
}

static int irt_link(const char *oldpath, const char *newpath) {
  return check_error(link(oldpath, newpath));
}

static int irt_rename(const char *oldpath, const char *newpath) {
  return check_error(rename(oldpath, newpath));
}

static int irt_symlink(const char *oldpath, const char *newpath) {
  return check_error(symlink(oldpath, newpath));
}

static int irt_chmod(const char *pathname, mode_t mode) {
  return check_error(chmod(pathname, mode));
}

static int irt_access(const char *pathname, int mode) {
  return check_error(access(pathname, mode));
}

static int irt_readlink(const char *path, char *buf, size_t count,
                        size_t *nread) {
  ssize_t result = readlink(path, buf, count);
  if (result < 0)
    return errno;
  *nread = result;
  return 0;
}

static int irt_getpid(int *pid) {
  *pid = getpid();
  return 0;
}

static void irt_stub_func(const char *name) {
  fprintf(stderr, "Error: Unimplemented IRT function: %s\n", name);
  abort();
}

#define DEFINE_STUB(name) \
    static void irt_stub_##name() { irt_stub_func(#name); }
#define USE_STUB(s, name) (__typeof__(s.name)) irt_stub_##name

const struct nacl_irt_basic nacl_irt_basic = {
  irt_exit,
  irt_gettod,
  irt_clock_func,
  irt_nanosleep,
  irt_sched_yield,
  irt_sysconf,
};

DEFINE_STUB(getdents)
const struct nacl_irt_dev_fdio_v0_2 nacl_irt_dev_fdio_v0_2 = {
  irt_close,
  irt_dup,
  irt_dup2,
  irt_read,
  irt_write,
  irt_seek,
  irt_fstat,
  USE_STUB(nacl_irt_dev_fdio_v0_2, getdents),
  irt_fchdir,
  irt_fchmod,
  irt_fsync,
  irt_fdatasync,
  irt_ftruncate,
};

const struct nacl_irt_fdio nacl_irt_fdio = {
  irt_close,
  irt_dup,
  irt_dup2,
  irt_read,
  irt_write,
  irt_seek,
  irt_fstat,
  USE_STUB(nacl_irt_fdio, getdents),
};

const struct nacl_irt_memory nacl_irt_memory = {
  irt_mmap,
  irt_munmap,
  irt_mprotect,
};

const struct nacl_irt_tls nacl_irt_tls = {
  tls_init,
  tls_get,
};

const struct nacl_irt_thread nacl_irt_thread = {
  thread_create,
  thread_exit,
  thread_nice,
};

#if defined(__native_client__)
const struct nacl_irt_thread_v0_2 nacl_irt_thread_v0_2 = {
  thread_create_nonsfi,
  thread_exit_nonsfi,
  thread_nice,
};
#endif

#if defined(__linux__)
const struct nacl_irt_futex nacl_irt_futex = {
  futex_wait_abs,
  futex_wake,
};
#elif !defined(__native_client__)
DEFINE_STUB(futex_wait_abs)
DEFINE_STUB(futex_wake)
const struct nacl_irt_futex nacl_irt_futex = {
  USE_STUB(nacl_irt_futex, futex_wait_abs),
  USE_STUB(nacl_irt_futex, futex_wake),
};
#endif

const struct nacl_irt_random nacl_irt_random = {
  nacl_secure_random,
};

#if defined(__linux__) || defined(__native_client__)
const struct nacl_irt_clock nacl_irt_clock = {
  irt_clock_getres,
  irt_clock_gettime,
};
#endif

const struct nacl_irt_dev_filename nacl_irt_dev_filename = {
  irt_open,
  irt_stat,
  irt_mkdir,
  irt_rmdir,
  irt_chdir,
  irt_getcwd,
  irt_unlink,
  irt_truncate,
  irt_lstat,
  irt_link,
  irt_rename,
  irt_symlink,
  irt_chmod,
  irt_access,
  irt_readlink,
  irt_utimes,
};

const struct nacl_irt_dev_getpid nacl_irt_dev_getpid = {
  irt_getpid,
};

/*
 * The following condition is true when building for Non-SFI Mode,
 * when we're calling Linux syscalls directly.  (Counter-intuitively,
 * "__linux__" is not #defined in this case.)
 */
#if defined(__native_client__)
const struct nacl_irt_exception_handling nacl_irt_exception_handling = {
  nacl_exception_get_and_set_handler,
  nacl_exception_set_stack,
  nacl_exception_clear_flag,
};

const struct nacl_irt_async_signal_handling nacl_irt_async_signal_handling = {
  nacl_async_signal_set_handler,
  nacl_async_signal_send_async_signal,
};
#endif

#if defined(__native_client__) && defined(__arm__)
const struct nacl_irt_icache nacl_irt_icache = {
  irt_clear_cache,
};
#endif

static int g_allow_dev_interfaces = 0;

void nacl_irt_nonsfi_allow_dev_interfaces() {
  g_allow_dev_interfaces = 1;
}

static int irt_dev_filter(void) {
  return g_allow_dev_interfaces;
}

static const struct nacl_irt_interface irt_interfaces[] = {
  { NACL_IRT_BASIC_v0_1, &nacl_irt_basic, sizeof(nacl_irt_basic), NULL },
  { NACL_IRT_DEV_FDIO_v0_2, &nacl_irt_dev_fdio_v0_2,
    sizeof(nacl_irt_dev_fdio_v0_2), NULL },
  { NACL_IRT_FDIO_v0_1, &nacl_irt_fdio, sizeof(nacl_irt_fdio), NULL },
  { NACL_IRT_MEMORY_v0_3, &nacl_irt_memory, sizeof(nacl_irt_memory), NULL },
  { NACL_IRT_TLS_v0_1, &nacl_irt_tls, sizeof(nacl_irt_tls), NULL },
  { NACL_IRT_THREAD_v0_1, &nacl_irt_thread, sizeof(nacl_irt_thread), NULL },
#if defined(__native_client__)
  { NACL_IRT_THREAD_v0_2, &nacl_irt_thread_v0_2,
    sizeof(nacl_irt_thread_v0_2), NULL },
#endif
  { NACL_IRT_FUTEX_v0_1, &nacl_irt_futex, sizeof(nacl_irt_futex), NULL },
  { NACL_IRT_RANDOM_v0_1, &nacl_irt_random, sizeof(nacl_irt_random), NULL },
#if defined(__linux__) || defined(__native_client__)
  { NACL_IRT_CLOCK_v0_1, &nacl_irt_clock, sizeof(nacl_irt_clock), NULL },
#endif
  { NACL_IRT_DEV_FILENAME_v0_3, &nacl_irt_dev_filename,
    sizeof(nacl_irt_dev_filename), irt_dev_filter },
  { NACL_IRT_DEV_GETPID_v0_1, &nacl_irt_dev_getpid,
    sizeof(nacl_irt_dev_getpid), irt_dev_filter },
#if defined(__native_client__)
  { NACL_IRT_EXCEPTION_HANDLING_v0_1, &nacl_irt_exception_handling,
    sizeof(nacl_irt_exception_handling), NULL },
  { NACL_IRT_ASYNC_SIGNAL_HANDLING_v0_1, &nacl_irt_async_signal_handling,
    sizeof(nacl_irt_async_signal_handling), NULL },
#endif
#if defined(__native_client__) && defined(__arm__)
  { NACL_IRT_ICACHE_v0_1, &nacl_irt_icache, sizeof(nacl_irt_icache), NULL },
#endif
};

size_t nacl_irt_query_core(const char *interface_ident,
                           void *table, size_t tablesize) {
  return nacl_irt_query_list(interface_ident, table, tablesize,
                             irt_interfaces, sizeof(irt_interfaces));
}

int nacl_irt_nonsfi_entry(int argc, char **argv, char **environ,
                          nacl_entry_func_t entry_func,
                          nacl_irt_query_func_t query_func) {
  /* Find size of environ array. */
  size_t env_count = 0;
  while (environ[env_count] != NULL)
    env_count++;

  size_t count =
      1  /* cleanup_func pointer */
      + 2  /* envc and argc counts */
      + argc + 1  /* argv array, with terminator */
      + env_count + 1  /* environ array, with terminator */
      + 4;  /* auxv: 2 entries, one of them the terminator */
  uint32_t *data = malloc(count * sizeof(uint32_t));
  if (data == NULL) {
    fprintf(stderr, "Failed to allocate argv/env/auxv array\n");
    return 1;
  }
  size_t pos = 0;
  data[pos++] = 0;  /* cleanup_func pointer */
  data[pos++] = env_count;
  data[pos++] = argc;
  /* Copy arrays, with terminators. */
  size_t i;
  for (i = 0; i < (size_t) argc; i++)
    data[pos++] = (uintptr_t) argv[i];
  data[pos++] = 0;
  for (i = 0; i < env_count; i++)
    data[pos++] = (uintptr_t) environ[i];
  data[pos++] = 0;
  /* auxv[0] */
  data[pos++] = AT_SYSINFO;
  data[pos++] = (uintptr_t) query_func;
  /* auxv[1] */
  data[pos++] = 0;
  data[pos++] = 0;
  assert(pos == count);

  entry_func(data);
  return 1;
}

#if defined(DEFINE_MAIN)
int main(int argc, char **argv, char **environ) {
  nacl_irt_nonsfi_allow_dev_interfaces();
  nacl_entry_func_t entry_func = USER_START;

  return nacl_irt_nonsfi_entry(argc, argv, environ, entry_func,
                               nacl_irt_query_core);
}
#endif
