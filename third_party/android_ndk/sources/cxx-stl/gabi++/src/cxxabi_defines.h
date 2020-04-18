// Copyright (C) 2013 The Android Open Source Project
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

#ifndef _GABIXX_CXXABI_DEFINES_H
#define _GABIXX_CXXABI_DEFINES_H

#include <cxxabi.h>
#include <stdint.h>

// Internal declarations for the implementation of <cxxabi.h> and
// related headers.

namespace __cxxabiv1 {

// Derived types of type_info below are based on 2.9.5 of C++ ABI.

class __shim_type_info : public std::type_info
{
  public:
  virtual ~__shim_type_info();
  virtual bool can_catch(const __shim_type_info* thrown_type,
                          void*& adjustedPtr) const = 0;
};

// Typeinfo for fundamental types.
class __fundamental_type_info : public __shim_type_info
{
public:
  virtual ~__fundamental_type_info();
  virtual bool can_catch(const __shim_type_info* thrown_type,
                          void*& adjustedPtr) const;
};

// Typeinfo for array types.
class __array_type_info : public __shim_type_info
{
public:
  virtual ~__array_type_info();
  virtual bool can_catch(const __shim_type_info* thrown_type,
                          void*& adjustedPtr) const;
};

// Typeinfo for function types.
class __function_type_info : public __shim_type_info
{
public:
  virtual ~__function_type_info();
  virtual bool can_catch(const __shim_type_info* thrown_type,
                          void*& adjustedPtr) const;
};

// Typeinfo for enum types.
class __enum_type_info : public __shim_type_info
{
public:
  virtual ~__enum_type_info();
  virtual bool can_catch(const __shim_type_info* thrown_type,
                          void*& adjustedPtr) const;
};


class __class_type_info;

// Used in __vmi_class_type_info
struct __base_class_type_info
{
public:
  const __class_type_info *__base_type;

  long __offset_flags;

  enum __offset_flags_masks {
    __virtual_mask = 0x1,
    __public_mask = 0x2,
    __offset_shift = 8   // lower 8 bits are flags
  };

  bool is_virtual() const {
    return (__offset_flags & __virtual_mask) != 0;
  }

  bool is_public() const {
    return (__offset_flags & __public_mask) != 0;
  }

  // FIXME: Right-shift of signed integer is implementation dependent.
  // GCC Implements it as signed (as we expect)
  long offset() const {
    return __offset_flags >> __offset_shift;
  }

  long flags() const {
    return __offset_flags & ((1 << __offset_shift) - 1);
  }
};

// Helper struct to support catch-clause match
struct __UpcastInfo {
  enum ContainedStatus {
    unknown = 0,
    has_public_contained,
    has_ambig_or_not_public
  };

  ContainedStatus status;
  const __class_type_info* base_type;
  void* adjustedPtr;
  unsigned int premier_flags;
  bool nullobj_may_conflict;

  __UpcastInfo(const __class_type_info* type);
};

// Typeinfo for classes with no bases.
class __class_type_info : public __shim_type_info
{
public:
  virtual ~__class_type_info();
  virtual bool can_catch(const __shim_type_info* thrown_type,
                          void*& adjustedPtr) const;

  enum class_type_info_code {
    CLASS_TYPE_INFO_CODE,
    SI_CLASS_TYPE_INFO_CODE,
    VMI_CLASS_TYPE_INFO_CODE
  };

  virtual class_type_info_code
    code() const { return CLASS_TYPE_INFO_CODE; }

  virtual bool walk_to(const __class_type_info* base_type,
                        void*& adjustedPtr,
                        __UpcastInfo& info) const;

protected:
  bool self_class_type_match(const __class_type_info* base_type,
                              void*& adjustedPtr,
                              __UpcastInfo& info) const;
};

// Typeinfo for classes containing only a single, public, non-virtual base at
// offset zero.
class __si_class_type_info : public __class_type_info
{
public:
  virtual ~__si_class_type_info();
  const __class_type_info *__base_type;

  virtual __class_type_info::class_type_info_code
    code() const { return SI_CLASS_TYPE_INFO_CODE; }

  virtual bool walk_to(const __class_type_info* base_type,
                        void*& adjustedPtr,
                        __UpcastInfo& info) const;
};


// Typeinfo for classes with bases that do not satisfy the
// __si_class_type_info constraints.
class __vmi_class_type_info : public __class_type_info
{
public:
  virtual ~__vmi_class_type_info();
  unsigned int __flags;
  unsigned int __base_count;
  __base_class_type_info __base_info[1];

