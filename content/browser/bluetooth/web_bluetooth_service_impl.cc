// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ID Not In Map Note: A service, characteristic, or descriptor ID not in the
// corresponding WebBluetoothServiceImpl map [service_id_to_device_address_,
// characteristic_id_to_service_id_, descriptor_id_to_characteristic_id_]
// implies a hostile renderer because a renderer obtains the corresponding ID
// from this class and it will be added to the map at that time.

#include "content/browser/bluetooth/web_bluetooth_service_impl.h"

#include <algorithm>

#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/bluetooth/bluetooth_blocklist.h"
#include "content/browser/bluetooth/bluetooth_device_chooser_controller.h"
#include "content/browser/bluetooth/bluetooth_metrics.h"
#include "content/browser/bluetooth/frame_connected_bluetooth_devices.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/storage_partition_impl.h"
#include "content/common/bluetooth/web_bluetooth_device_id.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "device/bluetooth/bluetooth_adapter_factory_wrapper.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_descriptor.h"

using device::BluetoothAdapterFactoryWrapper;
using device::BluetoothUUID;

namespace content {

namespace {

blink::mojom::WebBluetoothResult TranslateConnectErrorAndRecord(
    device::BluetoothDevice::ConnectErrorCode error_code) {
  switch (error_code) {
    case device::BluetoothDevice::ERROR_UNKNOWN:
      RecordConnectGATTOutcome(UMAConnectGATTOutcome::UNKNOWN);
      return blink::mojom::WebBluetoothResult::CONNECT_UNKNOWN_ERROR;
    case device::BluetoothDevice::ERROR_INPROGRESS:
      RecordConnectGATTOutcome(UMAConnectGATTOutcome::IN_PROGRESS);
      return blink::mojom::WebBluetoothResult::CONNECT_ALREADY_IN_PROGRESS;
    case device::BluetoothDevice::ERROR_FAILED:
      RecordConnectGATTOutcome(UMAConnectGATTOutcome::FAILED);
      return blink::mojom::WebBluetoothResult::CONNECT_UNKNOWN_FAILURE;
    case device::BluetoothDevice::ERROR_AUTH_FAILED:
      RecordConnectGATTOutcome(UMAConnectGATTOutcome::AUTH_FAILED);
      return blink::mojom::WebBluetoothResult::CONNECT_AUTH_FAILED;
    case device::BluetoothDevice::ERROR_AUTH_CANCELED:
      RecordConnectGATTOutcome(UMAConnectGATTOutcome::AUTH_CANCELED);
      return blink::mojom::WebBluetoothResult::CONNECT_AUTH_CANCELED;
    case device::BluetoothDevice::ERROR_AUTH_REJECTED:
      RecordConnectGATTOutcome(UMAConnectGATTOutcome::AUTH_REJECTED);
      return blink::mojom::WebBluetoothResult::CONNECT_AUTH_REJECTED;
    case device::BluetoothDevice::ERROR_AUTH_TIMEOUT:
      RecordConnectGATTOutcome(UMAConnectGATTOutcome::AUTH_TIMEOUT);
      return blink::mojom::WebBluetoothResult::CONNECT_AUTH_TIMEOUT;
    case device::BluetoothDevice::ERROR_UNSUPPORTED_DEVICE:
      RecordConnectGATTOutcome(UMAConnectGATTOutcome::UNSUPPORTED_DEVICE);
      return blink::mojom::WebBluetoothResult::CONNECT_UNSUPPORTED_DEVICE;
    case device::BluetoothDevice::NUM_CONNECT_ERROR_CODES:
      NOTREACHED();
      return blink::mojom::WebBluetoothResult::CONNECT_UNKNOWN_FAILURE;
  }
  NOTREACHED();
  return blink::mojom::WebBluetoothResult::CONNECT_UNKNOWN_FAILURE;
}

blink::mojom::WebBluetoothResult TranslateGATTErrorAndRecord(
    device::BluetoothRemoteGattService::GattErrorCode error_code,
    UMAGATTOperation operation) {
  switch (error_code) {
    case device::BluetoothRemoteGattService::GATT_ERROR_UNKNOWN:
      RecordGATTOperationOutcome(operation, UMAGATTOperationOutcome::UNKNOWN);
      return blink::mojom::WebBluetoothResult::GATT_UNKNOWN_ERROR;
    case device::BluetoothRemoteGattService::GATT_ERROR_FAILED:
      RecordGATTOperationOutcome(operation, UMAGATTOperationOutcome::FAILED);
      return blink::mojom::WebBluetoothResult::GATT_UNKNOWN_FAILURE;
    case device::BluetoothRemoteGattService::GATT_ERROR_IN_PROGRESS:
      RecordGATTOperationOutcome(operation,
                                 UMAGATTOperationOutcome::IN_PROGRESS);
      return blink::mojom::WebBluetoothResult::GATT_OPERATION_IN_PROGRESS;
    case device::BluetoothRemoteGattService::GATT_ERROR_INVALID_LENGTH:
      RecordGATTOperationOutcome(operation,
                                 UMAGATTOperationOutcome::INVALID_LENGTH);
      return blink::mojom::WebBluetoothResult::GATT_INVALID_ATTRIBUTE_LENGTH;
    case device::BluetoothRemoteGattService::GATT_ERROR_NOT_PERMITTED:
      RecordGATTOperationOutcome(operation,
                                 UMAGATTOperationOutcome::NOT_PERMITTED);
      return blink::mojom::WebBluetoothResult::GATT_NOT_PERMITTED;
    case device::BluetoothRemoteGattService::GATT_ERROR_NOT_AUTHORIZED:
      RecordGATTOperationOutcome(operation,
                                 UMAGATTOperationOutcome::NOT_AUTHORIZED);
      return blink::mojom::WebBluetoothResult::GATT_NOT_AUTHORIZED;
    case device::BluetoothRemoteGattService::GATT_ERROR_NOT_PAIRED:
      RecordGATTOperationOutcome(operation,
                                 UMAGATTOperationOutcome::NOT_PAIRED);
      return blink::mojom::WebBluetoothResult::GATT_NOT_PAIRED;
    case device::BluetoothRemoteGattService::GATT_ERROR_NOT_SUPPORTED:
      RecordGATTOperationOutcome(operation,
                                 UMAGATTOperationOutcome::NOT_SUPPORTED);
      return blink::mojom::WebBluetoothResult::GATT_NOT_SUPPORTED;
  }
  NOTREACHED();
  return blink::mojom::WebBluetoothResult::GATT_UNTRANSLATED_ERROR_CODE;
}
}  // namespace

// Struct that holds the result of a cache query.
struct CacheQueryResult {
  CacheQueryResult() : outcome(CacheQueryOutcome::SUCCESS) {}

