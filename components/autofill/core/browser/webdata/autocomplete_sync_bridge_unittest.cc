// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/webdata/autocomplete_sync_bridge.h"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/autofill/core/browser/webdata/autofill_entry.h"
#include "components/autofill/core/browser/webdata/autofill_table.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_backend.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_backend_impl.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/model_error.h"
#include "components/sync/model/recording_model_type_change_processor.h"
#include "components/webdata/common/web_database.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::UTF8ToUTF16;
using base::ScopedTempDir;
using base::Time;
using base::TimeDelta;
using sync_pb::AutofillSpecifics;
using sync_pb::EntityMetadata;
using sync_pb::EntitySpecifics;
using sync_pb::ModelTypeState;
using syncer::DataBatch;
using syncer::EntityChange;
using syncer::EntityChangeList;
using syncer::EntityData;
using syncer::EntityDataPtr;
using syncer::FakeModelTypeChangeProcessor;
using syncer::KeyAndData;
using syncer::ModelError;
using syncer::ModelType;
using syncer::ModelTypeChangeProcessor;
using syncer::ModelTypeSyncBridge;
using syncer::RecordingModelTypeChangeProcessor;

namespace autofill {

namespace {

const char kNameFormat[] = "name %d";
const char kValueFormat[] = "value %d";

void VerifyEqual(const AutofillSpecifics& s1, const AutofillSpecifics& s2) {
  // Instead of just comparing serialized strings, manually check fields to show
  // differences on failure.
  EXPECT_EQ(s1.name(), s2.name());
  EXPECT_EQ(s1.value(), s2.value());
  EXPECT_EQ(s1.usage_timestamp().size(), s2.usage_timestamp().size());
  int size = std::min(s1.usage_timestamp().size(), s2.usage_timestamp().size());
  for (int i = 0; i < size; ++i) {
    EXPECT_EQ(s1.usage_timestamp(i), s2.usage_timestamp(i))
        << "Values differ at index " << i;
  }
  // Neither should have any profile data, so we don't have to compare it.
  EXPECT_FALSE(s1.has_profile());
  EXPECT_FALSE(s2.has_profile());
}

void VerifyDataBatch(std::map<std::string, AutofillSpecifics> expected,
                     std::unique_ptr<DataBatch> batch) {
  while (batch->HasNext()) {
    const KeyAndData& pair = batch->Next();
    auto iter = expected.find(pair.first);
    ASSERT_NE(iter, expected.end());
    VerifyEqual(iter->second, pair.second->specifics.autofill());
    // Removing allows us to verify we don't see the same item multiple times,
    // and that we saw everything we expected.
    expected.erase(iter);
  }
  EXPECT_TRUE(expected.empty());
}

AutofillEntry CreateAutofillEntry(const AutofillSpecifics& autofill_specifics) {
  AutofillKey key(UTF8ToUTF16(autofill_specifics.name()),
                  UTF8ToUTF16(autofill_specifics.value()));
  Time date_created, date_last_used;
  const google::protobuf::RepeatedField<int64_t>& timestamps =
      autofill_specifics.usage_timestamp();
  if (!timestamps.empty()) {
    date_created = Time::FromInternalValue(*timestamps.begin());
    date_last_used = Time::FromInternalValue(*timestamps.rbegin());
  }
  return AutofillEntry(key, date_created, date_last_used);
}

EntityDataPtr SpecificsToEntity(const AutofillSpecifics& specifics) {
  EntityData data;
  data.client_tag_hash = "ignored";
  *data.specifics.mutable_autofill() = specifics;
  return data.PassToPtr();
}

class FakeAutofillBackend : public AutofillWebDataBackend {
 public:
  FakeAutofillBackend() {}
  ~FakeAutofillBackend() override {}
  WebDatabase* GetDatabase() override { return db_; }
  void AddObserver(
      autofill::AutofillWebDataServiceObserverOnDBSequence* observer) override {
  }
  void RemoveObserver(
      autofill::AutofillWebDataServiceObserverOnDBSequence* observer) override {
  }
  void RemoveExpiredFormElements() override {}
  void NotifyOfMultipleAutofillChanges() override {}
  void NotifyThatSyncHasStarted(ModelType model_type) override {}
  void SetWebDatabase(WebDatabase* db) { db_ = db; }

