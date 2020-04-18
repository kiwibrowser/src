// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/chromoting_test_fixture.h"
#include "remoting/test/connection_time_observer.h"

namespace {
const base::TimeDelta kPinBasedMaxConnectionTimeInSeconds =
    base::TimeDelta::FromSeconds(10);
}

namespace remoting {
namespace test {

// Note: Make sure to restart the host before running this test. Connecting to
// a previously connected host will output the time to reconnect versus the time
// to connect.
TEST_F(ChromotingTestFixture, TestMeasurePinBasedAuthentication) {
  bool connected = ConnectToHost(kPinBasedMaxConnectionTimeInSeconds);
  EXPECT_TRUE(connected);

  Disconnect();
  EXPECT_FALSE(connection_time_observer_->GetStateTransitionTime(
      protocol::ConnectionToHost::State::INITIALIZING,
      protocol::ConnectionToHost::State::CLOSED).is_max());

  connection_time_observer_->DisplayConnectionStats();
}

// Note: Make sure to restart the host before running this test. If the host
// is not restarted after a previous connection, then the first connection will
// be a reconnect and the second connection will be a second reconnect.
TEST_F(ChromotingTestFixture, TestMeasureReconnectPerformance) {
  bool connected = ConnectToHost(kPinBasedMaxConnectionTimeInSeconds);
  EXPECT_TRUE(connected);

  Disconnect();
  EXPECT_FALSE(connection_time_observer_->GetStateTransitionTime(
      protocol::ConnectionToHost::State::INITIALIZING,
      protocol::ConnectionToHost::State::CLOSED).is_max());

  // Begin reconnection to same host.
  connected = ConnectToHost(kPinBasedMaxConnectionTimeInSeconds);
  EXPECT_TRUE(connected);

  Disconnect();
  EXPECT_FALSE(connection_time_observer_->GetStateTransitionTime(
      protocol::ConnectionToHost::State::INITIALIZING,
      protocol::ConnectionToHost::State::CLOSED).is_max());

  connection_time_observer_->DisplayConnectionStats();
}

}  // namespace test
}  // namespace remoting
