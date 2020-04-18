// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_ble_connection.h"

#include <utility>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/string_piece.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/test/bluetooth_test.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "device/bluetooth/test/mock_bluetooth_device.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_characteristic.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_connection.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_notify_session.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_service.h"
#include "device/fido/fido_ble_uuids.h"
#include "device/fido/test_callback_receiver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_ANDROID)
#include "device/bluetooth/test/bluetooth_test_android.h"
#elif defined(OS_MACOSX)
#include "device/bluetooth/test/bluetooth_test_mac.h"
#elif defined(OS_WIN)
#include "device/bluetooth/test/bluetooth_test_win.h"
#elif defined(OS_CHROMEOS) || defined(OS_LINUX)
#include "device/bluetooth/test/bluetooth_test_bluez.h"
#endif

namespace device {

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::Return;

using NiceMockBluetoothAdapter = ::testing::NiceMock<MockBluetoothAdapter>;
using NiceMockBluetoothDevice = ::testing::NiceMock<MockBluetoothDevice>;
using NiceMockBluetoothGattService =
    ::testing::NiceMock<MockBluetoothGattService>;
using NiceMockBluetoothGattCharacteristic =
    ::testing::NiceMock<MockBluetoothGattCharacteristic>;
using NiceMockBluetoothGattConnection =
    ::testing::NiceMock<MockBluetoothGattConnection>;
using NiceMockBluetoothGattNotifySession =
    ::testing::NiceMock<MockBluetoothGattNotifySession>;

namespace {

std::vector<uint8_t> ToByteVector(base::StringPiece str) {
  return std::vector<uint8_t>(str.begin(), str.end());
}

BluetoothDevice* GetMockDevice(MockBluetoothAdapter* adapter,
                               const std::string& address) {
  const std::vector<BluetoothDevice*> devices = adapter->GetMockDevices();
  auto found = std::find_if(devices.begin(), devices.end(),
                            [&address](const auto* device) {
                              return device->GetAddress() == address;
                            });
  return found != devices.end() ? *found : nullptr;
}

class TestConnectionStatusCallback {
 public:
  void OnStatus(bool status) {
    status_ = status;
    run_loop_->Quit();
  }

  bool WaitForResult() {
    run_loop_->Run();
    run_loop_.emplace();
    return status_;
  }

  FidoBleConnection::ConnectionStatusCallback GetCallback() {
    return base::BindRepeating(&TestConnectionStatusCallback::OnStatus,
                               base::Unretained(this));
  }

 private:
  bool status_ = false;
  base::Optional<base::RunLoop> run_loop_{base::in_place};
};

class TestReadCallback {
 public:
  void OnRead(std::vector<uint8_t> value) {
    value_ = std::move(value);
    run_loop_->Quit();
  }

  const std::vector<uint8_t> WaitForResult() {
    run_loop_->Run();
    run_loop_.emplace();
    return value_;
  }

  FidoBleConnection::ReadCallback GetCallback() {
    return base::BindRepeating(&TestReadCallback::OnRead,
                               base::Unretained(this));
  }

 private:
  std::vector<uint8_t> value_;
  base::Optional<base::RunLoop> run_loop_{base::in_place};
};

using TestReadControlPointLengthCallback =
    test::ValueCallbackReceiver<base::Optional<uint16_t>>;

using TestReadServiceRevisionsCallback =
    test::ValueCallbackReceiver<std::set<FidoBleConnection::ServiceRevision>>;

using TestWriteCallback = test::ValueCallbackReceiver<bool>;
}  // namespace

class FidoBleConnectionTest : public ::testing::Test {
 public:
  FidoBleConnectionTest() {
    ON_CALL(*adapter_, GetDevice(_))
        .WillByDefault(Invoke([this](const std::string& address) {
          return GetMockDevice(adapter_.get(), address);
        }));

    BluetoothAdapterFactory::SetAdapterForTesting(adapter_);
  }

  void AddU2Device(const std::string& device_address) {
    auto u2f_device = std::make_unique<NiceMockBluetoothDevice>(
        adapter_.get(), /* bluetooth_class */ 0u,
        BluetoothTest::kTestDeviceNameU2f, device_address, /* paired */ true,
        /* connected */ false);
    u2f_device_ = u2f_device.get();
    adapter_->AddMockDevice(std::move(u2f_device));

    ON_CALL(*u2f_device_, GetGattServices())
        .WillByDefault(
            Invoke(u2f_device_, &MockBluetoothDevice::GetMockServices));

    ON_CALL(*u2f_device_, GetGattService(_))
        .WillByDefault(
            Invoke(u2f_device_, &MockBluetoothDevice::GetMockService));
    AddU2fService();
  }

