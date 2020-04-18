// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/device_sensors/device_motion_event_pump.h"

#include <string.h>

#include <memory>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/test/test_utils.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/device/public/cpp/generic_sensor/motion_data.h"
#include "services/device/public/cpp/test/fake_sensor_and_provider.h"
#include "services/device/public/mojom/sensor.mojom.h"
#include "services/device/public/mojom/sensor_provider.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/device_orientation/web_device_motion_listener.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "ui/gfx/geometry/angle_conversions.h"

namespace content {

using device::FakeSensorProvider;

class MockDeviceMotionListener : public blink::WebDeviceMotionListener {
 public:
  MockDeviceMotionListener()
      : did_change_device_motion_(false), number_of_events_(0) {
    memset(&data_, 0, sizeof(data_));
  }
  ~MockDeviceMotionListener() override {}

  void DidChangeDeviceMotion(const device::MotionData& data) override {
    memcpy(&data_, &data, sizeof(data));
    did_change_device_motion_ = true;
    ++number_of_events_;
  }

  bool did_change_device_motion() const {
    return did_change_device_motion_;
  }

  int number_of_events() const { return number_of_events_; }

  const device::MotionData& data() const { return data_; }

 private:
  bool did_change_device_motion_;
  int number_of_events_;
  device::MotionData data_;

  DISALLOW_COPY_AND_ASSIGN(MockDeviceMotionListener);
};

class DeviceMotionEventPumpForTesting : public DeviceMotionEventPump {
 public:
  DeviceMotionEventPumpForTesting() : DeviceMotionEventPump(nullptr) {}
  ~DeviceMotionEventPumpForTesting() override {}

  // DeviceMotionEventPump:
  void SendStartMessage() override {
    DeviceMotionEventPump::SendStartMessageImpl();
  }

  int pump_delay_microseconds() const { return kDefaultPumpDelayMicroseconds; }

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceMotionEventPumpForTesting);
};

class DeviceMotionEventPumpTest : public testing::Test {
 public:
  DeviceMotionEventPumpTest() = default;

 protected:
  void SetUp() override {
    motion_pump_.reset(new DeviceMotionEventPumpForTesting());
    device::mojom::SensorProviderPtr sensor_provider_ptr;
    sensor_provider_.Bind(mojo::MakeRequest(&sensor_provider_ptr));
    motion_pump_->SetSensorProviderForTesting(std::move(sensor_provider_ptr));

    listener_.reset(new MockDeviceMotionListener);

    ExpectAllThreeSensorsStateToBe(
        DeviceMotionEventPump::SensorState::NOT_INITIALIZED);
    EXPECT_EQ(DeviceMotionEventPump::PumpState::STOPPED,
              motion_pump()->GetPumpStateForTesting());
  }

  void FireEvent() { motion_pump_->FireEvent(); }

  void ExpectAccelerometerStateToBe(
      DeviceMotionEventPump::SensorState expected_sensor_state) {
    EXPECT_EQ(expected_sensor_state, motion_pump_->accelerometer_.sensor_state);
  }

  void ExpectLinearAccelerationSensorStateToBe(
      DeviceMotionEventPump::SensorState expected_sensor_state) {
    EXPECT_EQ(expected_sensor_state,
              motion_pump_->linear_acceleration_sensor_.sensor_state);
  }

  void ExpectGyroscopeStateToBe(
      DeviceMotionEventPump::SensorState expected_sensor_state) {
    EXPECT_EQ(expected_sensor_state, motion_pump_->gyroscope_.sensor_state);
  }

  void ExpectAllThreeSensorsStateToBe(
      DeviceMotionEventPump::SensorState expected_sensor_state) {
    ExpectAccelerometerStateToBe(expected_sensor_state);
    ExpectLinearAccelerationSensorStateToBe(expected_sensor_state);
    ExpectGyroscopeStateToBe(expected_sensor_state);
  }

  DeviceMotionEventPumpForTesting* motion_pump() { return motion_pump_.get(); }

  MockDeviceMotionListener* listener() { return listener_.get(); }

  FakeSensorProvider* sensor_provider() { return &sensor_provider_; }

 private:
  base::MessageLoop loop_;
  std::unique_ptr<DeviceMotionEventPumpForTesting> motion_pump_;
  std::unique_ptr<MockDeviceMotionListener> listener_;
  FakeSensorProvider sensor_provider_;

  DISALLOW_COPY_AND_ASSIGN(DeviceMotionEventPumpTest);
};

TEST_F(DeviceMotionEventPumpTest, MultipleStartAndStopWithWait) {
  motion_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::ACTIVE);
  EXPECT_EQ(DeviceMotionEventPump::PumpState::RUNNING,
            motion_pump()->GetPumpStateForTesting());

  motion_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
  EXPECT_EQ(DeviceMotionEventPump::PumpState::STOPPED,
            motion_pump()->GetPumpStateForTesting());

  motion_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::ACTIVE);
  EXPECT_EQ(DeviceMotionEventPump::PumpState::RUNNING,
            motion_pump()->GetPumpStateForTesting());

  motion_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
  EXPECT_EQ(DeviceMotionEventPump::PumpState::STOPPED,
            motion_pump()->GetPumpStateForTesting());
}

