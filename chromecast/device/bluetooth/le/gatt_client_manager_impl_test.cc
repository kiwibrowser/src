// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/device/bluetooth/le/gatt_client_manager_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/test/mock_callback.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromecast/device/bluetooth/bluetooth_util.h"
#include "chromecast/device/bluetooth/le/remote_characteristic.h"
#include "chromecast/device/bluetooth/le/remote_descriptor.h"
#include "chromecast/device/bluetooth/le/remote_device.h"
#include "chromecast/device/bluetooth/le/remote_service.h"
#include "chromecast/device/bluetooth/shlib/mock_gatt_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

namespace chromecast {
namespace bluetooth {

using MockStatusCallback = base::MockCallback<RemoteDevice::StatusCallback>;

namespace {

const bluetooth_v2_shlib::Addr kTestAddr1 = {
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}};

class MockGattClientManagerObserver : public GattClientManager::Observer {
 public:
  MOCK_METHOD2(OnConnectChanged,
               void(scoped_refptr<RemoteDevice> device, bool connected));
  MOCK_METHOD2(OnMtuChanged, void(scoped_refptr<RemoteDevice> device, int mtu));
  MOCK_METHOD2(OnServicesUpdated,
               void(scoped_refptr<RemoteDevice> device,
                    std::vector<scoped_refptr<RemoteService>> services));
  MOCK_METHOD3(OnCharacteristicNotification,
               void(scoped_refptr<RemoteDevice> device,
                    scoped_refptr<RemoteCharacteristic> characteristic,
                    std::vector<uint8_t> value));
};

std::vector<bluetooth_v2_shlib::Gatt::Service> GenerateServices() {
  std::vector<bluetooth_v2_shlib::Gatt::Service> ret;

  bluetooth_v2_shlib::Gatt::Service service;
  bluetooth_v2_shlib::Gatt::Characteristic characteristic;
  bluetooth_v2_shlib::Gatt::Descriptor descriptor;

  service.uuid = {{0x1}};
  service.handle = 0x1;
  service.primary = true;

  characteristic.uuid = {{0x1, 0x1}};
  characteristic.handle = 0x2;
  characteristic.permissions =
      static_cast<bluetooth_v2_shlib::Gatt::Permissions>(
          bluetooth_v2_shlib::Gatt::PERMISSION_READ |
          bluetooth_v2_shlib::Gatt::PERMISSION_WRITE);
  characteristic.properties = bluetooth_v2_shlib::Gatt::PROPERTY_NOTIFY;

  descriptor.uuid = {{0x1, 0x1, 0x1}};
  descriptor.handle = 0x3;
  descriptor.permissions = static_cast<bluetooth_v2_shlib::Gatt::Permissions>(
      bluetooth_v2_shlib::Gatt::PERMISSION_READ |
      bluetooth_v2_shlib::Gatt::PERMISSION_WRITE);
  characteristic.descriptors.push_back(descriptor);

  descriptor.uuid = RemoteDescriptor::kCccdUuid;
  descriptor.handle = 0x4;
  descriptor.permissions = static_cast<bluetooth_v2_shlib::Gatt::Permissions>(
      bluetooth_v2_shlib::Gatt::PERMISSION_READ |
      bluetooth_v2_shlib::Gatt::PERMISSION_WRITE);
  characteristic.descriptors.push_back(descriptor);
  service.characteristics.push_back(characteristic);

  characteristic.uuid = {{0x1, 0x2}};
  characteristic.handle = 0x5;
  characteristic.permissions =
      static_cast<bluetooth_v2_shlib::Gatt::Permissions>(
          bluetooth_v2_shlib::Gatt::PERMISSION_READ |
          bluetooth_v2_shlib::Gatt::PERMISSION_WRITE);
  characteristic.properties =
      static_cast<bluetooth_v2_shlib::Gatt::Properties>(0);
  characteristic.descriptors.clear();

  ret.push_back(service);

  service.uuid = {{0x2}};
  service.handle = 0x6;
  service.primary = true;
  service.characteristics.clear();
  ret.push_back(service);

  return ret;
}

class GattClientManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    message_loop_ =
        std::make_unique<base::MessageLoop>(base::MessageLoop::TYPE_DEFAULT);
    gatt_client_ = std::make_unique<bluetooth_v2_shlib::MockGattClient>();
    gatt_client_manager_ =
        std::make_unique<GattClientManagerImpl>(gatt_client_.get());
    observer_ = std::make_unique<MockGattClientManagerObserver>();

