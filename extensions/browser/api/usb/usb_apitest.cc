// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <numeric>

#include "base/memory/ref_counted_memory.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_utils.h"
#include "device/base/mock_device_client.h"
#include "device/usb/mock_usb_device.h"
#include "device/usb/mock_usb_device_handle.h"
#include "device/usb/mock_usb_service.h"
#include "extensions/browser/api/device_permissions_prompt.h"
#include "extensions/browser/api/usb/usb_api.h"
#include "extensions/shell/browser/shell_extensions_api_client.h"
#include "extensions/shell/test/shell_apitest.h"
#include "extensions/test/extension_test_message_listener.h"

using testing::_;
using testing::AnyNumber;
using testing::Invoke;
using testing::Return;
using device::MockDeviceClient;
using device::MockUsbDevice;
using device::MockUsbDeviceHandle;
using device::UsbConfigDescriptor;
using device::UsbControlTransferRecipient;
using device::UsbControlTransferType;
using device::UsbDeviceHandle;
using device::UsbInterfaceDescriptor;
using device::UsbTransferDirection;
using device::UsbTransferStatus;

namespace extensions {

namespace {

ACTION_TEMPLATE(InvokeCallback,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(p1)) {
  std::move(std::get<k>(args)).Run(p1);
}

ACTION_TEMPLATE(InvokeUsbTransferCallback,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(p1)) {
  scoped_refptr<base::RefCountedBytes> buffer;
  size_t length = 0;
  if (p1 != UsbTransferStatus::TRANSFER_ERROR) {
    length = 1;
    buffer = base::MakeRefCounted<base::RefCountedBytes>(length);
  }
  std::move(std::get<k>(args)).Run(p1, buffer, 1);
}

ACTION_P2(InvokeUsbIsochronousTransferOutCallback,
          transferred_length,
          success_packets) {
  std::vector<UsbDeviceHandle::IsochronousPacket> packets(arg2.size());
  for (size_t i = 0; i < packets.size(); ++i) {
    packets[i].length = arg2[i];
    if (i < success_packets) {
      packets[i].transferred_length = transferred_length;
      packets[i].status = UsbTransferStatus::COMPLETED;
    } else {
      packets[i].transferred_length = 0;
      packets[i].status = UsbTransferStatus::TRANSFER_ERROR;
    }
  }
  std::move(arg4).Run(arg1, packets);
}

ACTION_P2(InvokeUsbIsochronousTransferInCallback,
          transferred_length,
          success_packets) {
  size_t total_length = std::accumulate(arg1.begin(), arg1.end(), 0u);
  auto buffer = base::MakeRefCounted<base::RefCountedBytes>(total_length);
  std::vector<UsbDeviceHandle::IsochronousPacket> packets(arg1.size());
  for (size_t i = 0; i < packets.size(); ++i) {
    packets[i].length = arg1[i];
    packets[i].transferred_length = transferred_length;
    if (i < success_packets) {
      packets[i].transferred_length = transferred_length;
      packets[i].status = UsbTransferStatus::COMPLETED;
    } else {
      packets[i].transferred_length = 0;
      packets[i].status = UsbTransferStatus::TRANSFER_ERROR;
    }
  }
  std::move(arg3).Run(buffer, packets);
}

ACTION_P(SetConfiguration, mock_device) {
  mock_device->ActiveConfigurationChanged(arg0);
  std::move(arg1).Run(true);
}

MATCHER_P(BufferSizeIs, size, "") {
  return arg->size() == size;
}

class TestDevicePermissionsPrompt
    : public DevicePermissionsPrompt,
      public DevicePermissionsPrompt::Prompt::Observer {
 public:
  explicit TestDevicePermissionsPrompt(content::WebContents* web_contents)
      : DevicePermissionsPrompt(web_contents) {}

  void ShowDialog() override { prompt()->SetObserver(this); }

  void OnDeviceAdded(size_t index, const base::string16& device_name) override {
    OnDevicesChanged();
  }

  void OnDeviceRemoved(size_t index,
                       const base::string16& device_name) override {
    OnDevicesChanged();
  }

 private:
  void OnDevicesChanged() {
    for (size_t i = 0; i < prompt()->GetDeviceCount(); ++i) {
      prompt()->GrantDevicePermission(i);
      if (!prompt()->multiple()) {
        break;
      }
    }
    prompt()->Dismissed();
  }
};

class TestExtensionsAPIClient : public ShellExtensionsAPIClient {
 public:
  TestExtensionsAPIClient() : ShellExtensionsAPIClient() {}

