// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/policy_pref_names.h"

namespace policy {
namespace policy_prefs {

// 64-bit serialization of the time last policy usage statistics were collected
// by UMA_HISTOGRAM_ENUMERATION.
const char kLastPolicyStatisticsUpdate[] = "policy.last_statistics_update";

// Blocks access to the listed host patterns.
const char kUrlBlacklist[] = "policy.url_blacklist";

// Allows access to the listed host patterns, as exceptions to the blacklist.
const char kUrlWhitelist[] = "policy.url_whitelist";

// Integer that specifies the policy refresh rate for user-policy in
// milliseconds. Not all values are meaningful, so it is clamped to a sane range
// by the cloud policy subsystem.
const char kUserPolicyRefreshRate[] = "policy.user_refresh_rate";

// The enrollment token of machine level user cloud policy
const char kMachineLevelUserCloudPolicyEnrollmentToken[] =
    "policy.machine_level_user_cloud_policy_enrollment_token";

}  // namespace policy_prefs
}  // namespace policy