    // Normally bluetooth_manager does this.
    gatt_client_->SetDelegate(gatt_client_manager_.get());
    gatt_client_manager_->Initialize(base::ThreadTaskRunnerHandle::Get());
    gatt_client_manager_->AddObserver(observer_.get());
  }

  void TearDown() override {
    gatt_client_->SetDelegate(nullptr);
    gatt_client_manager_->RemoveObserver(observer_.get());
    gatt_client_manager_->Finalize();
  }

  scoped_refptr<RemoteDevice> GetDevice(const bluetooth_v2_shlib::Addr& addr) {
    scoped_refptr<RemoteDevice> ret;
    gatt_client_manager_->GetDevice(
        addr, base::BindOnce(
                  [](scoped_refptr<RemoteDevice>* ret_ptr,
                     scoped_refptr<RemoteDevice> result) { *ret_ptr = result; },
                  &ret));

    return ret;
  }

  std::vector<scoped_refptr<RemoteService>> GetServices(RemoteDevice* device) {
    std::vector<scoped_refptr<RemoteService>> ret;
    device->GetServices(base::BindOnce(
        [](std::vector<scoped_refptr<RemoteService>>* ret_ptr,
           std::vector<scoped_refptr<RemoteService>> result) {
          *ret_ptr = result;
        },
        &ret));

    return ret;
  }

  scoped_refptr<RemoteService> GetServiceByUuid(
      RemoteDevice* device,
      const bluetooth_v2_shlib::Uuid& uuid) {
    scoped_refptr<RemoteService> ret;
    device->GetServiceByUuid(uuid, base::BindOnce(
                                       [](scoped_refptr<RemoteService>* ret_ptr,
                                          scoped_refptr<RemoteService> result) {
                                         *ret_ptr = result;
                                       },
                                       &ret));
    return ret;
  }

  void Connect(const bluetooth_v2_shlib::Addr& addr) {
    EXPECT_CALL(*gatt_client_, Connect(addr)).WillOnce(Return(true));
    scoped_refptr<RemoteDevice> device = GetDevice(addr);
    EXPECT_CALL(cb_, Run(true));
    device->Connect(cb_.Get());
    bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
        gatt_client_->delegate();
    EXPECT_CALL(*gatt_client_, GetServices(kTestAddr1)).WillOnce(Return(true));
    delegate->OnConnectChanged(addr, true /* status */, true /* connected */);
    delegate->OnGetServices(addr, {});
    ASSERT_TRUE(device->IsConnected());
  }

  MockStatusCallback cb_;
  std::unique_ptr<base::MessageLoop> message_loop_;
  std::unique_ptr<GattClientManagerImpl> gatt_client_manager_;
  std::unique_ptr<bluetooth_v2_shlib::MockGattClient> gatt_client_;
  std::unique_ptr<MockGattClientManagerObserver> observer_;
};

}  // namespace

TEST_F(GattClientManagerTest, RemoteDeviceConnect) {
  bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
      gatt_client_->delegate();

  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);
  EXPECT_FALSE(device->IsConnected());
  EXPECT_EQ(kTestAddr1, device->addr());

  // These should fail if we're not connected.
  EXPECT_CALL(cb_, Run(false));
  device->Disconnect(cb_.Get());

  base::MockCallback<RemoteDevice::RssiCallback> rssi_cb;
  EXPECT_CALL(rssi_cb, Run(false, _));
  device->ReadRemoteRssi(rssi_cb.Get());

  EXPECT_CALL(cb_, Run(false));
  device->RequestMtu(512, cb_.Get());

  EXPECT_CALL(cb_, Run(false));
  device->ConnectionParameterUpdate(10, 10, 50, 100, cb_.Get());

  EXPECT_CALL(*gatt_client_, Connect(kTestAddr1)).WillOnce(Return(true));

  EXPECT_CALL(cb_, Run(true));
  device->Connect(cb_.Get());
  EXPECT_CALL(*gatt_client_, GetServices(kTestAddr1)).WillOnce(Return(true));
  delegate->OnConnectChanged(kTestAddr1, true /* status */,
                             true /* connected */);
  EXPECT_CALL(*observer_, OnConnectChanged(device, true));
  delegate->OnGetServices(kTestAddr1, {});

  EXPECT_TRUE(device->IsConnected());
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*gatt_client_, Disconnect(kTestAddr1)).WillOnce(Return(true));
  device->Disconnect({});
  EXPECT_TRUE(device->IsConnected());

  EXPECT_CALL(*observer_, OnConnectChanged(device, false));
  delegate->OnConnectChanged(kTestAddr1, true /* status */,
                             false /* connected */);
  EXPECT_FALSE(device->IsConnected());
  base::RunLoop().RunUntilIdle();
}

