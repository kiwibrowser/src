// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/device_info/device_info_sync_bridge.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "components/sync/base/time.h"
#include "components/sync/device_info/local_device_info_provider_mock.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/data_type_error_handler_mock.h"
#include "components/sync/model/entity_data.h"
#include "components/sync/model/fake_model_type_change_processor.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/mock_model_type_change_processor.h"
#include "components/sync/model/model_type_store_test_util.h"
#include "components/sync/protocol/model_type_state.pb.h"
#include "components/sync/test/test_matchers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace {

using base::OneShotTimer;
using sync_pb::DeviceInfoSpecifics;
using sync_pb::EntitySpecifics;
using sync_pb::ModelTypeState;
using testing::_;
using testing::IsEmpty;
using testing::Matcher;
using testing::NotNull;
using testing::Pair;
using testing::UnorderedElementsAre;

using DeviceInfoList = std::vector<std::unique_ptr<DeviceInfo>>;
using StorageKeyList = ModelTypeSyncBridge::StorageKeyList;
using RecordList = ModelTypeStore::RecordList;
using StartCallback = ModelTypeControllerDelegate::StartCallback;
using WriteBatch = ModelTypeStore::WriteBatch;

const char kGuidFormat[] = "cache guid %d";
const char kClientNameFormat[] = "client name %d";
const char kChromeVersionFormat[] = "chrome version %d";
const char kSyncUserAgentFormat[] = "sync user agent %d";
const char kSigninScopedDeviceIdFormat[] = "signin scoped device id %d";
const sync_pb::SyncEnums::DeviceType kDeviceType =
    sync_pb::SyncEnums_DeviceType_TYPE_LINUX;

// The |provider_| is first initialized with a model object created with this
// suffix. Local suffix can be changed by setting the provider and then
// initializing. Remote data should use other suffixes.
const int kDefaultLocalSuffix = 0;

MATCHER_P(HasDeviceInfo, expected, "") {
  return arg.device_info().SerializeAsString() == expected.SerializeAsString();
}

MATCHER_P(EqualsProto, expected, "") {
  return arg.SerializeAsString() == expected.SerializeAsString();
}

MATCHER_P(ModelEqualsSpecifics, expected_specifics, "") {
  return expected_specifics.cache_guid() == arg.guid() &&
         expected_specifics.client_name() == arg.client_name() &&
         expected_specifics.device_type() == arg.device_type() &&
         expected_specifics.sync_user_agent() == arg.sync_user_agent() &&
         expected_specifics.chrome_version() == arg.chrome_version() &&
         expected_specifics.signin_scoped_device_id() ==
             arg.signin_scoped_device_id();
}

Matcher<std::unique_ptr<EntityData>> HasSpecifics(
    const Matcher<sync_pb::EntitySpecifics>& m) {
  return testing::Pointee(testing::Field(&EntityData::specifics, m));
}

MATCHER(HasLastUpdatedAboutNow, "") {
  const sync_pb::DeviceInfoSpecifics& specifics = arg.device_info();
  const base::Time now = base::Time::Now();
  const base::TimeDelta tolerance = base::TimeDelta::FromMinutes(1);
  const base::Time actual_last_updated =
      ProtoTimeToTime(specifics.last_updated_timestamp());

  if (actual_last_updated < now - tolerance) {
    *result_listener << "which is too far in the past";
    return false;
  }
  if (actual_last_updated > now + tolerance) {
    *result_listener << "which is too far in the future";
    return false;
  }
  return true;
}

DeviceInfoSpecifics CreateSpecifics(
    int suffix,
    base::Time last_updated = base::Time::Now()) {
  DeviceInfoSpecifics specifics;
  specifics.set_cache_guid(base::StringPrintf(kGuidFormat, suffix));
  specifics.set_client_name(base::StringPrintf(kClientNameFormat, suffix));
  specifics.set_device_type(sync_pb::SyncEnums_DeviceType_TYPE_LINUX);
  specifics.set_sync_user_agent(
      base::StringPrintf(kSyncUserAgentFormat, suffix));
  specifics.set_chrome_version(
      base::StringPrintf(kChromeVersionFormat, suffix));
  specifics.set_signin_scoped_device_id(
      base::StringPrintf(kSigninScopedDeviceIdFormat, suffix));
  specifics.set_last_updated_timestamp(TimeToProtoTime(last_updated));
  return specifics;
}

