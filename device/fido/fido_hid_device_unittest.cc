// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_hid_device.h"

#include <memory>
#include <tuple>

#include "base/bind.h"
#include "base/containers/span.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "device/fido/fake_hid_impl_for_testing.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/test_callback_receiver.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/device/public/cpp/hid/hid_device_filter.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

using ::testing::_;
using ::testing::Invoke;

namespace {

// HID_MSG(83), followed by payload length(000b), followed by response data
// "MOCK_DATA", followed by APDU SW_NO_ERROR response code(9000).
constexpr uint8_t kU2fMockResponseMessage[] = {
    0x83, 0x00, 0x0b, 0x4d, 0x4f, 0x43, 0x4b,
    0x5f, 0x44, 0x41, 0x54, 0x41, 0x90, 0x00,
};

// APDU encoded success response with data "MOCK_DATA" followed by a SW_NO_ERROR
// APDU response code(9000).
constexpr uint8_t kU2fMockResponseData[] = {0x4d, 0x4f, 0x43, 0x4b, 0x5f, 0x44,
                                            0x41, 0x54, 0x41, 0x90, 0x00};

// HID_KEEP_ALIVE(bb), followed by payload length(0001), followed by
// status processing(01) byte.
constexpr uint8_t kMockKeepAliveResponseSuffix[] = {0xbb, 0x00, 0x01, 0x01};

// 4 byte broadcast channel id(ffffffff), followed by an HID_INIT command(86),
// followed by a fixed size payload length(11). 8 byte nonce and 4 byte channel
// ID must be appended to create a well formed  HID_INIT packet.
constexpr uint8_t kInitResponsePrefix[] = {
    0xff, 0xff, 0xff, 0xff, 0x86, 0x00, 0x11,
};

// Mock APDU encoded U2F request with empty data and mock P1 parameter(0x04).
constexpr uint8_t kMockU2fRequest[] = {0x00, 0x04, 0x00, 0x00,
                                       0x00, 0x00, 0x00};

// Returns HID_INIT request to send to device with mock connection.
std::vector<uint8_t> CreateMockInitResponse(
    base::span<const uint8_t> nonce,
    base::span<const uint8_t> channel_id) {
  auto init_response = fido_parsing_utils::Materialize(kInitResponsePrefix);
  fido_parsing_utils::Append(&init_response, nonce);
  fido_parsing_utils::Append(&init_response, channel_id);
  init_response.resize(64);
  return init_response;
}

// Returns HID keep alive message encoded into HID packet format.
std::vector<uint8_t> GetKeepAliveHidMessage(
    base::span<const uint8_t> channel_id) {
  auto response = fido_parsing_utils::Materialize(channel_id);
  fido_parsing_utils::Append(&response, kMockKeepAliveResponseSuffix);
  response.resize(64);
  return response;
}

// Returns "U2F_v2" as a mock response to version request with given channel id.
std::vector<uint8_t> CreateMockResponseWithChannelId(
    base::span<const uint8_t> channel_id,
    base::span<const uint8_t> response_buffer) {
  auto response = fido_parsing_utils::Materialize(channel_id);
  fido_parsing_utils::Append(&response, response_buffer);
  response.resize(64);
  return response;
}

// Returns a APDU encoded U2F version request for testing.
std::vector<uint8_t> GetMockDeviceRequest() {
  return fido_parsing_utils::Materialize(kMockU2fRequest);
}

device::mojom::HidDeviceInfoPtr TestHidDevice() {
  auto c_info = device::mojom::HidCollectionInfo::New();
  c_info->usage = device::mojom::HidUsageAndPage::New(1, 0xf1d0);
  auto hid_device = device::mojom::HidDeviceInfo::New();
  hid_device->guid = "A";
  hid_device->product_name = "Test Fido device";
  hid_device->serial_number = "123FIDO";
  hid_device->bus_type = device::mojom::HidBusType::kHIDBusTypeUSB;
  hid_device->collections.push_back(std::move(c_info));
  hid_device->max_input_report_size = 64;
  hid_device->max_output_report_size = 64;
  return hid_device;
}

class FidoDeviceEnumerateCallbackReceiver
    : public test::TestCallbackReceiver<std::vector<mojom::HidDeviceInfoPtr>> {
 public:
  explicit FidoDeviceEnumerateCallbackReceiver(
      device::mojom::HidManager* hid_manager)
      : hid_manager_(hid_manager) {}
  ~FidoDeviceEnumerateCallbackReceiver() = default;

  std::vector<std::unique_ptr<FidoHidDevice>> TakeReturnedDevicesFiltered() {
    std::vector<std::unique_ptr<FidoHidDevice>> filtered_results;
    std::vector<mojom::HidDeviceInfoPtr> results;
    std::tie(results) = TakeResult();
    for (auto& device_info : results) {
      HidDeviceFilter filter;
      filter.SetUsagePage(0xf1d0);
      if (filter.Matches(*device_info)) {
        filtered_results.push_back(std::make_unique<FidoHidDevice>(
            std::move(device_info), hid_manager_));
      }
    }
    return filtered_results;
  }

 private:
  device::mojom::HidManager* hid_manager_;

  DISALLOW_COPY_AND_ASSIGN(FidoDeviceEnumerateCallbackReceiver);
};

using TestDeviceCallbackReceiver =
    ::device::test::ValueCallbackReceiver<base::Optional<std::vector<uint8_t>>>;

}  // namespace

