/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time, non-platform specific system call helper routines.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYSCALL_COMMON_H__
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYSCALL_COMMON_H__ 1

#include "native_client/src/include/portability.h"

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"

EXTERN_C_BEGIN

struct NaClApp;
struct NaClAppThread;
struct NaClSocketAddress;
struct NaClDesc;
struct nacl_abi_stat;

int32_t NaClSysNotImplementedDecoder(struct NaClAppThread *natp);

void NaClAddSyscall(struct NaClApp *nap, uint32_t num,
                    int32_t (*fn)(struct NaClAppThread *));

int32_t NaClSysNull(struct NaClAppThread *natp);

int NaClHighResolutionTimerEnabled(void);

int32_t NaClSysGetpid(struct NaClAppThread *natp);

int32_t NaClSysExit(struct NaClAppThread  *natp,
                    int                   status);

int32_t NaClSysThreadExit(struct NaClAppThread  *natp,
                          uint32_t              stack_flag_addr);

extern int NaClAclBypassChecks;

void NaClInsecurelyBypassAllAclChecks(void);

/*
 * NaClRootDir will not be NULL when a root dir has been provided and NULL
 * otherwise.
 */
extern char *NaClRootDir;
extern size_t NaClRootDirLen;

/* bool */
int NaClMountRootDir(const char *root);

/* bool */
int NaClFileAccessEnabled(void);

/* bool */
int NaClSysCommonAddrRangeContainsExecutablePages(struct NaClApp *nap,
                                                  uintptr_t usraddr,
                                                  size_t length);

/* bool */
int NaClSysCommonAddrRangeInAllowedDynamicCodeSpace(struct NaClApp *nap,
                                                    uintptr_t usraddr,
                                                    size_t length);

int32_t NaClSysGetTimeOfDay(struct NaClAppThread *natp,
                            uint32_t             tv_addr);

int32_t NaClSysClockGetRes(struct NaClAppThread *natp,
                           int                  clk_id,
                           uint32_t             tsp);

int32_t NaClSysClockGetTime(struct NaClAppThread  *natp,
                            int                   clk_id,
                            uint32_t              tsp);

int32_t NaClSysTlsInit(struct NaClAppThread  *natp,
                       uint32_t              thread_ptr);

int32_t NaClSysThreadCreate(struct NaClAppThread *natp,
                            uint32_t             prog_ctr,
                            uint32_t             stack_ptr,
                            uint32_t             thread_ptr,
                            uint32_t             second_thread_ptr);

int32_t NaClSysTlsGet(struct NaClAppThread *natp);

int32_t NaClSysSecondTlsSet(struct NaClAppThread *natp,
                            uint32_t             new_value);

int32_t NaClSysSecondTlsGet(struct NaClAppThread *natp);

int32_t NaClSysThreadNice(struct NaClAppThread *natp,
                          const int nice);

/* mutex */

int32_t NaClSysMutexCreate(struct NaClAppThread *natp);

int32_t NaClSysMutexLock(struct NaClAppThread *natp,
                         int32_t              mutex_handle);

int32_t NaClSysMutexUnlock(struct NaClAppThread *natp,
                           int32_t              mutex_handle);

int32_t NaClSysMutexTrylock(struct NaClAppThread *natp,
                            int32_t              mutex_handle);

/* condition variable */

int32_t NaClSysCondCreate(struct NaClAppThread *natp);

int32_t NaClSysCondWait(struct NaClAppThread *natp,
                        int32_t              cond_handle,
                        int32_t              mutex_handle);

int32_t NaClSysCondSignal(struct NaClAppThread *natp,
                          int32_t              cond_handle);

int32_t NaClSysCondBroadcast(struct NaClAppThread *natp,
                             int32_t              cond_handle);

int32_t NaClSysCondTimedWaitAbs(struct NaClAppThread *natp,
                                int32_t              cond_handle,
                                int32_t              mutex_handle,
                                uint32_t             ts_addr);

/* Semaphores */
int32_t NaClSysSemCreate(struct NaClAppThread *natp,
                         int32_t              init_value);

int32_t NaClSysSemWait(struct NaClAppThread *natp,
                       int32_t              sem_handle);

int32_t NaClSysSemPost(struct NaClAppThread *natp,
                       int32_t              sem_handle);

int32_t NaClSysSemGetValue(struct NaClAppThread *natp,
                           int32_t              sem_handle);

int32_t NaClSysNanosleep(struct NaClAppThread *natp,
                         uint32_t             req_addr,
                         uint32_t             rem_addr);

int32_t NaClSysSchedYield(struct NaClAppThread *natp);

int32_t NaClSysSysconf(struct NaClAppThread *natp,
                       int32_t              name,
                       uint32_t             result_addr);

int32_t NaClSysTestInfoLeak(struct NaClAppThread *natp);

int32_t NaClSysTestCrash(struct NaClAppThread *natp, int crash_type);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYSCALL_COMMON_H__ */