  explicit CacheQueryResult(CacheQueryOutcome outcome) : outcome(outcome) {}

  ~CacheQueryResult() {}

  blink::mojom::WebBluetoothResult GetWebResult() const {
    switch (outcome) {
      case CacheQueryOutcome::SUCCESS:
      case CacheQueryOutcome::BAD_RENDERER:
        NOTREACHED();
        return blink::mojom::WebBluetoothResult::DEVICE_NO_LONGER_IN_RANGE;
      case CacheQueryOutcome::NO_DEVICE:
        return blink::mojom::WebBluetoothResult::DEVICE_NO_LONGER_IN_RANGE;
      case CacheQueryOutcome::NO_SERVICE:
        return blink::mojom::WebBluetoothResult::SERVICE_NO_LONGER_EXISTS;
      case CacheQueryOutcome::NO_CHARACTERISTIC:
        return blink::mojom::WebBluetoothResult::
            CHARACTERISTIC_NO_LONGER_EXISTS;
      case CacheQueryOutcome::NO_DESCRIPTOR:
        return blink::mojom::WebBluetoothResult::DESCRIPTOR_NO_LONGER_EXISTS;
    }
    NOTREACHED();
    return blink::mojom::WebBluetoothResult::DEVICE_NO_LONGER_IN_RANGE;
  }

  device::BluetoothDevice* device = nullptr;
  device::BluetoothRemoteGattService* service = nullptr;
  device::BluetoothRemoteGattCharacteristic* characteristic = nullptr;
  device::BluetoothRemoteGattDescriptor* descriptor = nullptr;
  CacheQueryOutcome outcome;
};

struct GATTNotifySessionAndCharacteristicClient {
  GATTNotifySessionAndCharacteristicClient(
      std::unique_ptr<device::BluetoothGattNotifySession> session,
      blink::mojom::WebBluetoothCharacteristicClientAssociatedPtr client)
      : gatt_notify_session(std::move(session)),
        characteristic_client(std::move(client)) {}

  std::unique_ptr<device::BluetoothGattNotifySession> gatt_notify_session;
  blink::mojom::WebBluetoothCharacteristicClientAssociatedPtr
      characteristic_client;
};

WebBluetoothServiceImpl::WebBluetoothServiceImpl(
    RenderFrameHost* render_frame_host,
    blink::mojom::WebBluetoothServiceRequest request)
    : WebContentsObserver(WebContents::FromRenderFrameHost(render_frame_host)),
      connected_devices_(new FrameConnectedBluetoothDevices(render_frame_host)),
      render_frame_host_(render_frame_host),
      binding_(this, std::move(request)),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  CHECK(web_contents());
}

WebBluetoothServiceImpl::~WebBluetoothServiceImpl() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ClearState();
}

void WebBluetoothServiceImpl::SetClientConnectionErrorHandler(
    base::OnceClosure closure) {
  binding_.set_connection_error_handler(std::move(closure));
}

bool WebBluetoothServiceImpl::IsDevicePaired(
    const std::string& device_address) {
  return allowed_devices().GetDeviceId(device_address) != nullptr;
}

void WebBluetoothServiceImpl::DidFinishNavigation(
    NavigationHandle* navigation_handle) {
  if (navigation_handle->HasCommitted() &&
      navigation_handle->GetRenderFrameHost() == render_frame_host_ &&
      !navigation_handle->IsSameDocument()) {
    ClearState();
  }
}

void WebBluetoothServiceImpl::AdapterPoweredChanged(
    device::BluetoothAdapter* adapter,
    bool powered) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (device_chooser_controller_.get()) {
    device_chooser_controller_->AdapterPoweredChanged(powered);
  }
}

void WebBluetoothServiceImpl::DeviceAdded(device::BluetoothAdapter* adapter,
                                          device::BluetoothDevice* device) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (device_chooser_controller_.get()) {
    device_chooser_controller_->AddFilteredDevice(*device);
  }
}

void WebBluetoothServiceImpl::DeviceChanged(device::BluetoothAdapter* adapter,
                                            device::BluetoothDevice* device) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (device_chooser_controller_.get()) {
    device_chooser_controller_->AddFilteredDevice(*device);
  }

  if (!device->IsGattConnected()) {
    base::Optional<WebBluetoothDeviceId> device_id =
        connected_devices_->CloseConnectionToDeviceWithAddress(
            device->GetAddress());

    // Since the device disconnected we need to send an error for pending
    // primary services requests.
    RunPendingPrimaryServicesRequests(device);
  }
}

void WebBluetoothServiceImpl::GattServicesDiscovered(
    device::BluetoothAdapter* adapter,
    device::BluetoothDevice* device) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  DVLOG(1) << "Services discovered for device: " << device->GetAddress();

  if (device_chooser_controller_.get()) {
    device_chooser_controller_->AddFilteredDevice(*device);
  }

  RunPendingPrimaryServicesRequests(device);
}

void WebBluetoothServiceImpl::GattCharacteristicValueChanged(
    device::BluetoothAdapter* adapter,
    device::BluetoothRemoteGattCharacteristic* characteristic,
    const std::vector<uint8_t>& value) {
  // Don't notify of characteristics that we haven't returned.
  if (!base::ContainsKey(characteristic_id_to_service_id_,
                         characteristic->GetIdentifier())) {
    return;
  }

  // On Chrome OS and Linux, GattCharacteristicValueChanged is called before the
  // success callback for ReadRemoteCharacteristic is called, which could result
  // in an event being fired before the readValue promise is resolved.
  if (!base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::BindOnce(
              &WebBluetoothServiceImpl::NotifyCharacteristicValueChanged,
              weak_ptr_factory_.GetWeakPtr(), characteristic->GetIdentifier(),
              value))) {
    LOG(WARNING) << "No TaskRunner.";
  }
}

void WebBluetoothServiceImpl::NotifyCharacteristicValueChanged(
    const std::string& characteristic_instance_id,
    const std::vector<uint8_t>& value) {
  auto iter =
      characteristic_id_to_notify_session_.find(characteristic_instance_id);
  if (iter != characteristic_id_to_notify_session_.end()) {
    iter->second->characteristic_client->RemoteCharacteristicValueChanged(
        value);
  }
}

