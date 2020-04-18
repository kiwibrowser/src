// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ALLOCATOR_PARTITION_ALLOCATOR_PAGE_ALLOCATOR_H
#define BASE_ALLOCATOR_PARTITION_ALLOCATOR_PAGE_ALLOCATOR_H

#include <stdint.h>

#include <cstddef>

#include "build/build_config.h"
#include "third_party/base/base_export.h"
#include "third_party/base/compiler_specific.h"

namespace pdfium {
namespace base {

#if defined(OS_WIN)
static const size_t kPageAllocationGranularityShift = 16;  // 64KB
#elif defined(_MIPS_ARCH_LOONGSON)
static const size_t kPageAllocationGranularityShift = 14;  // 16KB
#else
static const size_t kPageAllocationGranularityShift = 12;  // 4KB
#endif
static const size_t kPageAllocationGranularity =
    1 << kPageAllocationGranularityShift;
static const size_t kPageAllocationGranularityOffsetMask =
    kPageAllocationGranularity - 1;
static const size_t kPageAllocationGranularityBaseMask =
    ~kPageAllocationGranularityOffsetMask;

// All Blink-supported systems have 4096 sized system pages and can handle
// permissions and commit / decommit at this granularity.
// Loongson have 16384 sized system pages.
#if defined(_MIPS_ARCH_LOONGSON)
static const size_t kSystemPageSize = 16384;
#else
static const size_t kSystemPageSize = 4096;
#endif
static const size_t kSystemPageOffsetMask = kSystemPageSize - 1;
static const size_t kSystemPageBaseMask = ~kSystemPageOffsetMask;

enum PageAccessibilityConfiguration {
  PageAccessible,
  PageInaccessible,
};

// Allocate one or more pages.
// The requested address is just a hint; the actual address returned may
// differ. The returned address will be aligned at least to align bytes.
// len is in bytes, and must be a multiple of kPageAllocationGranularity.
// align is in bytes, and must be a power-of-two multiple of
// kPageAllocationGranularity.
// If addr is null, then a suitable and randomized address will be chosen
// automatically.
// PageAccessibilityConfiguration controls the permission of the
// allocated pages.
// This call will return null if the allocation cannot be satisfied.
BASE_EXPORT void* AllocPages(void* address,
                             size_t len,
                             size_t align,
                             PageAccessibilityConfiguration);

// Free one or more pages.
// addr and len must match a previous call to allocPages().
BASE_EXPORT void FreePages(void* address, size_t length);

// Mark one or more system pages as being inaccessible.
// Subsequently accessing any address in the range will fault, and the
// addresses will not be re-used by future allocations.
// len must be a multiple of kSystemPageSize bytes.
BASE_EXPORT void SetSystemPagesInaccessible(void* address, size_t length);

// Mark one or more system pages as being accessible.
// The pages will be readable and writeable.
// len must be a multiple of kSystemPageSize bytes.
// The result bool value indicates whether the permission
// change succeeded or not. You must check the result
// (in most cases you need to CHECK that it is true).
BASE_EXPORT WARN_UNUSED_RESULT bool SetSystemPagesAccessible(void* address,
                                                             size_t length);

// Decommit one or more system pages. Decommitted means that the physical memory
// is released to the system, but the virtual address space remains reserved.
// System pages are re-committed by calling recommitSystemPages(). Touching
// a decommitted page _may_ fault.
// Clients should not make any assumptions about the contents of decommitted
// system pages, before or after they write to the page. The only guarantee
// provided is that the contents of the system page will be deterministic again
// after recommitting and writing to it. In particlar note that system pages are
// not guaranteed to be zero-filled upon re-commit. len must be a multiple of
// kSystemPageSize bytes.
BASE_EXPORT void DecommitSystemPages(void* address, size_t length);

// Recommit one or more system pages. Decommitted system pages must be
// recommitted before they are read are written again.
// Note that this operation may be a no-op on some platforms.
// len must be a multiple of kSystemPageSize bytes.
BASE_EXPORT void RecommitSystemPages(void* address, size_t length);

// Discard one or more system pages. Discarding is a hint to the system that
// the page is no longer required. The hint may:
// - Do nothing.
// - Discard the page immediately, freeing up physical pages.
// - Discard the page at some time in the future in response to memory pressure.
// Only committed pages should be discarded. Discarding a page does not
// decommit it, and it is valid to discard an already-discarded page.
// A read or write to a discarded page will not fault.
// Reading from a discarded page may return the original page content, or a
// page full of zeroes.
// Writing to a discarded page is the only guaranteed way to tell the system
// that the page is required again. Once written to, the content of the page is
// guaranteed stable once more. After being written to, the page content may be
// based on the original page content, or a page of zeroes.
// len must be a multiple of kSystemPageSize bytes.
BASE_EXPORT void DiscardSystemPages(void* address, size_t length);

ALWAYS_INLINE uintptr_t RoundUpToSystemPage(uintptr_t address) {
  return (address + kSystemPageOffsetMask) & kSystemPageBaseMask;
}

ALWAYS_INLINE uintptr_t RoundDownToSystemPage(uintptr_t address) {
  return address & kSystemPageBaseMask;
}

// Returns errno (or GetLastError code) when mmap (or VirtualAlloc) fails.
BASE_EXPORT uint32_t GetAllocPageErrorCode();

}  // namespace base
}  // namespace pdfium

#endif  // BASE_ALLOCATOR_PARTITION_ALLOCATOR_PAGE_ALLOCATOR_H