std::unique_ptr<DeviceInfo> CreateModel(int suffix) {
  return std::make_unique<DeviceInfo>(
      base::StringPrintf(kGuidFormat, suffix),
      base::StringPrintf(kClientNameFormat, suffix),
      base::StringPrintf(kChromeVersionFormat, suffix),
      base::StringPrintf(kSyncUserAgentFormat, suffix), kDeviceType,
      base::StringPrintf(kSigninScopedDeviceIdFormat, suffix));
}

ModelTypeState StateWithEncryption(const std::string& encryption_key_name) {
  ModelTypeState state;
  state.set_encryption_key_name(encryption_key_name);
  return state;
}

// Creates an EntityData/EntityDataPtr around a copy of the given specifics.
EntityDataPtr SpecificsToEntity(const DeviceInfoSpecifics& specifics) {
  EntityData data;
  // These tests do not care about the tag hash, but EntityData and friends
  // cannot differentiate between the default EntityData object if the hash
  // is unset, which causes pass/copy operations to no-op and things start to
  // break, so we throw in a junk value and forget about it.
  data.client_tag_hash = "junk";
  *data.specifics.mutable_device_info() = specifics;
  return data.PassToPtr();
}

std::string CacheGuidToTag(const std::string& guid) {
  return "DeviceInfo_" + guid;
}

// Helper method to reduce duplicated code between tests. Wraps the given
// specifics objects in an EntityData and EntityChange of type ACTION_ADD, and
// returns an EntityChangeList containing them all. Order is maintained.
EntityChangeList EntityAddList(
    const std::vector<DeviceInfoSpecifics>& specifics_list) {
  EntityChangeList changes;
  for (const auto& specifics : specifics_list) {
    changes.push_back(EntityChange::CreateAdd(specifics.cache_guid(),
                                              SpecificsToEntity(specifics)));
  }
  return changes;
}

std::map<std::string, sync_pb::EntitySpecifics> DataBatchToSpecificsMap(
    std::unique_ptr<DataBatch> batch) {
  std::map<std::string, sync_pb::EntitySpecifics> storage_key_to_specifics;
  while (batch && batch->HasNext()) {
    const syncer::KeyAndData& pair = batch->Next();
    storage_key_to_specifics[pair.first] = pair.second->specifics;
  }
  return storage_key_to_specifics;
}

