/*
 * Copyright (C) 2007, 2008, 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_ATOMICS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_ATOMICS_H_

#include <stdint.h>
#include "build/build_config.h"
#include "third_party/blink/renderer/platform/wtf/address_sanitizer.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/cpu.h"

#if defined(COMPILER_MSVC)
#include <windows.h>
#endif

#if defined(THREAD_SANITIZER)
#include <sanitizer/tsan_interface_atomic.h>
#endif

#if defined(ADDRESS_SANITIZER)
#include <sanitizer/asan_interface.h>
#endif

namespace WTF {

#if defined(COMPILER_MSVC)

// atomicAdd returns the result of the addition.
ALWAYS_INLINE int AtomicAdd(int volatile* addend, int increment) {
  return InterlockedExchangeAdd(reinterpret_cast<long volatile*>(addend),
                                static_cast<long>(increment)) +
         increment;
}
ALWAYS_INLINE unsigned AtomicAdd(unsigned volatile* addend,
                                 unsigned increment) {
  return InterlockedExchangeAdd(reinterpret_cast<long volatile*>(addend),
                                static_cast<long>(increment)) +
         increment;
}
#if defined(_WIN64)
ALWAYS_INLINE unsigned long long AtomicAdd(unsigned long long volatile* addend,
                                           unsigned long long increment) {
  return InterlockedExchangeAdd64(reinterpret_cast<long long volatile*>(addend),
                                  static_cast<long long>(increment)) +
         increment;
}
#endif

// atomicSubtract returns the result of the subtraction.
ALWAYS_INLINE int AtomicSubtract(int volatile* addend, int decrement) {
  return InterlockedExchangeAdd(reinterpret_cast<long volatile*>(addend),
                                static_cast<long>(-decrement)) -
         decrement;
}
ALWAYS_INLINE unsigned AtomicSubtract(unsigned volatile* addend,
                                      unsigned decrement) {
  return InterlockedExchangeAdd(reinterpret_cast<long volatile*>(addend),
                                -static_cast<long>(decrement)) -
         decrement;
}
#if defined(_WIN64)
ALWAYS_INLINE unsigned long long AtomicSubtract(
    unsigned long long volatile* addend,
    unsigned long long decrement) {
  return InterlockedExchangeAdd64(reinterpret_cast<long long volatile*>(addend),
                                  -static_cast<long long>(decrement)) -
         decrement;
}
#endif

ALWAYS_INLINE int AtomicIncrement(int volatile* addend) {
  return InterlockedIncrement(reinterpret_cast<long volatile*>(addend));
}
ALWAYS_INLINE int AtomicDecrement(int volatile* addend) {
  return InterlockedDecrement(reinterpret_cast<long volatile*>(addend));
}

ALWAYS_INLINE int64_t AtomicIncrement(int64_t volatile* addend) {
  return InterlockedIncrement64(reinterpret_cast<long long volatile*>(addend));
}
ALWAYS_INLINE int64_t AtomicDecrement(int64_t volatile* addend) {
  return InterlockedDecrement64(reinterpret_cast<long long volatile*>(addend));
}

ALWAYS_INLINE int AtomicTestAndSetToOne(int volatile* ptr) {
  int ret = InterlockedExchange(reinterpret_cast<long volatile*>(ptr), 1);
  DCHECK(!ret || ret == 1);
  return ret;
}

ALWAYS_INLINE void AtomicSetOneToZero(int volatile* ptr) {
  DCHECK_EQ(*ptr, 1);
  InterlockedExchange(reinterpret_cast<long volatile*>(ptr), 0);
}

#else

// atomicAdd returns the result of the addition.
ALWAYS_INLINE int AtomicAdd(int volatile* addend, int increment) {
  return __sync_add_and_fetch(addend, increment);
}
ALWAYS_INLINE unsigned AtomicAdd(unsigned volatile* addend,
                                 unsigned increment) {
  return __sync_add_and_fetch(addend, increment);
}
ALWAYS_INLINE unsigned long AtomicAdd(unsigned long volatile* addend,
                                      unsigned long increment) {
  return __sync_add_and_fetch(addend, increment);
}
// atomicSubtract returns the result of the subtraction.
ALWAYS_INLINE int AtomicSubtract(int volatile* addend, int decrement) {
  return __sync_sub_and_fetch(addend, decrement);
}
ALWAYS_INLINE unsigned AtomicSubtract(unsigned volatile* addend,
                                      unsigned decrement) {
  return __sync_sub_and_fetch(addend, decrement);
}
ALWAYS_INLINE unsigned long AtomicSubtract(unsigned long volatile* addend,
                                           unsigned long decrement) {
  return __sync_sub_and_fetch(addend, decrement);
}

ALWAYS_INLINE int AtomicIncrement(int volatile* addend) {
  return AtomicAdd(addend, 1);
}
ALWAYS_INLINE int AtomicDecrement(int volatile* addend) {
  return AtomicSubtract(addend, 1);
}

ALWAYS_INLINE int64_t AtomicIncrement(int64_t volatile* addend) {
  return __sync_add_and_fetch(addend, 1);
}
ALWAYS_INLINE int64_t AtomicDecrement(int64_t volatile* addend) {
  return __sync_sub_and_fetch(addend, 1);
}

ALWAYS_INLINE int AtomicTestAndSetToOne(int volatile* ptr) {
  int ret = __sync_lock_test_and_set(ptr, 1);
  DCHECK(!ret || ret == 1);
  return ret;
}

ALWAYS_INLINE void AtomicSetOneToZero(int volatile* ptr) {
  DCHECK_EQ(*ptr, 1);
  __sync_lock_release(ptr);
}
#endif

#if defined(THREAD_SANITIZER)
// The definitions below assume an LP64 data model. This is fine because
// TSan is only supported on x86_64 Linux.
#if defined(ARCH_CPU_64_BITS) && defined(OS_LINUX)
ALWAYS_INLINE void ReleaseStore(volatile int* ptr, int value) {
  __tsan_atomic32_store(ptr, value, __tsan_memory_order_release);
}
ALWAYS_INLINE void ReleaseStore(volatile unsigned* ptr, unsigned value) {
  __tsan_atomic32_store(reinterpret_cast<volatile int*>(ptr),
                        static_cast<int>(value), __tsan_memory_order_release);
}
ALWAYS_INLINE void ReleaseStore(volatile long* ptr, long value) {
  __tsan_atomic64_store(reinterpret_cast<volatile __tsan_atomic64*>(ptr),
                        static_cast<__tsan_atomic64>(value),
                        __tsan_memory_order_release);
}
ALWAYS_INLINE void ReleaseStore(volatile unsigned long* ptr,
                                unsigned long value) {
  __tsan_atomic64_store(reinterpret_cast<volatile __tsan_atomic64*>(ptr),
                        static_cast<__tsan_atomic64>(value),
                        __tsan_memory_order_release);
}
ALWAYS_INLINE void ReleaseStore(volatile unsigned long long* ptr,
                                unsigned long long value) {
  __tsan_atomic64_store(reinterpret_cast<volatile __tsan_atomic64*>(ptr),
                        static_cast<__tsan_atomic64>(value),
                        __tsan_memory_order_release);
}
ALWAYS_INLINE void ReleaseStore(void* volatile* ptr, void* value) {
  __tsan_atomic64_store(reinterpret_cast<volatile __tsan_atomic64*>(ptr),
                        reinterpret_cast<__tsan_atomic64>(value),
                        __tsan_memory_order_release);
}
ALWAYS_INLINE int AcquireLoad(volatile const int* ptr) {
  return __tsan_atomic32_load(ptr, __tsan_memory_order_acquire);
}
ALWAYS_INLINE unsigned AcquireLoad(volatile const unsigned* ptr) {
  return static_cast<unsigned>(__tsan_atomic32_load(
      reinterpret_cast<volatile const int*>(ptr), __tsan_memory_order_acquire));
}
ALWAYS_INLINE long AcquireLoad(volatile const long* ptr) {
  return static_cast<long>(__tsan_atomic64_load(
      reinterpret_cast<volatile const __tsan_atomic64*>(ptr),
      __tsan_memory_order_acquire));
}
ALWAYS_INLINE unsigned long AcquireLoad(volatile const unsigned long* ptr) {
  return static_cast<unsigned long>(__tsan_atomic64_load(
      reinterpret_cast<volatile const __tsan_atomic64*>(ptr),
      __tsan_memory_order_acquire));
}
ALWAYS_INLINE void* AcquireLoad(void* volatile const* ptr) {
  return reinterpret_cast<void*>(__tsan_atomic64_load(
      reinterpret_cast<volatile const __tsan_atomic64*>(ptr),
      __tsan_memory_order_acquire));
}

// Do not use NoBarrierStore/NoBarrierLoad for synchronization.
ALWAYS_INLINE void NoBarrierStore(volatile float* ptr, float value) {
  static_assert(sizeof(int) == sizeof(float),
                "int and float are different sizes");
  union {
    int ivalue;
    float fvalue;
  } u;
  u.fvalue = value;
  __tsan_atomic32_store(reinterpret_cast<volatile __tsan_atomic32*>(ptr),
                        u.ivalue, __tsan_memory_order_relaxed);
}

ALWAYS_INLINE float NoBarrierLoad(volatile const float* ptr) {
  static_assert(sizeof(int) == sizeof(float),
                "int and float are different sizes");
  union {
    int ivalue;
    float fvalue;
  } u;
  u.ivalue = __tsan_atomic32_load(reinterpret_cast<volatile const int*>(ptr),
                                  __tsan_memory_order_relaxed);
  return u.fvalue;
}
#endif

#else  // defined(THREAD_SANITIZER)

#if defined(ARCH_CPU_X86_FAMILY)
// Only compiler barrier is needed.
#if defined(COMPILER_MSVC)
// Starting from Visual Studio 2005 compiler guarantees acquire and release
// semantics for operations on volatile variables. See MSDN entry for
// MemoryBarrier macro.
#define MEMORY_BARRIER()
#else
#define MEMORY_BARRIER() __asm__ __volatile__("" : : : "memory")
#endif
#else
// Fallback to the compiler intrinsic on all other platforms.
#define MEMORY_BARRIER() __sync_synchronize()
#endif

ALWAYS_INLINE void ReleaseStore(volatile int* ptr, int value) {
  MEMORY_BARRIER();
  *ptr = value;
}
ALWAYS_INLINE void ReleaseStore(volatile unsigned* ptr, unsigned value) {
  MEMORY_BARRIER();
  *ptr = value;
}
ALWAYS_INLINE void ReleaseStore(volatile long* ptr, long value) {
  MEMORY_BARRIER();
  *ptr = value;
}
ALWAYS_INLINE void ReleaseStore(volatile unsigned long* ptr,
                                unsigned long value) {
  MEMORY_BARRIER();
  *ptr = value;
}
#if defined(ARCH_CPU_64_BITS)
ALWAYS_INLINE void ReleaseStore(volatile unsigned long long* ptr,
                                unsigned long long value) {
  MEMORY_BARRIER();
  *ptr = value;
}
#endif
ALWAYS_INLINE void ReleaseStore(void* volatile* ptr, void* value) {
  MEMORY_BARRIER();
  *ptr = value;
}

ALWAYS_INLINE int AcquireLoad(volatile const int* ptr) {
  int value = *ptr;
  MEMORY_BARRIER();
  return value;
}
ALWAYS_INLINE unsigned AcquireLoad(volatile const unsigned* ptr) {
  unsigned value = *ptr;
  MEMORY_BARRIER();
  return value;
}
ALWAYS_INLINE long AcquireLoad(volatile const long* ptr) {
  long value = *ptr;
  MEMORY_BARRIER();
  return value;
}
ALWAYS_INLINE unsigned long AcquireLoad(volatile const unsigned long* ptr) {
  unsigned long value = *ptr;
  MEMORY_BARRIER();
  return value;
}
#if defined(ARCH_CPU_64_BITS)
ALWAYS_INLINE unsigned long long AcquireLoad(
    volatile const unsigned long long* ptr) {
  unsigned long long value = *ptr;
  MEMORY_BARRIER();
  return value;
}
#endif
ALWAYS_INLINE void* AcquireLoad(void* volatile const* ptr) {
  void* value = *ptr;
  MEMORY_BARRIER();
  return value;
}

// Do not use noBarrierStore/noBarrierLoad for synchronization.
ALWAYS_INLINE void NoBarrierStore(volatile float* ptr, float value) {
  *ptr = value;
}

ALWAYS_INLINE float NoBarrierLoad(volatile const float* ptr) {
  float value = *ptr;
  return value;
}

#if defined(ADDRESS_SANITIZER)

NO_SANITIZE_ADDRESS ALWAYS_INLINE void AsanUnsafeReleaseStore(
    volatile unsigned* ptr,
    unsigned value) {
  MEMORY_BARRIER();
  *ptr = value;
}

NO_SANITIZE_ADDRESS ALWAYS_INLINE unsigned AsanUnsafeAcquireLoad(
    volatile const unsigned* ptr) {
  unsigned value = *ptr;
  MEMORY_BARRIER();
  return value;
}

#endif  // defined(ADDRESS_SANITIZER)

#undef MEMORY_BARRIER

#endif

#if !defined(ADDRESS_SANITIZER)

ALWAYS_INLINE void AsanUnsafeReleaseStore(volatile unsigned* ptr,
                                          unsigned value) {
  ReleaseStore(ptr, value);
}

ALWAYS_INLINE unsigned AsanUnsafeAcquireLoad(volatile const unsigned* ptr) {
  return AcquireLoad(ptr);
}

#endif

}  // namespace WTF

using WTF::AtomicAdd;
using WTF::AtomicSubtract;
using WTF::AtomicDecrement;
using WTF::AtomicIncrement;
using WTF::AtomicTestAndSetToOne;
using WTF::AtomicSetOneToZero;
using WTF::AcquireLoad;
using WTF::ReleaseStore;
using WTF::NoBarrierLoad;
using WTF::NoBarrierStore;

// These methods allow loading from and storing to poisoned memory. Only
// use these methods if you know what you are doing since they will
// silence use-after-poison errors from ASan.
using WTF::AsanUnsafeAcquireLoad;
using WTF::AsanUnsafeReleaseStore;

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_ATOMICS_H_