 private:
  WebDatabase* db_;
};

}  // namespace

class AutocompleteSyncBridgeTest : public testing::Test {
 public:
  AutocompleteSyncBridgeTest() {
    if (temp_dir_.CreateUniqueTempDir()) {
      db_.AddTable(&table_);
      db_.Init(temp_dir_.GetPath().AppendASCII("SyncTestWebDatabase"));
      backend_.SetWebDatabase(&db_);
      ResetBridge();
    }
  }
  ~AutocompleteSyncBridgeTest() override {}

  void ResetBridge(bool expect_error = false) {
    bridge_.reset(new AutocompleteSyncBridge(
        &backend_,
        RecordingModelTypeChangeProcessor::CreateProcessorAndAssignRawPointer(
            &processor_, expect_error)));
  }

  void SaveSpecificsToTable(
      const std::vector<AutofillSpecifics>& specifics_list) {
    std::vector<AutofillEntry> new_entries;
    for (const auto& specifics : specifics_list) {
      new_entries.push_back(CreateAutofillEntry(specifics));
    }
    table_.UpdateAutofillEntries(new_entries);
  }

  AutofillSpecifics CreateSpecifics(const std::string& name,
                                    const std::string& value,
                                    const std::vector<int>& timestamps) {
    AutofillSpecifics specifics;
    specifics.set_name(name);
    specifics.set_value(value);
    for (int timestamp : timestamps) {
      specifics.add_usage_timestamp(
          Time::FromTimeT(timestamp).ToInternalValue());
    }
    return specifics;
  }

  AutofillSpecifics CreateSpecifics(int suffix,
                                    const std::vector<int>& timestamps) {
    return CreateSpecifics(base::StringPrintf(kNameFormat, suffix),
                           base::StringPrintf(kValueFormat, suffix),
                           timestamps);
  }

  AutofillSpecifics CreateSpecifics(int suffix) {
    return CreateSpecifics(suffix, std::vector<int>{0});
  }

  std::string GetClientTag(const AutofillSpecifics& specifics) {
    std::string tag =
        bridge()->GetClientTag(SpecificsToEntity(specifics).value());
    EXPECT_FALSE(tag.empty());
    return tag;
  }

  std::string GetStorageKey(const AutofillSpecifics& specifics) {
    std::string key =
        bridge()->GetStorageKey(SpecificsToEntity(specifics).value());
    EXPECT_FALSE(key.empty());
    return key;
  }

  EntityChangeList CreateEntityAddList(
      const std::vector<AutofillSpecifics>& specifics_vector) {
    EntityChangeList changes;
    for (const auto& specifics : specifics_vector) {
      changes.push_back(EntityChange::CreateAdd(GetStorageKey(specifics),
                                                SpecificsToEntity(specifics)));
    }
    return changes;
  }

  void VerifyApplyChanges(const EntityChangeList& changes) {
    const auto error = bridge()->ApplySyncChanges(
        bridge()->CreateMetadataChangeList(), changes);
    EXPECT_FALSE(error);
  }

  void VerifyApplyAdds(const std::vector<AutofillSpecifics>& specifics) {
    VerifyApplyChanges(CreateEntityAddList(specifics));
  }

  void VerifyMerge(const std::vector<AutofillSpecifics>& specifics) {
    const auto error = bridge()->MergeSyncData(
        bridge()->CreateMetadataChangeList(), CreateEntityAddList(specifics));
    EXPECT_FALSE(error);
  }

  std::map<std::string, AutofillSpecifics> ExpectedMap(
      const std::vector<AutofillSpecifics>& specifics_vector) {
    std::map<std::string, AutofillSpecifics> map;
    for (const auto& specifics : specifics_vector) {
      map[GetStorageKey(specifics)] = specifics;
    }
    return map;
  }

  void VerifyAllData(const std::vector<AutofillSpecifics>& expected) {
    bridge()->GetAllData(base::Bind(&VerifyDataBatch, ExpectedMap(expected)));
  }

