// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// See port_example.h for documentation for the following types/functions.

#ifndef STORAGE_LEVELDB_PORT_PORT_CHROMIUM_H_
#define STORAGE_LEVELDB_PORT_PORT_CHROMIUM_H_

#include <stddef.h>
#include <stdint.h>

#include <cstring>
#include <string>

#include "base/atomicops.h"
#include "base/macros.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "build/build_config.h"

// Linux's ThreadIdentifier() needs this.
#if defined(OS_LINUX)
#  include <linux/unistd.h>
#endif

#if defined(OS_WIN)
typedef SSIZE_T ssize_t;
#endif

namespace leveldb {
namespace port {

// Chromium only supports little endian.
static const bool kLittleEndian = true;

class LOCKABLE Mutex {
 public:
  Mutex();
  ~Mutex();
  void Lock() EXCLUSIVE_LOCK_FUNCTION();
  void Unlock() UNLOCK_FUNCTION();
  void AssertHeld() ASSERT_EXCLUSIVE_LOCK();

 private:
  base::Lock mu_;

  friend class CondVar;
  DISALLOW_COPY_AND_ASSIGN(Mutex);
};

class CondVar {
 public:
  explicit CondVar(Mutex* mu);
  ~CondVar();
  void Wait();
  void Signal();
  void SignalAll();

 private:
  base::ConditionVariable cv_;

  DISALLOW_COPY_AND_ASSIGN(CondVar);
};

class AtomicPointer {
 private:
  typedef base::subtle::AtomicWord Rep;
  Rep rep_;
 public:
  AtomicPointer() { }
  explicit AtomicPointer(void* p) : rep_(reinterpret_cast<Rep>(p)) {}
  inline void* Acquire_Load() const {
    return reinterpret_cast<void*>(base::subtle::Acquire_Load(&rep_));
  }
  inline void Release_Store(void* v) {
    base::subtle::Release_Store(&rep_, reinterpret_cast<Rep>(v));
  }
  inline void* NoBarrier_Load() const {
    return reinterpret_cast<void*>(base::subtle::NoBarrier_Load(&rep_));
  }
  inline void NoBarrier_Store(void* v) {
    base::subtle::NoBarrier_Store(&rep_, reinterpret_cast<Rep>(v));
  }
};

// Implementation of OnceType and InitOnce() pair, this is equivalent to
// pthread_once_t and pthread_once().
typedef base::subtle::Atomic32 OnceType;

enum {
  ONCE_STATE_UNINITIALIZED = 0,
  ONCE_STATE_EXECUTING_CLOSURE = 1,
  ONCE_STATE_DONE = 2
};

#define LEVELDB_ONCE_INIT   leveldb::port::ONCE_STATE_UNINITIALIZED

// slow code path
void InitOnceImpl(OnceType* once, void (*initializer)());

static inline void InitOnce(OnceType* once, void (*initializer)()) {
  if (base::subtle::Acquire_Load(once) != ONCE_STATE_DONE)
    InitOnceImpl(once, initializer);
}

bool Snappy_Compress(const char* input, size_t input_length,
                     std::string* output);
bool Snappy_GetUncompressedLength(const char* input, size_t length,
                                  size_t* result);
bool Snappy_Uncompress(const char* input_data, size_t input_length,
                       char* output);

inline bool GetHeapProfile(void (*func)(void*, const char*, int), void* arg) {
  return false;
}

uint32_t AcceleratedCRC32C(uint32_t crc, const char* buf, size_t size);

}  // namespace port
}  // namespace leveldb

#endif  // STORAGE_LEVELDB_PORT_PORT_CHROMIUM_H_
