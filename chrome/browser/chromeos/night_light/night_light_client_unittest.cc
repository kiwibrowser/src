// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/night_light/night_light_client.h"

#include "ash/public/interfaces/night_light_controller.mojom.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/clock.h"
#include "base/time/tick_clock.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using ScheduleType = ash::mojom::NightLightController::ScheduleType;

// A fake implementation of NightLightController for testing.
class FakeNightLightController : public ash::mojom::NightLightController {
 public:
  FakeNightLightController() : binding_(this) {}
  ~FakeNightLightController() override = default;

  const ash::mojom::SimpleGeopositionPtr& position() const { return position_; }

  int position_pushes_num() const { return position_pushes_num_; }

  ash::mojom::NightLightControllerPtr CreateInterfacePtrAndBind() {
    ash::mojom::NightLightControllerPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  // ash::mojom::NightLightController:
  void SetCurrentGeoposition(
      ash::mojom::SimpleGeopositionPtr position) override {
    position_ = std::move(position);
    ++position_pushes_num_;
  }

  void SetClient(ash::mojom::NightLightClientPtr client) override {
    client_ = std::move(client);
  }

  void NotifyScheduleTypeChanged(ScheduleType type) {
    client_->OnScheduleTypeChanged(type);
    client_.FlushForTesting();
  }

 private:
  ash::mojom::SimpleGeopositionPtr position_;
  ash::mojom::NightLightClientPtr client_;
  mojo::Binding<ash::mojom::NightLightController> binding_;

  // The number of times a new position is pushed to this controller.
  int position_pushes_num_ = 0;

  DISALLOW_COPY_AND_ASSIGN(FakeNightLightController);
};

// A fake implementation of NightLightClient that doesn't perform any actual
// geoposition requests.
class FakeNightLightClient : public NightLightClient,
                             public base::Clock,
                             public base::TickClock {
 public:
  FakeNightLightClient() : NightLightClient(nullptr /* url_context_getter */) {
    SetTimerForTesting(
        std::make_unique<base::OneShotTimer>(this /* tick_clock */));
    SetClockForTesting(this);
  }
  ~FakeNightLightClient() override = default;

  // base::Clock:
  base::Time Now() const override { return fake_now_; }

  // base::TickClock:
  base::TimeTicks NowTicks() const override { return fake_now_ticks_; }

  void set_fake_now(base::Time now) { fake_now_ = now; }
  void set_fake_now_ticks(base::TimeTicks now_ticks) {
    fake_now_ticks_ = now_ticks;
  }

  void set_position_to_send(const chromeos::Geoposition& position) {
    position_to_send_ = position;
  }

  int geoposition_requests_num() const { return geoposition_requests_num_; }

 private:
  // night_light::NightLightClient:
  void RequestGeoposition() override {
    OnGeoposition(position_to_send_, false, base::TimeDelta());
    ++geoposition_requests_num_;
  }

  base::Time fake_now_;
  base::TimeTicks fake_now_ticks_;

  // The position to send to the controller the next time OnGeoposition is
  // invoked.
  chromeos::Geoposition position_to_send_;

  // The number of new geoposition requests that have been triggered.
  int geoposition_requests_num_ = 0;

  DISALLOW_COPY_AND_ASSIGN(FakeNightLightClient);
};

// Base test fixture.
class NightLightClientTest : public testing::Test {
 public:
  NightLightClientTest() = default;
  ~NightLightClientTest() override = default;

