/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_handlers.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_register.h"
#include "native_client/src/trusted/service_runtime/nacl_text.h"
#include "native_client/src/trusted/service_runtime/sys_clock.h"
#include "native_client/src/trusted/service_runtime/sys_exception.h"
#include "native_client/src/trusted/service_runtime/sys_fdio.h"
#include "native_client/src/trusted/service_runtime/sys_filename.h"
#include "native_client/src/trusted/service_runtime/sys_imc.h"
#include "native_client/src/trusted/service_runtime/sys_list_mappings.h"
#include "native_client/src/trusted/service_runtime/sys_memory.h"
#include "native_client/src/trusted/service_runtime/sys_parallel_io.h"
#include "native_client/src/trusted/service_runtime/sys_random.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"

/*
 * The following declarations define wrapper functions which read in
 * syscall arguments and call the syscall implementation functions listed
 * here.
 */
NACL_DEFINE_SYSCALL_0(NaClSysNull)
NACL_DEFINE_SYSCALL_1(NaClSysDup)
NACL_DEFINE_SYSCALL_2(NaClSysDup2)
NACL_DEFINE_SYSCALL_3(NaClSysOpen)
NACL_DEFINE_SYSCALL_1(NaClSysClose)
NACL_DEFINE_SYSCALL_3(NaClSysRead)
NACL_DEFINE_SYSCALL_3(NaClSysWrite)
NACL_DEFINE_SYSCALL_3(NaClSysLseek)
NACL_DEFINE_SYSCALL_2(NaClSysFstat)
NACL_DEFINE_SYSCALL_2(NaClSysStat)
NACL_DEFINE_SYSCALL_3(NaClSysGetdents)
NACL_DEFINE_SYSCALL_1(NaClSysIsatty)
NACL_DEFINE_SYSCALL_1(NaClSysBrk)
NACL_DEFINE_SYSCALL_6(NaClSysMmap)
NACL_DEFINE_SYSCALL_3(NaClSysMprotect)
NACL_DEFINE_SYSCALL_2(NaClSysListMappings)
NACL_DEFINE_SYSCALL_2(NaClSysMunmap)
NACL_DEFINE_SYSCALL_1(NaClSysExit)
NACL_DEFINE_SYSCALL_0(NaClSysGetpid)
NACL_DEFINE_SYSCALL_1(NaClSysThreadExit)
NACL_DEFINE_SYSCALL_1(NaClSysGetTimeOfDay)
NACL_DEFINE_SYSCALL_0(NaClSysClock)
NACL_DEFINE_SYSCALL_2(NaClSysNanosleep)
NACL_DEFINE_SYSCALL_2(NaClSysClockGetRes)
NACL_DEFINE_SYSCALL_2(NaClSysClockGetTime)
NACL_DEFINE_SYSCALL_2(NaClSysMkdir)
NACL_DEFINE_SYSCALL_1(NaClSysRmdir)
NACL_DEFINE_SYSCALL_1(NaClSysChdir)
NACL_DEFINE_SYSCALL_1(NaClSysFchdir)
NACL_DEFINE_SYSCALL_2(NaClSysFchmod)
NACL_DEFINE_SYSCALL_2(NaClSysFtruncate)
NACL_DEFINE_SYSCALL_1(NaClSysFsync)
NACL_DEFINE_SYSCALL_1(NaClSysFdatasync)
NACL_DEFINE_SYSCALL_2(NaClSysGetcwd)
NACL_DEFINE_SYSCALL_1(NaClSysUnlink)
NACL_DEFINE_SYSCALL_2(NaClSysTruncate)
NACL_DEFINE_SYSCALL_2(NaClSysLstat)
NACL_DEFINE_SYSCALL_2(NaClSysLink)
NACL_DEFINE_SYSCALL_2(NaClSysRename)
NACL_DEFINE_SYSCALL_2(NaClSysSymlink)
NACL_DEFINE_SYSCALL_2(NaClSysChmod)
NACL_DEFINE_SYSCALL_2(NaClSysAccess)
NACL_DEFINE_SYSCALL_3(NaClSysReadlink)
NACL_DEFINE_SYSCALL_2(NaClSysUtimes)
NACL_DEFINE_SYSCALL_4(NaClSysPRead)
NACL_DEFINE_SYSCALL_4(NaClSysPWrite)
NACL_DEFINE_SYSCALL_1(NaClSysImcMakeBoundSock)
NACL_DEFINE_SYSCALL_1(NaClSysImcAccept)
NACL_DEFINE_SYSCALL_1(NaClSysImcConnect)
NACL_DEFINE_SYSCALL_3(NaClSysImcSendmsg)
NACL_DEFINE_SYSCALL_3(NaClSysImcRecvmsg)
NACL_DEFINE_SYSCALL_1(NaClSysImcMemObjCreate)
NACL_DEFINE_SYSCALL_1(NaClSysTlsInit)
NACL_DEFINE_SYSCALL_4(NaClSysThreadCreate)
NACL_DEFINE_SYSCALL_0(NaClSysTlsGet)
NACL_DEFINE_SYSCALL_1(NaClSysThreadNice)
NACL_DEFINE_SYSCALL_0(NaClSysMutexCreate)
NACL_DEFINE_SYSCALL_1(NaClSysMutexLock)
NACL_DEFINE_SYSCALL_1(NaClSysMutexUnlock)
NACL_DEFINE_SYSCALL_1(NaClSysMutexTrylock)
NACL_DEFINE_SYSCALL_0(NaClSysCondCreate)
NACL_DEFINE_SYSCALL_2(NaClSysCondWait)
NACL_DEFINE_SYSCALL_1(NaClSysCondSignal)
NACL_DEFINE_SYSCALL_1(NaClSysCondBroadcast)
NACL_DEFINE_SYSCALL_3(NaClSysCondTimedWaitAbs)
NACL_DEFINE_SYSCALL_1(NaClSysImcSocketPair)
NACL_DEFINE_SYSCALL_1(NaClSysSemCreate)
NACL_DEFINE_SYSCALL_1(NaClSysSemWait)
NACL_DEFINE_SYSCALL_1(NaClSysSemPost)
NACL_DEFINE_SYSCALL_1(NaClSysSemGetValue)
NACL_DEFINE_SYSCALL_0(NaClSysSchedYield)
NACL_DEFINE_SYSCALL_2(NaClSysSysconf)
NACL_DEFINE_SYSCALL_3(NaClSysDyncodeCreate)
NACL_DEFINE_SYSCALL_3(NaClSysDyncodeModify)
NACL_DEFINE_SYSCALL_2(NaClSysDyncodeDelete)
NACL_DEFINE_SYSCALL_1(NaClSysSecondTlsSet)
NACL_DEFINE_SYSCALL_0(NaClSysSecondTlsGet)
NACL_DEFINE_SYSCALL_2(NaClSysExceptionHandler)
NACL_DEFINE_SYSCALL_2(NaClSysExceptionStack)
NACL_DEFINE_SYSCALL_0(NaClSysExceptionClearFlag)
NACL_DEFINE_SYSCALL_0(NaClSysTestInfoLeak)
NACL_DEFINE_SYSCALL_1(NaClSysTestCrash)
NACL_DEFINE_SYSCALL_3(NaClSysFutexWaitAbs)
NACL_DEFINE_SYSCALL_2(NaClSysFutexWake)
NACL_DEFINE_SYSCALL_2(NaClSysGetRandomBytes)

