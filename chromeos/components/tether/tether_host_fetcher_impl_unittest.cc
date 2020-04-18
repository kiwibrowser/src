// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/tether_host_fetcher_impl.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/optional.h"
#include "components/cryptauth/fake_remote_device_provider.h"
#include "components/cryptauth/remote_device.h"
#include "components/cryptauth/remote_device_ref.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

namespace {

const size_t kNumTestDevices = 5;

class TestObserver : public TetherHostFetcher::Observer {
 public:
  TestObserver() = default;
  virtual ~TestObserver() = default;

  size_t num_updates() { return num_updates_; }

  // TetherHostFetcher::Observer:
  void OnTetherHostsUpdated() override { ++num_updates_; }

 private:
  size_t num_updates_ = 0;
};

}  // namespace

class TetherHostFetcherImplTest : public testing::Test {
 public:
  TetherHostFetcherImplTest()
      : test_remote_device_list_(CreateTestRemoteDeviceList()),
        test_remote_device_ref_list_(
            CreateTestRemoteDeviceRefList(test_remote_device_list_)) {}

  void SetUp() override {
    fake_remote_device_provider_ =
        std::make_unique<cryptauth::FakeRemoteDeviceProvider>();
    fake_remote_device_provider_->set_synced_remote_devices(
        test_remote_device_list_);

    tether_host_fetcher_ = TetherHostFetcherImpl::Factory::NewInstance(
        fake_remote_device_provider_.get());

    test_observer_ = std::make_unique<TestObserver>();
    tether_host_fetcher_->AddObserver(test_observer_.get());
  }

  void OnTetherHostListFetched(
      const cryptauth::RemoteDeviceRefList& device_list) {
    last_fetched_list_ = device_list;
  }

  void OnSingleTetherHostFetched(
      base::Optional<cryptauth::RemoteDeviceRef> device) {
    last_fetched_single_host_ = device;
  }

  void VerifyAllTetherHosts(
      const cryptauth::RemoteDeviceRefList expected_list) {
    tether_host_fetcher_->FetchAllTetherHosts(
        base::Bind(&TetherHostFetcherImplTest::OnTetherHostListFetched,
                   base::Unretained(this)));
    EXPECT_EQ(expected_list, last_fetched_list_);
  }

  void VerifySingleTetherHost(
      const std::string& device_id,
      base::Optional<cryptauth::RemoteDeviceRef> expected_device) {
    tether_host_fetcher_->FetchTetherHost(
        device_id,
        base::Bind(&TetherHostFetcherImplTest::OnSingleTetherHostFetched,
                   base::Unretained(this)));
    if (expected_device)
      EXPECT_EQ(expected_device, last_fetched_single_host_);
    else
      EXPECT_EQ(base::nullopt, last_fetched_single_host_);
  }

  cryptauth::RemoteDeviceList CreateTestRemoteDeviceList() {
    cryptauth::RemoteDeviceList list =
        cryptauth::CreateRemoteDeviceListForTest(kNumTestDevices);
    for (auto device : list)
      device.supports_mobile_hotspot = true;

    return list;
  }

  cryptauth::RemoteDeviceRefList CreateTestRemoteDeviceRefList(
      cryptauth::RemoteDeviceList remote_device_list) {
    cryptauth::RemoteDeviceRefList list;
    for (const auto& device : remote_device_list) {
      list.push_back(cryptauth::RemoteDeviceRef(
          std::make_shared<cryptauth::RemoteDevice>(device)));
    }
    return list;
  }

  cryptauth::RemoteDeviceList test_remote_device_list_;
  cryptauth::RemoteDeviceRefList test_remote_device_ref_list_;

  cryptauth::RemoteDeviceRefList last_fetched_list_;
  base::Optional<cryptauth::RemoteDeviceRef> last_fetched_single_host_;
  std::unique_ptr<TestObserver> test_observer_;

  std::unique_ptr<cryptauth::FakeRemoteDeviceProvider>
      fake_remote_device_provider_;

  std::unique_ptr<TetherHostFetcher> tether_host_fetcher_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TetherHostFetcherImplTest);
};

