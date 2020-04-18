/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file defines various POSIX-like functions directly using Linux
 * syscalls.  This is analogous to src/untrusted/nacl/sys_private.c, which
 * defines functions using NaCl syscalls directly.
 */

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "native_client/src/include/elf32.h"
#include "native_client/src/nonsfi/linux/abi_conversion.h"
#include "native_client/src/nonsfi/linux/linux_sys_private.h"
#include "native_client/src/nonsfi/linux/linux_syscall_defines.h"
#include "native_client/src/nonsfi/linux/linux_syscall_structs.h"
#include "native_client/src/nonsfi/linux/linux_syscall_wrappers.h"
#include "native_client/src/public/linux_syscalls/linux/net.h"
#include "native_client/src/public/linux_syscalls/poll.h"
#include "native_client/src/public/linux_syscalls/sched.h"
#include "native_client/src/public/linux_syscalls/sys/prctl.h"
#include "native_client/src/public/linux_syscalls/sys/resource.h"
#include "native_client/src/public/linux_syscalls/sys/socket.h"
#include "native_client/src/public/linux_syscalls/sys/syscall.h"
#include "native_client/src/untrusted/nacl/getcwd.h"
#include "native_client/src/untrusted/nacl/tls.h"


/* Used in openat() and fstatat(). */
#define LINUX_AT_FDCWD (-100)

/*
 * Note that Non-SFI NaCl uses a 4k page size, in contrast to SFI NaCl's
 * 64k page size.
 */
static const int kPageSize = 0x1000;

static uintptr_t errno_value_call(uintptr_t result) {
  if (linux_is_error_result(result)) {
    errno = -result;
    return -1;
  }
  return result;
}

int syscall(int number, ...) {
  va_list ap;
  va_start(ap, number);
  uint32_t arg1 = va_arg(ap, uint32_t);
  uint32_t arg2 = va_arg(ap, uint32_t);
  uint32_t arg3 = va_arg(ap, uint32_t);
  uint32_t arg4 = va_arg(ap, uint32_t);
  uint32_t arg5 = va_arg(ap, uint32_t);
  uint32_t arg6 = va_arg(ap, uint32_t);
  va_end(ap);
  return errno_value_call(
      linux_syscall6(number, arg1, arg2, arg3, arg4, arg5, arg6));
}

void _exit(int status) {
  linux_syscall1(__NR_exit_group, status);
  __builtin_trap();
}

int gettimeofday(struct timeval *tv, void *tz) {
  struct linux_abi_timeval linux_tv;
  int result = errno_value_call(
      linux_syscall2(__NR_gettimeofday, (uintptr_t) &linux_tv, 0));
  if (result == 0)
    linux_timeval_to_nacl_timeval(&linux_tv, tv);
  return result;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
  struct linux_abi_timespec linux_req;
  nacl_timespec_to_linux_timespec(req, &linux_req);
  struct linux_abi_timespec linux_rem;
  int result = errno_value_call(linux_syscall2(__NR_nanosleep,
                                               (uintptr_t) &linux_req,
                                               (uintptr_t) &linux_rem));

  /*
   * NaCl does not support async signals, so we don't fill out rem on
   * result == -EINTR.
   */
  if (result == 0 && rem != NULL)
    linux_timespec_to_nacl_timespec(&linux_rem, rem);
  return result;
}

int clock_gettime(clockid_t clk_id, struct timespec *ts) {
  struct linux_abi_timespec linux_ts;
  int result = errno_value_call(
      linux_syscall2(__NR_clock_gettime, clk_id, (uintptr_t) &linux_ts));
  if (result == 0)
    linux_timespec_to_nacl_timespec(&linux_ts, ts);
  return result;
}

int clock_getres(clockid_t clk_id, struct timespec *res) {
  struct linux_abi_timespec linux_res;
  int result = errno_value_call(
      linux_syscall2(__NR_clock_getres, clk_id, (uintptr_t) &linux_res));
  /* Unlike clock_gettime, clock_getres allows NULL timespecs. */
  if (result == 0 && res != NULL)
    linux_timespec_to_nacl_timespec(&linux_res, res);
  return result;
}

int sched_yield(void) {
  return errno_value_call(linux_syscall0(__NR_sched_yield));
}

long int sysconf(int name) {
  switch (name) {
    case _SC_PAGESIZE:
      return kPageSize;
  }
  errno = EINVAL;
  return -1;
}