  void SetupConnectingU2fDevice(const std::string& device_address) {
    auto run_cb_with_connection = [this, &device_address](
                                      const auto& callback,
                                      const auto& error_callback) {
      auto connection = std::make_unique<NiceMockBluetoothGattConnection>(
          adapter_, device_address);
      connection_ = connection.get();
      callback.Run(std::move(connection));
    };

    auto run_cb_with_notify_session = [this](const auto& callback,
                                             const auto& error_callback) {
      auto notify_session =
          std::make_unique<NiceMockBluetoothGattNotifySession>(
              u2f_status_->GetWeakPtr());
      notify_session_ = notify_session.get();
      callback.Run(std::move(notify_session));
    };

    ON_CALL(*u2f_device_, CreateGattConnection(_, _))
        .WillByDefault(Invoke(run_cb_with_connection));

    ON_CALL(*u2f_device_, IsGattServicesDiscoveryComplete())
        .WillByDefault(Return(true));

    ON_CALL(*u2f_status_, StartNotifySession(_, _))
        .WillByDefault(Invoke(run_cb_with_notify_session));
  }

  void SimulateDisconnect(const std::string& device_address) {
    if (u2f_device_->GetAddress() != device_address)
      return;

    u2f_device_->SetConnected(false);
    adapter_->NotifyDeviceChanged(u2f_device_);
  }

  void SimulateDeviceAddressChange(const std::string& old_address,
                                   const std::string& new_address) {
    if (!u2f_device_ || u2f_device_->GetAddress() != old_address)
      return;

    ON_CALL(*u2f_device_, GetAddress()).WillByDefault(Return(new_address));

    adapter_->NotifyDeviceChanged(u2f_device_);
    for (auto& observer : adapter_->GetObservers())
      observer.DeviceAddressChanged(adapter_.get(), u2f_device_, old_address);
  }

  void NotifyDeviceAdded(const std::string& device_address) {
    auto* device = adapter_->GetDevice(device_address);
    if (!device)
      return;

    for (auto& observer : adapter_->GetObservers())
      observer.DeviceAdded(adapter_.get(), device);
  }

  void NotifyStatusChanged(const std::vector<uint8_t>& value) {
    for (auto& observer : adapter_->GetObservers())
      observer.GattCharacteristicValueChanged(adapter_.get(), u2f_status_,
                                              value);
  }

  void SetNextReadControlPointLengthReponse(bool success,
                                            const std::vector<uint8_t>& value) {
    EXPECT_CALL(*u2f_control_point_length_, ReadRemoteCharacteristic(_, _))
        .WillOnce(Invoke([success, value](const auto& callback,
                                          const auto& error_callback) {
          success ? callback.Run(value)
                  : error_callback.Run(BluetoothGattService::GATT_ERROR_FAILED);
        }));
  }

  void SetNextReadServiceRevisionResponse(bool success,
                                          const std::vector<uint8_t>& value) {
    EXPECT_CALL(*u2f_service_revision_, ReadRemoteCharacteristic(_, _))
        .WillOnce(Invoke([success, value](const auto& callback,
                                          const auto& error_callback) {
          success ? callback.Run(value)
                  : error_callback.Run(BluetoothGattService::GATT_ERROR_FAILED);
        }));
  }

  void SetNextReadServiceRevisionBitfieldResponse(
      bool success,
      const std::vector<uint8_t>& value) {
    EXPECT_CALL(*u2f_service_revision_bitfield_, ReadRemoteCharacteristic(_, _))
        .WillOnce(Invoke([success, value](const auto& callback,
                                          const auto& error_callback) {
          success ? callback.Run(value)
                  : error_callback.Run(BluetoothGattService::GATT_ERROR_FAILED);
        }));
  }

  void SetNextWriteControlPointResponse(bool success) {
    EXPECT_CALL(*u2f_control_point_, WriteRemoteCharacteristic(_, _, _))
        .WillOnce(Invoke([success](const auto& data, const auto& callback,
                                   const auto& error_callback) {
          success ? callback.Run()
                  : error_callback.Run(BluetoothGattService::GATT_ERROR_FAILED);
        }));
  }