class DeviceInfoSyncBridgeTest : public testing::Test,
                                 public DeviceInfoTracker::Observer {
 protected:
  DeviceInfoSyncBridgeTest()
      : store_(ModelTypeStoreTestUtil::CreateInMemoryStoreForTest()),
        provider_(new LocalDeviceInfoProviderMock()) {
    provider_->Initialize(CreateModel(kDefaultLocalSuffix));

    ON_CALL(*processor(), IsTrackingMetadata())
        .WillByDefault(testing::Return(true));
  }

  ~DeviceInfoSyncBridgeTest() override {
    // Some tests may never initialize the bridge.
    if (bridge_)
      bridge_->RemoveObserver(this);

    // Force all remaining (store) tasks to execute so we don't leak memory.
    base::RunLoop().RunUntilIdle();
  }

  void OnDeviceInfoChange() override { change_count_++; }

  // Initialized the bridge based on the current local device and store. Can
  // only be called once per run, as it passes |store_|.
  void InitializeBridge() {
    ASSERT_TRUE(store_);
    bridge_ = std::make_unique<DeviceInfoSyncBridge>(
        provider_.get(),
        base::BindOnce(&ModelTypeStoreTestUtil::MoveStoreToCallback,
                       std::move(store_)),
        mock_processor_.CreateForwardingProcessor());
    bridge_->AddObserver(this);
  }

  // Creates the bridge and runs any outstanding tasks. This will typically
  // cause all initialization callbacks between the sevice and store to fire.
  void InitializeAndPump() {
    InitializeBridge();
    base::RunLoop().RunUntilIdle();
  }

  // Allows access to the store before that will ultimately be used to
  // initialize the bridge.
  ModelTypeStore* store() {
    EXPECT_TRUE(store_);
    return store_.get();
  }

  // Get the number of times the bridge notifies observers of changes.
  int change_count() { return change_count_; }

  // Allows overriding the provider before the bridge is initialized.
  void set_provider(std::unique_ptr<LocalDeviceInfoProviderMock> provider) {
    ASSERT_FALSE(bridge_);
    std::swap(provider_, provider);
  }
  LocalDeviceInfoProviderMock* local_device() { return provider_.get(); }

  // Allows access to the bridge after InitializeBridge() is called.
  DeviceInfoSyncBridge* bridge() {
    EXPECT_TRUE(bridge_);
    return bridge_.get();
  }

  MockModelTypeChangeProcessor* processor() { return &mock_processor_; }
  // Should only be called after the bridge has been initialized. Will first
  // recover the bridge's store, so another can be initialized later, and then
  // deletes the bridge.
  void PumpAndShutdown() {
    ASSERT_TRUE(bridge_);
    base::RunLoop().RunUntilIdle();
    bridge_->RemoveObserver(this);
    store_ =
        DeviceInfoSyncBridge::DestroyAndStealStoreForTest(std::move(bridge_));
  }

  void RestartBridge() {
    PumpAndShutdown();
    InitializeAndPump();
  }

  void ForcePulse() { bridge()->ForcePulseForTest(); }

  void WriteToStore(const std::vector<DeviceInfoSpecifics>& specifics_list,
                    std::unique_ptr<WriteBatch> batch) {
    // The bridge only reads from the store during initialization, so if the
    // |bridge_| has already been initialized, then it may not have an effect.
    EXPECT_FALSE(bridge_);
    for (auto& specifics : specifics_list) {
      batch->WriteData(specifics.cache_guid(), specifics.SerializeAsString());
    }
    base::RunLoop loop;
    store()->CommitWriteBatch(
        std::move(batch),
        base::BindOnce(
            [](base::RunLoop* loop, const base::Optional<ModelError>& result) {
              EXPECT_FALSE(result.has_value()) << result->ToString();
              loop->Quit();
            },
            &loop));
    loop.Run();
  }

  void WriteToStore(const std::vector<DeviceInfoSpecifics>& specifics_list,
                    ModelTypeState state) {
    std::unique_ptr<WriteBatch> batch = store()->CreateWriteBatch();
    batch->GetMetadataChangeList()->UpdateModelTypeState(state);
    WriteToStore(specifics_list, std::move(batch));
  }

  void WriteToStore(const std::vector<DeviceInfoSpecifics>& specifics_list) {
    WriteToStore(specifics_list, store()->CreateWriteBatch());
  }

  std::map<std::string, sync_pb::EntitySpecifics> GetAllData() {
    base::RunLoop loop;
    std::unique_ptr<DataBatch> batch;
    bridge_->GetAllData(base::BindOnce(
        [](base::RunLoop* loop, std::unique_ptr<DataBatch>* out_batch,
           std::unique_ptr<DataBatch> batch) {
          *out_batch = std::move(batch);
          loop->Quit();
        },
        &loop, &batch));
    loop.Run();
    EXPECT_NE(nullptr, batch);
    return DataBatchToSpecificsMap(std::move(batch));
  }

  std::map<std::string, sync_pb::EntitySpecifics> GetData(
      const std::vector<std::string>& storage_keys) {
    base::RunLoop loop;
    std::unique_ptr<DataBatch> batch;
    bridge_->GetData(storage_keys, base::BindOnce(
                                       [](base::RunLoop* loop,
                                          std::unique_ptr<DataBatch>* out_batch,
                                          std::unique_ptr<DataBatch> batch) {
                                         *out_batch = std::move(batch);
                                         loop->Quit();
                                       },
                                       &loop, &batch));
    loop.Run();
    EXPECT_NE(nullptr, batch);
    return DataBatchToSpecificsMap(std::move(batch));
  }

 private:
  int change_count_ = 0;

  // In memory model type store needs a MessageLoop.
  base::MessageLoop message_loop_;

  testing::NiceMock<MockModelTypeChangeProcessor> mock_processor_;

  // Holds the store while the bridge is not initialized.
  std::unique_ptr<ModelTypeStore> store_;

  // Provides information about the local device. Is initialized in each case's
  // constructor with a model object created from |kDefaultLocalSuffix|.
  std::unique_ptr<LocalDeviceInfoProviderMock> provider_;

  // Not initialized immediately (upon test's constructor). This allows each
  // test case to modify the dependencies the bridge will be constructed with.
  std::unique_ptr<DeviceInfoSyncBridge> bridge_;
};

TEST_F(DeviceInfoSyncBridgeTest, EmptyDataReconciliation) {
  InitializeAndPump();
  const DeviceInfoList devices = bridge()->GetAllDeviceInfo();
  ASSERT_EQ(1u, devices.size());
  EXPECT_TRUE(local_device()->GetLocalDeviceInfo()->Equals(*devices[0]));
}