void WebBluetoothServiceImpl::RequestDevice(
    blink::mojom::WebBluetoothRequestDeviceOptionsPtr options,
    RequestDeviceCallback callback) {
  RecordRequestDeviceOptions(options);

  if (!GetAdapter()) {
    if (BluetoothAdapterFactoryWrapper::Get().IsLowEnergySupported()) {
      BluetoothAdapterFactoryWrapper::Get().AcquireAdapter(
          this, base::Bind(&WebBluetoothServiceImpl::RequestDeviceImpl,
                           weak_ptr_factory_.GetWeakPtr(),
                           base::Passed(&options), base::Passed(&callback)));
      return;
    }
    RecordRequestDeviceOutcome(
        UMARequestDeviceOutcome::BLUETOOTH_LOW_ENERGY_NOT_AVAILABLE);
    std::move(callback).Run(
        blink::mojom::WebBluetoothResult::BLUETOOTH_LOW_ENERGY_NOT_AVAILABLE,
        nullptr /* device */);
    return;
  }
  RequestDeviceImpl(std::move(options), std::move(callback), GetAdapter());
}

void WebBluetoothServiceImpl::RemoteServerConnect(
    const WebBluetoothDeviceId& device_id,
    blink::mojom::WebBluetoothServerClientAssociatedPtrInfo client,
    RemoteServerConnectCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const CacheQueryResult query_result = QueryCacheForDevice(device_id);

  if (query_result.outcome != CacheQueryOutcome::SUCCESS) {
    RecordConnectGATTOutcome(query_result.outcome);
    std::move(callback).Run(query_result.GetWebResult());
    return;
  }

  if (connected_devices_->IsConnectedToDeviceWithId(device_id)) {
    DVLOG(1) << "Already connected.";
    std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS);
    return;
  }

  // It's possible for WebBluetoothServiceImpl to issue two successive
  // connection requests for which it would get two successive responses
  // and consequently try to insert two BluetoothGattConnections for the
  // same device. WebBluetoothServiceImpl should reject or queue connection
  // requests if there is a pending connection already, but the platform
  // abstraction doesn't currently support checking for pending connections.
  // TODO(ortuno): CHECK that this never happens once the platform
  // abstraction allows to check for pending connections.
  // http://crbug.com/583544
  const base::TimeTicks start_time = base::TimeTicks::Now();
  blink::mojom::WebBluetoothServerClientAssociatedPtr
      web_bluetooth_server_client;
  web_bluetooth_server_client.Bind(std::move(client));

  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  query_result.device->CreateGattConnection(
      base::Bind(&WebBluetoothServiceImpl::OnCreateGATTConnectionSuccess,
                 weak_ptr_factory_.GetWeakPtr(), device_id, start_time,
                 base::Passed(&web_bluetooth_server_client), copyable_callback),
      base::Bind(&WebBluetoothServiceImpl::OnCreateGATTConnectionFailed,
                 weak_ptr_factory_.GetWeakPtr(), start_time,
                 copyable_callback));
}

void WebBluetoothServiceImpl::RemoteServerDisconnect(
    const WebBluetoothDeviceId& device_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (connected_devices_->IsConnectedToDeviceWithId(device_id)) {
    DVLOG(1) << "Disconnecting device: " << device_id.str();
    connected_devices_->CloseConnectionToDeviceWithId(device_id);
  }
}

void WebBluetoothServiceImpl::RemoteServerGetPrimaryServices(
    const WebBluetoothDeviceId& device_id,
    blink::mojom::WebBluetoothGATTQueryQuantity quantity,
    const base::Optional<BluetoothUUID>& services_uuid,
    RemoteServerGetPrimaryServicesCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RecordGetPrimaryServicesServices(quantity, services_uuid);

  if (!allowed_devices().IsAllowedToAccessAtLeastOneService(device_id)) {
    std::move(callback).Run(
        blink::mojom::WebBluetoothResult::NOT_ALLOWED_TO_ACCESS_ANY_SERVICE,
        base::nullopt /* service */);
    return;
  }

  if (services_uuid &&
      !allowed_devices().IsAllowedToAccessService(device_id,
                                                  services_uuid.value())) {
    std::move(callback).Run(
        blink::mojom::WebBluetoothResult::NOT_ALLOWED_TO_ACCESS_SERVICE,
        base::nullopt /* service */);
    return;
  }

  const CacheQueryResult query_result = QueryCacheForDevice(device_id);

  if (query_result.outcome == CacheQueryOutcome::BAD_RENDERER) {
    return;
  }

  if (query_result.outcome != CacheQueryOutcome::SUCCESS) {
    RecordGetPrimaryServicesOutcome(quantity, query_result.outcome);
    std::move(callback).Run(query_result.GetWebResult(),
                            base::nullopt /* service */);
    return;
  }

  const std::string& device_address = query_result.device->GetAddress();

  // We can't know if a service is present or not until GATT service discovery
  // is complete for the device.
  if (query_result.device->IsGattServicesDiscoveryComplete()) {
    RemoteServerGetPrimaryServicesImpl(device_id, quantity, services_uuid,
                                       std::move(callback),
                                       query_result.device);
    return;
  }

  DVLOG(1) << "Services not yet discovered.";
  pending_primary_services_requests_[device_address].push_back(base::BindOnce(
      &WebBluetoothServiceImpl::RemoteServerGetPrimaryServicesImpl,
      base::Unretained(this), device_id, quantity, services_uuid,
      std::move(callback)));
}

