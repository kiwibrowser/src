// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/proximity_auth/bluetooth_low_energy_connection_finder.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "components/cryptauth/connection.h"
#include "components/cryptauth/fake_connection.h"
#include "components/cryptauth/remote_device_ref.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "components/cryptauth/wire_message.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "device/bluetooth/test/mock_bluetooth_device.h"
#include "device/bluetooth/test/mock_bluetooth_discovery_session.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_connection.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::AtLeast;
using testing::NiceMock;
using testing::Return;
using testing::StrictMock;
using testing::SaveArg;

using device::BluetoothDevice;
using device::MockBluetoothDevice;

namespace proximity_auth {
namespace {

const char kBLEGattServiceUUID[] = "b3b7e28e-a000-3e17-bd86-6e97b9e28c11";
const char kAdvertisementUUID[] = "0000fe50-0000-1000-8000-00805f9b34fb";
const int8_t kRssi = -30;
const char kEidForPreviousTimeQuantum[] = "\x12\x34";
const char kEidForCurrentTimeQuantum[] = "\xab\xcd";
const char kEidForNextTimeQuantum[] = "\x56\x78";
const char kWrongEid[] = "\xff\xff";
const int64_t kEidPeriodMs = 60 * 1000 * 15;  // 15 minutes.

class MockBluetoothLowEnergyConnectionFinder;
class FakeEidGenerator : public cryptauth::BackgroundEidGenerator {
 public:
  explicit FakeEidGenerator(
      MockBluetoothLowEnergyConnectionFinder* connection_finder)
      : connection_finder_(connection_finder) {}
  ~FakeEidGenerator() override {}

  std::vector<cryptauth::DataWithTimestamp> GenerateNearestEids(
      const std::vector<cryptauth::BeaconSeed>& beacon_seed) const override;

 private:
  MockBluetoothLowEnergyConnectionFinder* connection_finder_;

