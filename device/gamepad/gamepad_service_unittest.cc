// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/gamepad_service.h"

#include <string.h>

#include <memory>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "device/gamepad/gamepad_consumer.h"
#include "device/gamepad/gamepad_test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {
static const int kNumberOfGamepads = Gamepads::kItemsLengthCap;
}

class ConnectionListener : public device::GamepadConsumer {
 public:
  ConnectionListener() { ClearCounters(); }

  void OnGamepadConnected(unsigned index, const Gamepad& gamepad) override {
    connected_counter_++;
  }
  void OnGamepadDisconnected(unsigned index, const Gamepad& gamepad) override {
    disconnected_counter_++;
  }

  void ClearCounters() {
    connected_counter_ = 0;
    disconnected_counter_ = 0;
  }

  int connected_counter() const { return connected_counter_; }
  int disconnected_counter() const { return disconnected_counter_; }

 private:
  int connected_counter_;
  int disconnected_counter_;
};

class GamepadServiceTest : public testing::Test {
 protected:
  GamepadServiceTest();
  ~GamepadServiceTest() override;

  void InitializeSecondConsumer();
  void SetSecondConsumerActive(bool active);
  void SetPadsConnected(bool connected);
  void SimulateUserGesture(bool has_gesture);
  void SimulatePageReload();
  void ClearCounters();
  void WaitForData();

  int GetConnectedCounter() const {
    return connection_listener_->connected_counter();
  }
  int GetDisconnectedCounter() const {
    return connection_listener_->disconnected_counter();
  }

  int GetConnectedCounter2() const {
    if (!connection_listener2_)
      return 0;
    return connection_listener2_->connected_counter();
  }
  int GetDisconnectedCounter2() const {
    if (!connection_listener2_)
      return 0;
    return connection_listener2_->disconnected_counter();
  }

  void SetUp() override;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  device::MockGamepadDataFetcher* fetcher_;
  GamepadService* service_;
  std::unique_ptr<ConnectionListener> connection_listener_;
  std::unique_ptr<ConnectionListener> connection_listener2_;
  Gamepads test_data_;

  DISALLOW_COPY_AND_ASSIGN(GamepadServiceTest);
};

GamepadServiceTest::GamepadServiceTest() {
  memset(&test_data_, 0, sizeof(test_data_));

  // Configure the pad to have one button. We need our mock gamepad
  // to have at least one input so we can simulate a user gesture.
  test_data_.items[0].buttons_length = 1;
}

GamepadServiceTest::~GamepadServiceTest() {
  delete service_;
}

void GamepadServiceTest::SetUp() {
  fetcher_ = new device::MockGamepadDataFetcher(test_data_);
  service_ =
      new GamepadService(std::unique_ptr<device::GamepadDataFetcher>(fetcher_));
  connection_listener_.reset((new ConnectionListener));
  service_->SetSanitizationEnabled(false);
  service_->ConsumerBecameActive(connection_listener_.get());
}

void GamepadServiceTest::InitializeSecondConsumer() {
  connection_listener2_.reset((new ConnectionListener));
  service_->ConsumerBecameActive(connection_listener2_.get());
}

void GamepadServiceTest::SetSecondConsumerActive(bool active) {
  if (active) {
    service_->ConsumerBecameActive(connection_listener2_.get());
  } else {
    service_->ConsumerBecameInactive(connection_listener2_.get());
  }
}

void GamepadServiceTest::SetPadsConnected(bool connected) {
  for (int i = 0; i < kNumberOfGamepads; ++i) {
    test_data_.items[i].connected = connected;
  }
  fetcher_->SetTestData(test_data_);
}

void GamepadServiceTest::SimulateUserGesture(bool has_gesture) {
  test_data_.items[0].buttons[0].value = has_gesture ? 1.f : 0.f;
  test_data_.items[0].buttons[0].pressed = has_gesture ? true : false;
  fetcher_->SetTestData(test_data_);
}

void GamepadServiceTest::SimulatePageReload() {
  service_->ConsumerBecameInactive(connection_listener_.get());
  service_->ConsumerBecameActive(connection_listener_.get());
}

void GamepadServiceTest::ClearCounters() {
  connection_listener_->ClearCounters();
  if (connection_listener2_)
    connection_listener2_->ClearCounters();
}

void GamepadServiceTest::WaitForData() {
  fetcher_->WaitForDataReadAndCallbacksIssued();
  base::RunLoop().RunUntilIdle();
}

