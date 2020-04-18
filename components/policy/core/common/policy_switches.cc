// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/policy_switches.h"

namespace policy {
namespace switches {

// Specifies the URL at which to fetch configuration policy from the device
// management backend.
const char kDeviceManagementUrl[]           = "device-management-url";

// Disables fetching and storing cloud policy for components.
const char kDisableComponentCloudPolicy[]   = "disable-component-cloud-policy";

// Always treat user as affiliated.
// TODO(antrim): Remove once test servers correctly produce affiliation ids.
const char kUserAlwaysAffiliated[]  = "user-always-affiliated";

}  // namespace switches
}  // namespace policy
