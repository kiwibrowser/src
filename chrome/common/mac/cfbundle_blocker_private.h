// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_MAC_CFBUNDLE_BLOCKER_PRIVATE_H_
#define CHROME_COMMON_MAC_CFBUNDLE_BLOCKER_PRIVATE_H_

#include <CoreFoundation/CoreFoundation.h>

extern "C" {

// _CFBundleLoadExecutableAndReturnError is the internal implementation that
// results in a dylib being loaded via dlopen. Both CFBundleLoadExecutable and
// CFBundleLoadExecutableAndReturnError are funneled into this routine. Other
// CFBundle functions may also call directly into here, perhaps due to
// inlining their calls to CFBundleLoadExecutable.
//
// See CF-476.19/CFBundle.c (10.5.8), CF-550.43/CFBundle.c (10.6.8), and
// CF-635/Bundle.c (10.7.0) and the disassembly of the shipping object code.
//
// Because this is a private function not declared by
// <CoreFoundation/CoreFoundation.h>, provide a declaration here.
Boolean _CFBundleLoadExecutableAndReturnError(CFBundleRef bundle,
                                              Boolean force_global,
                                              CFErrorRef* error);

}  // extern "C"

// These are internal declarations that are shared between the implementation
// and the unit tests only.
namespace chrome {
namespace common {
namespace mac {

// Returns true if |bundle_id| and |version| identify a bundle that is allowed
// to be loaded even when found in a blocked directory.
//
// Exposed only for testing. Do not call from outside the implementation.
bool IsBundleAllowed(NSString* bundle_id, NSString* version);

typedef Boolean (*_CFBundleLoadExecutableAndReturnError_Type)(CFBundleRef,
                                                              Boolean,
                                                              CFErrorRef*);

// A function pointer that allows calling the original intercepted
// implementation. The unit tests use it to restore back the functionality.
// It is NULL until the blocking (intercepting) starts.
extern _CFBundleLoadExecutableAndReturnError_Type
    g_original_underscore_cfbundle_load_executable_and_return_error;

}  // namespace mac
}  // namespace common
}  // namespace chrome

#endif  // CHROME_COMMON_MAC_CFBUNDLE_BLOCKER_PRIVATE_H_