  std::unique_ptr<DevicePermissionsPrompt> CreateDevicePermissionsPrompt(
      content::WebContents* web_contents) const override {
    return std::make_unique<TestDevicePermissionsPrompt>(web_contents);
  }
};

class UsbApiTest : public ShellApiTest {
 public:
  void SetUpOnMainThread() override {
    ShellApiTest::SetUpOnMainThread();

    // MockDeviceClient replaces ShellDeviceClient.
    device_client_.reset(new MockDeviceClient());

    std::vector<UsbConfigDescriptor> configs;
    configs.emplace_back(1, false, false, 0);
    configs.emplace_back(2, false, false, 0);

    mock_device_ = new MockUsbDevice(0, 0, "Test Manufacturer", "Test Device",
                                     "ABC123", configs);
    mock_device_handle_ = new MockUsbDeviceHandle(mock_device_.get());
    EXPECT_CALL(*mock_device_, OpenInternal(_))
        .WillRepeatedly(InvokeCallback<0>(mock_device_handle_));
    device_client_->usb_service()->AddDevice(mock_device_);
  }

 protected:
  scoped_refptr<MockUsbDeviceHandle> mock_device_handle_;
  scoped_refptr<MockUsbDevice> mock_device_;
  std::unique_ptr<MockDeviceClient> device_client_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(UsbApiTest, DeviceHandling) {
  mock_device_->ActiveConfigurationChanged(1);
  EXPECT_CALL(*mock_device_handle_, Close()).Times(2);
  ASSERT_TRUE(RunAppTest("api_test/usb/device_handling"));
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, ResetDevice) {
  EXPECT_CALL(*mock_device_handle_, Close()).Times(2);
  EXPECT_CALL(*mock_device_handle_, ResetDeviceInternal(_))
      .WillOnce(InvokeCallback<0>(true))
      .WillOnce(InvokeCallback<0>(false));
  EXPECT_CALL(*mock_device_handle_,
              GenericTransferInternal(UsbTransferDirection::OUTBOUND, 2,
                                      BufferSizeIs(1u), _, _))
      .WillOnce(InvokeUsbTransferCallback<4>(UsbTransferStatus::COMPLETED));
  ASSERT_TRUE(RunAppTest("api_test/usb/reset_device"));
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, SetConfiguration) {
  EXPECT_CALL(*mock_device_handle_, SetConfigurationInternal(1, _))
      .WillOnce(SetConfiguration(mock_device_.get()));
  EXPECT_CALL(*mock_device_handle_, Close()).Times(1);
  ASSERT_TRUE(RunAppTest("api_test/usb/set_configuration"));
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, ListInterfaces) {
  mock_device_->ActiveConfigurationChanged(1);
  EXPECT_CALL(*mock_device_handle_, Close()).Times(1);
  ASSERT_TRUE(RunAppTest("api_test/usb/list_interfaces"));
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, TransferEvent) {
  EXPECT_CALL(*mock_device_handle_,
              ControlTransferInternal(UsbTransferDirection::OUTBOUND,
                                      UsbControlTransferType::STANDARD,
                                      UsbControlTransferRecipient::DEVICE, 1, 2,
                                      3, BufferSizeIs(1u), _, _))
      .WillOnce(InvokeUsbTransferCallback<8>(UsbTransferStatus::COMPLETED));
  EXPECT_CALL(*mock_device_handle_,
              GenericTransferInternal(UsbTransferDirection::OUTBOUND, 1,
                                      BufferSizeIs(1u), _, _))
      .WillOnce(InvokeUsbTransferCallback<4>(UsbTransferStatus::COMPLETED));
  EXPECT_CALL(*mock_device_handle_,
              GenericTransferInternal(UsbTransferDirection::OUTBOUND, 2,
                                      BufferSizeIs(1u), _, _))
      .WillOnce(InvokeUsbTransferCallback<4>(UsbTransferStatus::COMPLETED));
  EXPECT_CALL(*mock_device_handle_,
              IsochronousTransferOutInternal(3, _, _, _, _))
      .WillOnce(InvokeUsbIsochronousTransferOutCallback(1, 1u));
  EXPECT_CALL(*mock_device_handle_, Close()).Times(AnyNumber());
  ASSERT_TRUE(RunAppTest("api_test/usb/transfer_event"));
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, ZeroLengthTransfer) {
  EXPECT_CALL(*mock_device_handle_,
              GenericTransferInternal(_, _, BufferSizeIs(0u), _, _))
      .WillOnce(InvokeUsbTransferCallback<4>(UsbTransferStatus::COMPLETED));
  EXPECT_CALL(*mock_device_handle_, Close()).Times(AnyNumber());
  ASSERT_TRUE(RunAppTest("api_test/usb/zero_length_transfer"));
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, TransferFailure) {
  EXPECT_CALL(
      *mock_device_handle_,
      GenericTransferInternal(UsbTransferDirection::OUTBOUND, 1, _, _, _))
      .WillOnce(InvokeUsbTransferCallback<4>(UsbTransferStatus::COMPLETED))
      .WillOnce(InvokeUsbTransferCallback<4>(UsbTransferStatus::TRANSFER_ERROR))
      .WillOnce(InvokeUsbTransferCallback<4>(UsbTransferStatus::TIMEOUT));
  EXPECT_CALL(*mock_device_handle_, IsochronousTransferInInternal(2, _, _, _))
      .WillOnce(InvokeUsbIsochronousTransferInCallback(8, 10u))
      .WillOnce(InvokeUsbIsochronousTransferInCallback(8, 5u));
  EXPECT_CALL(*mock_device_handle_, Close()).Times(AnyNumber());
  ASSERT_TRUE(RunAppTest("api_test/usb/transfer_failure"));
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, InvalidLengthTransfer) {
  EXPECT_CALL(*mock_device_handle_, Close()).Times(AnyNumber());
  ASSERT_TRUE(RunAppTest("api_test/usb/invalid_length_transfer"));
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, InvalidTimeout) {
  EXPECT_CALL(*mock_device_handle_, Close()).Times(AnyNumber());
  ASSERT_TRUE(RunAppTest("api_test/usb/invalid_timeout"));
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, OnDeviceAdded) {
  ExtensionTestMessageListener load_listener("loaded", false);
  ExtensionTestMessageListener result_listener("success", false);
  result_listener.set_failure_message("failure");

  ASSERT_TRUE(LoadApp("api_test/usb/add_event"));
  ASSERT_TRUE(load_listener.WaitUntilSatisfied());

  scoped_refptr<MockUsbDevice> device(new MockUsbDevice(0x18D1, 0x58F0));
  device_client_->usb_service()->AddDevice(device);

  device = new MockUsbDevice(0x18D1, 0x58F1);
  device_client_->usb_service()->AddDevice(device);

  ASSERT_TRUE(result_listener.WaitUntilSatisfied());
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, OnDeviceRemoved) {
  ExtensionTestMessageListener load_listener("loaded", false);
  ExtensionTestMessageListener result_listener("success", false);
  result_listener.set_failure_message("failure");

  ASSERT_TRUE(LoadApp("api_test/usb/remove_event"));
  ASSERT_TRUE(load_listener.WaitUntilSatisfied());

  device_client_->usb_service()->RemoveDevice(mock_device_);
  ASSERT_TRUE(result_listener.WaitUntilSatisfied());
}

IN_PROC_BROWSER_TEST_F(UsbApiTest, GetUserSelectedDevices) {
  ExtensionTestMessageListener ready_listener("opened_device", false);
  ExtensionTestMessageListener result_listener("success", false);
  result_listener.set_failure_message("failure");

  EXPECT_CALL(*mock_device_handle_, Close()).Times(1);

  TestExtensionsAPIClient test_api_client;
  ASSERT_TRUE(LoadApp("api_test/usb/get_user_selected_devices"));
  ASSERT_TRUE(ready_listener.WaitUntilSatisfied());

  device_client_->usb_service()->RemoveDevice(mock_device_);
  ASSERT_TRUE(result_listener.WaitUntilSatisfied());
}

}  // namespace extensions
