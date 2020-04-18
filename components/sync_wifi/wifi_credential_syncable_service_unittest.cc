// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/wifi_credential_syncable_service.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "components/sync/model/fake_sync_change_processor.h"
#include "components/sync/model/sync_change.h"
#include "components/sync/model/sync_data.h"
#include "components/sync/model/sync_error.h"
#include "components/sync/model/sync_error_factory_mock.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync_wifi/wifi_config_delegate.h"
#include "components/sync_wifi/wifi_credential.h"
#include "components/sync_wifi/wifi_security_class.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_wifi {

using syncer::FakeSyncChangeProcessor;
using syncer::SyncErrorFactoryMock;

namespace {

const char kSsid[] = "fake-ssid";
const char kSsidNonUtf8[] = "\xc0";

// Fake implementation of WifiConfigDelegate, which provides the
// ability to check how many times a WifiConfigDelegate method has
// been called.
class FakeWifiConfigDelegate : public WifiConfigDelegate {
 public:
  FakeWifiConfigDelegate() : add_count_(0) {}
  ~FakeWifiConfigDelegate() override {}

  // WifiConfigDelegate implementation.
  void AddToLocalNetworks(const WifiCredential& network_credential) override {
    ++add_count_;
  }

  // Returns the number of times the AddToLocalNetworks method has
  // been called.
  unsigned int add_count() const { return add_count_; }

 private:
  // The number of times AddToLocalNetworks has been called on this fake.
  unsigned int add_count_;

  DISALLOW_COPY_AND_ASSIGN(FakeWifiConfigDelegate);
};

// Unit tests for WifiCredentialSyncableService.
class WifiCredentialSyncableServiceTest : public testing::Test {
 protected:
  WifiCredentialSyncableServiceTest()
      : config_delegate_(new FakeWifiConfigDelegate()),
        change_processor_(nullptr) {
    syncable_service_.reset(
        new WifiCredentialSyncableService(base::WrapUnique(config_delegate_)));
  }

  // Wrappers for methods in WifiCredentialSyncableService.
  syncer::SyncError ProcessSyncChanges(
      const syncer::SyncChangeList& change_list) {
    return syncable_service_->ProcessSyncChanges(FROM_HERE, change_list);
  }
  bool AddToSyncedNetworks(const std::string& item_id,
                           const WifiCredential& credential) {
    return syncable_service_->AddToSyncedNetworks(item_id, credential);
  }

  // Returns the number of change requests received by
  // |change_processor_|.
  int change_processor_changes_size() {
    CHECK(change_processor_);
    return change_processor_->changes().size();
  }

  // Returns the number of times AddToLocalNetworks has been called on
  // |config_delegate_|.
  unsigned int config_delegate_add_count() {
    return config_delegate_->add_count();
  }

  // Returns a new WifiCredential constructed from the given parameters.
  WifiCredential MakeCredential(const std::string& ssid,
                                WifiSecurityClass security_class,
                                const std::string& passphrase) {
    std::unique_ptr<WifiCredential> credential = WifiCredential::Create(
        WifiCredential::MakeSsidBytesForTest(ssid), security_class, passphrase);
    CHECK(credential);
    return *credential;
  }

  // Returns a new EntitySpecifics protobuf, with the
  // wifi_credential_specifics fields populated with the given
  // parameters.
  sync_pb::EntitySpecifics MakeWifiCredentialSpecifics(
      const std::string& ssid,
      sync_pb::WifiCredentialSpecifics_SecurityClass security_class,
      const std::string* passphrase) {
    const std::vector<uint8_t> ssid_bytes(ssid.begin(), ssid.end());
    sync_pb::EntitySpecifics sync_entity_specifics;
    sync_pb::WifiCredentialSpecifics* wifi_credential_specifics =
        sync_entity_specifics.mutable_wifi_credential();
    CHECK(wifi_credential_specifics);
    wifi_credential_specifics->set_ssid(ssid_bytes.data(), ssid_bytes.size());
    wifi_credential_specifics->set_security_class(security_class);
    if (passphrase) {
      wifi_credential_specifics->set_passphrase(passphrase->data(),
                                                passphrase->size());
    }
    return sync_entity_specifics;
  }

  syncer::SyncChange MakeActionAdd(
      int sync_item_id,
      const std::string& ssid,
      sync_pb::WifiCredentialSpecifics_SecurityClass security_class,
      const std::string* passphrase) {
    return syncer::SyncChange(
        FROM_HERE, syncer::SyncChange::ACTION_ADD,
        syncer::SyncData::CreateRemoteData(
            sync_item_id,
            MakeWifiCredentialSpecifics(ssid, security_class, passphrase),
            base::Time()));
  }

