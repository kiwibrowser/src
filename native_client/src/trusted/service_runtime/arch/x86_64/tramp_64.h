/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ARCH_X86_64_TRAMP_64_H__
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ARCH_X86_64_TRAMP_64_H__
/*
 * text addresses, not word aligned; these are .globl symbols from the
 * assembler source, and there is no type information associated with
 * them.  we could declare these to be (void (*)(void)), i suppose,
 * but it doesn't really matter since we convert their addresses to
 * uintptr_t at every use.
 */
extern char NaCl_trampoline_code;
extern char NaCl_trampoline_code_end;
extern char NaCl_trampoline_tramp_addr;
extern char NaCl_trampoline_call_target;

extern char NaClDispatchThunk;
extern char NaClDispatchThunk_jmp_target;
extern char NaClDispatchThunkEnd;

extern char NaClGetTlsFastPath1;
extern char NaClGetTlsFastPath1RspRestored;
extern char NaClGetTlsFastPath1End;
extern char NaClGetTlsFastPath2;
extern char NaClGetTlsFastPath2RspRestored;
extern char NaClGetTlsFastPath2End;

#endif
