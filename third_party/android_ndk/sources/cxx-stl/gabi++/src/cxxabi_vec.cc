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

#include <cstddef>
#include <new>

#include "cxxabi_defines.h"

using std::size_t;

namespace {

using namespace __cxxabiv1;

typedef __cxa_vec_constructor constructor_func;
typedef __cxa_vec_copy_constructor copy_constructor_func;
typedef __cxa_vec_destructor destructor_func;
typedef void* (*alloc_func)(size_t);
typedef void (*dealloc_func)(void*);
typedef void (*dealloc2_func)(void*, size_t);

// Helper class to ensure a ptr is deallocated on scope exit unless
// the release() method has been called.
class scoped_block {
public:
  scoped_block(void* ptr, size_t size, dealloc2_func dealloc)
    : ptr_(ptr), size_(size), dealloc_(dealloc) {}

  ~scoped_block() {
    if (dealloc_)
      dealloc_(ptr_, size_);
  }

  void release() {
    dealloc_ = 0;
  }

private:
  void* ptr_;
  size_t size_;
  dealloc2_func dealloc_;
};

// Helper class to ensure a vector is cleaned up on scope exit
// unless the release() method has been called.
class scoped_cleanup {
public:
  scoped_cleanup(void* array, size_t& index, size_t element_size,
                destructor_func destructor)
    : array_(array), index_(index), element_size_(element_size),
      destructor_(destructor) {}

  ~scoped_cleanup() {
    if (destructor_)
      __cxxabiv1::__cxa_vec_cleanup(array_,
                                    index_,
                                    element_size_,
                                    destructor_);
  }

  void release() {
    destructor_ = 0;
  }
private:
  void* array_;
  size_t& index_;
  size_t element_size_;
  destructor_func destructor_;
};

// Helper class that calls __fatal_error() with a given message if
// it exits a scope without a previous call to release().
class scoped_catcher {
public:
  scoped_catcher(const char* message) : message_(message) {}

  ~scoped_catcher() {
    if (message_)
      __gabixx::__fatal_error(message_);
  }

  void release() {
    message_ = 0;
  }

private:
  const char* message_;
};

}  // namespace

