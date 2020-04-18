/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file defines _Unwind_RaiseException() etc. using the SJLJ
 * (setjmp()/longjmp()-based) versions provided by PNaCl's libstdc++.
 *
 * This allows a single build of libstdc++ to support both the SJLJ
 * and non-SJLJ models of C++ exception handling.  The SJLJ version is
 * enabled by linking in this redirection file.
 */

#include <stdlib.h>


extern "C" {

struct _Unwind_Exception;

int __pnacl_eh_sjlj_Unwind_RaiseException(struct _Unwind_Exception *exc);
int __pnacl_eh_sjlj_Unwind_Resume_or_Rethrow(struct _Unwind_Exception *exc);
void __pnacl_eh_sjlj_Unwind_DeleteException(struct _Unwind_Exception *exc);
void __pnacl_eh_sjlj_cxa_call_unexpected(struct _Unwind_Exception *exc);


int _Unwind_RaiseException(struct _Unwind_Exception *exc) {
  return __pnacl_eh_sjlj_Unwind_RaiseException(exc);
}

int _Unwind_Resume_or_Rethrow(struct _Unwind_Exception *exc) {
  return __pnacl_eh_sjlj_Unwind_Resume_or_Rethrow(exc);
}

void _Unwind_DeleteException(struct _Unwind_Exception *exc) {
  __pnacl_eh_sjlj_Unwind_DeleteException(exc);
}

void __cxa_call_unexpected(struct _Unwind_Exception *exc) {
  __pnacl_eh_sjlj_cxa_call_unexpected(exc);
}

/*
 * This is a dummy version of the C++ personality function to prevent
 * libstdc++'s eh_personality.cc from being linked in, since that file
 * also defines __cxa_call_unexpected(), which would clash with our
 * definition above.
 *
 * The only references to this personality function will be from LLVM
 * "landingpad" instructions, which will be removed by the PNaClSjLjEH
 * pass.
 */
void __gxx_personality_v0() {
  abort();
}

}
