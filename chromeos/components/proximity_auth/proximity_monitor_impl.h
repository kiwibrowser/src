// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PROXIMITY_AUTH_PROXIMITY_MONITOR_IMPL_H_
#define CHROMEOS_COMPONENTS_PROXIMITY_AUTH_PROXIMITY_MONITOR_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chromeos/components/proximity_auth/proximity_monitor.h"
#include "components/cryptauth/connection.h"
#include "components/cryptauth/remote_device_ref.h"
#include "device/bluetooth/bluetooth_device.h"

namespace device {
class BluetoothAdapter;
}

namespace proximity_auth {

class ProximityAuthPrefManager;
class ProximityMonitorObserver;

// The concrete implemenation of the proximity monitor interface.
class ProximityMonitorImpl : public ProximityMonitor {
 public:
  // The |connection| is not owned, and must outlive |this| instance.
  ProximityMonitorImpl(cryptauth::Connection* connection,
                       ProximityAuthPrefManager* pref_manager);
  ~ProximityMonitorImpl() override;

  // ProximityMonitor:
  void Start() override;
  void Stop() override;
  bool IsUnlockAllowed() const override;
  void RecordProximityMetricsOnAuthSuccess() override;
  void AddObserver(ProximityMonitorObserver* observer) override;
  void RemoveObserver(ProximityMonitorObserver* observer) override;

 private:
  // Callback for asynchronous initialization of the Bluetooth adpater.
  void OnAdapterInitialized(scoped_refptr<device::BluetoothAdapter> adapter);

  // Ensures that the app is periodically polling for the proximity status
  // between the remote and the local device iff it should be, based on the
  // current app state.
  void UpdatePollingState();

  // Performs a scheduled |UpdatePollingState()| operation. This method is
  // used to distinguish periodically scheduled calls to |UpdatePollingState()|
  // from event-driven calls, which should be handled differently.
  void PerformScheduledUpdatePollingState();

  // Returns |true| iff the app should be periodically polling for the proximity
  // status between the remote and the local device.
  bool ShouldPoll() const;

  // Polls the connection information.
  void Poll();

  // Callback to received the polled-for connection info.
  void OnConnectionInfo(
      const device::BluetoothDevice::ConnectionInfo& connection_info);

  // Resets the proximity state to |false|, and clears all member variables
  // tracking the proximity state.
  void ClearProximityState();

  // Updates the proximity state with a new |connection_info| sample of the
  // current RSSI.
  void AddSample(
      const device::BluetoothDevice::ConnectionInfo& connection_info);

  // Checks whether the proximity state has changed based on the current
  // samples. Notifies |observers_| on a change.
  void CheckForProximityStateChange();

  // Gets the user-selected proximity threshold and converts it to a
  // RSSI value.
  void GetRssiThresholdFromPrefs();

  // The current connection being monitored. Not owned and must outlive this
  // instance.
  cryptauth::Connection* connection_;

  // The observers attached to the ProximityMonitor.
  base::ObserverList<ProximityMonitorObserver> observers_;

  // The Bluetooth adapter that will be polled for connection info.
  scoped_refptr<device::BluetoothAdapter> bluetooth_adapter_;

  // Whether the remote device is currently in close proximity to the local
  // device.
  bool remote_device_is_in_proximity_;

  // Whether the proximity monitor is active, i.e. should possibly be scanning
  // for proximity to the remote device.
  bool is_active_;

  // When the RSSI is below this value the phone the unlock is not allowed.
  int rssi_threshold_;

  // The exponentailly weighted rolling average of the RSSI, used to smooth the
  // RSSI readings. Null if the monitor is inactive, has not recently observed
  // an RSSI reading, or the most recent connection info included an invalid
  // measurement.
  std::unique_ptr<double> rssi_rolling_average_;

  // Contains perferences that outlive the lifetime of this object and across
  // process restarts. Not owned and must outlive this instance.
  ProximityAuthPrefManager* pref_manager_;

  // Used to vend weak pointers for polling. Using a separate factory for these
  // weak pointers allows the weak pointers to be invalidated when polling
  // stops, which effectively cancels the scheduled tasks.
  base::WeakPtrFactory<ProximityMonitorImpl> polling_weak_ptr_factory_;

  // Used to vend all other weak pointers.
  base::WeakPtrFactory<ProximityMonitorImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProximityMonitorImpl);
};

}  // namespace proximity_auth

#endif  // CHROMEOS_COMPONENTS_PROXIMITY_AUTH_PROXIMITY_MONITOR_IMPL_H_