namespace __cxxabiv1 {
extern "C" {

void* __cxa_vec_new(size_t element_count,
                    size_t element_size,
                    size_t padding_size,
                    constructor_func constructor,
                    destructor_func destructor) {
  return __cxa_vec_new2(element_count, element_size, padding_size,
                        constructor, destructor,
                        &operator new[], &operator delete []);
}

void* __cxa_vec_new2(size_t element_count,
                     size_t element_size,
                     size_t padding_size,
                     constructor_func constructor,
                     destructor_func destructor,
                     alloc_func alloc,
                     dealloc_func dealloc) {
  // The only difference with __cxa_vec_new3 is the type of the
  // dealloc parameter. force a cast because on all supported
  // platforms, it is possible to call the dealloc function here
  // with two parameters. The second one will simply be ignored.
  return __cxa_vec_new3(element_count, element_size, padding_size,
                        constructor, destructor, alloc,
                        reinterpret_cast<dealloc2_func>(dealloc));
}

void* __cxa_vec_new3(size_t element_count,
                     size_t element_size,
                     size_t padding_size,
                     constructor_func constructor,
                     destructor_func destructor,
                     alloc_func alloc,
                     dealloc2_func dealloc) {
  // Compute the size of the needed memory block, and throw
  // std::bad_alloc() on overflow.
  bool overflow = false;
  size_t size = 0;
  if (element_size > 0 && element_count > size_t(-1) / element_size)
    overflow = true;
  else {
    size = element_count * element_size;
    if (size + padding_size < size)
      overflow = true;
  }
  if (overflow)
    throw std::bad_alloc();

  // Allocate memory. Do not throw if NULL is returned.
  char* base = static_cast<char*>(alloc(size));
  if (!base)
    return base;

  // Ensure the block is freed if construction throws.
  scoped_block block(base, size, dealloc);

  if (padding_size) {
    base += padding_size;
    reinterpret_cast<size_t*>(base)[-1] = element_count;
#ifdef __arm__
    // Required by the ARM C++ ABI.
    reinterpret_cast<size_t*>(base)[-2] = element_size;
#endif
  }

  __cxa_vec_ctor(base, element_count, element_size,
                 constructor, destructor);

  // Construction succeeded, no need to release the block.
  block.release();
  return base;
}

#ifdef __arm__
// On ARM, __cxa_vec_ctor and __cxa_vec_cctor must return
// their first parameter. Handle this here.
#define _CXA_VEC_CTOR_RETURN(x) return x
#else
#define _CXA_VEC_CTOR_RETURN(x) return
#endif

__cxa_vec_ctor_return_type
__cxa_vec_ctor(void* array_address,
               size_t element_count,
               size_t element_size,
               constructor_func constructor,
               destructor_func destructor) {
  if (constructor) {
    size_t n = 0;
    char* base = static_cast<char*>(array_address);

    scoped_cleanup cleanup(array_address, n, element_size, destructor);
    for (; n != element_count; ++n) {
      constructor(base);
      base += element_size;
    }
    cleanup.release();
  }
  _CXA_VEC_CTOR_RETURN(array_address);
}

// Given the (data) address of an array, the number of elements,
// and the size of its elements, call the given destructor on each
// element. If the destructor throws an exception, rethrow after
// destroying the remaining elements if possible. If the destructor
// throws a second exception, call terminate(). The destructor
// pointer may be NULL, in which case this routine does nothing.
void __cxa_vec_dtor(void* array_address,
                    size_t element_count,
                    size_t element_size,
                    destructor_func destructor) {
  if (!destructor)
    return;

  char* base = static_cast<char*>(array_address);
  size_t n = element_count;
  scoped_cleanup cleanup(array_address, n, element_size, destructor);
  base += element_count * element_size;
  // Note: n must be decremented before the destructor call
  // to avoid cleaning up one extra unwanted item.
  while (n--) {
    base -= element_size;
    destructor(base);
  }
  cleanup.release();
}

// Given the (data) address of an array, the number of elements,
// and the size of its elements, call the given destructor on each
// element. If the destructor throws an exception, call terminate().
// The destructor pointer may be NULL, in which case this routine
// does nothing.
void __cxa_vec_cleanup(void* array_address,
                       size_t element_count,
                       size_t element_size,
                       destructor_func destructor) {
  if (!destructor)
    return;

  char* base = static_cast<char*>(array_address);
  size_t n = element_count;
  base += n * element_size;

  scoped_catcher catcher("exception raised in vector destructor!");
  while (n--) {
    base -= element_size;
    destructor(base);
  }
  catcher.release();
}

// If the array_address is NULL, return immediately. Otherwise,
// given the (data) address of an array, the non-negative size
// of prefix padding for the cookie, and the size of its elements,
// call the given destructor on each element, using the cookie to
// determine the number of elements, and then delete the space by
// calling ::operator delete[](void *). If the destructor throws an
// exception, rethrow after (a) destroying the remaining elements,
// and (b) deallocating the storage. If the destructor throws a
// second exception, call terminate(). If padding_size is 0, the
// destructor pointer must be NULL. If the destructor pointer is NULL,
// no destructor call is to be made.
void __cxa_vec_delete(void* array_address,
                      size_t element_size,
                      size_t padding_size,
                      destructor_func destructor) {
  __cxa_vec_delete2(array_address, element_size, padding_size,
                    destructor, &operator delete []);
}

// Same as __cxa_vec_delete, except that the given function is used
// for deallocation instead of the default delete function. If dealloc
// throws an exception, the result is undefined. The dealloc pointer
// may not be NULL.
void __cxa_vec_delete2(void* array_address,
                       size_t element_size,
                       size_t padding_size,
                       destructor_func destructor,
                       dealloc_func dealloc) {
  // Same trick than the one used on __cxa_vec_new2.
  __cxa_vec_delete3(array_address, element_size, padding_size,
                    destructor,
                    reinterpret_cast<dealloc2_func>(dealloc));
}

// Same as __cxa_vec_delete, except that the given function is used
// for deallocation instead of the default delete function. The
// deallocation function takes both the object address and its size.
// If dealloc throws an exception, the result is undefined. The dealloc
// pointer may not be NULL.
void __cxa_vec_delete3(void* array_address,
                       size_t element_size,
                       size_t padding_size,
                       destructor_func destructor,
                       dealloc2_func dealloc) {
  if (!array_address)
    return;

  char* base = static_cast<char*>(array_address);

  if (!padding_size) {
    // If here is no padding size, asume the deallocator knows
    // how to handle this. Useful when called from __cxa_vec_delete2.
    dealloc(base, 0);
    return;
  }

  size_t element_count = reinterpret_cast<size_t*>(base)[-1];
  base -= padding_size;
  size_t size = element_count * element_size + padding_size;

  // Always deallocate base on exit.
  scoped_block block(base, size, dealloc);

  if (padding_size > 0 && destructor != 0)
    __cxa_vec_dtor(array_address, element_count, element_size, destructor);
}

__cxa_vec_ctor_return_type
__cxa_vec_cctor(void* dst_array,
                void* src_array,
                size_t element_count,
                size_t element_size,
                copy_constructor_func copy_constructor,
                destructor_func destructor) {
  if (copy_constructor) {
    size_t n = 0;
    char* dst = static_cast<char*>(dst_array);
    char* src = static_cast<char*>(src_array);

    scoped_cleanup cleanup(dst_array, n, element_size, destructor);

    for ( ; n != element_count; ++n) {
      copy_constructor(dst, src);
      dst += element_size;
      src += element_size;
    }

    cleanup.release();
  }
  _CXA_VEC_CTOR_RETURN(dst_array);
}

}  // extern "C"
}  // namespace __cxxabiv1

