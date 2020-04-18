/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time, non-platform specific system call helper routines.
 */
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>

#include "native_client/src/include/build_config.h"

#if NACL_WINDOWS
#include <windows.h>
#endif

#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability_process.h"
#include "native_client/src/include/portability_string.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_clock.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/nacl_time.h"

#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_cond.h"
#include "native_client/src/trusted/desc/nacl_desc_mutex.h"
#include "native_client/src/trusted/desc/nacl_desc_semaphore.h"

#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/include/sys/unistd.h"

#include "native_client/src/trusted/service_runtime/include/sys/nacl_test_crash.h"

#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_handlers.h"
#include "native_client/src/trusted/service_runtime/nacl_thread_nice.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


int32_t NaClSysNotImplementedDecoder(struct NaClAppThread *natp) {
  NaClCopyDropLock(natp->nap);
  return -NACL_ABI_ENOSYS;
}

void NaClAddSyscall(struct NaClApp *nap, uint32_t num,
                    int32_t (*fn)(struct NaClAppThread *)) {
  CHECK(num < NACL_MAX_SYSCALLS);
  if (nap->syscall_table[num].handler != &NaClSysNotImplementedDecoder) {
    NaClLog(LOG_FATAL, "Duplicate syscall number %d\n", num);
  }
  nap->syscall_table[num].handler = fn;
}

int32_t NaClSysNull(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);
  return 0;
}

int NaClAclBypassChecks = 0;

void NaClInsecurelyBypassAllAclChecks(void) {
  NaClLog(LOG_WARNING, "BYPASSING ALL ACL CHECKS\n");
  NaClAclBypassChecks = 1;
}

char *NaClRootDir = NULL;
size_t NaClRootDirLen = 0;

#if NACL_WINDOWS
int NaClMountRootDir(const char *root) {
  /*
   * TODO(smklein): Implement this functionality (and functionality
   * in sys_filename.c) for Windows.
   */
  return 0;
}

#elif NACL_LINUX || NACL_OSX
int NaClMountRootDir(const char *root) {
  /* realpath mallocs NaClRootDir -- we must free it later */
  NaClRootDir = realpath(root, NULL);
  if (NaClRootDir == NULL) {
    NaClLog(LOG_INFO, "Could not sanitize mount directory\n");
    goto fail;
  }
  NaClRootDirLen = strlen(NaClRootDir);

  /*
   * Currently, mounting at '/' is not allowed.
   * This simplifies some path parsing issues later (for example, when
   * creating a new file in the mounted directory).
   */
  if (NaClRootDirLen <= 1)
    goto fail;
  if (NaClRootDirLen >= NACL_CONFIG_PATH_MAX)
    goto fail;
  return 1;
fail:
  free(NaClRootDir);
  NaClRootDir = NULL;
  NaClRootDirLen = 0;
  return 0;
}
#else
#error Unsupported platform
#endif

int NaClFileAccessEnabled(void) {
  return NaClAclBypassChecks || (NaClRootDir != NULL);
}

int NaClHighResolutionTimerEnabled(void) {
  return NaClAclBypassChecks;
}

int32_t NaClSysGetpid(struct NaClAppThread *natp) {
  int32_t pid;
  UNREFERENCED_PARAMETER(natp);

  if (NaClAclBypassChecks) {
    pid = GETPID();
  } else {
    pid = -NACL_ABI_ENOSYS;
  }
  NaClLog(4, "NaClSysGetpid: returning %d\n", pid);

  return pid;
}

int32_t NaClSysExit(struct NaClAppThread  *natp,
                    int                   status) {
  struct NaClApp *nap = natp->nap;

  NaClLog(1, "Exit syscall handler: %d\n", status);

  (void) NaClReportExitStatus(nap, NACL_ABI_W_EXITCODE(status, 0));

  NaClAppThreadTeardown(natp);
  /* NOTREACHED */
  return -NACL_ABI_EINVAL;
}

int32_t NaClSysThreadExit(struct NaClAppThread  *natp,
                          uint32_t              stack_flag_addr) {
  uint32_t  zero = 0;

  NaClLog(4, "NaClSysThreadExit(0x%08"NACL_PRIxPTR", "
          "0x%08"NACL_PRIx32"\n",
          (uintptr_t) natp, stack_flag_addr);
  /*
   * NB: NaClThreads are never joinable, but the abstraction for NaClApps
   * are.
   */

  if (0 != stack_flag_addr) {
    if (!NaClCopyOutToUser(natp->nap, (uintptr_t) stack_flag_addr,
                           &zero, sizeof zero)) {
      NaClLog(4, "NaClSysThreadExit: ignoring invalid stack_flag_addr\n");
    }
  }

  NaClAppThreadTeardown(natp);
  /* NOTREACHED */
  return -NACL_ABI_EINVAL;
}

