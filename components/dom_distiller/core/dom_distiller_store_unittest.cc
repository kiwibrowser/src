// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/dom_distiller_store.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "components/dom_distiller/core/article_entry.h"
#include "components/dom_distiller/core/dom_distiller_test_util.h"
#include "components/leveldb_proto/testing/fake_db.h"
#include "components/sync/protocol/sync.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Time;
using leveldb_proto::test::FakeDB;
using sync_pb::EntitySpecifics;
using syncer::ModelType;
using syncer::SyncChange;
using syncer::SyncChangeList;
using syncer::SyncChangeProcessor;
using syncer::SyncData;
using syncer::SyncDataList;
using syncer::SyncError;
using syncer::SyncErrorFactory;
using testing::AssertionFailure;
using testing::AssertionResult;
using testing::AssertionSuccess;
using testing::SaveArgPointee;
using testing::_;

namespace dom_distiller {

namespace {

const ModelType kDomDistillerModelType = syncer::ARTICLES;

typedef std::map<std::string, ArticleEntry> EntryMap;

void AddEntry(const ArticleEntry& e, EntryMap* map) {
  (*map)[e.entry_id()] = e;
}

class FakeSyncErrorFactory : public syncer::SyncErrorFactory {
 public:
  syncer::SyncError CreateAndUploadError(const base::Location& location,
                                         const std::string& message) override {
    return syncer::SyncError();
  }
};

class FakeSyncChangeProcessor : public syncer::SyncChangeProcessor {
 public:
  explicit FakeSyncChangeProcessor(EntryMap* model) : model_(model) {}

  syncer::SyncDataList GetAllSyncData(syncer::ModelType type) const override {
    ADD_FAILURE() << "FakeSyncChangeProcessor::GetAllSyncData not implemented.";
    return syncer::SyncDataList();
  }

  SyncError ProcessSyncChanges(const base::Location&,
                               const syncer::SyncChangeList& changes) override {
    for (SyncChangeList::const_iterator it = changes.begin();
         it != changes.end(); ++it) {
      AddEntry(GetEntryFromChange(*it), model_);
    }
    return SyncError();
  }

 private:
  EntryMap* model_;
};

ArticleEntry CreateEntry(const std::string& entry_id,
                         const std::string& page_url1,
                         const std::string& page_url2,
                         const std::string& page_url3) {
  ArticleEntry entry;
  entry.set_entry_id(entry_id);
  if (!page_url1.empty()) {
    ArticleEntryPage* page = entry.add_pages();
    page->set_url(page_url1);
  }
  if (!page_url2.empty()) {
    ArticleEntryPage* page = entry.add_pages();
    page->set_url(page_url2);
  }
  if (!page_url3.empty()) {
    ArticleEntryPage* page = entry.add_pages();
    page->set_url(page_url3);
  }
  return entry;
}

ArticleEntry GetSampleEntry(int id) {
  static ArticleEntry entries[] = {
      CreateEntry("entry0", "example.com/1", "example.com/2", "example.com/3"),
      CreateEntry("entry1", "example.com/1", "", ""),
      CreateEntry("entry2", "example.com/p1", "example.com/p2", ""),
      CreateEntry("entry3", "example.com/something/all", "", ""),
      CreateEntry("entry4", "example.com/somethingelse/1", "", ""),
      CreateEntry("entry5", "rock.example.com/p1", "rock.example.com/p2", ""),
      CreateEntry("entry7", "example.com/entry7/1", "example.com/entry7/2", ""),
      CreateEntry("entry8", "example.com/entry8/1", "", ""),
      CreateEntry("entry9", "example.com/entry9/all", "", ""),
  };
  EXPECT_LT(id, 9);
  return entries[id % 9];
}

class MockDistillerObserver : public DomDistillerObserver {
 public:
  MOCK_METHOD1(ArticleEntriesUpdated, void(const std::vector<ArticleUpdate>&));
  ~MockDistillerObserver() override {}
};

}  // namespace

class DomDistillerStoreTest : public testing::Test {
 public:
  void SetUp() override {
    db_model_.clear();
    sync_model_.clear();
    store_model_.clear();
    next_sync_id_ = 1;
  }

