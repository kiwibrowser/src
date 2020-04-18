// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/embedder.h"

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_runner.h"
#include "mojo/edk/embedder/entrypoints.h"
#include "mojo/edk/system/configuration.h"
#include "mojo/edk/system/core.h"
#include "mojo/edk/system/node_controller.h"
#include "mojo/public/c/system/thunks.h"

#if !defined(OS_NACL)
#include "crypto/random.h"
#endif

namespace mojo {
namespace edk {

void Init(const Configuration& configuration) {
  internal::g_configuration = configuration;
  InitializeCore();
  MojoEmbedderSetSystemThunks(&GetSystemThunks());
}

void Init() {
  Init(Configuration());
}

void SetDefaultProcessErrorCallback(const ProcessErrorCallback& callback) {
  Core::Get()->SetDefaultProcessErrorCallback(callback);
}

std::string GenerateRandomToken() {
  char random_bytes[16];
#if defined(OS_NACL)
  // Not secure. For NaCl only!
  base::RandBytes(random_bytes, 16);
#else
  crypto::RandBytes(random_bytes, 16);
#endif
  return base::HexEncode(random_bytes, 16);
}

MojoResult CreateInternalPlatformHandleWrapper(
    ScopedInternalPlatformHandle platform_handle,
    MojoHandle* platform_handle_wrapper_handle) {
  return Core::Get()->CreateInternalPlatformHandleWrapper(
      std::move(platform_handle), platform_handle_wrapper_handle);
}

MojoResult PassWrappedInternalPlatformHandle(
    MojoHandle platform_handle_wrapper_handle,
    ScopedInternalPlatformHandle* platform_handle) {
  return Core::Get()->PassWrappedInternalPlatformHandle(
      platform_handle_wrapper_handle, platform_handle);
}

scoped_refptr<base::TaskRunner> GetIOTaskRunner() {
  return Core::Get()->GetNodeController()->io_task_runner();
}

#if defined(OS_MACOSX) && !defined(OS_IOS)
void SetMachPortProvider(base::PortProvider* port_provider) {
  DCHECK(port_provider);
  Core::Get()->SetMachPortProvider(port_provider);
}
#endif

}  // namespace edk
}  // namespace mojo