void NaClAppRegisterDefaultSyscalls(struct NaClApp *nap) {
  NACL_REGISTER_SYSCALL(nap, NaClSysNull, NACL_sys_null);
  NACL_REGISTER_SYSCALL(nap, NaClSysDup, NACL_sys_dup);
  NACL_REGISTER_SYSCALL(nap, NaClSysDup2, NACL_sys_dup2);
  NACL_REGISTER_SYSCALL(nap, NaClSysOpen, NACL_sys_open);
  NACL_REGISTER_SYSCALL(nap, NaClSysClose, NACL_sys_close);
  NACL_REGISTER_SYSCALL(nap, NaClSysRead, NACL_sys_read);
  NACL_REGISTER_SYSCALL(nap, NaClSysWrite, NACL_sys_write);
  NACL_REGISTER_SYSCALL(nap, NaClSysLseek, NACL_sys_lseek);
  NACL_REGISTER_SYSCALL(nap, NaClSysFstat, NACL_sys_fstat);
  NACL_REGISTER_SYSCALL(nap, NaClSysStat, NACL_sys_stat);
  NACL_REGISTER_SYSCALL(nap, NaClSysGetdents, NACL_sys_getdents);
  NACL_REGISTER_SYSCALL(nap, NaClSysIsatty, NACL_sys_isatty);
  NACL_REGISTER_SYSCALL(nap, NaClSysBrk, NACL_sys_brk);
  NACL_REGISTER_SYSCALL(nap, NaClSysMmap, NACL_sys_mmap);
  NACL_REGISTER_SYSCALL(nap, NaClSysMprotect, NACL_sys_mprotect);
  NACL_REGISTER_SYSCALL(nap, NaClSysListMappings, NACL_sys_list_mappings);
  NACL_REGISTER_SYSCALL(nap, NaClSysMunmap, NACL_sys_munmap);
  NACL_REGISTER_SYSCALL(nap, NaClSysExit, NACL_sys_exit);
  NACL_REGISTER_SYSCALL(nap, NaClSysGetpid, NACL_sys_getpid);
  NACL_REGISTER_SYSCALL(nap, NaClSysThreadExit, NACL_sys_thread_exit);
  NACL_REGISTER_SYSCALL(nap, NaClSysGetTimeOfDay, NACL_sys_gettimeofday);
  NACL_REGISTER_SYSCALL(nap, NaClSysClock, NACL_sys_clock);
  NACL_REGISTER_SYSCALL(nap, NaClSysNanosleep, NACL_sys_nanosleep);
  NACL_REGISTER_SYSCALL(nap, NaClSysClockGetRes, NACL_sys_clock_getres);
  NACL_REGISTER_SYSCALL(nap, NaClSysClockGetTime, NACL_sys_clock_gettime);
  NACL_REGISTER_SYSCALL(nap, NaClSysMkdir, NACL_sys_mkdir);
  NACL_REGISTER_SYSCALL(nap, NaClSysRmdir, NACL_sys_rmdir);
  NACL_REGISTER_SYSCALL(nap, NaClSysChdir, NACL_sys_chdir);
  NACL_REGISTER_SYSCALL(nap, NaClSysFchdir, NACL_sys_fchdir);
  NACL_REGISTER_SYSCALL(nap, NaClSysFchmod, NACL_sys_fchmod);
  NACL_REGISTER_SYSCALL(nap, NaClSysFtruncate, NACL_sys_ftruncate);
  NACL_REGISTER_SYSCALL(nap, NaClSysFsync, NACL_sys_fsync);
  NACL_REGISTER_SYSCALL(nap, NaClSysFdatasync, NACL_sys_fdatasync);
  NACL_REGISTER_SYSCALL(nap, NaClSysGetcwd, NACL_sys_getcwd);
  NACL_REGISTER_SYSCALL(nap, NaClSysUnlink, NACL_sys_unlink);
  NACL_REGISTER_SYSCALL(nap, NaClSysTruncate, NACL_sys_truncate);
  NACL_REGISTER_SYSCALL(nap, NaClSysLstat, NACL_sys_lstat);
  NACL_REGISTER_SYSCALL(nap, NaClSysLink, NACL_sys_link);
  NACL_REGISTER_SYSCALL(nap, NaClSysRename, NACL_sys_rename);
  NACL_REGISTER_SYSCALL(nap, NaClSysSymlink, NACL_sys_symlink);
  NACL_REGISTER_SYSCALL(nap, NaClSysChmod, NACL_sys_chmod);
  NACL_REGISTER_SYSCALL(nap, NaClSysAccess, NACL_sys_access);
  NACL_REGISTER_SYSCALL(nap, NaClSysReadlink, NACL_sys_readlink);
  NACL_REGISTER_SYSCALL(nap, NaClSysUtimes, NACL_sys_utimes);
  NACL_REGISTER_SYSCALL(nap, NaClSysPRead, NACL_sys_pread);
  NACL_REGISTER_SYSCALL(nap, NaClSysPWrite, NACL_sys_pwrite);
  NACL_REGISTER_SYSCALL(nap, NaClSysImcMakeBoundSock,
                        NACL_sys_imc_makeboundsock);
  NACL_REGISTER_SYSCALL(nap, NaClSysImcAccept, NACL_sys_imc_accept);
  NACL_REGISTER_SYSCALL(nap, NaClSysImcConnect, NACL_sys_imc_connect);
  NACL_REGISTER_SYSCALL(nap, NaClSysImcSendmsg, NACL_sys_imc_sendmsg);
  NACL_REGISTER_SYSCALL(nap, NaClSysImcRecvmsg, NACL_sys_imc_recvmsg);
  NACL_REGISTER_SYSCALL(nap, NaClSysImcMemObjCreate,
                        NACL_sys_imc_mem_obj_create);
  NACL_REGISTER_SYSCALL(nap, NaClSysTlsInit, NACL_sys_tls_init);
  NACL_REGISTER_SYSCALL(nap, NaClSysThreadCreate, NACL_sys_thread_create);
  NACL_REGISTER_SYSCALL(nap, NaClSysTlsGet, NACL_sys_tls_get);
  NACL_REGISTER_SYSCALL(nap, NaClSysThreadNice, NACL_sys_thread_nice);
  NACL_REGISTER_SYSCALL(nap, NaClSysMutexCreate, NACL_sys_mutex_create);
  NACL_REGISTER_SYSCALL(nap, NaClSysMutexLock, NACL_sys_mutex_lock);
  NACL_REGISTER_SYSCALL(nap, NaClSysMutexUnlock, NACL_sys_mutex_unlock);
  NACL_REGISTER_SYSCALL(nap, NaClSysMutexTrylock, NACL_sys_mutex_trylock);
  NACL_REGISTER_SYSCALL(nap, NaClSysCondCreate, NACL_sys_cond_create);
  NACL_REGISTER_SYSCALL(nap, NaClSysCondWait, NACL_sys_cond_wait);
  NACL_REGISTER_SYSCALL(nap, NaClSysCondSignal, NACL_sys_cond_signal);
  NACL_REGISTER_SYSCALL(nap, NaClSysCondBroadcast, NACL_sys_cond_broadcast);
  NACL_REGISTER_SYSCALL(nap, NaClSysCondTimedWaitAbs,
                        NACL_sys_cond_timed_wait_abs);
  NACL_REGISTER_SYSCALL(nap, NaClSysImcSocketPair, NACL_sys_imc_socketpair);
  NACL_REGISTER_SYSCALL(nap, NaClSysSemCreate, NACL_sys_sem_create);
  NACL_REGISTER_SYSCALL(nap, NaClSysSemWait, NACL_sys_sem_wait);
  NACL_REGISTER_SYSCALL(nap, NaClSysSemPost, NACL_sys_sem_post);
  NACL_REGISTER_SYSCALL(nap, NaClSysSemGetValue, NACL_sys_sem_get_value);
  NACL_REGISTER_SYSCALL(nap, NaClSysSchedYield, NACL_sys_sched_yield);
  NACL_REGISTER_SYSCALL(nap, NaClSysSysconf, NACL_sys_sysconf);
  NACL_REGISTER_SYSCALL(nap, NaClSysDyncodeCreate, NACL_sys_dyncode_create);
  NACL_REGISTER_SYSCALL(nap, NaClSysDyncodeModify, NACL_sys_dyncode_modify);
  NACL_REGISTER_SYSCALL(nap, NaClSysDyncodeDelete, NACL_sys_dyncode_delete);
  NACL_REGISTER_SYSCALL(nap, NaClSysSecondTlsSet, NACL_sys_second_tls_set);
  NACL_REGISTER_SYSCALL(nap, NaClSysSecondTlsGet, NACL_sys_second_tls_get);
  NACL_REGISTER_SYSCALL(nap, NaClSysExceptionHandler,
                        NACL_sys_exception_handler);
  NACL_REGISTER_SYSCALL(nap, NaClSysExceptionStack, NACL_sys_exception_stack);
  NACL_REGISTER_SYSCALL(nap, NaClSysExceptionClearFlag,
                        NACL_sys_exception_clear_flag);
  NACL_REGISTER_SYSCALL(nap, NaClSysTestInfoLeak, NACL_sys_test_infoleak);
  NACL_REGISTER_SYSCALL(nap, NaClSysTestCrash, NACL_sys_test_crash);
  NACL_REGISTER_SYSCALL(nap, NaClSysFutexWaitAbs, NACL_sys_futex_wait_abs);
  NACL_REGISTER_SYSCALL(nap, NaClSysFutexWake, NACL_sys_futex_wake);
  NACL_REGISTER_SYSCALL(nap, NaClSysGetRandomBytes, NACL_sys_get_random_bytes);
}
