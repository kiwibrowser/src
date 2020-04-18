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
// class_type_info.cc: Methods for __class_type_info.

#include "cxxabi_defines.h"

namespace __cxxabiv1
{
  __class_type_info::~__class_type_info()
  {
  }

  bool __class_type_info::can_catch(const __shim_type_info* thrown_type,
                                    void*& adjustedPtr) const {
    if (*this == *thrown_type) {
      return true;
    }

    const __class_type_info* thrown_class_type =
      dynamic_cast<const __class_type_info*>(thrown_type);
    if (thrown_class_type == 0) {
      return false;
    }

    __UpcastInfo info(this);
    thrown_class_type->walk_to(this, adjustedPtr, info);

    if (info.status != __UpcastInfo::has_public_contained) {
      return false;
    }

    adjustedPtr = info.adjustedPtr;
    return true;
  }

  bool __class_type_info::walk_to(const __class_type_info* base_type,
                                  void*& adjustedPtr,
                                  __UpcastInfo& info) const {
    return self_class_type_match(base_type, adjustedPtr, info);
  }

  bool __class_type_info::self_class_type_match(const __class_type_info* base_type,
                                                void*& adjustedPtr,
                                                __UpcastInfo& info) const {
    if (*this == *base_type) {
      info.status = __UpcastInfo::has_public_contained;
      info.base_type = base_type;
      info.adjustedPtr = adjustedPtr;
      info.nullobj_may_conflict = true;
      return true;
    }

    return false;
  }


  __UpcastInfo::__UpcastInfo(const __class_type_info* type)
    : status(unknown), base_type(0), adjustedPtr(0),
      premier_flags(0), nullobj_may_conflict(true) {
    // Keep the shape information for future use.
    if (const __vmi_class_type_info* p =
           dynamic_cast<const __vmi_class_type_info*>(type)) {
      premier_flags = p->__flags;
    }
  }
} // namespace __cxxabiv1