  void StartSyncing() {
    std::unique_ptr<FakeSyncChangeProcessor> change_processor(
        new FakeSyncChangeProcessor());
    change_processor_ = change_processor.get();
    syncable_service_->MergeDataAndStartSyncing(
        syncer::WIFI_CREDENTIALS, syncer::SyncDataList(),
        std::move(change_processor), std::make_unique<SyncErrorFactoryMock>());
  }

 private:
  std::unique_ptr<WifiCredentialSyncableService> syncable_service_;
  FakeWifiConfigDelegate* config_delegate_;    // Owned by |syncable_service_|
  FakeSyncChangeProcessor* change_processor_;  // Owned by |syncable_service_|

  DISALLOW_COPY_AND_ASSIGN(WifiCredentialSyncableServiceTest);
};

}  // namespace

TEST_F(WifiCredentialSyncableServiceTest, ProcessSyncChangesNotStarted) {
  syncer::SyncError sync_error(ProcessSyncChanges(syncer::SyncChangeList()));
  ASSERT_TRUE(sync_error.IsSet());
  EXPECT_EQ(syncer::SyncError::UNREADY_ERROR, sync_error.error_type());
  EXPECT_EQ(0U, config_delegate_add_count());
}

TEST_F(WifiCredentialSyncableServiceTest,
       ProcessSyncChangesActionAddSecurityClassInvalid) {
  syncer::SyncChangeList changes;
  const int sync_item_id = 1;
  changes.push_back(MakeActionAdd(
      sync_item_id, kSsidNonUtf8,
      sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_INVALID, nullptr));
  StartSyncing();
  syncer::SyncError sync_error = ProcessSyncChanges(changes);
  EXPECT_FALSE(sync_error.IsSet());  // Bad items are ignored.
  EXPECT_EQ(0U, config_delegate_add_count());
}

TEST_F(WifiCredentialSyncableServiceTest,
       ProcessSyncChangesActionAddSecurityClassNoneSuccess) {
  syncer::SyncChangeList changes;
  const int sync_item_id = 1;
  changes.push_back(MakeActionAdd(
      sync_item_id, kSsidNonUtf8,
      sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_NONE, nullptr));
  StartSyncing();
  syncer::SyncError sync_error = ProcessSyncChanges(changes);
  EXPECT_FALSE(sync_error.IsSet());
  EXPECT_EQ(1U, config_delegate_add_count());
}

TEST_F(WifiCredentialSyncableServiceTest,
       ProcessSyncChangesActionAddSecurityClassWepMissingPassphrase) {
  syncer::SyncChangeList changes;
  const int sync_item_id = 1;
  changes.push_back(MakeActionAdd(
      sync_item_id, kSsidNonUtf8,
      sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_WEP, nullptr));
  StartSyncing();
  syncer::SyncError sync_error = ProcessSyncChanges(changes);
  EXPECT_FALSE(sync_error.IsSet());  // Bad items are ignored.
  EXPECT_EQ(0U, config_delegate_add_count());
}

TEST_F(WifiCredentialSyncableServiceTest,
       ProcessSyncChangesActionAddSecurityClassWepSuccess) {
  syncer::SyncChangeList changes;
  const int sync_item_id = 1;
  const std::string passphrase("abcde");
  changes.push_back(MakeActionAdd(
      sync_item_id, kSsidNonUtf8,
      sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_WEP, &passphrase));
  StartSyncing();
  syncer::SyncError sync_error = ProcessSyncChanges(changes);
  EXPECT_FALSE(sync_error.IsSet());
  EXPECT_EQ(1U, config_delegate_add_count());
}

TEST_F(WifiCredentialSyncableServiceTest,
       ProcessSyncChangesActionAddSecurityClassPskMissingPassphrase) {
  syncer::SyncChangeList changes;
  const int sync_item_id = 1;
  changes.push_back(MakeActionAdd(
      sync_item_id, kSsidNonUtf8,
      sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_PSK, nullptr));
  StartSyncing();
  syncer::SyncError sync_error = ProcessSyncChanges(changes);
  EXPECT_FALSE(sync_error.IsSet());  // Bad items are ignored.
  EXPECT_EQ(0U, config_delegate_add_count());
}

TEST_F(WifiCredentialSyncableServiceTest,
       ProcessSyncChangesActionAddSecurityClassPskSuccess) {
  syncer::SyncChangeList changes;
  const int sync_item_id = 1;
  const std::string passphrase("psk-passphrase");
  changes.push_back(MakeActionAdd(
      sync_item_id, kSsidNonUtf8,
      sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_PSK, &passphrase));
  StartSyncing();
  syncer::SyncError sync_error = ProcessSyncChanges(changes);
  EXPECT_FALSE(sync_error.IsSet());
  EXPECT_EQ(1U, config_delegate_add_count());
}

