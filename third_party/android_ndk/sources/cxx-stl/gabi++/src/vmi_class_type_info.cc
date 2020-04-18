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
// vmi_class_type_info.cc: Methods for __vmi_class_type_info.

#include <cassert>
#include "cxxabi_defines.h"

namespace __cxxabiv1
{
  __vmi_class_type_info::~__vmi_class_type_info()
  {
  }

  bool __vmi_class_type_info::walk_to(const __class_type_info* base_type,
                                      void*& adjustedPtr,
                                      __UpcastInfo& info) const {
    if (self_class_type_match(base_type, adjustedPtr, info)) {
      return true;
    }

    for (size_t i = 0, e = __base_count; i != e; ++i) {
      __UpcastInfo cur_base_info(this);
      void* cur_base_ptr = adjustedPtr;
      const __class_type_info* cur_base_type = __base_info[i].__base_type;
      long cur_base_offset = __base_info[i].offset();
      bool cur_base_is_virtual = __base_info[i].is_virtual();
      bool cur_base_is_public = __base_info[i].is_public();

      // Adjust cur_base_ptr to correct position
      if (cur_base_ptr) {
        if (cur_base_is_virtual) {
          void* vtable = *reinterpret_cast<void**>(cur_base_ptr);
          cur_base_offset = *reinterpret_cast<long*>(
                  static_cast<uint8_t*>(vtable) + cur_base_offset);
        }
        cur_base_ptr = static_cast<uint8_t*>(cur_base_ptr) + cur_base_offset;
      }


      if (!cur_base_is_public &&
          (info.premier_flags & __non_diamond_repeat_mask) == 0) {
        continue;
      }


      /*** Body ***/


      if (!cur_base_type->walk_to(base_type, cur_base_ptr, cur_base_info)) {
        continue;
      }


      /*** Post ***/


      if (!cur_base_is_public) {  // Narrow public attribute
        cur_base_info.status = __UpcastInfo::has_ambig_or_not_public;
      }

      if (cur_base_is_virtual) {
        // We can make sure it may not conflict from now.
        cur_base_info.nullobj_may_conflict = false;
      }

      // First time found
      if (info.base_type == NULL && cur_base_info.base_type != NULL) {
        info = cur_base_info;
        if (info.status == __UpcastInfo::has_public_contained &&
            (__flags & __non_diamond_repeat_mask) == 0) {
          // Don't need to call deeper recursively since
          // it has no non-diamond repeat superclass

          // Return true only means we found one.
          // It didn't guarantee it is the unique public one. We need to check
          // publicity in the recursive caller-side.
          //
          // Why we don't just return false? No, we can't.
          // Return false will make the caller ignore this base class directly,
          // but we need some information kept in the info.
          return true;
        }
        continue;
      }

      assert (info.base_type != NULL && cur_base_info.base_type != NULL);

      // Found twice, but different types
      if (*cur_base_info.base_type != *info.base_type) {
        info.status = __UpcastInfo::has_ambig_or_not_public;
        return true;
      }

      // Found twice, but null object
      if (!info.adjustedPtr && !cur_base_info.adjustedPtr) {
        if (info.nullobj_may_conflict || cur_base_info.nullobj_may_conflict) {
          info.status = __UpcastInfo::has_ambig_or_not_public;
          return true;
        }

        if (*info.base_type == *cur_base_info.base_type) {
          // The two ptr definitely not point to the same object, although
          // their base_type are the same.
          // Otherwise, It will immediately return back at the statement before:
          //
          //    if (info.status == __UpcastInfo::has_public_contained &&
          //        (__flags & __non_diamond_repeat_mask) == 0) { return true; }
          info.status = __UpcastInfo::has_ambig_or_not_public;
          return true;
        }
      }

      assert (*info.base_type == *cur_base_info.base_type);

      // Logically, this should be:
      //   assert (info.adjustedPtr || cur_base_info.adjustedPtr);
      // But in reality, this will be:
      assert (info.adjustedPtr && cur_base_info.adjustedPtr);

      // Found twice, but different real objects
      if (info.adjustedPtr != cur_base_info.adjustedPtr) {
        info.status = __UpcastInfo::has_ambig_or_not_public;
        return true;
      }

      // Found twice, but the same real object (virtual base)
      continue;
    }

    // We need information in info unless we know nothing
    return info.status != __UpcastInfo::unknown;
  }
} // namespace __cxxabiv1
