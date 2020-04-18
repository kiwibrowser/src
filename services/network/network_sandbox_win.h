// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_NETWORK_SANDBOX_WIN_H_
#define SERVICES_NETWORK_NETWORK_SANDBOX_WIN_H_

#include "base/component_export.h"
#include "sandbox/win/src/sandbox_policy_base.h"
#include "services/service_manager/sandbox/win/sandbox_win.h"

// These sandbox-config extension functions should be called from
// UtilitySandboxedProcessLauncherDelegate on Windows (or the appropriate
// Delegate if SANDBOX_TYPE_NETWORK is removed from SANDBOX_TYPE_UTILITY).
//
// NOTE: changes to this code need to be reviewed by the security team.

namespace network {

// PreSpawnTarget extension.
COMPONENT_EXPORT(NETWORK_SERVICE)
bool NetworkPreSpawnTarget(sandbox::TargetPolicy* policy);

}  // namespace network

#endif  // SERVICES_NETWORK_NETWORK_SANDBOX_WIN_H_