  void VerifyProcessorRecordedPut(const AutofillSpecifics& specifics,
                                  int position = 0) {
    const std::string storage_key = GetStorageKey(specifics);
    auto recorded_specifics_iterator =
        processor().put_multimap().find(storage_key);

    EXPECT_NE(processor().put_multimap().end(), recorded_specifics_iterator);
    while (position > 0) {
      recorded_specifics_iterator++;
      EXPECT_NE(processor().put_multimap().end(), recorded_specifics_iterator);
      position--;
    }

    AutofillSpecifics recorded_specifics =
        recorded_specifics_iterator->second->specifics.autofill();
    VerifyEqual(recorded_specifics, specifics);
  }

  AutocompleteSyncBridge* bridge() { return bridge_.get(); }

  const RecordingModelTypeChangeProcessor& processor() { return *processor_; }

  AutofillTable* table() { return &table_; }

  FakeAutofillBackend* backend() { return &backend_; }

 private:
  ScopedTempDir temp_dir_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  FakeAutofillBackend backend_;
  AutofillTable table_;
  WebDatabase db_;
  std::unique_ptr<AutocompleteSyncBridge> bridge_;
  // A non-owning pointer to the processor given to the bridge. Will be null
  // before being given to the bridge, to make ownership easier.
  RecordingModelTypeChangeProcessor* processor_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AutocompleteSyncBridgeTest);
};

TEST_F(AutocompleteSyncBridgeTest, GetClientTag) {
  std::string tag = GetClientTag(CreateSpecifics(1));
  EXPECT_EQ(tag, GetClientTag(CreateSpecifics(1)));
  EXPECT_NE(tag, GetClientTag(CreateSpecifics(2)));
}

TEST_F(AutocompleteSyncBridgeTest, GetClientTagNotAffectedByTimestamp) {
  AutofillSpecifics specifics = CreateSpecifics(1);
  std::string tag = GetClientTag(specifics);

  specifics.add_usage_timestamp(1);
  EXPECT_EQ(tag, GetClientTag(specifics));

  specifics.add_usage_timestamp(0);
  EXPECT_EQ(tag, GetClientTag(specifics));

  specifics.add_usage_timestamp(-1);
  EXPECT_EQ(tag, GetClientTag(specifics));
}

TEST_F(AutocompleteSyncBridgeTest, GetClientTagRespectsNullCharacter) {
  AutofillSpecifics specifics;
  std::string tag = GetClientTag(specifics);

  specifics.set_value(std::string("\0", 1));
  EXPECT_NE(tag, GetClientTag(specifics));
}

// The client tags should never change as long as we want to maintain backwards
// compatibility with the previous iteration of autocomplete-sync integration,
// AutocompleteSyncableService and Sync's Directory. This is because old clients
// will re-generate client tags and then hashes on local changes, and this
// process must create identical values to what this client has created. If this
// test case starts failing, you should not alter the fixed values here unless
// you know what you're doing.
TEST_F(AutocompleteSyncBridgeTest, GetClientTagFixed) {
  EXPECT_EQ("autofill_entry|name%201|value%201",
            GetClientTag(CreateSpecifics(1)));
  EXPECT_EQ("autofill_entry|name%202|value%202",
            GetClientTag(CreateSpecifics(2)));
  EXPECT_EQ("autofill_entry||", GetClientTag(AutofillSpecifics()));
  AutofillSpecifics specifics;
  specifics.set_name("\xEC\xA4\x91");
  specifics.set_value("\xD0\x80");
  EXPECT_EQ("autofill_entry|%EC%A4%91|%D0%80", GetClientTag(specifics));
}

TEST_F(AutocompleteSyncBridgeTest, GetStorageKey) {
  std::string key = GetStorageKey(CreateSpecifics(1));
  EXPECT_EQ(key, GetStorageKey(CreateSpecifics(1)));
  EXPECT_NE(key, GetStorageKey(CreateSpecifics(2)));
}

