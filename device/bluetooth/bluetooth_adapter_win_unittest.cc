// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/test_simple_task_runner.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_adapter_win.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_session_outcome.h"
#include "device/bluetooth/bluetooth_task_manager_win.h"
#include "device/bluetooth/test/test_bluetooth_adapter_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kAdapterAddress[] = "A1:B2:C3:D4:E5:F6";
const char kAdapterName[] = "Bluetooth Adapter Name";

const char kTestAudioSdpName[] = "Audio";
const char kTestAudioSdpName2[] = "Audio2";
const char kTestAudioSdpBytes[] =
    "35510900000a00010001090001350319110a09000435103506190100090019350619001909"
    "010209000535031910020900093508350619110d090102090100250c417564696f20536f75"
    "726365090311090001";
const device::BluetoothUUID kTestAudioSdpUuid("110a");

void MakeDeviceState(const std::string& name,
                     const std::string& address,
                     device::BluetoothTaskManagerWin::DeviceState* state) {
  state->name = name;
  state->address = address;
  state->bluetooth_class = 0;
  state->authenticated = false;
  state->connected = false;
}

}  // namespace

namespace device {

class BluetoothAdapterWinTest : public testing::Test {
 public:
  BluetoothAdapterWinTest()
      : ui_task_runner_(new base::TestSimpleTaskRunner()),
        bluetooth_task_runner_(new base::TestSimpleTaskRunner()),
        adapter_(new BluetoothAdapterWin(
            base::Bind(&BluetoothAdapterWinTest::RunInitCallback,
                       base::Unretained(this)))),
        adapter_win_(static_cast<BluetoothAdapterWin*>(adapter_.get())),
        observer_(adapter_),
        init_callback_called_(false) {
    adapter_win_->InitForTest(ui_task_runner_, bluetooth_task_runner_);
  }

  void SetUp() override {
    num_start_discovery_callbacks_ = 0;
    num_start_discovery_error_callbacks_ = 0;
    num_stop_discovery_callbacks_ = 0;
    num_stop_discovery_error_callbacks_ = 0;
  }

  void RunInitCallback() {
    init_callback_called_ = true;
  }

  void IncrementNumStartDiscoveryCallbacks() {
    num_start_discovery_callbacks_++;
  }

  void IncrementNumStartDiscoveryErrorCallbacks(
      UMABluetoothDiscoverySessionOutcome) {
    num_start_discovery_error_callbacks_++;
  }

  void IncrementNumStopDiscoveryCallbacks() {
    num_stop_discovery_callbacks_++;
  }

  void IncrementNumStopDiscoveryErrorCallbacks(
      UMABluetoothDiscoverySessionOutcome) {
    num_stop_discovery_error_callbacks_++;
  }

  typedef base::OnceCallback<void(UMABluetoothDiscoverySessionOutcome)>
      DiscoverySessionErrorCallback;

  void CallAddDiscoverySession(const base::Closure& callback,
                               DiscoverySessionErrorCallback error_callback) {
    adapter_win_->AddDiscoverySession(nullptr, callback,
                                      std::move(error_callback));
  }

  void CallRemoveDiscoverySession(
      const base::Closure& callback,
      DiscoverySessionErrorCallback error_callback) {
    adapter_win_->RemoveDiscoverySession(nullptr, callback,
                                         std::move(error_callback));
  }

