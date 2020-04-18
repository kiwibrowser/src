// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_SYSTEM_LINKER_H
#define CRAZY_LINKER_SYSTEM_LINKER_H

#include <dlfcn.h>

namespace crazy {

// Convenience wrapper for the system linker functions.
// Also helps synchronize access to the global link map list.
//
// TODO(digit): Use this in the future to mock different versions/behaviours
// of the Android system linker for unit-testing purposes.
struct SystemLinker {
  // Wrapper for dlopen().
  static void* Open(const char* path, int flags);

  // Wrapper for dlclose().
  static int Close(void* handle);

  // Wrapper for dlsym().
  static void* Resolve(void* handle, const char* symbol);

  // Wrapper for dlerror().
  static const char* Error();

  // Wrapper for dladdr();
  static int AddressInfo(void* addr, Dl_info* info);
};

}  // namespace crazy

#endif  // CRAZY_LINKER_SYSTEM_LINKER_H
