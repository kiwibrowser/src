// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/memory/ref_counted.h"
#include "device/usb/mock_usb_device.h"
#include "device/usb/usb_descriptors.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/features/feature_session_type.h"
#include "extensions/common/permissions/usb_device_permission.h"
#include "extensions/common/permissions/usb_device_permission_data.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

// ID of test USB device config to which interfaces described by
// |interface_classes| parameter of |CreateTestUsbDevice| should be added.
const int kUsbConfigWithInterfaces = 1;

// ID of test USB device config created by |CreateTestUsbDevice| that contains
// no interfaces.
const int kUsbConfigWithoutInterfaces = 2;

scoped_refptr<device::MockUsbDevice> CreateTestUsbDevice(
    uint16_t vendor_id,
    uint16_t product_id,
    uint8_t device_class,
    const std::vector<uint8_t> interface_classes) {
  std::vector<device::UsbConfigDescriptor> configs;
  configs.emplace_back(kUsbConfigWithInterfaces, false, false, 0);
  configs.emplace_back(kUsbConfigWithoutInterfaces, false, false, 0);

  for (size_t i = 0; i < interface_classes.size(); ++i)
    configs[0].interfaces.emplace_back(i, 0, interface_classes[i], 255, 255);

  return new device::MockUsbDevice(vendor_id, product_id, device_class,
                                   configs);
}

scoped_refptr<const Extension> CreateTestApp(
    std::unique_ptr<base::Value> usb_device_permission) {
  return ExtensionBuilder()
      .SetManifest(
          DictionaryBuilder()
              .Set("name", "test app")
              .Set("version", "1")
              .Set("app",
                   DictionaryBuilder()
                       .Set("background",
                            DictionaryBuilder()
                                .Set("scripts", ListBuilder()
                                                    .Append("background.js")
                                                    .Build())
                                .Build())
                       .Build())
              .Set("permissions",
                   ListBuilder()
                       .Append("usb")
                       .Append(DictionaryBuilder()
                                   .Set("usbDevices",
                                        ListBuilder()
                                            .Append(std::move(
                                                usb_device_permission))
                                            .Build())
                                   .Build())
                       .Build())
              .Build())
      .Build();
}

}  // namespace

TEST(USBDevicePermissionTest, PermissionDataOrder) {
  EXPECT_LT(UsbDevicePermissionData(0x02ad, 0x138c, -1, -1),
            UsbDevicePermissionData(0x02ad, 0x138d, -1, -1));
  ASSERT_LT(UsbDevicePermissionData(0x02ad, 0x138d, -1, -1),
            UsbDevicePermissionData(0x02ae, 0x138c, -1, -1));
  EXPECT_LT(UsbDevicePermissionData(0x02ad, 0x138c, 1, 3),
            UsbDevicePermissionData(0x02ad, 0x138c, 2, 2));
  EXPECT_LT(UsbDevicePermissionData(0x02ad, 0x138c, 1, 2),
            UsbDevicePermissionData(0x02ad, 0x138c, 1, 3));
}

