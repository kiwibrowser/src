// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_FX_MEMORY_H_
#define CORE_FXCRT_FX_MEMORY_H_

#include "core/fxcrt/fx_system.h"

#ifdef __cplusplus
extern "C" {
#endif

// For external C libraries to malloc through PDFium. These may return nullptr.
void* FXMEM_DefaultAlloc(size_t byte_size);
void* FXMEM_DefaultCalloc(size_t num_elems, size_t byte_size);
void* FXMEM_DefaultRealloc(void* pointer, size_t new_size);
void FXMEM_DefaultFree(void* pointer);

#ifdef __cplusplus
}  // extern "C"

#include <stdlib.h>
#include <limits>
#include <memory>
#include <new>

#include "core/fxcrt/fx_safe_types.h"
#include "third_party/base/allocator/partition_allocator/partition_alloc.h"

extern pdfium::base::PartitionAllocatorGeneric gArrayBufferPartitionAllocator;
extern pdfium::base::PartitionAllocatorGeneric gGeneralPartitionAllocator;
extern pdfium::base::PartitionAllocatorGeneric gStringPartitionAllocator;

void FXMEM_InitializePartitionAlloc();
NEVER_INLINE void FX_OutOfMemoryTerminate();

inline void* FX_SafeAlloc(size_t num_members, size_t member_size) {
  FX_SAFE_SIZE_T total = member_size;
  total *= num_members;
  if (!total.IsValid())
    return nullptr;

  void* result = pdfium::base::PartitionAllocGenericFlags(
      gGeneralPartitionAllocator.root(), pdfium::base::PartitionAllocReturnNull,
      total.ValueOrDie(), "GeneralPartition");
  if (result)
    memset(result, 0, total.ValueOrDie());
  return result;
}

inline void* FX_SafeRealloc(void* ptr, size_t num_members, size_t member_size) {
  FX_SAFE_SIZE_T size = num_members;
  size *= member_size;
  if (!size.IsValid())
    return nullptr;

  return pdfium::base::PartitionReallocGenericFlags(
      gGeneralPartitionAllocator.root(), pdfium::base::PartitionAllocReturnNull,
      ptr, size.ValueOrDie(), "GeneralPartition");
}

inline void* FX_AllocOrDie(size_t num_members, size_t member_size) {
  // TODO(tsepez): See if we can avoid the implicit memset(0).
  void* result = FX_SafeAlloc(num_members, member_size);
  if (!result)
    FX_OutOfMemoryTerminate();  // Never returns.

  return result;
}

inline void* FX_AllocOrDie2D(size_t w, size_t h, size_t member_size) {
  if (w >= std::numeric_limits<size_t>::max() / h)
    FX_OutOfMemoryTerminate();  // Never returns.

  return FX_AllocOrDie(w * h, member_size);
}

inline void* FX_ReallocOrDie(void* ptr,
                             size_t num_members,
                             size_t member_size) {
  void* result = FX_SafeRealloc(ptr, num_members, member_size);
  if (!result)
    FX_OutOfMemoryTerminate();  // Never returns.

  return result;
}

// These never return nullptr.
#define FX_Alloc(type, size) \
  static_cast<type*>(FX_AllocOrDie(size, sizeof(type)))
#define FX_Alloc2D(type, w, h) \
  static_cast<type*>(FX_AllocOrDie2D(w, h, sizeof(type)))
#define FX_Realloc(type, ptr, size) \
  static_cast<type*>(FX_ReallocOrDie(ptr, size, sizeof(type)))

// May return nullptr.
#define FX_TryAlloc(type, size) \
  static_cast<type*>(FX_SafeAlloc(size, sizeof(type)))
#define FX_TryRealloc(type, ptr, size) \
  static_cast<type*>(FX_SafeRealloc(ptr, size, sizeof(type)))

inline void FX_Free(void* ptr) {
  // TODO(palmer): Removing this check exposes crashes when PDFium callers
  // attempt to free |nullptr|. Although libc's |free| allows freeing |NULL|, no
  // other Partition Alloc callers need this tolerant behavior. Additionally,
  // checking for |nullptr| adds a branch to |PartitionFree|, and it's nice to
  // not have to have that.
  //
  // So this check is hiding (what I consider to be) bugs, and we should try to
  // fix them. https://bugs.chromium.org/p/pdfium/issues/detail?id=690
  if (ptr)
    pdfium::base::PartitionFree(ptr);
}

// The FX_ArraySize(arr) macro returns the # of elements in an array arr.
// The expression is a compile-time constant, and therefore can be
// used in defining new arrays, for example.  If you use FX_ArraySize on
// a pointer by mistake, you will get a compile-time error.
//
// One caveat is that FX_ArraySize() doesn't accept any array of an
// anonymous type or a type defined inside a function.
#define FX_ArraySize(array) (sizeof(ArraySizeHelper(array)))

// This template function declaration is used in defining FX_ArraySize.
// Note that the function doesn't need an implementation, as we only
// use its type.
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

// Used with std::unique_ptr to FX_Free raw memory.
struct FxFreeDeleter {
  inline void operator()(void* ptr) const { FX_Free(ptr); }
};

#endif  // __cplusplus

#endif  // CORE_FXCRT_FX_MEMORY_H_