static void *mmap_internal(void *start, size_t length, int prot, int flags,
                           int fd, off_t offset) {
#if defined(__i386__) || defined(__arm__)
  static const int kPageBits = 12;
  if (offset & ((1 << kPageBits) - 1)) {
    /* An unaligned offset is specified. */
    errno = EINVAL;
    return MAP_FAILED;
  }
  offset >>= kPageBits;

  return (void *) errno_value_call(
      linux_syscall6(__NR_mmap2, (uintptr_t) start, length,
                     prot, flags, fd, offset));
#else
# error Unsupported architecture
#endif
}

void *mmap(void *start, size_t length, int prot, int flags,
           int fd, off_t offset) {
  /*
   * On Chrome OS and on Chrome's seccomp sandbox, mmap() with PROT_EXEC is
   * prohibited. So, instead, mmap() the memory without PROT_EXEC first, and
   * then give it the PROT_EXEC by mprotect.
   */
  void *result =
      mmap_internal(start, length, (prot & ~PROT_EXEC), flags, fd, offset);
  if (result != MAP_FAILED && (prot & PROT_EXEC) != 0) {
    if (mprotect(result, length, prot) < 0) {
      /*
       * If mprotect failed, we cannot do much else other than abort(), because
       * we cannot undo the mmap() (specifically, when MAP_FIXED is set).
       */
      static const char msg[] =
          "mprotect() in mmap() to set PROT_EXEC failed.";
      write(2, msg, sizeof(msg) - 1);
      abort();
    }
  }
  return result;
}

int munmap(void *start, size_t length) {
  return errno_value_call(
      linux_syscall2(__NR_munmap, (uintptr_t) start, length));
}

int mprotect(void *start, size_t length, int prot) {
  return errno_value_call(
      linux_syscall3(__NR_mprotect, (uintptr_t) start, length, prot));
}

int read(int fd, void *buf, size_t count) {
  return errno_value_call(linux_syscall3(__NR_read, fd,
                                         (uintptr_t) buf, count));
}

int write(int fd, const void *buf, size_t count) {
  return errno_value_call(linux_syscall3(__NR_write, fd,
                                         (uintptr_t) buf, count));
}

int open(const char *pathname, int oflag, ...) {
  mode_t cmode;

  oflag = nacl_oflags_to_linux_oflags(oflag);
  if (oflag == -1) {
    errno = EINVAL;
    return -1;
  }

  if (oflag & LINUX_O_CREAT) {
    va_list ap;
    va_start(ap, oflag);
    cmode = va_arg(ap, mode_t);
    va_end(ap);
  } else {
    cmode = 0;
  }

  return errno_value_call(
      linux_syscall3(__NR_open, (uintptr_t) pathname, oflag, cmode));
}

int openat(int dirfd, const char *pathname, int oflag, ...) {
  mode_t cmode;

  if (dirfd == AT_FDCWD)
    dirfd = LINUX_AT_FDCWD;

  oflag = nacl_oflags_to_linux_oflags(oflag);
  if (oflag == -1) {
    errno = EINVAL;
    return -1;
  }

  if (oflag & LINUX_O_CREAT) {
    va_list ap;
    va_start(ap, oflag);
    cmode = va_arg(ap, mode_t);
    va_end(ap);
  } else {
    cmode = 0;
  }

  return errno_value_call(
      linux_syscall4(__NR_openat, dirfd, (uintptr_t) pathname, oflag, cmode));
}

int close(int fd) {
  return errno_value_call(linux_syscall1(__NR_close, fd));
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
  uint32_t offset_low = (uint32_t) offset;
  uint32_t offset_high = offset >> 32;
#if defined(__i386__)
  return errno_value_call(
      linux_syscall5(__NR_pread64, fd, (uintptr_t) buf, count,
                     offset_low, offset_high));
#elif defined(__arm__)
  /*
   * On ARM, a 64-bit parameter has to be in an even-odd register
   * pair. Hence these calls ignore their fourth argument (r3) so that
   * their fifth and sixth make such a pair (r4,r5).
   */
  return errno_value_call(
      linux_syscall6(__NR_pread64, fd, (uintptr_t) buf, count,
                     0  /* dummy */, offset_low, offset_high));
#else
# error Unsupported architecture
#endif
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
  uint32_t offset_low = (uint32_t) offset;
  uint32_t offset_high = offset >> 32;
#if defined(__i386__)
  return errno_value_call(
      linux_syscall5(__NR_pwrite64, fd, (uintptr_t) buf, count,
                     offset_low, offset_high));
#elif defined(__arm__)
  return errno_value_call(
      linux_syscall6(__NR_pwrite64, fd, (uintptr_t) buf, count,
                     0  /* dummy */, offset_low, offset_high));
#else
# error Unsupported architecture
#endif
}