TEST_F(GattClientManagerTest, RemoteDeviceReadRssi) {
  static const int kRssi = -34;

  bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
      gatt_client_->delegate();
  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);

  Connect(kTestAddr1);

  base::MockCallback<RemoteDevice::RssiCallback> rssi_cb;
  EXPECT_CALL(*gatt_client_, ReadRemoteRssi(kTestAddr1)).WillOnce(Return(true));
  EXPECT_CALL(rssi_cb, Run(true, kRssi));
  device->ReadRemoteRssi(rssi_cb.Get());

  delegate->OnReadRemoteRssi(kTestAddr1, true /* status */, kRssi);
}

TEST_F(GattClientManagerTest, RemoteDeviceRequestMtu) {
  static const int kMtu = 512;
  bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
      gatt_client_->delegate();
  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);
  Connect(kTestAddr1);

  EXPECT_EQ(RemoteDevice::kDefaultMtu, device->GetMtu());
  EXPECT_CALL(*gatt_client_, RequestMtu(kTestAddr1, kMtu))
      .WillOnce(Return(true));
  EXPECT_CALL(cb_, Run(true));
  device->RequestMtu(kMtu, cb_.Get());
  EXPECT_CALL(*observer_, OnMtuChanged(device, kMtu));
  delegate->OnMtuChanged(kTestAddr1, true, kMtu);
  EXPECT_EQ(kMtu, device->GetMtu());
  base::RunLoop().RunUntilIdle();
}

TEST_F(GattClientManagerTest, RemoteDeviceConnectionParameterUpdate) {
  const int kMinInterval = 10;
  const int kMaxInterval = 10;
  const int kLatency = 50;
  const int kTimeout = 100;

  Connect(kTestAddr1);

  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);
  EXPECT_CALL(*gatt_client_,
              ConnectionParameterUpdate(kTestAddr1, kMinInterval, kMaxInterval,
                                        kLatency, kTimeout))
      .WillOnce(Return(true));
  EXPECT_CALL(cb_, Run(true));
  device->ConnectionParameterUpdate(kMinInterval, kMaxInterval, kLatency,
                                    kTimeout, cb_.Get());
}

TEST_F(GattClientManagerTest, RemoteDeviceServices) {
  const auto kServices = GenerateServices();
  Connect(kTestAddr1);
  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);
  std::vector<scoped_refptr<RemoteService>> services;
  EXPECT_EQ(0ul, GetServices(device.get()).size());

  bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
      gatt_client_->delegate();
  delegate->OnServicesAdded(kTestAddr1, kServices);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(kServices.size(), GetServices(device.get()).size());
  for (const auto& service : kServices) {
    scoped_refptr<RemoteService> remote_service =
        GetServiceByUuid(device.get(), service.uuid);
    ASSERT_TRUE(remote_service);
    EXPECT_EQ(service.uuid, remote_service->uuid());
    EXPECT_EQ(service.handle, remote_service->handle());
    EXPECT_EQ(service.primary, remote_service->primary());
    EXPECT_EQ(service.characteristics.size(),
              remote_service->GetCharacteristics().size());

    for (const auto& characteristic : service.characteristics) {
      scoped_refptr<RemoteCharacteristic> remote_char =
          remote_service->GetCharacteristicByUuid(characteristic.uuid);
      ASSERT_TRUE(remote_char);
      EXPECT_EQ(characteristic.uuid, remote_char->uuid());
      EXPECT_EQ(characteristic.handle, remote_char->handle());
      EXPECT_EQ(characteristic.permissions, remote_char->permissions());
      EXPECT_EQ(characteristic.properties, remote_char->properties());
      EXPECT_EQ(characteristic.descriptors.size(),
                remote_char->GetDescriptors().size());

      for (const auto& descriptor : characteristic.descriptors) {
        scoped_refptr<RemoteDescriptor> remote_desc =
            remote_char->GetDescriptorByUuid(descriptor.uuid);
        ASSERT_TRUE(remote_desc);
        EXPECT_EQ(descriptor.uuid, remote_desc->uuid());
        EXPECT_EQ(descriptor.handle, remote_desc->handle());
        EXPECT_EQ(descriptor.permissions, remote_desc->permissions());
      }
    }
  }
}