TEST_F(DeviceMotionEventPumpTest, CallStop) {
  motion_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(
      DeviceMotionEventPump::SensorState::NOT_INITIALIZED);
}

TEST_F(DeviceMotionEventPumpTest, CallStartAndStop) {
  motion_pump()->Start(listener());
  motion_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceMotionEventPumpTest, CallStartMultipleTimes) {
  motion_pump()->Start(listener());
  motion_pump()->Start(listener());
  motion_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceMotionEventPumpTest, CallStopMultipleTimes) {
  motion_pump()->Start(listener());
  motion_pump()->Stop();
  motion_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
}

// Test multiple DeviceSensorEventPump::Start() calls only bind sensor once.
TEST_F(DeviceMotionEventPumpTest, SensorOnlyBindOnce) {
  motion_pump()->Start(listener());
  motion_pump()->Stop();
  motion_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::ACTIVE);

  motion_pump()->Stop();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceMotionEventPumpTest, AllSensorsAreActive) {
  motion_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateAccelerometerData(1, 2, 3);
  sensor_provider()->UpdateLinearAccelerationSensorData(4, 5, 6);
  sensor_provider()->UpdateGyroscopeData(7, 8, 9);

  FireEvent();

  device::MotionData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_motion());

  EXPECT_TRUE(received_data.has_acceleration_including_gravity_x);
  EXPECT_EQ(1, received_data.acceleration_including_gravity_x);
  EXPECT_TRUE(received_data.has_acceleration_including_gravity_y);
  EXPECT_EQ(2, received_data.acceleration_including_gravity_y);
  EXPECT_TRUE(received_data.has_acceleration_including_gravity_z);
  EXPECT_EQ(3, received_data.acceleration_including_gravity_z);

  EXPECT_TRUE(received_data.has_acceleration_x);
  EXPECT_EQ(4, received_data.acceleration_x);
  EXPECT_TRUE(received_data.has_acceleration_y);
  EXPECT_EQ(5, received_data.acceleration_y);
  EXPECT_TRUE(received_data.has_acceleration_z);
  EXPECT_EQ(6, received_data.acceleration_z);

  EXPECT_TRUE(received_data.has_rotation_rate_alpha);
  EXPECT_EQ(gfx::RadToDeg(7.0), received_data.rotation_rate_alpha);
  EXPECT_TRUE(received_data.has_rotation_rate_beta);
  EXPECT_EQ(gfx::RadToDeg(8.0), received_data.rotation_rate_beta);
  EXPECT_TRUE(received_data.has_rotation_rate_gamma);
  EXPECT_EQ(gfx::RadToDeg(9.0), received_data.rotation_rate_gamma);

  motion_pump()->Stop();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceMotionEventPumpTest, TwoSensorsAreActive) {
  sensor_provider()->set_linear_acceleration_sensor_is_available(false);

  motion_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAccelerometerStateToBe(DeviceMotionEventPump::SensorState::ACTIVE);
  ExpectLinearAccelerationSensorStateToBe(
      DeviceMotionEventPump::SensorState::NOT_INITIALIZED);
  ExpectGyroscopeStateToBe(DeviceMotionEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateAccelerometerData(1, 2, 3);
  sensor_provider()->UpdateGyroscopeData(7, 8, 9);

  FireEvent();

  device::MotionData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_motion());

  EXPECT_TRUE(received_data.has_acceleration_including_gravity_x);
  EXPECT_EQ(1, received_data.acceleration_including_gravity_x);
  EXPECT_TRUE(received_data.has_acceleration_including_gravity_y);
  EXPECT_EQ(2, received_data.acceleration_including_gravity_y);
  EXPECT_TRUE(received_data.has_acceleration_including_gravity_z);
  EXPECT_EQ(3, received_data.acceleration_including_gravity_z);

  EXPECT_FALSE(received_data.has_acceleration_x);
  EXPECT_FALSE(received_data.has_acceleration_y);
  EXPECT_FALSE(received_data.has_acceleration_z);

  EXPECT_TRUE(received_data.has_rotation_rate_alpha);
  EXPECT_EQ(gfx::RadToDeg(7.0), received_data.rotation_rate_alpha);
  EXPECT_TRUE(received_data.has_rotation_rate_beta);
  EXPECT_EQ(gfx::RadToDeg(8.0), received_data.rotation_rate_beta);
  EXPECT_TRUE(received_data.has_rotation_rate_gamma);
  EXPECT_EQ(gfx::RadToDeg(9.0), received_data.rotation_rate_gamma);

  motion_pump()->Stop();

  ExpectAccelerometerStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
  ExpectLinearAccelerationSensorStateToBe(
      DeviceMotionEventPump::SensorState::NOT_INITIALIZED);
  ExpectGyroscopeStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceMotionEventPumpTest, SomeSensorDataFieldsNotAvailable) {
  motion_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateAccelerometerData(NAN, 2, 3);
  sensor_provider()->UpdateLinearAccelerationSensorData(4, NAN, 6);
  sensor_provider()->UpdateGyroscopeData(7, 8, NAN);

  FireEvent();

  device::MotionData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_motion());

  EXPECT_FALSE(received_data.has_acceleration_including_gravity_x);
  EXPECT_TRUE(received_data.has_acceleration_including_gravity_y);
  EXPECT_EQ(2, received_data.acceleration_including_gravity_y);
  EXPECT_TRUE(received_data.has_acceleration_including_gravity_z);
  EXPECT_EQ(3, received_data.acceleration_including_gravity_z);

  EXPECT_TRUE(received_data.has_acceleration_x);
  EXPECT_EQ(4, received_data.acceleration_x);
  EXPECT_FALSE(received_data.has_acceleration_y);
  EXPECT_TRUE(received_data.has_acceleration_z);
  EXPECT_EQ(6, received_data.acceleration_z);

  EXPECT_TRUE(received_data.has_rotation_rate_alpha);
  EXPECT_EQ(gfx::RadToDeg(7.0), received_data.rotation_rate_alpha);
  EXPECT_TRUE(received_data.has_rotation_rate_beta);
  EXPECT_EQ(gfx::RadToDeg(8.0), received_data.rotation_rate_beta);
  EXPECT_FALSE(received_data.has_rotation_rate_gamma);

  motion_pump()->Stop();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceMotionEventPumpTest, FireAllNullEvent) {
  // No active sensors.
  sensor_provider()->set_accelerometer_is_available(false);
  sensor_provider()->set_linear_acceleration_sensor_is_available(false);
  sensor_provider()->set_gyroscope_is_available(false);

  motion_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(
      DeviceMotionEventPump::SensorState::NOT_INITIALIZED);

  FireEvent();

  device::MotionData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_motion());

  EXPECT_FALSE(received_data.has_acceleration_x);
  EXPECT_FALSE(received_data.has_acceleration_y);
  EXPECT_FALSE(received_data.has_acceleration_z);

  EXPECT_FALSE(received_data.has_acceleration_including_gravity_x);
  EXPECT_FALSE(received_data.has_acceleration_including_gravity_y);
  EXPECT_FALSE(received_data.has_acceleration_including_gravity_z);

  EXPECT_FALSE(received_data.has_rotation_rate_alpha);
  EXPECT_FALSE(received_data.has_rotation_rate_beta);
  EXPECT_FALSE(received_data.has_rotation_rate_gamma);

  motion_pump()->Stop();

  ExpectAllThreeSensorsStateToBe(
      DeviceMotionEventPump::SensorState::NOT_INITIALIZED);
}

