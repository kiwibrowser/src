// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/base/allocator/partition_allocator/page_allocator.h"

#include <limits.h>

#include <atomic>

#include "build/build_config.h"
#include "third_party/base/allocator/partition_allocator/address_space_randomization.h"
#include "third_party/base/base_export.h"
#include "third_party/base/logging.h"

#if defined(OS_POSIX)

#include <errno.h>
#include <sys/mman.h>

#ifndef MADV_FREE
#define MADV_FREE MADV_DONTNEED
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

// On POSIX |mmap| uses a nearby address if the hint address is blocked.
static const bool kHintIsAdvisory = true;
static std::atomic<int32_t> s_allocPageErrorCode{0};

#elif defined(OS_WIN)

#include <windows.h>

// |VirtualAlloc| will fail if allocation at the hint address is blocked.
static const bool kHintIsAdvisory = false;
static std::atomic<int32_t> s_allocPageErrorCode{ERROR_SUCCESS};

#else
#error Unknown OS
#endif  // defined(OS_POSIX)

namespace pdfium {
namespace base {

// This internal function wraps the OS-specific page allocation call:
// |VirtualAlloc| on Windows, and |mmap| on POSIX.
static void* SystemAllocPages(
    void* hint,
    size_t length,
    PageAccessibilityConfiguration page_accessibility) {
  DCHECK(!(length & kPageAllocationGranularityOffsetMask));
  DCHECK(!(reinterpret_cast<uintptr_t>(hint) &
           kPageAllocationGranularityOffsetMask));
  void* ret;
#if defined(OS_WIN)
  DWORD access_flag =
      page_accessibility == PageAccessible ? PAGE_READWRITE : PAGE_NOACCESS;
  ret = VirtualAlloc(hint, length, MEM_RESERVE | MEM_COMMIT, access_flag);
  if (!ret)
    s_allocPageErrorCode = GetLastError();
#else
  int access_flag = page_accessibility == PageAccessible
                        ? (PROT_READ | PROT_WRITE)
                        : PROT_NONE;
  ret = mmap(hint, length, access_flag, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (ret == MAP_FAILED) {
    s_allocPageErrorCode = errno;
    ret = 0;
  }
#endif
  return ret;
}

// Trims base to given length and alignment. Windows returns null on failure and
// frees base.
static void* TrimMapping(void* base,
                         size_t base_length,
                         size_t trim_length,
                         uintptr_t align,
                         PageAccessibilityConfiguration page_accessibility) {
  size_t pre_slack = reinterpret_cast<uintptr_t>(base) & (align - 1);
  if (pre_slack)
    pre_slack = align - pre_slack;
  size_t post_slack = base_length - pre_slack - trim_length;
  DCHECK(base_length >= trim_length || pre_slack || post_slack);
  DCHECK(pre_slack < base_length);
  DCHECK(post_slack < base_length);
  void* ret = base;

#if defined(OS_POSIX)  // On POSIX we can resize the allocation run.
  (void)page_accessibility;
  if (pre_slack) {
    int res = munmap(base, pre_slack);
    CHECK(!res);
    ret = reinterpret_cast<char*>(base) + pre_slack;
  }
  if (post_slack) {
    int res = munmap(reinterpret_cast<char*>(ret) + trim_length, post_slack);
    CHECK(!res);
  }
#else  // On Windows we can't resize the allocation run.
  if (pre_slack || post_slack) {
    ret = reinterpret_cast<char*>(base) + pre_slack;
    FreePages(base, base_length);
    ret = SystemAllocPages(ret, trim_length, page_accessibility);
  }
#endif

  return ret;
}

void* AllocPages(void* address,
                 size_t length,
                 size_t align,
                 PageAccessibilityConfiguration page_accessibility) {
  DCHECK(length >= kPageAllocationGranularity);
  DCHECK(!(length & kPageAllocationGranularityOffsetMask));
  DCHECK(align >= kPageAllocationGranularity);
  DCHECK(!(align & kPageAllocationGranularityOffsetMask));
  DCHECK(!(reinterpret_cast<uintptr_t>(address) &
           kPageAllocationGranularityOffsetMask));
  uintptr_t align_offset_mask = align - 1;
  uintptr_t align_base_mask = ~align_offset_mask;
  DCHECK(!(reinterpret_cast<uintptr_t>(address) & align_offset_mask));

  // If the client passed null as the address, choose a good one.
  if (!address) {
    address = GetRandomPageBase();
    address = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(address) &
                                      align_base_mask);
  }

  // First try to force an exact-size, aligned allocation from our random base.
  for (int count = 0; count < 3; ++count) {
    void* ret = SystemAllocPages(address, length, page_accessibility);
    if (kHintIsAdvisory || ret) {
      // If the alignment is to our liking, we're done.
      if (!(reinterpret_cast<uintptr_t>(ret) & align_offset_mask))
        return ret;
      FreePages(ret, length);
#if defined(ARCH_CPU_32_BITS)
      address = reinterpret_cast<void*>(
          (reinterpret_cast<uintptr_t>(ret) + align) & align_base_mask);
#endif
    } else if (!address) {  // We know we're OOM when an unhinted allocation
                            // fails.
      return nullptr;
    } else {
#if defined(ARCH_CPU_32_BITS)
      address = reinterpret_cast<char*>(address) + align;
#endif
    }

#if !defined(ARCH_CPU_32_BITS)
    // Keep trying random addresses on systems that have a large address space.
    address = GetRandomPageBase();
    address = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(address) &
                                      align_base_mask);
#endif
  }

