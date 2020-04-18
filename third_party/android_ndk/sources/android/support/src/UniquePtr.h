/*
 * Copyright (C) 2017 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef ANDROID_SUPPORT_UNIQUE_PTR_H
#define ANDROID_SUPPORT_UNIQUE_PTR_H

namespace {

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  void operator=(const TypeName&) = delete

template <typename T>
struct DefaultDelete {
  enum { type_must_be_complete = sizeof(T) };
  DefaultDelete() {
  }
  void operator()(T* p) const {
    delete p;
  }
};

template <typename T>
struct DefaultDelete<T[]> {
  enum { type_must_be_complete = sizeof(T) };
  void operator()(T* p) const {
    delete[] p;
  }
};

template <typename T, typename D = DefaultDelete<T> >
class UniquePtr {
 public:
  explicit UniquePtr(T* ptr = NULL) : mPtr(ptr) {
  }

  ~UniquePtr() {
    reset();
  }

  T& operator*() const {
    return *mPtr;
  }
  T* operator->() const {
    return mPtr;
  }
  T* get() const {
    return mPtr;
  }

  T* release() __attribute__((warn_unused_result)) {
    T* result = mPtr;
    mPtr = NULL;
    return result;
  }

  void reset(T* ptr = NULL) {
    if (ptr != mPtr) {
      D()(mPtr);
      mPtr = ptr;
    }
  }

 private:
  T* mPtr;

  template <typename T2>
  bool operator==(const UniquePtr<T2>& p) const;
  template <typename T2>
  bool operator!=(const UniquePtr<T2>& p) const;

  DISALLOW_COPY_AND_ASSIGN(UniquePtr);
};

// Partial specialization for array types. Like std::unique_ptr, this removes
// operator* and operator-> but adds operator[].
template <typename T, typename D>
class UniquePtr<T[], D> {
 public:
  explicit UniquePtr(T* ptr = NULL) : mPtr(ptr) {
  }

  ~UniquePtr() {
    reset();
  }

  T& operator[](size_t i) const {
    return mPtr[i];
  }
  T* get() const {
    return mPtr;
  }

  T* release() __attribute__((warn_unused_result)) {
    T* result = mPtr;
    mPtr = NULL;
    return result;
  }

  void reset(T* ptr = NULL) {
    if (ptr != mPtr) {
      D()(mPtr);
      mPtr = ptr;
    }
  }

 private:
  T* mPtr;

  DISALLOW_COPY_AND_ASSIGN(UniquePtr);
};

} // anonymous namespace

#endif  /* ANDROID_SUPPORT_UNIQUE_PTR_H */