TEST(USBDevicePermissionTest, CheckVendorAndProductId) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder()
          .Set("vendorId", 0x02ad)
          .Set("productId", 0x138c)
          .Build();

  UsbDevicePermissionData permission_data;
  ASSERT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x02ad, 0x138c,
            UsbDevicePermissionData::SPECIAL_VALUE_UNSPECIFIED);

    EXPECT_TRUE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x138c, 0x02ad,
            UsbDevicePermissionData::SPECIAL_VALUE_UNSPECIFIED);

    EXPECT_FALSE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x02ad, 0x138c, 3);

    EXPECT_FALSE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, CheckInterfaceId) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder()
          .Set("vendorId", 0x02ad)
          .Set("productId", 0x138c)
          .Set("interfaceId", 3)
          .Build();
  UsbDevicePermissionData permission_data;
  ASSERT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));
  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x02ad, 0x138c, 3);

    EXPECT_TRUE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x02ad, 0x138c, 2);

    EXPECT_FALSE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x02ad, 0x138c,
            UsbDevicePermissionData::SPECIAL_VALUE_UNSPECIFIED);

    EXPECT_TRUE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, InterfaceClass) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder().Set("interfaceClass", 3).Build();
  UsbDevicePermissionData permission_data;
  EXPECT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x02ad, 0x138c,
            UsbDevicePermissionData::SPECIAL_VALUE_UNSPECIFIED);

    // Interface class permission should be ignored when not in kiosk session.
    EXPECT_FALSE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x02ad, 0x138c,
            UsbDevicePermissionData::SPECIAL_VALUE_UNSPECIFIED);

    // |ForDeviceWithAnyInterfaceClass| creates check param that matches any
    // interfaceClass permission property - needed in cases where set of
    // interface classes supported by a USB device is not known.
    // In this case, there is a usbDevice permisison with only interfaceClass
    // set, so the param is accepted.
    EXPECT_TRUE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, InterfaceClassWithVendorId) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder()
          .Set("vendorId", 0x02ad)
          .Set("interfaceClass", 3)
          .Build();
  UsbDevicePermissionData permission_data;
  EXPECT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x02ad, 0x138c,
            UsbDevicePermissionData::SPECIAL_VALUE_UNSPECIFIED);

    EXPECT_FALSE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x02ad, 0x138c,
            UsbDevicePermissionData::SPECIAL_VALUE_UNSPECIFIED);

    EXPECT_TRUE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForDeviceWithAnyInterfaceClass(
            app.get(), 0x138c, 0x138c,
            UsbDevicePermissionData::SPECIAL_VALUE_UNSPECIFIED);

    EXPECT_FALSE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, CheckHidUsbAgainstInterfaceClass) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder()
          .Set("vendorId", 0x02ad)
          .Set("interfaceClass", 3)
          .Build();
  UsbDevicePermissionData permission_data;
  EXPECT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForHidDevice(app.get(), 0x02ad,
                                                      0x138c);

    EXPECT_FALSE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForHidDevice(app.get(), 0x02ad,
                                                      0x138c);

    EXPECT_TRUE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForHidDevice(app.get(), 0x138c,
                                                      0x138c);

    EXPECT_FALSE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, CheckHidUsbAgainstDeviceIds) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder()
          .Set("vendorId", 0x02ad)
          .Set("productId", 0x138c)
          .Build();
  UsbDevicePermissionData permission_data;
  EXPECT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForHidDevice(app.get(), 0x02ad,
                                                      0x138c);

    EXPECT_TRUE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForHidDevice(app.get(), 0x138c,
                                                      0x138c);

    EXPECT_FALSE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, CheckDeviceAgainstDeviceIds) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder()
          .Set("vendorId", 0x02ad)
          .Set("productId", 0x138c)
          .Build();
  UsbDevicePermissionData permission_data;
  EXPECT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0x9, std::vector<uint8_t>());
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_TRUE(permission_data.Check(param.get()));
  }

  {
    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x138c, 0x138c, 0x9, std::vector<uint8_t>());
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_FALSE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, CheckDeviceAgainstDeviceClass) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder().Set("interfaceClass", 0x9).Build();
  UsbDevicePermissionData permission_data;
  EXPECT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0x9, std::vector<uint8_t>());
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_FALSE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0x9, std::vector<uint8_t>());
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_TRUE(permission_data.Check(param.get()));
  }
  {
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0x3, std::vector<uint8_t>());
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_FALSE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, IgnoreNullDeviceClass) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder().Set("interfaceClass", 0).Build();
  UsbDevicePermissionData permission_data;
  EXPECT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0, std::vector<uint8_t>());
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_FALSE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, CheckDeviceAgainstInterfaceClass) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder().Set("interfaceClass", 0x3).Build();
  UsbDevicePermissionData permission_data;
  EXPECT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0, {2, 3});
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_FALSE(permission_data.Check(param.get()));
  }

  {
    // Interface should match inactive configuration when none of configurations
    // is active.
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0, {2, 3});
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_TRUE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0, {2, 3});
    device->ActiveConfigurationChanged(kUsbConfigWithInterfaces);
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_TRUE(permission_data.Check(param.get()));
  }

  {
    // Interface should match inactive configuration when another configuration
    // is active.
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0, {2, 3});
    device->ActiveConfigurationChanged(kUsbConfigWithoutInterfaces);
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_TRUE(permission_data.Check(param.get()));
  }

  {
    std::unique_ptr<base::AutoReset<FeatureSessionType>> scoped_session_type(
        ScopedCurrentFeatureSessionType(FeatureSessionType::KIOSK));
    ScopedCurrentChannel channel(version_info::Channel::DEV);

    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0, {4, 5});
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDevice(app.get(), device.get());

    EXPECT_FALSE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, CheckDeviceAndInterfaceId) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder()
          .Set("vendorId", 0x02ad)
          .Set("productId", 0x138c)
          .Set("interfaceId", 3)
          .Build();
  UsbDevicePermissionData permission_data;
  EXPECT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0, std::vector<uint8_t>());
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDeviceAndInterface(
            app.get(), device.get(), 3);

    EXPECT_TRUE(permission_data.Check(param.get()));
  }

  {
    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0, std::vector<uint8_t>());
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDeviceAndInterface(
            app.get(), device.get(), 2);

    EXPECT_FALSE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest,
     CheckDeviceAndInterfaceIDAgainstMissingInterfaceId) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder()
          .Set("vendorId", 0x02ad)
          .Set("productId", 0x138c)
          .Build();
  UsbDevicePermissionData permission_data;
  EXPECT_TRUE(permission_data.FromValue(permission_data_value.get()));

  scoped_refptr<const Extension> app =
      CreateTestApp(std::move(permission_data_value));

  {
    scoped_refptr<device::MockUsbDevice> device =
        CreateTestUsbDevice(0x02ad, 0x138c, 0, std::vector<uint8_t>());
    std::unique_ptr<UsbDevicePermission::CheckParam> param =
        UsbDevicePermission::CheckParam::ForUsbDeviceAndInterface(
            app.get(), device.get(), 3);

    EXPECT_FALSE(permission_data.Check(param.get()));
  }
}

