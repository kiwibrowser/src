// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/p2p/empty_network_manager.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/renderer/p2p/network_manager_uma.h"

namespace content {

EmptyNetworkManager::EmptyNetworkManager(rtc::NetworkManager* network_manager)
    : network_manager_(network_manager), weak_ptr_factory_(this) {
  DCHECK(network_manager);
  thread_checker_.DetachFromThread();
  set_enumeration_permission(ENUMERATION_BLOCKED);
  network_manager_->SignalNetworksChanged.connect(
      this, &EmptyNetworkManager::OnNetworksChanged);
}

EmptyNetworkManager::~EmptyNetworkManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void EmptyNetworkManager::StartUpdating() {
  DCHECK(thread_checker_.CalledOnValidThread());
  ++start_count_;
  network_manager_->StartUpdating();
}

void EmptyNetworkManager::StopUpdating() {
  DCHECK(thread_checker_.CalledOnValidThread());
  network_manager_->StopUpdating();
  --start_count_;
  DCHECK_GE(start_count_, 0);
}

void EmptyNetworkManager::GetNetworks(NetworkList* networks) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  networks->clear();
}

bool EmptyNetworkManager::GetDefaultLocalAddress(
    int family,
    rtc::IPAddress* ipaddress) const {
  return network_manager_->GetDefaultLocalAddress(family, ipaddress);
}

void EmptyNetworkManager::OnNetworksChanged() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!start_count_)
    return;

  if (!sent_first_update_)
    ReportIPPermissionStatus(PERMISSION_NOT_REQUESTED);

  sent_first_update_ = true;
  SignalNetworksChanged();
}

}  // namespace content