  void SetNextWriteServiceRevisionResponse(bool success) {
    EXPECT_CALL(*u2f_service_revision_bitfield_,
                WriteRemoteCharacteristic(_, _, _))
        .WillOnce(Invoke([success](const auto& data, const auto& callback,
                                   const auto& error_callback) {
          success ? callback.Run()
                  : error_callback.Run(BluetoothGattService::GATT_ERROR_FAILED);
        }));
  }

  void AddU2fService() {
    auto u2f_service = std::make_unique<NiceMockBluetoothGattService>(
        u2f_device_, "u2f_service", BluetoothUUID(kFidoServiceUUID),
        /* is_primary */ true, /* is_local */ false);
    u2f_service_ = u2f_service.get();
    u2f_device_->AddMockService(std::move(u2f_service));

    ON_CALL(*u2f_service_, GetCharacteristics())
        .WillByDefault(Invoke(
            u2f_service_, &MockBluetoothGattService::GetMockCharacteristics));

    ON_CALL(*u2f_service_, GetCharacteristic(_))
        .WillByDefault(Invoke(
            u2f_service_, &MockBluetoothGattService::GetMockCharacteristic));
    AddU2fCharacteristics();
  }

  void AddU2fCharacteristics() {
    const bool is_local = false;
    {
      auto u2f_control_point =
          std::make_unique<NiceMockBluetoothGattCharacteristic>(
              u2f_service_, "u2f_control_point",
              BluetoothUUID(kFidoControlPointUUID), is_local,
              BluetoothGattCharacteristic::PROPERTY_WRITE,
              BluetoothGattCharacteristic::PERMISSION_NONE);
      u2f_control_point_ = u2f_control_point.get();
      u2f_service_->AddMockCharacteristic(std::move(u2f_control_point));
    }

    {
      auto u2f_status = std::make_unique<NiceMockBluetoothGattCharacteristic>(
          u2f_service_, "u2f_status", BluetoothUUID(kFidoStatusUUID), is_local,
          BluetoothGattCharacteristic::PROPERTY_NOTIFY,
          BluetoothGattCharacteristic::PERMISSION_NONE);
      u2f_status_ = u2f_status.get();
      u2f_service_->AddMockCharacteristic(std::move(u2f_status));
    }

    {
      auto u2f_control_point_length =
          std::make_unique<NiceMockBluetoothGattCharacteristic>(
              u2f_service_, "u2f_control_point_length",
              BluetoothUUID(kFidoControlPointLengthUUID), is_local,
              BluetoothGattCharacteristic::PROPERTY_READ,
              BluetoothGattCharacteristic::PERMISSION_NONE);
      u2f_control_point_length_ = u2f_control_point_length.get();
      u2f_service_->AddMockCharacteristic(std::move(u2f_control_point_length));
    }

    {
      auto u2f_service_revision =
          std::make_unique<NiceMockBluetoothGattCharacteristic>(
              u2f_service_, "u2f_service_revision",
              BluetoothUUID(kFidoServiceRevisionUUID), is_local,
              BluetoothGattCharacteristic::PROPERTY_READ,
              BluetoothGattCharacteristic::PERMISSION_NONE);
      u2f_service_revision_ = u2f_service_revision.get();
      u2f_service_->AddMockCharacteristic(std::move(u2f_service_revision));
    }

    {
      auto u2f_service_revision_bitfield =
          std::make_unique<NiceMockBluetoothGattCharacteristic>(
              u2f_service_, "u2f_service_revision_bitfield",
              BluetoothUUID(kFidoServiceRevisionBitfieldUUID), is_local,
              BluetoothGattCharacteristic::PROPERTY_READ |
                  BluetoothGattCharacteristic::PROPERTY_WRITE,
              BluetoothGattCharacteristic::PERMISSION_NONE);
      u2f_service_revision_bitfield_ = u2f_service_revision_bitfield.get();
      u2f_service_->AddMockCharacteristic(
          std::move(u2f_service_revision_bitfield));
    }
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  scoped_refptr<MockBluetoothAdapter> adapter_ =
      base::MakeRefCounted<NiceMockBluetoothAdapter>();

  MockBluetoothDevice* u2f_device_;
  MockBluetoothGattService* u2f_service_;

  MockBluetoothGattCharacteristic* u2f_control_point_;
  MockBluetoothGattCharacteristic* u2f_status_;
  MockBluetoothGattCharacteristic* u2f_control_point_length_;
  MockBluetoothGattCharacteristic* u2f_service_revision_;
  MockBluetoothGattCharacteristic* u2f_service_revision_bitfield_;

  MockBluetoothGattConnection* connection_;
  MockBluetoothGattNotifySession* notify_session_;
};

TEST_F(FidoBleConnectionTest, Address) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  auto connect_do_nothing = [](bool) {};
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  FidoBleConnection connection(device_address,
                               base::BindRepeating(connect_do_nothing),
                               base::BindRepeating(read_do_nothing));
  connection.Connect();
  EXPECT_EQ(device_address, connection.address());
  AddU2Device(device_address);

