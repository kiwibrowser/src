// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/disks/suspend_unmount_manager.h"

#include "base/bind.h"
#include "base/location.h"
#include "chromeos/disks/disk_mount_manager.h"

namespace chromeos {
namespace disks {
namespace {

void OnRefreshCompleted(bool success) {}

}  // namespace

SuspendUnmountManager::SuspendUnmountManager(
    DiskMountManager* disk_mount_manager,
    PowerManagerClient* power_manager_client)
    : disk_mount_manager_(disk_mount_manager),
      power_manager_client_(power_manager_client),
      weak_ptr_factory_(this) {
  power_manager_client_->AddObserver(this);
}

SuspendUnmountManager::~SuspendUnmountManager() {
  power_manager_client_->RemoveObserver(this);
  if (!suspend_readiness_callback_.is_null())
    suspend_readiness_callback_.Run();
}

void SuspendUnmountManager::SuspendImminent(
    power_manager::SuspendImminent::Reason reason) {
  DCHECK(unmounting_paths_.empty());
  if (!unmounting_paths_.empty())
    return;
  std::set<std::string> mount_paths;
  for (const auto& pair : disk_mount_manager_->disks()) {
    if ((pair.second->device_type() == DEVICE_TYPE_USB ||
         pair.second->device_type() == DEVICE_TYPE_SD) &&
        !pair.second->mount_path().empty()) {
      mount_paths.insert(pair.second->mount_path());
    }
  }
  for (const auto& mount_path : mount_paths) {
    if (suspend_readiness_callback_.is_null()) {
      suspend_readiness_callback_ =
          power_manager_client_->GetSuspendReadinessCallback(FROM_HERE);
    }
    disk_mount_manager_->UnmountPath(
        mount_path, UNMOUNT_OPTIONS_NONE,
        base::Bind(&SuspendUnmountManager::OnUnmountComplete,
                   weak_ptr_factory_.GetWeakPtr(), mount_path));
    unmounting_paths_.insert(mount_path);
  }
}

void SuspendUnmountManager::SuspendDone(const base::TimeDelta& sleep_duration) {
  // SuspendDone can be called before OnUnmountComplete when suspend is
  // cancelled, or it takes long time to unmount volumes.
  unmounting_paths_.clear();
  disk_mount_manager_->EnsureMountInfoRefreshed(base::Bind(&OnRefreshCompleted),
                                                true /* force */);
  suspend_readiness_callback_.Reset();
}

void SuspendUnmountManager::OnUnmountComplete(const std::string& mount_path,
                                              chromeos::MountError error_code) {
  // This can happen when unmount completes after suspend done is called.
  if (unmounting_paths_.erase(mount_path) != 1)
    return;
  if (unmounting_paths_.empty() && !suspend_readiness_callback_.is_null()) {
    suspend_readiness_callback_.Run();
    suspend_readiness_callback_.Reset();
  }
}

}  // namespace disks
}  // namespace chromeos
