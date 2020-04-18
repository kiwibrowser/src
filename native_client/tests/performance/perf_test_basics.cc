/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <setjmp.h>
#include <sys/mman.h>

#include "native_client/src/include/build_config.h"

#if NACL_LINUX
# include <sys/syscall.h>
# include <time.h>
#endif

#include "native_client/src/include/nacl_assert.h"
#if defined(__native_client__)
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"
#endif
#include "native_client/tests/performance/perf_test_compat_osx.h"
#include "native_client/tests/performance/perf_test_runner.h"


// This measures the overhead of the test framework and a virtual
// function call.
class TestNull : public PerfTest {
 public:
  virtual void run() {
  }
};
PERF_TEST_DECLARE(TestNull)

#if defined(__native_client__)
class TestNaClSyscall : public PerfTest {
 public:
  virtual void run() {
    NACL_SYSCALL(null)();
  }
};
PERF_TEST_DECLARE(TestNaClSyscall)
#endif

#if NACL_LINUX || NACL_OSX
class TestHostSyscall : public PerfTest {
 public:
  virtual void run() {
#if NACL_LINUX
    // Don't use getpid() here, because glibc caches the pid in userland.
    int rc = syscall(__NR_getpid);
#elif NACL_OSX
    // Mac OS X's libsyscall caches the result of getpid, but not getppid.
    int rc = getppid();
#endif
    ASSERT_GT(rc, 0);
  }
};
PERF_TEST_DECLARE(TestHostSyscall)
#endif

// Measure the speed of saving and restoring all callee-saved
// registers, assuming the compiler does not optimize the setjmp() and
// longjmp() calls away.  This is likely to be slower than TestNull
// but faster than TestNaClSyscall.
class TestSetjmpLongjmp : public PerfTest {
 public:
  virtual void run() {
    jmp_buf buf;
    if (!setjmp(buf)) {
      longjmp(buf, 1);
    }
  }
};
PERF_TEST_DECLARE(TestSetjmpLongjmp)

// Measure the overhead of the clock_gettime() call that the test
// framework uses.  This is also an example of a not-quite-trivial
// NaCl syscall which writes to untrusted address space and might do a
// host OS syscall.
class TestClockGetTime : public PerfTest {
 public:
  virtual void run() {
    struct timespec time;
    ASSERT_EQ(clock_gettime(CLOCK_MONOTONIC, &time), 0);
  }
};
PERF_TEST_DECLARE(TestClockGetTime)

#if !NACL_OSX
// We declare this as "volatile" in an attempt to prevent the compiler
// from optimizing accesses away.
__thread volatile int g_tls_var = 123;

class TestTlsVariable : public PerfTest {
 public:
  virtual void run() {
    ASSERT_EQ(g_tls_var, 123);
  }
};
PERF_TEST_DECLARE(TestTlsVariable)
#endif

class TestMmapAnonymous : public PerfTest {
 public:
  virtual void run() {
    size_t size = 0x10000;
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_ANON | MAP_PRIVATE, -1, 0);
    ASSERT_NE(addr, MAP_FAILED);
    ASSERT_EQ(munmap(addr, size), 0);
  }
};
PERF_TEST_DECLARE(TestMmapAnonymous)
