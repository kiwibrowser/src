// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_cable_discovery.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/stl_util.h"
#include "build/build_config.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_advertisement.h"
#include "device/bluetooth/test/bluetooth_test.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "device/fido/fido_ble_device.h"
#include "device/fido/fido_ble_uuids.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/mock_fido_discovery_observer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::NiceMock;

namespace device {

namespace {

constexpr uint8_t kTestCableVersionNumber = 0x01;

// Constants required for discovering and constructing a Cable device that
// are given by the relying party via an extension.
constexpr FidoCableDiscovery::EidArray kClientEid = {
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11,
     0x12, 0x13, 0x14, 0x15}};

constexpr char kUuidFormattedClientEid[] =
    "00010203-0405-0607-0809-101112131415";

constexpr FidoCableDiscovery::EidArray kAuthenticatorEid = {
    {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
     0x01, 0x01, 0x01, 0x01}};

constexpr FidoCableDiscovery::EidArray kInvalidAuthenticatorEid = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00}};

constexpr FidoCableDiscovery::SessionKeyArray kTestSessionKey = {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

constexpr FidoCableDiscovery::EidArray kSecondaryClientEid = {
    {0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04,
     0x03, 0x02, 0x01, 0x00}};

constexpr char kUuidFormattedSecondaryClientEid[] =
    "15141312-1110-0908-0706-050403020100";

constexpr FidoCableDiscovery::EidArray kSecondaryAuthenticatorEid = {
    {0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
     0xee, 0xee, 0xee, 0xee}};

constexpr FidoCableDiscovery::SessionKeyArray kSecondarySessionKey = {
    {0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
     0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
     0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd}};

// Below constants are used to construct MockBluetoothDevice for testing.
constexpr char kTestBleDeviceAddress[] = "11:12:13:14:15:16";

constexpr char kTestBleDeviceName[] = "test_cable_device";

std::unique_ptr<MockBluetoothDevice> CreateTestBluetoothDevice() {
  return std::make_unique<testing::NiceMock<MockBluetoothDevice>>(
      nullptr /* adapter */, 0 /* bluetooth_class */, kTestBleDeviceName,
      kTestBleDeviceAddress, true /* paired */, true /* connected */);
}

// Matcher to compare the content of advertisement data received from the
// client.
MATCHER_P2(IsAdvertisementContent,
           expected_client_eid,
           expected_uuid_formatted_client_eid,
           "") {
#if defined(OS_MACOSX)
  const auto uuid_list = arg->service_uuids();
  return std::any_of(uuid_list->begin(), uuid_list->end(),
                     [this](const auto& uuid) {
                       return uuid == expected_uuid_formatted_client_eid;
                     });

#elif defined(OS_WIN)
  const auto manufacturer_data = arg->manufacturer_data();
  const auto manufacturer_data_value = manufacturer_data->find(0xFFFD);

  if (manufacturer_data_value == manufacturer_data->end())
    return false;

  return fido_parsing_utils::ExtractSuffixSpan(manufacturer_data_value->second,
                                               2) == expected_client_eid;

#elif defined(OS_LINUX) || defined(OS_CHROMEOS)
  const auto service_data = arg->service_data();
  const auto service_data_with_uuid =
      service_data->find(kCableAdvertisementUUID);

  if (service_data_with_uuid == service_data->end())
    return false;

  const auto& service_data_value = service_data_with_uuid->second;
  return service_data_value[0] == kTestCableVersionNumber &&
         service_data_value.size() == 24u &&
         base::make_span(service_data_value).subspan(8) == expected_client_eid;

#endif

  return true;
}

class CableMockBluetoothAdvertisement : public BluetoothAdvertisement {
 public:
  MOCK_METHOD2(Unregister,
               void(const SuccessCallback& success_callback,
                    const ErrorCallback& error_callback));

 private:
  ~CableMockBluetoothAdvertisement() override = default;
};

// Mock BLE adapter that abstracts out authenticator logic with the following
// logic:
//  - Responds to BluetoothAdapter::RegisterAdvertisement() by always invoking
//    success callback.
//  - Responds to BluetoothAdapter::StartDiscoverySessionWithFilter() by
//    invoking BluetoothAdapter::Observer::DeviceAdded() on a test bluetooth
//    device that includes service data containing authenticator EID.
class CableMockAdapter : public MockBluetoothAdapter {
 public:
  MOCK_METHOD3(RegisterAdvertisement,
               void(std::unique_ptr<BluetoothAdvertisement::Data>,
                    const CreateAdvertisementCallback&,
                    const AdvertisementErrorCallback&));

  void AddNewTestBluetoothDevice(
      base::span<const uint8_t, FidoCableDiscovery::kEphemeralIdSize>
          authenticator_eid) {
    auto mock_device = CreateTestBluetoothDevice();

    std::vector<uint8_t> service_data(8);
    fido_parsing_utils::Append(&service_data, authenticator_eid);
    BluetoothDevice::ServiceDataMap service_data_map;
    service_data_map.emplace(kCableAdvertisementUUID, std::move(service_data));

    mock_device->UpdateAdvertisementData(
        1 /* rssi */, BluetoothDevice::UUIDList(), std::move(service_data_map),
        BluetoothDevice::ManufacturerDataMap(), nullptr /* tx_power*/);

    auto* mock_device_ptr = mock_device.get();
    AddMockDevice(std::move(mock_device));

    for (auto& observer : GetObservers())
      observer.DeviceAdded(this, mock_device_ptr);
  }

  void ExpectRegisterAdvertisementWithResponse(
      bool simulate_success,
      base::span<const uint8_t> expected_client_eid,
      base::StringPiece expected_uuid_formatted_client_eid,
      scoped_refptr<CableMockBluetoothAdvertisement> advertisement_ptr =
          nullptr) {
    if (!advertisement_ptr)
      advertisement_ptr =
          base::MakeRefCounted<CableMockBluetoothAdvertisement>();

    EXPECT_CALL(*this,
                RegisterAdvertisement(
                    IsAdvertisementContent(expected_client_eid,
                                           expected_uuid_formatted_client_eid),
                    _, _))
        .WillOnce(::testing::WithArgs<1, 2>(
            [simulate_success, advertisement_ptr](
                const auto& success_callback, const auto& failure_callback) {
              simulate_success
                  ? success_callback.Run(advertisement_ptr)
                  : failure_callback.Run(BluetoothAdvertisement::ErrorCode::
                                             INVALID_ADVERTISEMENT_ERROR_CODE);
            }));
  }

  void ExpectSuccessCallbackToSetPowered() {
    EXPECT_CALL(*this, SetPowered(true, _, _))
        .WillOnce(::testing::WithArg<1>(
            [](const auto& callback) { callback.Run(); }));
  }

 protected:
  ~CableMockAdapter() override = default;
};

}  // namespace

