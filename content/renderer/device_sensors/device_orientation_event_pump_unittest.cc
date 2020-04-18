// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/device_sensors/device_orientation_event_pump.h"

#include <string.h>

#include <memory>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "content/public/test/test_utils.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/device/public/cpp/generic_sensor/orientation_data.h"
#include "services/device/public/cpp/test/fake_sensor_and_provider.h"
#include "services/device/public/mojom/sensor.mojom.h"
#include "services/device/public/mojom/sensor_provider.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/device_orientation/web_device_orientation_listener.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"

namespace {

constexpr double kEpsilon = 1e-8;

}  // namespace

namespace content {

using device::FakeSensorProvider;

class MockDeviceOrientationListener
    : public blink::WebDeviceOrientationListener {
 public:
  MockDeviceOrientationListener() : did_change_device_orientation_(false) {
    memset(&data_, 0, sizeof(data_));
  }
  ~MockDeviceOrientationListener() override {}

  void DidChangeDeviceOrientation(
      const device::OrientationData& data) override {
    memcpy(&data_, &data, sizeof(data));
    did_change_device_orientation_ = true;
  }

  bool did_change_device_orientation() const {
    return did_change_device_orientation_;
  }
  void set_did_change_device_orientation(bool value) {
    did_change_device_orientation_ = value;
  }
  const device::OrientationData& data() const { return data_; }

 private:
  bool did_change_device_orientation_;
  device::OrientationData data_;

  DISALLOW_COPY_AND_ASSIGN(MockDeviceOrientationListener);
};

class DeviceOrientationEventPumpForTesting : public DeviceOrientationEventPump {
 public:
  DeviceOrientationEventPumpForTesting(bool absolute)
      : DeviceOrientationEventPump(nullptr, absolute) {}
  ~DeviceOrientationEventPumpForTesting() override {}

  // DeviceOrientationEventPump:
  void SendStartMessage() override {
    DeviceOrientationEventPump::SendStartMessageImpl();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceOrientationEventPumpForTesting);
};

class DeviceOrientationEventPumpTest : public testing::Test {
 public:
  DeviceOrientationEventPumpTest() = default;

 protected:
  void SetUp() override {
    orientation_pump_.reset(
        new DeviceOrientationEventPumpForTesting(false /* absolute */));
    device::mojom::SensorProviderPtr sensor_provider_ptr;
    sensor_provider_.Bind(mojo::MakeRequest(&sensor_provider_ptr));
    orientation_pump_->SetSensorProviderForTesting(
        std::move(sensor_provider_ptr));

    listener_.reset(new MockDeviceOrientationListener);

    ExpectRelativeOrientationSensorStateToBe(
        DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
    ExpectAbsoluteOrientationSensorStateToBe(
        DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
    EXPECT_EQ(DeviceOrientationEventPump::PumpState::STOPPED,
              orientation_pump()->GetPumpStateForTesting());
  }

  void FireEvent() { orientation_pump_->FireEvent(); }

  void ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState expected_sensor_state) {
    EXPECT_EQ(expected_sensor_state,
              orientation_pump_->relative_orientation_sensor_.sensor_state);
  }

  void ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState expected_sensor_state) {
    EXPECT_EQ(expected_sensor_state,
              orientation_pump_->absolute_orientation_sensor_.sensor_state);
  }

  DeviceOrientationEventPump* orientation_pump() {
    return orientation_pump_.get();
  }

  MockDeviceOrientationListener* listener() { return listener_.get(); }

  FakeSensorProvider* sensor_provider() { return &sensor_provider_; }

 private:
  base::MessageLoop loop_;
  std::unique_ptr<DeviceOrientationEventPumpForTesting> orientation_pump_;
  std::unique_ptr<MockDeviceOrientationListener> listener_;
  FakeSensorProvider sensor_provider_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOrientationEventPumpTest);
};

