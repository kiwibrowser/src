// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_BLUETOOTH_ARC_BLUETOOTH_BRIDGE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_BLUETOOTH_ARC_BLUETOOTH_BRIDGE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/queue.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"
#include "components/arc/common/bluetooth.mojom.h"
#include "components/arc/common/intent_helper.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_advertisement.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_local_gatt_service.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_descriptor.h"
#include "device/bluetooth/bluetooth_remote_gatt_service.h"
#include "device/bluetooth/bluez/bluetooth_adapter_bluez.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

namespace mojom {
class AppHost;
class AppInstance;
class IntentHelperHost;
class IntentHelperInstance;
}  // namespace mojom

class ArcBridgeService;

class ArcBluetoothBridge
    : public KeyedService,
      public ConnectionObserver<mojom::BluetoothInstance>,
      public device::BluetoothAdapter::Observer,
      public device::BluetoothAdapterFactory::AdapterCallback,
      public device::BluetoothLocalGattService::Delegate,
      public mojom::BluetoothHost {
 public:
  using GattStatusCallback =
      base::OnceCallback<void(mojom::BluetoothGattStatus)>;
  using AdapterStateCallback =
      base::OnceCallback<void(mojom::BluetoothAdapterState)>;

  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcBluetoothBridge* GetForBrowserContext(
      content::BrowserContext* context);

  ArcBluetoothBridge(content::BrowserContext* context,
                     ArcBridgeService* bridge_service);
  ~ArcBluetoothBridge() override;

  // Overridden from ConnectionObserver<mojom::BluetoothInstance>:
  void OnConnectionReady() override;
  void OnConnectionClosed() override;

  void OnAdapterInitialized(scoped_refptr<device::BluetoothAdapter> adapter);

  // Overridden from device::BluetoothAdapter::Observer
  void AdapterPoweredChanged(device::BluetoothAdapter* adapter,
                             bool powered) override;

  void DeviceAdded(device::BluetoothAdapter* adapter,
                   device::BluetoothDevice* device) override;

  void DeviceChanged(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;

  void DeviceAddressChanged(device::BluetoothAdapter* adapter,
                            device::BluetoothDevice* device,
                            const std::string& old_address) override;

  void DevicePairedChanged(device::BluetoothAdapter* adapter,
                           device::BluetoothDevice* device,
                           bool new_paired_status) override;

  void DeviceMTUChanged(device::BluetoothAdapter* adapter,
                        device::BluetoothDevice* device,
                        uint16_t mtu) override;

  void DeviceRemoved(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;

  void GattServiceAdded(device::BluetoothAdapter* adapter,
                        device::BluetoothDevice* device,
                        device::BluetoothRemoteGattService* service) override;

  void GattServiceRemoved(device::BluetoothAdapter* adapter,
                          device::BluetoothDevice* device,
                          device::BluetoothRemoteGattService* service) override;

  void GattServicesDiscovered(device::BluetoothAdapter* adapter,
                              device::BluetoothDevice* device) override;

  void GattDiscoveryCompleteForService(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattService* service) override;

  void GattServiceChanged(device::BluetoothAdapter* adapter,
                          device::BluetoothRemoteGattService* service) override;

  void GattCharacteristicAdded(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattCharacteristic* characteristic) override;

  void GattCharacteristicRemoved(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattCharacteristic* characteristic) override;

  void GattDescriptorAdded(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattDescriptor* descriptor) override;

  void GattDescriptorRemoved(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattDescriptor* descriptor) override;

  void GattCharacteristicValueChanged(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value) override;

  void GattDescriptorValueChanged(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattDescriptor* descriptor,
      const std::vector<uint8_t>& value) override;

  // Overridden from device::BluetoothLocalGattService::Delegate
  void OnCharacteristicReadRequest(
      const device::BluetoothDevice* device,
      const device::BluetoothLocalGattCharacteristic* characteristic,
      int offset,
      const ValueCallback& callback,
      const ErrorCallback& error_callback) override;

  void OnCharacteristicWriteRequest(
      const device::BluetoothDevice* device,
      const device::BluetoothLocalGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value,
      int offset,
      const base::Closure& callback,
      const ErrorCallback& error_callback) override;

  void OnDescriptorReadRequest(
      const device::BluetoothDevice* device,
      const device::BluetoothLocalGattDescriptor* descriptor,
      int offset,
      const ValueCallback& callback,
      const ErrorCallback& error_callback) override;

  void OnDescriptorWriteRequest(
      const device::BluetoothDevice* device,
      const device::BluetoothLocalGattDescriptor* descriptor,
      const std::vector<uint8_t>& value,
      int offset,
      const base::Closure& callback,
      const ErrorCallback& error_callback) override;

  void OnNotificationsStart(
      const device::BluetoothDevice* device,
      const device::BluetoothLocalGattCharacteristic* characteristic) override;

  void OnNotificationsStop(
      const device::BluetoothDevice* device,
      const device::BluetoothLocalGattCharacteristic* characteristic) override;

  // Bluetooth Mojo host interface
  void EnableAdapter(EnableAdapterCallback callback) override;
  void DisableAdapter(DisableAdapterCallback callback) override;

  void GetAdapterProperty(mojom::BluetoothPropertyType type) override;
  void SetAdapterProperty(mojom::BluetoothPropertyPtr property) override;

  void GetRemoteDeviceProperty(mojom::BluetoothAddressPtr remote_addr,
                               mojom::BluetoothPropertyType type) override;
  void SetRemoteDeviceProperty(mojom::BluetoothAddressPtr remote_addr,
                               mojom::BluetoothPropertyPtr property) override;
  void GetRemoteServiceRecord(mojom::BluetoothAddressPtr remote_addr,
                              const device::BluetoothUUID& uuid) override;

  void GetRemoteServices(mojom::BluetoothAddressPtr remote_addr) override;

  void StartDiscovery() override;
  void CancelDiscovery() override;

  void CreateBond(mojom::BluetoothAddressPtr addr, int32_t transport) override;
  void RemoveBond(mojom::BluetoothAddressPtr addr) override;
  void CancelBond(mojom::BluetoothAddressPtr addr) override;

  void GetConnectionState(mojom::BluetoothAddressPtr addr,
                          GetConnectionStateCallback callback) override;

  // Bluetooth Mojo host interface - Bluetooth Gatt Client functions
  void StartLEScan() override;
  void StopLEScan() override;
  void ConnectLEDevice(mojom::BluetoothAddressPtr remote_addr) override;
  void DisconnectLEDevice(mojom::BluetoothAddressPtr remote_addr) override;
  void StartLEListen(StartLEListenCallback callback) override;
  void StopLEListen(StopLEListenCallback callback) override;
  void SearchService(mojom::BluetoothAddressPtr remote_addr) override;

  void GetGattDB(mojom::BluetoothAddressPtr remote_addr) override;
  void ReadGattCharacteristic(mojom::BluetoothAddressPtr remote_addr,
                              mojom::BluetoothGattServiceIDPtr service_id,
                              mojom::BluetoothGattIDPtr char_id,
                              ReadGattCharacteristicCallback callback) override;
  void WriteGattCharacteristic(
      mojom::BluetoothAddressPtr remote_addr,
      mojom::BluetoothGattServiceIDPtr service_id,
      mojom::BluetoothGattIDPtr char_id,
      mojom::BluetoothGattValuePtr value,
      WriteGattCharacteristicCallback callback) override;
  void ReadGattDescriptor(mojom::BluetoothAddressPtr remote_addr,
                          mojom::BluetoothGattServiceIDPtr service_id,
                          mojom::BluetoothGattIDPtr char_id,
                          mojom::BluetoothGattIDPtr desc_id,
                          ReadGattDescriptorCallback callback) override;
  void WriteGattDescriptor(mojom::BluetoothAddressPtr remote_addr,
                           mojom::BluetoothGattServiceIDPtr service_id,
                           mojom::BluetoothGattIDPtr char_id,
                           mojom::BluetoothGattIDPtr desc_id,
                           mojom::BluetoothGattValuePtr value,
                           WriteGattDescriptorCallback callback) override;
  void RegisterForGattNotification(
      mojom::BluetoothAddressPtr remote_addr,
      mojom::BluetoothGattServiceIDPtr service_id,
      mojom::BluetoothGattIDPtr char_id,
      RegisterForGattNotificationCallback callback) override;
  void DeregisterForGattNotification(
      mojom::BluetoothAddressPtr remote_addr,
      mojom::BluetoothGattServiceIDPtr service_id,
      mojom::BluetoothGattIDPtr char_id,
      DeregisterForGattNotificationCallback callback) override;
  void ReadRemoteRssi(mojom::BluetoothAddressPtr remote_addr,
                      ReadRemoteRssiCallback callback) override;

  void OpenBluetoothSocket(OpenBluetoothSocketCallback callback) override;

  // Bluetooth Mojo host interface - Bluetooth Gatt Server functions
  // Android counterpart link:
  // https://source.android.com/devices/halref/bt__gatt__server_8h.html
  // Create a new service. Chrome will create an integer service handle based on
  // that BlueZ identifier that will pass back to Android in the callback.
  // num_handles: number of handle for characteristic / descriptor that will be
  //              created in this service
  void AddService(mojom::BluetoothGattServiceIDPtr service_id,
                  int32_t num_handles,
                  AddServiceCallback callback) override;
  // Add a characteristic to a service and pass the characteristic handle back.
  void AddCharacteristic(int32_t service_handle,
                         const device::BluetoothUUID& uuid,
                         int32_t properties,
                         int32_t permissions,
                         AddCharacteristicCallback callback) override;
  // Add a descriptor to the last characteristic added to the given service
  // and pass the descriptor handle back.
  void AddDescriptor(int32_t service_handle,
                     const device::BluetoothUUID& uuid,
                     int32_t permissions,
                     AddDescriptorCallback callback) override;
  // Start a local service.
  void StartService(int32_t service_handle,
                    StartServiceCallback callback) override;
  // Stop a local service.
  void StopService(int32_t service_handle,
                   StopServiceCallback callback) override;
  // Delete a local service.
  void DeleteService(int32_t service_handle,
                     DeleteServiceCallback callback) override;
  // Send value indication to a remote device.
  void SendIndication(int32_t attribute_handle,
                      mojom::BluetoothAddressPtr address,
                      bool confirm,
                      const std::vector<uint8_t>& value,
                      SendIndicationCallback callback) override;

  // Bluetooth Mojo host interface - Bluetooth SDP functions
  void GetSdpRecords(mojom::BluetoothAddressPtr remote_addr,
                     const device::BluetoothUUID& target_uuid) override;
  void CreateSdpRecord(mojom::BluetoothSdpRecordPtr record_mojo,
                       CreateSdpRecordCallback callback) override;
  void RemoveSdpRecord(uint32_t service_handle,
                       RemoveSdpRecordCallback callback) override;

  // Set up or disable multiple advertising.
  void ReserveAdvertisementHandle(
      ReserveAdvertisementHandleCallback callback) override;
  void EnableAdvertisement(
      int32_t adv_handle,
      std::unique_ptr<device::BluetoothAdvertisement::Data> advertisement,
      EnableAdvertisementCallback callback) override;
  void DisableAdvertisement(int32_t adv_handle,
                            DisableAdvertisementCallback callback) override;
  void ReleaseAdvertisementHandle(
      int32_t adv_handle,
      ReleaseAdvertisementHandleCallback callback) override;

 private:
  template <typename... Args>
  void AddAdvertisementTask(
      base::OnceCallback<void(base::OnceCallback<void(Args...)>)> task,
      base::OnceCallback<void(Args...)> callback);
  template <typename... Args>
  void CompleteAdvertisementTask(base::OnceCallback<void(Args...)> callback,
                                 Args... args);
  void ReserveAdvertisementHandleImpl(
      ReserveAdvertisementHandleCallback callback);
  void EnableAdvertisementImpl(
      int32_t adv_handle,
      std::unique_ptr<device::BluetoothAdvertisement::Data> advertisement,
      EnableAdvertisementCallback callback);
  void DisableAdvertisementImpl(int32_t adv_handle,
                                DisableAdvertisementCallback callback);
  void ReleaseAdvertisementHandleImpl(
      int32_t adv_handle,
      ReleaseAdvertisementHandleCallback callback);

  template <typename InstanceType, typename HostType>
  class ConnectionObserverImpl;

  // Power state change on Bluetooth adapter.
  enum class AdapterPowerState { TURN_OFF, TURN_ON };

  // Chrome observer callbacks
  void OnPoweredOn(AdapterStateCallback callback, bool save_user_pref) const;
  void OnPoweredOff(AdapterStateCallback callback, bool save_user_pref) const;
  void OnPoweredError(AdapterStateCallback callback) const;
  void OnDiscoveryStarted(
      std::unique_ptr<device::BluetoothDiscoverySession> session);
  void OnDiscoveryStopped();
  void OnDiscoveryError();
  void OnPairing(mojom::BluetoothAddressPtr addr) const;
  void OnPairedDone(mojom::BluetoothAddressPtr addr) const;
  void OnPairedError(
      mojom::BluetoothAddressPtr addr,
      device::BluetoothDevice::ConnectErrorCode error_code) const;
  void OnForgetDone(mojom::BluetoothAddressPtr addr) const;
  void OnForgetError(mojom::BluetoothAddressPtr addr) const;

  void OnGattConnectStateChanged(mojom::BluetoothAddressPtr addr,
                                 bool connected) const;
  void OnGattConnected(
      mojom::BluetoothAddressPtr addr,
      std::unique_ptr<device::BluetoothGattConnection> connection);
  void OnGattConnectError(mojom::BluetoothAddressPtr addr,
                          device::BluetoothDevice::ConnectErrorCode error_code);
  void OnGattDisconnected(mojom::BluetoothAddressPtr addr);

  void OnStartLEListenDone(GattStatusCallback callback,
                           scoped_refptr<device::BluetoothAdvertisement> adv);
  void OnStartLEListenError(
      GattStatusCallback callback,
      device::BluetoothAdvertisement::ErrorCode error_code);

  void OnStopLEListenDone(GattStatusCallback callback);
  void OnStopLEListenError(
      GattStatusCallback callback,
      device::BluetoothAdvertisement::ErrorCode error_code);

  void OnGattNotifyStartDone(
      GattStatusCallback callback,
      const std::string char_string_id,
      std::unique_ptr<device::BluetoothGattNotifySession> notify_session);

  bool IsInstanceUp() const { return is_bluetooth_instance_up_; }

  // Indicates if a power change is initiated by Chrome / Android.
  bool IsPowerChangeInitiatedByRemote(
      ArcBluetoothBridge::AdapterPowerState powered) const;
  bool IsPowerChangeInitiatedByLocal(
      ArcBluetoothBridge::AdapterPowerState powered) const;

  // Sends initial power state when all preconditions are met.
  void MaybeSendInitialPowerChange();

  // Manages the powered change intents sent to Android.
  void EnqueueLocalPowerChange(AdapterPowerState powered);
  void DequeueLocalPowerChange(AdapterPowerState powered);

  // Manages the powered change requests received from Android.
  void EnqueueRemotePowerChange(AdapterPowerState powered,
                                AdapterStateCallback callback);
  void DequeueRemotePowerChange(AdapterPowerState powered);

  std::vector<mojom::BluetoothPropertyPtr> GetDeviceProperties(
      mojom::BluetoothPropertyType type,
      const device::BluetoothDevice* device) const;
  std::vector<mojom::BluetoothPropertyPtr> GetAdapterProperties(
      mojom::BluetoothPropertyType type) const;
  std::vector<mojom::BluetoothAdvertisingDataPtr> GetAdvertisingData(
      const device::BluetoothDevice* device) const;

  void SendCachedDevicesFound() const;

  device::BluetoothRemoteGattCharacteristic* FindGattCharacteristic(
      mojom::BluetoothAddressPtr remote_addr,
      mojom::BluetoothGattServiceIDPtr service_id,
      mojom::BluetoothGattIDPtr char_id) const;

  device::BluetoothRemoteGattDescriptor* FindGattDescriptor(
      mojom::BluetoothAddressPtr remote_addr,
      mojom::BluetoothGattServiceIDPtr service_id,
      mojom::BluetoothGattIDPtr char_id,
      mojom::BluetoothGattIDPtr desc_id) const;

  // Send the power status change to Android via an intent.
  void SendBluetoothPoweredStateBroadcast(AdapterPowerState powered) const;

  // Propagates the list of paired device to Android.
  void SendCachedPairedDevices() const;

  bool IsGattServerAttributeHandleAvailable(int need);
  int32_t GetNextGattServerAttributeHandle();
  template <class LocalGattAttribute>
  int32_t CreateGattAttributeHandle(LocalGattAttribute* attribute);

  // Common code for OnCharacteristicReadRequest and OnDescriptorReadRequest
  template <class LocalGattAttribute>
  void OnGattAttributeReadRequest(
      const device::BluetoothDevice* device,
      const LocalGattAttribute* attribute,
      int offset,
      mojom::BluetoothGattDBAttributeType attribute_type,
      const ValueCallback& success_callback,
      const ErrorCallback& error_callback);

  // Common code for OnCharacteristicWriteRequest and OnDescriptorWriteRequest
  template <class LocalGattAttribute>
  void OnGattAttributeWriteRequest(
      const device::BluetoothDevice* device,
      const LocalGattAttribute* attribute,
      const std::vector<uint8_t>& value,
      int offset,
      mojom::BluetoothGattDBAttributeType attribute_type,
      const base::Closure& success_callback,
      const ErrorCallback& error_callback);

  void OnSetDiscoverable(bool discoverable, bool success, uint32_t timeout);
  void SetDiscoverable(bool discoverable, uint32_t timeout);

  void OnGetServiceRecordsDone(
      mojom::BluetoothAddressPtr remote_addr,
      const device::BluetoothUUID& target_uuid,
      const std::vector<bluez::BluetoothServiceRecordBlueZ>& records_bluez);
  void OnGetServiceRecordsError(
      mojom::BluetoothAddressPtr remote_addr,
      const device::BluetoothUUID& target_uuid,
      bluez::BluetoothServiceRecordBlueZ::ErrorCode error_code);

  void OnSetAdapterProperty(mojom::BluetoothStatus success,
                            mojom::BluetoothPropertyPtr property);

  // Convenience function to set primary user's bluetooth pref.
  void SetPrimaryUserBluetoothPowerSetting(bool enabled) const;

  // Callbacks for managing advertisements registered from the instance.

  // Called when we have an open slot in the advertisement map and want to
  // register the advertisement given by |data| for handle |adv_handle|.
  void OnReadyToRegisterAdvertisement(
      GattStatusCallback callback,
      int32_t adv_handle,
      std::unique_ptr<device::BluetoothAdvertisement::Data> data);
  // Called when we've successfully registered a new advertisement for
  // handle |adv_handle|.
  void OnRegisterAdvertisementDone(
      GattStatusCallback callback,
      int32_t adv_handle,
      scoped_refptr<device::BluetoothAdvertisement> advertisement);
  // Called when the attempt to register an advertisement for handle
  // |adv_handle| has failed. |adv_handle| remains reserved, but no
  // advertisement is associated with it.
  void OnRegisterAdvertisementError(
      GattStatusCallback callback,
      int32_t adv_handle,
      device::BluetoothAdvertisement::ErrorCode error_code);
  void OnUnregisterAdvertisementDone(GattStatusCallback callback,
                                     int32_t adv_handle);
  void OnUnregisterAdvertisementError(
      GattStatusCallback callback,
      int32_t adv_handle,
      device::BluetoothAdvertisement::ErrorCode error_code);
  // Both of the following are called after we've tried to release |adv_handle|.
  // Either way, we will no longer be broadcasting this advertisement, so in
  // either case, the handle can be released.
  void OnReleaseAdvertisementHandleDone(GattStatusCallback callback,
                                        int32_t adv_handle);
  void OnReleaseAdvertisementHandleError(
      GattStatusCallback callback,
      int32_t adv_handle,
      device::BluetoothAdvertisement::ErrorCode error_code);
  // Find the next free advertisement handle and put it in *adv_handle,
  // or return false if the advertisement map is full.
  bool GetAdvertisementHandle(int32_t* adv_handle);

  void SendDevice(const device::BluetoothDevice* device) const;

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.

  scoped_refptr<bluez::BluetoothAdapterBlueZ> bluetooth_adapter_;
  scoped_refptr<device::BluetoothAdvertisement> advertisment_;
  std::unique_ptr<device::BluetoothDiscoverySession> discovery_session_;
  std::unordered_map<std::string,
                     std::unique_ptr<device::BluetoothGattNotifySession>>
      notification_session_;
  // Map from Android int handle to Chrome (BlueZ) string identifier.
  std::unordered_map<int32_t, std::string> gatt_identifier_;
  // Map from Chrome (BlueZ) string identifier to android int handle.
  std::unordered_map<std::string, int32_t> gatt_handle_;
  // Store last GattCharacteristic added to each GattService for GattServer.
  std::unordered_map<int32_t, int32_t> last_characteristic_;
  // Monotonically increasing value to use as handle to give to Android side.
  int32_t gatt_server_attribute_next_handle_ = 0;

  // For established connection from remote device, this is { CONNECTED, null }.
  // For ongoing connection to remote device, this is { CONNECTING, null }.
  // For established connection to remote device, this is { CONNECTED, ptr }.
  struct GattConnection {
    enum class ConnectionState { CONNECTING, CONNECTED } state;
    std::unique_ptr<device::BluetoothGattConnection> connection;
    GattConnection(ConnectionState state,
                   std::unique_ptr<device::BluetoothGattConnection> connection);
    GattConnection();
    ~GattConnection();
    GattConnection(GattConnection&&);
    GattConnection& operator=(GattConnection&&);

   private:
    DISALLOW_COPY_AND_ASSIGN(GattConnection);
  };
  std::unordered_map<std::string, GattConnection> gatt_connections_;

  // Timer to turn discovery off.
  base::OneShotTimer discovery_off_timer_;
  // Timer to turn adapter discoverable off.
  base::OneShotTimer discoverable_off_timer_;

  // This indicates whether the remote Bluetooth ARC instance is ready to
  // receive events.
  bool is_bluetooth_instance_up_;

  // Observers to listen the start-up of App and Intent Helper.
  std::unique_ptr<ConnectionObserverImpl<mojom::AppInstance, mojom::AppHost>>
      app_observer_;
  std::unique_ptr<ConnectionObserverImpl<mojom::IntentHelperInstance,
                                         mojom::IntentHelperHost>>
      intent_helper_observer_;
  // Queue to track the powered state changes initiated by Android.
  base::queue<AdapterPowerState> remote_power_changes_;
  // Queue to track the powered state changes initiated by Chrome.
  base::queue<AdapterPowerState> local_power_changes_;
  // Timer to track the completion of power-changed intent sent to Android.
  base::OneShotTimer power_intent_timer_;

  // Holds advertising data registered by the instance.
  //
  // When a handle is reserved, an entry is placed into the advertisements_
  // map. This entry is not yet associated with a device::BluetoothAdvertisement
  // because the instance hasn't sent us any advertising data yet, so its
  // mapped value is nullptr until that happens. Thus we have three states for a
  // handle:
  // * unmapped -> free
  // * mapped to nullptr -> reserved, awaiting data
  // * mapped to a device::BluetoothAdvertisement -> in use, and the mapped
  //   BluetoothAdvertisement is currently registered with the adapter.
  // TODO(crbug.com/658385) Change back to 5 when we support setting signal
  // strength per each advertisement slot.
  enum { kMaxAdvertisements = 1 };
  std::map<int32_t, scoped_refptr<device::BluetoothAdvertisement>>
      advertisements_;
  base::queue<base::OnceClosure> advertisement_task_queue_;

  THREAD_CHECKER(thread_checker_);

  // WeakPtrFactory to use for callbacks.
  base::WeakPtrFactory<ArcBluetoothBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcBluetoothBridge);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_BLUETOOTH_ARC_BLUETOOTH_BRIDGE_H_
