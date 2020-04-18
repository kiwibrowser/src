/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_NACL_BASE_NACL_REFCOUNT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_NACL_BASE_NACL_REFCOUNT_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/nacl_sync.h"

#ifdef __cplusplus
# include "native_client/src/include/nacl_scoped_ptr.h"
# define NACL_IS_REFCOUNT_SUBCLASS  ; typedef char is_refcount_subclass
#else
# define NACL_IS_REFCOUNT_SUBCLASS
#endif

EXTERN_C_BEGIN

/*
 * Yields an lvalue of the vtable pointer, when given a subclass of
 * NaClRefCount and a pointer to an instance -- with the convention
 * that the vtable pointer is at the first word.
 *
 * Since we have the naming convention that struct Type contains an
 * element of type struct TypeVtbl *, we make it so that less typing
 * is required for the use of the macro and automatically constructing
 * the vtable type name from the subclass name.
 */
#define NACL_VTBL(type, ptr) \
  (*(struct type ## Vtbl const **) (void *) ptr)

struct NaClRefCountVtbl;

struct NaClRefCount {
  struct NaClRefCountVtbl const *vtbl NACL_IS_REFCOUNT_SUBCLASS;

  /* protected */
  struct NaClFastMutex          mu;
  /*
   * To save on constructing another mutex, it is acceptable for a
   * subclass to use this mutex for short operations.
   */

  /* private */
  size_t                        ref_count;
};

struct NaClRefCountVtbl {
  void (*Dtor)(struct NaClRefCount  *vself);
};

/*
 * Placement new style ctor; creates w/ ref_count of 1.
 *
 * The subclasses' ctor must call this base class ctor during their
 * contruction.
 */
int NaClRefCountCtor(struct NaClRefCount *nrcp) NACL_WUR;

struct NaClRefCount *NaClRefCountRef(struct NaClRefCount *nrcp);

/* when ref_count reaches zero, will call dtor and free */
void NaClRefCountUnref(struct NaClRefCount *nrcp);

/*
 * NaClRefCountSafeUnref is just like NaCRefCountUnref, except that
 * nrcp may be NULL (in which case this is a noop).
 *
 * Used in failure cleanup of initialization code, esp in Ctors that
 * can fail.
 */
void NaClRefCountSafeUnref(struct NaClRefCount *nrcp);

/*
 * For subclasses that need a lock for fast operations.  Exposing this
 * eliminates the need for constructing another lock in the subclass.
 */
void NaClRefCountLock(struct NaClRefCount *nrcp);
void NaClRefCountUnlock(struct NaClRefCount *nrcp);

extern struct NaClRefCountVtbl const kNaClRefCountVtbl;

EXTERN_C_END

#ifdef __cplusplus

namespace nacl {

// syntactic glucose to handle transferring responsibility to release
// uninitialized memory from a scoped_ptr_malloc to a scoped pointer
// that handles dereferencing a refcounted object.

class NaClScopedRefCountNaClDescDtor {
 public:
  inline void operator()(NaClRefCount* x) const {
    NaClRefCountUnref(x);
  }
};

// RC must be a NaClRefCount subclass.  unfortunately we can only
// enforce this by checking that the RC struct contains a common
// attribute -- the NACL_IS_REFCOUNT_SUBCLASS macro must be included
// for every subclass that wants to use this template
template <typename RC, typename DtorProc = NaClScopedRefCountNaClDescDtor>
class scoped_ptr_nacl_refcount {
 public:
  // standard ctor
  scoped_ptr_nacl_refcount(nacl::scoped_ptr_malloc<RC>* p, int ctor_fn_result)
      : ptr_(NULL) {
    enum { must_be_subclass = sizeof(typename RC::is_refcount_subclass) };

    if (ctor_fn_result) {
      // we are now responsible for calling the unref, which, if the
      // refcount drops to zero, will handle the free.
      ptr_ = p->release();
    }
  }

  // copy ctor
  scoped_ptr_nacl_refcount(scoped_ptr_nacl_refcount const& other) {
    ptr_ = NaClRefCountRef(other.ptr_);
  }

  // assign
  scoped_ptr_nacl_refcount& operator=(scoped_ptr_nacl_refcount const& other) {
    if (this != &other) {  // exclude self-assignment, which should be no-op
      if (NULL != ptr_) {
        DtorProc deref;
        deref(reinterpret_cast<NaClRefCount*>(ptr_));
      }
      ptr_ = NaClRefCountRef(other.ptr_);
    }
    return *this;
  }

  ~scoped_ptr_nacl_refcount() {
    if (NULL != ptr_) {
      DtorProc deref;
      deref(reinterpret_cast<NaClRefCount*>(ptr_));
    }
  }

  bool constructed() { return NULL != ptr_; }

  void reset(nacl::scoped_ptr_malloc<RC>* p = NULL, int ctor_fn_result = 0) {
    if (NULL != ptr_) {
      DtorProc deref;
      deref(reinterpret_cast<NaClRefCount*>(ptr_));
      ptr_ = NULL;
    }
    if (0 != ctor_fn_result) {
      ptr_ = p->release();
    }
  }

  // Accessors
  RC& operator*() const {
    return *ptr_;
  }

  RC* operator->() const {
    return ptr_;
  }

  RC* get() const { return ptr_; }

  /*
   * ptr eq, so same pointer, and not equality testing on contents.
   */
  bool operator==(nacl::scoped_ptr_malloc<RC> const& p) const {
    return ptr_ == p.get();
  }
  bool operator==(RC const* p) const { return ptr_ == p; }

  bool operator!=(nacl::scoped_ptr_malloc<RC> const& p) const {
    return ptr_ != p.get();
  }
  bool operator!=(RC const* p) const { return ptr_ != p; }

  void swap(scoped_ptr_nacl_refcount& p2) {
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

template<typename RC, typename DP>
void swap(scoped_ptr_nacl_refcount<RC, DP>& a,
          scoped_ptr_nacl_refcount<RC, DP>& b) {
  a.swap(b);
}

template<typename RC, typename DP> inline
bool operator==(nacl::scoped_ptr_malloc<RC> const& a,
                scoped_ptr_nacl_refcount<RC, DP> const& b) {
  return a.get() == b.get();
}

template<class RC, typename DP> inline
bool operator==(RC const* a,
                scoped_ptr_nacl_refcount<RC, DP> const& b) {
  return a == b.get();
}

template<typename RC, typename DP> inline
bool operator!=(nacl::scoped_ptr_malloc<RC> const& a,
                scoped_ptr_nacl_refcount<RC, DP> const& b) {
  return a.get() != b.get();
}

template<typename RC, typename DP> inline
bool operator!=(RC const* a,
                scoped_ptr_nacl_refcount<RC, DP> const& b) {
  return a != b.get();
}

}  // namespace

#endif

#endif