TEST_F(AutocompleteSyncBridgeTest, GetStorageKeyNotAffectedByTimestamp) {
  AutofillSpecifics specifics = CreateSpecifics(1);
  std::string key = GetStorageKey(specifics);

  specifics.add_usage_timestamp(1);
  EXPECT_EQ(key, GetStorageKey(specifics));

  specifics.add_usage_timestamp(0);
  EXPECT_EQ(key, GetStorageKey(specifics));

  specifics.add_usage_timestamp(-1);
  EXPECT_EQ(key, GetStorageKey(specifics));
}

TEST_F(AutocompleteSyncBridgeTest, GetStorageKeyRespectsNullCharacter) {
  AutofillSpecifics specifics;
  std::string key = GetStorageKey(specifics);

  specifics.set_value(std::string("\0", 1));
  EXPECT_NE(key, GetStorageKey(specifics));
}

// The storage key should never accidentally change for existing data. This
// would cause lookups to fail and either lose or duplicate user data. It should
// be possible for the model type to migrate storage key formats, but doing so
// would need to be done very carefully.
TEST_F(AutocompleteSyncBridgeTest, GetStorageKeyFixed) {
  EXPECT_EQ("\n\x6name 1\x12\avalue 1", GetStorageKey(CreateSpecifics(1)));
  EXPECT_EQ("\n\x6name 2\x12\avalue 2", GetStorageKey(CreateSpecifics(2)));
  // This literal contains the null terminating character, which causes
  // std::string to stop copying early if we don't tell it how much to read.
  EXPECT_EQ(std::string("\n\0\x12\0", 4), GetStorageKey(AutofillSpecifics()));
  AutofillSpecifics specifics;
  specifics.set_name("\xEC\xA4\x91");
  specifics.set_value("\xD0\x80");
  EXPECT_EQ("\n\x3\xEC\xA4\x91\x12\x2\xD0\x80", GetStorageKey(specifics));
}

TEST_F(AutocompleteSyncBridgeTest, GetData) {
  const AutofillSpecifics specifics1 = CreateSpecifics(1);
  const AutofillSpecifics specifics2 = CreateSpecifics(2);
  const AutofillSpecifics specifics3 = CreateSpecifics(3);
  SaveSpecificsToTable({specifics1, specifics2, specifics3});
  bridge()->GetData(
      {GetStorageKey(specifics1), GetStorageKey(specifics3)},
      base::Bind(&VerifyDataBatch, ExpectedMap({specifics1, specifics3})));
}

TEST_F(AutocompleteSyncBridgeTest, GetDataNotExist) {
  const AutofillSpecifics specifics1 = CreateSpecifics(1);
  const AutofillSpecifics specifics2 = CreateSpecifics(2);
  const AutofillSpecifics specifics3 = CreateSpecifics(3);
  SaveSpecificsToTable({specifics1, specifics2});
  bridge()->GetData(
      {GetStorageKey(specifics1), GetStorageKey(specifics2),
       GetStorageKey(specifics3)},
      base::Bind(&VerifyDataBatch, ExpectedMap({specifics1, specifics2})));
}

TEST_F(AutocompleteSyncBridgeTest, GetAllData) {
  const AutofillSpecifics specifics1 = CreateSpecifics(1);
  const AutofillSpecifics specifics2 = CreateSpecifics(2);
  const AutofillSpecifics specifics3 = CreateSpecifics(3);
  SaveSpecificsToTable({specifics1, specifics2, specifics3});
  VerifyAllData({specifics1, specifics2, specifics3});
}

TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesEmpty) {
  // TODO(skym, crbug.com/672619): Ideally would like to verify that the db is
  // not accessed.
  VerifyApplyAdds(std::vector<AutofillSpecifics>());
}

TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesSimple) {
  AutofillSpecifics specifics1 = CreateSpecifics(1);
  AutofillSpecifics specifics2 = CreateSpecifics(2);
  ASSERT_NE(specifics1.SerializeAsString(), specifics2.SerializeAsString());
  ASSERT_NE(GetStorageKey(specifics1), GetStorageKey(specifics2));

  VerifyApplyAdds({specifics1, specifics2});
  VerifyAllData({specifics1, specifics2});

  VerifyApplyChanges({EntityChange::CreateDelete(GetStorageKey(specifics1))});
  VerifyAllData({specifics2});
}

