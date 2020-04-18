/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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

#include "third_party/blink/renderer/platform/wtf/threading.h"

#include "build/build_config.h"

#if defined(OS_POSIX) || defined(OS_FUCHSIA)

#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <sys/time.h>
#include "third_party/blink/renderer/platform/wtf/date_math.h"
#include "third_party/blink/renderer/platform/wtf/dtoa/double-conversion.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/thread_specific.h"
#include "third_party/blink/renderer/platform/wtf/threading_primitives.h"
#include "third_party/blink/renderer/platform/wtf/time.h"
#include "third_party/blink/renderer/platform/wtf/wtf_thread_data.h"

#if defined(OS_MACOSX)
#include <objc/objc-auto.h>
#endif

#if defined(OS_LINUX)
#include <sys/syscall.h>
#endif

#if defined(OS_LINUX) || defined(OS_ANDROID)
#include <unistd.h>
#endif

namespace WTF {

namespace internal {

ThreadIdentifier CurrentThreadSyscall() {
#if defined(OS_MACOSX)
  return pthread_mach_thread_np(pthread_self());
#elif defined(OS_LINUX)
  return syscall(__NR_gettid);
#elif defined(OS_ANDROID)
  return gettid();
#else
  return reinterpret_cast<uintptr_t>(pthread_self());
#endif
}

}  // namespace internal

void InitializeThreading() {
  // This should only be called once.
  WTFThreadData::Initialize();

  InitializeDates();
  // Force initialization of static DoubleToStringConverter converter variable
  // inside EcmaScriptConverter function while we are in single thread mode.
  double_conversion::DoubleToStringConverter::EcmaScriptConverter();
}

namespace {
pthread_key_t g_current_thread_key;
bool g_current_thread_key_initialized = false;
}  // namespace

void InitializeCurrentThread() {
  DCHECK(!g_current_thread_key_initialized);
  int error = pthread_key_create(&g_current_thread_key, nullptr);
  CHECK(!error);
  g_current_thread_key_initialized = true;
}

ThreadIdentifier CurrentThread() {
  // This doesn't use WTF::ThreadSpecific (e.g. WTFThreadData) because
  // ThreadSpecific now depends on currentThread. It is necessary to avoid this
  // or a similar loop:
  //
  // currentThread
  // -> wtfThreadData
  // -> ThreadSpecific::operator*
  // -> isMainThread
  // -> currentThread
  static_assert(sizeof(ThreadIdentifier) <= sizeof(void*),
                "ThreadIdentifier must fit in a void*.");
  DCHECK(g_current_thread_key_initialized);
  void* value = pthread_getspecific(g_current_thread_key);
  if (UNLIKELY(!value)) {
    value = reinterpret_cast<void*>(
        static_cast<intptr_t>(internal::CurrentThreadSyscall()));
    DCHECK(value);
    pthread_setspecific(g_current_thread_key, value);
  }
  return reinterpret_cast<intptr_t>(pthread_getspecific(g_current_thread_key));
}

MutexBase::MutexBase(bool recursive) {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(
      &attr, recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL);

  int result = pthread_mutex_init(&mutex_.internal_mutex_, &attr);
  DCHECK_EQ(result, 0);
#if DCHECK_IS_ON()
  mutex_.recursion_count_ = 0;
#endif

  pthread_mutexattr_destroy(&attr);
}

MutexBase::~MutexBase() {
  int result = pthread_mutex_destroy(&mutex_.internal_mutex_);
  DCHECK_EQ(result, 0);
}

void MutexBase::lock() {
  int result = pthread_mutex_lock(&mutex_.internal_mutex_);
  DCHECK_EQ(result, 0);
#if DCHECK_IS_ON()
  ++mutex_.recursion_count_;
#endif
}

void MutexBase::unlock() {
#if DCHECK_IS_ON()
  DCHECK(mutex_.recursion_count_);
  --mutex_.recursion_count_;
#endif
  int result = pthread_mutex_unlock(&mutex_.internal_mutex_);
  DCHECK_EQ(result, 0);
}

// There is a separate tryLock implementation for the Mutex and the
// RecursiveMutex since on Windows we need to manually check if tryLock should
// succeed or not for the non-recursive mutex. On Linux the two implementations
// are equal except we can assert the recursion count is always zero for the
// non-recursive mutex.
bool Mutex::TryLock() {
  int result = pthread_mutex_trylock(&mutex_.internal_mutex_);
  if (result == 0) {
#if DCHECK_IS_ON()
    // The Mutex class is not recursive, so the recursionCount should be
    // zero after getting the lock.
    DCHECK(!mutex_.recursion_count_);
    ++mutex_.recursion_count_;
#endif
    return true;
  }
  if (result == EBUSY)
    return false;

  NOTREACHED();
  return false;
}

bool RecursiveMutex::TryLock() {
  int result = pthread_mutex_trylock(&mutex_.internal_mutex_);
  if (result == 0) {
#if DCHECK_IS_ON()
    ++mutex_.recursion_count_;
#endif
    return true;
  }
  if (result == EBUSY)
    return false;

  NOTREACHED();
  return false;
}

ThreadCondition::ThreadCondition() {
  pthread_cond_init(&condition_, nullptr);
}

ThreadCondition::~ThreadCondition() {
  pthread_cond_destroy(&condition_);
}

void ThreadCondition::Wait(Mutex& mutex) {
  PlatformMutex& platform_mutex = mutex.Impl();
  int result = pthread_cond_wait(&condition_, &platform_mutex.internal_mutex_);
  DCHECK_EQ(result, 0);
#if DCHECK_IS_ON()
  ++platform_mutex.recursion_count_;
#endif
}

bool ThreadCondition::TimedWait(Mutex& mutex, double absolute_time) {
  if (absolute_time < CurrentTime())
    return false;

  if (absolute_time > INT_MAX) {
    Wait(mutex);
    return true;
  }

  int time_seconds = static_cast<int>(absolute_time);
  int time_nanoseconds = static_cast<int>((absolute_time - time_seconds) * 1E9);

  timespec target_time;
  target_time.tv_sec = time_seconds;
  target_time.tv_nsec = time_nanoseconds;

  PlatformMutex& platform_mutex = mutex.Impl();
  int result = pthread_cond_timedwait(
      &condition_, &platform_mutex.internal_mutex_, &target_time);
#if DCHECK_IS_ON()
  ++platform_mutex.recursion_count_;
#endif
  return result == 0;
}

void ThreadCondition::Signal() {
  int result = pthread_cond_signal(&condition_);
  DCHECK_EQ(result, 0);
}

void ThreadCondition::Broadcast() {
  int result = pthread_cond_broadcast(&condition_);
  DCHECK_EQ(result, 0);
}

#if DCHECK_IS_ON()
static bool g_thread_created = false;

Mutex& GetThreadCreatedMutex() {
  static Mutex g_thread_created_mutex;
  return g_thread_created_mutex;
}

bool IsBeforeThreadCreated() {
  MutexLocker locker(GetThreadCreatedMutex());
  return !g_thread_created;
}

void WillCreateThread() {
  MutexLocker locker(GetThreadCreatedMutex());
  g_thread_created = true;
}
#endif

}  // namespace WTF

#endif  // defined(OS_POSIX) || defined(OS_FUCHSIA)