  void TearDown() override {
    store_.reset();
    fake_db_ = nullptr;
    fake_sync_processor_ = nullptr;
  }

  // Creates a simple DomDistillerStore initialized with |store_model_| and
  // with a FakeDB backed by |db_model_|.
  void CreateStore() {
    fake_db_ = new FakeDB<ArticleEntry>(&db_model_);
    store_.reset(test::util::CreateStoreWithFakeDB(fake_db_, store_model_));
  }

  void StartSyncing() {
    fake_sync_processor_ = new FakeSyncChangeProcessor(&sync_model_);

    store_->MergeDataAndStartSyncing(
        kDomDistillerModelType, SyncDataFromEntryMap(sync_model_),
        base::WrapUnique<SyncChangeProcessor>(fake_sync_processor_),
        std::unique_ptr<SyncErrorFactory>(new FakeSyncErrorFactory()));
  }

 protected:
  SyncData CreateSyncData(const ArticleEntry& entry) {
    EntitySpecifics specifics = SpecificsFromEntry(entry);
    return SyncData::CreateRemoteData(next_sync_id_++, specifics,
                                      Time::UnixEpoch());
  }

  SyncDataList SyncDataFromEntryMap(const EntryMap& model) {
    SyncDataList data;
    for (EntryMap::const_iterator it = model.begin(); it != model.end(); ++it) {
      data.push_back(CreateSyncData(it->second));
    }
    return data;
  }

  base::MessageLoop message_loop_;

  EntryMap db_model_;
  EntryMap sync_model_;
  FakeDB<ArticleEntry>::EntryMap store_model_;

  std::unique_ptr<DomDistillerStore> store_;

  // Both owned by |store_|.
  FakeDB<ArticleEntry>* fake_db_;
  FakeSyncChangeProcessor* fake_sync_processor_;

  int64_t next_sync_id_;
};

AssertionResult AreEntriesEqual(const DomDistillerStore::EntryVector& entries,
                                EntryMap expected_entries) {
  if (entries.size() != expected_entries.size())
    return AssertionFailure() << "Expected " << expected_entries.size()
                              << " entries but found " << entries.size();

  for (DomDistillerStore::EntryVector::const_iterator it = entries.begin();
       it != entries.end(); ++it) {
    EntryMap::iterator expected_it = expected_entries.find(it->entry_id());
    if (expected_it == expected_entries.end()) {
      return AssertionFailure() << "Found unexpected entry with id <"
                                << it->entry_id() << ">";
    }
    if (!AreEntriesEqual(expected_it->second, *it)) {
      return AssertionFailure() << "Mismatched entry with id <"
                                << it->entry_id() << ">";
    }
    expected_entries.erase(expected_it);
  }
  return AssertionSuccess();
}

AssertionResult AreEntryMapsEqual(const EntryMap& left, const EntryMap& right) {
  DomDistillerStore::EntryVector entries;
  for (EntryMap::const_iterator it = left.begin(); it != left.end(); ++it) {
    entries.push_back(it->second);
  }
  return AreEntriesEqual(entries, right);
}

TEST_F(DomDistillerStoreTest, TestDatabaseLoad) {
  AddEntry(GetSampleEntry(0), &db_model_);
  AddEntry(GetSampleEntry(1), &db_model_);
  AddEntry(GetSampleEntry(2), &db_model_);

  CreateStore();

  fake_db_->InitCallback(true);
  EXPECT_EQ(fake_db_->GetDirectory(),
            FakeDB<ArticleEntry>::DirectoryForTestDB());

  fake_db_->LoadCallback(true);
  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), db_model_));
}

TEST_F(DomDistillerStoreTest, TestDatabaseLoadMerge) {
  AddEntry(GetSampleEntry(0), &db_model_);
  AddEntry(GetSampleEntry(1), &db_model_);
  AddEntry(GetSampleEntry(2), &db_model_);

  AddEntry(GetSampleEntry(2), &store_model_);
  AddEntry(GetSampleEntry(3), &store_model_);
  AddEntry(GetSampleEntry(4), &store_model_);

  EntryMap expected_model(db_model_);
  AddEntry(GetSampleEntry(3), &expected_model);
  AddEntry(GetSampleEntry(4), &expected_model);

  CreateStore();
  fake_db_->InitCallback(true);
  fake_db_->LoadCallback(true);

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), expected_model));
  EXPECT_TRUE(AreEntryMapsEqual(db_model_, expected_model));
}