TEST_F(GattClientManagerTest, RemoteDeviceCharacteristic) {
  const std::vector<uint8_t> kTestData1 = {0x1, 0x2, 0x3};
  const std::vector<uint8_t> kTestData2 = {0x4, 0x5, 0x6};
  const std::vector<uint8_t> kTestData3 = {0x7, 0x8, 0x9};
  const auto kServices = GenerateServices();
  const bluetooth_v2_shlib::Gatt::Client::AuthReq kAuthReq =
      bluetooth_v2_shlib::Gatt::Client::AUTH_REQ_MITM;
  const bluetooth_v2_shlib::Gatt::WriteType kWriteType =
      bluetooth_v2_shlib::Gatt::WRITE_TYPE_DEFAULT;

  Connect(kTestAddr1);
  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);
  bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
      gatt_client_->delegate();
  delegate->OnServicesAdded(kTestAddr1, kServices);
  std::vector<scoped_refptr<RemoteService>> services =
      GetServices(device.get());
  ASSERT_EQ(kServices.size(), services.size());

  auto service = services[0];
  std::vector<scoped_refptr<RemoteCharacteristic>> characteristics =
      service->GetCharacteristics();
  ASSERT_GE(characteristics.size(), 1ul);
  auto characteristic = characteristics[0];

  EXPECT_CALL(*gatt_client_,
              WriteCharacteristic(kTestAddr1, characteristic->characteristic(),
                                  kAuthReq, kWriteType, kTestData1))
      .WillOnce(Return(true));

  EXPECT_CALL(cb_, Run(true));
  characteristic->WriteAuth(kAuthReq, kWriteType, kTestData1, cb_.Get());
  delegate->OnCharacteristicWriteResponse(kTestAddr1, true,
                                          characteristic->handle());

  EXPECT_CALL(*gatt_client_,
              ReadCharacteristic(kTestAddr1, characteristic->characteristic(),
                                 kAuthReq))
      .WillOnce(Return(true));

  base::MockCallback<RemoteCharacteristic::ReadCallback> read_cb;
  EXPECT_CALL(read_cb, Run(true, kTestData2));
  characteristic->ReadAuth(kAuthReq, read_cb.Get());
  delegate->OnCharacteristicReadResponse(kTestAddr1, true,
                                         characteristic->handle(), kTestData2);

  EXPECT_CALL(*gatt_client_,
              SetCharacteristicNotification(
                  kTestAddr1, characteristic->characteristic(), true))
      .WillOnce(Return(true));

  EXPECT_CALL(cb_, Run(true));
  characteristic->SetNotification(true, cb_.Get());

  EXPECT_CALL(*observer_,
              OnCharacteristicNotification(device, characteristic, kTestData3));
  delegate->OnNotification(kTestAddr1, characteristic->handle(), kTestData3);
  base::RunLoop().RunUntilIdle();
}

TEST_F(GattClientManagerTest,
       RemoteDeviceCharacteristicSetRegisterNotification) {
  const std::vector<uint8_t> kTestData1 = {0x1, 0x2, 0x3};
  const auto kServices = GenerateServices();
  Connect(kTestAddr1);
  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);
  bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
      gatt_client_->delegate();
  delegate->OnServicesAdded(kTestAddr1, kServices);
  std::vector<scoped_refptr<RemoteService>> services =
      GetServices(device.get());
  ASSERT_EQ(kServices.size(), services.size());

  scoped_refptr<RemoteService> service = services[0];
  std::vector<scoped_refptr<RemoteCharacteristic>> characteristics =
      service->GetCharacteristics();
  ASSERT_GE(characteristics.size(), 1ul);
  scoped_refptr<RemoteCharacteristic> characteristic = characteristics[0];

  scoped_refptr<RemoteDescriptor> cccd =
      characteristic->GetDescriptorByUuid(RemoteDescriptor::kCccdUuid);
  ASSERT_TRUE(cccd);

  EXPECT_CALL(*gatt_client_,
              SetCharacteristicNotification(
                  kTestAddr1, characteristic->characteristic(), true))
      .WillOnce(Return(true));
  std::vector<uint8_t> cccd_enable_notification = {
      std::begin(bluetooth::RemoteDescriptor::kEnableNotificationValue),
      std::end(bluetooth::RemoteDescriptor::kEnableNotificationValue)};
  EXPECT_CALL(*gatt_client_, WriteDescriptor(kTestAddr1, cccd->descriptor(), _,
                                             cccd_enable_notification))
      .WillOnce(Return(true));

  characteristic->SetRegisterNotification(true, cb_.Get());
  EXPECT_CALL(cb_, Run(true));
  delegate->OnDescriptorWriteResponse(kTestAddr1, true, cccd->handle());

  EXPECT_CALL(*observer_,
              OnCharacteristicNotification(device, characteristic, kTestData1));
  delegate->OnNotification(kTestAddr1, characteristic->handle(), kTestData1);
  base::RunLoop().RunUntilIdle();
}

