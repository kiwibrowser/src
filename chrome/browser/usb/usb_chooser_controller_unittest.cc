// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/usb/usb_chooser_controller.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/web_contents_tester.h"
#include "device/base/mock_device_client.h"
#include "device/usb/mock_usb_device.h"
#include "device/usb/mock_usb_service.h"
#include "device/usb/public/mojom/device_manager.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

const char kDefaultTestUrl[] = "https://www.google.com/";

class MockUsbChooserView : public ChooserController::View {
 public:
  MockUsbChooserView() {}

  // ChooserController::View:
  MOCK_METHOD1(OnOptionAdded, void(size_t index));
  MOCK_METHOD1(OnOptionRemoved, void(size_t index));
  void OnOptionsInitialized() override {}
  void OnOptionUpdated(size_t index) override {}
  void OnAdapterEnabledChanged(bool enabled) override {}
  void OnRefreshStateChanged(bool enabled) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(MockUsbChooserView);
};

}  //  namespace

class UsbChooserControllerTest : public ChromeRenderViewHostTestHarness {
 public:
  UsbChooserControllerTest() {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    std::vector<device::mojom::UsbDeviceFilterPtr> device_filters;
    device::mojom::UsbChooserService::GetPermissionCallback callback;
    content::WebContentsTester* web_contents_tester =
        content::WebContentsTester::For(web_contents());
    web_contents_tester->NavigateAndCommit(GURL(kDefaultTestUrl));
    usb_chooser_controller_.reset(new UsbChooserController(
        main_rfh(), std::move(device_filters), std::move(callback)));
    mock_usb_chooser_view_.reset(new MockUsbChooserView());
    usb_chooser_controller_->set_view(mock_usb_chooser_view_.get());
  }

 protected:
  scoped_refptr<device::MockUsbDevice> CreateMockUsbDevice(
      const std::string& product_string,
      const std::string& serial_number) {
    scoped_refptr<device::MockUsbDevice> device(new device::MockUsbDevice(
        0, 1, "Google", product_string, serial_number));
    return device;
  }

  device::MockDeviceClient device_client_;
  std::unique_ptr<UsbChooserController> usb_chooser_controller_;
  std::unique_ptr<MockUsbChooserView> mock_usb_chooser_view_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UsbChooserControllerTest);
};

TEST_F(UsbChooserControllerTest, AddDevice) {
  scoped_refptr<device::MockUsbDevice> device_a =
      CreateMockUsbDevice("a", "001");
  EXPECT_CALL(*mock_usb_chooser_view_, OnOptionAdded(0)).Times(1);
  device_client_.usb_service()->AddDevice(device_a);
  EXPECT_EQ(1u, usb_chooser_controller_->NumOptions());
  EXPECT_EQ(base::ASCIIToUTF16("a"), usb_chooser_controller_->GetOption(0));

  scoped_refptr<device::MockUsbDevice> device_b =
      CreateMockUsbDevice("b", "002");
  EXPECT_CALL(*mock_usb_chooser_view_, OnOptionAdded(1)).Times(1);
  device_client_.usb_service()->AddDevice(device_b);
  EXPECT_EQ(2u, usb_chooser_controller_->NumOptions());
  EXPECT_EQ(base::ASCIIToUTF16("b"), usb_chooser_controller_->GetOption(1));

  scoped_refptr<device::MockUsbDevice> device_c =
      CreateMockUsbDevice("c", "003");
  EXPECT_CALL(*mock_usb_chooser_view_, OnOptionAdded(2)).Times(1);
  device_client_.usb_service()->AddDevice(device_c);
  EXPECT_EQ(3u, usb_chooser_controller_->NumOptions());
  EXPECT_EQ(base::ASCIIToUTF16("c"), usb_chooser_controller_->GetOption(2));
}