// Should be resilient to deleting and updating non-existent things, and adding
// existing ones.
TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesWrongChangeType) {
  AutofillSpecifics specifics = CreateSpecifics(1, {1});
  VerifyApplyChanges({EntityChange::CreateDelete(GetStorageKey(specifics))});
  VerifyAllData(std::vector<AutofillSpecifics>());

  VerifyApplyChanges({EntityChange::CreateUpdate(
      GetStorageKey(specifics), SpecificsToEntity(specifics))});
  VerifyAllData({specifics});

  specifics.add_usage_timestamp(Time::FromTimeT(2).ToInternalValue());
  VerifyApplyAdds({specifics});
  VerifyAllData({specifics});
}

// The format in the table has a fixed 2 timestamp format. Round tripping is
// lossy and the middle value should be thrown out.
TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesThreeTimestamps) {
  VerifyApplyAdds({CreateSpecifics(1, {1, 2, 3})});
  VerifyAllData({CreateSpecifics(1, {1, 3})});
}

// The correct format of timestamps is that the first should be smaller and the
// second should be larger. Bad foreign data should be repaired.
TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesWrongOrder) {
  VerifyApplyAdds({CreateSpecifics(1, {3, 2})});
  VerifyAllData({CreateSpecifics(1, {2, 3})});
}

// In a minor attempt to save bandwidth, we only send one of the two timestamps
// when they share a value.
TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesRepeatedTime) {
  VerifyApplyAdds({CreateSpecifics(1, {2, 2})});
  VerifyAllData({CreateSpecifics(1, {2})});
}

// Again, the format in the table is lossy, and cannot distinguish between no
// time, and valid time zero.
TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesNoTime) {
  VerifyApplyAdds({CreateSpecifics(1, std::vector<int>())});
  VerifyAllData({CreateSpecifics(1, {0})});
}

// If has_value() returns false, then the specifics are determined to be old
// style and ignored.
TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesNoValue) {
  AutofillSpecifics input = CreateSpecifics(1, {2, 3});
  input.clear_value();
  VerifyApplyAdds({input});
  VerifyAllData(std::vector<AutofillSpecifics>());
}

// Should be treated the same as an empty string name. This inconsistency is
// being perpetuated from the previous sync integration.
TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesNoName) {
  AutofillSpecifics input = CreateSpecifics(1, {2, 3});
  input.clear_name();
  VerifyApplyAdds({input});
  VerifyAllData({input});
}

// UTF-8 characters should not be dropped when round tripping, including middle
// of string \0 characters.
TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesUTF) {
  const AutofillSpecifics specifics =
      CreateSpecifics(std::string("\n\0\x12\0", 4), "\xEC\xA4\x91", {1});
  VerifyApplyAdds({specifics});
  VerifyAllData({specifics});
}

// Timestamps should always use the oldest creation time, and the most recent
// usage time.
TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesMinMaxTimestamps) {
  const AutofillSpecifics initial = CreateSpecifics(1, {3, 6});
  VerifyApplyAdds({initial});
  VerifyAllData({initial});

  VerifyApplyAdds({CreateSpecifics(1, {2, 5})});
  VerifyAllData({CreateSpecifics(1, {2, 6})});

  VerifyApplyAdds({CreateSpecifics(1, {4, 7})});
  VerifyAllData({CreateSpecifics(1, {2, 7})});
}

// An error should be generated when parsing the storage key happens. This
// should never happen in practice because storage keys should be generated at
// runtime by the bridge and not loaded from disk.
TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesBadStorageKey) {
  const auto error = bridge()->ApplySyncChanges(
      bridge()->CreateMetadataChangeList(),
      {EntityChange::CreateDelete("bogus storage key")});
  EXPECT_TRUE(error);
}

TEST_F(AutocompleteSyncBridgeTest, ApplySyncChangesDatabaseFailure) {
  // TODO(skym, crbug.com/672619): Should have tests that get false back when
  // making db calls and verify the errors are propagated up.
}