void WebBluetoothServiceImpl::RemoteServiceGetCharacteristics(
    const std::string& service_instance_id,
    blink::mojom::WebBluetoothGATTQueryQuantity quantity,
    const base::Optional<BluetoothUUID>& characteristics_uuid,
    RemoteServiceGetCharacteristicsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  RecordGetCharacteristicsCharacteristic(quantity, characteristics_uuid);

  if (characteristics_uuid &&
      BluetoothBlocklist::Get().IsExcluded(characteristics_uuid.value())) {
    RecordGetCharacteristicsOutcome(quantity,
                                    UMAGetCharacteristicOutcome::BLOCKLISTED);
    std::move(callback).Run(
        blink::mojom::WebBluetoothResult::BLOCKLISTED_CHARACTERISTIC_UUID,
        base::nullopt /* characteristics */);
    return;
  }

  const CacheQueryResult query_result =
      QueryCacheForService(service_instance_id);

  if (query_result.outcome == CacheQueryOutcome::BAD_RENDERER) {
    return;
  }

  if (query_result.outcome != CacheQueryOutcome::SUCCESS) {
    RecordGetCharacteristicsOutcome(quantity, query_result.outcome);
    std::move(callback).Run(query_result.GetWebResult(),
                            base::nullopt /* characteristics */);
    return;
  }

  std::vector<device::BluetoothRemoteGattCharacteristic*> characteristics =
      characteristics_uuid ? query_result.service->GetCharacteristicsByUUID(
                                 characteristics_uuid.value())
                           : query_result.service->GetCharacteristics();

  std::vector<blink::mojom::WebBluetoothRemoteGATTCharacteristicPtr>
      response_characteristics;
  for (device::BluetoothRemoteGattCharacteristic* characteristic :
       characteristics) {
    if (BluetoothBlocklist::Get().IsExcluded(characteristic->GetUUID())) {
      continue;
    }
    std::string characteristic_instance_id = characteristic->GetIdentifier();
    auto insert_result = characteristic_id_to_service_id_.insert(
        std::make_pair(characteristic_instance_id, service_instance_id));
    // If value is already in map, DCHECK it's valid.
    if (!insert_result.second)
      DCHECK(insert_result.first->second == service_instance_id);

    blink::mojom::WebBluetoothRemoteGATTCharacteristicPtr characteristic_ptr =
        blink::mojom::WebBluetoothRemoteGATTCharacteristic::New();
    characteristic_ptr->instance_id = characteristic_instance_id;
    characteristic_ptr->uuid = characteristic->GetUUID();
    characteristic_ptr->properties =
        static_cast<uint32_t>(characteristic->GetProperties());
    response_characteristics.push_back(std::move(characteristic_ptr));

    if (quantity == blink::mojom::WebBluetoothGATTQueryQuantity::SINGLE) {
      break;
    }
  }

  if (!response_characteristics.empty()) {
    RecordGetCharacteristicsOutcome(quantity,
                                    UMAGetCharacteristicOutcome::SUCCESS);
    std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS,
                            std::move(response_characteristics));
    return;
  }

  RecordGetCharacteristicsOutcome(
      quantity, characteristics_uuid
                    ? UMAGetCharacteristicOutcome::NOT_FOUND
                    : UMAGetCharacteristicOutcome::NO_CHARACTERISTICS);
  std::move(callback).Run(
      characteristics_uuid
          ? blink::mojom::WebBluetoothResult::CHARACTERISTIC_NOT_FOUND
          : blink::mojom::WebBluetoothResult::NO_CHARACTERISTICS_FOUND,
      base::nullopt /* characteristics */);
}

void WebBluetoothServiceImpl::RemoteCharacteristicGetDescriptors(
    const std::string& characteristic_instance_id,
    blink::mojom::WebBluetoothGATTQueryQuantity quantity,
    const base::Optional<BluetoothUUID>& descriptors_uuid,
    RemoteCharacteristicGetDescriptorsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  RecordGetDescriptorsDescriptor(quantity, descriptors_uuid);

  if (descriptors_uuid &&
      BluetoothBlocklist::Get().IsExcluded(descriptors_uuid.value())) {
    RecordGetDescriptorsOutcome(quantity, UMAGetDescriptorOutcome::BLOCKLISTED);
    std::move(callback).Run(
        blink::mojom::WebBluetoothResult::BLOCKLISTED_DESCRIPTOR_UUID,
        base::nullopt /* descriptor */);
    return;
  }

  const CacheQueryResult query_result =
      QueryCacheForCharacteristic(characteristic_instance_id);

  if (query_result.outcome == CacheQueryOutcome::BAD_RENDERER) {
    return;
  }

  if (query_result.outcome != CacheQueryOutcome::SUCCESS) {
    RecordGetDescriptorsOutcome(quantity, query_result.outcome);
    std::move(callback).Run(query_result.GetWebResult(),
                            base::nullopt /* descriptor */);
    return;
  }

  auto descriptors = descriptors_uuid
                         ? query_result.characteristic->GetDescriptorsByUUID(
                               descriptors_uuid.value())
                         : query_result.characteristic->GetDescriptors();

  std::vector<blink::mojom::WebBluetoothRemoteGATTDescriptorPtr>
      response_descriptors;
  for (device::BluetoothRemoteGattDescriptor* descriptor : descriptors) {
    if (BluetoothBlocklist::Get().IsExcluded(descriptor->GetUUID())) {
      continue;
    }
    std::string descriptor_instance_id = descriptor->GetIdentifier();
    auto insert_result = descriptor_id_to_characteristic_id_.insert(
        {descriptor_instance_id, characteristic_instance_id});
    // If value is already in map, DCHECK it's valid.
    if (!insert_result.second)
      DCHECK(insert_result.first->second == characteristic_instance_id);

    auto descriptor_ptr(blink::mojom::WebBluetoothRemoteGATTDescriptor::New());
    descriptor_ptr->instance_id = descriptor_instance_id;
    descriptor_ptr->uuid = descriptor->GetUUID();
    response_descriptors.push_back(std::move(descriptor_ptr));

    if (quantity == blink::mojom::WebBluetoothGATTQueryQuantity::SINGLE) {
      break;
    }
  }

  if (!response_descriptors.empty()) {
    RecordGetDescriptorsOutcome(quantity, UMAGetDescriptorOutcome::SUCCESS);
    std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS,
                            std::move(response_descriptors));
    return;
  }
  RecordGetDescriptorsOutcome(
      quantity, descriptors_uuid ? UMAGetDescriptorOutcome::NOT_FOUND
                                 : UMAGetDescriptorOutcome::NO_DESCRIPTORS);
  std::move(callback).Run(
      descriptors_uuid ? blink::mojom::WebBluetoothResult::DESCRIPTOR_NOT_FOUND
                       : blink::mojom::WebBluetoothResult::NO_DESCRIPTORS_FOUND,
      base::nullopt /* descriptors */);
}

