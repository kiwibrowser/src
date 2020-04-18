// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <tuple>
#include <utility>

#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "device/bluetooth/bluetooth_common.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "device/bluetooth/test/mock_bluetooth_device.h"
#include "device/bluetooth/test/mock_bluetooth_discovery_session.h"
#include "extensions/browser/api/bluetooth/bluetooth_api.h"
#include "extensions/browser/api/bluetooth/bluetooth_event_router.h"
#include "extensions/browser/event_router.h"
#include "extensions/common/api/bluetooth_private.h"
#include "extensions/common/switches.h"
#include "testing/gmock/include/gmock/gmock.h"

using device::BluetoothDiscoveryFilter;
using device::BluetoothUUID;
using device::MockBluetoothAdapter;
using device::MockBluetoothDevice;
using device::MockBluetoothDiscoverySession;
using testing::_;
using testing::Eq;
using testing::InSequence;
using testing::NiceMock;
using testing::Return;
using testing::ReturnPointee;
using testing::SaveArg;
using testing::WithArgs;
using testing::WithoutArgs;

namespace bt = extensions::api::bluetooth;
namespace bt_private = extensions::api::bluetooth_private;

namespace extensions {

namespace {
const char kTestExtensionId[] = "jofgjdphhceggjecimellaapdjjadibj";
const char kAdapterName[] = "Helix";
const char kDeviceName[] = "Red";
const char kDeviceAddress[] = "11:12:13:14:15:16";

MATCHER_P(IsFilterEqual, a, "") {
  return arg->Equals(*a);
}
}

class BluetoothPrivateApiTest : public ExtensionApiTest {
 public:
  BluetoothPrivateApiTest()
      : adapter_name_(kAdapterName),
        adapter_powered_(false),
        adapter_discoverable_(false) {}

  ~BluetoothPrivateApiTest() override {}

  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kWhitelistedExtensionID, kTestExtensionId);
    mock_adapter_ = new NiceMock<MockBluetoothAdapter>();
    event_router()->SetAdapterForTest(mock_adapter_.get());
    mock_device_.reset(new NiceMock<MockBluetoothDevice>(
        mock_adapter_.get(), 0, kDeviceName, kDeviceAddress, false, false));
    ON_CALL(*mock_adapter_, GetDevice(kDeviceAddress))
        .WillByDefault(Return(mock_device_.get()));
    ON_CALL(*mock_adapter_, IsPresent()).WillByDefault(Return(true));
  }

  BluetoothEventRouter* event_router() {
    return BluetoothAPI::Get(browser()->profile())->event_router();
  }

  void SetName(const std::string& name, const base::Closure& callback) {
    adapter_name_ = name;
    callback.Run();
  }

  void SetPowered(bool powered, const base::Closure& callback) {
    adapter_powered_ = powered;
    callback.Run();
  }

  void ForgetDevice(const base::Closure& callback) {
    mock_device_.reset();
    event_router()->SetAdapterForTest(nullptr);
    callback.Run();
  }

  void SetDiscoverable(bool discoverable, const base::Closure& callback) {
    adapter_discoverable_ = discoverable;
    callback.Run();
  }

  void DispatchPairingEvent(bt_private::PairingEventType pairing_event_type) {
    bt_private::PairingEvent pairing_event;
    pairing_event.pairing = pairing_event_type;
    pairing_event.device.name.reset(new std::string(kDeviceName));
    pairing_event.device.address = mock_device_->GetAddress();
    pairing_event.device.vendor_id_source = bt::VENDOR_ID_SOURCE_USB;
    pairing_event.device.type = bt::DEVICE_TYPE_PHONE;

    std::unique_ptr<base::ListValue> args =
        bt_private::OnPairing::Create(pairing_event);
    std::unique_ptr<Event> event(new Event(events::BLUETOOTH_PRIVATE_ON_PAIRING,
                                           bt_private::OnPairing::kEventName,
                                           std::move(args)));
    EventRouter::Get(browser()->profile())
        ->DispatchEventToExtension(kTestExtensionId, std::move(event));
  }

  void DispatchAuthorizePairingEvent() {
    DispatchPairingEvent(bt_private::PAIRING_EVENT_TYPE_REQUESTAUTHORIZATION);
  }

  void DispatchPincodePairingEvent() {
    DispatchPairingEvent(bt_private::PAIRING_EVENT_TYPE_REQUESTPINCODE);
  }

  void DispatchPasskeyPairingEvent() {
    DispatchPairingEvent(bt_private::PAIRING_EVENT_TYPE_REQUESTPASSKEY);
  }

  void DispatchConfirmPasskeyPairingEvent() {
    DispatchPairingEvent(bt_private::PAIRING_EVENT_TYPE_CONFIRMPASSKEY);
  }

  void CallSetDiscoveryFilterCallback(
      device::BluetoothAdapter::DiscoverySessionCallback callback) {
    auto session_ptr = std::unique_ptr<NiceMock<MockBluetoothDiscoverySession>>(
        mock_discovery_session_);

    callback.Run(std::move(session_ptr));
  }

 protected:
  std::string adapter_name_;
  bool adapter_powered_;
  bool adapter_discoverable_;

  scoped_refptr<NiceMock<MockBluetoothAdapter> > mock_adapter_;
  std::unique_ptr<NiceMock<MockBluetoothDevice>> mock_device_;

  // This discovery session will be owned by EventRouter, we'll only keep
  // pointer to it.
  NiceMock<MockBluetoothDiscoverySession>* mock_discovery_session_;
};

