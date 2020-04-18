/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


extern "C" void try_block_test();

class SomeException {
};

class ObjWithDtor {
 public:
  ~ObjWithDtor() {
    printf("[in ~ObjWithDtor()]\n");
  }
  void no_op() {
  }
};

// Stores x86-64's callee-saved registers (excluding r15).
const int kRegsCount = 4;
struct RegsState {
  uint64_t regs[kRegsCount];
};

struct RegsState g_expected_regs;
struct RegsState g_actual_regs;

jmp_buf g_jmp_buf;
int g_test_unwind_resume = 0;
int g_failed = 0;

extern "C" void throw_some_exception() {
  if (g_test_unwind_resume) {
    ObjWithDtor obj;
    // Prevent warning about variable not being used.
    obj.no_op();
    // This "throw" indirectly calls _Unwind_RaiseException(), which
    // passes control back to throw_some_exception(), which will:
    //  * run the destructor for ObjWithDtor;
    //  * call _Unwind_Resume(), which passes control back to
    //    throw_some_exception()'s callee.
    throw SomeException();
  } else {
    // This "throw" indirectly calls _Unwind_RaiseException(), which
    // will pass control back to throw_some_exception()'s callee.
    throw SomeException();
  }
}

extern "C" void check_register_state() {
  for (int i = 0; i < kRegsCount; i++) {
    printf("register value: expected=0x%llx, actual=0x%llx\n",
           (long long) g_expected_regs.regs[i],
           (long long) g_actual_regs.regs[i]);
    if (g_expected_regs.regs[i] != g_actual_regs.regs[i]) {
      g_failed = 1;
    }
  }
  longjmp(g_jmp_buf, 1);
}

void run_test() {
  try {
    if (!setjmp(g_jmp_buf))
      try_block_test();
  } catch (SomeException &) {
    // This catch block should never be executed: the test longjmp()s
    // past it.  Without this catch block, the unwinder will not see
    // any handlers for the exception and will abort before running
    // the assembly-code cleanup handler that calls
    // check_register_state().
    abort();
  }
}

int main() {
  printf("Testing restoring registers via _Unwind_RaiseException...\n");
  run_test();

  printf("Testing restoring registers via _Unwind_Resume...\n");
  g_test_unwind_resume = 1;
  run_test();

  return g_failed;
}
