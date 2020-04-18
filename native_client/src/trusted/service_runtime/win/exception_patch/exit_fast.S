/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"

/*
 * This routine terminates the process safely and is patched onto
 * NTDLL's KiUserExceptionDispatcher.
 *
 * To terminate the process safely, we set %rsp to zero and then
 * execute a HLT instruction.  If we do not set %rsp to zero, the HLT
 * instruction will cause the kernel to rerun the fault handler after
 * pushing another frame onto the stack, and the process will only
 * terminate after running out of stack.  Setting %rsp to a location
 * where another stack frame cannot be written stops the kernel from
 * rerunning the fault handler.
 *
 * We cannot do a system call to terminate the process because
 * Windows' syscall interface is undocumented/unstable and we cannot
 * call an official syscall wrapper without having a safe stack.  (Of
 * course, KiUserExceptionDispatcher is unofficial too, but we want to
 * minimise the number of assumptions here!)
 */

        .globl IDENTIFIER(NaCl_exception_dispatcher_exit_fast)
IDENTIFIER(NaCl_exception_dispatcher_exit_fast):
        xor %rsp, %rsp
        hlt
        .globl IDENTIFIER(NaCl_exception_dispatcher_exit_fast_end)
IDENTIFIER(NaCl_exception_dispatcher_exit_fast_end):