 protected:
  scoped_refptr<base::TestSimpleTaskRunner> ui_task_runner_;
  scoped_refptr<base::TestSimpleTaskRunner> bluetooth_task_runner_;
  scoped_refptr<BluetoothAdapter> adapter_;
  BluetoothAdapterWin* adapter_win_;
  TestBluetoothAdapterObserver observer_;
  bool init_callback_called_;
  int num_start_discovery_callbacks_;
  int num_start_discovery_error_callbacks_;
  int num_stop_discovery_callbacks_;
  int num_stop_discovery_error_callbacks_;
};

TEST_F(BluetoothAdapterWinTest, AdapterNotPresent) {
  BluetoothTaskManagerWin::AdapterState state;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_FALSE(adapter_win_->IsPresent());
}

TEST_F(BluetoothAdapterWinTest, AdapterPresent) {
  BluetoothTaskManagerWin::AdapterState state;
  state.address = kAdapterAddress;
  state.name = kAdapterName;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_TRUE(adapter_win_->IsPresent());
}

TEST_F(BluetoothAdapterWinTest, AdapterPresentChanged) {
  BluetoothTaskManagerWin::AdapterState state;
  state.address = kAdapterAddress;
  state.name = kAdapterName;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_EQ(1, observer_.present_changed_count());
  adapter_win_->AdapterStateChanged(state);
  EXPECT_EQ(1, observer_.present_changed_count());
  BluetoothTaskManagerWin::AdapterState empty_state;
  adapter_win_->AdapterStateChanged(empty_state);
  EXPECT_EQ(2, observer_.present_changed_count());
}

TEST_F(BluetoothAdapterWinTest, AdapterPoweredChanged) {
  BluetoothTaskManagerWin::AdapterState state;
  state.powered = true;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_EQ(1, observer_.powered_changed_count());
  adapter_win_->AdapterStateChanged(state);
  EXPECT_EQ(1, observer_.powered_changed_count());
  state.powered = false;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_EQ(2, observer_.powered_changed_count());
}

TEST_F(BluetoothAdapterWinTest, AdapterInitialized) {
  EXPECT_FALSE(adapter_win_->IsInitialized());
  EXPECT_FALSE(init_callback_called_);
  BluetoothTaskManagerWin::AdapterState state;
  adapter_win_->AdapterStateChanged(state);
  EXPECT_TRUE(adapter_win_->IsInitialized());
  EXPECT_TRUE(init_callback_called_);
}

TEST_F(BluetoothAdapterWinTest, SingleStartDiscovery) {
  bluetooth_task_runner_->ClearPendingTasks();
  CallAddDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
                 base::Unretained(this)),
      DiscoverySessionErrorCallback());
  EXPECT_FALSE(ui_task_runner_->HasPendingTask());
  EXPECT_EQ(1u, bluetooth_task_runner_->NumPendingTasks());
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(0, num_start_discovery_callbacks_);
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->RunPendingTasks();
  EXPECT_TRUE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_start_discovery_callbacks_);
  EXPECT_EQ(1, observer_.discovering_changed_count());
}

TEST_F(BluetoothAdapterWinTest, SingleStartDiscoveryFailure) {
  CallAddDiscoverySession(
      base::Closure(),
      base::Bind(
          &BluetoothAdapterWinTest::IncrementNumStartDiscoveryErrorCallbacks,
          base::Unretained(this)));
  adapter_win_->DiscoveryStarted(false);
  ui_task_runner_->RunPendingTasks();
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_start_discovery_error_callbacks_);
  EXPECT_EQ(0, observer_.discovering_changed_count());
}

TEST_F(BluetoothAdapterWinTest, MultipleStartDiscoveries) {
  bluetooth_task_runner_->ClearPendingTasks();
  int num_discoveries = 5;
  for (int i = 0; i < num_discoveries; i++) {
    CallAddDiscoverySession(
        base::Bind(
            &BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
            base::Unretained(this)),
        DiscoverySessionErrorCallback());
    EXPECT_EQ(1u, bluetooth_task_runner_->NumPendingTasks());
  }
  EXPECT_FALSE(ui_task_runner_->HasPendingTask());
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(0, num_start_discovery_callbacks_);
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->RunPendingTasks();
  EXPECT_TRUE(adapter_->IsDiscovering());
  EXPECT_EQ(num_discoveries, num_start_discovery_callbacks_);
  EXPECT_EQ(1, observer_.discovering_changed_count());
}

TEST_F(BluetoothAdapterWinTest, MultipleStartDiscoveriesFailure) {
  int num_discoveries = 5;
  for (int i = 0; i < num_discoveries; i++) {
    CallAddDiscoverySession(
        base::Closure(),
        base::Bind(
            &BluetoothAdapterWinTest::IncrementNumStartDiscoveryErrorCallbacks,
            base::Unretained(this)));
  }
  adapter_win_->DiscoveryStarted(false);
  ui_task_runner_->RunPendingTasks();
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(num_discoveries, num_start_discovery_error_callbacks_);
  EXPECT_EQ(0, observer_.discovering_changed_count());
}

