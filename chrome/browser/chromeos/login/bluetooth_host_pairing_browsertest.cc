// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/login/test/oobe_base_test.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_waiter.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "components/pairing/bluetooth_host_pairing_controller.h"
#include "components/pairing/bluetooth_pairing_constants.h"
#include "components/pairing/shark_connection_listener.h"
#include "content/public/browser/browser_thread.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluez/bluetooth_device_bluez.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/fake_bluetooth_adapter_client.h"
#include "device/bluetooth/dbus/fake_bluetooth_device_client.h"
#include "services/device/public/cpp/hid/fake_input_service_linux.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/device/public/mojom/input_service.mojom.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace chromeos {

namespace {

class TestDelegate
    : public pairing_chromeos::BluetoothHostPairingController::TestDelegate {
 public:
  TestDelegate() {}
  ~TestDelegate() override {}

  // pairing_chromeos::BluetoothHostPairingController::Delegate override:
  void OnAdapterReset() override {
    finished_ = true;
    if (run_loop_)
      run_loop_->Quit();
  }

  void WaitUntilAdapterReset() {
    if (finished_)
      return;
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
  }

 private:
  bool finished_ = false;
  std::unique_ptr<base::RunLoop> run_loop_;

  DISALLOW_COPY_AND_ASSIGN(TestDelegate);
};

}  // namespace

// This is the class to simulate the OOBE process for devices that don't have
// sufficient input, i.e., the first screen of OOBE is the HID detection screen.
// The device will put itself in Bluetooth discoverable mode.
class BluetoothHostPairingNoInputTest : public OobeBaseTest {
 public:
  void OnConnectSuccess(base::OnceClosure continuation_callback) {
    std::move(continuation_callback).Run();
  }
  void OnConnectFailed(base::OnceClosure continuation_callback,
                       device::BluetoothDevice::ConnectErrorCode error) {
    std::move(continuation_callback).Run();
  }

 protected:
  using InputDeviceInfoPtr = device::mojom::InputDeviceInfoPtr;

  BluetoothHostPairingNoInputTest() {
    fake_input_service_manager_ =
        std::make_unique<device::FakeInputServiceLinux>();

    service_manager::ServiceContext::SetGlobalBinderForTesting(
        device::mojom::kServiceName, device::mojom::InputDeviceManager::Name_,
        base::Bind(&device::FakeInputServiceLinux::Bind,
                   base::Unretained(fake_input_service_manager_.get())));

    // Set up the fake Bluetooth environment.
    std::unique_ptr<bluez::BluezDBusManagerSetter> bluez_dbus_setter =
        bluez::BluezDBusManager::GetSetterForTesting();
    bluez_dbus_setter->SetBluetoothAdapterClient(
        std::make_unique<bluez::FakeBluetoothAdapterClient>());
    bluez_dbus_setter->SetBluetoothDeviceClient(
        std::make_unique<bluez::FakeBluetoothDeviceClient>());

    // Get pointer.
    fake_bluetooth_device_client_ =
        static_cast<bluez::FakeBluetoothDeviceClient*>(
            bluez::BluezDBusManager::Get()->GetBluetoothDeviceClient());
  }
  ~BluetoothHostPairingNoInputTest() override {}

  // OobeBaseTest override:
  void SetUpOnMainThread() override {
    OobeBaseTest::SetUpOnMainThread();
    delegate_.reset(new TestDelegate);
    pairing_chromeos::SharkConnectionListener* shark_listener =
        WizardController::default_controller()
            ->GetSharkConnectionListenerForTesting();
    controller_ =
        shark_listener ? shark_listener->GetControllerForTesting() : nullptr;
    if (controller_) {
      controller_->SetDelegateForTesting(delegate_.get());
      bluetooth_adapter_ = controller_->GetAdapterForTesting();
      controller_->SetControllerDeviceAddressForTesting(
          bluez::FakeBluetoothDeviceClient::kConfirmPasskeyAddress);
    }
  }

  pairing_chromeos::BluetoothHostPairingController* controller() {
    return controller_;
  }

  device::BluetoothAdapter* bluetooth_adapter() {
    return bluetooth_adapter_.get();
  }

  TestDelegate* delegate() { return delegate_.get(); }

  bluez::FakeBluetoothDeviceClient* fake_bluetooth_device_client() {
    return fake_bluetooth_device_client_;
  }

  void ResetController() {
    if (controller_)
      controller_->Reset();
  }

  void AddUsbMouse() {
    auto mouse = device::mojom::InputDeviceInfo::New();
    mouse->id = "usb_mouse";
    mouse->subsystem = device::mojom::InputDeviceSubsystem::SUBSYSTEM_INPUT;
    mouse->type = device::mojom::InputDeviceType::TYPE_USB;
    mouse->is_mouse = true;
    fake_input_service_manager_->AddDevice(std::move(mouse));
  }

  void AddUsbKeyboard() {
    auto keyboard = device::mojom::InputDeviceInfo::New();
    keyboard->id = "usb_keyboard";
    keyboard->subsystem = device::mojom::InputDeviceSubsystem::SUBSYSTEM_INPUT;
    keyboard->type = device::mojom::InputDeviceType::TYPE_USB;
    keyboard->is_keyboard = true;
    fake_input_service_manager_->AddDevice(std::move(keyboard));
  }

