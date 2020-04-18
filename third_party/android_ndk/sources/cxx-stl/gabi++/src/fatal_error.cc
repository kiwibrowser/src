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

#include <android/log.h>
#include <dlfcn.h>
#include <stdio.h>

#include "cxxabi_defines.h"

namespace __gabixx {

_GABIXX_NORETURN void __fatal_error(const char* message) {

  // Note: Printing to stderr is only useful when running an executable
  // from a shell, e.g. when using 'adb shell'. For regular
  // applications, stderr is redirected to /dev/null by default.
  fprintf(stderr, "PANIC:GAbi++:%s\n", message);

  // Always print the message to the log, when possible. Use
  // dlopen()/dlsym() to avoid adding an explicit dependency
  // to -llog in GAbi++ for this sole feature.
  //
  // An explicit dependency to -ldl can be avoided because these
  // functions are implemented directly by the dynamic linker.
  // That is, except when this code is linked into a static
  // executable. In this case, adding -ldl to the final link command
  // will be necessary, but the dlopen() will always return NULL.
  //
  // There is unfortunately no way to detect where this code is going
  // to be used at compile time, but static executables are strongly
  // discouraged on the platform because they can't implement ASLR.
  //
  typedef void (*logfunc_t)(int, const char*, const char*);
  logfunc_t logger = NULL;

  // Note that this should always succeed in a regular application,
  // because the library is already loaded into the process' address
  // space by Zygote before forking the application process.
  // This will fail in static executables, because the static
  // version of -ldl only contains empty stubs.
  void* liblog = dlopen("liblog.so", RTLD_NOW);

  if (liblog != NULL) {
    logger = reinterpret_cast<logfunc_t>(dlsym(liblog, "__android_log_print"));
    if (logger != NULL) {
      (*logger)(ANDROID_LOG_FATAL, "GAbi++", message);
    }
    dlclose(liblog);
  }

  std::terminate();
}

}  // namespace __gabixx
