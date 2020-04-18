// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_ble_connection.h"

#include <utility>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"
#include "device/bluetooth/bluetooth_gatt_notify_session.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_service.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/fido/fido_ble_uuids.h"

namespace device {

namespace {

constexpr const char* ToString(BluetoothDevice::ConnectErrorCode error_code) {
  switch (error_code) {
    case BluetoothDevice::ERROR_AUTH_CANCELED:
      return "ERROR_AUTH_CANCELED";
    case BluetoothDevice::ERROR_AUTH_FAILED:
      return "ERROR_AUTH_FAILED";
    case BluetoothDevice::ERROR_AUTH_REJECTED:
      return "ERROR_AUTH_REJECTED";
    case BluetoothDevice::ERROR_AUTH_TIMEOUT:
      return "ERROR_AUTH_TIMEOUT";
    case BluetoothDevice::ERROR_FAILED:
      return "ERROR_FAILED";
    case BluetoothDevice::ERROR_INPROGRESS:
      return "ERROR_INPROGRESS";
    case BluetoothDevice::ERROR_UNKNOWN:
      return "ERROR_UNKNOWN";
    case BluetoothDevice::ERROR_UNSUPPORTED_DEVICE:
      return "ERROR_UNSUPPORTED_DEVICE";
    default:
      NOTREACHED();
      return "";
  }
}

constexpr const char* ToString(BluetoothGattService::GattErrorCode error_code) {
  switch (error_code) {
    case BluetoothGattService::GATT_ERROR_UNKNOWN:
      return "GATT_ERROR_UNKNOWN";
    case BluetoothGattService::GATT_ERROR_FAILED:
      return "GATT_ERROR_FAILED";
    case BluetoothGattService::GATT_ERROR_IN_PROGRESS:
      return "GATT_ERROR_IN_PROGRESS";
    case BluetoothGattService::GATT_ERROR_INVALID_LENGTH:
      return "GATT_ERROR_INVALID_LENGTH";
    case BluetoothGattService::GATT_ERROR_NOT_PERMITTED:
      return "GATT_ERROR_NOT_PERMITTED";
    case BluetoothGattService::GATT_ERROR_NOT_AUTHORIZED:
      return "GATT_ERROR_NOT_AUTHORIZED";
    case BluetoothGattService::GATT_ERROR_NOT_PAIRED:
      return "GATT_ERROR_NOT_PAIRED";
    case BluetoothGattService::GATT_ERROR_NOT_SUPPORTED:
      return "GATT_ERROR_NOT_SUPPORTED";
    default:
      NOTREACHED();
      return "";
  }
}

}  // namespace

FidoBleConnection::FidoBleConnection(std::string device_address)
    : address_(std::move(device_address)), weak_factory_(this) {}

FidoBleConnection::FidoBleConnection(
    std::string device_address,
    ConnectionStatusCallback connection_status_callback,
    ReadCallback read_callback)
    : address_(std::move(device_address)),
      connection_status_callback_(std::move(connection_status_callback)),
      read_callback_(std::move(read_callback)),
      weak_factory_(this) {
  DCHECK(!address_.empty());
}

FidoBleConnection::~FidoBleConnection() {
  if (adapter_)
    adapter_->RemoveObserver(this);
}

void FidoBleConnection::Connect() {
  BluetoothAdapterFactory::GetAdapter(
      base::Bind(&FidoBleConnection::OnGetAdapter, weak_factory_.GetWeakPtr()));
}

void FidoBleConnection::ReadControlPointLength(
    ControlPointLengthCallback callback) {
  const BluetoothRemoteGattService* u2f_service = GetFidoService();
  if (!u2f_service) {
    std::move(callback).Run(base::nullopt);
    return;
  }

  BluetoothRemoteGattCharacteristic* control_point_length =
      u2f_service->GetCharacteristic(*control_point_length_id_);
  if (!control_point_length) {
    DLOG(ERROR) << "No Control Point Length characteristic present.";
    std::move(callback).Run(base::nullopt);
    return;
  }

  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  control_point_length->ReadRemoteCharacteristic(
      base::Bind(OnReadControlPointLength, copyable_callback),
      base::Bind(OnReadControlPointLengthError, copyable_callback));
}

void FidoBleConnection::ReadServiceRevisions(
    ServiceRevisionsCallback callback) {
  const BluetoothRemoteGattService* u2f_service = GetFidoService();
  if (!u2f_service) {
    std::move(callback).Run({});
    return;
  }

  DCHECK(service_revision_id_ || service_revision_bitfield_id_);
  BluetoothRemoteGattCharacteristic* service_revision =
      service_revision_id_
          ? u2f_service->GetCharacteristic(*service_revision_id_)
          : nullptr;

  BluetoothRemoteGattCharacteristic* service_revision_bitfield =
      service_revision_bitfield_id_
          ? u2f_service->GetCharacteristic(*service_revision_bitfield_id_)
          : nullptr;

  if (!service_revision && !service_revision_bitfield) {
    DLOG(ERROR) << "Service Revision Characteristics do not exist.";
    std::move(callback).Run({});
    return;
  }

  // Start from a clean state.
  service_revisions_.clear();

  // In order to obtain the full set of supported service revisions it is
  // possible that both the |service_revision_| and |service_revision_bitfield_|
  // characteristics must be read. Potentially we need to take the union of
  // the individually supported service revisions, hence the indirection to
  // ReturnServiceRevisions() is introduced.
  base::Closure copyable_callback = base::AdaptCallbackForRepeating(
      base::BindOnce(&FidoBleConnection::ReturnServiceRevisions,
                     weak_factory_.GetWeakPtr(), std::move(callback)));

  // If the Service Revision Bitfield characteristic is not present, only
  // attempt to read the Service Revision characteristic.
  if (!service_revision_bitfield) {
    service_revision->ReadRemoteCharacteristic(
        base::Bind(&FidoBleConnection::OnReadServiceRevision,
                   weak_factory_.GetWeakPtr(), copyable_callback),
        base::Bind(&FidoBleConnection::OnReadServiceRevisionError,
                   weak_factory_.GetWeakPtr(), copyable_callback));
    return;
  }

  // If the Service Revision characteristic is not present, only
  // attempt to read the Service Revision Bitfield characteristic.
  if (!service_revision) {
    service_revision_bitfield->ReadRemoteCharacteristic(
        base::Bind(&FidoBleConnection::OnReadServiceRevisionBitfield,
                   weak_factory_.GetWeakPtr(), copyable_callback),
        base::Bind(&FidoBleConnection::OnReadServiceRevisionBitfieldError,
                   weak_factory_.GetWeakPtr(), copyable_callback));
    return;
  }

  // This is the case where both characteristics are present. These reads can
  // happen in parallel, but both must finish before a result can be returned.
  // Hence a BarrierClosure is introduced invoking ReturnServiceRevisions() once
  // both characteristic reads are done.
  base::RepeatingClosure barrier_closure =
      base::BarrierClosure(2, copyable_callback);

  service_revision->ReadRemoteCharacteristic(
      base::Bind(&FidoBleConnection::OnReadServiceRevision,
                 weak_factory_.GetWeakPtr(), barrier_closure),
      base::Bind(&FidoBleConnection::OnReadServiceRevisionError,
                 weak_factory_.GetWeakPtr(), barrier_closure));

  service_revision_bitfield->ReadRemoteCharacteristic(
      base::Bind(&FidoBleConnection::OnReadServiceRevisionBitfield,
                 weak_factory_.GetWeakPtr(), barrier_closure),
      base::Bind(&FidoBleConnection::OnReadServiceRevisionBitfieldError,
                 weak_factory_.GetWeakPtr(), barrier_closure));
}

void FidoBleConnection::WriteControlPoint(const std::vector<uint8_t>& data,
                                          WriteCallback callback) {
  const BluetoothRemoteGattService* u2f_service = GetFidoService();
  if (!u2f_service) {
    std::move(callback).Run(false);
    return;
  }

  BluetoothRemoteGattCharacteristic* control_point =
      u2f_service->GetCharacteristic(*control_point_id_);
  if (!control_point) {
    DLOG(ERROR) << "Control Point characteristic not present.";
    std::move(callback).Run(false);
    return;
  }

  // Attempt a write without response for performance reasons. Fall back to a
  // confirmed write in case of failure, e.g. when the characteristic does not
  // provide the required property.
  if (control_point->WriteWithoutResponse(data)) {
    DVLOG(2) << "Write without response succeeded.";
    std::move(callback).Run(true);
  } else {
    auto copyable_callback =
        base::AdaptCallbackForRepeating(std::move(callback));
    control_point->WriteRemoteCharacteristic(
        data, base::Bind(OnWrite, copyable_callback),
        base::Bind(OnWriteError, copyable_callback));
  }
}

void FidoBleConnection::WriteServiceRevision(ServiceRevision service_revision,
                                             WriteCallback callback) {
  const BluetoothRemoteGattService* u2f_service = GetFidoService();
  if (!u2f_service) {
    std::move(callback).Run(false);
    return;
  }

  BluetoothRemoteGattCharacteristic* service_revision_bitfield =
      u2f_service->GetCharacteristic(*service_revision_bitfield_id_);
  if (!service_revision_bitfield) {
    DLOG(ERROR) << "Service Revision Bitfield characteristic not present.";
    std::move(callback).Run(false);
    return;
  }

  std::vector<uint8_t> payload;
  switch (service_revision) {
    case ServiceRevision::VERSION_1_1:
      payload.push_back(0x80);
      break;
    case ServiceRevision::VERSION_1_2:
      payload.push_back(0x40);
      break;
    default:
      DLOG(ERROR)
          << "Write Service Revision Failed: Unsupported Service Revision.";
      std::move(callback).Run(false);
      return;
  }

  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  service_revision_bitfield->WriteRemoteCharacteristic(
      payload, base::Bind(OnWrite, copyable_callback),
      base::Bind(OnWriteError, copyable_callback));
}

void FidoBleConnection::OnGetAdapter(scoped_refptr<BluetoothAdapter> adapter) {
  if (!adapter) {
    DLOG(ERROR) << "Failed to get Adapter.";
    OnConnectionError();
    return;
  }

  DVLOG(2) << "Got Adapter: " << adapter->GetAddress();
  adapter_ = std::move(adapter);
  adapter_->AddObserver(this);
  CreateGattConnection();
}

void FidoBleConnection::CreateGattConnection() {
  BluetoothDevice* device = adapter_->GetDevice(address_);
  if (!device) {
    DLOG(ERROR) << "Failed to get Device.";
    OnConnectionError();
    return;
  }

  device->CreateGattConnection(
      base::Bind(&FidoBleConnection::OnCreateGattConnection,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&FidoBleConnection::OnCreateGattConnectionError,
                 weak_factory_.GetWeakPtr()));
}

void FidoBleConnection::OnCreateGattConnection(
    std::unique_ptr<BluetoothGattConnection> connection) {
  connection_ = std::move(connection);

  BluetoothDevice* device = adapter_->GetDevice(address_);
  if (!device) {
    DLOG(ERROR) << "Failed to get Device.";
    OnConnectionError();
    return;
  }

  if (device->IsGattServicesDiscoveryComplete())
    ConnectToU2fService();
}

void FidoBleConnection::OnCreateGattConnectionError(
    BluetoothDevice::ConnectErrorCode error_code) {
  DLOG(ERROR) << "CreateGattConnection() failed: " << ToString(error_code);
  OnConnectionError();
}

void FidoBleConnection::ConnectToU2fService() {
  BluetoothDevice* device = adapter_->GetDevice(address_);
  if (!device) {
    DLOG(ERROR) << "Failed to get Device.";
    OnConnectionError();
    return;
  }

  DCHECK(device->IsGattServicesDiscoveryComplete());
  const std::vector<BluetoothRemoteGattService*> services =
      device->GetGattServices();
  auto found =
      std::find_if(services.begin(), services.end(), [](const auto* service) {
        return service->GetUUID().canonical_value() == kFidoServiceUUID;
      });

  if (found == services.end()) {
    DLOG(ERROR) << "Failed to get U2F Service.";
    OnConnectionError();
    return;
  }

  const BluetoothRemoteGattService* u2f_service = *found;
  u2f_service_id_ = u2f_service->GetIdentifier();
  DVLOG(2) << "Got U2F Service: " << *u2f_service_id_;

  for (const auto* characteristic : u2f_service->GetCharacteristics()) {
    // NOTE: Since GetUUID() returns a temporary |uuid| can't be a reference,
    // even though canonical_value() returns a const reference.
    const std::string uuid = characteristic->GetUUID().canonical_value();
    if (uuid == kFidoControlPointLengthUUID) {
      control_point_length_id_ = characteristic->GetIdentifier();
      DVLOG(2) << "Got U2F Control Point Length: " << *control_point_length_id_;
    } else if (uuid == kFidoControlPointUUID) {
      control_point_id_ = characteristic->GetIdentifier();
      DVLOG(2) << "Got U2F Control Point: " << *control_point_id_;
    } else if (uuid == kFidoStatusUUID) {
      status_id_ = characteristic->GetIdentifier();
      DVLOG(2) << "Got U2F Status: " << *status_id_;
    } else if (uuid == kFidoServiceRevisionUUID) {
      service_revision_id_ = characteristic->GetIdentifier();
      DVLOG(2) << "Got U2F Service Revision: " << *service_revision_id_;
    } else if (uuid == kFidoServiceRevisionBitfieldUUID) {
      service_revision_bitfield_id_ = characteristic->GetIdentifier();
      DVLOG(2) << "Got U2F Service Revision Bitfield: "
               << *service_revision_bitfield_id_;
    }
  }

  if (!control_point_length_id_ || !control_point_id_ || !status_id_ ||
      (!service_revision_id_ && !service_revision_bitfield_id_)) {
    DLOG(ERROR) << "U2F characteristics missing.";
    OnConnectionError();
    return;
  }

  u2f_service->GetCharacteristic(*status_id_)
      ->StartNotifySession(
          base::Bind(&FidoBleConnection::OnStartNotifySession,
                     weak_factory_.GetWeakPtr()),
          base::Bind(&FidoBleConnection::OnStartNotifySessionError,
                     weak_factory_.GetWeakPtr()));
}

void FidoBleConnection::OnStartNotifySession(
    std::unique_ptr<BluetoothGattNotifySession> notify_session) {
  notify_session_ = std::move(notify_session);
  DVLOG(2) << "Created notification session. Connection established.";
  connection_status_callback_.Run(true);
}

void FidoBleConnection::OnStartNotifySessionError(
    BluetoothGattService::GattErrorCode error_code) {
  DLOG(ERROR) << "StartNotifySession() failed: " << ToString(error_code);
  OnConnectionError();
}

void FidoBleConnection::OnConnectionError() {
  connection_status_callback_.Run(false);

  connection_.reset();
  notify_session_.reset();

  u2f_service_id_.reset();
  control_point_length_id_.reset();
  control_point_id_.reset();
  status_id_.reset();
  service_revision_id_.reset();
  service_revision_bitfield_id_.reset();
}

const BluetoothRemoteGattService* FidoBleConnection::GetFidoService() const {
  if (!adapter_) {
    DLOG(ERROR) << "No adapter present.";
    return nullptr;
  }

  const BluetoothDevice* device = adapter_->GetDevice(address_);
  if (!device) {
    DLOG(ERROR) << "No device present.";
    return nullptr;
  }

  if (!u2f_service_id_) {
    DLOG(ERROR) << "Unknown U2F service id.";
    return nullptr;
  }

  const BluetoothRemoteGattService* u2f_service =
      device->GetGattService(*u2f_service_id_);
  if (!u2f_service) {
    DLOG(ERROR) << "No U2F service present.";
    return nullptr;
  }

  return u2f_service;
}

void FidoBleConnection::DeviceAdded(BluetoothAdapter* adapter,
                                    BluetoothDevice* device) {
  if (adapter != adapter_ || device->GetAddress() != address_)
    return;
  CreateGattConnection();
}

void FidoBleConnection::DeviceAddressChanged(BluetoothAdapter* adapter,
                                             BluetoothDevice* device,
                                             const std::string& old_address) {
  if (adapter != adapter_ || old_address != address_)
    return;
  address_ = device->GetAddress();
}

void FidoBleConnection::DeviceChanged(BluetoothAdapter* adapter,
                                      BluetoothDevice* device) {
  if (adapter != adapter_ || device->GetAddress() != address_)
    return;
  if (connection_ && !device->IsGattConnected()) {
    DLOG(ERROR) << "GATT Disconnected: " << device->GetAddress();
    OnConnectionError();
  }
}

void FidoBleConnection::GattCharacteristicValueChanged(
    BluetoothAdapter* adapter,
    BluetoothRemoteGattCharacteristic* characteristic,
    const std::vector<uint8_t>& value) {
  if (characteristic->GetIdentifier() != status_id_)
    return;
  DVLOG(2) << "Status characteristic value changed.";
  read_callback_.Run(value);
}

void FidoBleConnection::GattServicesDiscovered(BluetoothAdapter* adapter,
                                               BluetoothDevice* device) {
  if (adapter != adapter_ || device->GetAddress() != address_)
    return;
  ConnectToU2fService();
}

// static
void FidoBleConnection::OnReadControlPointLength(
    ControlPointLengthCallback callback,
    const std::vector<uint8_t>& value) {
  if (value.size() != 2) {
    DLOG(ERROR) << "Wrong Control Point Length: " << value.size() << " bytes";
    std::move(callback).Run(base::nullopt);
    return;
  }

  uint16_t length = (value[0] << 8) | value[1];
  DVLOG(2) << "Control Point Length: " << length;
  std::move(callback).Run(length);
}

// static
void FidoBleConnection::OnReadControlPointLengthError(
    ControlPointLengthCallback callback,
    BluetoothGattService::GattErrorCode error_code) {
  DLOG(ERROR) << "Error reading Control Point Length: " << ToString(error_code);
  std::move(callback).Run(base::nullopt);
}

void FidoBleConnection::OnReadServiceRevision(
    base::OnceClosure callback,
    const std::vector<uint8_t>& value) {
  std::string service_revision(value.begin(), value.end());
  DVLOG(2) << "Service Revision: " << service_revision;

  if (service_revision == "1.0") {
    service_revisions_.insert(ServiceRevision::VERSION_1_0);
  } else if (service_revision == "1.1") {
    service_revisions_.insert(ServiceRevision::VERSION_1_1);
  } else if (service_revision == "1.2") {
    service_revisions_.insert(ServiceRevision::VERSION_1_2);
  } else {
    DLOG(ERROR) << "Unknown Service Revision: " << service_revision;
    std::move(callback).Run();
    return;
  }

  std::move(callback).Run();
}

void FidoBleConnection::OnReadServiceRevisionError(
    base::OnceClosure callback,
    BluetoothGattService::GattErrorCode error_code) {
  DLOG(ERROR) << "Error reading Service Revision: " << ToString(error_code);
  std::move(callback).Run();
}

void FidoBleConnection::OnReadServiceRevisionBitfield(
    base::OnceClosure callback,
    const std::vector<uint8_t>& value) {
  if (value.empty()) {
    DLOG(ERROR) << "Service Revision Bitfield is empty.";
    std::move(callback).Run();
    return;
  }

  if (value.size() != 1u) {
    DLOG(ERROR) << "Service Revision Bitfield has unexpected size: "
                << value.size() << ". Ignoring all but the first byte.";
  }

  const uint8_t bitset = value[0];
  if (bitset & 0x3F) {
    DLOG(ERROR) << "Service Revision Bitfield has unexpected bits set: 0x"
                << std::hex << (bitset & 0x3F)
                << ". Ignoring all but the first two bits.";
  }

  if (bitset & 0x80) {
    service_revisions_.insert(ServiceRevision::VERSION_1_1);
    DVLOG(2) << "Detected Support for Service Revision 1.1";
  }

  if (bitset & 0x40) {
    service_revisions_.insert(ServiceRevision::VERSION_1_2);
    DVLOG(2) << "Detected Support for Service Revision 1.2";
  }

  std::move(callback).Run();
}

void FidoBleConnection::OnReadServiceRevisionBitfieldError(
    base::OnceClosure callback,
    BluetoothGattService::GattErrorCode error_code) {
  DLOG(ERROR) << "Error reading Service Revision Bitfield: "
              << ToString(error_code);
  std::move(callback).Run();
}

void FidoBleConnection::ReturnServiceRevisions(
    ServiceRevisionsCallback callback) {
  std::move(callback).Run(std::move(service_revisions_));
}

// static
void FidoBleConnection::OnWrite(WriteCallback callback) {
  DVLOG(2) << "Write succeeded.";
  std::move(callback).Run(true);
}

// static
void FidoBleConnection::OnWriteError(
    WriteCallback callback,
    BluetoothGattService::GattErrorCode error_code) {
  DLOG(ERROR) << "Write Failed: " << ToString(error_code);
  std::move(callback).Run(false);
}

}  // namespace device
