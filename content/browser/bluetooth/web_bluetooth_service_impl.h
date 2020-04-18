// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLUETOOTH_WEB_BLUETOOTH_SERVICE_IMPL_H_
#define CONTENT_BROWSER_BLUETOOTH_WEB_BLUETOOTH_SERVICE_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "content/browser/bad_message.h"
#include "content/browser/bluetooth/bluetooth_allowed_devices.h"
#include "content/common/content_export.h"
#include "content/public/browser/web_contents_observer.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"
#include "device/bluetooth/bluetooth_gatt_notify_session.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_service.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/platform/modules/bluetooth/web_bluetooth.mojom.h"

namespace url {
class Origin;
}  // namespace url

namespace content {

class BluetoothDeviceChooserController;
struct CacheQueryResult;
class FrameConnectedBluetoothDevices;
struct GATTNotifySessionAndCharacteristicClient;
class RenderFrameHost;
class RenderProcessHost;

// Implementation of Mojo WebBluetoothService located in
// third_party/WebKit/public/platform/modules/bluetooth.
// It handles Web Bluetooth API requests coming from Blink / renderer
// process and uses the platform abstraction of device/bluetooth.
// WebBluetoothServiceImpl is not thread-safe and should be created on the
// UI thread as required by device/bluetooth.
// This class is instantiated on-demand via Mojo's ConnectToRemoteService
// from the renderer when the first Web Bluetooth API request is handled.
// RenderFrameHostImpl will create an instance of this class and keep
// ownership of it.
class CONTENT_EXPORT WebBluetoothServiceImpl
    : public blink::mojom::WebBluetoothService,
      public WebContentsObserver,
      public device::BluetoothAdapter::Observer {
 public:
  // |render_frame_host|: The RFH that owns this instance.
  // |request|: The instance will be bound to this request's pipe.
  WebBluetoothServiceImpl(RenderFrameHost* render_frame_host,
                          blink::mojom::WebBluetoothServiceRequest request);
  ~WebBluetoothServiceImpl() override;

  void CrashRendererAndClosePipe(bad_message::BadMessageReason reason);

  // Sets the connection error handler for WebBluetoothServiceImpl's Binding.
  void SetClientConnectionErrorHandler(base::OnceClosure closure);

  // Returns whether the device is paired with the |render_frame_host_|'s
  // GetLastCommittedOrigin().
  bool IsDevicePaired(const std::string& device_address);

 private:
  friend class FrameConnectedBluetoothDevicesTest;
  using PrimaryServicesRequestCallback =
      base::OnceCallback<void(device::BluetoothDevice*)>;

  // WebContentsObserver:
  // These functions should always check that the affected RenderFrameHost
  // is this->render_frame_host_ and not some other frame in the same tab.
  void DidFinishNavigation(NavigationHandle* navigation_handle) override;

  // BluetoothAdapter::Observer:
  void AdapterPoweredChanged(device::BluetoothAdapter* adapter,
                             bool powered) override;
  void DeviceAdded(device::BluetoothAdapter* adapter,
                   device::BluetoothDevice* device) override;
  void DeviceChanged(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;
  void GattServicesDiscovered(device::BluetoothAdapter* adapter,
                              device::BluetoothDevice* device) override;
  void GattCharacteristicValueChanged(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value) override;

  // Notifies the WebBluetoothServiceClient that characteristic
  // |characteristic_instance_id| changed it's value. We only do this for
  // characteristics that have been returned to the client in the past.
  void NotifyCharacteristicValueChanged(
      const std::string& characteristic_instance_id,
      const std::vector<uint8_t>& value);

  // WebBluetoothService methods:
  void RequestDevice(blink::mojom::WebBluetoothRequestDeviceOptionsPtr options,
                     RequestDeviceCallback callback) override;
  void RemoteServerConnect(
      const WebBluetoothDeviceId& device_id,
      blink::mojom::WebBluetoothServerClientAssociatedPtrInfo client,
      RemoteServerConnectCallback callback) override;
  void RemoteServerDisconnect(const WebBluetoothDeviceId& device_id) override;
  void RemoteServerGetPrimaryServices(
      const WebBluetoothDeviceId& device_id,
      blink::mojom::WebBluetoothGATTQueryQuantity quantity,
      const base::Optional<device::BluetoothUUID>& services_uuid,
      RemoteServerGetPrimaryServicesCallback callback) override;
  void RemoteServiceGetCharacteristics(
      const std::string& service_instance_id,
      blink::mojom::WebBluetoothGATTQueryQuantity quantity,
      const base::Optional<device::BluetoothUUID>& characteristics_uuid,
      RemoteServiceGetCharacteristicsCallback callback) override;
  void RemoteCharacteristicReadValue(
      const std::string& characteristic_instance_id,
      RemoteCharacteristicReadValueCallback callback) override;
  void RemoteCharacteristicWriteValue(
      const std::string& characteristic_instance_id,
      const std::vector<uint8_t>& value,
      RemoteCharacteristicWriteValueCallback callback) override;
  void RemoteCharacteristicStartNotifications(
      const std::string& characteristic_instance_id,
      blink::mojom::WebBluetoothCharacteristicClientAssociatedPtrInfo client,
      RemoteCharacteristicStartNotificationsCallback callback) override;
  void RemoteCharacteristicStopNotifications(
      const std::string& characteristic_instance_id,
      RemoteCharacteristicStopNotificationsCallback callback) override;
  void RemoteCharacteristicGetDescriptors(
      const std::string& service_instance_id,
      blink::mojom::WebBluetoothGATTQueryQuantity quantity,
      const base::Optional<device::BluetoothUUID>& characteristics_uuid,
      RemoteCharacteristicGetDescriptorsCallback callback) override;
  void RemoteDescriptorReadValue(
      const std::string& characteristic_instance_id,
      RemoteDescriptorReadValueCallback callback) override;
  void RemoteDescriptorWriteValue(
      const std::string& descriptor_instance_id,
      const std::vector<uint8_t>& value,
      RemoteDescriptorWriteValueCallback callback) override;

  void RequestDeviceImpl(
      blink::mojom::WebBluetoothRequestDeviceOptionsPtr options,
      RequestDeviceCallback callback,
      device::BluetoothAdapter* adapter);

  // Should only be run after the services have been discovered for
  // |device_address|.
  void RemoteServerGetPrimaryServicesImpl(
      const WebBluetoothDeviceId& device_id,
      blink::mojom::WebBluetoothGATTQueryQuantity quantity,
      const base::Optional<device::BluetoothUUID>& services_uuid,
      RemoteServerGetPrimaryServicesCallback callback,
      device::BluetoothDevice* device);

  // Callbacks for BluetoothDeviceChooserController::GetDevice.
  void OnGetDeviceSuccess(
      RequestDeviceCallback callback,
      blink::mojom::WebBluetoothRequestDeviceOptionsPtr options,
      const std::string& device_id);
  void OnGetDeviceFailed(RequestDeviceCallback callback,
                         blink::mojom::WebBluetoothResult result);

  // Callbacks for BluetoothDevice::CreateGattConnection.
  void OnCreateGATTConnectionSuccess(
      const WebBluetoothDeviceId& device_id,
      base::TimeTicks start_time,
      blink::mojom::WebBluetoothServerClientAssociatedPtr client,
      RemoteServerConnectCallback callback,
      std::unique_ptr<device::BluetoothGattConnection> connection);
  void OnCreateGATTConnectionFailed(
      base::TimeTicks start_time,
      RemoteServerConnectCallback callback,
      device::BluetoothDevice::ConnectErrorCode error_code);

  // Callbacks for BluetoothRemoteGattCharacteristic::ReadRemoteCharacteristic.
  void OnCharacteristicReadValueSuccess(
      RemoteCharacteristicReadValueCallback callback,
      const std::vector<uint8_t>& value);
  void OnCharacteristicReadValueFailed(
      RemoteCharacteristicReadValueCallback callback,
      device::BluetoothRemoteGattService::GattErrorCode error_code);

  // Callbacks for BluetoothRemoteGattCharacteristic::WriteRemoteCharacteristic.
  void OnCharacteristicWriteValueSuccess(
      RemoteCharacteristicWriteValueCallback callback);
  void OnCharacteristicWriteValueFailed(
      RemoteCharacteristicWriteValueCallback callback,
      device::BluetoothRemoteGattService::GattErrorCode error_code);

  // Callbacks for BluetoothRemoteGattCharacteristic::StartNotifySession.
  void OnStartNotifySessionSuccess(
      blink::mojom::WebBluetoothCharacteristicClientAssociatedPtr client,
      RemoteCharacteristicStartNotificationsCallback callback,
      std::unique_ptr<device::BluetoothGattNotifySession> notify_session);
  void OnStartNotifySessionFailed(
      RemoteCharacteristicStartNotificationsCallback callback,
      device::BluetoothRemoteGattService::GattErrorCode error_code);

  // Callback for BluetoothGattNotifySession::Stop.
  void OnStopNotifySessionComplete(
      const std::string& characteristic_instance_id,
      RemoteCharacteristicStopNotificationsCallback callback);

  // Callbacks for BluetoothRemoteGattDescriptor::ReadRemoteDescriptor.
  void OnDescriptorReadValueSuccess(RemoteDescriptorReadValueCallback callback,
                                    const std::vector<uint8_t>& value);
  void OnDescriptorReadValueFailed(
      RemoteDescriptorReadValueCallback callback,
      device::BluetoothRemoteGattService::GattErrorCode error_code);

  // Callbacks for BluetoothRemoteGattDescriptor::WriteRemoteDescriptor.
  void OnDescriptorWriteValueSuccess(
      RemoteDescriptorWriteValueCallback callback);
  void OnDescriptorWriteValueFailed(
      RemoteDescriptorWriteValueCallback callback,
      device::BluetoothRemoteGattService::GattErrorCode error_code);

  // Functions to query the platform cache for the bluetooth object.
  // result.outcome == CacheQueryOutcome::SUCCESS if the object was found in the
  // cache. Otherwise result.outcome that can used to record the outcome and
  // result.error will contain the error that should be sent to the renderer.
  // One of the possible outcomes is BAD_RENDERER. In this case we crash the
  // renderer, record the reason and close the pipe, so it's safe to drop
  // any callbacks.

  // Queries the platform cache for a Device with |device_id| for |origin|.
  // Fills in the |outcome| field and the |device| field if successful.
  CacheQueryResult QueryCacheForDevice(const WebBluetoothDeviceId& device_id);

  // Queries the platform cache for a Service with |service_instance_id|. Fills
  // in the |outcome| field, and |device| and |service| fields if successful.
  CacheQueryResult QueryCacheForService(const std::string& service_instance_id);

  // Queries the platform cache for a characteristic with
  // |characteristic_instance_id|. Fills in the |outcome| field, and |device|,
  // |service| and |characteristic| fields if successful.
  CacheQueryResult QueryCacheForCharacteristic(
      const std::string& characteristic_instance_id);

  // Queries the platform cache for a descriptor with |descriptor_instance_id|.
  // Fills in the |outcome| field, and |device|, |service|, |characteristic|,
  // |descriptor| fields if successful.
  CacheQueryResult QueryCacheForDescriptor(
      const std::string& descriptor_instance_id);

  void RunPendingPrimaryServicesRequests(device::BluetoothDevice* device);

  RenderProcessHost* GetRenderProcessHost();
  device::BluetoothAdapter* GetAdapter();
  url::Origin GetOrigin();
  BluetoothAllowedDevices& allowed_devices();

  // Clears all state (maps, sets, etc).
  void ClearState();

  // Used to open a BluetoothChooser and start a device discovery session.
  std::unique_ptr<BluetoothDeviceChooserController> device_chooser_controller_;

  // Maps to get the object's parent based on its instanceID.
  std::unordered_map<std::string, std::string> service_id_to_device_address_;
  std::unordered_map<std::string, std::string> characteristic_id_to_service_id_;
  std::unordered_map<std::string, std::string>
      descriptor_id_to_characteristic_id_;

  // Map to keep track of the connected Bluetooth devices.
  std::unique_ptr<FrameConnectedBluetoothDevices> connected_devices_;

  // Maps a device address to callbacks that are waiting for services to
  // be discovered for that device.
  std::unordered_map<std::string, std::vector<PrimaryServicesRequestCallback>>
      pending_primary_services_requests_;

  // Map to keep track of the characteristics' notify sessions.
  std::unordered_map<std::string,
                     std::unique_ptr<GATTNotifySessionAndCharacteristicClient>>
      characteristic_id_to_notify_session_;

  // The RFH that owns this instance.
  RenderFrameHost* render_frame_host_;

  // The lifetime of this instance is exclusively managed by the RFH that
  // owns it so we use a "Binding" as opposed to a "StrongBinding" which deletes
  // the service on pipe connection errors.
  mojo::Binding<blink::mojom::WebBluetoothService> binding_;

  base::WeakPtrFactory<WebBluetoothServiceImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebBluetoothServiceImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BLUETOOTH_WEB_BLUETOOTH_SERVICE_IMPL_H_