  SimulateDeviceAddressChange(device_address, "new_device_address");
  EXPECT_EQ("new_device_address", connection.address());
}

TEST_F(FidoBleConnectionTest, DeviceNotPresent) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  TestConnectionStatusCallback connection_status_callback;
  auto do_nothing = [](std::vector<uint8_t>) {};

  FidoBleConnection connection(device_address,
                               connection_status_callback.GetCallback(),
                               base::BindRepeating(do_nothing));
  connection.Connect();
  bool result = connection_status_callback.WaitForResult();
  EXPECT_FALSE(result);
}

TEST_F(FidoBleConnectionTest, PreConnected) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  TestConnectionStatusCallback connection_status_callback;
  AddU2Device(device_address);
  SetupConnectingU2fDevice(device_address);

  auto do_nothing = [](std::vector<uint8_t>) {};
  FidoBleConnection connection(device_address,
                               connection_status_callback.GetCallback(),
                               base::BindRepeating(do_nothing));
  connection.Connect();
  EXPECT_TRUE(connection_status_callback.WaitForResult());
}

TEST_F(FidoBleConnectionTest, PostConnected) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  TestConnectionStatusCallback connection_status_callback;
  auto do_nothing = [](std::vector<uint8_t>) {};
  FidoBleConnection connection(device_address,
                               connection_status_callback.GetCallback(),
                               base::BindRepeating(do_nothing));
  connection.Connect();
  bool result = connection_status_callback.WaitForResult();
  EXPECT_FALSE(result);

  AddU2Device(device_address);
  SetupConnectingU2fDevice(device_address);
  NotifyDeviceAdded(device_address);
  EXPECT_TRUE(connection_status_callback.WaitForResult());
}

TEST_F(FidoBleConnectionTest, DeviceDisconnect) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  TestConnectionStatusCallback connection_status_callback;

  AddU2Device(device_address);
  SetupConnectingU2fDevice(device_address);
  auto do_nothing = [](std::vector<uint8_t>) {};
  FidoBleConnection connection(device_address,
                               connection_status_callback.GetCallback(),
                               base::BindRepeating(do_nothing));
  connection.Connect();
  bool result = connection_status_callback.WaitForResult();
  EXPECT_TRUE(result);

  SimulateDisconnect(device_address);
  result = connection_status_callback.WaitForResult();
  EXPECT_FALSE(result);
}

TEST_F(FidoBleConnectionTest, ReadStatusNotifications) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  TestConnectionStatusCallback connection_status_callback;
  TestReadCallback read_callback;

  AddU2Device(device_address);
  SetupConnectingU2fDevice(device_address);
  FidoBleConnection connection(device_address,
                               connection_status_callback.GetCallback(),
                               read_callback.GetCallback());
  connection.Connect();
  EXPECT_TRUE(connection_status_callback.WaitForResult());

  std::vector<uint8_t> payload = ToByteVector("foo");
  NotifyStatusChanged(payload);
  EXPECT_EQ(payload, read_callback.WaitForResult());

  payload = ToByteVector("bar");
  NotifyStatusChanged(payload);
  EXPECT_EQ(payload, read_callback.WaitForResult());
}

