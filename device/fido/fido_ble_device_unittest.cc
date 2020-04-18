// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_ble_device.h"

#include "base/optional.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_task_environment.h"
#include "device/bluetooth/test/bluetooth_test.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/mock_fido_ble_connection.h"
#include "device/fido/test_callback_receiver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {
namespace {

using ::testing::_;
using ::testing::Invoke;
using ::testing::Test;
using TestDeviceCallbackReceiver =
    test::ValueCallbackReceiver<base::Optional<std::vector<uint8_t>>>;

constexpr uint16_t kControlPointLength = 20;
constexpr uint8_t kTestData[] = {'T', 'E', 'S', 'T'};

std::vector<std::vector<uint8_t>> ToSerializedFragments(
    FidoBleDeviceCommand command,
    std::vector<uint8_t> payload,
    size_t max_fragment_size) {
  FidoBleFrame frame(command, std::move(payload));
  auto fragments = frame.ToFragments(max_fragment_size);

  const size_t num_fragments = 1 /* init_fragment */ + fragments.second.size();
  std::vector<std::vector<uint8_t>> serialized_fragments(num_fragments);

  fragments.first.Serialize(&serialized_fragments[0]);
  for (size_t i = 1; i < num_fragments; ++i) {
    fragments.second.front().Serialize(&serialized_fragments[i]);
    fragments.second.pop();
  }

  return serialized_fragments;
}

}  // namespace

class FidoBleDeviceTest : public Test {
 public:
  FidoBleDeviceTest() {
    auto connection = std::make_unique<MockFidoBleConnection>(
        BluetoothTestBase::kTestDeviceAddress1);
    connection_ = connection.get();
    device_ = std::make_unique<FidoBleDevice>(std::move(connection));
    connection_->connection_status_callback() =
        device_->GetConnectionStatusCallbackForTesting();
    connection_->read_callback() = device_->GetReadCallbackForTesting();
  }

  FidoBleDevice* device() { return device_.get(); }
  MockFidoBleConnection* connection() { return connection_; }

  void ConnectWithLength(uint16_t length) {
    EXPECT_CALL(*connection(), Connect()).WillOnce(Invoke([this] {
      connection()->connection_status_callback().Run(true);
    }));

    EXPECT_CALL(*connection(), ReadControlPointLengthPtr(_))
        .WillOnce(Invoke([length](auto* cb) { std::move(*cb).Run(length); }));

    device()->Connect();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_{
      base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME};

 private:
  MockFidoBleConnection* connection_;
  std::unique_ptr<FidoBleDevice> device_;
};

TEST_F(FidoBleDeviceTest, ConnectionFailureTest) {
  EXPECT_CALL(*connection(), Connect()).WillOnce(Invoke([this] {
    connection()->connection_status_callback().Run(false);
  }));
  device()->Connect();
}

TEST_F(FidoBleDeviceTest, SendPingTest_Failure_WriteFailed) {
  ConnectWithLength(kControlPointLength);

  EXPECT_CALL(*connection(), WriteControlPointPtr(_, _))
      .WillOnce(Invoke([this](const auto& data, auto* cb) {
        scoped_task_environment_.GetMainThreadTaskRunner()->PostTask(
            FROM_HERE, base::BindOnce(std::move(*cb), false));
      }));

  TestDeviceCallbackReceiver callback_receiver;
  auto payload = fido_parsing_utils::Materialize(kTestData);
  device()->SendPing(std::move(payload), callback_receiver.callback());

  callback_receiver.WaitForCallback();
  EXPECT_FALSE(callback_receiver.value());
}

TEST_F(FidoBleDeviceTest, SendPingTest_Failure_NoResponse) {
  ConnectWithLength(kControlPointLength);
  EXPECT_CALL(*connection(), WriteControlPointPtr(_, _))
      .WillOnce(Invoke([this](const auto& data, auto* cb) {
        scoped_task_environment_.GetMainThreadTaskRunner()->PostTask(
            FROM_HERE, base::BindOnce(std::move(*cb), true));
      }));

  TestDeviceCallbackReceiver callback_receiver;
  const auto payload = fido_parsing_utils::Materialize(kTestData);
  device()->SendPing(payload, callback_receiver.callback());

  callback_receiver.WaitForCallback();
  EXPECT_FALSE(callback_receiver.value().has_value());
}

TEST_F(FidoBleDeviceTest, SendPingTest_Failure_SlowResponse) {
  ConnectWithLength(kControlPointLength);
  EXPECT_CALL(*connection(), WriteControlPointPtr(_, _))
      .WillOnce(Invoke([this](const auto& data, auto* cb) {
        scoped_task_environment_.GetMainThreadTaskRunner()->PostTask(
            FROM_HERE, base::BindOnce(std::move(*cb), true));
      }));

  TestDeviceCallbackReceiver callback_receiver;
  auto payload = fido_parsing_utils::Materialize(kTestData);
  device()->SendPing(payload, callback_receiver.callback());
  callback_receiver.WaitForCallback();
  EXPECT_FALSE(callback_receiver.value());

  // Imitate a ping response from the device after the timeout has passed.
  for (auto&& fragment :
       ToSerializedFragments(FidoBleDeviceCommand::kPing, std::move(payload),
                             kControlPointLength)) {
    connection()->read_callback().Run(std::move(fragment));
  }
}

TEST_F(FidoBleDeviceTest, SendPingTest) {
  ConnectWithLength(kControlPointLength);

  EXPECT_CALL(*connection(), WriteControlPointPtr(_, _))
      .WillOnce(Invoke([this](const auto& data, auto* cb) {
        scoped_task_environment_.GetMainThreadTaskRunner()->PostTask(
            FROM_HERE, base::BindOnce(std::move(*cb), true));

        scoped_task_environment_.GetMainThreadTaskRunner()->PostTask(
            FROM_HERE, base::BindOnce(connection()->read_callback(), data));
      }));

  TestDeviceCallbackReceiver callback_receiver;
  const auto payload = fido_parsing_utils::Materialize(kTestData);
  device()->SendPing(payload, callback_receiver.callback());

  callback_receiver.WaitForCallback();
  const auto& value = callback_receiver.value();
  ASSERT_TRUE(value);
  EXPECT_EQ(payload, *value);
}

TEST_F(FidoBleDeviceTest, SendCancelTest) {
  // BLE cancel command, follow bytes 2 bytes of zero length payload.
  constexpr uint8_t kBleCancelCommand[] = {0xBE, 0x00, 0x00};

  ConnectWithLength(kControlPointLength);
  EXPECT_CALL(*connection(),
              WriteControlPointPtr(
                  fido_parsing_utils::Materialize(kBleCancelCommand), _));

  device()->Cancel();
  scoped_task_environment_.FastForwardUntilNoTasksRemain();
}

TEST_F(FidoBleDeviceTest, StaticGetIdTest) {
  std::string address = BluetoothTestBase::kTestDeviceAddress1;
  EXPECT_EQ("ble:" + address, FidoBleDevice::GetId(address));
}

TEST_F(FidoBleDeviceTest, TryWinkTest) {
  test::TestCallbackReceiver<> closure_receiver;
  device()->TryWink(closure_receiver.callback());
  closure_receiver.WaitForCallback();
}

TEST_F(FidoBleDeviceTest, GetIdTest) {
  EXPECT_EQ(std::string("ble:") + BluetoothTestBase::kTestDeviceAddress1,
            device()->GetId());
}

}  // namespace device
