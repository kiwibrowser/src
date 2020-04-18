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

#include <cstdlib>
#include <exception>
#include "cxxabi_defines.h"

namespace {

std::terminate_handler current_terminate = __gabixx::__default_terminate;
std::unexpected_handler current_unexpected = __gabixx::__default_unexpected;

}  // namespace


namespace __gabixx {

// The default std::unexpected() implementation will delegate to
// std::terminate() so that the user-defined std::terminate() handler can
// get the chance to be invoked.
//
_GABIXX_NORETURN void __default_unexpected(void) {
  std::terminate();
}

// The default std::terminate() implementation will crash the process.
// This is done to help debugging, i.e.:
//   - When running the program in a debugger, it's trivial to get
//     a complete stack trace explaining the failure.
//
//   - Otherwise, the default SIGSEGV handler will generate a stack
//     trace in the log, that can later be processed with ndk-stack
//     and other tools.
//
//   - Finally, this also works when a custom SIGSEGV handler has been
//     installed. E.g. when using Google Breakpad, the termination will
//     be recorded in a Minidump, which contains a stack trace to be
//     later analyzed.
//
// The C++ specification states that the default std::terminate()
// handler is library-specific, even though most implementation simply
// choose to call abort() in this case.
//
_GABIXX_NORETURN void __default_terminate(void) {
  // The crash address is just a "magic" constant that can be used to
  // identify stack traces (like 0xdeadbaad is used when heap corruption
  // is detected in the C library). 'cab1' stands for "C++ ABI" :-)
  *(reinterpret_cast<char*>(0xdeadcab1)) = 0;

  // should not be here, but just in case.
  abort();
}

_GABIXX_NORETURN void __terminate(std::terminate_handler handler) {
  if (!handler)
    handler = __default_terminate;

#if _GABIXX_HAS_EXCEPTIONS
  try {
    (*handler)();
  } catch (...) {
    /* nothing */
  }
#else
  (*handler)();
#endif
  __default_terminate();
}

}  // namespace __gabixx

namespace std {

terminate_handler get_terminate() _GABIXX_NOEXCEPT {
  return __gabixx_sync_load(&current_terminate);
}

terminate_handler set_terminate(terminate_handler f) _GABIXX_NOEXCEPT {
  if (!f)
    f = __gabixx::__default_terminate;

  return __gabixx_sync_swap(&current_terminate, f);
}

_GABIXX_NORETURN void terminate() _GABIXX_NOEXCEPT_CXX11_ONLY {
  __gabixx::__terminate(std::get_terminate());
}

unexpected_handler get_unexpected() _GABIXX_NOEXCEPT {
  return __gabixx_sync_load(&current_unexpected);
}

unexpected_handler set_unexpected(unexpected_handler f) _GABIXX_NOEXCEPT {
  if (!f)
    f = __gabixx::__default_terminate;

  return __gabixx_sync_swap(&current_unexpected, f);
}

_GABIXX_NORETURN void unexpected() {
  unexpected_handler handler = std::get_unexpected();
  if (handler)
    (*handler)();

  // If the handler returns, then call terminate().
  terminate();
}

} // namespace std