TEST_F(AutocompleteSyncBridgeTest, LocalEntriesAdded) {
  const AutofillSpecifics added_specifics1 = CreateSpecifics(1, {2, 3});
  const AutofillSpecifics added_specifics2 = CreateSpecifics(2, {2, 3});

  const AutofillEntry added_entry1 = CreateAutofillEntry(added_specifics1);
  const AutofillEntry added_entry2 = CreateAutofillEntry(added_specifics2);

  table()->UpdateAutofillEntries({added_entry1, added_entry2});

  bridge()->AutofillEntriesChanged(
      {AutofillChange(AutofillChange::ADD, added_entry1.key()),
       AutofillChange(AutofillChange::ADD, added_entry2.key())});

  EXPECT_EQ(2u, processor().put_multimap().size());

  VerifyProcessorRecordedPut(added_specifics1);
  VerifyProcessorRecordedPut(added_specifics2);
}

TEST_F(AutocompleteSyncBridgeTest, LocalEntryAddedThenUpdated) {
  const AutofillSpecifics added_specifics = CreateSpecifics(1, {2, 3});
  const AutofillEntry added_entry = CreateAutofillEntry(added_specifics);
  table()->UpdateAutofillEntries({added_entry});

  bridge()->AutofillEntriesChanged(
      {AutofillChange(AutofillChange::ADD, added_entry.key())});

  EXPECT_EQ(1u, processor().put_multimap().size());

  VerifyProcessorRecordedPut(added_specifics);

  const AutofillSpecifics updated_specifics = CreateSpecifics(1, {2, 4});
  const AutofillEntry updated_entry = CreateAutofillEntry(updated_specifics);
  table()->UpdateAutofillEntries({updated_entry});

  bridge()->AutofillEntriesChanged(
      {AutofillChange(AutofillChange::UPDATE, updated_entry.key())});

  VerifyProcessorRecordedPut(updated_specifics, 1);
}

TEST_F(AutocompleteSyncBridgeTest, LocalEntryDeleted) {
  const AutofillSpecifics deleted_specifics = CreateSpecifics(1, {2, 3});
  const AutofillEntry deleted_entry = CreateAutofillEntry(deleted_specifics);
  const std::string storage_key = GetStorageKey(deleted_specifics);

  bridge()->AutofillEntriesChanged(
      {AutofillChange(AutofillChange::REMOVE, deleted_entry.key())});

  EXPECT_EQ(1u, processor().delete_set().size());
  EXPECT_NE(processor().delete_set().end(),
            processor().delete_set().find(storage_key));
}

TEST_F(AutocompleteSyncBridgeTest, LoadMetadataCalled) {
  EXPECT_NE(nullptr, processor().metadata());
  EXPECT_FALSE(processor().metadata()->GetModelTypeState().initial_sync_done());
  EXPECT_EQ(0u, processor().metadata()->TakeAllMetadata().size());

  ModelTypeState model_type_state;
  model_type_state.set_initial_sync_done(true);
  EXPECT_TRUE(
      table()->UpdateModelTypeState(syncer::AUTOFILL, model_type_state));
  EXPECT_TRUE(
      table()->UpdateSyncMetadata(syncer::AUTOFILL, "key", EntityMetadata()));

  ResetBridge();

  EXPECT_NE(nullptr, processor().metadata());
  EXPECT_TRUE(processor().metadata()->GetModelTypeState().initial_sync_done());
  EXPECT_EQ(1u, processor().metadata()->TakeAllMetadata().size());
}

TEST_F(AutocompleteSyncBridgeTest, LoadMetadataReportsErrorForMissingDB) {
  backend()->SetWebDatabase(nullptr);
  // The processor's destructor will verify that an error has occured.
  ResetBridge(/*expect_error=*/true);
}

TEST_F(AutocompleteSyncBridgeTest, MergeSyncDataEmpty) {
  VerifyMerge(std::vector<AutofillSpecifics>());

  VerifyAllData(std::vector<AutofillSpecifics>());
  EXPECT_EQ(0u, processor().delete_set().size());
  EXPECT_EQ(0u, processor().put_multimap().size());
}