TEST_F(DeviceOrientationEventPumpTest, MultipleStartAndStopWithWait) {
  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::RUNNING,
            orientation_pump()->GetPumpStateForTesting());

  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::STOPPED,
            orientation_pump()->GetPumpStateForTesting());

  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::RUNNING,
            orientation_pump()->GetPumpStateForTesting());

  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::STOPPED,
            orientation_pump()->GetPumpStateForTesting());
}

TEST_F(DeviceOrientationEventPumpTest,
       MultipleStartAndStopWithWaitWithSensorFallback) {
  sensor_provider()->set_relative_orientation_sensor_is_available(false);

  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::RUNNING,
            orientation_pump()->GetPumpStateForTesting());

  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::STOPPED,
            orientation_pump()->GetPumpStateForTesting());

  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::RUNNING,
            orientation_pump()->GetPumpStateForTesting());

  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::STOPPED,
            orientation_pump()->GetPumpStateForTesting());
}

TEST_F(DeviceOrientationEventPumpTest, CallStop) {
  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
}

TEST_F(DeviceOrientationEventPumpTest, CallStopWithSensorFallback) {
  sensor_provider()->set_relative_orientation_sensor_is_available(false);

  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
}

TEST_F(DeviceOrientationEventPumpTest, CallStartAndStop) {
  orientation_pump()->Start(listener());
  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest, CallStartAndStopWithSensorFallback) {
  sensor_provider()->set_relative_orientation_sensor_is_available(false);

  orientation_pump()->Start(listener());
  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest, CallStartMultipleTimes) {
  orientation_pump()->Start(listener());
  orientation_pump()->Start(listener());
  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest,
       CallStartMultipleTimesWithSensorFallback) {
  sensor_provider()->set_relative_orientation_sensor_is_available(false);

  orientation_pump()->Start(listener());
  orientation_pump()->Start(listener());
  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest, CallStopMultipleTimes) {
  orientation_pump()->Start(listener());
  orientation_pump()->Stop();
  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest,
       CallStopMultipleTimesWithSensorFallback) {
  sensor_provider()->set_relative_orientation_sensor_is_available(false);

  orientation_pump()->Start(listener());
  orientation_pump()->Stop();
  orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

// Test a sequence of Start(), Stop(), Start() calls only bind sensor once.
TEST_F(DeviceOrientationEventPumpTest, SensorOnlyBindOnce) {
  orientation_pump()->Start(listener());
  orientation_pump()->Stop();
  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

// Test when using fallback from relative orientation to absolute orientation,
// a sequence of Start(), Stop(), Start() calls only bind sensor once.
TEST_F(DeviceOrientationEventPumpTest, SensorOnlyBindOnceWithSensorFallback) {
  sensor_provider()->set_relative_orientation_sensor_is_available(false);

  orientation_pump()->Start(listener());
  orientation_pump()->Stop();
  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest, SensorIsActive) {
  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateRelativeOrientationSensorData(
      1 /* alpha */, 2 /* beta */, 3 /* gamma */);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  // DeviceOrientation Event provides relative orientation data when it is
  // available.
  EXPECT_DOUBLE_EQ(1, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(2, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(3, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_FALSE(received_data.absolute);

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest, SensorIsActiveWithSensorFallback) {
  sensor_provider()->set_relative_orientation_sensor_is_available(false);

  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateAbsoluteOrientationSensorData(
      4 /* alpha */, 5 /* beta */, 6 /* gamma */);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  // DeviceOrientation Event provides absolute orientation data when relative
  // orientation data is not available but absolute orientation data is
  // available.
  EXPECT_DOUBLE_EQ(4, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(5, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(6, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  // Since no relative orientation data is available, DeviceOrientationEvent
  // fallback to provide absolute orientation data.
  EXPECT_TRUE(received_data.absolute);

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest, SomeSensorDataFieldsNotAvailable) {
  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateRelativeOrientationSensorData(
      NAN /* alpha */, 2 /* beta */, 3 /* gamma */);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  EXPECT_FALSE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(2, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(3, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_FALSE(received_data.absolute);

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest,
       SomeSensorDataFieldsNotAvailableWithSensorFallback) {
  sensor_provider()->set_relative_orientation_sensor_is_available(false);

  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateAbsoluteOrientationSensorData(
      4 /* alpha */, NAN /* beta */, 6 /* gamma */);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  // DeviceOrientation Event provides absolute orientation data when relative
  // orientation data is not available but absolute orientation data is
  // available.
  EXPECT_DOUBLE_EQ(4, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_FALSE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(6, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  // Since no relative orientation data is available, DeviceOrientationEvent
  // fallback to provide absolute orientation data.
  EXPECT_TRUE(received_data.absolute);

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest, FireAllNullEvent) {
  // No active sensors.
  sensor_provider()->set_relative_orientation_sensor_is_available(false);
  sensor_provider()->set_absolute_orientation_sensor_is_available(false);

  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  EXPECT_FALSE(received_data.has_alpha);
  EXPECT_FALSE(received_data.has_beta);
  EXPECT_FALSE(received_data.has_gamma);
  EXPECT_FALSE(received_data.absolute);

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
}

TEST_F(DeviceOrientationEventPumpTest,
       NotFireEventWhenSensorReadingTimeStampIsZero) {
  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  FireEvent();

  EXPECT_FALSE(listener()->did_change_device_orientation());

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest,
       NotFireEventWhenSensorReadingTimeStampIsZeroWithSensorFallback) {
  sensor_provider()->set_relative_orientation_sensor_is_available(false);

  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  FireEvent();

  EXPECT_FALSE(listener()->did_change_device_orientation());

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest, UpdateRespectsOrientationThreshold) {
  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateRelativeOrientationSensorData(
      1 /* alpha */, 2 /* beta */, 3 /* gamma */);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  // DeviceOrientation Event provides relative orientation data when it is
  // available.
  EXPECT_DOUBLE_EQ(1, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(2, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(3, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_FALSE(received_data.absolute);

  listener()->set_did_change_device_orientation(false);

  sensor_provider()->UpdateRelativeOrientationSensorData(
      1 + DeviceOrientationEventPump::kOrientationThreshold / 2.0 /* alpha */,
      2 /* beta */, 3 /* gamma */);

  FireEvent();

  received_data = listener()->data();
  EXPECT_FALSE(listener()->did_change_device_orientation());

  EXPECT_DOUBLE_EQ(1, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(2, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(3, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_FALSE(received_data.absolute);

  listener()->set_did_change_device_orientation(false);

  sensor_provider()->UpdateRelativeOrientationSensorData(
      1 + DeviceOrientationEventPump::kOrientationThreshold /* alpha */,
      2 /* beta */, 3 /* gamma */);

  FireEvent();

  received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  EXPECT_DOUBLE_EQ(1 + DeviceOrientationEventPump::kOrientationThreshold,
                   received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(2, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(3, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_FALSE(received_data.absolute);

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceOrientationEventPumpTest,
       UpdateRespectsOrientationThresholdWithSensorFallback) {
  sensor_provider()->set_relative_orientation_sensor_is_available(false);

  orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateAbsoluteOrientationSensorData(
      4 /* alpha */, 5 /* beta */, 6 /* gamma */);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  // DeviceOrientation Event provides absolute orientation data when relative
  // orientation data is not available but absolute orientation data is
  // available.
  EXPECT_DOUBLE_EQ(4, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(5, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(6, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  // Since no relative orientation data is available, DeviceOrientationEvent
  // fallback to provide absolute orientation data.
  EXPECT_TRUE(received_data.absolute);

  listener()->set_did_change_device_orientation(false);

  sensor_provider()->UpdateAbsoluteOrientationSensorData(
      4 /* alpha */,
      5 + DeviceOrientationEventPump::kOrientationThreshold / 2.0 /* beta */,
      6 /* gamma */);

  FireEvent();

  received_data = listener()->data();
  EXPECT_FALSE(listener()->did_change_device_orientation());

  EXPECT_DOUBLE_EQ(4, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(5, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(6, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_TRUE(received_data.absolute);

  listener()->set_did_change_device_orientation(false);

  sensor_provider()->UpdateAbsoluteOrientationSensorData(
      4 /* alpha */,
      5 + DeviceOrientationEventPump::kOrientationThreshold +
          kEpsilon /* beta */,
      6 /* gamma */);

  FireEvent();

  received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  EXPECT_DOUBLE_EQ(4, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(
      5 + DeviceOrientationEventPump::kOrientationThreshold + kEpsilon,
      received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(6, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_TRUE(received_data.absolute);

  orientation_pump()->Stop();

  ExpectRelativeOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

class DeviceAbsoluteOrientationEventPumpTest : public testing::Test {
 public:
  DeviceAbsoluteOrientationEventPumpTest() = default;

 protected:
  void SetUp() override {
    absolute_orientation_pump_.reset(
        new DeviceOrientationEventPumpForTesting(true /* absolute */));
    device::mojom::SensorProviderPtr sensor_provider_ptr;
    sensor_provider_.Bind(mojo::MakeRequest(&sensor_provider_ptr));
    absolute_orientation_pump_->SetSensorProviderForTesting(
        std::move(sensor_provider_ptr));

    listener_.reset(new MockDeviceOrientationListener);

    ExpectAbsoluteOrientationSensorStateToBe(
        DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
    EXPECT_EQ(DeviceOrientationEventPump::PumpState::STOPPED,
              absolute_orientation_pump()->GetPumpStateForTesting());
  }

  void FireEvent() { absolute_orientation_pump_->FireEvent(); }

  void ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState expected_sensor_state) {
    EXPECT_EQ(
        expected_sensor_state,
        absolute_orientation_pump_->absolute_orientation_sensor_.sensor_state);
  }

  DeviceOrientationEventPump* absolute_orientation_pump() {
    return absolute_orientation_pump_.get();
  }

  MockDeviceOrientationListener* listener() { return listener_.get(); }

  FakeSensorProvider* sensor_provider() { return &sensor_provider_; }

 private:
  base::MessageLoop loop_;
  std::unique_ptr<DeviceOrientationEventPumpForTesting>
      absolute_orientation_pump_;
  std::unique_ptr<MockDeviceOrientationListener> listener_;
  FakeSensorProvider sensor_provider_;

  DISALLOW_COPY_AND_ASSIGN(DeviceAbsoluteOrientationEventPumpTest);
};

TEST_F(DeviceAbsoluteOrientationEventPumpTest, MultipleStartAndStopWithWait) {
  absolute_orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::RUNNING,
            absolute_orientation_pump()->GetPumpStateForTesting());

  absolute_orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::STOPPED,
            absolute_orientation_pump()->GetPumpStateForTesting());

  absolute_orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::RUNNING,
            absolute_orientation_pump()->GetPumpStateForTesting());

  absolute_orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
  EXPECT_EQ(DeviceOrientationEventPump::PumpState::STOPPED,
            absolute_orientation_pump()->GetPumpStateForTesting());
}

TEST_F(DeviceAbsoluteOrientationEventPumpTest, CallStop) {
  absolute_orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
}

TEST_F(DeviceAbsoluteOrientationEventPumpTest, CallStartAndStop) {
  absolute_orientation_pump()->Start(listener());
  absolute_orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceAbsoluteOrientationEventPumpTest, CallStartMultipleTimes) {
  absolute_orientation_pump()->Start(listener());
  absolute_orientation_pump()->Start(listener());
  absolute_orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceAbsoluteOrientationEventPumpTest, CallStopMultipleTimes) {
  absolute_orientation_pump()->Start(listener());
  absolute_orientation_pump()->Stop();
  absolute_orientation_pump()->Stop();
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

// Test multiple DeviceSensorEventPump::Start() calls only bind sensor once.
TEST_F(DeviceAbsoluteOrientationEventPumpTest, SensorOnlyBindOnce) {
  absolute_orientation_pump()->Start(listener());
  absolute_orientation_pump()->Stop();
  absolute_orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  absolute_orientation_pump()->Stop();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceAbsoluteOrientationEventPumpTest, SensorIsActive) {
  absolute_orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateAbsoluteOrientationSensorData(
      4 /* alpha */, 5 /* beta */, 6 /* gamma */);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  EXPECT_DOUBLE_EQ(4, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(5, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(6, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_TRUE(received_data.absolute);

  absolute_orientation_pump()->Stop();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceAbsoluteOrientationEventPumpTest,
       SomeSensorDataFieldsNotAvailable) {
  absolute_orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateAbsoluteOrientationSensorData(
      4 /* alpha */, NAN /* beta */, 6 /* gamma */);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  EXPECT_DOUBLE_EQ(4, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_FALSE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(6, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_TRUE(received_data.absolute);

  absolute_orientation_pump()->Stop();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceAbsoluteOrientationEventPumpTest, FireAllNullEvent) {
  // No active sensor.
  sensor_provider()->set_absolute_orientation_sensor_is_available(false);

  absolute_orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  EXPECT_FALSE(received_data.has_alpha);
  EXPECT_FALSE(received_data.has_beta);
  EXPECT_FALSE(received_data.has_gamma);
  EXPECT_TRUE(received_data.absolute);

  absolute_orientation_pump()->Stop();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::NOT_INITIALIZED);
}

TEST_F(DeviceAbsoluteOrientationEventPumpTest,
       NotFireEventWhenSensorReadingTimeStampIsZero) {
  absolute_orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  FireEvent();

  EXPECT_FALSE(listener()->did_change_device_orientation());

  absolute_orientation_pump()->Stop();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

TEST_F(DeviceAbsoluteOrientationEventPumpTest,
       UpdateRespectsOrientationThreshold) {
  absolute_orientation_pump()->Start(listener());
  base::RunLoop().RunUntilIdle();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::ACTIVE);

  sensor_provider()->UpdateAbsoluteOrientationSensorData(
      4 /* alpha */, 5 /* beta */, 6 /* gamma */);

  FireEvent();

  device::OrientationData received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  EXPECT_DOUBLE_EQ(4, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(5, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(6, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_TRUE(received_data.absolute);

  listener()->set_did_change_device_orientation(false);

  sensor_provider()->UpdateAbsoluteOrientationSensorData(
      4 /* alpha */,
      5 + DeviceOrientationEventPump::kOrientationThreshold / 2.0 /* beta */,
      6 /* gamma */);

  FireEvent();

  received_data = listener()->data();
  EXPECT_FALSE(listener()->did_change_device_orientation());

  EXPECT_DOUBLE_EQ(4, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(5, received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(6, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_TRUE(received_data.absolute);

  listener()->set_did_change_device_orientation(false);

  sensor_provider()->UpdateAbsoluteOrientationSensorData(
      4 /* alpha */,
      5 + DeviceOrientationEventPump::kOrientationThreshold +
          kEpsilon /* beta */,
      6 /* gamma */);

  FireEvent();

  received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_orientation());

  EXPECT_DOUBLE_EQ(4, received_data.alpha);
  EXPECT_TRUE(received_data.has_alpha);
  EXPECT_DOUBLE_EQ(
      5 + DeviceOrientationEventPump::kOrientationThreshold + kEpsilon,
      received_data.beta);
  EXPECT_TRUE(received_data.has_beta);
  EXPECT_DOUBLE_EQ(6, received_data.gamma);
  EXPECT_TRUE(received_data.has_gamma);
  EXPECT_TRUE(received_data.absolute);

  absolute_orientation_pump()->Stop();

  ExpectAbsoluteOrientationSensorStateToBe(
      DeviceOrientationEventPump::SensorState::SUSPENDED);
}

}  // namespace content