TEST_F(BluetoothAdapterWinTest, MultipleStartDiscoveriesAfterDiscovering) {
  CallAddDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
                 base::Unretained(this)),
      DiscoverySessionErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->RunPendingTasks();
  EXPECT_TRUE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_start_discovery_callbacks_);

  bluetooth_task_runner_->ClearPendingTasks();
  for (int i = 0; i < 5; i++) {
    int num_start_discovery_callbacks = num_start_discovery_callbacks_;
    CallAddDiscoverySession(
        base::Bind(
            &BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
            base::Unretained(this)),
        DiscoverySessionErrorCallback());
    EXPECT_TRUE(adapter_->IsDiscovering());
    EXPECT_FALSE(bluetooth_task_runner_->HasPendingTask());
    EXPECT_FALSE(ui_task_runner_->HasPendingTask());
    EXPECT_EQ(num_start_discovery_callbacks + 1,
              num_start_discovery_callbacks_);
  }
  EXPECT_EQ(1, observer_.discovering_changed_count());
}

TEST_F(BluetoothAdapterWinTest, StartDiscoveryAfterDiscoveringFailure) {
  CallAddDiscoverySession(
      base::Closure(),
      base::Bind(
          &BluetoothAdapterWinTest::IncrementNumStartDiscoveryErrorCallbacks,
          base::Unretained(this)));
  adapter_win_->DiscoveryStarted(false);
  ui_task_runner_->RunPendingTasks();
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_start_discovery_error_callbacks_);

  CallAddDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
                 base::Unretained(this)),
      DiscoverySessionErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->RunPendingTasks();
  EXPECT_TRUE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_start_discovery_callbacks_);
}

TEST_F(BluetoothAdapterWinTest, SingleStopDiscovery) {
  CallAddDiscoverySession(base::Closure(), DiscoverySessionErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->ClearPendingTasks();
  CallRemoveDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                 base::Unretained(this)),
      DiscoverySessionErrorCallback());
  EXPECT_TRUE(adapter_->IsDiscovering());
  EXPECT_EQ(0, num_stop_discovery_callbacks_);
  bluetooth_task_runner_->ClearPendingTasks();
  adapter_win_->DiscoveryStopped();
  ui_task_runner_->RunPendingTasks();
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(1, num_stop_discovery_callbacks_);
  EXPECT_FALSE(bluetooth_task_runner_->HasPendingTask());
  EXPECT_EQ(2, observer_.discovering_changed_count());
}

TEST_F(BluetoothAdapterWinTest, MultipleStopDiscoveries) {
  int num_discoveries = 5;
  for (int i = 0; i < num_discoveries; i++) {
    CallAddDiscoverySession(base::Closure(), DiscoverySessionErrorCallback());
  }
  adapter_win_->DiscoveryStarted(true);
  ui_task_runner_->ClearPendingTasks();
  bluetooth_task_runner_->ClearPendingTasks();
  for (int i = 0; i < num_discoveries - 1; i++) {
    CallRemoveDiscoverySession(
        base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                   base::Unretained(this)),
        DiscoverySessionErrorCallback());
    EXPECT_FALSE(bluetooth_task_runner_->HasPendingTask());
    ui_task_runner_->RunPendingTasks();
    EXPECT_EQ(i + 1, num_stop_discovery_callbacks_);
  }
  CallRemoveDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                 base::Unretained(this)),
      DiscoverySessionErrorCallback());
  EXPECT_EQ(1u, bluetooth_task_runner_->NumPendingTasks());
  EXPECT_TRUE(adapter_->IsDiscovering());
  adapter_win_->DiscoveryStopped();
  ui_task_runner_->RunPendingTasks();
  EXPECT_FALSE(adapter_->IsDiscovering());
  EXPECT_EQ(num_discoveries, num_stop_discovery_callbacks_);
  EXPECT_EQ(2, observer_.discovering_changed_count());
}