TEST_F(DeviceInfoSyncBridgeTest, EmptyDataReconciliationSlowLoad) {
  InitializeBridge();
  EXPECT_EQ(0u, bridge()->GetAllDeviceInfo().size());
  base::RunLoop().RunUntilIdle();
  const DeviceInfoList devices = bridge()->GetAllDeviceInfo();
  ASSERT_EQ(1u, devices.size());
  EXPECT_TRUE(local_device()->GetLocalDeviceInfo()->Equals(*devices[0]));
}

TEST_F(DeviceInfoSyncBridgeTest, LocalProviderSubscription) {
  set_provider(std::make_unique<LocalDeviceInfoProviderMock>());
  InitializeAndPump();

  EXPECT_EQ(0u, bridge()->GetAllDeviceInfo().size());
  local_device()->Initialize(CreateModel(1));
  base::RunLoop().RunUntilIdle();

  const DeviceInfoList devices = bridge()->GetAllDeviceInfo();
  ASSERT_EQ(1u, devices.size());
  EXPECT_TRUE(local_device()->GetLocalDeviceInfo()->Equals(*devices[0]));
}

// Metadata shouldn't be loaded before the provider is initialized.
TEST_F(DeviceInfoSyncBridgeTest, LocalProviderInitRace) {
  EXPECT_CALL(*processor(), ModelReadyToSync(_)).Times(0);
  set_provider(std::make_unique<LocalDeviceInfoProviderMock>());
  InitializeAndPump();
  EXPECT_EQ(0u, bridge()->GetAllDeviceInfo().size());

  EXPECT_CALL(*processor(), ModelReadyToSync(_));
  local_device()->Initialize(CreateModel(1));
  base::RunLoop().RunUntilIdle();

  DeviceInfoList devices = bridge()->GetAllDeviceInfo();
  ASSERT_EQ(1u, devices.size());
  EXPECT_TRUE(local_device()->GetLocalDeviceInfo()->Equals(*devices[0]));
}

// Simulate shutting down sync during the ModelTypeStore callbacks. The pulse
// timer should still be initialized, even though reconcile never occurs.
TEST_F(DeviceInfoSyncBridgeTest, ClearProviderDuringInit) {
  InitializeBridge();
  local_device()->Clear();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0u, bridge()->GetAllDeviceInfo().size());
  EXPECT_TRUE(bridge()->IsPulseTimerRunningForTest());
}

TEST_F(DeviceInfoSyncBridgeTest, GetClientTagNormal) {
  InitializeBridge();
  const std::string guid = "abc";
  EntitySpecifics entity_specifics;
  entity_specifics.mutable_device_info()->set_cache_guid(guid);
  EntityData entity_data;
  entity_data.specifics = entity_specifics;
  EXPECT_EQ(CacheGuidToTag(guid), bridge()->GetClientTag(entity_data));
}

TEST_F(DeviceInfoSyncBridgeTest, GetClientTagEmpty) {
  InitializeBridge();
  EntitySpecifics entity_specifics;
  entity_specifics.mutable_device_info();
  EntityData entity_data;
  entity_data.specifics = entity_specifics;
  EXPECT_EQ(CacheGuidToTag(""), bridge()->GetClientTag(entity_data));
}

TEST_F(DeviceInfoSyncBridgeTest, TestWithLocalData) {
  const DeviceInfoSpecifics specifics = CreateSpecifics(1);
  WriteToStore({specifics});
  InitializeAndPump();

  ASSERT_EQ(2u, bridge()->GetAllDeviceInfo().size());
  EXPECT_THAT(*bridge()->GetDeviceInfo(specifics.cache_guid()),
              ModelEqualsSpecifics(specifics));
}

TEST_F(DeviceInfoSyncBridgeTest, TestWithLocalMetadata) {
  WriteToStore(std::vector<DeviceInfoSpecifics>(), StateWithEncryption("ekn"));

  EXPECT_CALL(*processor(), Put(_, _, _));
  InitializeAndPump();

  const DeviceInfoList devices = bridge()->GetAllDeviceInfo();
  ASSERT_EQ(1u, devices.size());
  EXPECT_TRUE(local_device()->GetLocalDeviceInfo()->Equals(*devices[0]));
}

