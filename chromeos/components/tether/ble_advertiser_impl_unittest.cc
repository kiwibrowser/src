// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_advertiser_impl.h"

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_simple_task_runner.h"
#include "chromeos/components/tether/ble_constants.h"
#include "chromeos/components/tether/error_tolerant_ble_advertisement_impl.h"
#include "chromeos/components/tether/fake_ble_synchronizer.h"
#include "chromeos/components/tether/fake_error_tolerant_ble_advertisement.h"
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

std::vector<cryptauth::DataWithTimestamp> GenerateFakeAdvertisements() {
  cryptauth::DataWithTimestamp advertisement1("advertisement1", 1000L, 2000L);
  cryptauth::DataWithTimestamp advertisement2("advertisement2", 2000L, 3000L);
  cryptauth::DataWithTimestamp advertisement3("advertisement3", 3000L, 4000L);

  std::vector<cryptauth::DataWithTimestamp> advertisements = {
      advertisement1, advertisement2, advertisement3};
  return advertisements;
}

class FakeErrorTolerantBleAdvertisementFactory
    : public ErrorTolerantBleAdvertisementImpl::Factory {
 public:
  FakeErrorTolerantBleAdvertisementFactory() = default;
  ~FakeErrorTolerantBleAdvertisementFactory() override = default;

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

class TestObserver final : public BleAdvertiser::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  size_t num_times_all_advertisements_unregistered() {
    return num_times_all_advertisements_unregistered_;
  }

  // BleAdvertiser::Observer:
  void OnAllAdvertisementsUnregistered() override {
    ++num_times_all_advertisements_unregistered_;
  }

 private:
  size_t num_times_all_advertisements_unregistered_ = 0;
};

// Deletes the BleAdvertiser when notified.
class DeletingObserver final : public BleAdvertiser::Observer {
 public:
  DeletingObserver(std::unique_ptr<BleAdvertiserImpl>& ble_advertiser)
      : ble_advertiser_(ble_advertiser) {
    ble_advertiser_->AddObserver(this);
  }

  ~DeletingObserver() override = default;

  // BleAdvertiser::Observer:
  void OnAllAdvertisementsUnregistered() override {
    ble_advertiser_->RemoveObserver(this);
    ble_advertiser_.reset();
  }

 private:
  std::unique_ptr<BleAdvertiserImpl>& ble_advertiser_;
};

}  // namespace

class BleAdvertiserImplTest : public testing::Test {
 protected:
  BleAdvertiserImplTest()
      : fake_devices_(cryptauth::CreateRemoteDeviceRefListForTest(3)),
        fake_advertisements_(GenerateFakeAdvertisements()) {}

  void SetUp() override {
    fake_generator_ =
        std::make_unique<cryptauth::FakeBleAdvertisementGenerator>();
    cryptauth::BleAdvertisementGenerator::SetInstanceForTesting(
        fake_generator_.get());

    mock_seed_fetcher_ =
        std::make_unique<cryptauth::MockRemoteBeaconSeedFetcher>();
    mock_local_data_provider_ =
        std::make_unique<cryptauth::MockLocalDeviceDataProvider>();
    fake_ble_synchronizer_ = std::make_unique<FakeBleSynchronizer>();

    fake_advertisement_factory_ =
        base::WrapUnique(new FakeErrorTolerantBleAdvertisementFactory());
    ErrorTolerantBleAdvertisementImpl::Factory::SetInstanceForTesting(
        fake_advertisement_factory_.get());

    ble_advertiser_ = base::WrapUnique(new BleAdvertiserImpl(
        mock_local_data_provider_.get(), mock_seed_fetcher_.get(),
        fake_ble_synchronizer_.get()));

    test_task_runner_ = base::MakeRefCounted<base::TestSimpleTaskRunner>();
    ble_advertiser_->SetTaskRunnerForTesting(test_task_runner_);

    test_observer_ = base::WrapUnique(new TestObserver());
    ble_advertiser_->AddObserver(test_observer_.get());
  }

  void TearDown() override {
    ErrorTolerantBleAdvertisementImpl::Factory::SetInstanceForTesting(nullptr);
    cryptauth::BleAdvertisementGenerator::SetInstanceForTesting(nullptr);
  }

