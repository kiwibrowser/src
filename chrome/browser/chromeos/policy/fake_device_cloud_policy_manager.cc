// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/fake_device_cloud_policy_manager.h"

#include <utility>

#include "base/callback.h"
#include "chrome/browser/chromeos/policy/device_cloud_policy_store_chromeos.h"

namespace policy {

FakeDeviceCloudPolicyManager::FakeDeviceCloudPolicyManager(
    std::unique_ptr<DeviceCloudPolicyStoreChromeOS> store,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : DeviceCloudPolicyManagerChromeOS(std::move(store), task_runner, NULL),
      unregister_result_(true) {}

FakeDeviceCloudPolicyManager::~FakeDeviceCloudPolicyManager() {
  Shutdown();
}

void FakeDeviceCloudPolicyManager::Unregister(
    const UnregisterCallback& callback) {
  callback.Run(unregister_result_);
}

void FakeDeviceCloudPolicyManager::Disconnect() {
}

}  // namespace policy
