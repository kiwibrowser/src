// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_MEMORY_MANAGER_H_
#define GESTURES_MEMORY_MANAGER_H_

#include <new>

#include "gestures/include/logging.h"
#include "gestures/include/macros.h"

namespace gestures {

// A simple memory manager class that pre-allocates a size of
// buffer and garbage collects freed variables. It is not intended
// to be used directly. User classes should inherit the
// MemoryManaged wrapper class and use the provided Allocate/Free
// interface instead (see below).

template<typename T>
class MemoryManager {
 public:
  explicit MemoryManager(size_t size) : buf_(new T[size]),
    free_slots_(new T *[size]), used_mark_(new bool[size]()),
    max_size_(size), head_(size) {
    for (size_t i = 0; i < max_size_; i++)
      free_slots_[i] = buf_.get() + i;
  }

  size_t Size() const { return max_size_ - head_; }
  size_t MaxSize() const { return max_size_; }

  T* Allocate() {
    if (!head_) {
      Err("MemoryManager::Allocate: out of space");
      return NULL;
    }

    T* ptr = free_slots_[--head_];
    used_mark_[ptr - buf_.get()] = true;
    return ptr;
  }

  bool Free(T* ptr) {
    // Check for invalid pointer and double-free
    if (ptr < buf_.get() || ptr >= buf_.get() + max_size_) {
      Err("MemoryManager::Free: pointer out of bounds");
      return false;
    }
    size_t offset_in_bytes = reinterpret_cast<size_t>(ptr) -
        reinterpret_cast<size_t>(buf_.get());
    if (offset_in_bytes % sizeof(T)) {
      Err("MemoryManager::Free: unaligned pointer");
      return false;
    }
    size_t offset = ptr - buf_.get();
    if (!used_mark_[offset]) {
      Err("MemoryManager::Free: double-free");
      return false;
    }

    free_slots_[head_++] = ptr;
    used_mark_[offset] = false;
    return true;
  }

 private:
  std::unique_ptr<T[]> buf_;
  std::unique_ptr<T*[]> free_slots_;
  std::unique_ptr<bool[]> used_mark_;
  size_t max_size_;
  size_t head_;
  DISALLOW_COPY_AND_ASSIGN(MemoryManager<T>);
};

}  // namespace gestures

#endif  // MEMORY_MANAGER_H_
