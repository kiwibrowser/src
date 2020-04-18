/*
 *  Copyright (C) 2005, 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_RETAIN_PTR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_RETAIN_PTR_H_

#include <algorithm>
#include <type_traits>
#include <utility>
#include "third_party/blink/renderer/platform/wtf/compiler.h"
#include "third_party/blink/renderer/platform/wtf/hash_table_deleted_value_type.h"
#include "third_party/blink/renderer/platform/wtf/hash_traits.h"
#include "third_party/blink/renderer/platform/wtf/type_traits.h"

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif

#ifndef CF_RELEASES_ARGUMENT
#define CF_RELEASES_ARGUMENT
#endif

#ifndef NS_RELEASES_ARGUMENT
#define NS_RELEASES_ARGUMENT
#endif

namespace WTF {

// Unlike most most of our smart pointers, RetainPtr can take either the pointer
// type or the pointed-to type, so both RetainPtr<NSDictionary> and
// RetainPtr<CFDictionaryRef> will work.

enum AdoptCFTag { kAdoptCF };
enum AdoptNSTag { kAdoptNS };

#ifdef __OBJC__
inline void AdoptNSReference(id ptr) {
  if (ptr) {
    CFRetain(ptr);
    [ptr release];
  }
}
#endif

template <typename T>
class RetainPtr {
 public:
  typedef typename std::remove_pointer<T>::type ValueType;
  typedef ValueType* PtrType;

  RetainPtr() : ptr_(nullptr) {}
  RetainPtr(PtrType ptr) : ptr_(ptr) {
    if (ptr)
      CFRetain(ptr);
  }

  RetainPtr(AdoptCFTag, PtrType ptr) : ptr_(ptr) {}
  RetainPtr(AdoptNSTag, PtrType ptr) : ptr_(ptr) { AdoptNSReference(ptr); }

  RetainPtr(const RetainPtr& o) : ptr_(o.ptr_) {
    if (PtrType ptr = ptr_)
      CFRetain(ptr);
  }

  RetainPtr(RetainPtr&& o) : ptr_(o.LeakRef()) {}

  // Hash table deleted values, which are only constructed and never copied or
  // destroyed.
  RetainPtr(HashTableDeletedValueType) : ptr_(HashTableDeletedValue()) {}
  bool IsHashTableDeletedValue() const {
    return ptr_ == HashTableDeletedValue();
  }

  ~RetainPtr() {
    if (PtrType ptr = ptr_)
      CFRelease(ptr);
  }

  template <typename U>
  RetainPtr(const RetainPtr<U>&);

  void Clear();
  WARN_UNUSED_RESULT PtrType LeakRef();

  PtrType Get() const { return ptr_; }
  PtrType operator->() const { return ptr_; }

  bool operator!() const { return !ptr_; }
  explicit operator bool() const { return ptr_; }

  RetainPtr& operator=(const RetainPtr&);
  template <typename U>
  RetainPtr& operator=(const RetainPtr<U>&);
  RetainPtr& operator=(PtrType);
  template <typename U>
  RetainPtr& operator=(U*);

  RetainPtr& operator=(RetainPtr&&);
  template <typename U>
  RetainPtr& operator=(RetainPtr<U>&&);

  RetainPtr& operator=(std::nullptr_t) {
    Clear();
    return *this;
  }

  void AdoptCF(PtrType);
  void AdoptNS(PtrType);

  void Swap(RetainPtr&);

 private:
  static PtrType HashTableDeletedValue() {
    return reinterpret_cast<PtrType>(-1);
  }

  PtrType ptr_;
};

template <typename T>
template <typename U>
inline RetainPtr<T>::RetainPtr(const RetainPtr<U>& o) : ptr_(o.Get()) {
  if (PtrType ptr = ptr_)
    CFRetain(ptr);
}

template <typename T>
inline void RetainPtr<T>::Clear() {
  if (PtrType ptr = ptr_) {
    ptr_ = nullptr;
    CFRelease(ptr);
  }
}

template <typename T>
inline typename RetainPtr<T>::PtrType RetainPtr<T>::LeakRef() {
  PtrType ptr = ptr_;
  ptr_ = nullptr;
  return ptr;
}

template <typename T>
inline RetainPtr<T>& RetainPtr<T>::operator=(const RetainPtr<T>& o) {
  PtrType optr = o.Get();
  if (optr)
    CFRetain(optr);
  PtrType ptr = ptr_;
  ptr_ = optr;
  if (ptr)
    CFRelease(ptr);
  return *this;
}

template <typename T>
template <typename U>
inline RetainPtr<T>& RetainPtr<T>::operator=(const RetainPtr<U>& o) {
  PtrType optr = o.Get();
  if (optr)
    CFRetain(optr);
  PtrType ptr = ptr_;
  ptr_ = optr;
  if (ptr)
    CFRelease(ptr);
  return *this;
}

template <typename T>
inline RetainPtr<T>& RetainPtr<T>::operator=(PtrType optr) {
  if (optr)
    CFRetain(optr);
  PtrType ptr = ptr_;
  ptr_ = optr;
  if (ptr)
    CFRelease(ptr);
  return *this;
}

template <typename T>
template <typename U>
inline RetainPtr<T>& RetainPtr<T>::operator=(U* optr) {
  if (optr)
    CFRetain(optr);
  PtrType ptr = ptr_;
  ptr_ = optr;
  if (ptr)
    CFRelease(ptr);
  return *this;
}

template <typename T>
inline RetainPtr<T>& RetainPtr<T>::operator=(RetainPtr<T>&& o) {
  AdoptCF(o.LeakRef());
  return *this;
}

template <typename T>
template <typename U>
inline RetainPtr<T>& RetainPtr<T>::operator=(RetainPtr<U>&& o) {
  AdoptCF(o.LeakRef());
  return *this;
}

template <typename T>
inline void RetainPtr<T>::AdoptCF(PtrType optr) {
  PtrType ptr = ptr_;
  ptr_ = optr;
  if (ptr)
    CFRelease(ptr);
}

template <typename T>
inline void RetainPtr<T>::AdoptNS(PtrType optr) {
  AdoptNSReference(optr);

  PtrType ptr = ptr_;
  ptr_ = optr;
  if (ptr)
    CFRelease(ptr);
}

template <typename T>
inline void RetainPtr<T>::Swap(RetainPtr<T>& o) {
  std::swap(ptr_, o.ptr_);
}

template <typename T>
inline void swap(RetainPtr<T>& a, RetainPtr<T>& b) {
  a.Swap(b);
}

template <typename T, typename U>
inline bool operator==(const RetainPtr<T>& a, const RetainPtr<U>& b) {
  return a.Get() == b.Get();
}

template <typename T, typename U>
inline bool operator==(const RetainPtr<T>& a, U* b) {
  return a.Get() == b;
}

template <typename T, typename U>
inline bool operator==(T* a, const RetainPtr<U>& b) {
  return a == b.Get();
}

template <typename T, typename U>
inline bool operator!=(const RetainPtr<T>& a, const RetainPtr<U>& b) {
  return a.Get() != b.Get();
}

template <typename T, typename U>
inline bool operator!=(const RetainPtr<T>& a, U* b) {
  return a.Get() != b;
}

template <typename T, typename U>
inline bool operator!=(T* a, const RetainPtr<U>& b) {
  return a != b.Get();
}

template <typename T>
WARN_UNUSED_RESULT inline RetainPtr<T> AdoptCF(T CF_RELEASES_ARGUMENT);
template <typename T>
inline RetainPtr<T> AdoptCF(T o) {
  return RetainPtr<T>(kAdoptCF, o);
}

template <typename T>
WARN_UNUSED_RESULT inline RetainPtr<T> AdoptNS(T NS_RELEASES_ARGUMENT);
template <typename T>
inline RetainPtr<T> AdoptNS(T o) {
  return RetainPtr<T>(kAdoptNS, o);
}

template <typename T>
struct HashTraits<RetainPtr<T>> : SimpleClassHashTraits<RetainPtr<T>> {};

template <typename T>
struct RetainPtrHash
    : PtrHash<
          typename std::remove_pointer<typename RetainPtr<T>::PtrType>::type> {
  using Base = PtrHash<
      typename std::remove_pointer<typename RetainPtr<T>::PtrType>::type>;
  using Base::GetHash;
  static unsigned GetHash(const RetainPtr<T>& key) { return hash(key.Get()); }
  using Base::Equal;
  static bool Equal(const RetainPtr<T>& a, const RetainPtr<T>& b) {
    return a == b;
  }
  static bool Equal(typename RetainPtr<T>::PtrType a, const RetainPtr<T>& b) {
    return a == b;
  }
  static bool Equal(const RetainPtr<T>& a, typename RetainPtr<T>::PtrType b) {
    return a == b;
  }
};

template <typename T>
struct DefaultHash<RetainPtr<T>> {
  using Hash = RetainPtrHash<T>;
};

}  // namespace WTF

using WTF::kAdoptCF;
using WTF::kAdoptNS;
using WTF::AdoptCF;
using WTF::AdoptNS;
using WTF::RetainPtr;

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_RETAIN_PTR_H_
