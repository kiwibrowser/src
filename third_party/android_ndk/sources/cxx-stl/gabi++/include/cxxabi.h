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

#ifndef __GABIXX_CXXABI_H__
#define __GABIXX_CXXABI_H__

// The specifications for the declarations found in this header are
// the following:
//
// - Itanium C++ ABI [1]
//   Used on about every CPU architecture, _except_ ARM, this
//   is also commonly referred as the "generic C++ ABI".
//
//   NOTE: This document seems to only covers C++98
//
// - Itanium C++ ABI: Exception Handling. [2]
//   Supplement to the above document describing how exception
//   handle works with the generic C++ ABI. Again, this only
//   seems to support C++98.
//
// - C++ ABI for the ARM architecture [3]
//   Describes the ARM C++ ABI, mainly as a set of differences from
//   the generic one.
//
// - Exception Handling for the ARM Architecture [4]
//   Describes exception handling for ARM in detail. There are rather
//   important differences in the stack unwinding process and
//   exception cleanup.
//
// There are also no freely availabel documentation about certain
// features introduced in C++0x or later. In this case, the best
// source for information are the GNU and LLVM C++ runtime libraries
// (libcxxabi, libsupc++ and even libc++ sources), as well as a few
// proposals, for example:
//
// - For exception propagation:
//   http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2007/n2179.html
//   But the paper only describs the high-level language feature, not
//   the low-level runtime support required to implement it.
//
// - For nested exceptions:
//   http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2008/n2559.html
//   Yet another high-level description without low-level details.
//
#include <gabixx_config.h>

#include <exception>
#include <stdint.h>
#include <typeinfo>
#include <unwind.h>

// When LIBCXXABI, gabi++ should emulate libc++abi. _LIBCPPABI_VERSION must
// be defined in cxxabi.h to complete this abstraction for libc++.
#if defined(LIBCXXABI)
#define _LIBCPPABI_VERSION 1001
#endif

namespace __cxxabiv1
{
  extern "C" {

    // TODO: Support dependent exception
    // TODO: Support C++0x exception propagation
    // http://sourcery.mentor.com/archives/cxx-abi-dev/msg01924.html
    struct __cxa_exception;
    struct __cxa_eh_globals;

    __cxa_eh_globals* __cxa_get_globals() _GABIXX_NOEXCEPT ;
    __cxa_eh_globals* __cxa_get_globals_fast() _GABIXX_NOEXCEPT;

    void* __cxa_allocate_exception(size_t thrown_size) _GABIXX_NOEXCEPT;
    void __cxa_free_exception(void* thrown_exception) _GABIXX_NOEXCEPT;

    void __cxa_throw(void* thrown_exception,
                     std::type_info* tinfo,
                     void (*dest)(void*)) _GABIXX_NORETURN;

    void __cxa_rethrow() _GABIXX_NORETURN;

    void* __cxa_begin_catch(void* exceptionObject) _GABIXX_NOEXCEPT;
    void __cxa_end_catch() _GABIXX_NOEXCEPT;

#ifdef __arm__
    bool __cxa_begin_cleanup(_Unwind_Exception*);
    void __cxa_end_cleanup();
#endif

    void __cxa_bad_cast() _GABIXX_NORETURN;
    void __cxa_bad_typeid() _GABIXX_NORETURN;

    void* __cxa_get_exception_ptr(void* exceptionObject) _GABIXX_NOEXCEPT;

    void __cxa_pure_virtual() _GABIXX_NORETURN;
    void __cxa_deleted_virtual() _GABIXX_NORETURN;

    // Missing libcxxabi functions.
    bool __cxa_uncaught_exception() _GABIXX_NOEXCEPT;

    void __cxa_decrement_exception_refcount(void* exceptionObject)
        _GABIXX_NOEXCEPT;

    void __cxa_increment_exception_refcount(void* exceptionObject)
        _GABIXX_NOEXCEPT;

    void __cxa_rethrow_primary_exception(void* exceptionObject);

    void* __cxa_current_primary_exception() _GABIXX_NOEXCEPT;

    // The ARM ABI mandates that constructors and destructors
    // must return 'this', i.e. their first parameter. This is
    // also true for __cxa_vec_ctor and __cxa_vec_cctor.
#ifdef __arm__
    typedef void* __cxa_vec_ctor_return_type;
#else
    typedef void __cxa_vec_ctor_return_type;
#endif

    typedef __cxa_vec_ctor_return_type
        (*__cxa_vec_constructor)(void *);

    typedef __cxa_vec_constructor __cxa_vec_destructor;

    typedef __cxa_vec_ctor_return_type
        (*__cxa_vec_copy_constructor)(void*, void*);

    void* __cxa_vec_new(size_t element_count,
                        size_t element_size,
                        size_t padding_size,
                        __cxa_vec_constructor constructor,
                        __cxa_vec_destructor destructor);

    void* __cxa_vec_new2(size_t element_count,
                         size_t element_size,
                         size_t padding_size,
                         __cxa_vec_constructor constructor,
                         __cxa_vec_destructor destructor,
                         void* (*alloc)(size_t),
                         void  (*dealloc)(void*));

    void* __cxa_vec_new3(size_t element_count,
                         size_t element_size,
                         size_t padding_size,
                         __cxa_vec_constructor constructor,
                         __cxa_vec_destructor destructor,
                         void* (*alloc)(size_t),
                         void  (*dealloc)(void*, size_t));

    __cxa_vec_ctor_return_type
    __cxa_vec_ctor(void*  array_address,
                   size_t element_count,
                   size_t element_size,
                   __cxa_vec_constructor constructor,
                   __cxa_vec_destructor destructor);

    void __cxa_vec_dtor(void*  array_address,
                        size_t element_count,
                        size_t element_size,
                        __cxa_vec_destructor destructor);

    void __cxa_vec_cleanup(void* array_address,
                           size_t element_count,
                           size_t element_size,
                           __cxa_vec_destructor destructor);

    void __cxa_vec_delete(void*  array_address,
                          size_t element_size,
                          size_t padding_size,
                          __cxa_vec_destructor destructor);

    void __cxa_vec_delete2(void* array_address,
                           size_t element_size,
                           size_t padding_size,
                           __cxa_vec_destructor destructor,
                           void  (*dealloc)(void*));

    void __cxa_vec_delete3(void* array_address,
                           size_t element_size,
                           size_t padding_size,
                           __cxa_vec_destructor destructor,
                           void  (*dealloc) (void*, size_t));

    __cxa_vec_ctor_return_type
    __cxa_vec_cctor(void*  dest_array,
                    void*  src_array,
                    size_t element_count,
                    size_t element_size,
                    __cxa_vec_copy_constructor constructor,
                    __cxa_vec_destructor destructor );

  } // extern "C"

} // namespace __cxxabiv1