void WebBluetoothServiceImpl::RemoteCharacteristicReadValue(
    const std::string& characteristic_instance_id,
    RemoteCharacteristicReadValueCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const CacheQueryResult query_result =
      QueryCacheForCharacteristic(characteristic_instance_id);

  if (query_result.outcome == CacheQueryOutcome::BAD_RENDERER) {
    return;
  }

  if (query_result.outcome != CacheQueryOutcome::SUCCESS) {
    RecordCharacteristicReadValueOutcome(query_result.outcome);
    std::move(callback).Run(query_result.GetWebResult(),
                            base::nullopt /* value */);
    return;
  }

  if (BluetoothBlocklist::Get().IsExcludedFromReads(
          query_result.characteristic->GetUUID())) {
    RecordCharacteristicReadValueOutcome(UMAGATTOperationOutcome::BLOCKLISTED);
    std::move(callback).Run(blink::mojom::WebBluetoothResult::BLOCKLISTED_READ,
                            base::nullopt /* value */);
    return;
  }

  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto copyable_callback = AdaptCallbackForRepeating(std::move(callback));
  query_result.characteristic->ReadRemoteCharacteristic(
      base::Bind(&WebBluetoothServiceImpl::OnCharacteristicReadValueSuccess,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback),
      base::Bind(&WebBluetoothServiceImpl::OnCharacteristicReadValueFailed,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback));
}

void WebBluetoothServiceImpl::RemoteCharacteristicWriteValue(
    const std::string& characteristic_instance_id,
    const std::vector<uint8_t>& value,
    RemoteCharacteristicWriteValueCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // We perform the length check on the renderer side. So if we
  // get a value with length > 512, we can assume it's a hostile
  // renderer and kill it.
  if (value.size() > 512) {
    CrashRendererAndClosePipe(bad_message::BDH_INVALID_WRITE_VALUE_LENGTH);
    return;
  }

  const CacheQueryResult query_result =
      QueryCacheForCharacteristic(characteristic_instance_id);

  if (query_result.outcome == CacheQueryOutcome::BAD_RENDERER) {
    return;
  }

  if (query_result.outcome != CacheQueryOutcome::SUCCESS) {
    RecordCharacteristicWriteValueOutcome(query_result.outcome);
    std::move(callback).Run(query_result.GetWebResult());
    return;
  }

  if (BluetoothBlocklist::Get().IsExcludedFromWrites(
          query_result.characteristic->GetUUID())) {
    RecordCharacteristicWriteValueOutcome(UMAGATTOperationOutcome::BLOCKLISTED);
    std::move(callback).Run(
        blink::mojom::WebBluetoothResult::BLOCKLISTED_WRITE);
    return;
  }

  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  query_result.characteristic->WriteRemoteCharacteristic(
      value,
      base::Bind(&WebBluetoothServiceImpl::OnCharacteristicWriteValueSuccess,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback),
      base::Bind(&WebBluetoothServiceImpl::OnCharacteristicWriteValueFailed,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback));
}

void WebBluetoothServiceImpl::RemoteCharacteristicStartNotifications(
    const std::string& characteristic_instance_id,
    blink::mojom::WebBluetoothCharacteristicClientAssociatedPtrInfo client,
    RemoteCharacteristicStartNotificationsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto iter =
      characteristic_id_to_notify_session_.find(characteristic_instance_id);
  if (iter != characteristic_id_to_notify_session_.end() &&
      iter->second->gatt_notify_session->IsActive()) {
    // If the frame has already started notifications and the notifications
    // are active we return SUCCESS.
    std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS);
    return;
  }

  const CacheQueryResult query_result =
      QueryCacheForCharacteristic(characteristic_instance_id);

  if (query_result.outcome == CacheQueryOutcome::BAD_RENDERER) {
    return;
  }

  if (query_result.outcome != CacheQueryOutcome::SUCCESS) {
    RecordStartNotificationsOutcome(query_result.outcome);
    std::move(callback).Run(query_result.GetWebResult());
    return;
  }

  device::BluetoothRemoteGattCharacteristic::Properties notify_or_indicate =
      query_result.characteristic->GetProperties() &
      (device::BluetoothRemoteGattCharacteristic::PROPERTY_NOTIFY |
       device::BluetoothRemoteGattCharacteristic::PROPERTY_INDICATE);
  if (!notify_or_indicate) {
    std::move(callback).Run(
        blink::mojom::WebBluetoothResult::GATT_NOT_SUPPORTED);
    return;
  }

  blink::mojom::WebBluetoothCharacteristicClientAssociatedPtr
      characteristic_client;
  characteristic_client.Bind(std::move(client));

  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  query_result.characteristic->StartNotifySession(
      base::Bind(&WebBluetoothServiceImpl::OnStartNotifySessionSuccess,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Passed(&characteristic_client), copyable_callback),
      base::Bind(&WebBluetoothServiceImpl::OnStartNotifySessionFailed,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback));
}

void WebBluetoothServiceImpl::RemoteCharacteristicStopNotifications(
    const std::string& characteristic_instance_id,
    RemoteCharacteristicStopNotificationsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const CacheQueryResult query_result =
      QueryCacheForCharacteristic(characteristic_instance_id);

  if (query_result.outcome == CacheQueryOutcome::BAD_RENDERER) {
    return;
  }

  auto notify_session_iter =
      characteristic_id_to_notify_session_.find(characteristic_instance_id);
  if (notify_session_iter == characteristic_id_to_notify_session_.end()) {
    // If the frame hasn't subscribed to notifications before we just
    // run the callback.
    std::move(callback).Run();
    return;
  }
  notify_session_iter->second->gatt_notify_session->Stop(
      base::Bind(&WebBluetoothServiceImpl::OnStopNotifySessionComplete,
                 weak_ptr_factory_.GetWeakPtr(), characteristic_instance_id,
                 base::Passed(&callback)));
}

void WebBluetoothServiceImpl::RemoteDescriptorReadValue(
    const std::string& descriptor_instance_id,
    RemoteDescriptorReadValueCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const CacheQueryResult query_result =
      QueryCacheForDescriptor(descriptor_instance_id);

  if (query_result.outcome == CacheQueryOutcome::BAD_RENDERER) {
    return;
  }

  if (query_result.outcome != CacheQueryOutcome::SUCCESS) {
    RecordDescriptorReadValueOutcome(query_result.outcome);
    std::move(callback).Run(query_result.GetWebResult(),
                            base::nullopt /* value */);
    return;
  }

  if (BluetoothBlocklist::Get().IsExcludedFromReads(
          query_result.descriptor->GetUUID())) {
    RecordDescriptorReadValueOutcome(UMAGATTOperationOutcome::BLOCKLISTED);
    std::move(callback).Run(blink::mojom::WebBluetoothResult::BLOCKLISTED_READ,
                            base::nullopt /* value */);
    return;
  }

  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  query_result.descriptor->ReadRemoteDescriptor(
      base::Bind(&WebBluetoothServiceImpl::OnDescriptorReadValueSuccess,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback),
      base::Bind(&WebBluetoothServiceImpl::OnDescriptorReadValueFailed,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback));
}