TEST_F(FidoBleConnectionTest, ReadControlPointLength) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  TestConnectionStatusCallback connection_status_callback;
  AddU2Device(device_address);
  SetupConnectingU2fDevice(device_address);
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  FidoBleConnection connection(device_address,
                               connection_status_callback.GetCallback(),
                               base::BindRepeating(read_do_nothing));
  connection.Connect();
  EXPECT_TRUE(connection_status_callback.WaitForResult());

  TestReadControlPointLengthCallback length_callback;
  SetNextReadControlPointLengthReponse(false, {});
  connection.ReadControlPointLength(length_callback.callback());
  EXPECT_EQ(base::nullopt, length_callback.value());

  // The Control Point Length should consist of exactly two bytes, hence we
  // EXPECT_EQ(base::nullopt) for payloads of size 0, 1 and 3.
  SetNextReadControlPointLengthReponse(true, {});
  connection.ReadControlPointLength(length_callback.callback());
  EXPECT_EQ(base::nullopt, length_callback.value());

  SetNextReadControlPointLengthReponse(true, {0xAB});
  connection.ReadControlPointLength(length_callback.callback());
  EXPECT_EQ(base::nullopt, length_callback.value());

  SetNextReadControlPointLengthReponse(true, {0xAB, 0xCD});
  connection.ReadControlPointLength(length_callback.callback());
  EXPECT_EQ(0xABCD, *length_callback.value());

  SetNextReadControlPointLengthReponse(true, {0xAB, 0xCD, 0xEF});
  connection.ReadControlPointLength(length_callback.callback());
  EXPECT_EQ(base::nullopt, length_callback.value());
}

TEST_F(FidoBleConnectionTest, ReadServiceRevisions) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  TestConnectionStatusCallback connection_status_callback;
  AddU2Device(device_address);
  SetupConnectingU2fDevice(device_address);
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  FidoBleConnection connection(device_address,
                               connection_status_callback.GetCallback(),
                               base::BindRepeating(read_do_nothing));
  connection.Connect();
  EXPECT_TRUE(connection_status_callback.WaitForResult());

  TestReadServiceRevisionsCallback revisions_callback;
  SetNextReadServiceRevisionResponse(false, {});
  SetNextReadServiceRevisionBitfieldResponse(false, {});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(), IsEmpty());

  SetNextReadServiceRevisionResponse(true, ToByteVector("bogus"));
  SetNextReadServiceRevisionBitfieldResponse(false, {});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(), IsEmpty());

  SetNextReadServiceRevisionResponse(true, ToByteVector("1.0"));
  SetNextReadServiceRevisionBitfieldResponse(false, {});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(),
              ElementsAre(FidoBleConnection::ServiceRevision::VERSION_1_0));

  SetNextReadServiceRevisionResponse(true, ToByteVector("1.1"));
  SetNextReadServiceRevisionBitfieldResponse(false, {});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(),
              ElementsAre(FidoBleConnection::ServiceRevision::VERSION_1_1));

  SetNextReadServiceRevisionResponse(true, ToByteVector("1.2"));
  SetNextReadServiceRevisionBitfieldResponse(false, {});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(),
              ElementsAre(FidoBleConnection::ServiceRevision::VERSION_1_2));

  // Version 1.3 currently does not exist, so this should be treated as an
  // error.
  SetNextReadServiceRevisionResponse(true, ToByteVector("1.3"));
  SetNextReadServiceRevisionBitfieldResponse(false, {});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(), IsEmpty());

  SetNextReadServiceRevisionResponse(false, {});
  SetNextReadServiceRevisionBitfieldResponse(true, {0x00});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(), IsEmpty());

  SetNextReadServiceRevisionResponse(false, {});
  SetNextReadServiceRevisionBitfieldResponse(true, {0x80});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(),
              ElementsAre(FidoBleConnection::ServiceRevision::VERSION_1_1));

  SetNextReadServiceRevisionResponse(false, {});
  SetNextReadServiceRevisionBitfieldResponse(true, {0x40});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(),
              ElementsAre(FidoBleConnection::ServiceRevision::VERSION_1_2));

  SetNextReadServiceRevisionResponse(false, {});
  SetNextReadServiceRevisionBitfieldResponse(true, {0xC0});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(),
              ElementsAre(FidoBleConnection::ServiceRevision::VERSION_1_1,
                          FidoBleConnection::ServiceRevision::VERSION_1_2));

  // All bits except the first two should be ignored.
  SetNextReadServiceRevisionResponse(false, {});
  SetNextReadServiceRevisionBitfieldResponse(true, {0xFF});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(),
              ElementsAre(FidoBleConnection::ServiceRevision::VERSION_1_1,
                          FidoBleConnection::ServiceRevision::VERSION_1_2));

  // All bytes except the first one should be ignored.
  SetNextReadServiceRevisionResponse(false, {});
  SetNextReadServiceRevisionBitfieldResponse(true, {0xC0, 0xFF});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(),
              ElementsAre(FidoBleConnection::ServiceRevision::VERSION_1_1,
                          FidoBleConnection::ServiceRevision::VERSION_1_2));

  // The combination of a service revision string and bitfield should be
  // supported as well.
  SetNextReadServiceRevisionResponse(true, ToByteVector("1.0"));
  SetNextReadServiceRevisionBitfieldResponse(true, {0xC0});
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(),
              ElementsAre(FidoBleConnection::ServiceRevision::VERSION_1_0,
                          FidoBleConnection::ServiceRevision::VERSION_1_1,
                          FidoBleConnection::ServiceRevision::VERSION_1_2));
}

