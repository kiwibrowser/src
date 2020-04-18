// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_STATE_HANDLER_OBSERVER_H_
#define CHROMEOS_NETWORK_NETWORK_STATE_HANDLER_OBSERVER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "chromeos/chromeos_export.h"

namespace chromeos {

class DeviceState;
class NetworkState;

// Observer class for all network state changes, including changes to
// active (connecting or connected) services.
class CHROMEOS_EXPORT NetworkStateHandlerObserver {
 public:
  NetworkStateHandlerObserver();
  virtual ~NetworkStateHandlerObserver();

  // The list of networks changed.
  virtual void NetworkListChanged();

  // The list of devices changed. Use DevicePropertiesUpdated to be notified
  // when a Device property changes.
  virtual void DeviceListChanged();

  // The default network changed (includes VPNs) or one of its properties
  // changed. This won't be called if the WiFi signal strength property
  // changes. If interested in those events, use NetworkPropertiesUpdated()
  // below.
  // |network| will be NULL if there is no longer a default network.
  virtual void DefaultNetworkChanged(const NetworkState* network);

  // The connection state of |network| changed.
  virtual void NetworkConnectionStateChanged(const NetworkState* network);

  // One or more properties of |network| have been updated. Note: this will get
  // called in *addition* to NetworkConnectionStateChanged() when the
  // connection state property changes. Use this to track properties like
  // wifi strength.
  virtual void NetworkPropertiesUpdated(const NetworkState* network);

  // One or more properties of |device| have been updated.
  virtual void DevicePropertiesUpdated(const DeviceState* device);

  // A scan for has been requested.
  virtual void ScanRequested();

  // A scan for |device| completed.
  virtual void ScanCompleted(const DeviceState* device);

  // Called just before NetworkStateHandler is destroyed so that observers
  // can safely stop observing.
  virtual void OnShuttingDown();

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkStateHandlerObserver);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_STATE_HANDLER_OBSERVER_H_
