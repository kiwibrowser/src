// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_sync/background_sync_network_observer.h"

#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"

namespace content {

// static
bool BackgroundSyncNetworkObserver::ignore_network_change_notifier_ = false;

// static
void BackgroundSyncNetworkObserver::SetIgnoreNetworkChangeNotifierForTests(
    bool ignore) {
  ignore_network_change_notifier_ = ignore;
}

BackgroundSyncNetworkObserver::BackgroundSyncNetworkObserver(
    const base::RepeatingClosure& network_changed_callback)
    : connection_type_(net::NetworkChangeNotifier::GetConnectionType()),
      network_changed_callback_(network_changed_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

BackgroundSyncNetworkObserver::~BackgroundSyncNetworkObserver() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

bool BackgroundSyncNetworkObserver::NetworkSufficient(
    SyncNetworkState network_state) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  switch (network_state) {
    case NETWORK_STATE_ANY:
      return true;
    case NETWORK_STATE_AVOID_CELLULAR:
      // Note that this returns true for CONNECTION_UNKNOWN to avoid never
      // firing.
      return connection_type_ != net::NetworkChangeNotifier::CONNECTION_NONE &&
             !net::NetworkChangeNotifier::IsConnectionCellular(
                 connection_type_);
    case NETWORK_STATE_ONLINE:
      return connection_type_ != net::NetworkChangeNotifier::CONNECTION_NONE;
  }

  NOTREACHED();
  return false;
}

void BackgroundSyncNetworkObserver::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType connection_type) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (ignore_network_change_notifier_)
    return;
  NotifyManagerIfNetworkChanged(connection_type);
}

void BackgroundSyncNetworkObserver::NotifyManagerIfNetworkChangedForTesting(
    net::NetworkChangeNotifier::ConnectionType connection_type) {
  NotifyManagerIfNetworkChanged(connection_type);
}

void BackgroundSyncNetworkObserver::NotifyManagerIfNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType connection_type) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (connection_type == connection_type_)
    return;

  connection_type_ = connection_type;
  NotifyNetworkChanged();
}

void BackgroundSyncNetworkObserver::NotifyNetworkChanged() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                network_changed_callback_);
}

}  // namespace content