  void AddBluetoothMouse() {
    auto mouse = device::mojom::InputDeviceInfo::New();
    mouse->id = "bluetooth_mouse";
    mouse->subsystem = device::mojom::InputDeviceSubsystem::SUBSYSTEM_INPUT;
    mouse->type = device::mojom::InputDeviceType::TYPE_BLUETOOTH;
    mouse->is_mouse = true;
    fake_input_service_manager_->AddDevice(std::move(mouse));
  }

 private:
  std::unique_ptr<device::FakeInputServiceLinux> fake_input_service_manager_;
  scoped_refptr<device::BluetoothAdapter> bluetooth_adapter_;
  std::unique_ptr<TestDelegate> delegate_;
  pairing_chromeos::BluetoothHostPairingController* controller_ = nullptr;

  bluez::FakeBluetoothDeviceClient* fake_bluetooth_device_client_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BluetoothHostPairingNoInputTest);
};

// Test that in normal user OOBE login flow for devices lacking input devices,
// if there is no Bluetooth device connected, the Bluetooth adapter should be
// disabled when OOBE reaches login screen (which means OOBE has been completed)
IN_PROC_BROWSER_TEST_F(BluetoothHostPairingNoInputTest,
                       NoBluetoothDeviceConnected) {
  OobeScreenWaiter(OobeScreen::SCREEN_OOBE_HID_DETECTION).Wait();
  EXPECT_EQ(bluetooth_adapter()->IsPowered(), true);
  WizardController::default_controller()->SkipToLoginForTesting(
      LoginScreenContext());
  OobeScreenWaiter(OobeScreen::SCREEN_GAIA_SIGNIN).Wait();
  delegate()->WaitUntilAdapterReset();
  EXPECT_EQ(bluetooth_adapter()->IsPowered(), false);
}

// Test that in normal user OOBE login flow for devices lacking input devices,
// if there is any Bluetooth device connected, the Bluetooth adapter should not
// be disabled after OOBE completes.
IN_PROC_BROWSER_TEST_F(BluetoothHostPairingNoInputTest,
                       BluetoothDeviceConnected) {
  OobeScreenWaiter(OobeScreen::SCREEN_OOBE_HID_DETECTION).Wait();
  AddBluetoothMouse();
  EXPECT_EQ(bluetooth_adapter()->IsPowered(), true);
  WizardController::default_controller()->SkipToLoginForTesting(
      LoginScreenContext());
  OobeScreenWaiter(OobeScreen::SCREEN_GAIA_SIGNIN).Wait();
  delegate()->WaitUntilAdapterReset();
  EXPECT_EQ(bluetooth_adapter()->IsPowered(), true);
}

// Test that the paired Master Bluetooth device is disconnected after the
// enrollment is done or failed.
IN_PROC_BROWSER_TEST_F(BluetoothHostPairingNoInputTest, ForgetDevice) {
  OobeScreenWaiter(OobeScreen::SCREEN_OOBE_HID_DETECTION).Wait();
  EXPECT_TRUE(bluetooth_adapter()->IsDiscoverable());
  EXPECT_TRUE(bluetooth_adapter()->IsPowered());

  fake_bluetooth_device_client()->CreateDevice(
      dbus::ObjectPath(bluez::FakeBluetoothAdapterClient::kAdapterPath),
      dbus::ObjectPath(bluez::FakeBluetoothDeviceClient::kConfirmPasskeyPath));

  device::BluetoothDevice* device = bluetooth_adapter()->GetDevice(
      bluez::FakeBluetoothDeviceClient::kConfirmPasskeyAddress);
  ASSERT_TRUE(device);
  EXPECT_FALSE(device->IsPaired());
  EXPECT_EQ(3U, bluetooth_adapter()->GetDevices().size());

  base::RunLoop run_loop;
  device->Connect(
      controller(),
      base::Bind(&BluetoothHostPairingNoInputTest::OnConnectSuccess,
                 base::Unretained(this), run_loop.QuitWhenIdleClosure()),
      base::Bind(&BluetoothHostPairingNoInputTest::OnConnectFailed,
                 base::Unretained(this), run_loop.QuitWhenIdleClosure()));
  run_loop.Run();
  EXPECT_TRUE(device->IsPaired());

  ResetController();
  delegate()->WaitUntilAdapterReset();

  // The device should have been removed now.
  EXPECT_TRUE(!bluetooth_adapter()->GetDevice(
      bluez::FakeBluetoothDeviceClient::kConfirmPasskeyAddress));
  EXPECT_EQ(2U, bluetooth_adapter()->GetDevices().size());
  EXPECT_FALSE(bluetooth_adapter()->IsDiscoverable());
  EXPECT_FALSE(bluetooth_adapter()->IsPowered());
}

// This is the class to simulate the OOBE process for devices that have
// sufficient input, i.e., the first screen of OOBE is the network screen.
// The device will not put itself in Bluetooth discoverable mode until the user
// manually trigger it using the proper accelerator.
class BluetoothHostPairingWithInputTest
    : public BluetoothHostPairingNoInputTest {
 public:
  BluetoothHostPairingWithInputTest() {
    AddUsbMouse();
    AddUsbKeyboard();
  }
  ~BluetoothHostPairingWithInputTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothHostPairingWithInputTest);
};

// Test that in normal user OOBE login flow for devices that have input devices,
// the Bluetooth is disabled by default.
IN_PROC_BROWSER_TEST_F(BluetoothHostPairingWithInputTest,
                       BluetoothDisableByDefault) {
  OobeScreenWaiter(OobeScreen::SCREEN_OOBE_NETWORK).Wait();
  EXPECT_FALSE(controller());
  EXPECT_FALSE(bluetooth_adapter());
}

}  // namespace chromeos
