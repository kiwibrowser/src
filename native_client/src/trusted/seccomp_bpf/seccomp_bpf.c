/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Seccomp filter policy for x86-64 with BPF macros
 */

#include <linux/ptrace.h>

#include "native_client/src/include/nacl_compiler_annotations.h"

#if defined(PTRACE_O_TRACESECCOMP)
#define SUPPORTED_OS 1
#endif

#if defined(SUPPORTED_OS)

#include <linux/audit.h>
#include <linux/errno.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SyscallArg(n) (offsetof(struct seccomp_data, args[n]))
#define SyscallArch (offsetof(struct seccomp_data, arch))
#define SyscallNr (offsetof(struct seccomp_data, nr))


#define REG_RESULT  REG_RAX
#define REG_SYSCALL  REG_RAX
#define REG_ARG0  REG_RDI
#define REG_ARG1  REG_RSI
#define REG_ARG2  REG_RDX
#define REG_ARG3  REG_R10
#define REG_ARG4  REG_R8
#define REG_ARG5  REG_R9

#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS 38
#endif

#ifndef SYS_SECCOMP
#define SYS_SECCOMP 1
#endif

#define BUF_SIZE 1024

static void NaClSeccompBpfSigsysHandler(int nr, siginfo_t *info,
                                        void *void_context) {
  ucontext_t *ctx = (ucontext_t *) void_context;
  int syscall;
  char buf[BUF_SIZE];
  int n;
  char *filename;
  int fatal = 1;
  UNREFERENCED_PARAMETER(nr);
  UNREFERENCED_PARAMETER(info);

  syscall = ctx->uc_mcontext.gregs[REG_SYSCALL];
  switch (syscall) {
    case SYS_open:
      filename = (char *) ctx->uc_mcontext.gregs[REG_ARG0];
      n = snprintf(buf, BUF_SIZE,
                   "[SECCOMP BPF] Blocked open(\"%s\")\n", filename);
      fatal = 0;
      break;
    default:
      n = snprintf(buf, BUF_SIZE, "[SECCOMP BPF] "
                   "Linux syscall %d is not allowed by the policy\n",
                   syscall);
      break;
  }
  if (write(STDERR_FILENO, buf, n)) {}
  if (fatal) {
    _exit(255);
  }
  ctx->uc_mcontext.gregs[REG_RESULT] = -EACCES;
}

static struct sock_filter filter[] = {
  /*
   * Check that an x86-64 syscall is called.
   * This will save us from the case when x86-64 process
   * calls an x86-32 system call using int $0x80.
   * See http://scary.beasts.org/security/CESA-2009-001.html
   */
  BPF_STMT(BPF_LD + BPF_W + BPF_ABS, SyscallArch),
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, AUDIT_ARCH_X86_64, 1, 0),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_KILL),

  /* Grab the system call number */
  BPF_STMT(BPF_LD + BPF_W + BPF_ABS, SyscallNr),
  /* Jump table for the allowed syscalls */
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_rt_sigreturn, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

#ifdef __NR_sigreturn
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_sigreturn, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

  /*
   * Note that gettid is an information leak if not running
   * under an outer sandbox that uses CLONE_NEWPID.
   * It's allowed here for the benefit of the ASan runtime.
   */
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_gettid, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_exit_group, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_exit, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_read, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_write, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_brk, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_mmap, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

#ifdef __NR_mmap2
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_mmap2, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_munmap, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_mprotect, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

#ifdef __NR_pread
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_pread, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

#ifdef __NR_pwrite
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_pwrite, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

#ifdef __NR_pread64
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_pread64, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

#ifdef __NR_pwrite64
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_pwrite64, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

#ifdef __NR_modify_ldt
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_modify_ldt, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_clone, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

#ifdef __NR_flock
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_flock, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_set_robust_list, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

#ifdef __NR_socketpair
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_socketpair, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

#ifdef __NR_socketcall
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_socketcall, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_close, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_dup, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_dup2, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_rt_sigaction, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_futex, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_clock_getres, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_nanosleep, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_time, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_times, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_clock_gettime, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_gettimeofday, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_fstat, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

#ifdef __NR_fstat64
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_fstat64, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_stat, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

#ifdef __NR_stat64
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_stat64, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

#ifdef __NR_sendmsg
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_sendmsg, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

#ifdef __NR_recvmsg
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_recvmsg, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),
#endif

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_sigaltstack, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_madvise, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_exit, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_sched_yield, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_sched_getaffinity, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  /* for abort(), called as tgkill(pid,tid,SIGABORT) */
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_tgkill, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  /* for abort(), called as rt_sigprocmask(SIG_UNBLOCK, [ABRT], NULL, 8) */
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_rt_sigprocmask, 0, 1),
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW),

  /* send SIGSYS for other syscalls, not listed above */
  BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_TRAP),
};

int NaClInstallBpfFilter(void) {
  struct sock_fprog prog = {
    .len = (uint16_t) (sizeof(filter) / sizeof(filter[0])),
    .filter = filter,
  };
  sigset_t mask;
  struct sigaction act;

  /* Unmask SIGSYS */
  if (sigemptyset(&mask) ||
      sigaddset(&mask, SIGSYS)) {
    perror("sig{empty,add}set");
    return 1;
  }

  memset(&act, 0, sizeof(act));
  act.sa_sigaction = &NaClSeccompBpfSigsysHandler;
  act.sa_flags = SA_SIGINFO | SA_ONSTACK;
  if (sigemptyset(&act.sa_mask)) {
    perror("sigemptyset");
    return 1;
  }
  if (sigaction(SIGSYS, &act, NULL) < 0) {
    perror("sigaction");
    return 1;
  }

  if (sigprocmask(SIG_UNBLOCK, &mask, NULL)) {
    perror("unmasking SIGSYS");
    return 1;
  }

  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
    perror("prctl(NO_NEW_PRIVS)");
    return 1;
  }

  if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog)) {
    perror("prctl");
    return 1;
  }
  return 0;
}

#else   /* SUPPORTED_ARCH_AND_OS */

int NaClInstallBpfFilter(void) {
  return 1;
}

#endif  /* SUPPORTED_ARCH_AND_OS */
