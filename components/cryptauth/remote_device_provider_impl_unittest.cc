// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_provider_impl.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/fake_cryptauth_device_manager.h"
#include "components/cryptauth/fake_secure_message_delegate.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_loader.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "components/cryptauth/secure_message_delegate_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;

namespace cryptauth {

namespace {

const char kTestUserId[] = "testUserId";
const char kTestUserPrivateKey[] = "kTestUserPrivateKey";

class FakeSecureMessageDelegateFactory
    : public cryptauth::SecureMessageDelegateImpl::Factory {
 public:
  // cryptauth::SecureMessageDelegateImpl::Factory:
  std::unique_ptr<cryptauth::SecureMessageDelegate> BuildInstance() override {
    cryptauth::FakeSecureMessageDelegate* delegate =
        new cryptauth::FakeSecureMessageDelegate();
    created_delegates_.push_back(delegate);
    return base::WrapUnique(delegate);
  }

 private:
  std::vector<cryptauth::FakeSecureMessageDelegate*> created_delegates_;
};

std::vector<cryptauth::ExternalDeviceInfo>
CreateExternalDeviceInfosForRemoteDevices(
    const cryptauth::RemoteDeviceList remote_devices) {
  std::vector<cryptauth::ExternalDeviceInfo> device_infos;
  for (const auto& remote_device : remote_devices) {
    // Add an ExternalDeviceInfo with the same public key as the RemoteDevice.
    cryptauth::ExternalDeviceInfo info;
    info.set_public_key(remote_device.public_key);
    device_infos.push_back(info);
  }
  return device_infos;
}

class TestObserver : public RemoteDeviceProvider::Observer {
 public:
  TestObserver() {}
  ~TestObserver() override {}

  int num_times_device_list_changed() { return num_times_device_list_changed_; }

  // RemoteDeviceProvider::Observer:
  void OnSyncDeviceListChanged() override { num_times_device_list_changed_++; }

 private:
  int num_times_device_list_changed_ = 0;
};

}  // namespace

class FakeDeviceLoader final : public cryptauth::RemoteDeviceLoader {
 public:
  class TestRemoteDeviceLoaderFactory final
      : public RemoteDeviceLoader::Factory {
   public:
    explicit TestRemoteDeviceLoaderFactory()
        : test_devices_(cryptauth::CreateRemoteDeviceListForTest(5)),
          test_device_infos_(
              CreateExternalDeviceInfosForRemoteDevices(test_devices_)) {}

    ~TestRemoteDeviceLoaderFactory() {}

    std::unique_ptr<RemoteDeviceLoader> BuildInstance(
        const std::vector<cryptauth::ExternalDeviceInfo>& device_info_list,
        const std::string& user_id,
        const std::string& user_private_key,
        std::unique_ptr<cryptauth::SecureMessageDelegate>
            secure_message_delegate) override {
      EXPECT_EQ(std::string(kTestUserId), user_id);
      EXPECT_EQ(std::string(kTestUserPrivateKey), user_private_key);
      std::unique_ptr<FakeDeviceLoader> device_loader =
          std::make_unique<FakeDeviceLoader>();
      device_loader->remote_device_loader_factory_ = this;
      return std::move(device_loader);
    }

    void InvokeLastCallback(
        const std::vector<cryptauth::ExternalDeviceInfo>& device_info_list) {
      ASSERT_TRUE(!callback_.is_null());
      // Fetch only the devices inserted by tests, since test_devices_ contains
      // all available devices.
      RemoteDeviceList devices;
      for (const auto remote_device : test_devices_) {
        for (const auto& external_device_info : device_info_list) {
          if (remote_device.public_key == external_device_info.public_key())
            devices.push_back(remote_device);
        }
      }
      callback_.Run(devices);
      callback_.Reset();
    }

    // Fetch is only started if the change result passed to OnSyncFinished() is
    // CHANGED and sync is SUCCESS.
    bool HasQueuedCallback() { return !callback_.is_null(); }
    const cryptauth::RemoteDeviceList test_devices_;
    const std::vector<cryptauth::ExternalDeviceInfo> test_device_infos_;

    void QueueCallback(const RemoteDeviceCallback& callback) {
      callback_ = callback;
    }

   private:
    cryptauth::RemoteDeviceLoader::RemoteDeviceCallback callback_;
  };