TEST_F(FidoBleConnectionTest, WriteControlPoint) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  TestConnectionStatusCallback connection_status_callback;
  AddU2Device(device_address);
  SetupConnectingU2fDevice(device_address);
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  FidoBleConnection connection(device_address,
                               connection_status_callback.GetCallback(),
                               base::BindRepeating(read_do_nothing));
  connection.Connect();
  bool result = connection_status_callback.WaitForResult();
  EXPECT_TRUE(result);

  TestWriteCallback write_callback;
  SetNextWriteControlPointResponse(false);
  connection.WriteControlPoint({}, write_callback.callback());
  result = write_callback.value();
  EXPECT_FALSE(result);

  SetNextWriteControlPointResponse(true);
  connection.WriteControlPoint({}, write_callback.callback());
  result = write_callback.value();
  EXPECT_TRUE(result);
}

TEST_F(FidoBleConnectionTest, WriteServiceRevision) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  TestConnectionStatusCallback connection_status_callback;
  AddU2Device(device_address);
  SetupConnectingU2fDevice(device_address);
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  FidoBleConnection connection(device_address,
                               connection_status_callback.GetCallback(),
                               base::BindRepeating(read_do_nothing));
  connection.Connect();
  bool result = connection_status_callback.WaitForResult();
  EXPECT_TRUE(result);

  // Expect that errors are properly propagated.
  TestWriteCallback write_callback;
  SetNextWriteServiceRevisionResponse(false);
  connection.WriteServiceRevision(
      FidoBleConnection::ServiceRevision::VERSION_1_1,
      write_callback.callback());
  result = write_callback.value();
  EXPECT_FALSE(result);

  // Expect a successful write of version 1.1.
  SetNextWriteServiceRevisionResponse(true);
  connection.WriteServiceRevision(
      FidoBleConnection::ServiceRevision::VERSION_1_1,
      write_callback.callback());
  result = write_callback.value();
  EXPECT_TRUE(result);

  // Expect a successful write of version 1.2.
  SetNextWriteServiceRevisionResponse(true);
  connection.WriteServiceRevision(
      FidoBleConnection::ServiceRevision::VERSION_1_2,
      write_callback.callback());
  result = write_callback.value();
  EXPECT_TRUE(result);

  // Writing version 1.0 to the bitfield is not intended, so this should fail.
  connection.WriteServiceRevision(
      FidoBleConnection::ServiceRevision::VERSION_1_0,
      write_callback.callback());
  result = write_callback.value();
  EXPECT_FALSE(result);
}

TEST_F(FidoBleConnectionTest, ReadsAndWriteFailWhenDisconnected) {
  const std::string device_address = BluetoothTest::kTestDeviceAddress1;
  TestConnectionStatusCallback connection_status_callback;

  AddU2Device(device_address);
  SetupConnectingU2fDevice(device_address);
  auto do_nothing = [](std::vector<uint8_t>) {};
  FidoBleConnection connection(device_address,
                               connection_status_callback.GetCallback(),
                               base::BindRepeating(do_nothing));
  connection.Connect();
  bool result = connection_status_callback.WaitForResult();
  EXPECT_TRUE(result);

  SimulateDisconnect(device_address);
  result = connection_status_callback.WaitForResult();
  EXPECT_FALSE(result);

  // Reads should always fail on a disconnected device.
  TestReadControlPointLengthCallback length_callback;
  connection.ReadControlPointLength(length_callback.callback());
  EXPECT_EQ(base::nullopt, length_callback.value());

  TestReadServiceRevisionsCallback revisions_callback;
  connection.ReadServiceRevisions(revisions_callback.callback());
  EXPECT_THAT(revisions_callback.value(), IsEmpty());

  // Writes should always fail on a disconnected device.
  TestWriteCallback write_callback;
  connection.WriteServiceRevision(
      FidoBleConnection::ServiceRevision::VERSION_1_1,
      write_callback.callback());
  result = write_callback.value();
  EXPECT_FALSE(result);

  connection.WriteControlPoint({}, write_callback.callback());
  result = write_callback.value();
  EXPECT_FALSE(result);
}

}  // namespace device
