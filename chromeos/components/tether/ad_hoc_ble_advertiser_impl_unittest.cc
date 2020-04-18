// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ad_hoc_ble_advertiser_impl.h"

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_simple_task_runner.h"
#include "base/timer/mock_timer.h"
#include "chromeos/components/tether/ble_constants.h"
#include "chromeos/components/tether/error_tolerant_ble_advertisement_impl.h"
#include "chromeos/components/tether/fake_ble_synchronizer.h"
#include "chromeos/components/tether/fake_error_tolerant_ble_advertisement.h"
#include "chromeos/components/tether/timer_factory.h"
#include "components/cryptauth/ble/ble_advertisement_generator.h"
#include "components/cryptauth/ble/fake_ble_advertisement_generator.h"
#include "components/cryptauth/mock_foreground_eid_generator.h"
#include "components/cryptauth/mock_local_device_data_provider.h"
#include "components/cryptauth/mock_remote_beacon_seed_fetcher.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

namespace {

constexpr const int64_t kNumSecondsToAdvertise = 12;

std::vector<cryptauth::DataWithTimestamp> GenerateFakeAdvertisements() {
  cryptauth::DataWithTimestamp advertisement1("advertisement1", 1000L, 2000L);
  cryptauth::DataWithTimestamp advertisement2("advertisement2", 2000L, 3000L);

  std::vector<cryptauth::DataWithTimestamp> advertisements = {advertisement1,
                                                              advertisement2};
  return advertisements;
}

class FakeErrorTolerantBleAdvertisementFactory final
    : public ErrorTolerantBleAdvertisementImpl::Factory {
 public:
  FakeErrorTolerantBleAdvertisementFactory() {}
  ~FakeErrorTolerantBleAdvertisementFactory() override {}

  const std::vector<FakeErrorTolerantBleAdvertisement*>&
  active_advertisements() {
    return active_advertisements_;
  }

  size_t num_created() { return num_created_; }

  // ErrorTolerantBleAdvertisementImpl::Factory:
  std::unique_ptr<ErrorTolerantBleAdvertisement> BuildInstance(
      const std::string& device_id,
      std::unique_ptr<cryptauth::DataWithTimestamp> advertisement_data,
      BleSynchronizerBase* ble_synchronizer) override {
    FakeErrorTolerantBleAdvertisement* fake_advertisement =
        new FakeErrorTolerantBleAdvertisement(
            device_id, base::Bind(&FakeErrorTolerantBleAdvertisementFactory::
                                      OnFakeAdvertisementDeleted,
                                  base::Unretained(this)));
    active_advertisements_.push_back(fake_advertisement);
    ++num_created_;
    return base::WrapUnique(fake_advertisement);
  }

 protected:
  void OnFakeAdvertisementDeleted(
      FakeErrorTolerantBleAdvertisement* fake_advertisement) {
    EXPECT_TRUE(std::find(active_advertisements_.begin(),
                          active_advertisements_.end(),
                          fake_advertisement) != active_advertisements_.end());
    base::Erase(active_advertisements_, fake_advertisement);
  }

 private:
  std::vector<FakeErrorTolerantBleAdvertisement*> active_advertisements_;
  size_t num_created_ = 0;
};

class TestObserver final : public AdHocBleAdvertiser::Observer {
 public:
  TestObserver() {}
  ~TestObserver() override {}

  size_t num_times_shutdown_complete() { return num_times_shutdown_complete_; }

  // AdHocBleAdvertiser::Observer:
  void OnAsynchronousShutdownComplete() override {
    ++num_times_shutdown_complete_;
  }

 private:
  size_t num_times_shutdown_complete_ = 0;
};

class TestTimerFactory : public TimerFactory {
 public:
  ~TestTimerFactory() override {}

  // TimerFactory:
  std::unique_ptr<base::Timer> CreateOneShotTimer() override {
    EXPECT_FALSE(device_id_for_next_timer_.empty());
    base::MockTimer* mock_timer = new base::MockTimer(
        false /* retain_user_task */, false /* is_repeating */);
    device_id_to_timer_map_[device_id_for_next_timer_] = mock_timer;
    return base::WrapUnique(mock_timer);
  }

  base::MockTimer* GetTimerForDeviceId(const std::string& device_id) {
    return device_id_to_timer_map_[device_id];
  }

  void set_device_id_for_next_timer(
      const std::string& device_id_for_next_timer) {
    device_id_for_next_timer_ = device_id_for_next_timer;
  }

