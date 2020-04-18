// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_ASYNCHRONOUS_SHUTDOWN_OBJECT_CONTAINER_IMPL_H_
#define CHROMEOS_COMPONENTS_TETHER_ASYNCHRONOUS_SHUTDOWN_OBJECT_CONTAINER_IMPL_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromeos/components/tether/ad_hoc_ble_advertiser.h"
#include "chromeos/components/tether/asynchronous_shutdown_object_container.h"
#include "chromeos/components/tether/ble_advertiser.h"
#include "chromeos/components/tether/ble_scanner.h"
#include "chromeos/components/tether/disconnect_tethering_request_sender.h"

class PrefService;

namespace cryptauth {
class CryptAuthService;
class LocalDeviceDataProvider;
class RemoteBeaconSeedFetcher;
}  // namespace cryptauth

namespace device {
class BluetoothAdapter;
}  // namespace device

namespace chromeos {

class ManagedNetworkConfigurationHandler;
class NetworkConnectionHandler;
class NetworkStateHandler;

namespace tether {

class BleAdvertisementDeviceQueue;
class BleConnectionManager;
class BleConnectionMetricsLogger;
class BleSynchronizer;
class NetworkConfigurationRemover;
class TetherHostFetcher;
class WifiHotspotDisconnector;

// Concrete AsynchronousShutdownObjectContainer implementation.
class AsynchronousShutdownObjectContainerImpl
    : public AsynchronousShutdownObjectContainer,
      public BleAdvertiser::Observer,
      public BleScanner::Observer,
      public DisconnectTetheringRequestSender::Observer,
      public AdHocBleAdvertiser::Observer {
 public:
  class Factory {
   public:
    static std::unique_ptr<AsynchronousShutdownObjectContainer> NewInstance(
        scoped_refptr<device::BluetoothAdapter> adapter,
        cryptauth::CryptAuthService* cryptauth_service,
        TetherHostFetcher* tether_host_fetcher,
        NetworkStateHandler* network_state_handler,
        ManagedNetworkConfigurationHandler*
            managed_network_configuration_handler,
        NetworkConnectionHandler* network_connection_handler,
        PrefService* pref_service);
    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<AsynchronousShutdownObjectContainer> BuildInstance(
        scoped_refptr<device::BluetoothAdapter> adapter,
        cryptauth::CryptAuthService* cryptauth_service,
        TetherHostFetcher* tether_host_fetcher,
        NetworkStateHandler* network_state_handler,
        ManagedNetworkConfigurationHandler*
            managed_network_configuration_handler,
        NetworkConnectionHandler* network_connection_handler,
        PrefService* pref_service);
    virtual ~Factory();

   private:
    static Factory* factory_instance_;
  };

  ~AsynchronousShutdownObjectContainerImpl() override;

  // AsynchronousShutdownObjectContainer:
  void Shutdown(const base::Closure& shutdown_complete_callback) override;
  TetherHostFetcher* tether_host_fetcher() override;
  BleConnectionManager* ble_connection_manager() override;
  DisconnectTetheringRequestSender* disconnect_tethering_request_sender()
      override;
  NetworkConfigurationRemover* network_configuration_remover() override;
  WifiHotspotDisconnector* wifi_hotspot_disconnector() override;

 protected:
  AsynchronousShutdownObjectContainerImpl(
      scoped_refptr<device::BluetoothAdapter> adapter,
      cryptauth::CryptAuthService* cryptauth_service,
      TetherHostFetcher* tether_host_fetcher,
      NetworkStateHandler* network_state_handler,
      ManagedNetworkConfigurationHandler* managed_network_configuration_handler,
      NetworkConnectionHandler* network_connection_handler,
      PrefService* pref_service);

  // BleAdvertiser::Observer:
  void OnAllAdvertisementsUnregistered() override;

  // BleScanner::Observer:
  void OnDiscoverySessionStateChanged(bool discovery_session_active) override;

  // DisconnectTetheringRequestSender::Observer:
  void OnPendingDisconnectRequestsComplete() override;

  // AdHocBleAdvertiser::Observer:
  void OnAsynchronousShutdownComplete() override;

 private:
  friend class AsynchronousShutdownObjectContainerImplTest;

  void ShutdownIfPossible();
  bool AreAsynchronousOperationsActive();

  void SetTestDoubles(
      std::unique_ptr<BleAdvertiser> ble_advertiser,
      std::unique_ptr<BleScanner> ble_scanner,
      std::unique_ptr<DisconnectTetheringRequestSender>
          disconnect_tethering_request_sender,
      std::unique_ptr<AdHocBleAdvertiser> ad_hoc_ble_advertiser);

  scoped_refptr<device::BluetoothAdapter> adapter_;

  TetherHostFetcher* tether_host_fetcher_;
  std::unique_ptr<cryptauth::LocalDeviceDataProvider>
      local_device_data_provider_;
  std::unique_ptr<cryptauth::RemoteBeaconSeedFetcher>
      remote_beacon_seed_fetcher_;
  std::unique_ptr<BleAdvertisementDeviceQueue> ble_advertisement_device_queue_;
  std::unique_ptr<BleSynchronizer> ble_synchronizer_;
  std::unique_ptr<BleAdvertiser> ble_advertiser_;
  std::unique_ptr<BleScanner> ble_scanner_;
  std::unique_ptr<AdHocBleAdvertiser> ad_hoc_ble_advertiser_;
  std::unique_ptr<BleConnectionManager> ble_connection_manager_;
  std::unique_ptr<BleConnectionMetricsLogger> ble_connection_metrics_logger_;
  std::unique_ptr<DisconnectTetheringRequestSender>
      disconnect_tethering_request_sender_;
  std::unique_ptr<NetworkConfigurationRemover> network_configuration_remover_;
  std::unique_ptr<WifiHotspotDisconnector> wifi_hotspot_disconnector_;

  // Not set until Shutdown() is invoked.
  base::Closure shutdown_complete_callback_;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousShutdownObjectContainerImpl);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_ASYNCHRONOUS_SHUTDOWN_OBJECT_CONTAINER_IMPL_H_