TEST_F(GattClientManagerTest, RemoteDeviceDescriptor) {
  const std::vector<uint8_t> kTestData1 = {0x1, 0x2, 0x3};
  const std::vector<uint8_t> kTestData2 = {0x4, 0x5, 0x6};
  const bluetooth_v2_shlib::Gatt::Client::AuthReq kAuthReq =
      bluetooth_v2_shlib::Gatt::Client::AUTH_REQ_MITM;
  const auto kServices = GenerateServices();
  Connect(kTestAddr1);
  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);
  bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
      gatt_client_->delegate();
  delegate->OnServicesAdded(kTestAddr1, kServices);
  std::vector<scoped_refptr<RemoteService>> services =
      GetServices(device.get());
  ASSERT_EQ(kServices.size(), services.size());

  auto service = services[0];
  std::vector<scoped_refptr<RemoteCharacteristic>> characteristics =
      service->GetCharacteristics();
  ASSERT_GE(characteristics.size(), 1ul);
  auto characteristic = characteristics[0];

  std::vector<scoped_refptr<RemoteDescriptor>> descriptors =
      characteristic->GetDescriptors();
  ASSERT_GE(descriptors.size(), 1ul);
  auto descriptor = descriptors[0];

  EXPECT_CALL(*gatt_client_,
              WriteDescriptor(kTestAddr1, descriptor->descriptor(), kAuthReq,
                              kTestData1))
      .WillOnce(Return(true));

  EXPECT_CALL(cb_, Run(true));
  descriptor->WriteAuth(kAuthReq, kTestData1, cb_.Get());
  delegate->OnDescriptorWriteResponse(kTestAddr1, true, descriptor->handle());

  EXPECT_CALL(*gatt_client_,
              ReadDescriptor(kTestAddr1, descriptor->descriptor(), kAuthReq))
      .WillOnce(Return(true));

  base::MockCallback<RemoteDescriptor::ReadCallback> read_cb;
  EXPECT_CALL(read_cb, Run(true, kTestData2));
  descriptor->ReadAuth(kAuthReq, read_cb.Get());
  delegate->OnDescriptorReadResponse(kTestAddr1, true, descriptor->handle(),
                                     kTestData2);
}

TEST_F(GattClientManagerTest, FakeCccd) {
  std::vector<bluetooth_v2_shlib::Gatt::Service> input_services(1);
  input_services[0].uuid = {{0x1}};
  input_services[0].handle = 0x1;
  input_services[0].primary = true;

  bluetooth_v2_shlib::Gatt::Characteristic input_characteristic;
  input_characteristic.uuid = {{0x1, 0x1}};
  input_characteristic.handle = 0x2;
  input_characteristic.permissions = bluetooth_v2_shlib::Gatt::PERMISSION_READ;
  input_characteristic.properties = bluetooth_v2_shlib::Gatt::PROPERTY_NOTIFY;
  input_services[0].characteristics.push_back(input_characteristic);

  // Test indicate as well
  input_characteristic.uuid = {{0x1, 0x2}};
  input_characteristic.handle = 0x3;
  input_characteristic.properties = bluetooth_v2_shlib::Gatt::PROPERTY_INDICATE;
  input_services[0].characteristics.push_back(input_characteristic);

  Connect(kTestAddr1);
  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);
  bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
      gatt_client_->delegate();
  delegate->OnServicesAdded(kTestAddr1, input_services);
  std::vector<scoped_refptr<RemoteService>> services =
      GetServices(device.get());
  ASSERT_EQ(input_services.size(), services.size());

  auto service = services[0];
  std::vector<scoped_refptr<RemoteCharacteristic>> characteristics =
      service->GetCharacteristics();
  ASSERT_EQ(2u, characteristics.size());
  for (const auto& characteristic : characteristics) {
    // A CCCD should have been created.
    std::vector<scoped_refptr<RemoteDescriptor>> descriptors =
        characteristic->GetDescriptors();
    ASSERT_EQ(descriptors.size(), 1ul);
    auto descriptor = descriptors[0];
    EXPECT_EQ(RemoteDescriptor::kCccdUuid, descriptor->uuid());
    EXPECT_EQ(static_cast<bluetooth_v2_shlib::Gatt::Permissions>(
                  bluetooth_v2_shlib::Gatt::PERMISSION_READ |
                  bluetooth_v2_shlib::Gatt::PERMISSION_WRITE),
              descriptor->permissions());
  }
}