  void SetUp() override {
    // Deterministic fake time that doesn't change for the sake of testing.
    client_.set_fake_now(base::Time::Now());
    client_.set_fake_now_ticks(base::TimeTicks::Now());

    client_.SetNightLightControllerPtrForTesting(
        controller_.CreateInterfacePtrAndBind());
    client_.Start();
    client_.FlushNightLightControllerForTesting();
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  FakeNightLightController controller_;
  FakeNightLightClient client_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NightLightClientTest);
};

// Test that the client is retrieving geoposition periodically only when the
// schedule type is "sunset to sunrise".
TEST_F(NightLightClientTest, TestClientRunningOnlyWhenSunsetToSunriseSchedule) {
  EXPECT_FALSE(client_.using_geoposition());
  controller_.NotifyScheduleTypeChanged(ScheduleType::kNone);
  EXPECT_FALSE(client_.using_geoposition());
  controller_.NotifyScheduleTypeChanged(ScheduleType::kCustom);
  controller_.NotifyScheduleTypeChanged(ScheduleType::kSunsetToSunrise);
  scoped_task_environment_.RunUntilIdle();
  client_.FlushNightLightControllerForTesting();
  EXPECT_TRUE(client_.using_geoposition());

  // Client should stop retrieving geopositions when schedule type changes to
  // something else.
  controller_.NotifyScheduleTypeChanged(ScheduleType::kNone);
  EXPECT_FALSE(client_.using_geoposition());
}

// Test that client only pushes valid positions.
TEST_F(NightLightClientTest, TestInvalidPositions) {
  EXPECT_EQ(0, controller_.position_pushes_num());
  chromeos::Geoposition position;
  position.latitude = 32.0;
  position.longitude = 31.0;
  position.status = chromeos::Geoposition::STATUS_TIMEOUT;
  position.accuracy = 10;
  position.timestamp = base::Time::Now();
  client_.set_position_to_send(position);
  controller_.NotifyScheduleTypeChanged(ScheduleType::kSunsetToSunrise);
  scoped_task_environment_.RunUntilIdle();
  client_.FlushNightLightControllerForTesting();
  EXPECT_EQ(1, client_.geoposition_requests_num());
  EXPECT_EQ(0, controller_.position_pushes_num());
}

// Test that successive changes of the schedule type to sunset to sunrise do not
// trigger repeated geoposition requests.
TEST_F(NightLightClientTest, TestRepeatedScheduleTypeChanges) {
  // Start with a valid position, and expect it to be delivered to the
  // controller.
  EXPECT_EQ(0, controller_.position_pushes_num());
  chromeos::Geoposition position1;
  position1.latitude = 32.0;
  position1.longitude = 31.0;
  position1.status = chromeos::Geoposition::STATUS_OK;
  position1.accuracy = 10;
  position1.timestamp = base::Time::Now();
  client_.set_position_to_send(position1);
  controller_.NotifyScheduleTypeChanged(ScheduleType::kSunsetToSunrise);
  scoped_task_environment_.RunUntilIdle();
  client_.FlushNightLightControllerForTesting();
  EXPECT_EQ(1, client_.geoposition_requests_num());
  EXPECT_EQ(1, controller_.position_pushes_num());
  EXPECT_EQ(client_.Now(), client_.last_successful_geo_request_time());

  // A new different position just for the sake of comparison with position1 to
  // make sure that no new requests are triggered and the same old position will
  // be resent to the controller.
  chromeos::Geoposition position2;
  position2.latitude = 100.0;
  position2.longitude = 200.0;
  position2.status = chromeos::Geoposition::STATUS_OK;
  position2.accuracy = 10;
  position2.timestamp = base::Time::Now();
  client_.set_position_to_send(position2);
  controller_.NotifyScheduleTypeChanged(ScheduleType::kSunsetToSunrise);
  scoped_task_environment_.RunUntilIdle();
  client_.FlushNightLightControllerForTesting();
  // No new request has been triggered, however the same old valid position was
  // pushed to the controller.
  EXPECT_EQ(1, client_.geoposition_requests_num());
  EXPECT_EQ(2, controller_.position_pushes_num());
  EXPECT_TRUE(ash::mojom::SimpleGeoposition::New(position1.latitude,
                                                 position1.longitude)
                  .Equals(controller_.position()));

  // The timer should be running scheduling a next request that is a
  // kNextRequestDelayAfterSuccess from the last successful request time.
  EXPECT_TRUE(client_.timer().IsRunning());
  base::TimeDelta expected_delay =
      client_.last_successful_geo_request_time() +
      NightLightClient::GetNextRequestDelayAfterSuccessForTesting() -
      client_.Now();
  EXPECT_EQ(expected_delay, client_.timer().GetCurrentDelay());
}

}  // namespace