TEST_F(GamepadServiceTest, ConnectionsTest) {
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());

  ClearCounters();
  SimulateUserGesture(true);
  SetPadsConnected(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());

  ClearCounters();
  SetPadsConnected(false);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetDisconnectedCounter());

  ClearCounters();
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
}

TEST_F(GamepadServiceTest, ConnectionThenGestureTest) {
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());

  // No connection events are sent until a user gesture is seen.
  ClearCounters();
  SetPadsConnected(true);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());

  ClearCounters();
  SimulateUserGesture(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());

  ClearCounters();
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
}

TEST_F(GamepadServiceTest, ReloadTest) {
  // No connection events are sent until a user gesture is seen.
  SetPadsConnected(true);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());

  ClearCounters();
  SimulatePageReload();
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());

  // After a user gesture, the connection listener is notified about connected
  // gamepads.
  ClearCounters();
  SimulateUserGesture(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());

  // After a reload, if the gamepads were already connected (and we have seen
  // a user gesture) then the connection listener is notified about connected
  // gamepads.
  ClearCounters();
  SimulatePageReload();
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());

  ClearCounters();
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
}

// Flaky, see https://crbug.com/795170
TEST_F(GamepadServiceTest, DISABLED_SecondConsumerGestureTest) {
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  // Simulate a user gesture. The gesture is received before the second
  // consumer is active.
  ClearCounters();
  SetPadsConnected(true);
  SimulateUserGesture(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  // The second consumer becomes active, but should not receive connection
  // events until a new user gesture is received.
  ClearCounters();
  SimulateUserGesture(false);
  InitializeSecondConsumer();
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  // Connection events should only be sent to the second consumer.
  ClearCounters();
  SimulateUserGesture(true);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());
}

TEST_F(GamepadServiceTest, ConnectWhileInactiveTest) {
  // Ensure the initial user gesture is received by both consumers.
  InitializeSecondConsumer();
  SimulateUserGesture(true);
  SetPadsConnected(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetPadsConnected(false);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(kNumberOfGamepads, GetDisconnectedCounter2());

  // Check that connecting gamepads while a consumer is inactive will notify
  // once the consumer is active.
  ClearCounters();
  SetSecondConsumerActive(false);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetPadsConnected(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetSecondConsumerActive(true);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());
}

TEST_F(GamepadServiceTest, ConnectAndDisconnectWhileInactiveTest) {
  // Ensure the initial user gesture is received by both consumers.
  InitializeSecondConsumer();
  SimulateUserGesture(true);
  SetPadsConnected(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetPadsConnected(false);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(kNumberOfGamepads, GetDisconnectedCounter2());

  // Check that a connection and then disconnection is NOT reported once the
  // consumer is active.
  ClearCounters();
  SetSecondConsumerActive(false);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetPadsConnected(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetPadsConnected(false);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetSecondConsumerActive(true);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());
}

TEST_F(GamepadServiceTest, DisconnectWhileInactiveTest) {
  // Ensure the initial user gesture is received by both consumers.
  InitializeSecondConsumer();
  SimulateUserGesture(true);
  SetPadsConnected(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  // Check that disconnecting gamepads while a consumer is inactive will notify
  // once the consumer is active.
  ClearCounters();
  SetSecondConsumerActive(false);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetPadsConnected(false);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetSecondConsumerActive(true);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(kNumberOfGamepads, GetDisconnectedCounter2());

  ClearCounters();
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());
}

TEST_F(GamepadServiceTest, DisconnectAndConnectWhileInactiveTest) {
  // Ensure the initial user gesture is received by both consumers.
  InitializeSecondConsumer();
  SimulateUserGesture(true);
  SetPadsConnected(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  // Check that a disconnection and then connection is reported as a connection
  // (and no disconnection) once the consumer is active.
  ClearCounters();
  SetSecondConsumerActive(false);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetPadsConnected(false);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetPadsConnected(true);
  WaitForData();
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(0, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());

  ClearCounters();
  SetSecondConsumerActive(true);
  WaitForData();
  EXPECT_EQ(0, GetConnectedCounter());
  EXPECT_EQ(0, GetDisconnectedCounter());
  EXPECT_EQ(kNumberOfGamepads, GetConnectedCounter2());
  EXPECT_EQ(0, GetDisconnectedCounter2());
}

}  // namespace device
