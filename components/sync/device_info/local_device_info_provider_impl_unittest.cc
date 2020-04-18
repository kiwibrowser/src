// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/device_info/local_device_info_provider_impl.h"

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/sync/base/get_session_name.h"
#include "components/version_info/version_string.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

const char kLocalDeviceGuid[] = "foo";
const char kSigninScopedDeviceId[] = "device_id";

class LocalDeviceInfoProviderImplTest : public testing::Test {
 public:
  LocalDeviceInfoProviderImplTest() : called_back_(false) {}
  ~LocalDeviceInfoProviderImplTest() override {}

  void SetUp() override {
    provider_ = std::make_unique<LocalDeviceInfoProviderImpl>(
        version_info::Channel::UNKNOWN,
        version_info::GetVersionStringWithModifier("UNKNOWN"), false);
  }

  void TearDown() override {
    provider_.reset();
    called_back_ = false;
  }

 protected:
  void StartInitializeProvider() { StartInitializeProvider(kLocalDeviceGuid); }

  void StartInitializeProvider(const std::string& guid) {
    provider_->Initialize(guid, kSigninScopedDeviceId);
  }

  void FinishInitializeProvider() {
    // Subscribe to the notification and wait until the callback
    // is called. The callback will quit the loop.
    base::RunLoop run_loop;
    std::unique_ptr<LocalDeviceInfoProvider::Subscription> subscription =
        provider_->RegisterOnInitializedCallback(
            base::Bind(&LocalDeviceInfoProviderImplTest::QuitLoopOnInitialized,
                       base::Unretained(this), &run_loop));
    run_loop.Run();
  }

  void InitializeProvider() {
    StartInitializeProvider();
    FinishInitializeProvider();
  }

  void QuitLoopOnInitialized(base::RunLoop* loop) {
    called_back_ = true;
    loop->Quit();
  }

  std::unique_ptr<LocalDeviceInfoProviderImpl> provider_;

  bool called_back_;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(LocalDeviceInfoProviderImplTest, OnInitializedCallback) {
  ASSERT_FALSE(called_back_);
  StartInitializeProvider();
  ASSERT_FALSE(called_back_);
  FinishInitializeProvider();
  EXPECT_TRUE(called_back_);
}

TEST_F(LocalDeviceInfoProviderImplTest, GetLocalDeviceInfo) {
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
  StartInitializeProvider();
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
  FinishInitializeProvider();

  const DeviceInfo* local_device_info = provider_->GetLocalDeviceInfo();
  ASSERT_NE(nullptr, local_device_info);
  EXPECT_EQ(std::string(kLocalDeviceGuid), local_device_info->guid());
  EXPECT_EQ(std::string(kSigninScopedDeviceId),
            local_device_info->signin_scoped_device_id());
  EXPECT_EQ(GetSessionNameSynchronouslyForTesting(),
            local_device_info->client_name());

  EXPECT_EQ(provider_->GetSyncUserAgent(),
            local_device_info->sync_user_agent());

  provider_->Clear();
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
}

TEST_F(LocalDeviceInfoProviderImplTest, GetLocalSyncCacheGUID) {
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());

  StartInitializeProvider();
  EXPECT_EQ(std::string(kLocalDeviceGuid), provider_->GetLocalSyncCacheGUID());

  FinishInitializeProvider();
  EXPECT_EQ(std::string(kLocalDeviceGuid), provider_->GetLocalSyncCacheGUID());

  provider_->Clear();
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());
}

TEST_F(LocalDeviceInfoProviderImplTest, InitClearRace) {
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());
  StartInitializeProvider();

  provider_->Clear();
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());

  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());
}

TEST_F(LocalDeviceInfoProviderImplTest, InitClearInitRace) {
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());
  StartInitializeProvider();
  provider_->Clear();

  const std::string guid2 = "guid2";
  StartInitializeProvider(guid2);
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
  EXPECT_EQ(guid2, provider_->GetLocalSyncCacheGUID());

  FinishInitializeProvider();
  const DeviceInfo* local_device_info = provider_->GetLocalDeviceInfo();
  ASSERT_NE(nullptr, local_device_info);
  EXPECT_EQ(guid2, local_device_info->guid());
  EXPECT_EQ(guid2, provider_->GetLocalSyncCacheGUID());
}

}  // namespace syncer