#if _GABIXX_ARM_ABI
// The following functions are required by the ARM ABI, even
// though neither GCC nor LLVM generate any code that uses it.
// This may be important for machine code generated by other
// compilers though (e.g. RCVT), which may depend on them.
// They're supposed to simplify calling code.
namespace __aeabiv1 {

extern "C" {

using namespace __cxxabiv1;

void* __aeabi_vec_ctor_nocookie_nodtor(void* array_address,
                                       constructor_func constructor,
                                       size_t element_size,
                                       size_t element_count) {
  return __cxa_vec_ctor(array_address,
                        element_count,
                        element_size,
                        constructor,
                        /* destructor */ NULL);
}

void* __aeabi_vec_ctor_cookie_nodtor(void* array_address,
                                     constructor_func constructor,
                                     size_t element_size,
                                     size_t element_count) {
  if (!array_address)
    return array_address;

  size_t* base = reinterpret_cast<size_t*>(array_address) + 2;
  base[-2] = element_size;
  base[-1] = element_count;
  return __cxa_vec_ctor(base,
                        element_count,
                        element_size,
                        constructor,
                        /* destructor */ NULL);
}

void* __aeabi_vec_cctor_nocookie_nodtor(
    void* dst_array,
    void* src_array,
    size_t element_size,
    size_t element_count,
    copy_constructor_func copy_constructor) {
  return __cxa_vec_cctor(dst_array, src_array, element_count,
                         element_size, copy_constructor, NULL);
}

void* __aeabi_vec_new_cookie_noctor(size_t element_size,
                                    size_t element_count) {
  return __cxa_vec_new(element_count, element_size,
                       /* padding */ 2 * sizeof(size_t),
                       /* constructor */ NULL,
                       /* destructor */ NULL);
}

void* __aeabi_vec_new_nocookie(size_t element_size,
                               size_t element_count,
                               constructor_func constructor) {
  return __cxa_vec_new(element_count,
                       element_size,
                       /* padding */ 0,
                       constructor,
                       /* destructor */ NULL);
}

void* __aeabi_vec_new_cookie_nodtor(size_t element_size,
                                    size_t element_count,
                                    constructor_func constructor) {
  return __cxa_vec_new(element_count,
                       element_size,
                       /* padding */ 2 * sizeof(size_t),
                       constructor,
                       /* destructor */ NULL);
}

void* __aeabi_vec_new_cookie(size_t element_size,
                             size_t element_count,
                             constructor_func constructor,
                             destructor_func destructor) {
  return __cxa_vec_new(element_count,
                       element_size,
                       /* padding */ 2 * sizeof(size_t),
                       constructor,
                       destructor);
}

void* __aeabi_vec_dtor(void* array_address,
                       destructor_func destructor,
                       size_t element_size,
                       size_t element_count) {
  __cxa_vec_dtor(array_address, element_count, element_size,
                 destructor);
    return reinterpret_cast<size_t*>(array_address) - 2;
}

void* __aeabi_vec_dtor_cookie(void* array_address,
                              destructor_func destructor) {
  if (!array_address)
    return NULL;

  size_t* base = reinterpret_cast<size_t*>(array_address);
  __cxa_vec_dtor(array_address,
                 /* element_count */ base[-1],
                 /* element_size */ base[-2],
                 destructor);
  return base - 2;
}

void __aeabi_vec_delete(void* array_address,
                        destructor_func destructor)  {
  if (array_address) {
    size_t* base = reinterpret_cast<size_t*>(array_address);

    __cxa_vec_delete(array_address,
                    /* element_size */ base[-2],
                    /* padding */ 2 * sizeof(size_t),
                    destructor);
  }
}

void __aeabi_vec_delete3(void* array_address,
                         destructor_func destructor,
                         dealloc2_func dealloc) {
  if (array_address) {
    size_t* base = reinterpret_cast<size_t*>(array_address);

    __cxa_vec_delete3(array_address,
                     /* element_size */ base[-2],
                     /* padding */ 2 * sizeof(size_t),
                     destructor,
                     dealloc);
  }
}

void __aeabi_vec_delete3_nodtor(void* array_address,
                                dealloc2_func dealloc) {
  if (array_address) {
    size_t* base = reinterpret_cast<size_t*>(array_address);

    __cxa_vec_delete3(array_address,
                     /* element_size */ base[-2],
                     /* padding */ 2 * sizeof(size_t),
                     /* destructor */ NULL,
                     dealloc);
  }
}

}  // extern "C"
}  // namespace __aeabiv1

#endif  // _GABIXX_ARM_ABI