TEST_F(DeviceMotionEventPumpTest,
       NotFireEventWhenSensorReadingTimeStampIsZero) {
  motion_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::ACTIVE);

  FireEvent();
  EXPECT_FALSE(listener()->did_change_device_motion());

  sensor_provider()->UpdateAccelerometerData(1, 2, 3);
  FireEvent();
  EXPECT_FALSE(listener()->did_change_device_motion());

  sensor_provider()->UpdateLinearAccelerationSensorData(4, 5, 6);
  FireEvent();
  EXPECT_FALSE(listener()->did_change_device_motion());

  sensor_provider()->UpdateGyroscopeData(7, 8, 9);
  FireEvent();
  // Event is fired only after all the available sensors have data.
  EXPECT_TRUE(listener()->did_change_device_motion());

  motion_pump()->Stop();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);
}

// Confirm that the frequency of pumping events is not greater than 60Hz.
// A rate above 60Hz would allow for the detection of keystrokes.
// (crbug.com/421691)
TEST_F(DeviceMotionEventPumpTest, PumpThrottlesEventRate) {
  // Confirm that the delay for pumping events is 60 Hz.
  EXPECT_GE(60, base::Time::kMicrosecondsPerSecond /
      motion_pump()->pump_delay_microseconds());

  motion_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateAccelerometerData(1, 2, 3);
  sensor_provider()->UpdateLinearAccelerationSensorData(4, 5, 6);
  sensor_provider()->UpdateGyroscopeData(7, 8, 9);

  blink::scheduler::GetSingleThreadTaskRunnerForTesting()->PostDelayedTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated(),
      base::TimeDelta::FromMilliseconds(100));
  base::RunLoop().Run();
  motion_pump()->Stop();

  ExpectAllThreeSensorsStateToBe(DeviceMotionEventPump::SensorState::SUSPENDED);

  // Check that the blink::WebDeviceMotionListener does not receive excess
  // events.
  EXPECT_TRUE(listener()->did_change_device_motion());
  EXPECT_GE(6, listener()->number_of_events());
}

}  // namespace content