  FakeDeviceLoader()
      : cryptauth::RemoteDeviceLoader(
            std::vector<cryptauth::ExternalDeviceInfo>(),
            "",
            "",
            nullptr) {}

  ~FakeDeviceLoader() override {}

  TestRemoteDeviceLoaderFactory* remote_device_loader_factory_;

  void Load(bool should_load_beacon_seeds,
            const RemoteDeviceCallback& callback) override {
    remote_device_loader_factory_->QueueCallback(callback);
  }
};

class RemoteDeviceProviderImplTest : public testing::Test {
 public:
  RemoteDeviceProviderImplTest() {}

  void SetUp() override {
    fake_device_manager_ = std::make_unique<FakeCryptAuthDeviceManager>();
    fake_secure_message_delegate_factory_ =
        std::make_unique<FakeSecureMessageDelegateFactory>();
    cryptauth::SecureMessageDelegateImpl::Factory::SetInstanceForTesting(
        fake_secure_message_delegate_factory_.get());
    test_device_loader_factory_ =
        std::make_unique<FakeDeviceLoader::TestRemoteDeviceLoaderFactory>();
    cryptauth::RemoteDeviceLoader::Factory::SetInstanceForTesting(
        test_device_loader_factory_.get());
    test_observer_ = std::make_unique<TestObserver>();
  }

  void TearDown() override {
    cryptauth::SecureMessageDelegateImpl::Factory::SetInstanceForTesting(
        nullptr);
  }

  void CreateRemoteDeviceProvider() {
    remote_device_provider_ = std::make_unique<RemoteDeviceProviderImpl>(
        fake_device_manager_.get(), kTestUserId, kTestUserPrivateKey);
    remote_device_provider_->AddObserver(test_observer_.get());
    EXPECT_EQ(0u, remote_device_provider_->GetSyncedDevices().size());
    test_device_loader_factory_->InvokeLastCallback(
        fake_device_manager_->GetSyncedDevices());
    VerifySyncedDevicesMatchExpectation(
        fake_device_manager_->GetSyncedDevices().size());
  }

  void VerifySyncedDevicesMatchExpectation(size_t expected_size) {
    cryptauth::RemoteDeviceList synced_devices =
        remote_device_provider_->GetSyncedDevices();
    EXPECT_EQ(expected_size, synced_devices.size());
    EXPECT_EQ(expected_size, fake_device_manager_->GetSyncedDevices().size());
    std::unordered_set<std::string> public_keys;
    for (const auto& device_info : fake_device_manager_->GetSyncedDevices()) {
      public_keys.insert(device_info.public_key());
    }
    for (const auto& remote_device : synced_devices) {
      EXPECT_TRUE(public_keys.find(remote_device.public_key) !=
                  public_keys.end());
    }
  }

  cryptauth::RemoteDeviceList test_devices() {
    return test_device_loader_factory_->test_devices_;
  }

  ExternalDeviceInfo test_device_infos_(int val) {
    return test_device_loader_factory_->test_device_infos_[val];
  }

  std::unique_ptr<FakeSecureMessageDelegateFactory>
      fake_secure_message_delegate_factory_;

  std::unique_ptr<FakeCryptAuthDeviceManager> fake_device_manager_;

  std::unique_ptr<FakeDeviceLoader::TestRemoteDeviceLoaderFactory>
      test_device_loader_factory_;

  std::unique_ptr<RemoteDeviceProviderImpl> remote_device_provider_;

  std::unique_ptr<TestObserver> test_observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceProviderImplTest);
};