 private:
  std::string device_id_for_next_timer_;
  std::unordered_map<std::string, base::MockTimer*> device_id_to_timer_map_;
};

}  // namespace

class AdHocBleAdvertiserImplTest : public testing::Test {
 protected:
  AdHocBleAdvertiserImplTest()
      : fake_devices_(cryptauth::CreateRemoteDeviceRefListForTest(2)),
        fake_advertisements_(GenerateFakeAdvertisements()) {}

  void SetUp() override {
    fake_generator_ =
        std::make_unique<cryptauth::FakeBleAdvertisementGenerator>();
    cryptauth::BleAdvertisementGenerator::SetInstanceForTesting(
        fake_generator_.get());

    fake_advertisement_factory_ =
        base::WrapUnique(new FakeErrorTolerantBleAdvertisementFactory());
    ErrorTolerantBleAdvertisementImpl::Factory::SetInstanceForTesting(
        fake_advertisement_factory_.get());

    mock_seed_fetcher_ =
        std::make_unique<cryptauth::MockRemoteBeaconSeedFetcher>();
    mock_local_data_provider_ =
        std::make_unique<cryptauth::MockLocalDeviceDataProvider>();
    fake_ble_synchronizer_ = std::make_unique<FakeBleSynchronizer>();

    workaround_ = std::make_unique<AdHocBleAdvertiserImpl>(
        mock_local_data_provider_.get(), mock_seed_fetcher_.get(),
        fake_ble_synchronizer_.get());

    test_timer_factory_ = new TestTimerFactory();
    workaround_->SetTimerFactoryForTesting(
        base::WrapUnique(test_timer_factory_));

    test_observer_ = base::WrapUnique(new TestObserver());
    workaround_->AddObserver(test_observer_.get());
  }

  void TearDown() override {
    ErrorTolerantBleAdvertisementImpl::Factory::SetInstanceForTesting(nullptr);
    cryptauth::BleAdvertisementGenerator::SetInstanceForTesting(nullptr);
  }

  void SetAdvertisement(size_t index) {
    fake_generator_->set_advertisement(
        std::make_unique<cryptauth::DataWithTimestamp>(
            fake_advertisements_[0]));
  }

  void FireTimer(cryptauth::RemoteDeviceRef remote_device) {
    base::MockTimer* timer =
        test_timer_factory_->GetTimerForDeviceId(remote_device.GetDeviceId());
    ASSERT_TRUE(timer);
    EXPECT_EQ(base::TimeDelta::FromSeconds(kNumSecondsToAdvertise),
              timer->GetCurrentDelay());
    timer->Fire();
  }

  void FinishStoppingAdvertisement(size_t expected_index,
                                   cryptauth::RemoteDeviceRef remote_device) {
    FakeErrorTolerantBleAdvertisement* advertisement =
        fake_advertisement_factory_->active_advertisements()[expected_index];
    ASSERT_TRUE(advertisement);
    EXPECT_EQ(remote_device.GetDeviceId(), advertisement->device_id());
    advertisement->InvokeStopCallback();
  }

  const cryptauth::RemoteDeviceRefList fake_devices_;
  const std::vector<cryptauth::DataWithTimestamp> fake_advertisements_;

  std::unique_ptr<cryptauth::MockRemoteBeaconSeedFetcher> mock_seed_fetcher_;
  std::unique_ptr<cryptauth::MockLocalDeviceDataProvider>
      mock_local_data_provider_;
  std::unique_ptr<FakeBleSynchronizer> fake_ble_synchronizer_;

  std::unique_ptr<cryptauth::FakeBleAdvertisementGenerator> fake_generator_;
  TestTimerFactory* test_timer_factory_;

  std::unique_ptr<TestObserver> test_observer_;

  std::unique_ptr<FakeErrorTolerantBleAdvertisementFactory>
      fake_advertisement_factory_;

  std::unique_ptr<AdHocBleAdvertiserImpl> workaround_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AdHocBleAdvertiserImplTest);
};

