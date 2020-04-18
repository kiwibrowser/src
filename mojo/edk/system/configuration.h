// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_CONFIGURATION_H_
#define MOJO_EDK_SYSTEM_CONFIGURATION_H_

#include "mojo/edk/embedder/configuration.h"
#include "mojo/edk/system/system_impl_export.h"

namespace mojo {
namespace edk {

namespace internal {
MOJO_SYSTEM_IMPL_EXPORT extern Configuration g_configuration;
}  // namespace internal

MOJO_SYSTEM_IMPL_EXPORT inline const Configuration& GetConfiguration() {
  return internal::g_configuration;
}

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_CONFIGURATION_H_
