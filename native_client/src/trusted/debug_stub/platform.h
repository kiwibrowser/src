/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// This module defines the interface for platform specific support
// functions, such as thread creation, logging, exception catching,
// etc...  This API is not expected to throw, and instead will return
// false on any function that can fail.  Since this is a collection
// of helpers functions there is expected to be only one platform object
// which can be retrieved with the static "Get" member.
#ifndef NATIVE_CLIENT_PORT_PLATFORM_H_
#define NATIVE_CLIENT_PORT_PLATFORM_H_ 1

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/debug_stub/transport.h"

struct NaClApp;

namespace port {

class IPlatform {
 public:
  //  Called to get or set process memory.
  //  NOTE:  These functions should change the protection of the underlying
  //    page if needed to provide access.  It should only return false if
  //    the page is not mapped into the debugged process.
  static bool GetMemory(uint64_t address, uint32_t length, void *dst);
  static bool SetMemory(struct NaClApp *nap, uint64_t address, uint32_t length,
                        void *src);
};

}  // namespace port


#endif  // NATIVE_CLIENT_PORT_PLATFORM_H_