class FidoCableDiscoveryTest : public ::testing::Test {
 public:
  std::unique_ptr<FidoCableDiscovery> CreateDiscovery() {
    std::vector<FidoCableDiscovery::CableDiscoveryData> discovery_data;
    discovery_data.emplace_back(kTestCableVersionNumber, kClientEid,
                                kAuthenticatorEid, kTestSessionKey);
    return std::make_unique<FidoCableDiscovery>(std::move(discovery_data));
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

// Tests regular successful discovery flow for Cable device.
TEST_F(FidoCableDiscoveryTest, TestDiscoveryFindsNewDevice) {
  auto cable_discovery = CreateDiscovery();
  NiceMock<MockFidoDiscoveryObserver> mock_observer;
  EXPECT_CALL(mock_observer, DeviceAdded(_, _));
  cable_discovery->set_observer(&mock_observer);

  auto mock_adapter =
      base::MakeRefCounted<::testing::NiceMock<CableMockAdapter>>();
  ::testing::InSequence testing_sequence;
  mock_adapter->ExpectSuccessCallbackToSetPowered();
  mock_adapter->ExpectRegisterAdvertisementWithResponse(
      true /* simulate_success */, kClientEid, kUuidFormattedClientEid);
  EXPECT_CALL(*mock_adapter, StartDiscoverySessionWithFilterRaw(_, _, _))
      .WillOnce(::testing::WithArg<1>([&mock_adapter](const auto& callback) {
        mock_adapter->AddNewTestBluetoothDevice(kAuthenticatorEid);
        callback.Run(nullptr);
      }));

  BluetoothAdapterFactory::SetAdapterForTesting(mock_adapter);
  cable_discovery->Start();
  scoped_task_environment_.RunUntilIdle();
}

// Tests a scenario where upon broadcasting advertisement and scanning, client
// discovers a device with an incorrect authenticator EID. Observer::AddDevice()
// must not be called.
TEST_F(FidoCableDiscoveryTest, TestDiscoveryFindsIncorrectDevice) {
  auto cable_discovery = CreateDiscovery();
  NiceMock<MockFidoDiscoveryObserver> mock_observer;
  EXPECT_CALL(mock_observer, DeviceAdded(_, _)).Times(0);
  cable_discovery->set_observer(&mock_observer);

  auto mock_adapter =
      base::MakeRefCounted<::testing::NiceMock<CableMockAdapter>>();
  ::testing::InSequence testing_sequence;
  mock_adapter->ExpectSuccessCallbackToSetPowered();
  mock_adapter->ExpectRegisterAdvertisementWithResponse(
      true /* simulate_success */, kClientEid, kUuidFormattedClientEid);
  EXPECT_CALL(*mock_adapter, StartDiscoverySessionWithFilterRaw(_, _, _))
      .WillOnce(::testing::WithArg<1>([&mock_adapter](const auto& callback) {
        mock_adapter->AddNewTestBluetoothDevice(kInvalidAuthenticatorEid);
        callback.Run(nullptr);
      }));

  BluetoothAdapterFactory::SetAdapterForTesting(mock_adapter);
  cable_discovery->Start();
  scoped_task_environment_.RunUntilIdle();
}

// Tests Cable discovery flow when multiple(2) sets of client/authenticator EIDs
// are passed on from the relying party. We should expect 2 invocations of
// BluetoothAdapter::RegisterAdvertisement().
TEST_F(FidoCableDiscoveryTest, TestDiscoveryWithMultipleEids) {
  std::vector<FidoCableDiscovery::CableDiscoveryData> discovery_data;
  discovery_data.emplace_back(kTestCableVersionNumber, kClientEid,
                              kAuthenticatorEid, kTestSessionKey);
  discovery_data.emplace_back(kTestCableVersionNumber, kSecondaryClientEid,
                              kSecondaryAuthenticatorEid, kSecondarySessionKey);
  auto cable_discovery =
      std::make_unique<FidoCableDiscovery>(std::move(discovery_data));
  NiceMock<MockFidoDiscoveryObserver> mock_observer;
  EXPECT_CALL(mock_observer, DeviceAdded(_, _));
  cable_discovery->set_observer(&mock_observer);

  auto mock_adapter =
      base::MakeRefCounted<::testing::NiceMock<CableMockAdapter>>();
  ::testing::InSequence testing_sequence;
  mock_adapter->ExpectSuccessCallbackToSetPowered();
  mock_adapter->ExpectRegisterAdvertisementWithResponse(
      true /* simulate_success */, kClientEid, kUuidFormattedClientEid);
  mock_adapter->ExpectRegisterAdvertisementWithResponse(
      true /* simulate_success */, kSecondaryClientEid,
      kUuidFormattedSecondaryClientEid);
  EXPECT_CALL(*mock_adapter, StartDiscoverySessionWithFilterRaw(_, _, _))
      .WillOnce(::testing::WithArg<1>([&mock_adapter](const auto& callback) {
        mock_adapter->AddNewTestBluetoothDevice(kAuthenticatorEid);
        callback.Run(nullptr);
      }));

  BluetoothAdapterFactory::SetAdapterForTesting(mock_adapter);
  cable_discovery->Start();
  scoped_task_environment_.RunUntilIdle();
}

// Tests a scenario where only one of the two client EID's are advertised
// successfully. Since at least one advertisement are successfully processed,
// scanning process should be invoked.
TEST_F(FidoCableDiscoveryTest, TestDiscoveryWithPartialAdvertisementSuccess) {
  std::vector<FidoCableDiscovery::CableDiscoveryData> discovery_data;
  discovery_data.emplace_back(kTestCableVersionNumber, kClientEid,
                              kAuthenticatorEid, kTestSessionKey);
  discovery_data.emplace_back(kTestCableVersionNumber, kSecondaryClientEid,
                              kSecondaryAuthenticatorEid, kSecondarySessionKey);
  auto cable_discovery =
      std::make_unique<FidoCableDiscovery>(std::move(discovery_data));
  NiceMock<MockFidoDiscoveryObserver> mock_observer;
  EXPECT_CALL(mock_observer, DeviceAdded(_, _));
  cable_discovery->set_observer(&mock_observer);

  auto mock_adapter =
      base::MakeRefCounted<::testing::NiceMock<CableMockAdapter>>();
  ::testing::InSequence testing_sequence;
  mock_adapter->ExpectSuccessCallbackToSetPowered();
  mock_adapter->ExpectRegisterAdvertisementWithResponse(
      true /* simulate_success */, kClientEid, kUuidFormattedClientEid);
  mock_adapter->ExpectRegisterAdvertisementWithResponse(
      false /* simulate_success */, kSecondaryClientEid,
      kUuidFormattedSecondaryClientEid);
  EXPECT_CALL(*mock_adapter, StartDiscoverySessionWithFilterRaw(_, _, _))
      .WillOnce(::testing::WithArg<1>([&mock_adapter](const auto& callback) {
        mock_adapter->AddNewTestBluetoothDevice(kAuthenticatorEid);
        callback.Run(nullptr);
      }));

  BluetoothAdapterFactory::SetAdapterForTesting(mock_adapter);
  cable_discovery->Start();
  scoped_task_environment_.RunUntilIdle();
}

// Test the scenario when all advertisement for client EID's fails.
TEST_F(FidoCableDiscoveryTest, TestDiscoveryWithAdvertisementFailures) {
  std::vector<FidoCableDiscovery::CableDiscoveryData> discovery_data;
  discovery_data.emplace_back(kTestCableVersionNumber, kClientEid,
                              kAuthenticatorEid, kTestSessionKey);
  discovery_data.emplace_back(kTestCableVersionNumber, kSecondaryClientEid,
                              kSecondaryAuthenticatorEid, kSecondarySessionKey);
  auto cable_discovery =
      std::make_unique<FidoCableDiscovery>(std::move(discovery_data));

  NiceMock<MockFidoDiscoveryObserver> mock_observer;
  EXPECT_CALL(mock_observer, DeviceAdded(_, _)).Times(0);
  cable_discovery->set_observer(&mock_observer);

  auto mock_adapter =
      base::MakeRefCounted<::testing::NiceMock<CableMockAdapter>>();
  ::testing::InSequence testing_sequence;
  mock_adapter->ExpectSuccessCallbackToSetPowered();
  mock_adapter->ExpectRegisterAdvertisementWithResponse(
      false /* simulate_success */, kClientEid, kUuidFormattedClientEid);
  mock_adapter->ExpectRegisterAdvertisementWithResponse(
      false /* simulate_success */, kSecondaryClientEid,
      kUuidFormattedSecondaryClientEid);
  EXPECT_CALL(*mock_adapter, StartDiscoverySessionWithFilterRaw(_, _, _))
      .Times(0);

  BluetoothAdapterFactory::SetAdapterForTesting(mock_adapter);
  cable_discovery->Start();
  scoped_task_environment_.RunUntilIdle();
}

TEST_F(FidoCableDiscoveryTest, TestUnregisterAdvertisementUponDestruction) {
  auto cable_discovery = CreateDiscovery();
  CableMockBluetoothAdvertisement* advertisement =
      new CableMockBluetoothAdvertisement();
  EXPECT_CALL(*advertisement, Unregister(_, _)).Times(1);

  ::testing::InSequence testing_sequence;
  auto mock_adapter =
      base::MakeRefCounted<::testing::NiceMock<CableMockAdapter>>();
  mock_adapter->ExpectSuccessCallbackToSetPowered();
  mock_adapter->ExpectRegisterAdvertisementWithResponse(
      true /* simulate_success */, kClientEid, kUuidFormattedClientEid,
      base::WrapRefCounted(advertisement));

  BluetoothAdapterFactory::SetAdapterForTesting(mock_adapter);
  cable_discovery->Start();
  scoped_task_environment_.RunUntilIdle();

  EXPECT_EQ(1u, cable_discovery->advertisements_.size());
  cable_discovery.reset();
}

}  // namespace device