  // Map a larger allocation so we can force alignment, but continue randomizing
  // only on 64-bit POSIX.
  size_t try_length = length + (align - kPageAllocationGranularity);
  CHECK(try_length >= length);
  void* ret;

  do {
    // Don't continue to burn cycles on mandatory hints (Windows).
    address = kHintIsAdvisory ? GetRandomPageBase() : nullptr;
    ret = SystemAllocPages(address, try_length, page_accessibility);
    // The retries are for Windows, where a race can steal our mapping on
    // resize.
  } while (ret &&
           (ret = TrimMapping(ret, try_length, length, align,
                              page_accessibility)) == nullptr);

  return ret;
}

void FreePages(void* address, size_t length) {
  DCHECK(!(reinterpret_cast<uintptr_t>(address) &
           kPageAllocationGranularityOffsetMask));
  DCHECK(!(length & kPageAllocationGranularityOffsetMask));
#if defined(OS_POSIX)
  int ret = munmap(address, length);
  CHECK(!ret);
#else
  BOOL ret = VirtualFree(address, 0, MEM_RELEASE);
  CHECK(ret);
#endif
}

void SetSystemPagesInaccessible(void* address, size_t length) {
  DCHECK(!(length & kSystemPageOffsetMask));
#if defined(OS_POSIX)
  int ret = mprotect(address, length, PROT_NONE);
  CHECK(!ret);
#else
  BOOL ret = VirtualFree(address, length, MEM_DECOMMIT);
  CHECK(ret);
#endif
}

bool SetSystemPagesAccessible(void* address, size_t length) {
  DCHECK(!(length & kSystemPageOffsetMask));
#if defined(OS_POSIX)
  return !mprotect(address, length, PROT_READ | PROT_WRITE);
#else
  return !!VirtualAlloc(address, length, MEM_COMMIT, PAGE_READWRITE);
#endif
}

void DecommitSystemPages(void* address, size_t length) {
  DCHECK(!(length & kSystemPageOffsetMask));
#if defined(OS_POSIX)
  int ret = madvise(address, length, MADV_FREE);
  if (ret != 0 && errno == EINVAL) {
    // MADV_FREE only works on Linux 4.5+ . If request failed,
    // retry with older MADV_DONTNEED . Note that MADV_FREE
    // being defined at compile time doesn't imply runtime support.
    ret = madvise(address, length, MADV_DONTNEED);
  }
  CHECK(!ret);
#else
  SetSystemPagesInaccessible(address, length);
#endif
}

void RecommitSystemPages(void* address, size_t length) {
  DCHECK(!(length & kSystemPageOffsetMask));
#if defined(OS_POSIX)
  (void)address;
#else
  CHECK(SetSystemPagesAccessible(address, length));
#endif
}

void DiscardSystemPages(void* address, size_t length) {
  DCHECK(!(length & kSystemPageOffsetMask));
#if defined(OS_POSIX)
  // On POSIX, the implementation detail is that discard and decommit are the
  // same, and lead to pages that are returned to the system immediately and
  // get replaced with zeroed pages when touched. So we just call
  // DecommitSystemPages() here to avoid code duplication.
  DecommitSystemPages(address, length);
#else
  // On Windows discarded pages are not returned to the system immediately and
  // not guaranteed to be zeroed when returned to the application.
  using DiscardVirtualMemoryFunction =
      DWORD(WINAPI*)(PVOID virtualAddress, SIZE_T size);
  static DiscardVirtualMemoryFunction discard_virtual_memory =
      reinterpret_cast<DiscardVirtualMemoryFunction>(-1);
  if (discard_virtual_memory ==
      reinterpret_cast<DiscardVirtualMemoryFunction>(-1))
    discard_virtual_memory =
        reinterpret_cast<DiscardVirtualMemoryFunction>(GetProcAddress(
            GetModuleHandle(L"Kernel32.dll"), "DiscardVirtualMemory"));
  // Use DiscardVirtualMemory when available because it releases faster than
  // MEM_RESET.
  DWORD ret = 1;
  if (discard_virtual_memory)
    ret = discard_virtual_memory(address, length);
  // DiscardVirtualMemory is buggy in Win10 SP0, so fall back to MEM_RESET on
  // failure.
  if (ret) {
    void* ret = VirtualAlloc(address, length, MEM_RESET, PAGE_READWRITE);
    CHECK(ret);
  }
#endif
}

uint32_t GetAllocPageErrorCode() {
  return s_allocPageErrorCode;
}

}  // namespace base
}  // namespace pdfium