TEST_F(DeviceInfoSyncBridgeTest, TestWithLocalDataAndMetadata) {
  const DeviceInfoSpecifics specifics = CreateSpecifics(1);
  ModelTypeState state = StateWithEncryption("ekn");
  WriteToStore({specifics}, state);

  EXPECT_CALL(*processor(),
              ModelReadyToSync(MetadataBatchContains(
                  HasEncryptionKeyName(state.encryption_key_name()),
                  /*entities=*/IsEmpty())));
  InitializeAndPump();

  ASSERT_EQ(2u, bridge()->GetAllDeviceInfo().size());
  EXPECT_THAT(*bridge()->GetDeviceInfo(specifics.cache_guid()),
              ModelEqualsSpecifics(specifics));
}

TEST_F(DeviceInfoSyncBridgeTest, GetData) {
  const DeviceInfoSpecifics specifics1 = CreateSpecifics(1);
  const DeviceInfoSpecifics specifics2 = CreateSpecifics(2);
  const DeviceInfoSpecifics specifics3 = CreateSpecifics(3);
  WriteToStore({specifics1, specifics2, specifics3});
  InitializeAndPump();

  EXPECT_THAT(GetData({specifics1.cache_guid()}),
              UnorderedElementsAre(
                  Pair(specifics1.cache_guid(), HasDeviceInfo(specifics1))));

  EXPECT_THAT(GetData({specifics1.cache_guid(), specifics3.cache_guid()}),
              UnorderedElementsAre(
                  Pair(specifics1.cache_guid(), HasDeviceInfo(specifics1)),
                  Pair(specifics3.cache_guid(), HasDeviceInfo(specifics3))));

  EXPECT_THAT(GetData({specifics1.cache_guid(), specifics2.cache_guid(),
                       specifics3.cache_guid()}),
              UnorderedElementsAre(
                  Pair(specifics1.cache_guid(), HasDeviceInfo(specifics1)),
                  Pair(specifics2.cache_guid(), HasDeviceInfo(specifics2)),
                  Pair(specifics3.cache_guid(), HasDeviceInfo(specifics3))));
}

TEST_F(DeviceInfoSyncBridgeTest, GetDataMissing) {
  InitializeAndPump();
  EXPECT_THAT(GetData({"does_not_exist"}), IsEmpty());
}

TEST_F(DeviceInfoSyncBridgeTest, GetAllData) {
  const DeviceInfoSpecifics specifics1 = CreateSpecifics(1);
  const DeviceInfoSpecifics specifics2 = CreateSpecifics(2);
  WriteToStore({specifics1, specifics2});
  InitializeAndPump();

  EXPECT_THAT(GetAllData(),
              UnorderedElementsAre(
                  Pair(local_device()->GetLocalDeviceInfo()->guid(), _),
                  Pair(specifics1.cache_guid(), HasDeviceInfo(specifics1)),
                  Pair(specifics2.cache_guid(), HasDeviceInfo(specifics2))));
}

TEST_F(DeviceInfoSyncBridgeTest, ApplySyncChangesEmpty) {
  InitializeAndPump();
  ASSERT_EQ(1, change_count());

  auto error = bridge()->ApplySyncChanges(bridge()->CreateMetadataChangeList(),
                                          EntityChangeList());
  EXPECT_FALSE(error);
  EXPECT_EQ(1, change_count());
}

TEST_F(DeviceInfoSyncBridgeTest, ApplySyncChangesInMemory) {
  InitializeAndPump();
  ASSERT_EQ(1, change_count());

  const DeviceInfoSpecifics specifics = CreateSpecifics(1);
  auto error_on_add = bridge()->ApplySyncChanges(
      bridge()->CreateMetadataChangeList(), EntityAddList({specifics}));

  EXPECT_FALSE(error_on_add);
  std::unique_ptr<DeviceInfo> info =
      bridge()->GetDeviceInfo(specifics.cache_guid());
  ASSERT_TRUE(info);
  EXPECT_THAT(*info, ModelEqualsSpecifics(specifics));
  EXPECT_EQ(2, change_count());

  auto error_on_delete = bridge()->ApplySyncChanges(
      bridge()->CreateMetadataChangeList(),
      {EntityChange::CreateDelete(specifics.cache_guid())});

  EXPECT_FALSE(error_on_delete);
  EXPECT_FALSE(bridge()->GetDeviceInfo(specifics.cache_guid()));
  EXPECT_EQ(3, change_count());
}

