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

#include <gabixx_config.h>
#include <stdlib.h>
#include <new>

using std::new_handler;
namespace {
  new_handler cur_handler;
}

namespace std {

#if !defined(LIBCXXABI)
  const nothrow_t nothrow = {};
#endif

  bad_alloc::bad_alloc() _GABIXX_NOEXCEPT {
  }

  bad_alloc::~bad_alloc() _GABIXX_NOEXCEPT {
  }

  const char* bad_alloc::what() const _GABIXX_NOEXCEPT {
    return "std::bad_alloc";
  }

  bad_array_new_length::bad_array_new_length() _GABIXX_NOEXCEPT
  {
  }

  bad_array_new_length::~bad_array_new_length() _GABIXX_NOEXCEPT
  {
  }

  const char*
  bad_array_new_length::what() const _GABIXX_NOEXCEPT
  {
    return "bad_array_new_length";
  }

#if __cplusplus > 201103L
// C++14 stuff
  bad_array_length::bad_array_length() _GABIXX_NOEXCEPT
  {
  }

  bad_array_length::~bad_array_length() _GABIXX_NOEXCEPT
  {
  }

  const char*
  bad_array_length::what() const _GABIXX_NOEXCEPT
  {
    return "bad_array_length";
  }
#endif

  new_handler set_new_handler(new_handler next_handler) _GABIXX_NOEXCEPT {
    return __gabixx_sync_swap(&cur_handler, next_handler);
  }

  new_handler get_new_handler() _GABIXX_NOEXCEPT {
    return __gabixx_sync_load(&cur_handler);
  }

} // namespace std

_GABIXX_WEAK
void* operator new(std::size_t size) throw(std::bad_alloc) {
  void* space;
  do {
    space = malloc(size);
    if (space) {
      return space;
    }
    new_handler handler = std::get_new_handler();
    if (handler == NULL) {
      throw std::bad_alloc();
    }
    handler();
  } while (space == 0);
  __builtin_unreachable();
}

_GABIXX_WEAK
void* operator new(std::size_t size, const std::nothrow_t& no)
    _GABIXX_NOEXCEPT {
  try {
    return ::operator new(size);
  } catch (const std::bad_alloc&) {
    return 0;
  }
}

_GABIXX_WEAK
void* operator new[](std::size_t size) throw(std::bad_alloc) {
  return ::operator new(size);
}

_GABIXX_WEAK
void* operator new[](std::size_t size, const std::nothrow_t& no)
    _GABIXX_NOEXCEPT {
  return ::operator new(size, no);
}