TEST_F(AutocompleteSyncBridgeTest, MergeSyncDataRemoteOnly) {
  const AutofillSpecifics specifics1 = CreateSpecifics(1, {2});
  const AutofillSpecifics specifics2 = CreateSpecifics(2, {3, 4});

  VerifyMerge({specifics1, specifics2});

  VerifyAllData({specifics1, specifics2});
  EXPECT_EQ(0u, processor().delete_set().size());
  EXPECT_EQ(0u, processor().put_multimap().size());
}

TEST_F(AutocompleteSyncBridgeTest, MergeSyncDataLocalOnly) {
  const AutofillSpecifics specifics1 = CreateSpecifics(1, {2});
  const AutofillSpecifics specifics2 = CreateSpecifics(2, {3, 4});
  VerifyApplyAdds({specifics1, specifics2});
  VerifyAllData({specifics1, specifics2});

  VerifyMerge(std::vector<AutofillSpecifics>());

  VerifyAllData({specifics1, specifics2});
  EXPECT_EQ(2u, processor().put_multimap().size());
  VerifyProcessorRecordedPut(specifics1);
  VerifyProcessorRecordedPut(specifics2);
  EXPECT_EQ(0u, processor().delete_set().size());
}

TEST_F(AutocompleteSyncBridgeTest, MergeSyncDataAllMerged) {
  const AutofillSpecifics local1 = CreateSpecifics(1, {2});
  const AutofillSpecifics local2 = CreateSpecifics(2, {3, 4});
  const AutofillSpecifics local3 = CreateSpecifics(3, {4});
  const AutofillSpecifics local4 = CreateSpecifics(4, {5, 6});
  const AutofillSpecifics local5 = CreateSpecifics(5, {6, 9});
  const AutofillSpecifics local6 = CreateSpecifics(6, {7, 9});
  const AutofillSpecifics remote1 = local1;
  const AutofillSpecifics remote2 = local2;
  const AutofillSpecifics remote3 = CreateSpecifics(3, {5});
  const AutofillSpecifics remote4 = CreateSpecifics(4, {7, 8});
  const AutofillSpecifics remote5 = CreateSpecifics(5, {8, 9});
  const AutofillSpecifics remote6 = CreateSpecifics(6, {8, 10});
  const AutofillSpecifics merged1 = local1;
  const AutofillSpecifics merged2 = local2;
  const AutofillSpecifics merged3 = CreateSpecifics(3, {4, 5});
  const AutofillSpecifics merged4 = CreateSpecifics(4, {5, 8});
  const AutofillSpecifics merged5 = local5;
  const AutofillSpecifics merged6 = CreateSpecifics(6, {7, 10});
  VerifyApplyAdds({local1, local2, local3, local4, local5, local6});

  VerifyMerge({remote1, remote2, remote3, remote4, remote5, remote6});

  VerifyAllData({merged1, merged2, merged3, merged4, merged5, merged6});
  EXPECT_EQ(4u, processor().put_multimap().size());
  VerifyProcessorRecordedPut(merged3);
  VerifyProcessorRecordedPut(merged4);
  VerifyProcessorRecordedPut(merged5);
  VerifyProcessorRecordedPut(merged6);
  EXPECT_EQ(0u, processor().delete_set().size());
}

TEST_F(AutocompleteSyncBridgeTest, MergeSyncDataMixed) {
  const AutofillSpecifics local1 = CreateSpecifics(1, {2, 3});
  const AutofillSpecifics remote2 = CreateSpecifics(2, {2, 3});
  const AutofillSpecifics specifics3 = CreateSpecifics(3, {2, 3});
  const AutofillSpecifics local4 = CreateSpecifics(4, {1, 3});
  const AutofillSpecifics remote4 = CreateSpecifics(4, {2, 4});
  const AutofillSpecifics merged4 = CreateSpecifics(4, {1, 4});

  VerifyApplyAdds({local1, specifics3, local4});

  VerifyMerge({remote2, specifics3, remote4});

  VerifyAllData({local1, remote2, specifics3, merged4});
  EXPECT_EQ(2u, processor().put_multimap().size());
  VerifyProcessorRecordedPut(local1);
  VerifyProcessorRecordedPut(merged4);
  EXPECT_EQ(0u, processor().delete_set().size());
}

}  // namespace autofill