TEST_F(UsbChooserControllerTest, RemoveDevice) {
  scoped_refptr<device::MockUsbDevice> device_a =
      CreateMockUsbDevice("a", "001");
  device_client_.usb_service()->AddDevice(device_a);
  scoped_refptr<device::MockUsbDevice> device_b =
      CreateMockUsbDevice("b", "002");
  device_client_.usb_service()->AddDevice(device_b);
  scoped_refptr<device::MockUsbDevice> device_c =
      CreateMockUsbDevice("c", "003");
  device_client_.usb_service()->AddDevice(device_c);

  EXPECT_CALL(*mock_usb_chooser_view_, OnOptionRemoved(1)).Times(1);
  device_client_.usb_service()->RemoveDevice(device_b);
  EXPECT_EQ(2u, usb_chooser_controller_->NumOptions());
  EXPECT_EQ(base::ASCIIToUTF16("a"), usb_chooser_controller_->GetOption(0));
  EXPECT_EQ(base::ASCIIToUTF16("c"), usb_chooser_controller_->GetOption(1));

  // Remove a non-existent device, the number of devices should not change.
  scoped_refptr<device::MockUsbDevice> device_non_existent =
      CreateMockUsbDevice("d", "001");
  device_client_.usb_service()->RemoveDevice(device_non_existent);
  EXPECT_EQ(2u, usb_chooser_controller_->NumOptions());
  EXPECT_EQ(base::ASCIIToUTF16("a"), usb_chooser_controller_->GetOption(0));
  EXPECT_EQ(base::ASCIIToUTF16("c"), usb_chooser_controller_->GetOption(1));

  EXPECT_CALL(*mock_usb_chooser_view_, OnOptionRemoved(0)).Times(1);
  device_client_.usb_service()->RemoveDevice(device_a);
  EXPECT_EQ(1u, usb_chooser_controller_->NumOptions());
  EXPECT_EQ(base::ASCIIToUTF16("c"), usb_chooser_controller_->GetOption(0));

  EXPECT_CALL(*mock_usb_chooser_view_, OnOptionRemoved(0)).Times(1);
  device_client_.usb_service()->RemoveDevice(device_c);
  EXPECT_EQ(0u, usb_chooser_controller_->NumOptions());
}

TEST_F(UsbChooserControllerTest, AddAndRemoveDeviceWithSameName) {
  scoped_refptr<device::MockUsbDevice> device_a_1 =
      CreateMockUsbDevice("a", "001");
  device_client_.usb_service()->AddDevice(device_a_1);
  EXPECT_EQ(base::ASCIIToUTF16("a"), usb_chooser_controller_->GetOption(0));
  scoped_refptr<device::MockUsbDevice> device_b =
      CreateMockUsbDevice("b", "002");
  device_client_.usb_service()->AddDevice(device_b);
  scoped_refptr<device::MockUsbDevice> device_a_2 =
      CreateMockUsbDevice("a", "002");
  device_client_.usb_service()->AddDevice(device_a_2);
  EXPECT_EQ(base::ASCIIToUTF16("a (001)"),
            usb_chooser_controller_->GetOption(0));
  EXPECT_EQ(base::ASCIIToUTF16("b"), usb_chooser_controller_->GetOption(1));
  EXPECT_EQ(base::ASCIIToUTF16("a (002)"),
            usb_chooser_controller_->GetOption(2));

  device_client_.usb_service()->RemoveDevice(device_a_1);
  EXPECT_EQ(base::ASCIIToUTF16("b"), usb_chooser_controller_->GetOption(0));
  EXPECT_EQ(base::ASCIIToUTF16("a"), usb_chooser_controller_->GetOption(1));
}

TEST_F(UsbChooserControllerTest, UnknownDeviceName) {
  uint16_t vendor_id = 123;
  uint16_t product_id = 456;
  scoped_refptr<device::MockUsbDevice> device =
      new device::MockUsbDevice(vendor_id, product_id);
  device_client_.usb_service()->AddDevice(device);
  EXPECT_EQ(base::ASCIIToUTF16("Unknown device [007b:01c8]"),
            usb_chooser_controller_->GetOption(0));
}
