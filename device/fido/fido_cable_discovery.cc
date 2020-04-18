// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_cable_discovery.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/strings/stringprintf.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "build/build_config.h"
#include "device/bluetooth/bluetooth_advertisement.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/fido/fido_ble_uuids.h"
#include "device/fido/fido_cable_device.h"
#include "device/fido/fido_parsing_utils.h"

namespace device {

namespace {

#if defined(OS_MACOSX)

// Convert byte array into GUID formatted string as defined by RFC 4122.
// As we are converting 128 bit UUID, |bytes| must be have length of 16.
// https://tools.ietf.org/html/rfc4122
std::string ConvertBytesToUuid(base::span<const uint8_t, 16> bytes) {
  uint64_t most_significant_bytes = 0;
  for (size_t i = 0; i < sizeof(uint64_t); i++) {
    most_significant_bytes |= base::strict_cast<uint64_t>(bytes[i])
                              << 8 * (7 - i);
  }

  uint64_t least_significant_bytes = 0;
  for (size_t i = 0; i < sizeof(uint64_t); i++) {
    least_significant_bytes |= base::strict_cast<uint64_t>(bytes[i + 8])
                               << 8 * (7 - i);
  }

  return base::StringPrintf(
      "%08x-%04x-%04x-%04x-%012llx",
      static_cast<unsigned int>(most_significant_bytes >> 32),
      static_cast<unsigned int>((most_significant_bytes >> 16) & 0x0000ffff),
      static_cast<unsigned int>(most_significant_bytes & 0x0000ffff),
      static_cast<unsigned int>(least_significant_bytes >> 48),
      least_significant_bytes & 0x0000ffff'ffffffffULL);
}

#endif

const BluetoothUUID& CableAdvertisementUUID() {
  static const BluetoothUUID service_uuid(kCableAdvertisementUUID);
  return service_uuid;
}

bool IsCableDevice(const BluetoothDevice* device) {
  return base::ContainsKey(device->GetServiceData(), CableAdvertisementUUID());
}

// Construct advertisement data with different formats depending on client's
// operating system. Ideally, we advertise EIDs as part of Service Data, but
// this isn't available on all platforms. On Windows we use Manufacturer Data
// instead, and on Mac our only option is to advertise an additional service
// with the EID as its UUID.
std::unique_ptr<BluetoothAdvertisement::Data> ConstructAdvertisementData(
    uint8_t version_number,
    base::span<const uint8_t, FidoCableDiscovery::kEphemeralIdSize>
        client_eid) {
  auto advertisement_data = std::make_unique<BluetoothAdvertisement::Data>(
      BluetoothAdvertisement::AdvertisementType::ADVERTISEMENT_TYPE_BROADCAST);

#if defined(OS_MACOSX)
  auto list = std::make_unique<BluetoothAdvertisement::UUIDList>();
  list->emplace_back(kCableAdvertisementUUID);
  list->emplace_back(ConvertBytesToUuid(client_eid));
  advertisement_data->set_service_uuids(std::move(list));

#elif defined(OS_WIN)
  constexpr uint16_t kFidoManufacturerId = 0xFFFD;
  constexpr std::array<uint8_t, 2> kFidoManufacturerDataHeader = {0x51, 0xFE};

  auto manufacturer_data =
      std::make_unique<BluetoothAdvertisement::ManufacturerData>();
  std::vector<uint8_t> manufacturer_data_value;
  fido_parsing_utils::Append(&manufacturer_data_value,
                             kFidoManufacturerDataHeader);
  fido_parsing_utils::Append(&manufacturer_data_value, client_eid);
  manufacturer_data->emplace(kFidoManufacturerId,
                             std::move(manufacturer_data_value));
  advertisement_data->set_manufacturer_data(std::move(manufacturer_data));

#elif defined(OS_LINUX) || defined(OS_CHROMEOS)
  // Service data for ChromeOS and Linux is 1 byte corresponding to Cable
  // version number, followed by 7 empty(0x00) bytes, followed by 16 bytes
  // corresponding to client EID.
  auto service_data = std::make_unique<BluetoothAdvertisement::ServiceData>();
  std::vector<uint8_t> service_data_value(24, 0);
  service_data_value[0] = version_number;
  std::copy(client_eid.begin(), client_eid.end(),
            service_data_value.begin() + 8);
  service_data->emplace(kCableAdvertisementUUID, std::move(service_data_value));
  advertisement_data->set_service_data(std::move(service_data));
#endif

  return advertisement_data;
}

}  // namespace

// FidoCableDiscovery::CableDiscoveryData -------------------------------------

FidoCableDiscovery::CableDiscoveryData::CableDiscoveryData(
    uint8_t version,
    const EidArray& client_eid,
    const EidArray& authenticator_eid,
    const SessionKeyArray& session_key)
    : version(version),
      client_eid(client_eid),
      authenticator_eid(authenticator_eid),
      session_key(session_key) {}

FidoCableDiscovery::CableDiscoveryData::CableDiscoveryData(
    const CableDiscoveryData& data) = default;

FidoCableDiscovery::CableDiscoveryData& FidoCableDiscovery::CableDiscoveryData::
operator=(const CableDiscoveryData& other) = default;

FidoCableDiscovery::CableDiscoveryData::~CableDiscoveryData() = default;

// FidoCableDiscovery ---------------------------------------------------------

FidoCableDiscovery::FidoCableDiscovery(
    std::vector<CableDiscoveryData> discovery_data)
    : discovery_data_(std::move(discovery_data)), weak_factory_(this) {}

// This is a workaround for https://crbug.com/846522
FidoCableDiscovery::~FidoCableDiscovery() {
  for (auto advertisement : advertisements_)
    advertisement.second->Unregister(base::DoNothing(), base::DoNothing());
}

void FidoCableDiscovery::DeviceAdded(BluetoothAdapter* adapter,
                                     BluetoothDevice* device) {
  if (!IsCableDevice(device))
    return;

  DVLOG(2) << "Discovered Cable device: " << device->GetAddress();
  CableDeviceFound(adapter, device);
}

void FidoCableDiscovery::DeviceChanged(BluetoothAdapter* adapter,
                                       BluetoothDevice* device) {
  if (!IsCableDevice(device))
    return;

  DVLOG(2) << "Device changed for Cable device: " << device->GetAddress();
  CableDeviceFound(adapter, device);
}

void FidoCableDiscovery::DeviceRemoved(BluetoothAdapter* adapter,
                                       BluetoothDevice* device) {
  if (IsCableDevice(device) && GetFoundCableDiscoveryData(device)) {
    const auto& device_address = device->GetAddress();
    VLOG(2) << "Cable device removed: " << device_address;
    RemoveDevice(FidoBleDevice::GetId(device_address));
  }
}

void FidoCableDiscovery::OnSetPowered() {
  DCHECK(adapter());

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&FidoCableDiscovery::StartAdvertisement,
                                weak_factory_.GetWeakPtr()));
}