namespace abi = __cxxabiv1;

#if _GABIXX_ARM_ABI
// ARM-specific ABI additions. They  must be provided by the
// C++ runtime to simplify calling code generated by the compiler.
// Note that neither GCC nor Clang seem to use these, but this can
// happen when using machine code generated with other ocmpilers
// like RCVT.

namespace __aeabiv1 {
extern "C" {

using __cxxabiv1::__cxa_vec_constructor;
using __cxxabiv1::__cxa_vec_copy_constructor;
using __cxxabiv1::__cxa_vec_destructor;

void* __aeabi_vec_ctor_nocookie_nodtor(void* array_address,
                                       __cxa_vec_constructor constructor,
                                       size_t element_size,
                                       size_t element_count);

void* __aeabi_vec_ctor_cookie_nodtor(void* array_address,
                                     __cxa_vec_constructor constructor,
                                     size_t element_size,
                                     size_t element_count);

void* __aeabi_vec_cctor_nocookie_nodtor(
    void* dst_array,
    void* src_array,
    size_t element_size,
    size_t element_count,
    __cxa_vec_copy_constructor constructor);

void* __aeabi_vec_new_nocookie_noctor(size_t element_size,
                                      size_t element_count);

void* __aeabi_vec_new_nocookie(size_t element_size,
                               size_t element_count,
                               __cxa_vec_constructor constructor);

void* __aeabi_vec_new_cookie_nodtor(size_t element_size,
                                    size_t element_count,
                                    __cxa_vec_constructor constructor);

void* __aeabi_vec_new_cookie(size_t element_size,
                             size_t element_count,
                             __cxa_vec_constructor constructor,
                             __cxa_vec_destructor destructor);

void* __aeabi_vec_dtor(void* array_address,
                       __cxa_vec_destructor destructor,
                       size_t element_size,
                       size_t element_count);
  
void* __aeabi_vec_dtor_cookie(void* array_address,
                              __cxa_vec_destructor destructor);

void __aeabi_vec_delete(void* array_address,
                        __cxa_vec_destructor destructor);

void __aeabi_vec_delete3(void* array_address,
                         __cxa_vec_destructor destructor,
                         void (*dealloc)(void*, size_t));

void __aeabi_vec_delete3_nodtor(void* array_address,
                                void (*dealloc)(void*, size_t));

}  // extern "C"
}  // namespace __

#endif  // _GABIXX_ARM_ABI == 1

#endif /* defined(__GABIXX_CXXABI_H__) */