TEST_F(AdHocBleAdvertiserImplTest, CannotGenerateAdvertisement) {
  fake_generator_->set_advertisement(nullptr);
  workaround_->RequestGattServicesForDevice(fake_devices_[0].GetDeviceId());
  EXPECT_EQ(0u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(0u, test_observer_->num_times_shutdown_complete());
  EXPECT_FALSE(workaround_->HasPendingRequests());
}

TEST_F(AdHocBleAdvertiserImplTest, AdvertiseAndStop) {
  SetAdvertisement(0 /* index */);
  test_timer_factory_->set_device_id_for_next_timer(
      fake_devices_[0].GetDeviceId());

  workaround_->RequestGattServicesForDevice(fake_devices_[0].GetDeviceId());
  EXPECT_TRUE(workaround_->HasPendingRequests());
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());

  FireTimer(fake_devices_[0]);
  EXPECT_EQ(0u, test_observer_->num_times_shutdown_complete());
  EXPECT_TRUE(workaround_->HasPendingRequests());

  FinishStoppingAdvertisement(0u /* expected_index */, fake_devices_[0]);
  EXPECT_FALSE(workaround_->HasPendingRequests());
  EXPECT_EQ(1u, test_observer_->num_times_shutdown_complete());
}

TEST_F(AdHocBleAdvertiserImplTest, TwoRequestsForSameDevice_BeforeTimer) {
  SetAdvertisement(0 /* index */);
  test_timer_factory_->set_device_id_for_next_timer(
      fake_devices_[0].GetDeviceId());

  workaround_->RequestGattServicesForDevice(fake_devices_[0].GetDeviceId());
  EXPECT_TRUE(workaround_->HasPendingRequests());
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());

  // No additional advertisement should be created.
  workaround_->RequestGattServicesForDevice(fake_devices_[0].GetDeviceId());
  EXPECT_TRUE(workaround_->HasPendingRequests());
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());

  FireTimer(fake_devices_[0]);
  EXPECT_TRUE(workaround_->HasPendingRequests());

  FinishStoppingAdvertisement(0u /* expected_index */, fake_devices_[0]);
  EXPECT_FALSE(workaround_->HasPendingRequests());
  EXPECT_EQ(1u, test_observer_->num_times_shutdown_complete());
}

TEST_F(AdHocBleAdvertiserImplTest, TwoRequestsForSameDevice_AfterTimer) {
  SetAdvertisement(0 /* index */);
  test_timer_factory_->set_device_id_for_next_timer(
      fake_devices_[0].GetDeviceId());

  workaround_->RequestGattServicesForDevice(fake_devices_[0].GetDeviceId());
  EXPECT_TRUE(workaround_->HasPendingRequests());
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());

  FireTimer(fake_devices_[0]);
  EXPECT_TRUE(workaround_->HasPendingRequests());

  // No additional advertisement should be created.
  workaround_->RequestGattServicesForDevice(fake_devices_[0].GetDeviceId());
  EXPECT_TRUE(workaround_->HasPendingRequests());
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());

  FinishStoppingAdvertisement(0u /* expected_index */, fake_devices_[0]);
  EXPECT_FALSE(workaround_->HasPendingRequests());
  EXPECT_EQ(1u, test_observer_->num_times_shutdown_complete());
}

TEST_F(AdHocBleAdvertiserImplTest, TwoRequestsForDifferentDevices) {
  SetAdvertisement(0 /* index */);
  test_timer_factory_->set_device_id_for_next_timer(
      fake_devices_[0].GetDeviceId());
  workaround_->RequestGattServicesForDevice(fake_devices_[0].GetDeviceId());
  EXPECT_TRUE(workaround_->HasPendingRequests());
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());

  SetAdvertisement(1 /* index */);
  test_timer_factory_->set_device_id_for_next_timer(
      fake_devices_[1].GetDeviceId());
  workaround_->RequestGattServicesForDevice(fake_devices_[1].GetDeviceId());
  EXPECT_TRUE(workaround_->HasPendingRequests());
  EXPECT_EQ(2u, fake_advertisement_factory_->num_created());

  FireTimer(fake_devices_[0]);
  EXPECT_TRUE(workaround_->HasPendingRequests());

  // Finish stopping the 0th device. There should still be pending requests.
  FinishStoppingAdvertisement(0u /* expected_index */, fake_devices_[0]);
  EXPECT_TRUE(workaround_->HasPendingRequests());
  EXPECT_EQ(0u, test_observer_->num_times_shutdown_complete());

  FireTimer(fake_devices_[1]);
  EXPECT_TRUE(workaround_->HasPendingRequests());

  // |expected_index| is still 0u, since the other advertisement was deleted
  // from the vector.
  FinishStoppingAdvertisement(0u /* expected_index */, fake_devices_[1]);
  EXPECT_FALSE(workaround_->HasPendingRequests());
  EXPECT_EQ(1u, test_observer_->num_times_shutdown_complete());
}

}  // namespace tether

}  // namespace chromeos