TEST_F(BluetoothAdapterWinTest,
       StartDiscoveryAndStartDiscoveryAndStopDiscoveries) {
  CallAddDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
                 base::Unretained(this)),
      DiscoverySessionErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  CallAddDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
                 base::Unretained(this)),
      DiscoverySessionErrorCallback());
  ui_task_runner_->ClearPendingTasks();
  bluetooth_task_runner_->ClearPendingTasks();
  CallRemoveDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                 base::Unretained(this)),
      DiscoverySessionErrorCallback());
  EXPECT_FALSE(bluetooth_task_runner_->HasPendingTask());
  CallRemoveDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                 base::Unretained(this)),
      DiscoverySessionErrorCallback());
  EXPECT_EQ(1u, bluetooth_task_runner_->NumPendingTasks());
}

TEST_F(BluetoothAdapterWinTest,
       StartDiscoveryAndStopDiscoveryAndStartDiscovery) {
  CallAddDiscoverySession(base::Closure(), DiscoverySessionErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  EXPECT_TRUE(adapter_->IsDiscovering());
  CallRemoveDiscoverySession(base::Closure(), DiscoverySessionErrorCallback());
  adapter_win_->DiscoveryStopped();
  EXPECT_FALSE(adapter_->IsDiscovering());
  CallAddDiscoverySession(base::Closure(), DiscoverySessionErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  EXPECT_TRUE(adapter_->IsDiscovering());
}

TEST_F(BluetoothAdapterWinTest, StartDiscoveryBeforeDiscoveryStopped) {
  CallAddDiscoverySession(base::Closure(), DiscoverySessionErrorCallback());
  adapter_win_->DiscoveryStarted(true);
  CallRemoveDiscoverySession(base::Closure(), DiscoverySessionErrorCallback());
  CallAddDiscoverySession(base::Closure(), DiscoverySessionErrorCallback());
  bluetooth_task_runner_->ClearPendingTasks();
  adapter_win_->DiscoveryStopped();
  EXPECT_EQ(1u, bluetooth_task_runner_->NumPendingTasks());
}

TEST_F(BluetoothAdapterWinTest, StopDiscoveryWithoutStartDiscovery) {
  CallRemoveDiscoverySession(
      base::Closure(),
      base::Bind(
          &BluetoothAdapterWinTest::IncrementNumStopDiscoveryErrorCallbacks,
          base::Unretained(this)));
  EXPECT_EQ(1, num_stop_discovery_error_callbacks_);
}

TEST_F(BluetoothAdapterWinTest, StopDiscoveryBeforeDiscoveryStarted) {
  CallAddDiscoverySession(base::Closure(), DiscoverySessionErrorCallback());
  CallRemoveDiscoverySession(base::Closure(), DiscoverySessionErrorCallback());
  bluetooth_task_runner_->ClearPendingTasks();
  adapter_win_->DiscoveryStarted(true);
  EXPECT_EQ(1u, bluetooth_task_runner_->NumPendingTasks());
}

TEST_F(BluetoothAdapterWinTest, StartAndStopBeforeDiscoveryStarted) {
  int num_expected_start_discoveries = 3;
  int num_expected_stop_discoveries = 2;
  for (int i = 0; i < num_expected_start_discoveries; i++) {
    CallAddDiscoverySession(
        base::Bind(
            &BluetoothAdapterWinTest::IncrementNumStartDiscoveryCallbacks,
            base::Unretained(this)),
        DiscoverySessionErrorCallback());
  }
  for (int i = 0; i < num_expected_stop_discoveries; i++) {
    CallRemoveDiscoverySession(
        base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                   base::Unretained(this)),
        DiscoverySessionErrorCallback());
  }
  bluetooth_task_runner_->ClearPendingTasks();
  adapter_win_->DiscoveryStarted(true);
  EXPECT_FALSE(bluetooth_task_runner_->HasPendingTask());
  ui_task_runner_->RunPendingTasks();
  EXPECT_EQ(num_expected_start_discoveries, num_start_discovery_callbacks_);
  EXPECT_EQ(num_expected_stop_discoveries, num_stop_discovery_callbacks_);
}

TEST_F(BluetoothAdapterWinTest, StopDiscoveryBeforeDiscoveryStartedAndFailed) {
  CallAddDiscoverySession(
      base::Closure(),
      base::Bind(
          &BluetoothAdapterWinTest::IncrementNumStartDiscoveryErrorCallbacks,
          base::Unretained(this)));
  CallRemoveDiscoverySession(
      base::Bind(&BluetoothAdapterWinTest::IncrementNumStopDiscoveryCallbacks,
                 base::Unretained(this)),
      DiscoverySessionErrorCallback());
  ui_task_runner_->ClearPendingTasks();
  adapter_win_->DiscoveryStarted(false);
  ui_task_runner_->RunPendingTasks();
  EXPECT_EQ(1, num_start_discovery_error_callbacks_);
  EXPECT_EQ(1, num_stop_discovery_callbacks_);
  EXPECT_EQ(0, observer_.discovering_changed_count());
}

TEST_F(BluetoothAdapterWinTest, DevicesPolled) {
  std::vector<std::unique_ptr<BluetoothTaskManagerWin::DeviceState>> devices;
  devices.push_back(std::make_unique<BluetoothTaskManagerWin::DeviceState>());
  devices.push_back(std::make_unique<BluetoothTaskManagerWin::DeviceState>());
  devices.push_back(std::make_unique<BluetoothTaskManagerWin::DeviceState>());

  BluetoothTaskManagerWin::DeviceState* android_phone_state = devices[0].get();
  BluetoothTaskManagerWin::DeviceState* laptop_state = devices[1].get();
  BluetoothTaskManagerWin::DeviceState* iphone_state = devices[2].get();
  MakeDeviceState("phone", "A1:B2:C3:D4:E5:E0", android_phone_state);
  MakeDeviceState("laptop", "A1:B2:C3:D4:E5:E1", laptop_state);
  MakeDeviceState("phone", "A1:B2:C3:D4:E5:E2", iphone_state);

  // Add 3 devices
  observer_.Reset();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(3, observer_.device_added_count());
  EXPECT_EQ(0, observer_.device_removed_count());
  EXPECT_EQ(0, observer_.device_changed_count());

  // Change a device name
  android_phone_state->name = std::string("phone2");
  observer_.Reset();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(0, observer_.device_added_count());
  EXPECT_EQ(0, observer_.device_removed_count());
  EXPECT_EQ(1, observer_.device_changed_count());

  // Change a device address
  android_phone_state->address = "A1:B2:C3:D4:E5:E6";
  observer_.Reset();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(1, observer_.device_added_count());
  EXPECT_EQ(1, observer_.device_removed_count());
  EXPECT_EQ(0, observer_.device_changed_count());

  // Remove a device
  devices.erase(devices.begin());
  observer_.Reset();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(0, observer_.device_added_count());
  EXPECT_EQ(1, observer_.device_removed_count());
  EXPECT_EQ(0, observer_.device_changed_count());

  // Add a service
  laptop_state->service_record_states.push_back(
      std::make_unique<BluetoothTaskManagerWin::ServiceRecordState>());
  BluetoothTaskManagerWin::ServiceRecordState* audio_state =
      laptop_state->service_record_states.back().get();
  audio_state->name = kTestAudioSdpName;
  base::HexStringToBytes(kTestAudioSdpBytes, &audio_state->sdp_bytes);
  observer_.Reset();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(0, observer_.device_added_count());
  EXPECT_EQ(0, observer_.device_removed_count());
  EXPECT_EQ(1, observer_.device_changed_count());

  // Change a service
  audio_state->name = kTestAudioSdpName2;
  observer_.Reset();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(0, observer_.device_added_count());
  EXPECT_EQ(0, observer_.device_removed_count());
  EXPECT_EQ(1, observer_.device_changed_count());

  // Remove a service
  laptop_state->service_record_states.clear();
  observer_.Reset();
  adapter_win_->DevicesPolled(devices);
  EXPECT_EQ(0, observer_.device_added_count());
  EXPECT_EQ(0, observer_.device_removed_count());
  EXPECT_EQ(1, observer_.device_changed_count());
}

}  // namespace device