off_t lseek(int fd, off_t offset, int whence) {
#if defined(__i386__) || defined(__arm__)
  uint32_t offset_low = (uint32_t) offset;
  uint32_t offset_high = offset >> 32;
  off_t result;
  int rc = errno_value_call(
      linux_syscall5(__NR__llseek, fd, offset_high, offset_low,
                     (uintptr_t) &result, whence));
  if (rc == -1)
    return -1;
  return result;
#else
# error Unsupported architecture
#endif
}

int dup(int fd) {
  return errno_value_call(linux_syscall1(__NR_dup, fd));
}

int dup2(int oldfd, int newfd) {
  return errno_value_call(linux_syscall2(__NR_dup2, oldfd, newfd));
}

int fstat(int fd, struct stat *st) {
  struct linux_abi_stat64 linux_st;
  int rc = errno_value_call(
      linux_syscall2(__NR_fstat64, fd, (uintptr_t) &linux_st));
  if (rc == -1)
    return -1;
  linux_stat_to_nacl_stat(&linux_st, st);
  return 0;
}

int stat(const char *file, struct stat *st) {
  struct linux_abi_stat64 linux_st;
  int rc = errno_value_call(
      linux_syscall2(__NR_stat64, (uintptr_t) file, (uintptr_t) &linux_st));
  if (rc == -1)
    return -1;
  linux_stat_to_nacl_stat(&linux_st, st);
  return 0;
}

int lstat(const char *file, struct stat *st) {
  struct linux_abi_stat64 linux_st;
  int rc = errno_value_call(
      linux_syscall2(__NR_lstat64, (uintptr_t) file, (uintptr_t) &linux_st));
  if (rc == -1)
    return -1;
  linux_stat_to_nacl_stat(&linux_st, st);
  return 0;
}

int fstatat(int dirfd, const char *file, struct stat *st, int flags) {
  struct linux_abi_stat64 linux_st;
  if (dirfd == AT_FDCWD)
    dirfd = LINUX_AT_FDCWD;
  int rc = errno_value_call(linux_syscall4(
      __NR_fstatat64, dirfd, (uintptr_t) file, (uintptr_t) &linux_st, flags));
  if (rc == -1)
    return -1;
  linux_stat_to_nacl_stat(&linux_st, st);
  return 0;
}

int mkdir(const char *path, mode_t mode) {
  return errno_value_call(linux_syscall2(__NR_mkdir, (uintptr_t) path, mode));
}

int rmdir(const char *path) {
  return errno_value_call(linux_syscall1(__NR_rmdir, (uintptr_t) path));
}

int chdir(const char *path) {
  return errno_value_call(linux_syscall1(__NR_chdir, (uintptr_t) path));
}

int fchdir(int fd) {
  return errno_value_call(linux_syscall1(__NR_fchdir, fd));
}

int fchmod(int fd, mode_t mode) {
  return errno_value_call(linux_syscall2(__NR_fchmod, fd, mode));
}

int fsync(int fd) {
  return errno_value_call(linux_syscall1(__NR_fsync, fd));
}

int fdatasync(int fd) {
  return errno_value_call(linux_syscall1(__NR_fdatasync, fd));
}

int ftruncate(int fd, off_t length) {
  return errno_value_call(linux_syscall2(__NR_ftruncate, fd, length));
}

int utimes(const char *filename, const struct timeval *times) {
  return errno_value_call(linux_syscall2(__NR_utimes, (uintptr_t) filename,
        (uintptr_t) times));
}

char *__getcwd_without_malloc(char *buffer, size_t len) {
  int rc = errno_value_call(
      linux_syscall2(__NR_getcwd, (uintptr_t) buffer, len));
  if (rc == -1)
    return NULL;
  return buffer;
}

int unlink(const char *path) {
  return errno_value_call(linux_syscall1(__NR_unlink, (uintptr_t) path));
}