  void VerifyAdvertisementHasBeenStopped(
      size_t index,
      const std::string& expected_device_id) {
    FakeErrorTolerantBleAdvertisement* advertisement =
        fake_advertisement_factory_->active_advertisements()[index];
    EXPECT_EQ(expected_device_id, advertisement->device_id());
    EXPECT_TRUE(advertisement->HasBeenStopped());
  }

  void InvokeAdvertisementStoppedCallback(
      size_t index,
      const std::string& expected_device_id) {
    FakeErrorTolerantBleAdvertisement* advertisement =
        fake_advertisement_factory_->active_advertisements()[index];
    EXPECT_EQ(expected_device_id, advertisement->device_id());
    advertisement->InvokeStopCallback();
  }

  const base::test::ScopedTaskEnvironment scoped_task_environment_;
  const cryptauth::RemoteDeviceRefList fake_devices_;
  const std::vector<cryptauth::DataWithTimestamp> fake_advertisements_;

  std::unique_ptr<cryptauth::MockRemoteBeaconSeedFetcher> mock_seed_fetcher_;
  std::unique_ptr<cryptauth::MockLocalDeviceDataProvider>
      mock_local_data_provider_;
  std::unique_ptr<FakeBleSynchronizer> fake_ble_synchronizer_;

  std::unique_ptr<cryptauth::FakeBleAdvertisementGenerator> fake_generator_;

  scoped_refptr<base::TestSimpleTaskRunner> test_task_runner_;

  std::unique_ptr<TestObserver> test_observer_;

  std::unique_ptr<FakeErrorTolerantBleAdvertisementFactory>
      fake_advertisement_factory_;

  std::unique_ptr<BleAdvertiserImpl> ble_advertiser_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BleAdvertiserImplTest);
};