  DISALLOW_COPY_AND_ASSIGN(FakeEidGenerator);
};

class MockBluetoothLowEnergyConnectionFinder
    : public BluetoothLowEnergyConnectionFinder {
 public:
  MockBluetoothLowEnergyConnectionFinder()
      : BluetoothLowEnergyConnectionFinder(
            cryptauth::CreateRemoteDeviceRefForTest(),
            kBLEGattServiceUUID,
            std::make_unique<FakeEidGenerator>(this)) {}

  ~MockBluetoothLowEnergyConnectionFinder() override {}

  // Mock methods don't support return type std::unique_ptr<>. This is a
  // possible workaround: mock a proxy method to be called by the target
  // overridden method (CreateConnection).
  MOCK_METHOD0(CreateConnectionProxy, cryptauth::Connection*());

  // Creates a mock connection and sets an expectation that the mock connection
  // finder's CreateConnection() method will be called and will return the
  // created connection. Returns a reference to the created connection.
  // NOTE: The returned connection's lifetime is managed by the connection
  // finder.
  cryptauth::FakeConnection* ExpectCreateConnection() {
    std::unique_ptr<cryptauth::FakeConnection> connection(
        new cryptauth::FakeConnection(
            cryptauth::CreateRemoteDeviceRefForTest()));
    cryptauth::FakeConnection* connection_alias = connection.get();
    EXPECT_CALL(*this, CreateConnectionProxy())
        .WillOnce(Return(connection.release()));
    return connection_alias;
  }

  void SetNearestEids(const std::vector<std::string>& eids) {
    nearest_eids_ = eids;
  }

  const std::vector<std::string>& nearest_eids() { return nearest_eids_; }

 protected:
  std::unique_ptr<cryptauth::Connection> CreateConnection(
      device::BluetoothDevice* bluetooth_device) override {
    return base::WrapUnique(CreateConnectionProxy());
  }

 private:
  std::vector<std::string> nearest_eids_;

  DISALLOW_COPY_AND_ASSIGN(MockBluetoothLowEnergyConnectionFinder);
};

// Not declared in-line due to dependency on
// MockBluetoothLowEnergyConnectionFinder.
std::vector<cryptauth::DataWithTimestamp> FakeEidGenerator::GenerateNearestEids(
    const std::vector<cryptauth::BeaconSeed>& beacon_seed) const {
  std::vector<std::string> nearest_eids = connection_finder_->nearest_eids();

  std::vector<cryptauth::DataWithTimestamp> eid_data_with_timestamps;
  int64_t start_of_period_ms = 0;
  for (const std::string& eid : nearest_eids) {
    eid_data_with_timestamps.push_back(cryptauth::DataWithTimestamp(
        eid, start_of_period_ms, start_of_period_ms + kEidPeriodMs));
    start_of_period_ms += kEidPeriodMs;
  }

  return eid_data_with_timestamps;
}

}  // namespace

class ProximityAuthBluetoothLowEnergyConnectionFinderTest
    : public testing::Test {
 protected:
  ProximityAuthBluetoothLowEnergyConnectionFinderTest()
      : adapter_(new NiceMock<device::MockBluetoothAdapter>),
        connection_callback_(
            base::Bind(&ProximityAuthBluetoothLowEnergyConnectionFinderTest::
                           OnConnectionFound,
                       base::Unretained(this))),
        device_(new NiceMock<device::MockBluetoothDevice>(
            adapter_.get(),
            0,
            cryptauth::kTestRemoteDeviceName,
            std::string(),
            false,
            false)),
        last_discovery_session_alias_(nullptr) {
    device::BluetoothAdapterFactory::SetAdapterForTesting(adapter_);

    std::vector<const device::BluetoothDevice*> devices;
    ON_CALL(*adapter_, GetDevices()).WillByDefault(Return(devices));

    ON_CALL(*adapter_, IsPresent()).WillByDefault(Return(true));
    ON_CALL(*adapter_, IsPowered()).WillByDefault(Return(true));

    std::vector<std::string> nearest_eids;
    nearest_eids.push_back(kEidForPreviousTimeQuantum);
    nearest_eids.push_back(kEidForCurrentTimeQuantum);
    nearest_eids.push_back(kEidForNextTimeQuantum);
    connection_finder_.SetNearestEids(nearest_eids);
  }

  void OnConnectionFound(std::unique_ptr<cryptauth::Connection> connection) {
    last_found_connection_ = std::move(connection);
  }

  void FindAndExpectStartDiscovery() {
    device::BluetoothAdapter::DiscoverySessionCallback discovery_callback;
    std::unique_ptr<device::MockBluetoothDiscoverySession> discovery_session(
        new NiceMock<device::MockBluetoothDiscoverySession>());
    last_discovery_session_alias_ = discovery_session.get();

    // Starting a discovery session. StartDiscoveryWithFilterRaw is a proxy for
    // StartDiscoveryWithFilter.
    EXPECT_CALL(*adapter_, StartDiscoverySessionWithFilterRaw(_, _, _))
        .WillOnce(SaveArg<1>(&discovery_callback));
    EXPECT_CALL(*adapter_, AddObserver(_));
    ON_CALL(*last_discovery_session_alias_, IsActive())
        .WillByDefault(Return(true));
    connection_finder_.Find(connection_callback_);
    ASSERT_FALSE(discovery_callback.is_null());
    discovery_callback.Run(std::move(discovery_session));

    EXPECT_CALL(*adapter_, RemoveObserver(_)).Times(AtLeast(1));
  }

  // Prepare |device_| with the given EID.
  void PrepareDevice(const std::string& eid) {
    PrepareDevice(device_.get(), eid);
  }

  void PrepareDevice(MockBluetoothDevice* device, const std::string& eid) {
    device::BluetoothUUID advertisement_uuid(kAdvertisementUUID);
    std::vector<uint8_t> eid_vector(eid.c_str(), eid.c_str() + eid.length());
    device::BluetoothDevice::UUIDList uuid_list;
    uuid_list.push_back(advertisement_uuid);
    device::BluetoothDevice::ServiceDataMap service_data_map;
    service_data_map[advertisement_uuid] = eid_vector;
    device_->UpdateAdvertisementData(kRssi, uuid_list, service_data_map,
                                     {} /* manufacturer_data */, nullptr);
  }

  scoped_refptr<device::MockBluetoothAdapter> adapter_;
  cryptauth::ConnectionFinder::ConnectionCallback connection_callback_;
  std::unique_ptr<device::MockBluetoothDevice> device_;
  std::unique_ptr<cryptauth::Connection> last_found_connection_;
  device::MockBluetoothDiscoverySession* last_discovery_session_alias_;
  StrictMock<MockBluetoothLowEnergyConnectionFinder> connection_finder_;

 private:
  base::MessageLoop message_loop_;
};

TEST_F(ProximityAuthBluetoothLowEnergyConnectionFinderTest,
       Find_StartsDiscoverySession) {
  EXPECT_CALL(*adapter_, StartDiscoverySessionWithFilterRaw(_, _, _));
  EXPECT_CALL(*adapter_, AddObserver(_));
  connection_finder_.Find(connection_callback_);
}

TEST_F(ProximityAuthBluetoothLowEnergyConnectionFinderTest,
       Find_StopsDiscoverySessionBeforeDestroying) {
  device::BluetoothAdapter::DiscoverySessionCallback discovery_callback;
  std::unique_ptr<device::MockBluetoothDiscoverySession> discovery_session(
      new NiceMock<device::MockBluetoothDiscoverySession>());
  device::MockBluetoothDiscoverySession* discovery_session_alias =
      discovery_session.get();

  EXPECT_CALL(*adapter_, StartDiscoverySessionWithFilterRaw(_, _, _))
      .WillOnce(SaveArg<1>(&discovery_callback));
  ON_CALL(*discovery_session_alias, IsActive()).WillByDefault(Return(true));
  EXPECT_CALL(*adapter_, AddObserver(_));
  connection_finder_.Find(connection_callback_);

  ASSERT_FALSE(discovery_callback.is_null());
  discovery_callback.Run(std::move(discovery_session));

  EXPECT_CALL(*adapter_, RemoveObserver(_));
}

TEST_F(ProximityAuthBluetoothLowEnergyConnectionFinderTest,
       Find_DeviceAdded_EidMatches) {
  FindAndExpectStartDiscovery();

  connection_finder_.ExpectCreateConnection();
  PrepareDevice(kEidForCurrentTimeQuantum);
  connection_finder_.DeviceAdded(adapter_.get(), device_.get());
}

TEST_F(ProximityAuthBluetoothLowEnergyConnectionFinderTest,
       Find_DeviceChanged_EidMatches) {
  FindAndExpectStartDiscovery();

  connection_finder_.DeviceAdded(adapter_.get(), device_.get());

  connection_finder_.ExpectCreateConnection();
  PrepareDevice(kEidForPreviousTimeQuantum);
  connection_finder_.DeviceChanged(adapter_.get(), device_.get());
}

TEST_F(ProximityAuthBluetoothLowEnergyConnectionFinderTest,
       Find_DeviceAdded_EidDoesNotMatch) {
  FindAndExpectStartDiscovery();

  PrepareDevice(kWrongEid);

  EXPECT_CALL(connection_finder_, CreateConnectionProxy()).Times(0);
  connection_finder_.DeviceAdded(adapter_.get(), device_.get());
}

TEST_F(ProximityAuthBluetoothLowEnergyConnectionFinderTest,
       Find_DeviceChanged_EidDoesNotMatch) {
  FindAndExpectStartDiscovery();

  PrepareDevice(kWrongEid);
  connection_finder_.DeviceAdded(adapter_.get(), device_.get());

  EXPECT_CALL(connection_finder_, CreateConnectionProxy()).Times(0);
  connection_finder_.DeviceChanged(adapter_.get(), device_.get());
}

TEST_F(ProximityAuthBluetoothLowEnergyConnectionFinderTest,
       Find_CreatesOnlyOneConnection) {
  FindAndExpectStartDiscovery();

  // Prepare first device with valid EID.
  PrepareDevice(kEidForCurrentTimeQuantum);

  // Prepare second device with valid EID.
  NiceMock<device::MockBluetoothDevice> other_device(
      adapter_.get(), 0, cryptauth::kTestRemoteDeviceName, std::string(), false,
      false);
  PrepareDevice(&other_device, kEidForPreviousTimeQuantum);

  // Add the devices. Only one connection is expected.
  connection_finder_.ExpectCreateConnection();
  connection_finder_.DeviceAdded(adapter_.get(), device_.get());
  connection_finder_.DeviceAdded(adapter_.get(), &other_device);
}

TEST_F(ProximityAuthBluetoothLowEnergyConnectionFinderTest,
       Find_EidMatches_ConnectionSucceeds) {
  // Starting discovery.
  FindAndExpectStartDiscovery();

  // Finding and creating a connection to the right device.
  cryptauth::FakeConnection* connection =
      connection_finder_.ExpectCreateConnection();
  PrepareDevice(kEidForCurrentTimeQuantum);
  connection_finder_.DeviceAdded(adapter_.get(), device_.get());

  // Creating a connection.
  base::RunLoop run_loop;
  EXPECT_FALSE(last_found_connection_);
  connection->SetStatus(cryptauth::Connection::Status::IN_PROGRESS);
  connection->SetStatus(cryptauth::Connection::Status::CONNECTED);
  run_loop.RunUntilIdle();
  EXPECT_TRUE(last_found_connection_);
}

TEST_F(ProximityAuthBluetoothLowEnergyConnectionFinderTest,
       Find_ConnectionFails_RestartDiscoveryAndConnectionSucceeds) {
  // Starting discovery.
  FindAndExpectStartDiscovery();

  // Preparing to create a GATT connection to the right device.
  PrepareDevice(kEidForNextTimeQuantum);
  cryptauth::FakeConnection* connection =
      connection_finder_.ExpectCreateConnection();

  // Trying to create a connection.
  connection_finder_.DeviceAdded(adapter_.get(), device_.get());
  ASSERT_FALSE(last_found_connection_);
  connection->SetStatus(cryptauth::Connection::Status::IN_PROGRESS);

  // Preparing to restart the discovery session.
  device::BluetoothAdapter::DiscoverySessionCallback discovery_callback;
  std::vector<const device::BluetoothDevice*> devices;
  ON_CALL(*adapter_, GetDevices()).WillByDefault(Return(devices));
  EXPECT_CALL(*adapter_, StartDiscoverySessionWithFilterRaw(_, _, _))
      .WillOnce(SaveArg<1>(&discovery_callback));

  // Connection fails.
  {
    base::RunLoop run_loop;
    connection->SetStatus(cryptauth::Connection::Status::DISCONNECTED);
    run_loop.RunUntilIdle();
  }

  // Restarting the discovery session.
  std::unique_ptr<device::MockBluetoothDiscoverySession> discovery_session(
      new NiceMock<device::MockBluetoothDiscoverySession>());
  last_discovery_session_alias_ = discovery_session.get();
  ON_CALL(*last_discovery_session_alias_, IsActive())
      .WillByDefault(Return(true));
  ASSERT_FALSE(discovery_callback.is_null());
  discovery_callback.Run(std::move(discovery_session));

  // Connect again.
  PrepareDevice(kEidForNextTimeQuantum);
  connection = connection_finder_.ExpectCreateConnection();
  connection_finder_.DeviceAdded(adapter_.get(), device_.get());

  // Completing the connection.
  {
    base::RunLoop run_loop;
    EXPECT_FALSE(last_found_connection_);
    connection->SetStatus(cryptauth::Connection::Status::IN_PROGRESS);
    connection->SetStatus(cryptauth::Connection::Status::CONNECTED);
    run_loop.RunUntilIdle();
  }
  EXPECT_TRUE(last_found_connection_);
}

TEST_F(ProximityAuthBluetoothLowEnergyConnectionFinderTest,
       Find_AdapterRemoved_RestartDiscoveryAndConnectionSucceeds) {
  // Starting discovery.
  FindAndExpectStartDiscovery();

  // Removing the adapter.
  ON_CALL(*adapter_, IsPresent()).WillByDefault(Return(false));
  ON_CALL(*adapter_, IsPowered()).WillByDefault(Return(false));
  ON_CALL(*last_discovery_session_alias_, IsActive())
      .WillByDefault(Return(false));
  connection_finder_.AdapterPoweredChanged(adapter_.get(), false);
  connection_finder_.AdapterPresentChanged(adapter_.get(), false);

  // Adding the adapter.
  ON_CALL(*adapter_, IsPresent()).WillByDefault(Return(true));
  ON_CALL(*adapter_, IsPowered()).WillByDefault(Return(true));

  device::BluetoothAdapter::DiscoverySessionCallback discovery_callback;
  std::unique_ptr<device::MockBluetoothDiscoverySession> discovery_session(
      new NiceMock<device::MockBluetoothDiscoverySession>());
  last_discovery_session_alias_ = discovery_session.get();

  // Restarting the discovery session.
  EXPECT_CALL(*adapter_, StartDiscoverySessionWithFilterRaw(_, _, _))
      .WillOnce(SaveArg<1>(&discovery_callback));
  connection_finder_.AdapterPresentChanged(adapter_.get(), true);
  connection_finder_.AdapterPoweredChanged(adapter_.get(), true);
  ON_CALL(*last_discovery_session_alias_, IsActive())
      .WillByDefault(Return(true));

  ASSERT_FALSE(discovery_callback.is_null());
  discovery_callback.Run(std::move(discovery_session));

  // Preparing to create a GATT connection to the right device.
  PrepareDevice(kEidForPreviousTimeQuantum);
  cryptauth::FakeConnection* connection =
      connection_finder_.ExpectCreateConnection();

  // Trying to create a connection.
  connection_finder_.DeviceAdded(adapter_.get(), device_.get());

  // Completing the connection.
  base::RunLoop run_loop;
  ASSERT_FALSE(last_found_connection_);
  connection->SetStatus(cryptauth::Connection::Status::IN_PROGRESS);
  connection->SetStatus(cryptauth::Connection::Status::CONNECTED);
  run_loop.RunUntilIdle();
  EXPECT_TRUE(last_found_connection_);
}

}  // namespace proximity_auth
