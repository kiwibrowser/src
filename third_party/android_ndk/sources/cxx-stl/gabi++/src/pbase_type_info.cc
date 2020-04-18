// Copyright (C) 2011 The Android Open Source Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
// pbase_type_info.cc: Methods for __pbase_type_info.

#include "cxxabi_defines.h"
#include "typeinfo"

#if __cplusplus < 201103L
extern "C" std::type_info _ZTIDn;
#endif

namespace __cxxabiv1
{
  __pbase_type_info::~__pbase_type_info()
  {
  }

  bool __pbase_type_info::can_catch(const __shim_type_info* thr_type,
                                    void*& adjustedPtr) const {
    if (can_catch_typeinfo_wrapper(thr_type, adjustedPtr, first_time_init)) {
      return true;
    }

#if __cplusplus >= 201103L
    // In C++ 11, the type of nullptr is std::nullptr_t, but nullptr can be
    // casted to every pointer types.  Thus, we can return true whenever
    // the exception object is an instance of std::nullptr_t.
    return (*thr_type == typeid(decltype(nullptr)));
#else
    return (*thr_type == _ZTIDn);
#endif
  }

  bool __pbase_type_info::can_catch_typeinfo_wrapper(const __shim_type_info* thr_type,
                                                     void*& adjustedPtr,
                                                     unsigned tracker) const {
    if (*this == *thr_type) {
      return true;
    }

    if (typeid(*this) != typeid(*thr_type)) {
      return false;
    }
    const __pbase_type_info *thrown_type =
        static_cast<const __pbase_type_info *>(thr_type);

    // Both side must be the same cv-qualified
    if (~__flags & thrown_type->__flags) {
      return false;
    }

    // Handle the annoying constness problem
    // Ref: http://www.parashift.com/c++-faq-lite/constptrptr-conversion.html
    if (tracker == first_time_init &&
        (tracker & keep_constness) != keep_constness &&
        (tracker & after_gap) != after_gap) {
      tracker |= keep_constness;
    } else {
      tracker &= ~first_time_init;
    }

    if ((tracker & first_time_init) != first_time_init &&
        (tracker & after_gap) == after_gap) {
      return false;
    }

    if (!(__flags & __const_mask)) {
      tracker |= after_gap;
    }

    return can_catch_ptr(thrown_type, adjustedPtr, tracker);
  }

  bool __pbase_type_info::can_catch_ptr(const __pbase_type_info* thrown_type,
                                        void*& adjustedPtr,
                                        unsigned tracker) const {
    bool result;
    if (do_can_catch_ptr(thrown_type, adjustedPtr, tracker,
                         result)) {
      return result;
    }

    const __pbase_type_info* ptr_pointee =
        dynamic_cast<const __pbase_type_info*>(__pointee);

    if (ptr_pointee) {
      return ptr_pointee->can_catch_typeinfo_wrapper(thrown_type->__pointee,
                                                     adjustedPtr,
                                                     tracker);
    } else {
      return __pointee->can_catch(thrown_type->__pointee, adjustedPtr);
    }
  }
} // namespace __cxxabiv1
