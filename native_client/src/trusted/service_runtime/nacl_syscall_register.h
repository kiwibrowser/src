/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_SYSCALL_REGISTER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_SYSCALL_REGISTER_H_ 1

#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"

/*
 * This file defines macros for defining and registering NaCl syscalls.
 *
 *  * NACL_DEFINE_SYSCALL_<N>(Func) defines a syscall, implemented by
 *    Func(), that takes N arguments.  This should be used at the top
 *    level, outside of any function.  It defines a wrapper function that
 *    retrieves the syscall's arguments and passes them to Func().
 *
 *  * NACL_REGISTER_SYSCALL(Func, SysNum) registers the syscall implemented
 *    by Func() at run time, with syscall number |SysNum|.
 *
 * Example usage:
 *
 *   NACL_DEFINE_SYSCALL_3(NaClSysRead)
 *
 *   void RegisterMySyscalls(void) {
 *     NACL_REGISTER_SYSCALL(NaClSysRead, NACL_sys_read);
 *   }
 */


/*
 * Internal helper macro which defines a function which reads in NUM_ARGS
 * syscall arguments.  It calls CALL_EXPR, which may refer to
 * |sys_args|.
 */
#define NACL_DEFINE_SYSCALL_N(NUM_ARGS, FUNC, CALL_EXPR) \
    static int32_t FUNC##Decoder(struct NaClAppThread *natp) { \
      uint32_t sys_args[NUM_ARGS]; \
      if (!NaClCopyInFromUserAndDropLock(natp->nap, sys_args, \
                                         natp->usr_syscall_args, \
                                         sizeof(sys_args))) { \
        return -NACL_ABI_EFAULT; \
      } \
      return CALL_EXPR; \
    }


#define NACL_DEFINE_SYSCALL_0(FUNC) \
    static int32_t FUNC##Decoder(struct NaClAppThread *natp) { \
      NaClCopyDropLock(natp->nap); \
      return FUNC(natp); \
    }

#define NACL_DEFINE_SYSCALL_1(FUNC) \
    NACL_DEFINE_SYSCALL_N(1, FUNC, FUNC(natp, sys_args[0]))

#define NACL_DEFINE_SYSCALL_2(FUNC) \
    NACL_DEFINE_SYSCALL_N(2, FUNC, FUNC(natp, sys_args[0], sys_args[1]))

#define NACL_DEFINE_SYSCALL_3(FUNC) \
    NACL_DEFINE_SYSCALL_N(3, FUNC, \
                          FUNC(natp, sys_args[0], sys_args[1], sys_args[2]))

#define NACL_DEFINE_SYSCALL_4(FUNC) \
    NACL_DEFINE_SYSCALL_N(4, FUNC, \
                          FUNC(natp, sys_args[0], sys_args[1], sys_args[2], \
                                     sys_args[3]))

#define NACL_DEFINE_SYSCALL_5(FUNC) \
    NACL_DEFINE_SYSCALL_N(5, FUNC, \
                          FUNC(natp, sys_args[0], sys_args[1], sys_args[2], \
                                     sys_args[3], sys_args[4]))

#define NACL_DEFINE_SYSCALL_6(FUNC) \
    NACL_DEFINE_SYSCALL_N(6, FUNC, \
                          FUNC(natp, sys_args[0], sys_args[1], sys_args[2], \
                                     sys_args[3], sys_args[4], sys_args[5]))

#define NACL_REGISTER_SYSCALL(NAP, FUNC, NUM) \
    NaClAddSyscall((NAP), (NUM), FUNC##Decoder)

#endif