  enum __flags_masks {
    __non_diamond_repeat_mask = 0x1,
    __diamond_shaped_mask = 0x2,
  };

  virtual __class_type_info::class_type_info_code
    code() const { return VMI_CLASS_TYPE_INFO_CODE; }

  virtual bool walk_to(const __class_type_info* base_type,
                        void*& adjustedPtr,
                        __UpcastInfo& info) const;
};

class __pbase_type_info : public __shim_type_info
{
public:
  virtual ~__pbase_type_info();
  virtual bool can_catch(const __shim_type_info* thrown_type,
                          void*& adjustedPtr) const;
  unsigned int __flags;
  const __shim_type_info* __pointee;

  enum __masks {
    __const_mask = 0x1,
    __volatile_mask = 0x2,
    __restrict_mask = 0x4,
    __incomplete_mask = 0x8,
    __incomplete_class_mask = 0x10
  };


  virtual bool can_catch_typeinfo_wrapper(const __shim_type_info* thrown_type,
                                          void*& adjustedPtr,
                                          unsigned tracker) const;

protected:
  enum __constness_tracker_status {
    first_time_init = 0x1,
    keep_constness = 0x2,
    after_gap = 0x4         // after one non-const qualified,
                            // we cannot face const again in future
  };

private:
  bool can_catch_ptr(const __pbase_type_info *thrown_type,
                      void *&adjustedPtr,
                      unsigned tracker) const;

  // Return true if making decision done.
  virtual bool do_can_catch_ptr(const __pbase_type_info* thrown_type,
                                void*& adjustedPtr,
                                unsigned tracker,
                                bool& result) const = 0;
};

class __pointer_type_info : public __pbase_type_info
{
public:
  virtual ~__pointer_type_info();

private:
  virtual bool do_can_catch_ptr(const __pbase_type_info* thrown_type,
                                void*& adjustedPtr,
                                unsigned tracker,
                                bool& result) const;
};

class __pointer_to_member_type_info : public __pbase_type_info
{
public:
  __class_type_info* __context;

  virtual ~__pointer_to_member_type_info();

private:
  virtual bool do_can_catch_ptr(const __pbase_type_info* thrown_type,
                                void*& adjustedPtr,
                                unsigned tracker,
                                bool& result) const;
};

extern "C" {

// Compatible with GNU C++
const uint64_t __gxx_exception_class = 0x474E5543432B2B00LL; // GNUCC++\0

struct __cxa_exception {
  size_t referenceCount;

  std::type_info* exceptionType;
  void (*exceptionDestructor)(void*);
  std::unexpected_handler unexpectedHandler;
  std::terminate_handler terminateHandler;
  __cxa_exception* nextException;

    int handlerCount;
#ifdef __arm__
  /**
    * ARM EHABI requires the unwind library to keep track of exceptions
    * during cleanups.  These support nesting, so we need to keep a list of
    * them.
    */
  __cxa_exception* nextCleanup;
  int cleanupCount;
#endif
    int handlerSwitchValue;
    const uint8_t* actionRecord;
    const uint8_t* languageSpecificData;
    void* catchTemp;
    void* adjustedPtr;

    _Unwind_Exception unwindHeader; // must be last
};

struct __cxa_eh_globals {
  __cxa_exception* caughtExceptions;
  unsigned int uncaughtExceptions;
#ifdef __arm__
  __cxa_exception* cleanupExceptions;
#endif
};

}  // extern "C"
}  // namespace __cxxabiv1

namespace __gabixx {

// Default unexpected handler.
_GABIXX_NORETURN void __default_unexpected(void) _GABIXX_HIDDEN;

// Default terminate handler.
_GABIXX_NORETURN void __default_terminate(void) _GABIXX_HIDDEN;

// Call |handler| and if it returns, call __default_terminate.
_GABIXX_NORETURN void __terminate(std::terminate_handler handler)
    _GABIXX_HIDDEN;

// Print a fatal error message to the log+stderr, then call
// std::terminate().
_GABIXX_NORETURN void __fatal_error(const char* message) _GABIXX_HIDDEN;

}  // __gabixx

#endif  // _GABIXX_CXXABI_DEFINES_H