TEST_F(RemoteDeviceProviderImplTest, TestMultipleSyncs) {
  // Initialize with devices 0 and 1.
  fake_device_manager_->synced_devices().push_back(test_device_infos_(0));
  fake_device_manager_->synced_devices().push_back(test_device_infos_(1));
  CreateRemoteDeviceProvider();
  VerifySyncedDevicesMatchExpectation(2u /* expected_size */);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());

  // Now add device 2 and trigger another sync.
  fake_device_manager_->synced_devices().push_back(test_device_infos_(2));
  fake_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  test_device_loader_factory_->InvokeLastCallback(
      fake_device_manager_->GetSyncedDevices());
  VerifySyncedDevicesMatchExpectation(3u /* expected_size */);
  EXPECT_EQ(2, test_observer_->num_times_device_list_changed());

  // Now, simulate a sync which shows that device 0 was removed.
  fake_device_manager_->synced_devices().erase(
      fake_device_manager_->synced_devices().begin());
  fake_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  test_device_loader_factory_->InvokeLastCallback(
      fake_device_manager_->GetSyncedDevices());
  VerifySyncedDevicesMatchExpectation(2u /* expected_size */);
  EXPECT_EQ(3, test_observer_->num_times_device_list_changed());
}

TEST_F(RemoteDeviceProviderImplTest,
       TestNotifySyncFinishedParameterCombinations) {
  fake_device_manager_->synced_devices().push_back(test_device_infos_(0));
  CreateRemoteDeviceProvider();
  VerifySyncedDevicesMatchExpectation(1u /* expected_size */);
  fake_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::FAILURE,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  EXPECT_FALSE(test_device_loader_factory_->HasQueuedCallback());
  VerifySyncedDevicesMatchExpectation(1u /* expected_size */);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());

  fake_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::UNCHANGED);
  EXPECT_FALSE(test_device_loader_factory_->HasQueuedCallback());
  VerifySyncedDevicesMatchExpectation(1u /* expected_size */);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());

  fake_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::FAILURE,
      CryptAuthDeviceManager::DeviceChangeResult::UNCHANGED);
  EXPECT_FALSE(test_device_loader_factory_->HasQueuedCallback());
  VerifySyncedDevicesMatchExpectation(1u /* expected_size */);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());

  fake_device_manager_->synced_devices().push_back(test_device_infos_(1));
  fake_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  test_device_loader_factory_->InvokeLastCallback(
      fake_device_manager_->GetSyncedDevices());
  VerifySyncedDevicesMatchExpectation(2u /* expected_size */);
  EXPECT_EQ(2, test_observer_->num_times_device_list_changed());
}

TEST_F(RemoteDeviceProviderImplTest, TestNewSyncDuringDeviceRegeneration) {
  fake_device_manager_->synced_devices().push_back(test_device_infos_(0));
  CreateRemoteDeviceProvider();
  VerifySyncedDevicesMatchExpectation(1u /* expected_size */);

  // Add device 1 and trigger a sync.
  fake_device_manager_->synced_devices().push_back(test_device_infos_(1));
  fake_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());

  // Do not wait for the new devices to be generated (i.e., don't call
  // test_device_loader_factory_->InvokeLastCallback() yet). Trigger a new
  // sync with device 2 included.
  fake_device_manager_->synced_devices().push_back(test_device_infos_(2));
  fake_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  test_device_loader_factory_->InvokeLastCallback(
      fake_device_manager_->GetSyncedDevices());
  VerifySyncedDevicesMatchExpectation(3u /* expected_size */);
  EXPECT_EQ(2, test_observer_->num_times_device_list_changed());
}

TEST_F(RemoteDeviceProviderImplTest, TestZeroSyncedDevices) {
  CreateRemoteDeviceProvider();
  VerifySyncedDevicesMatchExpectation(0 /* expected_size */);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());
  fake_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::UNCHANGED);
  EXPECT_FALSE(test_device_loader_factory_->HasQueuedCallback());
  VerifySyncedDevicesMatchExpectation(0 /* expected_size */);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());
}

}  // namespace cryptauth