ACTION_TEMPLATE(InvokeCallbackArgument,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_0_VALUE_PARAMS()) {
  std::get<k>(args).Run();
}

IN_PROC_BROWSER_TEST_F(BluetoothPrivateApiTest, SetAdapterState) {
  ON_CALL(*mock_adapter_, GetName())
      .WillByDefault(ReturnPointee(&adapter_name_));
  ON_CALL(*mock_adapter_, IsPowered())
      .WillByDefault(ReturnPointee(&adapter_powered_));
  ON_CALL(*mock_adapter_, IsDiscoverable())
      .WillByDefault(ReturnPointee(&adapter_discoverable_));

  EXPECT_CALL(*mock_adapter_, SetName("Dome", _, _))
      .WillOnce(
          WithArgs<0, 1>(Invoke(this, &BluetoothPrivateApiTest::SetName)));
  EXPECT_CALL(*mock_adapter_, SetPowered(true, _, _))
      .WillOnce(
          WithArgs<0, 1>(Invoke(this, &BluetoothPrivateApiTest::SetPowered)));
  EXPECT_CALL(*mock_adapter_, SetDiscoverable(true, _, _))
      .WillOnce(WithArgs<0, 1>(
          Invoke(this, &BluetoothPrivateApiTest::SetDiscoverable)));

  ASSERT_TRUE(RunComponentExtensionTest("bluetooth_private/adapter_state"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(BluetoothPrivateApiTest, NoBluetoothAdapter) {
  ON_CALL(*mock_adapter_, IsPresent()).WillByDefault(Return(false));
  ASSERT_TRUE(RunComponentExtensionTest("bluetooth_private/no_adapter"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(BluetoothPrivateApiTest, CancelPairing) {
  InSequence s;
  EXPECT_CALL(*mock_adapter_,
              AddPairingDelegate(
                  _, device::BluetoothAdapter::PAIRING_DELEGATE_PRIORITY_HIGH))
      .WillOnce(WithoutArgs(Invoke(
          this, &BluetoothPrivateApiTest::DispatchAuthorizePairingEvent)));
  EXPECT_CALL(*mock_device_, ExpectingConfirmation())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_device_, CancelPairing());
  ASSERT_TRUE(RunComponentExtensionTest("bluetooth_private/cancel_pairing"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(BluetoothPrivateApiTest, PincodePairing) {
  EXPECT_CALL(*mock_adapter_,
              AddPairingDelegate(
                  _, device::BluetoothAdapter::PAIRING_DELEGATE_PRIORITY_HIGH))
      .WillOnce(WithoutArgs(
          Invoke(this, &BluetoothPrivateApiTest::DispatchPincodePairingEvent)));
  EXPECT_CALL(*mock_device_, ExpectingPinCode()).WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_device_, SetPinCode("abbbbbbk"));
  ASSERT_TRUE(RunComponentExtensionTest("bluetooth_private/pincode_pairing"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(BluetoothPrivateApiTest, PasskeyPairing) {
  EXPECT_CALL(*mock_adapter_,
              AddPairingDelegate(
                  _, device::BluetoothAdapter::PAIRING_DELEGATE_PRIORITY_HIGH))
      .WillOnce(WithoutArgs(
          Invoke(this, &BluetoothPrivateApiTest::DispatchPasskeyPairingEvent)));
  EXPECT_CALL(*mock_device_, ExpectingPasskey()).WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_device_, SetPasskey(900531));
  ASSERT_TRUE(RunComponentExtensionTest("bluetooth_private/passkey_pairing"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(BluetoothPrivateApiTest, DisconnectAll) {
  EXPECT_CALL(*mock_device_, IsConnected())
      .Times(6)
      .WillOnce(Return(false))
      .WillOnce(Return(true))
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_device_, Disconnect(_, _))
      .Times(3)
      .WillOnce(InvokeCallbackArgument<1>())
      .WillOnce(InvokeCallbackArgument<1>())
      .WillOnce(InvokeCallbackArgument<0>());
  ASSERT_TRUE(RunComponentExtensionTest("bluetooth_private/disconnect"))
      << message_;
}

// Device::Forget not implemented on OSX.
#if !defined(OS_MACOSX)
IN_PROC_BROWSER_TEST_F(BluetoothPrivateApiTest, ForgetDevice) {
  EXPECT_CALL(*mock_device_, Forget(_, _))
      .WillOnce(
          WithArgs<0>(Invoke(this, &BluetoothPrivateApiTest::ForgetDevice)));
  ASSERT_TRUE(RunComponentExtensionTest("bluetooth_private/forget_device"))
      << message_;
}
#endif

IN_PROC_BROWSER_TEST_F(BluetoothPrivateApiTest, DiscoveryFilter) {
  mock_discovery_session_ = new NiceMock<MockBluetoothDiscoverySession>();

  BluetoothDiscoveryFilter discovery_filter(device::BLUETOOTH_TRANSPORT_LE);
  discovery_filter.SetPathloss(50);
  discovery_filter.AddUUID(BluetoothUUID("cafe"));
  discovery_filter.AddUUID(
      BluetoothUUID("0000bebe-0000-1000-8000-00805f9b34fb"));

  EXPECT_CALL(*mock_adapter_, StartDiscoverySessionWithFilterRaw(
                                  IsFilterEqual(&discovery_filter), _, _))
      .Times(1)
      .WillOnce(WithArgs<1>(Invoke(
          this, &BluetoothPrivateApiTest::CallSetDiscoveryFilterCallback)));
  EXPECT_CALL(*mock_discovery_session_, IsActive())
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_discovery_session_,
              SetDiscoveryFilterRaw(Eq(nullptr), _, _))
      .Times(1)
      .WillOnce(InvokeCallbackArgument<1>());
  ASSERT_TRUE(RunComponentExtensionTest("bluetooth_private/discovery_filter"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(BluetoothPrivateApiTest, Connect) {
  EXPECT_CALL(*mock_device_, IsConnected())
      .Times(2)
      .WillOnce(Return(false))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_device_, Connect(_, _, _))
      .WillOnce(InvokeCallbackArgument<1>());
  ASSERT_TRUE(RunComponentExtensionTest("bluetooth_private/connect"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(BluetoothPrivateApiTest, Pair) {
  EXPECT_CALL(*mock_adapter_,
              AddPairingDelegate(
                  _, device::BluetoothAdapter::PAIRING_DELEGATE_PRIORITY_HIGH));
  EXPECT_CALL(*mock_device_, ExpectingConfirmation())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_device_, Pair(_, _, _))
      .WillOnce(DoAll(
          WithoutArgs(Invoke(
              this,
              &BluetoothPrivateApiTest::DispatchConfirmPasskeyPairingEvent)),
          InvokeCallbackArgument<1>()));
  ASSERT_TRUE(RunComponentExtensionTest("bluetooth_private/pair")) << message_;
}

}  // namespace extensions