int truncate(const char *path, off_t length) {
  uint32_t length_low = (uint32_t) length;
  uint32_t length_high = length >> 32;
#if defined(__i386__)
  return errno_value_call(
      linux_syscall3(__NR_truncate64, (uintptr_t) path,
                     length_low, length_high));
#elif defined(__arm__)
  /*
   * On ARM, a 64-bit parameter has to be in an even-odd register
   * pair. Hence these calls ignore their second argument (r1) so that
   * their third and fourth make such a pair (r2,r3).
   */
  return errno_value_call(
      linux_syscall4(__NR_truncate64, (uintptr_t) path,
                     0  /* dummy */, length_low, length_high));
#else
# error Unsupported architecture
#endif
}

int link(const char *oldpath, const char *newpath) {
  return errno_value_call(
      linux_syscall2(__NR_link, (uintptr_t) oldpath, (uintptr_t) newpath));
}

int rename(const char *oldpath, const char* newpath) {
  return errno_value_call(
      linux_syscall2(__NR_rename, (uintptr_t) oldpath, (uintptr_t) newpath));
}

int symlink(const char *oldpath, const char* newpath) {
  return errno_value_call(
      linux_syscall2(__NR_symlink, (uintptr_t) oldpath, (uintptr_t) newpath));
}

int chmod(const char *path, mode_t mode) {
  return errno_value_call(
      linux_syscall2(__NR_chmod, (uintptr_t) path, mode));
}

int access(const char *path, int amode) {
  return errno_value_call(
      linux_syscall2(__NR_access, (uintptr_t) path, amode));
}

int readlink(const char *path, char *buf, int bufsize) {
  return errno_value_call(
      linux_syscall3(__NR_readlink, (uintptr_t) path,
                     (uintptr_t) buf, bufsize));
}

int fcntl(int fd, int cmd, ...) {
  if (cmd == F_GETFD) {
    return errno_value_call(linux_syscall2(__NR_fcntl64, fd, cmd));
  }
  if (cmd == F_GETFL) {
    int rc = errno_value_call(linux_syscall2(__NR_fcntl64, fd, cmd));
    if (rc == -1)
      return -1;
    return linux_oflags_to_nacl_oflags(rc);
  }
  if (cmd == F_SETFD) {
    va_list ap;
    va_start(ap, cmd);
    int32_t arg = va_arg(ap, int32_t);
    va_end(ap);
    return errno_value_call(linux_syscall3(__NR_fcntl64, fd, cmd, arg));
  }
  if (cmd == F_SETFL) {
    va_list ap;
    va_start(ap, cmd);
    int32_t arg = va_arg(ap, int32_t);
    va_end(ap);
    arg = nacl_oflags_to_linux_oflags(arg);
    if (arg == -1) {
      errno = EINVAL;
      return -1;
    }
    return errno_value_call(linux_syscall3(__NR_fcntl64, fd, cmd, arg));
  }
  /* We only support the fcntl commands above. */
  errno = EINVAL;
  return -1;
}

int getpid(void) {
  return errno_value_call(linux_syscall0(__NR_getpid));
}

int fork(void) {
  /* Set SIGCHLD as flag so we can wait. */
  return errno_value_call(
      linux_syscall5(__NR_clone, LINUX_SIGCHLD,
                     0 /* stack */, 0 /* ptid */, 0 /* tls */, 0 /* ctid */));
}

#if defined(__i386__) || defined(__arm__)
struct linux_termios {
  uint32_t c_iflag;
  uint32_t c_oflag;
  uint32_t c_cflag;
  uint32_t c_lflag;
  int8_t c_line;
  int8_t c_cc[19];
};
#else
# error Unsupported architecture
#endif

int isatty(int fd) {
  struct linux_termios term;
  return errno_value_call(
      linux_syscall3(__NR_ioctl, fd, LINUX_TCGETS, (uintptr_t) &term)) == 0;
}

int pipe(int pipefd[2]) {
  return errno_value_call(linux_syscall1(__NR_pipe, (uintptr_t) pipefd));
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  return errno_value_call(
      linux_syscall3(__NR_poll, (uintptr_t) fds, nfds, timeout));
}

int prctl(int option, ...) {
  va_list ap;
  va_start(ap, option);
  uintptr_t arg2 = va_arg(ap, uintptr_t);
  uintptr_t arg3 = va_arg(ap, uintptr_t);
  uintptr_t arg4 = va_arg(ap, uintptr_t);
  uintptr_t arg5 = va_arg(ap, uintptr_t);
  va_end(ap);

  return errno_value_call(
      linux_syscall5(__NR_prctl, option, arg2, arg3, arg4, arg5));
}

