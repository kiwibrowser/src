// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "chrome/installer/mini_installer/mini_installer.h"

// http://blogs.msdn.com/oldnewthing/archive/2004/10/25/247180.aspx
extern "C" IMAGE_DOS_HEADER __ImageBase;

extern "C" int __stdcall MainEntryPoint() {
  mini_installer::ProcessExitResult result =
      mini_installer::WMain(reinterpret_cast<HMODULE>(&__ImageBase));
  ::ExitProcess(result.exit_code);
}

#if defined(ADDRESS_SANITIZER)
// Executables instrumented with ASAN need CRT functions. We do not use
// the /ENTRY switch for ASAN instrumented executable and a "main" function
// is required.
extern "C" int WINAPI wWinMain(HINSTANCE /* instance */,
                               HINSTANCE /* previous_instance */,
                               LPWSTR /* command_line */,
                               int /* command_show */) {
  return MainEntryPoint();
}
#endif

// VC Express editions don't come with the memset CRT obj file and linking to
// the obj files between versions becomes a bit problematic. Therefore,
// simply implement memset.
//
// This also avoids having to explicitly set the __sse2_available hack when
// linking with both the x64 and x86 obj files which is required when not
// linking with the std C lib in certain instances (including Chromium) with
// MSVC.  __sse2_available determines whether to use SSE2 intructions with
// std C lib routines, and is set by MSVC's std C lib implementation normally.
extern "C" {
#ifdef __clang__
// Marking memset as used is necessary in order to link with LLVM link-time
// optimization (LTO). It prevents LTO from discarding the memset symbol,
// allowing for compiler-generated references to memset to be satisfied.
__attribute__((used))
#else
// MSVC only allows declaring an intrinsic function if it's marked
// as `pragma function` first. `pragma function` also means that calls
// to memset must not use the intrinsic; we don't care abou this second
// (and main) meaning of the pragma.
// clang-cl doesn't implement this pragma at all, so don't use it there.
#pragma function(memset)
#endif
void* memset(void* dest, int c, size_t count) {
  uint8_t* scan = reinterpret_cast<uint8_t*>(dest);
  while (count--)
    *scan++ = static_cast<uint8_t>(c);
  return dest;
}
}  // extern "C"