void FidoCableDiscovery::StartAdvertisement() {
  DCHECK(adapter());

  for (const auto& data : discovery_data_) {
    adapter()->RegisterAdvertisement(
        ConstructAdvertisementData(data.version, data.client_eid),
        base::AdaptCallbackForRepeating(
            base::BindOnce(&FidoCableDiscovery::OnAdvertisementRegistered,
                           weak_factory_.GetWeakPtr(), data.client_eid)),
        base::AdaptCallbackForRepeating(
            base::BindOnce(&FidoCableDiscovery::OnAdvertisementRegisterError,
                           weak_factory_.GetWeakPtr())));
  }
}

void FidoCableDiscovery::OnAdvertisementRegistered(
    const EidArray& client_eid,
    scoped_refptr<BluetoothAdvertisement> advertisement) {
  DVLOG(2) << "Advertisement registered.";
  advertisements_.emplace(client_eid, std::move(advertisement));
  RecordAdvertisementResult(true /* is_success */);
}

void FidoCableDiscovery::OnAdvertisementRegisterError(
    BluetoothAdvertisement::ErrorCode error_code) {
  DLOG(ERROR) << "Failed to register advertisement: " << error_code;
  RecordAdvertisementResult(false /* is_success */);
}

void FidoCableDiscovery::RecordAdvertisementResult(bool is_success) {
  is_success ? ++advertisement_success_counter_
             : ++advertisement_failure_counter_;

  // Wait until all advertisements are sent out.
  if (advertisement_success_counter_ + advertisement_failure_counter_ !=
      discovery_data_.size()) {
    return;
  }

  // No advertisements succeeded, no point in starting scanning.
  if (!advertisement_success_counter_) {
    NotifyDiscoveryStarted(false);
    return;
  }

  // At least one advertisement succeeded and all advertisement has been
  // processed. Start scanning.
  adapter()->StartDiscoverySessionWithFilter(
      std::make_unique<BluetoothDiscoveryFilter>(
          BluetoothTransport::BLUETOOTH_TRANSPORT_LE),
      base::AdaptCallbackForRepeating(
          base::BindOnce(&FidoCableDiscovery::OnStartDiscoverySessionWithFilter,
                         weak_factory_.GetWeakPtr())),
      base::AdaptCallbackForRepeating(
          base::BindOnce(&FidoCableDiscovery::OnStartDiscoverySessionError,
                         weak_factory_.GetWeakPtr())));
}

void FidoCableDiscovery::CableDeviceFound(BluetoothAdapter* adapter,
                                          BluetoothDevice* device) {
  const auto* found_cable_device_data = GetFoundCableDiscoveryData(device);
  if (!found_cable_device_data)
    return;

  DVLOG(2) << "Found new Cable device.";
  // Nonce is embedded as first 8 bytes of client EID.
  std::array<uint8_t, 8> nonce;
  bool extract_success = fido_parsing_utils::ExtractArray(
      found_cable_device_data->client_eid, 0, &nonce);
  if (!extract_success)
    return;

  AddDevice(std::make_unique<FidoCableDevice>(
      device->GetAddress(),
      std::string(found_cable_device_data->session_key.begin(),
                  found_cable_device_data->session_key.end()),
      nonce));
}

const FidoCableDiscovery::CableDiscoveryData*
FidoCableDiscovery::GetFoundCableDiscoveryData(
    const BluetoothDevice* device) const {
  const auto* service_data =
      device->GetServiceDataForUUID(CableAdvertisementUUID());
  DCHECK(service_data);

  EidArray received_authenticator_eid;
  bool extract_success = fido_parsing_utils::ExtractArray(
      *service_data, 8, &received_authenticator_eid);
  if (!extract_success)
    return nullptr;

  auto discovery_data_iterator = std::find_if(
      discovery_data_.begin(), discovery_data_.end(),
      [&received_authenticator_eid](const auto& data) {
        return received_authenticator_eid == data.authenticator_eid;
      });

  return discovery_data_iterator != discovery_data_.end()
             ? &(*discovery_data_iterator)
             : nullptr;
}

}  // namespace device