TEST_F(BleAdvertiserImplTest, CannotGenerateAdvertisement) {
  fake_generator_->set_advertisement(nullptr);
  EXPECT_FALSE(ble_advertiser_->StartAdvertisingToDevice(
      fake_devices_[0].GetDeviceId()));
  EXPECT_EQ(0u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(0u, test_observer_->num_times_all_advertisements_unregistered());
}

TEST_F(BleAdvertiserImplTest, AdvertisementRegisteredSuccessfully) {
  fake_generator_->set_advertisement(
      std::make_unique<cryptauth::DataWithTimestamp>(fake_advertisements_[0]));

  EXPECT_TRUE(ble_advertiser_->StartAdvertisingToDevice(
      fake_devices_[0].GetDeviceId()));
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(1u, fake_advertisement_factory_->active_advertisements().size());
  EXPECT_TRUE(ble_advertiser_->AreAdvertisementsRegistered());
  EXPECT_EQ(0u, test_observer_->num_times_all_advertisements_unregistered());

  // Now, unregister.
  EXPECT_TRUE(
      ble_advertiser_->StopAdvertisingToDevice(fake_devices_[0].GetDeviceId()));
  EXPECT_TRUE(ble_advertiser_->AreAdvertisementsRegistered());

  // The advertisement should have been stopped, but it should not yet have
  // been removed.
  EXPECT_EQ(1u, fake_advertisement_factory_->active_advertisements().size());
  VerifyAdvertisementHasBeenStopped(0u /* index */,
                                    fake_devices_[0].GetDeviceId());

  // Invoke the stop callback and ensure the advertisement was deleted.
  InvokeAdvertisementStoppedCallback(0u /* index */,
                                     fake_devices_[0].GetDeviceId());
  test_task_runner_->RunUntilIdle();
  EXPECT_EQ(0u, fake_advertisement_factory_->active_advertisements().size());
  EXPECT_FALSE(ble_advertiser_->AreAdvertisementsRegistered());
  EXPECT_EQ(1u, test_observer_->num_times_all_advertisements_unregistered());
}

TEST_F(BleAdvertiserImplTest, AdvertisementRegisteredSuccessfully_TwoDevices) {
  // Register device 0.
  fake_generator_->set_advertisement(
      std::make_unique<cryptauth::DataWithTimestamp>(fake_advertisements_[0]));
  EXPECT_TRUE(ble_advertiser_->StartAdvertisingToDevice(
      fake_devices_[0].GetDeviceId()));
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(1u, fake_advertisement_factory_->active_advertisements().size());
  EXPECT_TRUE(ble_advertiser_->AreAdvertisementsRegistered());
  EXPECT_EQ(0u, test_observer_->num_times_all_advertisements_unregistered());

  // Register device 1.
  fake_generator_->set_advertisement(
      std::make_unique<cryptauth::DataWithTimestamp>(fake_advertisements_[1]));
  EXPECT_TRUE(ble_advertiser_->StartAdvertisingToDevice(
      fake_devices_[1].GetDeviceId()));
  EXPECT_EQ(2u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(2u, fake_advertisement_factory_->active_advertisements().size());
  EXPECT_TRUE(ble_advertiser_->AreAdvertisementsRegistered());
  EXPECT_EQ(0u, test_observer_->num_times_all_advertisements_unregistered());

  // Unregister device 0.
  EXPECT_TRUE(
      ble_advertiser_->StopAdvertisingToDevice(fake_devices_[0].GetDeviceId()));
  EXPECT_EQ(2u, fake_advertisement_factory_->active_advertisements().size());
  InvokeAdvertisementStoppedCallback(0u /* index */,
                                     fake_devices_[0].GetDeviceId());
  EXPECT_EQ(1u, fake_advertisement_factory_->active_advertisements().size());
  EXPECT_TRUE(ble_advertiser_->AreAdvertisementsRegistered());
  EXPECT_EQ(0u, test_observer_->num_times_all_advertisements_unregistered());

  // Unregister device 1.
  EXPECT_TRUE(
      ble_advertiser_->StopAdvertisingToDevice(fake_devices_[1].GetDeviceId()));
  EXPECT_EQ(1u, fake_advertisement_factory_->active_advertisements().size());
  InvokeAdvertisementStoppedCallback(0u /* index */,
                                     fake_devices_[1].GetDeviceId());
  test_task_runner_->RunUntilIdle();
  EXPECT_EQ(0u, fake_advertisement_factory_->active_advertisements().size());
  EXPECT_FALSE(ble_advertiser_->AreAdvertisementsRegistered());
  EXPECT_EQ(1u, test_observer_->num_times_all_advertisements_unregistered());
}

TEST_F(BleAdvertiserImplTest, TooManyDevicesRegistered) {
  ASSERT_EQ(2u, kMaxConcurrentAdvertisements);

  // Register device 0.
  fake_generator_->set_advertisement(
      std::make_unique<cryptauth::DataWithTimestamp>(fake_advertisements_[0]));
  EXPECT_TRUE(ble_advertiser_->StartAdvertisingToDevice(
      fake_devices_[0].GetDeviceId()));
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(1u, fake_advertisement_factory_->active_advertisements().size());

  // Register device 1.
  fake_generator_->set_advertisement(
      std::make_unique<cryptauth::DataWithTimestamp>(fake_advertisements_[1]));
  EXPECT_TRUE(ble_advertiser_->StartAdvertisingToDevice(
      fake_devices_[1].GetDeviceId()));
  EXPECT_EQ(2u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(2u, fake_advertisement_factory_->active_advertisements().size());

  // Register device 2. This should fail, since it is over the limit.
  fake_generator_->set_advertisement(
      std::make_unique<cryptauth::DataWithTimestamp>(fake_advertisements_[2]));
  EXPECT_FALSE(ble_advertiser_->StartAdvertisingToDevice(
      fake_devices_[2].GetDeviceId()));
  EXPECT_EQ(2u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(2u, fake_advertisement_factory_->active_advertisements().size());

  // Now, stop advertising to device 1. It should now be possible to advertise
  // to device 2.
  EXPECT_TRUE(
      ble_advertiser_->StopAdvertisingToDevice(fake_devices_[1].GetDeviceId()));
  EXPECT_TRUE(ble_advertiser_->StartAdvertisingToDevice(
      fake_devices_[2].GetDeviceId()));

  // However, the advertisement for device 1 should still be present, and no new
  // advertisement for device 2 should have yet been created.
  EXPECT_EQ(2u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(2u, fake_advertisement_factory_->active_advertisements().size());
  VerifyAdvertisementHasBeenStopped(1u /* index */,
                                    fake_devices_[1].GetDeviceId());

  // Stop the advertisement for device 1. This should cause a new advertisement
  // for device 2 to be created.
  InvokeAdvertisementStoppedCallback(1u /* index */,
                                     fake_devices_[1].GetDeviceId());
  test_task_runner_->RunUntilIdle();
  EXPECT_EQ(3u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(2u, fake_advertisement_factory_->active_advertisements().size());

  // Verify that the remaining active advertisements correspond to the correct
  // devices.
  EXPECT_EQ(
      fake_devices_[0].GetDeviceId(),
      fake_advertisement_factory_->active_advertisements()[0]->device_id());
  EXPECT_EQ(
      fake_devices_[2].GetDeviceId(),
      fake_advertisement_factory_->active_advertisements()[1]->device_id());
}

// Regression test for crbug.com/739883. This issue arises when the following
// occurs:
//   (1) BleAdvertiserImpl starts advertising to device A.
//   (2) BleAdvertiserImpl stops advertising to device A. The advertisement
//       starts its asynchyronous unregistration flow.
//   (3) BleAdvertiserImpl starts advertising to device A again, but the
//       previous advertisement has not yet been fully unregistered.
// Before the fix for crbug.com/739883, this would cause an error of type
// ERROR_ADVERTISEMENT_ALREADY_EXISTS. However, the fix for this issue ensures
// that the new advertisement in step (3) above does not start until the
// previous one has been finished.
TEST_F(BleAdvertiserImplTest, SameAdvertisementAdded_FirstHasNotBeenStopped) {
  fake_generator_->set_advertisement(
      std::make_unique<cryptauth::DataWithTimestamp>(fake_advertisements_[0]));

  EXPECT_TRUE(ble_advertiser_->StartAdvertisingToDevice(
      fake_devices_[0].GetDeviceId()));
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(1u, fake_advertisement_factory_->active_advertisements().size());

  // Unregister, but do not invoke the stop callback.
  EXPECT_TRUE(
      ble_advertiser_->StopAdvertisingToDevice(fake_devices_[0].GetDeviceId()));
  VerifyAdvertisementHasBeenStopped(0u /* index */,
                                    fake_devices_[0].GetDeviceId());

  // Start advertising again, to the same device. Since the previous
  // advertisement has not successfully stopped, no new advertisement should
  // have been created yet.
  fake_generator_->set_advertisement(
      std::make_unique<cryptauth::DataWithTimestamp>(fake_advertisements_[0]));
  EXPECT_TRUE(ble_advertiser_->StartAdvertisingToDevice(
      fake_devices_[0].GetDeviceId()));
  EXPECT_EQ(1u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(1u, fake_advertisement_factory_->active_advertisements().size());

  // Now, complete the previous stop. This should cause a new advertisement to
  // be generated, but only the new one should be active.
  InvokeAdvertisementStoppedCallback(0u /* index */,
                                     fake_devices_[0].GetDeviceId());
  test_task_runner_->RunUntilIdle();
  EXPECT_EQ(2u, fake_advertisement_factory_->num_created());
  EXPECT_EQ(1u, fake_advertisement_factory_->active_advertisements().size());
}

// Regression test for crbug.com/776241. This bug could cause a crash if, when
// BleAdvertiserImpl notifies observers that all advertisements were
// unregistered, an observer deletes BleAdvertiserImpl. The fix for this issue
// is simply processing the next advertisement in a new task so that the new
// task will be canceled if the object is deleted. Without the fix for
// crbug.com/776241, this test would crash.
TEST_F(BleAdvertiserImplTest, ObserverDeletesObjectWhenNotified) {
  // For this test, use a DeletingObserver instead.
  DeletingObserver deleting_observer(ble_advertiser_);
  ble_advertiser_->RemoveObserver(test_observer_.get());

  fake_generator_->set_advertisement(
      std::make_unique<cryptauth::DataWithTimestamp>(fake_advertisements_[0]));
  ble_advertiser_->StartAdvertisingToDevice(fake_devices_[0].GetDeviceId());
  ble_advertiser_->StopAdvertisingToDevice(fake_devices_[0].GetDeviceId());
  InvokeAdvertisementStoppedCallback(0u /* index */,
                                     fake_devices_[0].GetDeviceId());
  test_task_runner_->RunUntilIdle();
}

}  // namespace tether

}  // namespace chromeos