TEST_F(WifiCredentialSyncableServiceTest,
       ProcessSyncChangesContinuesAfterSecurityClassInvalid) {
  syncer::SyncChangeList changes;
  changes.push_back(MakeActionAdd(
      1 /* sync item id */, kSsidNonUtf8,
      sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_INVALID, nullptr));
  changes.push_back(MakeActionAdd(
      2 /* sync item id */, kSsidNonUtf8,
      sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_NONE, nullptr));
  StartSyncing();
  syncer::SyncError sync_error = ProcessSyncChanges(changes);
  EXPECT_FALSE(sync_error.IsSet());  // Bad items are ignored.
  EXPECT_EQ(1U, config_delegate_add_count());
}

TEST_F(WifiCredentialSyncableServiceTest,
       ProcessSyncChangesContinuesAfterMissingPassphrase) {
  syncer::SyncChangeList changes;
  changes.push_back(MakeActionAdd(
      1 /* sync item id */, kSsidNonUtf8,
      sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_WEP, nullptr));
  changes.push_back(MakeActionAdd(
      2 /* sync item id */, kSsidNonUtf8,
      sync_pb::WifiCredentialSpecifics::SECURITY_CLASS_NONE, nullptr));
  StartSyncing();
  syncer::SyncError sync_error = ProcessSyncChanges(changes);
  EXPECT_FALSE(sync_error.IsSet());  // Bad items are ignored.
  EXPECT_EQ(1U, config_delegate_add_count());
}

TEST_F(WifiCredentialSyncableServiceTest, AddToSyncedNetworksNotStarted) {
  EXPECT_FALSE(AddToSyncedNetworks(
      "fake-item-id", MakeCredential(kSsidNonUtf8, SECURITY_CLASS_NONE, "")));
}

TEST_F(WifiCredentialSyncableServiceTest, AddToSyncedNetworksSuccess) {
  StartSyncing();
  EXPECT_TRUE(AddToSyncedNetworks(
      "fake-item-id", MakeCredential(kSsidNonUtf8, SECURITY_CLASS_NONE, "")));
  EXPECT_EQ(1, change_processor_changes_size());
}

TEST_F(WifiCredentialSyncableServiceTest,
       AddToSyncedNetworksDifferentSecurityClassesSuccess) {
  StartSyncing();
  EXPECT_TRUE(AddToSyncedNetworks(
      "fake-item-id", MakeCredential(kSsidNonUtf8, SECURITY_CLASS_NONE, "")));
  EXPECT_TRUE(AddToSyncedNetworks(
      "fake-item-id-2", MakeCredential(kSsidNonUtf8, SECURITY_CLASS_WEP, "")));
  EXPECT_EQ(2, change_processor_changes_size());
}

TEST_F(WifiCredentialSyncableServiceTest,
       AddToSyncedNetworksDifferentSsidsSuccess) {
  StartSyncing();
  EXPECT_TRUE(AddToSyncedNetworks(
      "fake-item-id", MakeCredential(kSsidNonUtf8, SECURITY_CLASS_NONE, "")));
  EXPECT_TRUE(AddToSyncedNetworks(
      "fake-item-id-2", MakeCredential(kSsid, SECURITY_CLASS_NONE, "")));
  EXPECT_EQ(2, change_processor_changes_size());
}

TEST_F(WifiCredentialSyncableServiceTest,
       AddToSyncedNetworksDuplicateAddPskNetwork) {
  const std::string passphrase("psk-passphrase");
  StartSyncing();
  EXPECT_TRUE(AddToSyncedNetworks(
      "fake-item-id",
      MakeCredential(kSsidNonUtf8, SECURITY_CLASS_PSK, passphrase)));
  EXPECT_EQ(1, change_processor_changes_size());
  EXPECT_FALSE(AddToSyncedNetworks(
      "fake-item-id",
      MakeCredential(kSsidNonUtf8, SECURITY_CLASS_PSK, passphrase)));
  EXPECT_EQ(1, change_processor_changes_size());
}

TEST_F(WifiCredentialSyncableServiceTest,
       AddToSyncedNetworksDuplicateAddOpenNetwork) {
  StartSyncing();
  EXPECT_TRUE(AddToSyncedNetworks(
      "fake-item-id", MakeCredential(kSsidNonUtf8, SECURITY_CLASS_NONE, "")));
  EXPECT_EQ(1, change_processor_changes_size());
  EXPECT_FALSE(AddToSyncedNetworks(
      "fake-item-id", MakeCredential(kSsidNonUtf8, SECURITY_CLASS_NONE, "")));
  EXPECT_EQ(1, change_processor_changes_size());
}

}  // namespace sync_wifi
