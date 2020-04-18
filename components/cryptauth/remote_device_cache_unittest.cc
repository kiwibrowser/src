// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_cache.h"

#include <algorithm>

#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cryptauth {

class RemoteDeviceCacheTest : public testing::Test {
 protected:
  RemoteDeviceCacheTest()
      : test_remote_device_list_(CreateRemoteDeviceListForTest(5)),
        test_remote_device_ref_list_(CreateRemoteDeviceRefListForTest(5)){};

  // testing::Test:
  void SetUp() override { cache_ = std::make_unique<RemoteDeviceCache>(); }

  void VerifyCacheRemoteDevices(
      RemoteDeviceRefList expected_remote_device_ref_list) {
    RemoteDeviceRefList remote_device_ref_list = cache_->GetRemoteDevices();
    std::sort(remote_device_ref_list.begin(), remote_device_ref_list.end(),
              [](const auto& device_1, const auto& device_2) {
                return device_1 < device_2;
              });

    EXPECT_EQ(expected_remote_device_ref_list, remote_device_ref_list);
  }

  const RemoteDeviceList test_remote_device_list_;
  const RemoteDeviceRefList test_remote_device_ref_list_;
  std::unique_ptr<RemoteDeviceCache> cache_;

  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceCacheTest);
};

TEST_F(RemoteDeviceCacheTest, TestNoRemoteDevices) {
  VerifyCacheRemoteDevices(RemoteDeviceRefList());
  EXPECT_EQ(base::nullopt, cache_->GetRemoteDevice(
                               test_remote_device_ref_list_[0].GetDeviceId()));
}

TEST_F(RemoteDeviceCacheTest, TestSetAndGetRemoteDevices) {
  cache_->SetRemoteDevices(test_remote_device_list_);

  VerifyCacheRemoteDevices(test_remote_device_ref_list_);
  EXPECT_EQ(
      test_remote_device_ref_list_[0],
      cache_->GetRemoteDevice(test_remote_device_ref_list_[0].GetDeviceId()));
}

TEST_F(RemoteDeviceCacheTest,
       TestSetRemoteDevices_RemoteDeviceRefsRemainValidAfterCacheRemoval) {
  cache_->SetRemoteDevices(test_remote_device_list_);

  VerifyCacheRemoteDevices(test_remote_device_ref_list_);

  cache_->SetRemoteDevices(RemoteDeviceList());

  VerifyCacheRemoteDevices(test_remote_device_ref_list_);
}

TEST_F(RemoteDeviceCacheTest,
       TestSetRemoteDevices_RemoteDeviceRefsRemainValidAfterCacheUpdate) {
  cryptauth::RemoteDevice remote_device =
      cryptauth::CreateRemoteDeviceForTest();
  cache_->SetRemoteDevices({remote_device});

  cryptauth::RemoteDeviceRef remote_device_ref =
      *cache_->GetRemoteDevice(remote_device.GetDeviceId());
  EXPECT_EQ(remote_device.name, remote_device_ref.name());

  remote_device.name = "new name";
  cache_->SetRemoteDevices({remote_device});

  EXPECT_EQ(remote_device.name, remote_device_ref.name());
}

// GetRemoteDevice

}  // namespace cryptauth