#if defined(__i386__)
/* On x86-32 Linux, socket related syscalls are defined by using socketcall. */

static uintptr_t socketcall(int op, void *args) {
  return errno_value_call(
      linux_syscall2(__NR_socketcall, op, (uintptr_t) args));
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  uint32_t args[] = { sockfd, (uintptr_t) msg, flags };
  return socketcall(SYS_RECVMSG, args);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  uint32_t args[] = { sockfd, (uintptr_t) msg, flags };
  return socketcall(SYS_SENDMSG, args);
}

int shutdown(int sockfd, int how) {
  uint32_t args[] = { sockfd, how };
  return socketcall(SYS_SHUTDOWN, args);
}

int socketpair(int domain, int type, int protocol, int sv[2]) {
  uint32_t args[] = { domain, type, protocol, (uintptr_t) sv };
  return socketcall(SYS_SOCKETPAIR, args);
}

#elif defined(__arm__)
/* On ARM Linux, socketcall is not defined. Instead use each syscall. */

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  return errno_value_call(
      linux_syscall3(__NR_recvmsg, sockfd, (uintptr_t) msg, flags));
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  return errno_value_call(
      linux_syscall3(__NR_sendmsg, sockfd, (uintptr_t) msg, flags));
}

int shutdown(int sockfd, int how) {
  return errno_value_call(linux_syscall2(__NR_shutdown, sockfd, how));
}

int socketpair(int domain, int type, int protocol, int sv[2]) {
  return errno_value_call(
      linux_syscall4(__NR_socketpair, domain, type, protocol, (uintptr_t) sv));
}

#else
# error Unsupported architecture
#endif

pid_t waitpid(pid_t pid, int *status, int options) {
  return errno_value_call(
      linux_syscall4(__NR_wait4, pid, (uintptr_t) status, options,
                     0  /* rusage */));
}

int getrlimit(int resource, struct rlimit *rlim) {
  return errno_value_call(
      linux_syscall2(__NR_ugetrlimit, resource, (uintptr_t) rlim));
}

int setrlimit(int resource, const struct rlimit *rlim) {
  return errno_value_call(
      linux_syscall2(__NR_setrlimit, resource, (uintptr_t) rlim));
}

int linux_getdents64(int fd, struct linux_abi_dirent64 *dirp, int count) {
  return errno_value_call(
      linux_syscall3(__NR_getdents64, fd, (uintptr_t) dirp, count));
}

int linux_sigaction(int signum, const struct linux_sigaction *act,
                    struct linux_sigaction *oldact) {
  /*
   * We do not support returning via sigreturn from a signal handler
   * invoked by a real time signal. To support this, we need to set
   * sa_restorer when it is not set by the caller, but we probably
   * will not need this. See the following for how we do it.
   * https://code.google.com/p/linux-syscall-support/source/browse/trunk/lss/linux_syscall_support.h
   */
  return errno_value_call(
      linux_syscall4(__NR_rt_sigaction, signum,
                     (uintptr_t) act, (uintptr_t) oldact,
                     sizeof(act->sa_mask)));
}

int linux_sigprocmask(int how,
                      const linux_sigset_t *set,
                      linux_sigset_t *oset) {
  return errno_value_call(
      linux_syscall4(__NR_rt_sigprocmask, how,
                     (uintptr_t) set, (uintptr_t) oset,
                     sizeof(*set)));
}

int linux_tgkill(int tgid, int tid, int sig) {
  return errno_value_call(
      linux_syscall3(__NR_tgkill, tgid, tid, sig));
}

/*
 * Obtain Linux signal number from portable signal number.
 */
static int nacl_signum_to_linux_signum(int signum) {
  /* SIGSTKFLT is not defined in newlib, hence no mapping. */
#define HANDLE_SIGNUM(SIGNUM) case SIGNUM: return LINUX_##SIGNUM;
  switch(signum) {
    HANDLE_SIGNUM(SIGHUP);
    HANDLE_SIGNUM(SIGINT);
    HANDLE_SIGNUM(SIGQUIT);
    HANDLE_SIGNUM(SIGILL);
    HANDLE_SIGNUM(SIGTRAP);
    HANDLE_SIGNUM(SIGABRT);
    HANDLE_SIGNUM(SIGBUS);
    HANDLE_SIGNUM(SIGFPE);
    HANDLE_SIGNUM(SIGKILL);
    HANDLE_SIGNUM(SIGUSR1);
    HANDLE_SIGNUM(SIGSEGV);
    HANDLE_SIGNUM(SIGUSR2);
    HANDLE_SIGNUM(SIGPIPE);
    HANDLE_SIGNUM(SIGALRM);
    HANDLE_SIGNUM(SIGTERM);
    HANDLE_SIGNUM(SIGSYS);
  }
#undef HANDLE_SIGNUM
  errno = EINVAL;
  return -1;
}

