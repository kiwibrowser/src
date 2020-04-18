// Copyright (c) 2011 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These tests exercise NaCl's sysbrk() system call.

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "native_client/src/trusted/service_runtime/include/sys/nacl_syscalls.h"
#include "native_client/tests/syscalls/test.h"

/*
 * This is defined by the linker as the address of the end of our data segment.
 * That's where the break starts out by default.
 */
extern "C" {
  extern char end;
}

namespace {
// Note: these parameters to sysbrk are not supposed to be const.

// The defined error return address.
void* kSysbrkErrorAddress = reinterpret_cast<void*>(-1);

// This is an address outside of the 1Gb address range allowed for NaCl
// modules.
void* kIllegalBreakAddress = reinterpret_cast<void*>(~0U);

// Make sure the current break address is non-0 when using sysbrk().
int TestCurrentBreakAddr() {
  START_TEST("TestCurrentBreakAddr");

  // Clear errno incase a previous function set it.
  errno = 0;

  void* break_addr = sysbrk(NULL);
  EXPECT(NULL != break_addr);
  EXPECT(kSysbrkErrorAddress != break_addr);
  EXPECT(0 == errno);
  END_TEST();
}


// Try to reset the program's break address to a legitimate value.
int TestSysbrk() {
  // Round up to the end of the page that's our last initial data page.
  // Then add 10MB for good measure to be out of the way of any allocations
  // that might have been done before we got here.
  void* const sysbrkBase = reinterpret_cast<void*>
    (((reinterpret_cast<uintptr_t>(&end) + 0xffff) & -0x10000) + (10 << 20));

  START_TEST("TestSysbrk");

  // Clear errno incase a previous function set it.
  errno = 0;

  void* break_addr = sysbrk(sysbrkBase);
  EXPECT(NULL != break_addr);
  EXPECT(kSysbrkErrorAddress != break_addr);
  EXPECT(sysbrkBase == break_addr);
  EXPECT(0 == errno);
  END_TEST();
}


// Try to reset the program's break address to something illegal using sysbrk().
// When sysbrk() fails, it is supposed to return the old break address and set
// |errno| "to an appropriate value" (in this case, EINVAL).
int TestIllegalSysbrk() {
  START_TEST("TestIllegalSysbrk");

  // Clear errno incase a previous function set it.
  errno = 0;

  void* current_break = sysbrk(NULL);
  void* break_addr = sysbrk(kIllegalBreakAddress);
  /* sysbrk does not touch errno, only the sbrk wrapper would */
  EXPECT(0 == errno);
  EXPECT(NULL != break_addr);
  EXPECT(current_break == break_addr);
  END_TEST();
}
}  // namespace

// Run through the complete sequence of sysbrk tests.  Sets the exit code to
// the number of failed tests.  Exit code 0 means all passed.
int main() {
  int fail_count = 0;
  fail_count += TestCurrentBreakAddr();
  fail_count += TestSysbrk();
  fail_count += TestIllegalSysbrk();
  return fail_count;
}