void WebBluetoothServiceImpl::RemoteDescriptorWriteValue(
    const std::string& descriptor_instance_id,
    const std::vector<uint8_t>& value,
    RemoteDescriptorWriteValueCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // We perform the length check on the renderer side. So if we
  // get a value with length > 512, we can assume it's a hostile
  // renderer and kill it.
  if (value.size() > 512) {
    CrashRendererAndClosePipe(bad_message::BDH_INVALID_WRITE_VALUE_LENGTH);
    return;
  }

  const CacheQueryResult query_result =
      QueryCacheForDescriptor(descriptor_instance_id);

  if (query_result.outcome == CacheQueryOutcome::BAD_RENDERER) {
    return;
  }

  if (query_result.outcome != CacheQueryOutcome::SUCCESS) {
    RecordDescriptorWriteValueOutcome(query_result.outcome);
    std::move(callback).Run(query_result.GetWebResult());
    return;
  }

  if (BluetoothBlocklist::Get().IsExcludedFromWrites(
          query_result.descriptor->GetUUID())) {
    RecordDescriptorWriteValueOutcome(UMAGATTOperationOutcome::BLOCKLISTED);
    std::move(callback).Run(
        blink::mojom::WebBluetoothResult::BLOCKLISTED_WRITE);
    return;
  }

  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  query_result.descriptor->WriteRemoteDescriptor(
      value,
      base::Bind(&WebBluetoothServiceImpl::OnDescriptorWriteValueSuccess,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback),
      base::Bind(&WebBluetoothServiceImpl::OnDescriptorWriteValueFailed,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback));
}

void WebBluetoothServiceImpl::RequestDeviceImpl(
    blink::mojom::WebBluetoothRequestDeviceOptionsPtr options,
    RequestDeviceCallback callback,
    device::BluetoothAdapter* adapter) {
  // Calls to requestDevice() require user activation (user gestures).  We
  // should close any opened chooser when a duplicate requestDevice call is made
  // with the same user activation or when any gesture occurs outside of the
  // opened chooser. This does not happen on all platforms so we don't DCHECK
  // that the old one is closed.  We destroy the old chooser before constructing
  // the new one to make sure they can't conflict.
  device_chooser_controller_.reset();

  device_chooser_controller_.reset(
      new BluetoothDeviceChooserController(this, render_frame_host_, adapter));

  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  device_chooser_controller_->GetDevice(
      std::move(options),
      base::Bind(&WebBluetoothServiceImpl::OnGetDeviceSuccess,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback),
      base::Bind(&WebBluetoothServiceImpl::OnGetDeviceFailed,
                 weak_ptr_factory_.GetWeakPtr(), copyable_callback));
}

void WebBluetoothServiceImpl::RemoteServerGetPrimaryServicesImpl(
    const WebBluetoothDeviceId& device_id,
    blink::mojom::WebBluetoothGATTQueryQuantity quantity,
    const base::Optional<BluetoothUUID>& services_uuid,
    RemoteServerGetPrimaryServicesCallback callback,
    device::BluetoothDevice* device) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!device->IsGattConnected()) {
    // The device disconnected while discovery was pending. The returned error
    // does not matter because the renderer ignores the error if the device
    // disconnected.
    RecordGetPrimaryServicesOutcome(
        quantity, UMAGetPrimaryServiceOutcome::DEVICE_DISCONNECTED);
    std::move(callback).Run(blink::mojom::WebBluetoothResult::NO_SERVICES_FOUND,
                            base::nullopt /* services */);
    return;
  }

  DCHECK(device->IsGattServicesDiscoveryComplete());

  std::vector<device::BluetoothRemoteGattService*> services =
      services_uuid ? device->GetPrimaryServicesByUUID(services_uuid.value())
                    : device->GetPrimaryServices();

  std::vector<blink::mojom::WebBluetoothRemoteGATTServicePtr> response_services;
  for (device::BluetoothRemoteGattService* service : services) {
    if (!allowed_devices().IsAllowedToAccessService(device_id,
                                                    service->GetUUID())) {
      continue;
    }
    std::string service_instance_id = service->GetIdentifier();
    const std::string& device_address = device->GetAddress();
    auto insert_result = service_id_to_device_address_.insert(
        make_pair(service_instance_id, device_address));
    // If value is already in map, DCHECK it's valid.
    if (!insert_result.second)
      DCHECK_EQ(insert_result.first->second, device_address);

    blink::mojom::WebBluetoothRemoteGATTServicePtr service_ptr =
        blink::mojom::WebBluetoothRemoteGATTService::New();
    service_ptr->instance_id = service_instance_id;
    service_ptr->uuid = service->GetUUID();
    response_services.push_back(std::move(service_ptr));

    if (quantity == blink::mojom::WebBluetoothGATTQueryQuantity::SINGLE) {
      break;
    }
  }

  if (!response_services.empty()) {
    DVLOG(1) << "Services found in device.";
    RecordGetPrimaryServicesOutcome(quantity,
                                    UMAGetPrimaryServiceOutcome::SUCCESS);
    std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS,
                            std::move(response_services));
    return;
  }

  DVLOG(1) << "Services not found in device.";
  RecordGetPrimaryServicesOutcome(
      quantity, services_uuid ? UMAGetPrimaryServiceOutcome::NOT_FOUND
                              : UMAGetPrimaryServiceOutcome::NO_SERVICES);
  std::move(callback).Run(
      services_uuid ? blink::mojom::WebBluetoothResult::SERVICE_NOT_FOUND
                    : blink::mojom::WebBluetoothResult::NO_SERVICES_FOUND,
      base::nullopt /* services */);
}

void WebBluetoothServiceImpl::OnGetDeviceSuccess(
    RequestDeviceCallback callback,
    blink::mojom::WebBluetoothRequestDeviceOptionsPtr options,
    const std::string& device_address) {
  device_chooser_controller_.reset();

  const device::BluetoothDevice* const device =
      GetAdapter()->GetDevice(device_address);
  if (device == nullptr) {
    DVLOG(1) << "Device " << device_address << " no longer in adapter";
    RecordRequestDeviceOutcome(UMARequestDeviceOutcome::CHOSEN_DEVICE_VANISHED);
    std::move(callback).Run(
        blink::mojom::WebBluetoothResult::CHOSEN_DEVICE_VANISHED,
        nullptr /* device */);
    return;
  }

  const WebBluetoothDeviceId device_id =
      allowed_devices().AddDevice(device_address, options);

  DVLOG(1) << "Device: " << device->GetNameForDisplay();

  blink::mojom::WebBluetoothDevicePtr device_ptr =
      blink::mojom::WebBluetoothDevice::New();
  device_ptr->id = device_id;
  device_ptr->name = device->GetName();

  RecordRequestDeviceOutcome(UMARequestDeviceOutcome::SUCCESS);
  std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS,
                          std::move(device_ptr));
}

