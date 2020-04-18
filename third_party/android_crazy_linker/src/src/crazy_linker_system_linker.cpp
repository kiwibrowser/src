// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_system_linker.h"

#include "crazy_linker_globals.h"

#include <dlfcn.h>

namespace crazy {

// static
void* SystemLinker::Open(const char* path, int mode) {
  // NOTE: The system linker will likely modify the global _r_debug link map
  // so ensure this doesn't conflict with other threads performing delayed
  // updates on it.
  ScopedLinkMapLocker locker;
  return ::dlopen(path, mode);
}

// static
int SystemLinker::Close(void* handle) {
  // Similarly, though unlikely, this operation may modify the global link map.
  ScopedLinkMapLocker locker;
  return ::dlclose(handle);
}

// static
void* SystemLinker::Resolve(void* handle, const char* symbol) {
  // Just in case the system linker performs lazy symbol resolution
  // that would modify the global link map.
  ScopedLinkMapLocker locker;
  return ::dlsym(handle, symbol);
}

// static
const char* SystemLinker::Error() {
  return ::dlerror();
}

int SystemLinker::AddressInfo(void* address, Dl_info* info) {
  ::dlerror();
  return ::dladdr(address, info);
}

}  // namespace crazy