TEST_F(DeviceInfoSyncBridgeTest, ApplySyncChangesStore) {
  InitializeAndPump();
  ASSERT_EQ(1, change_count());

  const DeviceInfoSpecifics specifics = CreateSpecifics(1);
  ModelTypeState state = StateWithEncryption("ekn");
  std::unique_ptr<MetadataChangeList> metadata_changes =
      bridge()->CreateMetadataChangeList();
  metadata_changes->UpdateModelTypeState(state);

  auto error = bridge()->ApplySyncChanges(std::move(metadata_changes),
                                          EntityAddList({specifics}));
  EXPECT_FALSE(error);
  EXPECT_EQ(2, change_count());

  EXPECT_CALL(*processor(),
              ModelReadyToSync(MetadataBatchContains(
                  HasEncryptionKeyName(state.encryption_key_name()),
                  /*entities=*/IsEmpty())));
  RestartBridge();

  std::unique_ptr<DeviceInfo> info =
      bridge()->GetDeviceInfo(specifics.cache_guid());
  ASSERT_TRUE(info);
  EXPECT_THAT(*info, ModelEqualsSpecifics(specifics));
}

TEST_F(DeviceInfoSyncBridgeTest, ApplySyncChangesWithLocalGuid) {
  InitializeAndPump();

  ASSERT_TRUE(
      bridge()->GetDeviceInfo(local_device()->GetLocalDeviceInfo()->guid()));
  ASSERT_EQ(1, change_count());

  // The bridge should ignore these changes using this specifics because its
  // guid will match the local device.
  EXPECT_CALL(*processor(), Put(_, _, _)).Times(0);

  const DeviceInfoSpecifics specifics = CreateSpecifics(kDefaultLocalSuffix);
  auto error_on_add = bridge()->ApplySyncChanges(
      bridge()->CreateMetadataChangeList(), EntityAddList({specifics}));
  EXPECT_FALSE(error_on_add);
  EXPECT_EQ(1, change_count());

  auto error_on_delete = bridge()->ApplySyncChanges(
      bridge()->CreateMetadataChangeList(),
      {EntityChange::CreateDelete(specifics.cache_guid())});
  EXPECT_FALSE(error_on_delete);
  EXPECT_EQ(1, change_count());
}

TEST_F(DeviceInfoSyncBridgeTest, ApplyDeleteNonexistent) {
  InitializeAndPump();
  ASSERT_EQ(1, change_count());

  EXPECT_CALL(*processor(), Delete(_, _)).Times(0);
  auto error = bridge()->ApplySyncChanges(bridge()->CreateMetadataChangeList(),
                                          {EntityChange::CreateDelete("guid")});
  EXPECT_FALSE(error);
  EXPECT_EQ(1, change_count());
}

TEST_F(DeviceInfoSyncBridgeTest, ClearProviderAndApply) {
  // This will initialize the provider a first time.
  InitializeAndPump();
  EXPECT_EQ(1u, bridge()->GetAllDeviceInfo().size());

  const DeviceInfoSpecifics specifics = CreateSpecifics(1);

  local_device()->Clear();
  auto error1 = bridge()->ApplySyncChanges(bridge()->CreateMetadataChangeList(),
                                           EntityAddList({specifics}));
  EXPECT_FALSE(error1);
  EXPECT_EQ(1u, bridge()->GetAllDeviceInfo().size());

  local_device()->Initialize(CreateModel(kDefaultLocalSuffix));
  auto error2 = bridge()->ApplySyncChanges(bridge()->CreateMetadataChangeList(),
                                           EntityAddList({specifics}));
  EXPECT_FALSE(error2);
  EXPECT_EQ(2u, bridge()->GetAllDeviceInfo().size());
}

TEST_F(DeviceInfoSyncBridgeTest, MergeEmpty) {
  InitializeAndPump();

  // TODO(skym): Stop sending local twice. The first of the two puts will
  // probably happen before the processor is tracking metadata yet, and so there
  // should not be much overhead.
  EXPECT_CALL(*processor(),
              Put(local_device()->GetLocalDeviceInfo()->guid(), _, _));
  EXPECT_CALL(*processor(), Delete(_, _)).Times(0);

  auto error = bridge()->MergeSyncData(bridge()->CreateMetadataChangeList(),
                                       EntityChangeList());
  EXPECT_FALSE(error);
  EXPECT_EQ(1, change_count());
}