int32_t NaClSysTlsInit(struct NaClAppThread  *natp,
                       uint32_t              thread_ptr) {
  int32_t   retval = -NACL_ABI_EINVAL;
  uintptr_t sys_tls;

  NaClLog(3,
          ("Entered NaClSysTlsInit(0x%08"NACL_PRIxPTR
           ", 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp, (uintptr_t) thread_ptr);

  /* Verify that the address in the app's range and translated from
   * nacl module address to service runtime address - a nop on ARM
   */
  sys_tls = NaClUserToSysAddrRange(natp->nap, thread_ptr, 4);
  NaClLog(4,
          "NaClSysTlsInit: thread_ptr 0x%"NACL_PRIx32
          ", sys_tls 0x%"NACL_PRIxPTR"\n",
          thread_ptr, sys_tls);
  if (kNaClBadAddress == sys_tls) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  NaClTlsSetTlsValue1(natp, thread_ptr);
  retval = 0;
cleanup:
  return retval;
}

int32_t NaClSysThreadCreate(struct NaClAppThread *natp,
                            uint32_t             prog_ctr,
                            uint32_t             stack_ptr,
                            uint32_t             thread_ptr,
                            uint32_t             second_thread_ptr) {
  struct NaClApp *nap = natp->nap;
  int32_t     retval = -NACL_ABI_EINVAL;
  uintptr_t   sys_tls;
  uintptr_t   sys_stack;

  NaClLog(3,
          ("Entered NaClSysThreadCreate(0x%08"NACL_PRIxPTR
           " pc=0x%08"NACL_PRIx32", sp=0x%08"NACL_PRIx32", thread_ptr=0x%08"
           NACL_PRIx32")\n"),
          (uintptr_t) natp, prog_ctr, stack_ptr, thread_ptr);

  if (!NaClIsValidJumpTarget(nap, prog_ctr)) {
    NaClLog(LOG_ERROR, "NaClSysThreadCreate: Bad function pointer\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  /* Align the stack pointer. */
  stack_ptr = ((stack_ptr + NACL_STACK_PAD_BELOW_ALIGN)
               & ~NACL_STACK_ALIGN_MASK) - NACL_STACK_PAD_BELOW_ALIGN
              - NACL_STACK_ARGS_SIZE;

  sys_stack = NaClUserToSysAddr(nap, stack_ptr);
  if (kNaClBadAddress == sys_stack) {
    NaClLog(LOG_ERROR, "bad stack\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  sys_tls = NaClUserToSysAddrRange(nap, thread_ptr, 4);
  if (kNaClBadAddress == sys_tls) {
    NaClLog(LOG_ERROR, "bad TLS pointer\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  NaClVmHoleWaitToStartThread(nap);

  retval = NaClCreateAdditionalThread(nap,
                                      prog_ctr,
                                      sys_stack,
                                      thread_ptr,
                                      second_thread_ptr);

cleanup:
  return retval;
}

/*
 * This is not used on x86-64 and its functionality is replaced by
 * NaClGetTlsFastPath1 (see nacl_syscall_64.S).
 */
int32_t NaClSysTlsGet(struct NaClAppThread *natp) {
  return NaClTlsGetTlsValue1(natp);
}

int32_t NaClSysSecondTlsSet(struct NaClAppThread *natp,
                            uint32_t             new_value) {
  NaClTlsSetTlsValue2(natp, new_value);
  return 0;
}

/*
 * This is not used on x86-64 and its functionality is replaced by
 * NaClGetTlsFastPath2 (see nacl_syscall_64.S).
 */
int32_t NaClSysSecondTlsGet(struct NaClAppThread *natp) {
  return NaClTlsGetTlsValue2(natp);
}

int NaClSysThreadNice(struct NaClAppThread *natp,
                      const int            nice) {
  /* Note: implementation of nacl_thread_nice is OS dependent. */
  UNREFERENCED_PARAMETER(natp);
  return nacl_thread_nice(nice);
}

int32_t NaClSysMutexCreate(struct NaClAppThread *natp) {
  struct NaClApp       *nap = natp->nap;
  int32_t              retval = -NACL_ABI_EINVAL;
  struct NaClDescMutex *desc;

  NaClLog(3,
          ("Entered NaClSysMutexCreate(0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp);

  desc = malloc(sizeof(*desc));

  if (!desc || !NaClDescMutexCtor(desc)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  retval = NaClAppSetDescAvail(nap, (struct NaClDesc *) desc);
  desc = NULL;
cleanup:
  free(desc);
  NaClLog(3,
          ("NaClSysMutexCreate(0x%08"NACL_PRIxPTR") = %d\n"),
          (uintptr_t) natp, retval);
  return retval;
}

int32_t NaClSysMutexLock(struct NaClAppThread  *natp,
                         int32_t               mutex_handle) {
  struct NaClApp        *nap = natp->nap;
  int32_t               retval = -NACL_ABI_EINVAL;
  struct NaClDesc       *desc;

  NaClLog(3,
          ("Entered NaClSysMutexLock(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, mutex_handle);

  desc = NaClAppGetDesc(nap, mutex_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->Lock)(desc);
  NaClDescUnref(desc);

cleanup:
  return retval;
}

int32_t NaClSysMutexUnlock(struct NaClAppThread  *natp,
                           int32_t               mutex_handle) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClSysMutexUnlock(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, mutex_handle);

  desc = NaClAppGetDesc(nap, mutex_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->Unlock)(desc);
  NaClDescUnref(desc);

cleanup:
  return retval;
}

int32_t NaClSysMutexTrylock(struct NaClAppThread   *natp,
                            int32_t                 mutex_handle) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClSysMutexTrylock(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, mutex_handle);

  desc = NaClAppGetDesc(nap, mutex_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->TryLock)(desc);
  NaClDescUnref(desc);

cleanup:
  return retval;
}

int32_t NaClSysCondCreate(struct NaClAppThread *natp) {
  struct NaClApp         *nap = natp->nap;
  int32_t                retval = -NACL_ABI_EINVAL;
  struct NaClDescCondVar *desc;

  NaClLog(3,
          ("Entered NaClSysCondCreate(0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp);

  desc = malloc(sizeof(*desc));

  if (!desc || !NaClDescCondVarCtor(desc)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  retval = NaClAppSetDescAvail(nap, (struct NaClDesc *)desc);
  desc = NULL;
cleanup:
  free(desc);
  NaClLog(3,
          ("NaClSysCondCreate(0x%08"NACL_PRIxPTR") = %d\n"),
          (uintptr_t) natp, retval);
  return retval;
}

int32_t NaClSysCondWait(struct NaClAppThread *natp,
                        int32_t              cond_handle,
                        int32_t              mutex_handle) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *cv_desc;
  struct NaClDesc *mutex_desc;

  NaClLog(3,
          ("Entered NaClSysCondWait(0x%08"NACL_PRIxPTR", %d, %d)\n"),
          (uintptr_t) natp, cond_handle, mutex_handle);

  cv_desc = NaClAppGetDesc(nap, cond_handle);

  if (NULL == cv_desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  mutex_desc = NaClAppGetDesc(nap, mutex_handle);
  if (NULL == mutex_desc) {
    NaClDescUnref(cv_desc);
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) cv_desc->base.vtbl)->
            Wait)(cv_desc, mutex_desc);
  NaClDescUnref(cv_desc);
  NaClDescUnref(mutex_desc);

cleanup:
  return retval;
}

int32_t NaClSysCondSignal(struct NaClAppThread *natp,
                          int32_t              cond_handle) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClSysCondSignal(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, cond_handle);

  desc = NaClAppGetDesc(nap, cond_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->Signal)(desc);
  NaClDescUnref(desc);
cleanup:
  return retval;
}

int32_t NaClSysCondBroadcast(struct NaClAppThread  *natp,
                             int32_t               cond_handle) {
  struct NaClApp  *nap = natp->nap;
  struct NaClDesc *desc;
  int32_t         retval = -NACL_ABI_EINVAL;

  NaClLog(3,
          ("Entered NaClSysCondBroadcast(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, cond_handle);

  desc = NaClAppGetDesc(nap, cond_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->Broadcast)(desc);
  NaClDescUnref(desc);

cleanup:
  return retval;
}

int32_t NaClSysCondTimedWaitAbs(struct NaClAppThread *natp,
                                int32_t              cond_handle,
                                int32_t              mutex_handle,
                                uint32_t             ts_addr) {
  struct NaClApp           *nap = natp->nap;
  int32_t                  retval = -NACL_ABI_EINVAL;
  struct NaClDesc          *cv_desc;
  struct NaClDesc          *mutex_desc;
  struct nacl_abi_timespec trusted_ts;

  NaClLog(3,
          ("Entered NaClSysCondTimedWaitAbs(0x%08"NACL_PRIxPTR
           ", %d, %d, 0x%08"NACL_PRIx32")\n"),
          (uintptr_t) natp, cond_handle, mutex_handle, ts_addr);

  if (!NaClCopyInFromUser(nap, &trusted_ts, ts_addr, sizeof trusted_ts)) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  /* TODO(gregoryd): validate ts - do we have a limit for time to wait? */

  cv_desc = NaClAppGetDesc(nap, cond_handle);
  if (NULL == cv_desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  mutex_desc = NaClAppGetDesc(nap, mutex_handle);
  if (NULL == mutex_desc) {
    NaClDescUnref(cv_desc);
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) cv_desc->base.vtbl)->
            TimedWaitAbs)(cv_desc,
                          mutex_desc,
                          &trusted_ts);
  NaClDescUnref(cv_desc);
  NaClDescUnref(mutex_desc);
cleanup:
  return retval;
}

int32_t NaClSysSemCreate(struct NaClAppThread *natp,
                         int32_t              init_value) {
  struct NaClApp           *nap = natp->nap;
  int32_t                  retval = -NACL_ABI_EINVAL;
  struct NaClDescSemaphore *desc;

  NaClLog(3,
          ("Entered NaClSysSemCreate(0x%08"NACL_PRIxPTR
           ", %d)\n"),
          (uintptr_t) natp, init_value);

  desc = malloc(sizeof(*desc));

  if (!desc || !NaClDescSemaphoreCtor(desc, init_value)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  retval = NaClAppSetDescAvail(nap, (struct NaClDesc *) desc);
  desc = NULL;
cleanup:
  free(desc);
  return retval;
}


int32_t NaClSysSemWait(struct NaClAppThread *natp,
                       int32_t              sem_handle) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClSysSemWait(0x%08"NACL_PRIxPTR
           ", %d)\n"),
          (uintptr_t) natp, sem_handle);

  desc = NaClAppGetDesc(nap, sem_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  /*
   * TODO(gregoryd): we have to decide on the syscall API: do we
   * switch to read/write/ioctl API or do we stay with the more
   * detailed API. Anyway, using a single syscall for waiting on all
   * synchronization objects makes sense.
   */
  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->SemWait)(desc);
  NaClDescUnref(desc);
cleanup:
  return retval;
}

int32_t NaClSysSemPost(struct NaClAppThread *natp,
                       int32_t              sem_handle) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClSysSemPost(0x%08"NACL_PRIxPTR
           ", %d)\n"),
          (uintptr_t) natp, sem_handle);

  desc = NaClAppGetDesc(nap, sem_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->Post)(desc);
  NaClDescUnref(desc);
cleanup:
  return retval;
}

int32_t NaClSysSemGetValue(struct NaClAppThread *natp,
                           int32_t              sem_handle) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClSysSemGetValue(0x%08"NACL_PRIxPTR
           ", %d)\n"),
          (uintptr_t) natp, sem_handle);

  desc = NaClAppGetDesc(nap, sem_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->GetValue)(desc);
  NaClDescUnref(desc);
cleanup:
  return retval;
}

int32_t NaClSysNanosleep(struct NaClAppThread *natp,
                         uint32_t             req_addr,
                         uint32_t             rem_addr) {
  struct NaClApp            *nap = natp->nap;
  struct nacl_abi_timespec  t_sleep;
  struct nacl_abi_timespec  t_rem;
  struct nacl_abi_timespec  *remptr;
  int                       retval = -NACL_ABI_EINVAL;

  NaClLog(3,
          ("Entered NaClSysNanosleep(0x%08"NACL_PRIxPTR
           ", 0x%08"NACL_PRIx32", 0x%08"NACL_PRIx32"x)\n"),
          (uintptr_t) natp, req_addr, rem_addr);

  /* do the check before we sleep */
  if (0 != rem_addr && kNaClBadAddress ==
      NaClUserToSysAddrRange(nap, rem_addr, sizeof *remptr)) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  if (!NaClCopyInFromUser(nap, &t_sleep, req_addr, sizeof t_sleep)) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  remptr = (0 == rem_addr) ? NULL : &t_rem;
  /* NULL != remptr \equiv NULL != rem */

  /*
   * We assume that we do not need to normalize the time request values.
   *
   * If bogus values can cause the underlying OS to get into trouble,
   * then we need more checking here.
   */
  NaClLog(4, "NaClSysNanosleep(time = %"NACL_PRId64".%09"NACL_PRId64" S)\n",
          (int64_t) t_sleep.tv_sec, (int64_t) t_sleep.tv_nsec);
  retval = NaClNanosleep(&t_sleep, remptr);
  NaClLog(4, "NaClNanosleep returned %d\n", retval);

  if (-EINTR == retval && 0 != rem_addr &&
      !NaClCopyOutToUser(nap, rem_addr, remptr, sizeof *remptr)) {
    NaClLog(LOG_FATAL, "NaClSysNanosleep: check rem failed at copyout\n");
  }

cleanup:
  NaClLog(4, "nanosleep done.\n");
  return retval;
}

int32_t NaClSysSchedYield(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);

  NaClThreadYield();
  return 0;
}

int32_t NaClSysSysconf(struct NaClAppThread *natp,
                       int32_t              name,
                       uint32_t             result_addr) {
  struct NaClApp  *nap = natp->nap;
  int32_t         retval = -NACL_ABI_EINVAL;
  int32_t         result_value;

  NaClLog(3,
          ("Entered NaClSysSysconf(%08"NACL_PRIxPTR
           "x, %d, 0x%08"NACL_PRIx32")\n"),
          (uintptr_t) natp, name, result_addr);

  switch (name) {
    case NACL_ABI__SC_NPROCESSORS_ONLN: {
#if NACL_WINDOWS
      static int32_t number_of_workers = 0;
      if (0 == number_of_workers) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        number_of_workers = (int32_t) si.dwNumberOfProcessors;
      }
      result_value = number_of_workers;
#elif NACL_LINUX || NACL_OSX
      if (-1 == nap->sc_nprocessors_onln) {
        /* Unable to get the number of processors at startup. */
        goto cleanup;
      }
      result_value = nap->sc_nprocessors_onln;
#else
#error Unsupported platform
#endif
      break;
    }
    case NACL_ABI__SC_PAGESIZE: {
      result_value = 1 << 16;  /* always 64k pages */
      break;
    }
    case NACL_ABI__SC_NACL_FILE_ACCESS_ENABLED: {
      result_value = NaClFileAccessEnabled();
      break;
    }
    case NACL_ABI__SC_NACL_LIST_MAPPINGS_ENABLED: {
      result_value = nap->enable_list_mappings;
      break;
    }
    case NACL_ABI__SC_NACL_PNACL_MODE: {
      result_value = nap->pnacl_mode;
      break;
    }
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
    case NACL_ABI__SC_NACL_CPU_FEATURE_X86: {
      NaClCPUFeaturesX86 *features = (NaClCPUFeaturesX86 *) nap->cpu_features;
      if (nap->pnacl_mode) {
        goto cleanup;
      }
      /*
       * The result value is modelled after the first three bits of XCR0.
       */
      result_value =
        (NaClGetCPUFeatureX86(features, NaClCPUFeatureX86_x87) << 0) |
        (NaClGetCPUFeatureX86(features, NaClCPUFeatureX86_SSE) << 1) |
        (NaClGetCPUFeatureX86(features, NaClCPUFeatureX86_AVX) << 2);
      break;
    }
#endif
    default: {
      retval = -NACL_ABI_EINVAL;
      goto cleanup;
    }
  }
  if (!NaClCopyOutToUser(nap, result_addr, &result_value,
                         sizeof result_value)) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  retval = 0;
cleanup:
  return retval;
}

int32_t NaClSysTestInfoLeak(struct NaClAppThread *natp) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  /*
   * Put some interesting bits into the x87 and SSE registers.
   */
  union fxsave {
    char buf[512];
    struct {
      uint16_t fcw;
      uint16_t fsw;
      uint16_t ftw;
      uint16_t fop;
      union {
        struct {
          uint64_t rip;
          uint64_t rdp;
        } x64;
        struct {
          uint32_t fpu_ip;
          uint32_t cs;
          uint32_t fpu_dp;
          uint32_t ds;
        } ia32;
      } bitness;
      uint32_t mxcsr;
      uint32_t mxcsr_mask;
      struct {
        uint8_t st[10];
        uint8_t reserved[6];
      } st_space[8];
      uint32_t xmm_space[64];
    } fxsave;
  };

  static const char tenbytes[10] = "SecretBit";
  static const char manybytes[256] =
      "Highly sensitive information must not be leaked to untrusted code!\n"
      "xyzzy\nplugh\nYou are likely to be eaten by a grue.\n"
      "When in the Course of human events it becomes necessary for one people"
      " to dissolve the political bands which have connected them with ...\n";

# ifdef __GNUC__
  union fxsave u __attribute__((aligned(16)));
# elif NACL_WINDOWS
  __declspec(align(16)) union fxsave u;
# else
#  error Unsupported platform
# endif

  int i;

# ifdef __GNUC__
  __asm__("fxsave %0" : "=m" (u));
# elif NACL_WINDOWS
#  if NACL_BUILD_SUBARCH == 64
  NaClDoFxsave(&u);
#  else
  __asm {
    fxsave u
  };
#  endif
# else
# error Unsupported platform
# endif

  for (i = 0; i < 8; ++i)
    memcpy(&u.fxsave.st_space[i], tenbytes, sizeof(tenbytes));

  memcpy(u.fxsave.xmm_space, manybytes, sizeof(u.fxsave.xmm_space));

  /*
   * Set the MXCSR to an unlikely (but valid) value: all valid bits set.
   * The mask is provided by the hardware to say which bits can be set
   * (all others are reserved).  The untrusted test code (in
   * tests/infoleak/test_infoleak.c) sets MXCSR to zero before
   * making this system call so this value ensures that the test
   * actually verifies the behavior of the syscall return path.
   */
  u.fxsave.mxcsr = u.fxsave.mxcsr_mask;

# ifdef __GNUC__
  __asm__ volatile("fxrstor %0" :: "m" (u));
# elif NACL_WINDOWS
#  if NACL_BUILD_SUBARCH == 64
  NaClDoFxrstor(&u);
#  else
  __asm {
    fxrstor u
  };
#  endif
# else
# error Unsupported platform
# endif

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  /*
   * Put some interesting bits into the VFP registers.
   */

  static const char manybytes[64] =
      "Sensitive information must not be leaked to untrusted code!!!!\n";

  __asm__ volatile("vldm %0, {d0-d7}" :: "r" (manybytes) :
                   "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7");

  /*
   * __builtin_arm_set_fpscr is not available yet in gcc (not even in 4.8)
   * and clang doesn't know about the "vfpcc" register.
   * TODO(sbc): Always use __builtin_arm_set_fpscr once we are using a
   * version of gcc that supports it.
   */
#ifdef __clang__
  __builtin_arm_set_fpscr(0xdeadbeef);
#else
  __asm__ volatile("fmxr fpscr, %0" :: "r" (0xdeadbeef) : "vfpcc");
#endif

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips

  static const char manybytes[128] =
      "Sensitive information must not be leaked to untrusted code!!!\n"
      "Where is Maletoth?\n";

  __asm__ volatile("ldc1  $f0,  0(%0)\n"
                   "ldc1  $f2,  8(%0)\n"
                   "ldc1  $f4,  16(%0)\n"
                   "ldc1  $f6,  24(%0)\n"
                   "ldc1  $f8,  32(%0)\n"
                   "ldc1  $f10, 40(%0)\n"
                   "ldc1  $f12, 48(%0)\n"
                   "ldc1  $f14, 56(%0)\n"
                   "ldc1  $f16, 64(%0)\n"
                   "ldc1  $f18, 72(%0)\n"
                   : : "r" (manybytes)
                   : "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7",
                     "$f8", "$f9", "$f10", "$f11", "$f12", "$f13", "$f14",
                     "$f15", "$f16", "$f17", "$f18", "$f19");

#endif

  UNREFERENCED_PARAMETER(natp);

  return -NACL_ABI_ENOSYS;
}

/*
 * This syscall is intended for testing NaCl's support for Breakpad
 * crash reporting inside Chromium.  When
 * http://code.google.com/p/nativeclient/issues/detail?id=579 is
 * addressed, we might put this syscall behind a flag.  Until then,
 * untrusted code can trigger Breakpad-reported crashes inside
 * syscalls, so there is no benefit to restricting this syscall.
 */
int32_t NaClSysTestCrash(struct NaClAppThread *natp, int crash_type) {
  /*
   * Despite being volatile, the Apple system compiler, llvm-gcc, still
   * optimizes the null pointer dereference into an illegal instruction when
   * written as a one-liner. That interferes with tests that expect precisely
   * a SIGSEGV, because they'll see a SIGILL instead.
   */
  volatile int *volatile p = 0;
  UNREFERENCED_PARAMETER(natp);

  switch (crash_type) {
    case NACL_TEST_CRASH_MEMORY:
      *p = 0;
      break;
    case NACL_TEST_CRASH_LOG_FATAL:
      NaClLog(LOG_FATAL, "NaClSysTestCrash: This is a test error\n");
      break;
    case NACL_TEST_CRASH_CHECK_FAILURE:
      CHECK(0);
      break;
  }
  return -NACL_ABI_EINVAL;
}

/*
 * This syscall does not take a "tz" (timezone) argument.  tz is not
 * supported in linux, nor is it supported by glibc, since tzset(3) and the
 * zoneinfo file should be used instead.
 */
int32_t NaClSysGetTimeOfDay(struct NaClAppThread *natp,
                            uint32_t             tv_addr) {
  int                     retval;
  struct nacl_abi_timeval now;

  NaClLog(3,
          ("Entered NaClSysGetTimeOfDay(%08"NACL_PRIxPTR
           ", 0x%08"NACL_PRIx32")\n"),
          (uintptr_t) natp, tv_addr);

  /* memset() call is required to clear padding in struct nacl_abi_timeval. */
  memset(&now, 0, sizeof(now));
  retval = NaClGetTimeOfDay(&now);
  if (0 != retval) {
    return retval;
  }
#if !NACL_WINDOWS
  /*
   * Coarsen the time to the same level we get on Windows -
   * 10 microseconds.
   */
  if (!NaClHighResolutionTimerEnabled()) {
    now.nacl_abi_tv_usec = (now.nacl_abi_tv_usec / 10) * 10;
  }
#endif
  CHECK(now.nacl_abi_tv_usec >= 0);
  CHECK(now.nacl_abi_tv_usec < NACL_MICROS_PER_UNIT);
  if (!NaClCopyOutToUser(natp->nap, tv_addr, &now, sizeof now)) {
    return -NACL_ABI_EFAULT;
  }
  return 0;
}

static int NaClIsValidClockId(int clk_id) {
  switch (clk_id) {
    case NACL_ABI_CLOCK_REALTIME:
    case NACL_ABI_CLOCK_MONOTONIC:
    case NACL_ABI_CLOCK_PROCESS_CPUTIME_ID:
    case NACL_ABI_CLOCK_THREAD_CPUTIME_ID:
      return 1;
  }
  return 0;
}

static int32_t NaClSysClockGetCommon(struct NaClAppThread  *natp,
                                     int                   clk_id,
                                     uint32_t              ts_addr,
                                     int                   (*timefunc)(
                                         nacl_clockid_t            clk_id,
                                         struct nacl_abi_timespec  *tp),
                                     int null_ok) {
  struct NaClApp            *nap = natp->nap;
  int                       retval = -NACL_ABI_EINVAL;
  struct nacl_abi_timespec  out_buf;

  if (!NaClIsValidClockId(clk_id)) {
    goto done;
  }
  /* memset() call is required to clear padding in struct nacl_abi_timespec. */
  memset(&out_buf, 0, sizeof(out_buf));
  retval = (*timefunc)((nacl_clockid_t) clk_id, &out_buf);
  if (0 == retval) {
    if (ts_addr == 0 ? !null_ok :
        !NaClCopyOutToUser(nap, (uintptr_t) ts_addr,
                           &out_buf, sizeof out_buf)) {
      retval = -NACL_ABI_EFAULT;
    }
  }
 done:
  return retval;
}

int32_t NaClSysClockGetRes(struct NaClAppThread *natp,
                           int                  clk_id,
                           uint32_t             tsp) {
  return NaClSysClockGetCommon(natp, clk_id, (uintptr_t) tsp,
                               NaClClockGetRes, 1);
}

int32_t NaClSysClockGetTime(struct NaClAppThread  *natp,
                            int                   clk_id,
                            uint32_t              tsp) {
  return NaClSysClockGetCommon(natp, clk_id, (uintptr_t) tsp,
                               NaClClockGetTime, 0);
}
