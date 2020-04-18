// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/base/allocator/partition_allocator/address_space_randomization.h"

#include "build/build_config.h"
#include "third_party/base/allocator/partition_allocator/page_allocator.h"
#include "third_party/base/allocator/partition_allocator/spin_lock.h"

#if defined(OS_WIN)
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

// VersionHelpers.h must be included after windows.h.
#if defined(OS_WIN)
#include <VersionHelpers.h>
#endif

namespace pdfium {
namespace base {

namespace {

// This is the same PRNG as used by tcmalloc for mapping address randomness;
// see http://burtleburtle.net/bob/rand/smallprng.html
struct ranctx {
  subtle::SpinLock lock;
  bool initialized;
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
};

#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

uint32_t ranvalInternal(ranctx* x) {
  uint32_t e = x->a - rot(x->b, 27);
  x->a = x->b ^ rot(x->c, 17);
  x->b = x->c + x->d;
  x->c = x->d + e;
  x->d = e + x->a;
  return x->d;
}

#undef rot

uint32_t ranval(ranctx* x) {
  subtle::SpinLock::Guard guard(x->lock);
  if (UNLIKELY(!x->initialized)) {
    x->initialized = true;
    char c;
    uint32_t seed = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&c));
    uint32_t pid;
    uint32_t usec;
#if defined(OS_WIN)
    pid = GetCurrentProcessId();
    SYSTEMTIME st;
    GetSystemTime(&st);
    usec = static_cast<uint32_t>(st.wMilliseconds * 1000);
#else
    pid = static_cast<uint32_t>(getpid());
    struct timeval tv;
    gettimeofday(&tv, 0);
    usec = static_cast<uint32_t>(tv.tv_usec);
#endif
    seed ^= pid;
    seed ^= usec;
    x->a = 0xf1ea5eed;
    x->b = x->c = x->d = seed;
    for (int i = 0; i < 20; ++i) {
      (void)ranvalInternal(x);
    }
  }
  uint32_t ret = ranvalInternal(x);
  return ret;
}

static struct ranctx s_ranctx;

}  // namespace

// Calculates a random preferred mapping address. In calculating an address, we
// balance good ASLR against not fragmenting the address space too badly.
void* GetRandomPageBase() {
  uintptr_t random;
  random = static_cast<uintptr_t>(ranval(&s_ranctx));
#if defined(ARCH_CPU_X86_64)
  random <<= 32UL;
  random |= static_cast<uintptr_t>(ranval(&s_ranctx));
// This address mask gives a low likelihood of address space collisions. We
// handle the situation gracefully if there is a collision.
#if defined(OS_WIN)
  random &= 0x3ffffffffffUL;
  // Windows >= 8.1 has the full 47 bits. Use them where available.
  static bool windows_81 = false;
  static bool windows_81_initialized = false;
  if (!windows_81_initialized) {
    windows_81 = IsWindows8Point1OrGreater();
    windows_81_initialized = true;
  }
  if (!windows_81) {
    random += 0x10000000000UL;
  }
#elif defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
  // This range is copied from the TSan source, but works for all tools.
  random &= 0x007fffffffffUL;
  random += 0x7e8000000000UL;
#else
  // Linux and OS X support the full 47-bit user space of x64 processors.
  random &= 0x3fffffffffffUL;
#endif
#elif defined(ARCH_CPU_ARM64)
  // ARM64 on Linux has 39-bit user space.
  random &= 0x3fffffffffUL;
  random += 0x1000000000UL;
#else  // !defined(ARCH_CPU_X86_64) && !defined(ARCH_CPU_ARM64)
#if defined(OS_WIN)
  // On win32 host systems the randomization plus huge alignment causes
  // excessive fragmentation. Plus most of these systems lack ASLR, so the
  // randomization isn't buying anything. In that case we just skip it.
  // TODO(jschuh): Just dump the randomization when HE-ASLR is present.
  static BOOL isWow64 = -1;
  if (isWow64 == -1 && !IsWow64Process(GetCurrentProcess(), &isWow64))
    isWow64 = FALSE;
  if (!isWow64)
    return nullptr;
#endif  // defined(OS_WIN)
  // This is a good range on Windows, Linux and Mac.
  // Allocates in the 0.5-1.5GB region.
  random &= 0x3fffffff;
  random += 0x20000000;
#endif  // defined(ARCH_CPU_X86_64)
  random &= kPageAllocationGranularityBaseMask;
  return reinterpret_cast<void*>(random);
}

}  // namespace base
}  // namespace pdfium