TEST_F(DeviceInfoSyncBridgeTest, MergeWithData) {
  const DeviceInfoSpecifics unique_local = CreateSpecifics(1);
  DeviceInfoSpecifics conflict_local = CreateSpecifics(2);
  DeviceInfoSpecifics conflict_remote = CreateSpecifics(3);
  const DeviceInfoSpecifics unique_remote = CreateSpecifics(4);

  const std::string conflict_guid = "conflict_guid";
  conflict_local.set_cache_guid(conflict_guid);
  conflict_remote.set_cache_guid(conflict_guid);

  WriteToStore({unique_local, conflict_local});
  InitializeAndPump();
  ASSERT_EQ(1, change_count());

  ModelTypeState state = StateWithEncryption("ekn");
  std::unique_ptr<MetadataChangeList> metadata_changes =
      bridge()->CreateMetadataChangeList();
  metadata_changes->UpdateModelTypeState(state);

  EXPECT_CALL(*processor(), Delete(_, _)).Times(0);

  EXPECT_CALL(*processor(),
              Put(local_device()->GetLocalDeviceInfo()->guid(), _, _));
  // Bridge should tell the processor about the existence of unique_local.
  EXPECT_CALL(*processor(), Put(unique_local.cache_guid(),
                                HasSpecifics(HasDeviceInfo(unique_local)), _));

  auto error =
      bridge()->MergeSyncData(std::move(metadata_changes),
                              EntityAddList({conflict_remote, unique_remote}));
  EXPECT_FALSE(error);
  EXPECT_EQ(2, change_count());

  // The remote should beat the local in conflict.
  EXPECT_EQ(4u, bridge()->GetAllDeviceInfo().size());
  EXPECT_THAT(*bridge()->GetDeviceInfo(unique_local.cache_guid()),
              ModelEqualsSpecifics(unique_local));
  EXPECT_THAT(*bridge()->GetDeviceInfo(unique_remote.cache_guid()),
              ModelEqualsSpecifics(unique_remote));
  EXPECT_THAT(*bridge()->GetDeviceInfo(conflict_guid),
              ModelEqualsSpecifics(conflict_remote));

  EXPECT_CALL(*processor(),
              ModelReadyToSync(MetadataBatchContains(
                  HasEncryptionKeyName(state.encryption_key_name()),
                  /*entities=*/IsEmpty())));
  RestartBridge();
}

TEST_F(DeviceInfoSyncBridgeTest, MergeLocalGuid) {
  // If not recent, then reconcile is going to try to send an updated version to
  // Sync, which makes interpreting change_count() more difficult.
  const DeviceInfoSpecifics specifics = CreateSpecifics(kDefaultLocalSuffix);
  WriteToStore({specifics});
  InitializeAndPump();

  EXPECT_CALL(*processor(), Put(_, _, _)).Times(0);
  EXPECT_CALL(*processor(), Delete(_, _)).Times(0);

  auto error = bridge()->MergeSyncData(bridge()->CreateMetadataChangeList(),
                                       EntityAddList({specifics}));
  EXPECT_FALSE(error);
  EXPECT_EQ(0, change_count());
  EXPECT_EQ(1u, bridge()->GetAllDeviceInfo().size());
}

TEST_F(DeviceInfoSyncBridgeTest, MergeLocalGuidBeforeReconcile) {
  InitializeBridge();

  // The message loop is never pumped, which means local data/metadata is never
  // loaded, and thus reconcile is never called. The bridge should ignore this
  // EntityData because its cache guid is the same the local device's.
  auto error = bridge()->MergeSyncData(
      bridge()->CreateMetadataChangeList(),
      EntityAddList({CreateSpecifics(kDefaultLocalSuffix)}));
  EXPECT_FALSE(error);
  EXPECT_EQ(0, change_count());
  EXPECT_EQ(0u, bridge()->GetAllDeviceInfo().size());
}

TEST_F(DeviceInfoSyncBridgeTest, ClearProviderAndMerge) {
  // This will initialize the provider a first time.
  InitializeAndPump();
  EXPECT_EQ(1u, bridge()->GetAllDeviceInfo().size());

  const DeviceInfoSpecifics specifics = CreateSpecifics(1);

  local_device()->Clear();
  auto error1 = bridge()->MergeSyncData(bridge()->CreateMetadataChangeList(),
                                        EntityAddList({specifics}));
  EXPECT_FALSE(error1);
  EXPECT_EQ(1u, bridge()->GetAllDeviceInfo().size());

  local_device()->Initialize(CreateModel(kDefaultLocalSuffix));
  auto error2 = bridge()->MergeSyncData(bridge()->CreateMetadataChangeList(),
                                        EntityAddList({specifics}));
  EXPECT_FALSE(error2);
  EXPECT_EQ(2u, bridge()->GetAllDeviceInfo().size());
}