TEST_F(DomDistillerStoreTest, TestAddAndRemoveEntry) {
  CreateStore();
  fake_db_->InitCallback(true);
  fake_db_->LoadCallback(true);

  EXPECT_TRUE(store_->GetEntries().empty());
  EXPECT_TRUE(db_model_.empty());

  store_->AddEntry(GetSampleEntry(0));

  EntryMap expected_model;
  AddEntry(GetSampleEntry(0), &expected_model);

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), expected_model));
  EXPECT_TRUE(AreEntryMapsEqual(db_model_, expected_model));

  store_->RemoveEntry(GetSampleEntry(0));
  expected_model.clear();

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), expected_model));
  EXPECT_TRUE(AreEntryMapsEqual(db_model_, expected_model));
}

TEST_F(DomDistillerStoreTest, TestAddAndUpdateEntry) {
  CreateStore();
  fake_db_->InitCallback(true);
  fake_db_->LoadCallback(true);

  EXPECT_TRUE(store_->GetEntries().empty());
  EXPECT_TRUE(db_model_.empty());

  store_->AddEntry(GetSampleEntry(0));

  EntryMap expected_model;
  AddEntry(GetSampleEntry(0), &expected_model);

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), expected_model));
  EXPECT_TRUE(AreEntryMapsEqual(db_model_, expected_model));

  EXPECT_FALSE(store_->UpdateEntry(GetSampleEntry(0)));

  ArticleEntry updated_entry(GetSampleEntry(0));
  updated_entry.set_title("updated title.");
  EXPECT_TRUE(store_->UpdateEntry(updated_entry));
  expected_model.clear();
  AddEntry(updated_entry, &expected_model);

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), expected_model));
  EXPECT_TRUE(AreEntryMapsEqual(db_model_, expected_model));

  store_->RemoveEntry(updated_entry);
  EXPECT_FALSE(store_->UpdateEntry(updated_entry));
  EXPECT_FALSE(store_->UpdateEntry(GetSampleEntry(0)));
}

TEST_F(DomDistillerStoreTest, TestSyncMergeWithEmptyDatabase) {
  AddEntry(GetSampleEntry(0), &sync_model_);
  AddEntry(GetSampleEntry(1), &sync_model_);
  AddEntry(GetSampleEntry(2), &sync_model_);

  CreateStore();
  fake_db_->InitCallback(true);
  fake_db_->LoadCallback(true);

  StartSyncing();

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), sync_model_));
  EXPECT_TRUE(AreEntryMapsEqual(db_model_, sync_model_));
}

TEST_F(DomDistillerStoreTest, TestSyncMergeAfterDatabaseLoad) {
  AddEntry(GetSampleEntry(0), &db_model_);
  AddEntry(GetSampleEntry(1), &db_model_);
  AddEntry(GetSampleEntry(2), &db_model_);

  AddEntry(GetSampleEntry(2), &sync_model_);
  AddEntry(GetSampleEntry(3), &sync_model_);
  AddEntry(GetSampleEntry(4), &sync_model_);

  EntryMap expected_model(db_model_);
  AddEntry(GetSampleEntry(3), &expected_model);
  AddEntry(GetSampleEntry(4), &expected_model);

  CreateStore();
  fake_db_->InitCallback(true);
  fake_db_->LoadCallback(true);

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), db_model_));

  StartSyncing();

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), expected_model));
  EXPECT_TRUE(AreEntryMapsEqual(db_model_, expected_model));
  EXPECT_TRUE(AreEntryMapsEqual(sync_model_, expected_model));
}

