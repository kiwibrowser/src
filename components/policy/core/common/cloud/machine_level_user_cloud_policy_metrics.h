// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_CLOUD_MACHINE_LEVEL_USER_CLOUD_POLICY_METRICS_H_
#define COMPONENTS_POLICY_CORE_COMMON_CLOUD_MACHINE_LEVEL_USER_CLOUD_POLICY_METRICS_H_

namespace policy {

// This enum is used for recording the metrics. It must match the
// MachineLevelUserCloudPolicyEnrollmentResult in enums.xml and should not be
// reordered. |kMaxValue| must be assigned to the last entry of the enum.
enum MachineLevelUserCloudPolicyEnrollmentResult {
  kSuccess = 0,
  kFailedToFetch = 1,
  kFailedToStore = 2,
  kMaxValue = kFailedToStore,
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_CLOUD_MACHINE_LEVEL_USER_CLOUD_POLICY_METRICS_H_