TEST_F(TetherHostFetcherImplTest, TestHasSyncedTetherHosts) {
  EXPECT_TRUE(tether_host_fetcher_->HasSyncedTetherHosts());
  EXPECT_EQ(0u, test_observer_->num_updates());

  // Update the list of devices to be empty.
  fake_remote_device_provider_->set_synced_remote_devices(
      cryptauth::RemoteDeviceList());
  fake_remote_device_provider_->NotifyObserversDeviceListChanged();
  EXPECT_FALSE(tether_host_fetcher_->HasSyncedTetherHosts());
  EXPECT_EQ(1u, test_observer_->num_updates());

  // Notify that the list has changed, even though it hasn't. There should be no
  // update.
  fake_remote_device_provider_->NotifyObserversDeviceListChanged();
  EXPECT_FALSE(tether_host_fetcher_->HasSyncedTetherHosts());
  EXPECT_EQ(1u, test_observer_->num_updates());

  // Update the list to include device 0 only.
  fake_remote_device_provider_->set_synced_remote_devices(
      {test_remote_device_list_[0]});
  fake_remote_device_provider_->NotifyObserversDeviceListChanged();
  EXPECT_TRUE(tether_host_fetcher_->HasSyncedTetherHosts());
  EXPECT_EQ(2u, test_observer_->num_updates());

  // Notify that the list has changed, even though it hasn't. There should be no
  // update.
  fake_remote_device_provider_->NotifyObserversDeviceListChanged();
  EXPECT_TRUE(tether_host_fetcher_->HasSyncedTetherHosts());
  EXPECT_EQ(2u, test_observer_->num_updates());
}

TEST_F(TetherHostFetcherImplTest, TestFetchAllTetherHosts) {
  // Create a list of test devices, only some of which are valid tether hosts.
  // Ensure that only that subset is fetched.

  test_remote_device_list_[3].supports_mobile_hotspot = false;
  test_remote_device_list_[4].supports_mobile_hotspot = false;

  cryptauth::RemoteDeviceRefList host_device_list(CreateTestRemoteDeviceRefList(
      {test_remote_device_list_[0], test_remote_device_list_[1],
       test_remote_device_list_[2]}));

  fake_remote_device_provider_->set_synced_remote_devices(
      test_remote_device_list_);
  fake_remote_device_provider_->NotifyObserversDeviceListChanged();
  VerifyAllTetherHosts(host_device_list);
}

TEST_F(TetherHostFetcherImplTest, TestSingleTetherHost) {
  VerifySingleTetherHost(test_remote_device_ref_list_[0].GetDeviceId(),
                         test_remote_device_ref_list_[0]);

  // Now, set device 0 as the only device. It should still be returned when
  // requested.
  fake_remote_device_provider_->set_synced_remote_devices(
      cryptauth::RemoteDeviceList{test_remote_device_list_[0]});
  fake_remote_device_provider_->NotifyObserversDeviceListChanged();
  VerifySingleTetherHost(test_remote_device_ref_list_[0].GetDeviceId(),
                         test_remote_device_ref_list_[0]);

  // Now, set another device as the only device, but remove its mobile data
  // support. It should not be returned.
  cryptauth::RemoteDevice remote_device = cryptauth::RemoteDevice();
  remote_device.supports_mobile_hotspot = false;

  fake_remote_device_provider_->set_synced_remote_devices(
      cryptauth::RemoteDeviceList{remote_device});
  fake_remote_device_provider_->NotifyObserversDeviceListChanged();
  VerifySingleTetherHost(test_remote_device_ref_list_[0].GetDeviceId(),
                         base::nullopt);

  // Update the list; now, there are no more devices.
  fake_remote_device_provider_->set_synced_remote_devices(
      cryptauth::RemoteDeviceList());
  fake_remote_device_provider_->NotifyObserversDeviceListChanged();
  VerifySingleTetherHost(test_remote_device_ref_list_[0].GetDeviceId(),
                         base::nullopt);
}

TEST_F(TetherHostFetcherImplTest,
       TestSingleTetherHost_IdDoesNotCorrespondToDevice) {
  VerifySingleTetherHost("nonexistentId", base::nullopt);
}

}  // namespace tether

}  // namespace chromeos