TEST_F(DomDistillerStoreTest, TestDatabaseLoadAfterSyncMerge) {
  AddEntry(GetSampleEntry(0), &db_model_);
  AddEntry(GetSampleEntry(1), &db_model_);
  AddEntry(GetSampleEntry(2), &db_model_);

  AddEntry(GetSampleEntry(2), &sync_model_);
  AddEntry(GetSampleEntry(3), &sync_model_);
  AddEntry(GetSampleEntry(4), &sync_model_);

  EntryMap expected_model(db_model_);
  AddEntry(GetSampleEntry(3), &expected_model);
  AddEntry(GetSampleEntry(4), &expected_model);

  CreateStore();
  StartSyncing();

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), sync_model_));

  fake_db_->InitCallback(true);
  fake_db_->LoadCallback(true);

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), expected_model));
  EXPECT_TRUE(AreEntryMapsEqual(db_model_, expected_model));
  EXPECT_TRUE(AreEntryMapsEqual(sync_model_, expected_model));
}

TEST_F(DomDistillerStoreTest, TestGetAllSyncData) {
  AddEntry(GetSampleEntry(0), &db_model_);
  AddEntry(GetSampleEntry(1), &db_model_);
  AddEntry(GetSampleEntry(2), &db_model_);

  AddEntry(GetSampleEntry(2), &sync_model_);
  AddEntry(GetSampleEntry(3), &sync_model_);
  AddEntry(GetSampleEntry(4), &sync_model_);

  EntryMap expected_model(db_model_);
  AddEntry(GetSampleEntry(3), &expected_model);
  AddEntry(GetSampleEntry(4), &expected_model);

  CreateStore();

  fake_db_->InitCallback(true);
  fake_db_->LoadCallback(true);

  StartSyncing();

  SyncDataList data = store_->GetAllSyncData(kDomDistillerModelType);
  DomDistillerStore::EntryVector entries;
  for (SyncDataList::iterator it = data.begin(); it != data.end(); ++it) {
    entries.push_back(EntryFromSpecifics(it->GetSpecifics()));
  }
  EXPECT_TRUE(AreEntriesEqual(entries, expected_model));
}

TEST_F(DomDistillerStoreTest, TestProcessSyncChanges) {
  AddEntry(GetSampleEntry(0), &db_model_);
  AddEntry(GetSampleEntry(1), &db_model_);
  sync_model_ = db_model_;

  EntryMap expected_model(db_model_);
  AddEntry(GetSampleEntry(2), &expected_model);
  AddEntry(GetSampleEntry(3), &expected_model);

  CreateStore();

  fake_db_->InitCallback(true);
  fake_db_->LoadCallback(true);

  StartSyncing();

  SyncChangeList changes;
  changes.push_back(SyncChange(FROM_HERE, SyncChange::ACTION_ADD,
                               CreateSyncData(GetSampleEntry(2))));
  changes.push_back(SyncChange(FROM_HERE, SyncChange::ACTION_ADD,
                               CreateSyncData(GetSampleEntry(3))));

  store_->ProcessSyncChanges(FROM_HERE, changes);

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), expected_model));
  EXPECT_TRUE(AreEntryMapsEqual(db_model_, expected_model));
}