void WebBluetoothServiceImpl::OnGetDeviceFailed(
    RequestDeviceCallback callback,
    blink::mojom::WebBluetoothResult result) {
  // Errors are recorded by the *device_chooser_controller_.
  std::move(callback).Run(result, nullptr /* device */);
  device_chooser_controller_.reset();
}

void WebBluetoothServiceImpl::OnCreateGATTConnectionSuccess(
    const WebBluetoothDeviceId& device_id,
    base::TimeTicks start_time,
    blink::mojom::WebBluetoothServerClientAssociatedPtr client,
    RemoteServerConnectCallback callback,
    std::unique_ptr<device::BluetoothGattConnection> connection) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RecordConnectGATTTimeSuccess(base::TimeTicks::Now() - start_time);
  RecordConnectGATTOutcome(UMAConnectGATTOutcome::SUCCESS);

  if (connected_devices_->IsConnectedToDeviceWithId(device_id)) {
    DVLOG(1) << "Already connected.";
    std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS);
    return;
  }

  std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS);
  connected_devices_->Insert(device_id, std::move(connection),
                             std::move(client));
}

void WebBluetoothServiceImpl::OnCreateGATTConnectionFailed(
    base::TimeTicks start_time,
    RemoteServerConnectCallback callback,
    device::BluetoothDevice::ConnectErrorCode error_code) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RecordConnectGATTTimeFailed(base::TimeTicks::Now() - start_time);
  std::move(callback).Run(TranslateConnectErrorAndRecord(error_code));
}

void WebBluetoothServiceImpl::OnCharacteristicReadValueSuccess(
    RemoteCharacteristicReadValueCallback callback,
    const std::vector<uint8_t>& value) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RecordCharacteristicReadValueOutcome(UMAGATTOperationOutcome::SUCCESS);
  std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS, value);
}

void WebBluetoothServiceImpl::OnCharacteristicReadValueFailed(
    RemoteCharacteristicReadValueCallback callback,
    device::BluetoothRemoteGattService::GattErrorCode error_code) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::move(callback).Run(
      TranslateGATTErrorAndRecord(error_code,
                                  UMAGATTOperation::CHARACTERISTIC_READ),
      base::nullopt /* value */);
}

void WebBluetoothServiceImpl::OnCharacteristicWriteValueSuccess(
    RemoteCharacteristicWriteValueCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RecordCharacteristicWriteValueOutcome(UMAGATTOperationOutcome::SUCCESS);
  std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS);
}

void WebBluetoothServiceImpl::OnCharacteristicWriteValueFailed(
    RemoteCharacteristicWriteValueCallback callback,
    device::BluetoothRemoteGattService::GattErrorCode error_code) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::move(callback).Run(TranslateGATTErrorAndRecord(
      error_code, UMAGATTOperation::CHARACTERISTIC_WRITE));
}

void WebBluetoothServiceImpl::OnStartNotifySessionSuccess(
    blink::mojom::WebBluetoothCharacteristicClientAssociatedPtr client,
    RemoteCharacteristicStartNotificationsCallback callback,
    std::unique_ptr<device::BluetoothGattNotifySession> notify_session) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Copy Characteristic Instance ID before passing a unique pointer because
  // compilers may evaluate arguments in any order.
  std::string characteristic_instance_id =
      notify_session->GetCharacteristicIdentifier();

  std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS);
  // Saving the BluetoothGattNotifySession keeps notifications active.
  auto gatt_notify_session_and_client =
      std::make_unique<GATTNotifySessionAndCharacteristicClient>(
          std::move(notify_session), std::move(client));
  characteristic_id_to_notify_session_[characteristic_instance_id] =
      std::move(gatt_notify_session_and_client);
}

void WebBluetoothServiceImpl::OnStartNotifySessionFailed(
    RemoteCharacteristicStartNotificationsCallback callback,
    device::BluetoothRemoteGattService::GattErrorCode error_code) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::move(callback).Run(TranslateGATTErrorAndRecord(
      error_code, UMAGATTOperation::START_NOTIFICATIONS));
}

void WebBluetoothServiceImpl::OnStopNotifySessionComplete(
    const std::string& characteristic_instance_id,
    RemoteCharacteristicStopNotificationsCallback callback) {
  characteristic_id_to_notify_session_.erase(characteristic_instance_id);
  std::move(callback).Run();
}

void WebBluetoothServiceImpl::OnDescriptorReadValueSuccess(
    RemoteDescriptorReadValueCallback callback,
    const std::vector<uint8_t>& value) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RecordDescriptorReadValueOutcome(UMAGATTOperationOutcome::SUCCESS);
  std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS, value);
}

void WebBluetoothServiceImpl::OnDescriptorReadValueFailed(
    RemoteDescriptorReadValueCallback callback,
    device::BluetoothRemoteGattService::GattErrorCode error_code) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::move(callback).Run(TranslateGATTErrorAndRecord(
                              error_code, UMAGATTOperation::DESCRIPTOR_READ),
                          base::nullopt /* value */);
}

void WebBluetoothServiceImpl::OnDescriptorWriteValueSuccess(
    RemoteDescriptorWriteValueCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // TODO(667319): We are reporting failures to UMA but not reporting successes
  std::move(callback).Run(blink::mojom::WebBluetoothResult::SUCCESS);
}

void WebBluetoothServiceImpl::OnDescriptorWriteValueFailed(
    RemoteDescriptorWriteValueCallback callback,
    device::BluetoothRemoteGattService::GattErrorCode error_code) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RecordDescriptorWriteValueOutcome(UMAGATTOperationOutcome::SUCCESS);
  std::move(callback).Run(TranslateGATTErrorAndRecord(
      error_code, UMAGATTOperation::DESCRIPTOR_WRITE));
}

