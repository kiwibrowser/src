// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/test_mock_time_task_runner.h"
#include "services/ui/input_devices/input_device_server.h"
#include "services/ui/public/cpp/input_devices/input_device_client.h"
#include "services/ui/public/interfaces/input_devices/input_device_server.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/devices/device_data_manager.h"
#include "ui/events/devices/device_hotplug_event_observer.h"

namespace ui {

namespace {

// Helper to place items into a std::vector<T> to provide as input.
template <class T>
std::vector<T> AsVector(std::initializer_list<T> input) {
  return std::vector<T>(input);
}

// Test client that doesn't register itself as the InputDeviceManager.
class TestInputDeviceClient : public InputDeviceClient {
 public:
  TestInputDeviceClient() : InputDeviceClient(false) {}
  ~TestInputDeviceClient() override {}

  using InputDeviceClient::GetIntefacePtr;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestInputDeviceClient);
};

class InputDeviceTest : public testing::Test {
 public:
  InputDeviceTest() {}
  ~InputDeviceTest() override {}

  void RunUntilIdle() { task_runner_->RunUntilIdle(); }

  void AddClientAsObserver(TestInputDeviceClient* client) {
    server_.AddObserver(client->GetIntefacePtr());
  }

  DeviceHotplugEventObserver* GetHotplugObserver() {
    return DeviceDataManager::GetInstance();
  }

  // testing::Test:
  void SetUp() override {
    task_runner_ = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
        base::Time::Now(), base::TimeTicks::Now());
    message_loop_.SetTaskRunner(task_runner_);

    DeviceDataManager::CreateInstance();
    server_.RegisterAsObserver();
  }

  void TearDown() override { DeviceDataManager::DeleteInstance(); }

 private:
  base::MessageLoop message_loop_;
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  InputDeviceServer server_;

  DISALLOW_COPY_AND_ASSIGN(InputDeviceTest);
};

}  // namespace

TEST_F(InputDeviceTest, DeviceListsComplete) {
  TestInputDeviceClient client;
  AddClientAsObserver(&client);

  RunUntilIdle();
  EXPECT_FALSE(client.AreDeviceListsComplete());

  GetHotplugObserver()->OnDeviceListsComplete();

  // Observer should get notification for device lists complete now.
  RunUntilIdle();
  EXPECT_TRUE(client.AreDeviceListsComplete());
}

TEST_F(InputDeviceTest, DeviceListsCompleteTwoClients) {
  TestInputDeviceClient client1;
  AddClientAsObserver(&client1);

  TestInputDeviceClient client2;
  AddClientAsObserver(&client2);

  RunUntilIdle();
  EXPECT_FALSE(client1.AreDeviceListsComplete());
  EXPECT_FALSE(client2.AreDeviceListsComplete());

  GetHotplugObserver()->OnDeviceListsComplete();

  // Both observers should get notifications for device lists complete now.
  RunUntilIdle();
  EXPECT_TRUE(client1.AreDeviceListsComplete());
  EXPECT_TRUE(client2.AreDeviceListsComplete());
}

TEST_F(InputDeviceTest, AddDevices) {
  const TouchscreenDevice touchscreen(100, INPUT_DEVICE_INTERNAL, "Touchscreen",
                                      gfx::Size(2600, 1700), 3);

  TestInputDeviceClient client;
  AddClientAsObserver(&client);

  // Add keyboard and mark device lists complete.
  GetHotplugObserver()->OnTouchscreenDevicesUpdated(AsVector({touchscreen}));
  GetHotplugObserver()->OnDeviceListsComplete();

  RunUntilIdle();
  EXPECT_TRUE(client.AreDeviceListsComplete());
  EXPECT_EQ(1u, client.GetTouchscreenDevices().size());
  EXPECT_EQ(0u, client.GetKeyboardDevices().size());
  EXPECT_EQ(0u, client.GetMouseDevices().size());
  EXPECT_EQ(0u, client.GetTouchpadDevices().size());
}

TEST_F(InputDeviceTest, AddDeviceAfterComplete) {
  const InputDevice keyboard1(100, INPUT_DEVICE_INTERNAL, "Keyboard1");
  const InputDevice keyboard2(200, INPUT_DEVICE_EXTERNAL, "Keyboard2");
  const InputDevice mouse(300, INPUT_DEVICE_EXTERNAL, "Mouse");

  TestInputDeviceClient client;
  AddClientAsObserver(&client);

  // Add mouse and mark device lists complete.
  GetHotplugObserver()->OnKeyboardDevicesUpdated(AsVector({keyboard1}));
  GetHotplugObserver()->OnDeviceListsComplete();

  RunUntilIdle();
  EXPECT_TRUE(client.AreDeviceListsComplete());
  EXPECT_EQ(1lu, client.GetKeyboardDevices().size());

  // Add a second keyboard and a mouse.
  GetHotplugObserver()->OnMouseDevicesUpdated(AsVector({mouse}));
  GetHotplugObserver()->OnKeyboardDevicesUpdated(
      AsVector({keyboard1, keyboard2}));

  RunUntilIdle();
  EXPECT_EQ(0u, client.GetTouchscreenDevices().size());
  EXPECT_EQ(2u, client.GetKeyboardDevices().size());
  EXPECT_EQ(1u, client.GetMouseDevices().size());
  EXPECT_EQ(0u, client.GetTouchpadDevices().size());
}

TEST_F(InputDeviceTest, AddThenRemoveDevice) {
  const InputDevice mouse(100, INPUT_DEVICE_INTERNAL, "Mouse");

  TestInputDeviceClient client;
  AddClientAsObserver(&client);

  // Add mouse and mark device lists complete.
  GetHotplugObserver()->OnMouseDevicesUpdated(AsVector({mouse}));
  GetHotplugObserver()->OnDeviceListsComplete();

  RunUntilIdle();
  EXPECT_TRUE(client.AreDeviceListsComplete());
  EXPECT_EQ(1u, client.GetMouseDevices().size());

  // Remove mouse device.
  GetHotplugObserver()->OnMouseDevicesUpdated(AsVector<InputDevice>({}));

  RunUntilIdle();
  EXPECT_EQ(0u, client.GetMouseDevices().size());
}

TEST_F(InputDeviceTest, CheckClientDeviceFields) {
  const InputDevice touchpad(100, INPUT_DEVICE_INTERNAL, "Touchpad");

  TestInputDeviceClient client;
  AddClientAsObserver(&client);

  // Add touchpad and mark device lists complete.
  GetHotplugObserver()->OnTouchpadDevicesUpdated(AsVector({touchpad}));
  GetHotplugObserver()->OnDeviceListsComplete();

  RunUntilIdle();
  EXPECT_TRUE(client.AreDeviceListsComplete());
  EXPECT_EQ(1u, client.GetTouchpadDevices().size());

  // Check the touchpad fields match.
  const InputDevice& output = client.GetTouchpadDevices()[0];
  EXPECT_EQ(touchpad.id, output.id);
  EXPECT_EQ(touchpad.type, output.type);
  EXPECT_EQ(touchpad.name, output.name);
}

}  // namespace ui
