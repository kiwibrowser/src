// Copyright (C) 2012 The Android Open Source Project
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

#include <exception>
#include <typeinfo>

#include "cxxabi_defines.h"

namespace std {

#if !defined(LIBCXXABI)
exception::exception() _GABIXX_NOEXCEPT {
}
#endif // !defined(LIBCXXABI)

exception::~exception() _GABIXX_NOEXCEPT {
}

const char* exception::what() const _GABIXX_NOEXCEPT {
  return "std::exception";
}

#if !defined(LIBCXXABI)
bad_exception::bad_exception() _GABIXX_NOEXCEPT {
}
#endif // !defined(LIBCXXABI)

bad_exception::~bad_exception() _GABIXX_NOEXCEPT {
}

const char* bad_exception::what() const _GABIXX_NOEXCEPT {
  return "std::bad_exception";
}

#if !defined(LIBCXXABI)
bool uncaught_exception() _GABIXX_NOEXCEPT {
  using namespace __cxxabiv1;

  __cxa_eh_globals* globals = __cxa_get_globals();
  return globals->uncaughtExceptions != 0;
}
#endif // !defined(LIBCXXABI)

}  // namespace std
