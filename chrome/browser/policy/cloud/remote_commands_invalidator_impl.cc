// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/cloud/remote_commands_invalidator_impl.h"

#include "base/logging.h"
#include "components/policy/core/common/remote_commands/remote_commands_service.h"

namespace policy {

RemoteCommandsInvalidatorImpl::RemoteCommandsInvalidatorImpl(
    CloudPolicyCore* core)
    : core_(core) {
  DCHECK(core_);
}

void RemoteCommandsInvalidatorImpl::OnInitialize() {
  core_->AddObserver(this);
  if (core_->remote_commands_service())
    OnRemoteCommandsServiceStarted(core_);
}

void RemoteCommandsInvalidatorImpl::OnShutdown() {
  core_->RemoveObserver(this);
}

void RemoteCommandsInvalidatorImpl::OnStart() {
  core_->store()->AddObserver(this);
  OnStoreLoaded(core_->store());
}

void RemoteCommandsInvalidatorImpl::OnStop() {
  core_->store()->RemoveObserver(this);
}

void RemoteCommandsInvalidatorImpl::DoRemoteCommandsFetch() {
  DCHECK(core_->remote_commands_service());
  core_->remote_commands_service()->FetchRemoteCommands();
}

void RemoteCommandsInvalidatorImpl::OnCoreConnected(CloudPolicyCore* core) {
}

void RemoteCommandsInvalidatorImpl::OnRefreshSchedulerStarted(
    CloudPolicyCore* core) {
}

void RemoteCommandsInvalidatorImpl::OnCoreDisconnecting(CloudPolicyCore* core) {
  Stop();
}

void RemoteCommandsInvalidatorImpl::OnRemoteCommandsServiceStarted(
    CloudPolicyCore* core) {
  Start();
}

void RemoteCommandsInvalidatorImpl::OnStoreLoaded(CloudPolicyStore* core) {
  ReloadPolicyData(core_->store()->policy());
}

void RemoteCommandsInvalidatorImpl::OnStoreError(CloudPolicyStore* core) {
}

}  // namespace policy