TEST_F(GattClientManagerTest, WriteType) {
  const std::vector<uint8_t> kTestData1 = {0x1, 0x2, 0x3};

  bluetooth_v2_shlib::Gatt::Service service;
  bluetooth_v2_shlib::Gatt::Characteristic characteristic;

  service.uuid = {{0x1}};
  service.handle = 0x1;
  service.primary = true;

  characteristic.uuid = {{0x1, 0x1}};
  characteristic.handle = 0x2;
  characteristic.permissions = bluetooth_v2_shlib::Gatt::PERMISSION_WRITE;
  characteristic.properties = bluetooth_v2_shlib::Gatt::PROPERTY_WRITE;
  service.characteristics.push_back(characteristic);

  characteristic.uuid = {{0x1, 0x2}};
  characteristic.handle = 0x3;
  characteristic.permissions = bluetooth_v2_shlib::Gatt::PERMISSION_WRITE;
  characteristic.properties =
      bluetooth_v2_shlib::Gatt::PROPERTY_WRITE_NO_RESPONSE;
  service.characteristics.push_back(characteristic);

  characteristic.uuid = {{0x1, 0x3}};
  characteristic.handle = 0x4;
  characteristic.permissions = bluetooth_v2_shlib::Gatt::PERMISSION_WRITE;
  characteristic.properties = bluetooth_v2_shlib::Gatt::PROPERTY_SIGNED_WRITE;
  service.characteristics.push_back(characteristic);

  Connect(kTestAddr1);
  bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
      gatt_client_->delegate();
  delegate->OnServicesAdded(kTestAddr1, {service});

  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);

  std::vector<scoped_refptr<RemoteService>> services =
      GetServices(device.get());
  ASSERT_EQ(1u, services.size());

  std::vector<scoped_refptr<RemoteCharacteristic>> characteristics =
      services[0]->GetCharacteristics();
  ASSERT_EQ(3u, characteristics.size());

  using WriteType = bluetooth_v2_shlib::Gatt::WriteType;

  // The current implementation of RemoteDevice will put the characteristics in
  // the order reported by libcast_bluetooth.
  const WriteType kWriteTypes[] = {WriteType::WRITE_TYPE_DEFAULT,
                                   WriteType::WRITE_TYPE_NO_RESPONSE,
                                   WriteType::WRITE_TYPE_SIGNED};

  for (size_t i = 0; i < characteristics.size(); ++i) {
    const auto& characteristic = characteristics[i];
    EXPECT_CALL(
        *gatt_client_,
        WriteCharacteristic(kTestAddr1, characteristic->characteristic(),
                            bluetooth_v2_shlib::Gatt::Client::AUTH_REQ_NONE,
                            kWriteTypes[i], kTestData1))
        .WillOnce(Return(true));

    base::MockCallback<RemoteCharacteristic::StatusCallback> write_cb;
    EXPECT_CALL(write_cb, Run(true));
    characteristic->Write(kTestData1, write_cb.Get());
    delegate->OnCharacteristicWriteResponse(kTestAddr1, true,
                                            characteristic->handle());
  }
}

TEST_F(GattClientManagerTest, ConnectMultiple) {
  bluetooth_v2_shlib::Gatt::Client::Delegate* delegate =
      gatt_client_->delegate();
  scoped_refptr<RemoteDevice> device = GetDevice(kTestAddr1);
  for (size_t i = 0; i < 5; ++i) {
    Connect(kTestAddr1);
    EXPECT_TRUE(device->IsConnected());
    EXPECT_CALL(*gatt_client_, Disconnect(kTestAddr1)).WillOnce(Return(true));
    device->Disconnect({});
    delegate->OnConnectChanged(kTestAddr1, true /* status */,
                               false /* connected */);
    EXPECT_FALSE(device->IsConnected());
  }
}

}  // namespace bluetooth
}  // namespace chromecast
