/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_SCOPED_PTR_REFCOUNT_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_SCOPED_PTR_REFCOUNT_H_

namespace nacl {

template <typename RC>
class scoped_ptr_refcount {
 public:
  // standard ctors
  explicit scoped_ptr_refcount(RC* p = NULL)
      : ptr_(p) {
  }

  // copy ctor
  scoped_ptr_refcount(scoped_ptr_refcount const& other) {
    ptr_ = other.ptr_->Ref();
  }

  // assign
  scoped_ptr_refcount& operator=(scoped_ptr_refcount const& other) {
    if (this != &other) {  // exclude self-assignment, which should be no-op
      if (NULL != ptr_) {
        ptr_->Unref();
      }
      ptr_ = other.ptr_->Ref();
    }
    return *this;
  }

  ~scoped_ptr_refcount() {
    if (NULL != ptr_) {
      ptr_->Unref();
    }
  }

  void reset(RC *p = NULL) {
    if (NULL != ptr_) {
      ptr_->Unref();
    }
    ptr_ = p;
  }

  // Accessors
  RC& operator*() const {
    return *ptr_;  // may fault if ptr_ is NULL
  }

  RC* operator->() const {
    return ptr_;
  }

  RC* get() const { return ptr_; }

  /*
   * ptr eq, so same pointer, and not equality testing on contents.
   */
  bool operator==(RC const* p) const { return ptr_ == p; }
  bool operator!=(RC const* p) const { return ptr_ != p; }

  bool operator==(scoped_ptr_refcount const&p2) const {
    return ptr_ == p2.ptr;
  }
  bool operator!=(scoped_ptr_refcount const&p2) const {
    return ptr_ != p2.ptr;
  }

  void swap(scoped_ptr_refcount& p2) {
    RC* tmp = ptr_;
    ptr_ = p2.ptr_;
    p2.ptr_ = tmp;
  }

  RC* release() {
    RC* tmp = ptr_;
    ptr_ = NULL;
    return tmp;
  }

 private:
  RC* ptr_;
};

template<typename RC>
void swap(scoped_ptr_refcount<RC>& a,
          scoped_ptr_refcount<RC>& b) {
  a.swap(b);
}

template<class RC> inline
bool operator==(RC const* a,
                scoped_ptr_refcount<RC> const& b) {
  return a == b.get();
}

template<typename RC, typename DP> inline
bool operator!=(RC const* a,
                scoped_ptr_refcount<RC> const& b) {
  return a != b.get();
}

}  // namespace

#endif  /* NATIVE_CLIENT_SRC_SHARED_PLATFORM_SCOPED_PTR_REFCOUNT_H_ */