CacheQueryResult WebBluetoothServiceImpl::QueryCacheForDevice(
    const WebBluetoothDeviceId& device_id) {
  const std::string& device_address =
      allowed_devices().GetDeviceAddress(device_id);
  if (device_address.empty()) {
    CrashRendererAndClosePipe(bad_message::BDH_DEVICE_NOT_ALLOWED_FOR_ORIGIN);
    return CacheQueryResult(CacheQueryOutcome::BAD_RENDERER);
  }

  CacheQueryResult result;
  result.device = GetAdapter()->GetDevice(device_address);

  // When a device can't be found in the BluetoothAdapter, that generally
  // indicates that it's gone out of range. We reject with a NetworkError in
  // that case.
  if (result.device == nullptr) {
    result.outcome = CacheQueryOutcome::NO_DEVICE;
  }
  return result;
}

CacheQueryResult WebBluetoothServiceImpl::QueryCacheForService(
    const std::string& service_instance_id) {
  auto device_iter = service_id_to_device_address_.find(service_instance_id);

  // Kill the render, see "ID Not in Map Note" above.
  if (device_iter == service_id_to_device_address_.end()) {
    CrashRendererAndClosePipe(bad_message::BDH_INVALID_SERVICE_ID);
    return CacheQueryResult(CacheQueryOutcome::BAD_RENDERER);
  }

  const WebBluetoothDeviceId* device_id =
      allowed_devices().GetDeviceId(device_iter->second);
  // Kill the renderer if origin is not allowed to access the device.
  if (device_id == nullptr) {
    CrashRendererAndClosePipe(bad_message::BDH_DEVICE_NOT_ALLOWED_FOR_ORIGIN);
    return CacheQueryResult(CacheQueryOutcome::BAD_RENDERER);
  }

  CacheQueryResult result = QueryCacheForDevice(*device_id);
  if (result.outcome != CacheQueryOutcome::SUCCESS) {
    return result;
  }

  result.service = result.device->GetGattService(service_instance_id);
  if (result.service == nullptr) {
    result.outcome = CacheQueryOutcome::NO_SERVICE;
  } else if (!allowed_devices().IsAllowedToAccessService(
                 *device_id, result.service->GetUUID())) {
    CrashRendererAndClosePipe(bad_message::BDH_SERVICE_NOT_ALLOWED_FOR_ORIGIN);
    return CacheQueryResult(CacheQueryOutcome::BAD_RENDERER);
  }
  return result;
}

CacheQueryResult WebBluetoothServiceImpl::QueryCacheForCharacteristic(
    const std::string& characteristic_instance_id) {
  auto characteristic_iter =
      characteristic_id_to_service_id_.find(characteristic_instance_id);

  // Kill the render, see "ID Not in Map Note" above.
  if (characteristic_iter == characteristic_id_to_service_id_.end()) {
    CrashRendererAndClosePipe(bad_message::BDH_INVALID_CHARACTERISTIC_ID);
    return CacheQueryResult(CacheQueryOutcome::BAD_RENDERER);
  }

  CacheQueryResult result = QueryCacheForService(characteristic_iter->second);

  if (result.outcome != CacheQueryOutcome::SUCCESS) {
    return result;
  }

  result.characteristic =
      result.service->GetCharacteristic(characteristic_instance_id);

  if (result.characteristic == nullptr) {
    result.outcome = CacheQueryOutcome::NO_CHARACTERISTIC;
  }

  return result;
}

CacheQueryResult WebBluetoothServiceImpl::QueryCacheForDescriptor(
    const std::string& descriptor_instance_id) {
  auto descriptor_iter =
      descriptor_id_to_characteristic_id_.find(descriptor_instance_id);

  // Kill the render, see "ID Not in Map Note" above.
  if (descriptor_iter == descriptor_id_to_characteristic_id_.end()) {
    CrashRendererAndClosePipe(bad_message::BDH_INVALID_DESCRIPTOR_ID);
    return CacheQueryResult(CacheQueryOutcome::BAD_RENDERER);
  }

  CacheQueryResult result =
      QueryCacheForCharacteristic(descriptor_iter->second);

  if (result.outcome != CacheQueryOutcome::SUCCESS) {
    return result;
  }

  result.descriptor =
      result.characteristic->GetDescriptor(descriptor_instance_id);

  if (result.descriptor == nullptr) {
    result.outcome = CacheQueryOutcome::NO_DESCRIPTOR;
  }

  return result;
}

void WebBluetoothServiceImpl::RunPendingPrimaryServicesRequests(
    device::BluetoothDevice* device) {
  const std::string& device_address = device->GetAddress();

  auto iter = pending_primary_services_requests_.find(device_address);
  if (iter == pending_primary_services_requests_.end()) {
    return;
  }
  std::vector<PrimaryServicesRequestCallback> requests =
      std::move(iter->second);
  pending_primary_services_requests_.erase(iter);

  for (PrimaryServicesRequestCallback& request : requests) {
    std::move(request).Run(device);
  }

  // Sending get-service responses unexpectedly queued another request.
  DCHECK(
      !base::ContainsKey(pending_primary_services_requests_, device_address));
}

RenderProcessHost* WebBluetoothServiceImpl::GetRenderProcessHost() {
  return render_frame_host_->GetProcess();
}

device::BluetoothAdapter* WebBluetoothServiceImpl::GetAdapter() {
  return BluetoothAdapterFactoryWrapper::Get().GetAdapter(this);
}

void WebBluetoothServiceImpl::CrashRendererAndClosePipe(
    bad_message::BadMessageReason reason) {
  bad_message::ReceivedBadMessage(GetRenderProcessHost(), reason);
  binding_.Close();
}

url::Origin WebBluetoothServiceImpl::GetOrigin() {
  return render_frame_host_->GetLastCommittedOrigin();
}

BluetoothAllowedDevices& WebBluetoothServiceImpl::allowed_devices() {
  StoragePartitionImpl* partition = static_cast<StoragePartitionImpl*>(
      BrowserContext::GetDefaultStoragePartition(
          web_contents()->GetBrowserContext()));
  scoped_refptr<BluetoothAllowedDevicesMap> allowed_devices_map =
      partition->GetBluetoothAllowedDevicesMap();
  return allowed_devices_map->GetOrCreateAllowedDevices(GetOrigin());
}

void WebBluetoothServiceImpl::ClearState() {
  characteristic_id_to_notify_session_.clear();
  pending_primary_services_requests_.clear();
  descriptor_id_to_characteristic_id_.clear();
  characteristic_id_to_service_id_.clear();
  service_id_to_device_address_.clear();
  connected_devices_.reset(
      new FrameConnectedBluetoothDevices(render_frame_host_));
  device_chooser_controller_.reset();
  BluetoothAdapterFactoryWrapper::Get().ReleaseAdapter(this);
}

}  // namespace content
