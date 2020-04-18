// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_CLOUD_CLOUD_POLICY_UTIL_H_
#define COMPONENTS_POLICY_CORE_COMMON_CLOUD_CLOUD_POLICY_UTIL_H_

#include <string>

#include "components/policy/policy_export.h"

namespace policy {

// Returns the name of the machine. This function is platform specific.
POLICY_EXPORT std::string GetMachineName();

// Returns the OS version of the machine. This function is platform specific.
POLICY_EXPORT std::string GetOSVersion();

// Returns the OS platform of the machine. This function is platform specific.
POLICY_EXPORT std::string GetOSPlatform();

// Returns the bitness of the OS. This function is platform specific.
POLICY_EXPORT std::string GetOSArchitecture();

// Returns the username of the logged in user in the OS. This function is
// platform specific.
POLICY_EXPORT std::string GetOSUsername();

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_CLOUD_CLOUD_POLICY_UTIL_H_
