// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_C_MAIN_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_C_MAIN_H_

#include "mojo/public/c/system/types.h"

// Implement ServiceMain directly as the entry point for a service.
//
// MojoResult ServiceMain(MojoHandle service_request) {
//   ...
// }
//

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(WIN32)
__declspec(dllexport) MojoResult
    __cdecl ServiceMain(MojoHandle application_request);
#else  // !defined(WIN32)
__attribute__((visibility("default"))) MojoResult
    ServiceMain(MojoHandle service_provider_handle);
#endif  // defined(WIN32)

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_C_MAIN_H_