TEST_F(DeviceInfoSyncBridgeTest, CountActiveDevices) {
  InitializeAndPump();
  EXPECT_EQ(1, bridge()->CountActiveDevices());

  // Regardless of the time, these following two ApplySyncChanges(...) calls
  // have the same guid as the local device.
  bridge()->ApplySyncChanges(
      bridge()->CreateMetadataChangeList(),
      EntityAddList({CreateSpecifics(kDefaultLocalSuffix)}));
  EXPECT_EQ(1, bridge()->CountActiveDevices());

  bridge()->ApplySyncChanges(
      bridge()->CreateMetadataChangeList(),
      EntityAddList({CreateSpecifics(kDefaultLocalSuffix)}));
  EXPECT_EQ(1, bridge()->CountActiveDevices());

  // A different guid will actually contribute to the count.
  bridge()->ApplySyncChanges(bridge()->CreateMetadataChangeList(),
                             EntityAddList({CreateSpecifics(1)}));
  EXPECT_EQ(2, bridge()->CountActiveDevices());

  // Now set time to long ago in the past, it should not be active anymore.
  bridge()->ApplySyncChanges(
      bridge()->CreateMetadataChangeList(),
      EntityAddList({CreateSpecifics(
          1, base::Time::Now() - base::TimeDelta::FromDays(365))}));
  EXPECT_EQ(1, bridge()->CountActiveDevices());
}

TEST_F(DeviceInfoSyncBridgeTest, MultipleOnProviderInitialized) {
  EXPECT_CALL(*processor(), ModelReadyToSync(_)).Times(0);
  set_provider(std::make_unique<LocalDeviceInfoProviderMock>());
  InitializeAndPump();

  // Verify the processor is given metadata.
  EXPECT_CALL(*processor(), ModelReadyToSync(NotNull()));
  local_device()->Initialize(CreateModel(0));
  base::RunLoop().RunUntilIdle();

  // Initializing the provider again shouldn't trigger ModelReadyToSync() again.
  EXPECT_CALL(*processor(), ModelReadyToSync(_)).Times(0);
  local_device()->Initialize(CreateModel(0));
  base::RunLoop().RunUntilIdle();
}

TEST_F(DeviceInfoSyncBridgeTest, SendLocalData) {
  // Ensure |last_updated| is about now, plus or minus a little bit.
  EXPECT_CALL(*processor(), Put(_, HasSpecifics(HasLastUpdatedAboutNow()), _));
  InitializeAndPump();
  EXPECT_EQ(1, change_count());
  testing::Mock::VerifyAndClearExpectations(processor());

  // Ensure |last_updated| is about now, plus or minus a little bit.
  EXPECT_CALL(*processor(), Put(_, HasSpecifics(HasLastUpdatedAboutNow()), _));
  ForcePulse();
  EXPECT_EQ(2, change_count());

  // After clearing, pulsing should no-op and not result in a processor put or
  // a notification to observers.
  EXPECT_CALL(*processor(), Put(_, _, _)).Times(0);
  local_device()->Clear();
  ForcePulse();
  EXPECT_EQ(2, change_count());
}

TEST_F(DeviceInfoSyncBridgeTest, ApplyDisableSyncChanges) {
  InitializeAndPump();
  EXPECT_EQ(1u, bridge()->GetAllDeviceInfo().size());
  EXPECT_EQ(1, change_count());

  const DeviceInfoSpecifics specifics = CreateSpecifics(1);
  auto error = bridge()->ApplySyncChanges(bridge()->CreateMetadataChangeList(),
                                          EntityAddList({specifics}));

  EXPECT_FALSE(error);
  EXPECT_EQ(2u, bridge()->GetAllDeviceInfo().size());
  EXPECT_EQ(2, change_count());

  // Should clear out all local data and notify observers.
  bridge()->ApplyDisableSyncChanges({});
  EXPECT_EQ(0u, bridge()->GetAllDeviceInfo().size());
  EXPECT_EQ(3, change_count());

  // Reloading from storage shouldn't contain remote data.
  RestartBridge();
  EXPECT_EQ(1u, bridge()->GetAllDeviceInfo().size());
  EXPECT_EQ(4, change_count());
}

}  // namespace

}  // namespace syncer
