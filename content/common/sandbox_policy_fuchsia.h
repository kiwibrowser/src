// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SANDBOX_POLICY_FUCHSIA_H_
#define CONTENT_COMMON_SANDBOX_POLICY_FUCHSIA_H_

#include "services/service_manager/sandbox/sandbox_type.h"

namespace base {
struct LaunchOptions;
}  // namespace base

namespace content {

// Modifies the process launch |options| to achieve the level of isolation
// appropriate for the sandbox |type|. The caller may then add any
// descriptors or handles afterward to grant additional capabiltiies to the new
// process.
void UpdateLaunchOptionsForSandbox(service_manager::SandboxType type,
                                   base::LaunchOptions* options);

}  // namespace content

#endif  // CONTENT_COMMON_SANDBOX_POLICY_FUCHSIA_H_