sighandler_t signal(int signum, sighandler_t handler) {
  int linux_signum = nacl_signum_to_linux_signum(signum);
  if (linux_signum == -1)
    return SIG_ERR;

  struct linux_sigaction sa;
  memset(&sa, 0, sizeof(sa));
  /*
   * In Linux's sigaction, sa_sigaction and sa_handler share the same
   * memory region by union.
   */
  sa.sa_sigaction = (void (*)(int, linux_siginfo_t *, void *)) handler;
  sa.sa_flags = LINUX_SA_RESTART;
  /*
   * Reuse the sigemptyset/sigaddset for the first 32 bits of the
   * sigmask. Works on little endian systems only.
   */
  sigset_t *mask = (sigset_t *)&sa.sa_mask;
  sigemptyset(mask);
  sigaddset(mask, linux_signum);
  struct linux_sigaction osa;
  int result = linux_sigaction(linux_signum, &sa, &osa);
  if (result != 0)
    return SIG_ERR;
  return (sighandler_t) osa.sa_sigaction;
}

/*
 * This is a stub since _start will call it but we don't want to
 * do the normal initialization.
 */
void __libnacl_irt_init(Elf32_auxv_t *auxv) {
}

int nacl_tls_init(void *thread_ptr) {
#if defined(__i386__)
  struct linux_user_desc desc = create_linux_user_desc(
      1 /* allocate_new_entry */, thread_ptr);
  uint32_t result = linux_syscall1(__NR_set_thread_area, (uint32_t) &desc);
  if (result != 0)
    __builtin_trap();
  /*
   * Leave the segment selector's bit 2 (table indicator) as zero because
   * set_thread_area() always allocates an entry in the GDT.
   */
  int privilege_level = 3;
  int gs_segment_selector = (desc.entry_number << 3) + privilege_level;
  __asm__("mov %0, %%gs" : : "r"(gs_segment_selector));
#elif defined(__arm__)
  uint32_t result = linux_syscall1(__NR_ARM_set_tls, (uint32_t) thread_ptr);
  if (result != 0)
    __builtin_trap();
#else
# error Unsupported architecture
#endif
  /*
   * Sanity check: Ensure that the thread pointer reads back correctly.
   * This checks that the set_thread_area() syscall worked and that the
   * thread pointer points to itself, which is required on x86-32.
   */
  if (__nacl_read_tp() != thread_ptr)
    __builtin_trap();
  return 0;
}

void *nacl_tls_get(void) {
  void *result;
#if defined(__i386__)
  __asm__("mov %%gs:0, %0" : "=r"(result));
#elif defined(__arm__)
  __asm__("mrc p15, 0, %0, c13, c0, 3" : "=r"(result));
#endif
  return result;
}