TEST(USBDevicePermissionTest, InvalidPermission_NoVendorId) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder()
          .Set("productId", 0x138c)
          .Set("interfaceClass", 3)
          .Build();
  UsbDevicePermissionData permission_data;
  ASSERT_FALSE(permission_data.FromValue(permission_data_value.get()));
}

TEST(USBDevicePermissionTest, InvalidPermission_OnlyVendorId) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder().Set("vendorId", 0x02ad).Build();
  UsbDevicePermissionData permission_data;
  ASSERT_FALSE(permission_data.FromValue(permission_data_value.get()));
}

TEST(USBDevicePermissionTest, InvalidPermission_NoProductIdWithInterfaceId) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder().Set("vendorId", 0x02ad).Set("interfaceId", 3).Build();
  UsbDevicePermissionData permission_data;
  ASSERT_FALSE(permission_data.FromValue(permission_data_value.get()));
}

TEST(USBDevicePermissionTest, RejectInterfaceIdIfInterfaceClassPresent) {
  std::unique_ptr<base::Value> permission_data_value =
      DictionaryBuilder()
          .Set("vendorId", 0x02ad)
          .Set("productId", 0x128c)
          .Set("interfaceId", 3)
          .Set("interfaceClass", 7)
          .Build();
  UsbDevicePermissionData permission_data;
  ASSERT_FALSE(permission_data.FromValue(permission_data_value.get()));
}

}  // namespace extensions