TEST_F(DomDistillerStoreTest, TestSyncMergeWithSecondDomDistillerStore) {
  AddEntry(GetSampleEntry(0), &db_model_);
  AddEntry(GetSampleEntry(1), &db_model_);
  AddEntry(GetSampleEntry(2), &db_model_);

  EntryMap other_db_model;
  AddEntry(GetSampleEntry(2), &other_db_model);
  AddEntry(GetSampleEntry(3), &other_db_model);
  AddEntry(GetSampleEntry(4), &other_db_model);

  EntryMap expected_model(db_model_);
  AddEntry(GetSampleEntry(3), &expected_model);
  AddEntry(GetSampleEntry(4), &expected_model);

  CreateStore();

  fake_db_->InitCallback(true);
  fake_db_->LoadCallback(true);

  FakeDB<ArticleEntry>* other_fake_db =
      new FakeDB<ArticleEntry>(&other_db_model);
  std::unique_ptr<DomDistillerStore> owned_other_store(new DomDistillerStore(
      std::unique_ptr<leveldb_proto::ProtoDatabase<ArticleEntry>>(
          other_fake_db),
      std::vector<ArticleEntry>(),
      base::FilePath(FILE_PATH_LITERAL("/fake/other/path"))));
  DomDistillerStore* other_store = owned_other_store.get();
  other_fake_db->InitCallback(true);
  other_fake_db->LoadCallback(true);

  EXPECT_FALSE(AreEntriesEqual(store_->GetEntries(), expected_model));
  EXPECT_FALSE(AreEntriesEqual(other_store->GetEntries(), expected_model));
  ASSERT_TRUE(AreEntriesEqual(other_store->GetEntries(), other_db_model));

  FakeSyncErrorFactory* other_error_factory = new FakeSyncErrorFactory();
  store_->MergeDataAndStartSyncing(
      kDomDistillerModelType, SyncDataFromEntryMap(other_db_model),
      std::move(owned_other_store),
      base::WrapUnique<SyncErrorFactory>(other_error_factory));

  EXPECT_TRUE(AreEntriesEqual(store_->GetEntries(), expected_model));
  EXPECT_TRUE(AreEntriesEqual(other_store->GetEntries(), expected_model));
}

TEST_F(DomDistillerStoreTest, TestObserver) {
  CreateStore();
  MockDistillerObserver observer;
  store_->AddObserver(&observer);
  fake_db_->InitCallback(true);
  fake_db_->LoadCallback(true);
  std::vector<DomDistillerObserver::ArticleUpdate> expected_updates;
  DomDistillerObserver::ArticleUpdate update;
  update.entry_id = GetSampleEntry(0).entry_id();
  update.update_type = DomDistillerObserver::ArticleUpdate::ADD;
  expected_updates.push_back(update);
  EXPECT_CALL(observer, ArticleEntriesUpdated(
                            test::util::HasExpectedUpdates(expected_updates)));
  store_->AddEntry(GetSampleEntry(0));

  expected_updates.clear();
  update.entry_id = GetSampleEntry(1).entry_id();
  update.update_type = DomDistillerObserver::ArticleUpdate::ADD;
  expected_updates.push_back(update);
  EXPECT_CALL(observer, ArticleEntriesUpdated(
                            test::util::HasExpectedUpdates(expected_updates)));
  store_->AddEntry(GetSampleEntry(1));

  expected_updates.clear();
  update.entry_id = GetSampleEntry(0).entry_id();
  update.update_type = DomDistillerObserver::ArticleUpdate::REMOVE;
  expected_updates.clear();
  expected_updates.push_back(update);
  EXPECT_CALL(observer, ArticleEntriesUpdated(
                            test::util::HasExpectedUpdates(expected_updates)));
  store_->RemoveEntry(GetSampleEntry(0));

  // Add entry_id = 3 and update entry_id = 1.
  expected_updates.clear();
  SyncDataList change_data;
  change_data.push_back(CreateSyncData(GetSampleEntry(3)));
  ArticleEntry updated_entry(GetSampleEntry(1));
  updated_entry.set_title("changed_title");
  change_data.push_back(CreateSyncData(updated_entry));
  update.entry_id = GetSampleEntry(3).entry_id();
  update.update_type = DomDistillerObserver::ArticleUpdate::ADD;
  expected_updates.push_back(update);
  update.entry_id = GetSampleEntry(1).entry_id();
  update.update_type = DomDistillerObserver::ArticleUpdate::UPDATE;
  expected_updates.push_back(update);
  EXPECT_CALL(observer, ArticleEntriesUpdated(
                            test::util::HasExpectedUpdates(expected_updates)));

  FakeSyncErrorFactory* fake_error_factory = new FakeSyncErrorFactory();
  EntryMap fake_model;
  FakeSyncChangeProcessor* fake_sync_change_processor =
      new FakeSyncChangeProcessor(&fake_model);
  store_->MergeDataAndStartSyncing(
      kDomDistillerModelType, change_data,
      base::WrapUnique<SyncChangeProcessor>(fake_sync_change_processor),
      base::WrapUnique<SyncErrorFactory>(fake_error_factory));
}

}  // namespace dom_distiller