int linux_clone_wrapper(uintptr_t fn, uintptr_t arg,
                        int flags, void *child_stack,
                        void *ptid, void *thread_ptr, void *ctid) {
  /*
   * The prototype of clone(2) is
   *
   * clone(int flags, void *stack, pid_t *ptid, void *tls, pid_t *ctid);
   *
   * See linux_syscall_wrappers.h for syscalls' calling conventions.
   */

#if defined(__i386__)
  /* On i386 architecture, we need to wrap thread_ptr by user_desc. */
  struct linux_user_desc desc;
  if (flags & CLONE_SETTLS) {
    desc = create_linux_user_desc(
        0 /* allocate_new_entry */, thread_ptr);
    thread_ptr = &desc;
  }
#endif

  /*
   * This function is called only from clone() below or
   * nacl_irt_thread_create() defined in linux_pthread_private.c.
   * In both cases, |child_stack| will never be NULL, although it is allowed
   * for direct clone() syscall. So, we skip that case's implementation for
   * simplicity here.
   *
   * Here we reserve 6 * 4 bytes for three purposes described below:
   * 1) At the beginning of the child process, we call fn(arg). To pass
   *    the function pointer and arguments, we use |stack| for |arg|,
   *    |stack + 4| for |fn|.
   * 2) Our syscall() implementation reads six 4-byte arguments regardless
   *    of its actual arguments.
   * 3) Similar to 2), our clone() implementation reads three 4-byte arguments
   *    regardless of its actual arguments.
   * So, here we need max size of those three cases (= 6 * 4 bytes) on top of
   * the stack, with 16-byte alignment.
   */
  static const int kStackAlignmentMask = ~15;
  void *stack = (void *) (((uintptr_t) child_stack - sizeof(uintptr_t) * 6) &
                          kStackAlignmentMask);
  /* Put |fn| and |arg| on child process's stack. */
  ((uintptr_t *) stack)[0] = arg;
  ((uintptr_t *) stack)[1] = fn;

#if defined(__i386__)
  uint32_t result;
  __asm__ __volatile__("int $0x80\n"
                       /*
                        * If the return value of clone is non-zero, we are
                        * in the parent thread of clone.
                        */
                       "cmp $0, %%eax\n"
                       "jne 0f\n"
                       /*
                        * In child thread. Clear the frame pointer to
                        * prevent debuggers from unwinding beyond this.
                        */
                       "mov $0, %%ebp\n"
                       /*
                        * Call fn(arg). Note that |arg| is already ready on top
                        * of the stack, here.
                        */
                       "call *4(%%esp)\n"
                       /* Then call _exit(2) with the return value. */
                       "mov %%eax, %%ebx\n"
                       "mov %[exit_sysno], %%eax\n"
                       "int $0x80\n"
                       /* _exit(2) will never return. */
                       "hlt\n"
                       "0:\n"
                       : "=a"(result)
                       : "a"(__NR_clone), "b"(flags), "c"(stack),
                         "d"(ptid), "S"(&desc), "D"(ctid),
                         [exit_sysno] "I"(__NR_exit)
                       : "memory");
#elif defined(__arm__)
  register uint32_t result __asm__("r0");
  register uint32_t sysno __asm__("r7") = __NR_clone;
  register uint32_t a1 __asm__("r0") = flags;
  register uint32_t a2 __asm__("r1") = (uint32_t) stack;
  register uint32_t a3 __asm__("r2") = (uint32_t) ptid;
  register uint32_t a4 __asm__("r3") = (uint32_t) thread_ptr;
  register uint32_t a5 __asm__("r4") = (uint32_t) ctid;
  __asm__ __volatile__("svc #0\n"
                       /*
                        * If the return value of clone is non-zero, we are
                        * in the parent thread of clone.
                        */
                       "cmp r0, #0\n"
                       "bne 0f\n"
                       /*
                        * In child thread. Clear the frame pointer to
                        * prevent debuggers from unwinding beyond this,
                        * load start_func from the stack and call it.
                        */
                       "mov fp, #0\n"
                       /* Load |arg| to r0 register, then call |fn|. */
                       "ldr r0, [sp]\n"
                       "ldr r1, [sp, #4]\n"
                       "blx r1\n"
                       /*
                        * Then, call _exit(2) with the returned value.
                        * r0 keeps the return value of |fn(arg)|.
                        */
                       "mov r7, %[exit_sysno]\n"
                       "svc #0\n"
                       /* _exit(2) will never return. */
                       "bkpt #0\n"
                       "0:\n"
                       : "=r"(result)
                       : "r"(sysno),
                         "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                         [exit_sysno] "i"(__NR_exit)
                       : "memory");
#else
# error Unsupported architecture
#endif
  return result;
}

int clone(int (*fn)(void *), void *child_stack, int flags, void *arg, ...) {
  if (child_stack == NULL) {
    /*
     * NULL child_stack is not allowed, although it is on direct clone()
     * syscall.
     */
    errno = EINVAL;
    return -1;
  }

  /* Read three arguments regardless whether the caller passes it or not. */
  va_list ap;
  va_start(ap, arg);
  void *ptid = va_arg(ap, void *);
  void *tls = va_arg(ap, void *);
  void *ctid = va_arg(ap, void *);
  va_end(ap);

  return errno_value_call(linux_clone_wrapper(
      (uintptr_t) fn, (uintptr_t) arg, flags, child_stack, ptid, tls, ctid));
}
