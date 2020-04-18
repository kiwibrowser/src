// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_NETWORK_OBSERVER_H_
#define CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_NETWORK_OBSERVER_H_

#include "base/bind.h"
#include "base/macros.h"
#include "content/browser/background_sync/background_sync.pb.h"
#include "content/common/content_export.h"
#include "net/base/network_change_notifier.h"

namespace content {

class CONTENT_EXPORT BackgroundSyncNetworkObserver
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  // Creates a BackgroundSyncNetworkObserver. |network_changed_callback| is
  // called when the network connection changes asynchronously via PostMessage.
  BackgroundSyncNetworkObserver(
      const base::RepeatingClosure& network_changed_callback);

  ~BackgroundSyncNetworkObserver() override;

  // Enable or disable notifications coming from the NetworkChangeNotifier. (For
  // preventing flakes in tests)
  static void SetIgnoreNetworkChangeNotifierForTests(bool ignore);

  // Returns true if the state of the network meets the needs of
  // |network_state|.
  bool NetworkSufficient(SyncNetworkState network_state);

  // NetworkChangeObserver overrides
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType connection_type) override;

  // Allow tests to call NotifyManagerIfNetworkChanged.
  void NotifyManagerIfNetworkChangedForTesting(
      net::NetworkChangeNotifier::ConnectionType connection_type);

 private:
  // Calls NotifyNetworkChanged if the connection type has changed.
  void NotifyManagerIfNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType connection_type);

  void NotifyNetworkChanged();

  net::NetworkChangeNotifier::ConnectionType connection_type_;

  // The callback to run when the network changes.
  base::RepeatingClosure network_changed_callback_;

  // Set true to ignore notifications coming from the NetworkChangeNotifier
  // (to prevent flakes in tests).
  static bool ignore_network_change_notifier_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundSyncNetworkObserver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_NETWORK_OBSERVER_H_