class FidoHidDeviceTest : public ::testing::Test {
 public:
  void SetUp() override {
    fake_hid_manager_ = std::make_unique<FakeHidManager>();
    fake_hid_manager_->AddBinding2(mojo::MakeRequest(&hid_manager_));
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_{
      base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME};
  device::mojom::HidManagerPtr hid_manager_;
  std::unique_ptr<FakeHidManager> fake_hid_manager_;
};

TEST_F(FidoHidDeviceTest, TestConnectionFailure) {
  // Setup and enumerate mock device.
  FidoDeviceEnumerateCallbackReceiver receiver(hid_manager_.get());
  auto hid_device = TestHidDevice();
  fake_hid_manager_->AddDevice(std::move(hid_device));
  hid_manager_->GetDevices(receiver.callback());
  receiver.WaitForCallback();

  std::vector<std::unique_ptr<FidoHidDevice>> u2f_devices =
      receiver.TakeReturnedDevicesFiltered();

  ASSERT_EQ(static_cast<size_t>(1), u2f_devices.size());
  auto& device = u2f_devices.front();
  // Put device in IDLE state.
  device->state_ = FidoDevice::State::kReady;

  // Manually delete connection.
  device->connection_ = nullptr;

  // Add pending transactions manually and ensure they are processed.
  TestDeviceCallbackReceiver receiver_1;
  device->pending_transactions_.emplace(GetMockDeviceRequest(),
                                        receiver_1.callback());
  TestDeviceCallbackReceiver receiver_2;
  device->pending_transactions_.emplace(GetMockDeviceRequest(),
                                        receiver_2.callback());
  TestDeviceCallbackReceiver receiver_3;
  device->DeviceTransact(GetMockDeviceRequest(), receiver_3.callback());

  EXPECT_EQ(FidoDevice::State::kDeviceError, device->state_);

  EXPECT_FALSE(receiver_1.value());
  EXPECT_FALSE(receiver_2.value());
  EXPECT_FALSE(receiver_3.value());
}

TEST_F(FidoHidDeviceTest, TestDeviceError) {
  // Setup and enumerate mock device.
  FidoDeviceEnumerateCallbackReceiver receiver(hid_manager_.get());

  auto hid_device = TestHidDevice();
  fake_hid_manager_->AddDevice(std::move(hid_device));
  hid_manager_->GetDevices(receiver.callback());
  receiver.WaitForCallback();

  std::vector<std::unique_ptr<FidoHidDevice>> u2f_devices =
      receiver.TakeReturnedDevicesFiltered();

  ASSERT_EQ(static_cast<size_t>(1), u2f_devices.size());
  auto& device = u2f_devices.front();

  // Mock connection where writes always fail.
  FakeHidConnection::mock_connection_error_ = true;
  device->state_ = FidoDevice::State::kReady;

  TestDeviceCallbackReceiver receiver_0;
  device->DeviceTransact(GetMockDeviceRequest(), receiver_0.callback());
  EXPECT_FALSE(receiver_0.value());
  EXPECT_EQ(FidoDevice::State::kDeviceError, device->state_);

  // Add pending transactions manually and ensure they are processed.
  TestDeviceCallbackReceiver receiver_1;
  device->pending_transactions_.emplace(GetMockDeviceRequest(),
                                        receiver_1.callback());
  TestDeviceCallbackReceiver receiver_2;
  device->pending_transactions_.emplace(GetMockDeviceRequest(),
                                        receiver_2.callback());
  TestDeviceCallbackReceiver receiver_3;
  device->DeviceTransact(GetMockDeviceRequest(), receiver_3.callback());
  FakeHidConnection::mock_connection_error_ = false;

  EXPECT_EQ(FidoDevice::State::kDeviceError, device->state_);
  EXPECT_FALSE(receiver_1.value());
  EXPECT_FALSE(receiver_2.value());
  EXPECT_FALSE(receiver_3.value());
}

TEST_F(FidoHidDeviceTest, TestRetryChannelAllocation) {
  constexpr uint8_t kIncorrectNonce[] = {0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00};

  constexpr uint8_t kChannelId[] = {0x01, 0x02, 0x03, 0x04};

  auto hid_device = TestHidDevice();

  // Replace device HID connection with custom client connection bound to mock
  // server-side mojo connection.
  device::mojom::HidConnectionPtr connection_client;
  MockHidConnection mock_connection(
      hid_device.Clone(), mojo::MakeRequest(&connection_client),
      fido_parsing_utils::Materialize(kChannelId));

  // Initial write for establishing a channel ID.
  mock_connection.ExpectWriteHidInit();

  // HID_MSG request to authenticator for version request.
  mock_connection.ExpectHidWriteWithCommand(FidoHidDeviceCommand::kMsg);

  EXPECT_CALL(mock_connection, ReadPtr(_))
      // First response to HID_INIT request with an incorrect nonce.
      .WillOnce(
          Invoke([kIncorrectNonce, &mock_connection](auto* cb) {
            std::move(*cb).Run(
                true, 0,
                CreateMockInitResponse(
                    kIncorrectNonce, mock_connection.connection_channel_id()));
          }))
      // Second response to HID_INIT request with a correct nonce.
      .WillOnce(Invoke(
          [&mock_connection](device::mojom::HidConnection::ReadCallback* cb) {
            std::move(*cb).Run(true, 0,
                               CreateMockInitResponse(
                                   mock_connection.nonce(),
                                   mock_connection.connection_channel_id()));
          }))
      // Version response from the authenticator.
      .WillOnce(Invoke(
          [&mock_connection](device::mojom::HidConnection::ReadCallback* cb) {
            std::move(*cb).Run(true, 0,
                               CreateMockResponseWithChannelId(
                                   mock_connection.connection_channel_id(),
                                   kU2fMockResponseMessage));
          }));

  // Add device and set mock connection to fake hid manager.
  fake_hid_manager_->AddDeviceAndSetConnection(std::move(hid_device),
                                               std::move(connection_client));
  FidoDeviceEnumerateCallbackReceiver receiver(hid_manager_.get());
  hid_manager_->GetDevices(receiver.callback());
  receiver.WaitForCallback();

  std::vector<std::unique_ptr<FidoHidDevice>> u2f_devices =
      receiver.TakeReturnedDevicesFiltered();
  ASSERT_EQ(1u, u2f_devices.size());
  auto& device = u2f_devices.front();

  TestDeviceCallbackReceiver cb;
  device->DeviceTransact(GetMockDeviceRequest(), cb.callback());
  cb.WaitForCallback();

  const auto& value = cb.value();
  ASSERT_TRUE(value);
  EXPECT_THAT(*value, testing::ElementsAreArray(kU2fMockResponseData));
}

TEST_F(FidoHidDeviceTest, TestKeepAliveMessage) {
  constexpr uint8_t kChannelId[] = {0x01, 0x02, 0x03, 0x04};

  auto hid_device = TestHidDevice();

  // Replace device HID connection with custom client connection bound to mock
  // server-side mojo connection.
  device::mojom::HidConnectionPtr connection_client;
  MockHidConnection mock_connection(
      hid_device.Clone(), mojo::MakeRequest(&connection_client),
      fido_parsing_utils::Materialize(kChannelId));

  // Initial write for establishing channel ID.
  mock_connection.ExpectWriteHidInit();

  // HID_CBOR request to authenticator.
  mock_connection.ExpectHidWriteWithCommand(FidoHidDeviceCommand::kCbor);

  EXPECT_CALL(mock_connection, ReadPtr(_))
      // Response to HID_INIT request.
      .WillOnce(Invoke([&](device::mojom::HidConnection::ReadCallback* cb) {
        std::move(*cb).Run(
            true, 0,
            CreateMockInitResponse(mock_connection.nonce(),
                                   mock_connection.connection_channel_id()));
      }))
      // // Keep alive message sent from the authenticator.
      .WillOnce(Invoke([&](device::mojom::HidConnection::ReadCallback* cb) {
        std::move(*cb).Run(
            true, 0,
            GetKeepAliveHidMessage(mock_connection.connection_channel_id()));
      }))
      // Repeated Read() invocation due to keep alive message. Sends a dummy
      // response that corresponds to U2F version response.
      .WillOnce(Invoke([&](device::mojom::HidConnection::ReadCallback* cb) {
        auto almost_time_out =
            kDeviceTimeout - base::TimeDelta::FromMicroseconds(1);
        scoped_task_environment_.FastForwardBy(almost_time_out);

        std::move(*cb).Run(true, 0,
                           CreateMockResponseWithChannelId(
                               mock_connection.connection_channel_id(),
                               kU2fMockResponseMessage));
      }));

  // Add device and set mock connection to fake hid manager.
  fake_hid_manager_->AddDeviceAndSetConnection(std::move(hid_device),
                                               std::move(connection_client));

  FidoDeviceEnumerateCallbackReceiver receiver(hid_manager_.get());
  hid_manager_->GetDevices(receiver.callback());
  receiver.WaitForCallback();

  std::vector<std::unique_ptr<FidoHidDevice>> u2f_devices =
      receiver.TakeReturnedDevicesFiltered();
  ASSERT_EQ(1u, u2f_devices.size());
  auto& device = u2f_devices.front();

  // Keep alive message handling is only supported for CTAP HID device.
  device->set_supported_protocol(ProtocolVersion::kCtap);
  TestDeviceCallbackReceiver cb;
  device->DeviceTransact(GetMockDeviceRequest(), cb.callback());
  cb.WaitForCallback();
  const auto& value = cb.value();
  ASSERT_TRUE(value);
  EXPECT_THAT(*value, testing::ElementsAreArray(kU2fMockResponseData));
}

TEST_F(FidoHidDeviceTest, TestDeviceTimeoutAfterKeepAliveMessage) {
  constexpr uint8_t kChannelId[] = {0x01, 0x02, 0x03, 0x04};

  auto hid_device = TestHidDevice();

  // Replace device HID connection with custom client connection bound to mock
  // server-side mojo connection.
  device::mojom::HidConnectionPtr connection_client;
  MockHidConnection mock_connection(
      hid_device.Clone(), mojo::MakeRequest(&connection_client),
      fido_parsing_utils::Materialize(kChannelId));

  // Initial write for establishing channel ID.
  mock_connection.ExpectWriteHidInit();

  // HID_CBOR request to authenticator.
  mock_connection.ExpectHidWriteWithCommand(FidoHidDeviceCommand::kCbor);

  EXPECT_CALL(mock_connection, ReadPtr(_))
      // Response to HID_INIT request.
      .WillOnce(Invoke([&](device::mojom::HidConnection::ReadCallback* cb) {
        std::move(*cb).Run(
            true, 0,
            CreateMockInitResponse(mock_connection.nonce(),
                                   mock_connection.connection_channel_id()));
      }))
      // // Keep alive message sent from the authenticator.
      .WillOnce(Invoke([&](device::mojom::HidConnection::ReadCallback* cb) {
        std::move(*cb).Run(
            true, 0,
            GetKeepAliveHidMessage(mock_connection.connection_channel_id()));
      }))
      // Repeated Read() invocation due to keep alive message. The callback
      // is invoked only after 3 seconds, which should cause device to timeout.
      .WillOnce(Invoke([&](device::mojom::HidConnection::ReadCallback* cb) {
        scoped_task_environment_.FastForwardBy(kDeviceTimeout);
        std::move(*cb).Run(true, 0,
                           CreateMockResponseWithChannelId(
                               mock_connection.connection_channel_id(),
                               kU2fMockResponseMessage));
      }));

  // Add device and set mock connection to fake hid manager.
  fake_hid_manager_->AddDeviceAndSetConnection(std::move(hid_device),
                                               std::move(connection_client));

  FidoDeviceEnumerateCallbackReceiver receiver(hid_manager_.get());
  hid_manager_->GetDevices(receiver.callback());
  receiver.WaitForCallback();

  std::vector<std::unique_ptr<FidoHidDevice>> u2f_devices =
      receiver.TakeReturnedDevicesFiltered();
  ASSERT_EQ(1u, u2f_devices.size());
  auto& device = u2f_devices.front();

  // Keep alive message handling is only supported for CTAP HID device.
  device->set_supported_protocol(ProtocolVersion::kCtap);
  TestDeviceCallbackReceiver cb;
  device->DeviceTransact(GetMockDeviceRequest(), cb.callback());
  cb.WaitForCallback();
  const auto& value = cb.value();
  EXPECT_FALSE(value);
  EXPECT_EQ(FidoDevice::State::kDeviceError, device->state());
}

TEST_F(FidoHidDeviceTest, TestCancel) {
  constexpr uint8_t kChannelId[] = {0x01, 0x02, 0x03, 0x04};

  auto hid_device = TestHidDevice();

  // Replace device HID connection with custom client connection bound to mock
  // server-side mojo connection.
  device::mojom::HidConnectionPtr connection_client;
  MockHidConnection mock_connection(
      hid_device.Clone(), mojo::MakeRequest(&connection_client),
      fido_parsing_utils::Materialize(kChannelId));

  // Initial write for establishing channel ID.
  mock_connection.ExpectWriteHidInit();

  // HID_CBOR request to authenticator.
  mock_connection.ExpectHidWriteWithCommand(FidoHidDeviceCommand::kCbor);

  // Cancel request to authenticator.
  mock_connection.ExpectHidWriteWithCommand(FidoHidDeviceCommand::kCancel);

  EXPECT_CALL(mock_connection, ReadPtr(_))
      // Response to HID_INIT request.
      .WillOnce(Invoke([&](device::mojom::HidConnection::ReadCallback* cb) {
        std::move(*cb).Run(
            true, 0,
            CreateMockInitResponse(mock_connection.nonce(),
                                   mock_connection.connection_channel_id()));
      }))
      // Device response with a significant delay.
      .WillOnce(Invoke([&](device::mojom::HidConnection::ReadCallback* cb) {
        auto delay = base::TimeDelta::FromSeconds(2);
        base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
            FROM_HERE,
            base::BindOnce(std::move(*cb), true, 0,
                           CreateMockResponseWithChannelId(
                               mock_connection.connection_channel_id(),
                               kU2fMockResponseMessage)),
            delay);
      }));

  // Add device and set mock connection to fake hid manager.
  fake_hid_manager_->AddDeviceAndSetConnection(std::move(hid_device),
                                               std::move(connection_client));

  FidoDeviceEnumerateCallbackReceiver receiver(hid_manager_.get());
  hid_manager_->GetDevices(receiver.callback());
  receiver.WaitForCallback();

  std::vector<std::unique_ptr<FidoHidDevice>> u2f_devices =
      receiver.TakeReturnedDevicesFiltered();
  ASSERT_EQ(1u, u2f_devices.size());
  auto& device = u2f_devices.front();

  // Keep alive message handling is only supported for CTAP HID device.
  device->set_supported_protocol(ProtocolVersion::kCtap);
  TestDeviceCallbackReceiver cb;
  device->DeviceTransact(GetMockDeviceRequest(), cb.callback());
  device->Cancel();
  scoped_task_environment_.FastForwardUntilNoTasksRemain();
}

}  // namespace device
