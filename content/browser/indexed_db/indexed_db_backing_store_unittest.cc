// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_backing_store.h"

#include <stddef.h>
#include <stdint.h>
#include <utility>

#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/default_clock.h"
#include "content/browser/indexed_db/indexed_db_class_factory.h"
#include "content/browser/indexed_db/indexed_db_context_impl.h"
#include "content/browser/indexed_db/indexed_db_factory_impl.h"
#include "content/browser/indexed_db/indexed_db_leveldb_coding.h"
#include "content/browser/indexed_db/indexed_db_leveldb_operations.h"
#include "content/browser/indexed_db/indexed_db_metadata_coding.h"
#include "content/browser/indexed_db/indexed_db_value.h"
#include "content/browser/indexed_db/leveldb/leveldb_factory.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "net/url_request/url_request_test_util.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/quota/special_storage_policy.h"
#include "storage/browser/test/mock_quota_manager_proxy.h"
#include "storage/browser/test/mock_special_storage_policy.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"

using base::ASCIIToUTF16;
using url::Origin;

namespace content {
namespace indexed_db_backing_store_unittest {

static const size_t kDefaultMaxOpenIteratorsPerDatabase = 50;

// Write |content| to |file|. Returns true on success.
bool WriteFile(const base::FilePath& file, base::StringPiece content) {
  int write_size = base::WriteFile(file, content.data(), content.length());
  return write_size >= 0 && write_size == static_cast<int>(content.length());
}

class Comparator : public LevelDBComparator {
 public:
  int Compare(const base::StringPiece& a,
              const base::StringPiece& b) const override {
    return content::Compare(a, b, false /*index_keys*/);
  }
  const char* Name() const override { return "idb_cmp1"; }
};

class DefaultLevelDBFactory : public LevelDBFactory {
 public:
  DefaultLevelDBFactory() {}

  leveldb::Status OpenLevelDB(const base::FilePath& file_name,
                              const LevelDBComparator* comparator,
                              std::unique_ptr<LevelDBDatabase>* db,
                              bool* is_disk_full) override {
    return LevelDBDatabase::Open(file_name, comparator,
                                 kDefaultMaxOpenIteratorsPerDatabase, db,
                                 is_disk_full);
  }
  leveldb::Status DestroyLevelDB(const base::FilePath& file_name) override {
    return LevelDBDatabase::Destroy(file_name);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultLevelDBFactory);
};

class TestableIndexedDBBackingStore : public IndexedDBBackingStore {
 public:
  static scoped_refptr<TestableIndexedDBBackingStore> Open(
      IndexedDBFactory* indexed_db_factory,
      const Origin& origin,
      const base::FilePath& path_base,
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      LevelDBFactory* leveldb_factory,
      base::SequencedTaskRunner* task_runner,
      leveldb::Status* status) {
    DCHECK(!path_base.empty());

    std::unique_ptr<LevelDBComparator> comparator =
        std::make_unique<Comparator>();

    if (!base::CreateDirectory(path_base)) {
      *status = leveldb::Status::IOError("Unable to create base dir");
      return scoped_refptr<TestableIndexedDBBackingStore>();
    }

    const base::FilePath file_path = path_base.AppendASCII("test_db_path");
    const base::FilePath blob_path = path_base.AppendASCII("test_blob_path");

    std::unique_ptr<LevelDBDatabase> db;
    bool is_disk_full = false;
    *status = leveldb_factory->OpenLevelDB(
        file_path, comparator.get(), &db, &is_disk_full);

    if (!db || !status->ok())
      return scoped_refptr<TestableIndexedDBBackingStore>();

    scoped_refptr<TestableIndexedDBBackingStore> backing_store(
        new TestableIndexedDBBackingStore(indexed_db_factory, origin, blob_path,
                                          request_context_getter, std::move(db),
                                          std::move(comparator), task_runner));

    *status = backing_store->SetUpMetadata();
    if (!status->ok())
      return scoped_refptr<TestableIndexedDBBackingStore>();

    return backing_store;
  }

  const std::vector<IndexedDBBackingStore::Transaction::WriteDescriptor>&
  writes() const {
    return writes_;
  }
  void ClearWrites() { writes_.clear(); }
  const std::vector<int64_t>& removals() const { return removals_; }
  void ClearRemovals() { removals_.clear(); }

  void StartJournalCleaningTimer() override {
    IndexedDBBackingStore::StartJournalCleaningTimer();
  }

 protected:
  ~TestableIndexedDBBackingStore() override {}

  bool WriteBlobFile(
      int64_t database_id,
      const Transaction::WriteDescriptor& descriptor,
      Transaction::ChainedBlobWriter* chained_blob_writer) override {
    if (KeyPrefix::IsValidDatabaseId(database_id_)) {
      if (database_id_ != database_id) {
        return false;
      }
    } else {
      database_id_ = database_id;
    }
    writes_.push_back(descriptor);
    task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&Transaction::ChainedBlobWriter::ReportWriteCompletion,
                       chained_blob_writer, true, 1));
    return true;
  }

  bool RemoveBlobFile(int64_t database_id, int64_t key) const override {
    if (database_id_ != database_id ||
        !KeyPrefix::IsValidDatabaseId(database_id)) {
      return false;
    }
    removals_.push_back(key);
    return true;
  }

 private:
  TestableIndexedDBBackingStore(
      IndexedDBFactory* indexed_db_factory,
      const Origin& origin,
      const base::FilePath& blob_path,
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      std::unique_ptr<LevelDBDatabase> db,
      std::unique_ptr<LevelDBComparator> comparator,
      base::SequencedTaskRunner* task_runner)
      : IndexedDBBackingStore(indexed_db_factory,
                              origin,
                              blob_path,
                              request_context_getter,
                              std::move(db),
                              std::move(comparator),
                              task_runner),
        database_id_(0) {}

  int64_t database_id_;
  std::vector<Transaction::WriteDescriptor> writes_;

  // This is modified in an overridden virtual function that is properly const
  // in the real implementation, therefore must be mutable here.
  mutable std::vector<int64_t> removals_;

  DISALLOW_COPY_AND_ASSIGN(TestableIndexedDBBackingStore);
};

class TestIDBFactory : public IndexedDBFactoryImpl {
 public:
  explicit TestIDBFactory(IndexedDBContextImpl* idb_context)
      : IndexedDBFactoryImpl(idb_context, base::DefaultClock::GetInstance()) {}

  scoped_refptr<TestableIndexedDBBackingStore> OpenBackingStoreForTest(
      const Origin& origin,
      scoped_refptr<net::URLRequestContextGetter> url_request_context_getter) {
    IndexedDBDataLossInfo data_loss_info;
    bool disk_full;
    leveldb::Status status;
    scoped_refptr<IndexedDBBackingStore> backing_store = OpenBackingStore(
        origin, context()->data_path(), url_request_context_getter,
        &data_loss_info, &disk_full, &status);
    scoped_refptr<TestableIndexedDBBackingStore> testable_store =
        static_cast<TestableIndexedDBBackingStore*>(backing_store.get());
    return testable_store;
  }

 protected:
  ~TestIDBFactory() override {}

  scoped_refptr<IndexedDBBackingStore> OpenBackingStoreHelper(
      const Origin& origin,
      const base::FilePath& data_directory,
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      IndexedDBDataLossInfo* data_loss_info,
      bool* disk_full,
      bool first_time,
      leveldb::Status* status) override {
    DefaultLevelDBFactory leveldb_factory;
    return TestableIndexedDBBackingStore::Open(
        this, origin, data_directory, request_context_getter, &leveldb_factory,
        context()->TaskRunner(), status);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestIDBFactory);
};

class IndexedDBBackingStoreTest : public testing::Test {
 public:
  IndexedDBBackingStoreTest()
      : url_request_context_getter_(
            base::MakeRefCounted<net::TestURLRequestContextGetter>(
                BrowserThread::GetTaskRunnerForThread(BrowserThread::UI))),
        special_storage_policy_(
            base::MakeRefCounted<MockSpecialStoragePolicy>()),
        quota_manager_proxy_(
            base::MakeRefCounted<MockQuotaManagerProxy>(nullptr, nullptr)) {}

  void CreateFactoryAndBackingStore() {
    // Factory and backing store must be created on IDB task runner.
    idb_context_->TaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](IndexedDBBackingStoreTest* test) {
              const Origin origin = Origin::Create(GURL("http://localhost:81"));
              test->idb_factory_ = base::MakeRefCounted<TestIDBFactory>(
                  test->idb_context_.get());
              test->backing_store_ =
                  test->idb_factory_->OpenBackingStoreForTest(
                      origin, test->url_request_context_getter_);
            },
            base::Unretained(this)));
    RunAllTasksUntilIdle();
  }

  void DestroyFactoryAndBackingStore() {
    // Factory and backing store must be destroyed on IDB task runner.
    idb_context_->TaskRunner()->PostTask(
        FROM_HERE, base::BindOnce(
                       [](IndexedDBBackingStoreTest* test) {
                         test->idb_factory_ = nullptr;
                         test->backing_store_ = nullptr;
                       },
                       base::Unretained(this)));
    RunAllTasksUntilIdle();
  }

  void SetUp() override {
    special_storage_policy_->SetAllUnlimited(true);
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    idb_context_ = base::MakeRefCounted<IndexedDBContextImpl>(
        temp_dir_.GetPath(), special_storage_policy_, quota_manager_proxy_);

    CreateFactoryAndBackingStore();

    // useful keys and values during tests
    value1_ = IndexedDBValue("value1", std::vector<IndexedDBBlobInfo>());
    value2_ = IndexedDBValue("value2", std::vector<IndexedDBBlobInfo>());

    key1_ = IndexedDBKey(99, blink::kWebIDBKeyTypeNumber);
    key2_ = IndexedDBKey(ASCIIToUTF16("key2"));
  }

  void TearDown() override {
    DestroyFactoryAndBackingStore();

    quota_manager_proxy_->SimulateQuotaManagerDestroyed();
  }

  TestableIndexedDBBackingStore* backing_store() const {
    return backing_store_.get();
  }

  // Sample keys and values that are consistent. Public so that posted lambdas
  // passed |this| can access them.
  IndexedDBKey key1_;
  IndexedDBKey key2_;
  IndexedDBValue value1_;
  IndexedDBValue value2_;

 protected:
  // Must be initialized before url_request_context_getter_
  TestBrowserThreadBundle thread_bundle_;

  base::ScopedTempDir temp_dir_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  scoped_refptr<MockSpecialStoragePolicy> special_storage_policy_;
  scoped_refptr<MockQuotaManagerProxy> quota_manager_proxy_;
  scoped_refptr<IndexedDBContextImpl> idb_context_;
  scoped_refptr<TestIDBFactory> idb_factory_;

  scoped_refptr<TestableIndexedDBBackingStore> backing_store_;

 private:
  DISALLOW_COPY_AND_ASSIGN(IndexedDBBackingStoreTest);
};

class IndexedDBBackingStoreTestWithBlobs : public IndexedDBBackingStoreTest {
 public:
  IndexedDBBackingStoreTestWithBlobs() {}

  void SetUp() override {
    IndexedDBBackingStoreTest::SetUp();

    // useful keys and values during tests
    blob_info_.push_back(
        IndexedDBBlobInfo("uuid 3", base::UTF8ToUTF16("blob type"), 1));
    blob_info_.push_back(IndexedDBBlobInfo(
        "uuid 4", base::FilePath(FILE_PATH_LITERAL("path/to/file")),
        base::UTF8ToUTF16("file name"), base::UTF8ToUTF16("file type")));
    blob_info_.push_back(IndexedDBBlobInfo("uuid 5", base::FilePath(),
                                           base::UTF8ToUTF16("file name"),
                                           base::UTF8ToUTF16("file type")));
    value3_ = IndexedDBValue("value3", blob_info_);

    key3_ = IndexedDBKey(ASCIIToUTF16("key3"));
  }

  // This just checks the data that survive getting stored and recalled, e.g.
  // the file path and UUID will change and thus aren't verified.
  bool CheckBlobInfoMatches(const std::vector<IndexedDBBlobInfo>& reads) const {
    DCHECK(idb_context_->TaskRunner()->RunsTasksInCurrentSequence());

    if (blob_info_.size() != reads.size())
      return false;
    for (size_t i = 0; i < blob_info_.size(); ++i) {
      const IndexedDBBlobInfo& a = blob_info_[i];
      const IndexedDBBlobInfo& b = reads[i];
      if (a.is_file() != b.is_file())
        return false;
      if (a.type() != b.type())
        return false;
      if (a.is_file()) {
        if (a.file_name() != b.file_name())
          return false;
      } else {
        if (a.size() != b.size())
          return false;
      }
    }
    return true;
  }

  bool CheckBlobReadsMatchWrites(
      const std::vector<IndexedDBBlobInfo>& reads) const {
    DCHECK(idb_context_->TaskRunner()->RunsTasksInCurrentSequence());

    if (backing_store_->writes().size() != reads.size())
      return false;
    std::set<int64_t> ids;
    for (const auto& write : backing_store_->writes())
      ids.insert(write.key());
    if (ids.size() != backing_store_->writes().size())
      return false;
    for (const auto& read : reads) {
      if (ids.count(read.key()) != 1)
        return false;
    }
    return true;
  }

  bool CheckBlobWrites() const {
    DCHECK(idb_context_->TaskRunner()->RunsTasksInCurrentSequence());

    if (backing_store_->writes().size() != blob_info_.size())
      return false;
    for (size_t i = 0; i < backing_store_->writes().size(); ++i) {
      const IndexedDBBackingStore::Transaction::WriteDescriptor& desc =
          backing_store_->writes()[i];
      const IndexedDBBlobInfo& info = blob_info_[i];
      if (desc.is_file() != info.is_file()) {
        if (!info.is_file() || !info.file_path().empty())
          return false;
      } else if (desc.is_file()) {
        if (desc.file_path() != info.file_path())
          return false;
      } else {
        if (desc.url() != GURL("blob:uuid/" + info.uuid()))
          return false;
      }
    }
    return true;
  }

  bool CheckBlobRemovals() const {
    DCHECK(idb_context_->TaskRunner()->RunsTasksInCurrentSequence());

    if (backing_store_->removals().size() != backing_store_->writes().size())
      return false;
    for (size_t i = 0; i < backing_store_->writes().size(); ++i) {
      if (backing_store_->writes()[i].key() != backing_store_->removals()[i])
        return false;
    }
    return true;
  }

  // Sample keys and values that are consistent. Public so that posted lambdas
  // passed |this| can access them.
  IndexedDBKey key3_;
  IndexedDBValue value3_;

 private:
  // Blob details referenced by |value3_|. The various CheckBlob*() methods
  // can be used to verify the state as a test progresses.
  std::vector<IndexedDBBlobInfo> blob_info_;

  DISALLOW_COPY_AND_ASSIGN(IndexedDBBackingStoreTestWithBlobs);
};

class TestCallback : public IndexedDBBackingStore::BlobWriteCallback {
 public:
  TestCallback() : called(false), succeeded(false) {}
  leveldb::Status Run(IndexedDBBackingStore::BlobWriteResult result) override {
    called = true;
    switch (result) {
      case IndexedDBBackingStore::BlobWriteResult::FAILURE_ASYNC:
        succeeded = false;
        break;
      case IndexedDBBackingStore::BlobWriteResult::SUCCESS_ASYNC:
      case IndexedDBBackingStore::BlobWriteResult::SUCCESS_SYNC:
        succeeded = true;
        break;
    }
    return leveldb::Status::OK();
  }
  bool called;
  bool succeeded;

 protected:
  ~TestCallback() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestCallback);
};

TEST_F(IndexedDBBackingStoreTest, PutGetConsistency) {
  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStore* backing_store, IndexedDBKey key,
             IndexedDBValue value) {
            {
              IndexedDBBackingStore::Transaction transaction1(backing_store);
              transaction1.Begin();
              std::vector<std::unique_ptr<storage::BlobDataHandle>> handles;
              IndexedDBBackingStore::RecordIdentifier record;
              leveldb::Status s = backing_store->PutRecord(
                  &transaction1, 1, 1, key, &value, &handles, &record);
              EXPECT_TRUE(s.ok());
              scoped_refptr<TestCallback> callback(
                  base::MakeRefCounted<TestCallback>());
              EXPECT_TRUE(transaction1.CommitPhaseOne(callback).ok());
              EXPECT_TRUE(callback->called);
              EXPECT_TRUE(callback->succeeded);
              EXPECT_TRUE(transaction1.CommitPhaseTwo().ok());
            }

            {
              IndexedDBBackingStore::Transaction transaction2(backing_store);
              transaction2.Begin();
              IndexedDBValue result_value;
              EXPECT_TRUE(
                  backing_store
                      ->GetRecord(&transaction2, 1, 1, key, &result_value)
                      .ok());
              scoped_refptr<TestCallback> callback(
                  base::MakeRefCounted<TestCallback>());
              EXPECT_TRUE(transaction2.CommitPhaseOne(callback).ok());
              EXPECT_TRUE(callback->called);
              EXPECT_TRUE(callback->succeeded);
              EXPECT_TRUE(transaction2.CommitPhaseTwo().ok());
              EXPECT_EQ(value.bits, result_value.bits);
            }
          },
          base::Unretained(backing_store()), key1_, value1_));
  RunAllTasksUntilIdle();
}

TEST_F(IndexedDBBackingStoreTestWithBlobs, PutGetConsistencyWithBlobs) {
  struct TestState {
    std::unique_ptr<IndexedDBBackingStore::Transaction> transaction1;
    scoped_refptr<TestCallback> callback1;
    std::unique_ptr<IndexedDBBackingStore::Transaction> transaction3;
    scoped_refptr<TestCallback> callback3;
  } state;

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            // Initiate transaction1 - writing blobs.
            state->transaction1 =
                std::make_unique<IndexedDBBackingStore::Transaction>(
                    test->backing_store());
            state->transaction1->Begin();
            std::vector<std::unique_ptr<storage::BlobDataHandle>> handles;
            IndexedDBBackingStore::RecordIdentifier record;
            EXPECT_TRUE(test->backing_store()
                            ->PutRecord(state->transaction1.get(), 1, 1,
                                        test->key3_, &test->value3_, &handles,
                                        &record)
                            .ok());
            state->callback1 = base::MakeRefCounted<TestCallback>();
            EXPECT_TRUE(
                state->transaction1->CommitPhaseOne(state->callback1).ok());
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            // Finish up transaction1, verifying blob writes.
            EXPECT_TRUE(state->callback1->called);
            EXPECT_TRUE(state->callback1->succeeded);
            EXPECT_TRUE(test->CheckBlobWrites());
            EXPECT_TRUE(state->transaction1->CommitPhaseTwo().ok());

            // Initiate transaction2, reading blobs.
            IndexedDBBackingStore::Transaction transaction2(
                test->backing_store());
            transaction2.Begin();
            IndexedDBValue result_value;
            EXPECT_TRUE(
                test->backing_store()
                    ->GetRecord(&transaction2, 1, 1, test->key3_, &result_value)
                    .ok());

            // Finish up transaction2, verifying blob reads.
            scoped_refptr<TestCallback> callback(
                base::MakeRefCounted<TestCallback>());
            EXPECT_TRUE(transaction2.CommitPhaseOne(callback).ok());
            EXPECT_TRUE(callback->called);
            EXPECT_TRUE(callback->succeeded);
            EXPECT_TRUE(transaction2.CommitPhaseTwo().ok());
            EXPECT_EQ(test->value3_.bits, result_value.bits);
            EXPECT_TRUE(test->CheckBlobInfoMatches(result_value.blob_info));
            EXPECT_TRUE(
                test->CheckBlobReadsMatchWrites(result_value.blob_info));

            // Initiate transaction3, deleting blobs.
            state->transaction3 =
                std::make_unique<IndexedDBBackingStore::Transaction>(
                    test->backing_store());
            state->transaction3->Begin();
            EXPECT_TRUE(test->backing_store()
                            ->DeleteRange(state->transaction3.get(), 1, 1,
                                          IndexedDBKeyRange(test->key3_))
                            .ok());
            state->callback3 = base::MakeRefCounted<TestCallback>();
            EXPECT_TRUE(
                state->transaction3->CommitPhaseOne(state->callback3).ok());

          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            // Finish up transaction 3, verifying blob deletes.
            EXPECT_TRUE(state->transaction3->CommitPhaseTwo().ok());
            EXPECT_TRUE(test->CheckBlobRemovals());

            // Clean up Transactions, etc on the IDB thread.
            *state = TestState();
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();
}

TEST_F(IndexedDBBackingStoreTest, DeleteRange) {
  const std::vector<IndexedDBKey> keys = {
      IndexedDBKey(ASCIIToUTF16("key0")), IndexedDBKey(ASCIIToUTF16("key1")),
      IndexedDBKey(ASCIIToUTF16("key2")), IndexedDBKey(ASCIIToUTF16("key3"))};
  const IndexedDBKeyRange ranges[] = {
      IndexedDBKeyRange(keys[1], keys[2], false, false),
      IndexedDBKeyRange(keys[1], keys[2], false, false),
      IndexedDBKeyRange(keys[0], keys[2], true, false),
      IndexedDBKeyRange(keys[1], keys[3], false, true),
      IndexedDBKeyRange(keys[0], keys[3], true, true)};

  for (size_t i = 0; i < arraysize(ranges); ++i) {
    const int64_t database_id = 1;
    const int64_t object_store_id = i + 1;
    const IndexedDBKeyRange& range = ranges[i];

    struct TestState {
      std::unique_ptr<IndexedDBBackingStore::Transaction> transaction1;
      scoped_refptr<TestCallback> callback1;
      std::unique_ptr<IndexedDBBackingStore::Transaction> transaction2;
      scoped_refptr<TestCallback> callback2;
    } state;

    idb_context_->TaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](TestableIndexedDBBackingStore* backing_store, TestState* state,
               const std::vector<IndexedDBKey>& keys, int64_t database_id,
               int64_t object_store_id) {
              // Reset from previous iteration.
              backing_store->ClearWrites();
              backing_store->ClearRemovals();

              std::vector<IndexedDBValue> values = {
                  IndexedDBValue(
                      "value0", {IndexedDBBlobInfo(
                                    "uuid 0", base::UTF8ToUTF16("type 0"), 1)}),
                  IndexedDBValue(
                      "value1", {IndexedDBBlobInfo(
                                    "uuid 1", base::UTF8ToUTF16("type 1"), 1)}),
                  IndexedDBValue(
                      "value2", {IndexedDBBlobInfo(
                                    "uuid 2", base::UTF8ToUTF16("type 2"), 1)}),
                  IndexedDBValue(
                      "value3",
                      {IndexedDBBlobInfo("uuid 3", base::UTF8ToUTF16("type 3"),
                                         1)})};
              ASSERT_GE(keys.size(), values.size());

              // Initiate transaction1 - write records.
              state->transaction1 =
                  std::make_unique<IndexedDBBackingStore::Transaction>(
                      backing_store);
              state->transaction1->Begin();
              std::vector<std::unique_ptr<storage::BlobDataHandle>> handles;
              IndexedDBBackingStore::RecordIdentifier record;
              for (size_t i = 0; i < values.size(); ++i) {
                EXPECT_TRUE(backing_store
                                ->PutRecord(state->transaction1.get(),
                                            database_id, object_store_id,
                                            keys[i], &values[i], &handles,
                                            &record)
                                .ok());
              }

              // Start committing transaction1.
              state->callback1 = base::MakeRefCounted<TestCallback>();
              EXPECT_TRUE(
                  state->transaction1->CommitPhaseOne(state->callback1).ok());
            },
            base::Unretained(backing_store()), base::Unretained(&state),
            base::ConstRef(keys), database_id, object_store_id));
    RunAllTasksUntilIdle();

    idb_context_->TaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](TestableIndexedDBBackingStore* backing_store, TestState* state,
               IndexedDBKeyRange range, int64_t database_id,
               int64_t object_store_id) {
              // Finish committing transaction1.
              EXPECT_TRUE(state->callback1->called);
              EXPECT_TRUE(state->callback1->succeeded);
              EXPECT_TRUE(state->transaction1->CommitPhaseTwo().ok());

              // Initiate transaction 2 - delete range.
              state->transaction2 =
                  std::make_unique<IndexedDBBackingStore::Transaction>(
                      backing_store);
              state->transaction2->Begin();
              IndexedDBValue result_value;
              EXPECT_TRUE(backing_store
                              ->DeleteRange(state->transaction2.get(),
                                            database_id, object_store_id, range)
                              .ok());

              // Start committing transaction2.
              state->callback2 = base::MakeRefCounted<TestCallback>();
              EXPECT_TRUE(
                  state->transaction2->CommitPhaseOne(state->callback2).ok());
            },
            base::Unretained(backing_store()), base::Unretained(&state), range,
            database_id, object_store_id));
    RunAllTasksUntilIdle();

    idb_context_->TaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](TestableIndexedDBBackingStore* backing_store, TestState* state) {
              // Finish committing transaction2.
              EXPECT_TRUE(state->callback2->called);
              EXPECT_TRUE(state->callback2->succeeded);
              EXPECT_TRUE(state->transaction2->CommitPhaseTwo().ok());

              // Verify blob removals.
              ASSERT_EQ(2UL, backing_store->removals().size());
              EXPECT_EQ(backing_store->writes()[1].key(),
                        backing_store->removals()[0]);
              EXPECT_EQ(backing_store->writes()[2].key(),
                        backing_store->removals()[1]);

              // Clean up Transactions, etc on the IDB thread.
              *state = TestState();
            },
            base::Unretained(backing_store()), base::Unretained(&state)));
    RunAllTasksUntilIdle();
  }
}

TEST_F(IndexedDBBackingStoreTest, DeleteRangeEmptyRange) {
  const std::vector<IndexedDBKey> keys = {
      IndexedDBKey(ASCIIToUTF16("key0")), IndexedDBKey(ASCIIToUTF16("key1")),
      IndexedDBKey(ASCIIToUTF16("key2")), IndexedDBKey(ASCIIToUTF16("key3")),
      IndexedDBKey(ASCIIToUTF16("key4"))};
  const IndexedDBKeyRange ranges[] = {
      IndexedDBKeyRange(keys[3], keys[4], true, false),
      IndexedDBKeyRange(keys[2], keys[1], false, false),
      IndexedDBKeyRange(keys[2], keys[1], true, true)};

  for (size_t i = 0; i < arraysize(ranges); ++i) {
    const int64_t database_id = 1;
    const int64_t object_store_id = i + 1;
    const IndexedDBKeyRange& range = ranges[i];

    struct TestState {
      std::unique_ptr<IndexedDBBackingStore::Transaction> transaction1;
      scoped_refptr<TestCallback> callback1;
      std::unique_ptr<IndexedDBBackingStore::Transaction> transaction2;
      scoped_refptr<TestCallback> callback2;
    } state;

    idb_context_->TaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](TestableIndexedDBBackingStore* backing_store, TestState* state,
               const std::vector<IndexedDBKey>& keys, int64_t database_id,
               int64_t object_store_id) {
              // Reset from previous iteration.
              backing_store->ClearWrites();
              backing_store->ClearRemovals();

              std::vector<IndexedDBValue> values = {
                  IndexedDBValue(
                      "value0", {IndexedDBBlobInfo(
                                    "uuid 0", base::UTF8ToUTF16("type 0"), 1)}),
                  IndexedDBValue(
                      "value1", {IndexedDBBlobInfo(
                                    "uuid 1", base::UTF8ToUTF16("type 1"), 1)}),
                  IndexedDBValue(
                      "value2", {IndexedDBBlobInfo(
                                    "uuid 2", base::UTF8ToUTF16("type 2"), 1)}),
                  IndexedDBValue(
                      "value3",
                      {IndexedDBBlobInfo("uuid 3", base::UTF8ToUTF16("type 3"),
                                         1)})};
              ASSERT_GE(keys.size(), values.size());

              // Initiate transaction1 - write records.
              state->transaction1 =
                  std::make_unique<IndexedDBBackingStore::Transaction>(
                      backing_store);
              state->transaction1->Begin();

              std::vector<std::unique_ptr<storage::BlobDataHandle>> handles;
              IndexedDBBackingStore::RecordIdentifier record;
              for (size_t i = 0; i < values.size(); ++i) {
                EXPECT_TRUE(backing_store
                                ->PutRecord(state->transaction1.get(),
                                            database_id, object_store_id,
                                            keys[i], &values[i], &handles,
                                            &record)
                                .ok());
              }
              // Start committing transaction1.
              state->callback1 = base::MakeRefCounted<TestCallback>();
              EXPECT_TRUE(
                  state->transaction1->CommitPhaseOne(state->callback1).ok());
            },
            base::Unretained(backing_store()), base::Unretained(&state),
            base::ConstRef(keys), database_id, object_store_id));
    RunAllTasksUntilIdle();

    idb_context_->TaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](TestableIndexedDBBackingStore* backing_store, TestState* state,
               IndexedDBKeyRange range, int64_t database_id,
               int64_t object_store_id) {
              // Finish committing transaction1.
              EXPECT_TRUE(state->callback1->called);
              EXPECT_TRUE(state->callback1->succeeded);
              EXPECT_TRUE(state->transaction1->CommitPhaseTwo().ok());

              // Initiate transaction 2 - delete range.
              state->transaction2 =
                  std::make_unique<IndexedDBBackingStore::Transaction>(
                      backing_store);
              state->transaction2->Begin();
              IndexedDBValue result_value;
              EXPECT_TRUE(backing_store
                              ->DeleteRange(state->transaction2.get(),
                                            database_id, object_store_id, range)
                              .ok());

              // Start committing transaction2.
              state->callback2 = base::MakeRefCounted<TestCallback>();
              EXPECT_TRUE(
                  state->transaction2->CommitPhaseOne(state->callback2).ok());
            },
            base::Unretained(backing_store()), base::Unretained(&state), range,
            database_id, object_store_id));
    RunAllTasksUntilIdle();

    idb_context_->TaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](TestableIndexedDBBackingStore* backing_store, TestState* state) {
              // Finish committing transaction2.
              EXPECT_TRUE(state->callback2->called);
              EXPECT_TRUE(state->callback2->succeeded);
              EXPECT_TRUE(state->transaction2->CommitPhaseTwo().ok());

              // Verify blob removals.
              EXPECT_EQ(0UL, backing_store->removals().size());

              // Clean up Transactions, etc on the IDB thread.
              *state = TestState();
            },
            base::Unretained(backing_store()), base::Unretained(&state)));
    RunAllTasksUntilIdle();
  }
}

TEST_F(IndexedDBBackingStoreTestWithBlobs, BlobJournalInterleavedTransactions) {
  struct TestState {
    std::unique_ptr<IndexedDBBackingStore::Transaction> transaction1;
    scoped_refptr<TestCallback> callback1;
    std::unique_ptr<IndexedDBBackingStore::Transaction> transaction2;
    scoped_refptr<TestCallback> callback2;
  } state;

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            // Initiate transaction1.
            state->transaction1 =
                std::make_unique<IndexedDBBackingStore::Transaction>(
                    test->backing_store());
            state->transaction1->Begin();
            std::vector<std::unique_ptr<storage::BlobDataHandle>> handles1;
            IndexedDBBackingStore::RecordIdentifier record1;
            EXPECT_TRUE(test->backing_store()
                            ->PutRecord(state->transaction1.get(), 1, 1,
                                        test->key3_, &test->value3_, &handles1,
                                        &record1)
                            .ok());
            state->callback1 = base::MakeRefCounted<TestCallback>();
            EXPECT_TRUE(
                state->transaction1->CommitPhaseOne(state->callback1).ok());
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            // Verify transaction1 phase one completed.
            EXPECT_TRUE(state->callback1->called);
            EXPECT_TRUE(state->callback1->succeeded);
            EXPECT_TRUE(test->CheckBlobWrites());
            EXPECT_EQ(0U, test->backing_store()->removals().size());

            // Initiate transaction2.
            state->transaction2 =
                std::make_unique<IndexedDBBackingStore::Transaction>(
                    test->backing_store());
            state->transaction2->Begin();
            std::vector<std::unique_ptr<storage::BlobDataHandle>> handles2;
            IndexedDBBackingStore::RecordIdentifier record2;
            EXPECT_TRUE(test->backing_store()
                            ->PutRecord(state->transaction2.get(), 1, 1,
                                        test->key1_, &test->value1_, &handles2,
                                        &record2)
                            .ok());
            state->callback2 = base::MakeRefCounted<TestCallback>();
            EXPECT_TRUE(
                state->transaction2->CommitPhaseOne(state->callback2).ok());
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            // Verify transaction2 phase one completed.
            EXPECT_TRUE(state->callback2->called);
            EXPECT_TRUE(state->callback2->succeeded);
            EXPECT_TRUE(test->CheckBlobWrites());
            EXPECT_EQ(0U, test->backing_store()->removals().size());

            // Finalize both transactions.
            EXPECT_TRUE(state->transaction1->CommitPhaseTwo().ok());
            EXPECT_EQ(0U, test->backing_store()->removals().size());

            EXPECT_TRUE(state->transaction2->CommitPhaseTwo().ok());
            EXPECT_EQ(0U, test->backing_store()->removals().size());

            // Clean up Transactions, etc on the IDB thread.
            *state = TestState();
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();
}

TEST_F(IndexedDBBackingStoreTestWithBlobs, LiveBlobJournal) {
  struct TestState {
    std::unique_ptr<IndexedDBBackingStore::Transaction> transaction1;
    scoped_refptr<TestCallback> callback1;
    std::unique_ptr<IndexedDBBackingStore::Transaction> transaction3;
    scoped_refptr<TestCallback> callback3;
    IndexedDBValue read_result_value;
  } state;

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            state->transaction1 =
                std::make_unique<IndexedDBBackingStore::Transaction>(
                    test->backing_store());
            state->transaction1->Begin();
            std::vector<std::unique_ptr<storage::BlobDataHandle>> handles;
            IndexedDBBackingStore::RecordIdentifier record;
            EXPECT_TRUE(test->backing_store()
                            ->PutRecord(state->transaction1.get(), 1, 1,
                                        test->key3_, &test->value3_, &handles,
                                        &record)
                            .ok());
            state->callback1 = base::MakeRefCounted<TestCallback>();
            EXPECT_TRUE(
                state->transaction1->CommitPhaseOne(state->callback1).ok());
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            EXPECT_TRUE(state->callback1->called);
            EXPECT_TRUE(state->callback1->succeeded);
            EXPECT_TRUE(test->CheckBlobWrites());
            EXPECT_TRUE(state->transaction1->CommitPhaseTwo().ok());

            IndexedDBBackingStore::Transaction transaction2(
                test->backing_store());
            transaction2.Begin();
            EXPECT_TRUE(test->backing_store()
                            ->GetRecord(&transaction2, 1, 1, test->key3_,
                                        &state->read_result_value)
                            .ok());
            scoped_refptr<TestCallback> callback(
                base::MakeRefCounted<TestCallback>());
            EXPECT_TRUE(transaction2.CommitPhaseOne(callback).ok());
            EXPECT_TRUE(callback->called);
            EXPECT_TRUE(callback->succeeded);
            EXPECT_TRUE(transaction2.CommitPhaseTwo().ok());
            EXPECT_EQ(test->value3_.bits, state->read_result_value.bits);
            EXPECT_TRUE(
                test->CheckBlobInfoMatches(state->read_result_value.blob_info));
            EXPECT_TRUE(test->CheckBlobReadsMatchWrites(
                state->read_result_value.blob_info));
            for (size_t i = 0; i < state->read_result_value.blob_info.size();
                 ++i) {
              state->read_result_value.blob_info[i].mark_used_callback().Run();
            }

            state->transaction3 =
                std::make_unique<IndexedDBBackingStore::Transaction>(
                    test->backing_store());
            state->transaction3->Begin();
            EXPECT_TRUE(test->backing_store()
                            ->DeleteRange(state->transaction3.get(), 1, 1,
                                          IndexedDBKeyRange(test->key3_))
                            .ok());
            state->callback3 = base::MakeRefCounted<TestCallback>();
            EXPECT_TRUE(
                state->transaction3->CommitPhaseOne(state->callback3).ok());
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            EXPECT_TRUE(state->callback3->called);
            EXPECT_TRUE(state->callback3->succeeded);
            EXPECT_TRUE(state->transaction3->CommitPhaseTwo().ok());
            EXPECT_EQ(0U, test->backing_store()->removals().size());
            for (size_t i = 0; i < state->read_result_value.blob_info.size();
                 ++i) {
              state->read_result_value.blob_info[i].release_callback().Run(
                  state->read_result_value.blob_info[i].file_path());
            }
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            EXPECT_TRUE(test->backing_store()->IsBlobCleanupPending());
#if DCHECK_IS_ON()
            EXPECT_EQ(3,
                      test->backing_store()
                          ->NumAggregatedJournalCleaningRequestsForTesting());
#endif
            for (int i = 3; i < IndexedDBBackingStore::kMaxJournalCleanRequests;
                 ++i) {
              test->backing_store()->StartJournalCleaningTimer();
            }
            EXPECT_NE(0U, test->backing_store()->removals().size());
            EXPECT_TRUE(test->CheckBlobRemovals());
#if DCHECK_IS_ON()
            EXPECT_EQ(0,
                      test->backing_store()->NumBlobFilesDeletedForTesting());
#endif
            EXPECT_FALSE(test->backing_store()->IsBlobCleanupPending());

            // Clean up Transactions, etc on the IDB thread.
            *state = TestState();
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();
}

// Make sure that using very high ( more than 32 bit ) values for database_id
// and object_store_id still work.
TEST_F(IndexedDBBackingStoreTest, HighIds) {
  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStore* backing_store, IndexedDBKey key1,
             IndexedDBKey key2, IndexedDBValue value1) {
            const int64_t high_database_id = 1ULL << 35;
            const int64_t high_object_store_id = 1ULL << 39;
            // index_ids are capped at 32 bits for storage purposes.
            const int64_t high_index_id = 1ULL << 29;

            const int64_t invalid_high_index_id = 1ULL << 37;

            const IndexedDBKey& index_key = key2;
            std::string index_key_raw;
            EncodeIDBKey(index_key, &index_key_raw);
            {
              IndexedDBBackingStore::Transaction transaction1(backing_store);
              transaction1.Begin();
              std::vector<std::unique_ptr<storage::BlobDataHandle>> handles;
              IndexedDBBackingStore::RecordIdentifier record;
              leveldb::Status s = backing_store->PutRecord(
                  &transaction1, high_database_id, high_object_store_id, key1,
                  &value1, &handles, &record);
              EXPECT_TRUE(s.ok());

              s = backing_store->PutIndexDataForRecord(
                  &transaction1, high_database_id, high_object_store_id,
                  invalid_high_index_id, index_key, record);
              EXPECT_FALSE(s.ok());

              s = backing_store->PutIndexDataForRecord(
                  &transaction1, high_database_id, high_object_store_id,
                  high_index_id, index_key, record);
              EXPECT_TRUE(s.ok());

              scoped_refptr<TestCallback> callback(
                  base::MakeRefCounted<TestCallback>());
              EXPECT_TRUE(transaction1.CommitPhaseOne(callback).ok());
              EXPECT_TRUE(callback->called);
              EXPECT_TRUE(callback->succeeded);
              EXPECT_TRUE(transaction1.CommitPhaseTwo().ok());
            }

            {
              IndexedDBBackingStore::Transaction transaction2(backing_store);
              transaction2.Begin();
              IndexedDBValue result_value;
              leveldb::Status s = backing_store->GetRecord(
                  &transaction2, high_database_id, high_object_store_id, key1,
                  &result_value);
              EXPECT_TRUE(s.ok());
              EXPECT_EQ(value1.bits, result_value.bits);

              std::unique_ptr<IndexedDBKey> new_primary_key;
              s = backing_store->GetPrimaryKeyViaIndex(
                  &transaction2, high_database_id, high_object_store_id,
                  invalid_high_index_id, index_key, &new_primary_key);
              EXPECT_FALSE(s.ok());

              s = backing_store->GetPrimaryKeyViaIndex(
                  &transaction2, high_database_id, high_object_store_id,
                  high_index_id, index_key, &new_primary_key);
              EXPECT_TRUE(s.ok());
              EXPECT_TRUE(new_primary_key->Equals(key1));

              scoped_refptr<TestCallback> callback(
                  base::MakeRefCounted<TestCallback>());
              EXPECT_TRUE(transaction2.CommitPhaseOne(callback).ok());
              EXPECT_TRUE(callback->called);
              EXPECT_TRUE(callback->succeeded);
              EXPECT_TRUE(transaction2.CommitPhaseTwo().ok());
            }
          },
          base::Unretained(backing_store()), key1_, key2_, value1_));
  RunAllTasksUntilIdle();
}

// Make sure that other invalid ids do not crash.
TEST_F(IndexedDBBackingStoreTest, InvalidIds) {
  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStore* backing_store, IndexedDBKey key,
             IndexedDBValue value) {
            // valid ids for use when testing invalid ids
            const int64_t database_id = 1;
            const int64_t object_store_id = 1;
            const int64_t index_id = kMinimumIndexId;
            // index_ids must be > kMinimumIndexId
            const int64_t invalid_low_index_id = 19;
            IndexedDBValue result_value;

            IndexedDBBackingStore::Transaction transaction1(backing_store);
            transaction1.Begin();

            std::vector<std::unique_ptr<storage::BlobDataHandle>> handles;
            IndexedDBBackingStore::RecordIdentifier record;
            leveldb::Status s = backing_store->PutRecord(
                &transaction1, database_id, KeyPrefix::kInvalidId, key, &value,
                &handles, &record);
            EXPECT_FALSE(s.ok());
            s = backing_store->PutRecord(&transaction1, database_id, 0, key,
                                         &value, &handles, &record);
            EXPECT_FALSE(s.ok());
            s = backing_store->PutRecord(&transaction1, KeyPrefix::kInvalidId,
                                         object_store_id, key, &value, &handles,
                                         &record);
            EXPECT_FALSE(s.ok());
            s = backing_store->PutRecord(&transaction1, 0, object_store_id, key,
                                         &value, &handles, &record);
            EXPECT_FALSE(s.ok());

            s = backing_store->GetRecord(&transaction1, database_id,
                                         KeyPrefix::kInvalidId, key,
                                         &result_value);
            EXPECT_FALSE(s.ok());
            s = backing_store->GetRecord(&transaction1, database_id, 0, key,
                                         &result_value);
            EXPECT_FALSE(s.ok());
            s = backing_store->GetRecord(&transaction1, KeyPrefix::kInvalidId,
                                         object_store_id, key, &result_value);
            EXPECT_FALSE(s.ok());
            s = backing_store->GetRecord(&transaction1, 0, object_store_id, key,
                                         &result_value);
            EXPECT_FALSE(s.ok());

            std::unique_ptr<IndexedDBKey> new_primary_key;
            s = backing_store->GetPrimaryKeyViaIndex(
                &transaction1, database_id, object_store_id,
                KeyPrefix::kInvalidId, key, &new_primary_key);
            EXPECT_FALSE(s.ok());
            s = backing_store->GetPrimaryKeyViaIndex(
                &transaction1, database_id, object_store_id,
                invalid_low_index_id, key, &new_primary_key);
            EXPECT_FALSE(s.ok());
            s = backing_store->GetPrimaryKeyViaIndex(&transaction1, database_id,
                                                     object_store_id, 0, key,
                                                     &new_primary_key);
            EXPECT_FALSE(s.ok());

            s = backing_store->GetPrimaryKeyViaIndex(
                &transaction1, KeyPrefix::kInvalidId, object_store_id, index_id,
                key, &new_primary_key);
            EXPECT_FALSE(s.ok());
            s = backing_store->GetPrimaryKeyViaIndex(
                &transaction1, database_id, KeyPrefix::kInvalidId, index_id,
                key, &new_primary_key);
            EXPECT_FALSE(s.ok());
          },
          base::Unretained(backing_store()), key1_, value1_));
  RunAllTasksUntilIdle();
}

TEST_F(IndexedDBBackingStoreTest, CreateDatabase) {
  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStore* backing_store) {
            const base::string16 database_name(ASCIIToUTF16("db1"));
            int64_t database_id;
            const int64_t version = 9;

            const int64_t object_store_id = 99;
            const base::string16 object_store_name(
                ASCIIToUTF16("object_store1"));
            const bool auto_increment = true;
            const IndexedDBKeyPath object_store_key_path(
                ASCIIToUTF16("object_store_key"));

            const int64_t index_id = 999;
            const base::string16 index_name(ASCIIToUTF16("index1"));
            const bool unique = true;
            const bool multi_entry = true;
            const IndexedDBKeyPath index_key_path(ASCIIToUTF16("index_key"));

            IndexedDBMetadataCoding metadata_coding;

            {
              IndexedDBDatabaseMetadata database;
              leveldb::Status s = metadata_coding.CreateDatabase(
                  backing_store->db(), backing_store->origin_identifier(),
                  database_name, version, &database);
              EXPECT_TRUE(s.ok());
              EXPECT_GT(database.id, 0);
              database_id = database.id;

              IndexedDBBackingStore::Transaction transaction(backing_store);
              transaction.Begin();

              IndexedDBObjectStoreMetadata object_store;
              s = metadata_coding.CreateObjectStore(
                  transaction.transaction(), database.id, object_store_id,
                  object_store_name, object_store_key_path, auto_increment,
                  &object_store);
              EXPECT_TRUE(s.ok());

              IndexedDBIndexMetadata index;
              s = metadata_coding.CreateIndex(
                  transaction.transaction(), database.id, object_store.id,
                  index_id, index_name, index_key_path, unique, multi_entry,
                  &index);
              EXPECT_TRUE(s.ok());

              scoped_refptr<TestCallback> callback(
                  base::MakeRefCounted<TestCallback>());
              EXPECT_TRUE(transaction.CommitPhaseOne(callback).ok());
              EXPECT_TRUE(callback->called);
              EXPECT_TRUE(callback->succeeded);
              EXPECT_TRUE(transaction.CommitPhaseTwo().ok());
            }

            {
              IndexedDBDatabaseMetadata database;
              bool found;
              leveldb::Status s = metadata_coding.ReadMetadataForDatabaseName(
                  backing_store->db(), backing_store->origin_identifier(),
                  database_name, &database, &found);
              EXPECT_TRUE(s.ok());
              EXPECT_TRUE(found);

              // database.name is not filled in by the implementation.
              EXPECT_EQ(version, database.version);
              EXPECT_EQ(database_id, database.id);

              EXPECT_EQ(1UL, database.object_stores.size());
              IndexedDBObjectStoreMetadata object_store =
                  database.object_stores[object_store_id];
              EXPECT_EQ(object_store_name, object_store.name);
              EXPECT_EQ(object_store_key_path, object_store.key_path);
              EXPECT_EQ(auto_increment, object_store.auto_increment);

              EXPECT_EQ(1UL, object_store.indexes.size());
              IndexedDBIndexMetadata index = object_store.indexes[index_id];
              EXPECT_EQ(index_name, index.name);
              EXPECT_EQ(index_key_path, index.key_path);
              EXPECT_EQ(unique, index.unique);
              EXPECT_EQ(multi_entry, index.multi_entry);
            }
          },
          base::Unretained(backing_store())));
  RunAllTasksUntilIdle();
}

TEST_F(IndexedDBBackingStoreTest, GetDatabaseNames) {
  idb_context_->TaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](IndexedDBBackingStore* backing_store) {
                       const base::string16 db1_name(ASCIIToUTF16("db1"));
                       const int64_t db1_version = 1LL;

                       // Database records with DEFAULT_VERSION represent
                       // stale data, and should not be enumerated.
                       const base::string16 db2_name(ASCIIToUTF16("db2"));
                       const int64_t db2_version =
                           IndexedDBDatabaseMetadata::DEFAULT_VERSION;
                       IndexedDBMetadataCoding metadata_coding;

                       IndexedDBDatabaseMetadata db1;
                       leveldb::Status s = metadata_coding.CreateDatabase(
                           backing_store->db(),
                           backing_store->origin_identifier(), db1_name,
                           db1_version, &db1);
                       EXPECT_TRUE(s.ok());
                       EXPECT_GT(db1.id, 0LL);

                       IndexedDBDatabaseMetadata db2;
                       s = metadata_coding.CreateDatabase(
                           backing_store->db(),
                           backing_store->origin_identifier(), db2_name,
                           db2_version, &db2);
                       EXPECT_TRUE(s.ok());
                       EXPECT_GT(db2.id, db1.id);

                       std::vector<base::string16> names;
                       s = metadata_coding.ReadDatabaseNames(
                           backing_store->db(),
                           backing_store->origin_identifier(), &names);
                       EXPECT_TRUE(s.ok());
                       ASSERT_EQ(1U, names.size());
                       EXPECT_EQ(db1_name, names[0]);
                     },
                     base::Unretained(backing_store())));
  RunAllTasksUntilIdle();
}

TEST_F(IndexedDBBackingStoreTest, ReadCorruptionInfo) {
  // No |path_base|.
  std::string message;
  EXPECT_FALSE(IndexedDBBackingStore::ReadCorruptionInfo(base::FilePath(),
                                                         Origin(), &message));
  EXPECT_TRUE(message.empty());
  message.clear();

  const base::FilePath path_base = temp_dir_.GetPath();
  const Origin origin = Origin::Create(GURL("http://www.google.com/"));
  ASSERT_FALSE(path_base.empty());
  ASSERT_TRUE(PathIsWritable(path_base));

  // File not found.
  EXPECT_FALSE(
      IndexedDBBackingStore::ReadCorruptionInfo(path_base, origin, &message));
  EXPECT_TRUE(message.empty());
  message.clear();

  const base::FilePath info_path =
      path_base.AppendASCII("http_www.google.com_0.indexeddb.leveldb")
          .AppendASCII("corruption_info.json");
  ASSERT_TRUE(CreateDirectory(info_path.DirName()));

  // Empty file.
  std::string dummy_data;
  ASSERT_TRUE(WriteFile(info_path, dummy_data));
  EXPECT_FALSE(
      IndexedDBBackingStore::ReadCorruptionInfo(path_base, origin, &message));
  EXPECT_FALSE(PathExists(info_path));
  EXPECT_TRUE(message.empty());
  message.clear();

  // File size > 4 KB.
  dummy_data.resize(5000, 'c');
  ASSERT_TRUE(WriteFile(info_path, dummy_data));
  EXPECT_FALSE(
      IndexedDBBackingStore::ReadCorruptionInfo(path_base, origin, &message));
  EXPECT_FALSE(PathExists(info_path));
  EXPECT_TRUE(message.empty());
  message.clear();

  // Random string.
  ASSERT_TRUE(WriteFile(info_path, "foo bar"));
  EXPECT_FALSE(
      IndexedDBBackingStore::ReadCorruptionInfo(path_base, origin, &message));
  EXPECT_FALSE(PathExists(info_path));
  EXPECT_TRUE(message.empty());
  message.clear();

  // Not a dictionary.
  ASSERT_TRUE(WriteFile(info_path, "[]"));
  EXPECT_FALSE(
      IndexedDBBackingStore::ReadCorruptionInfo(path_base, origin, &message));
  EXPECT_FALSE(PathExists(info_path));
  EXPECT_TRUE(message.empty());
  message.clear();

  // Empty dictionary.
  ASSERT_TRUE(WriteFile(info_path, "{}"));
  EXPECT_FALSE(
      IndexedDBBackingStore::ReadCorruptionInfo(path_base, origin, &message));
  EXPECT_FALSE(PathExists(info_path));
  EXPECT_TRUE(message.empty());
  message.clear();

  // Dictionary, no message key.
  ASSERT_TRUE(WriteFile(info_path, "{\"foo\":\"bar\"}"));
  EXPECT_FALSE(
      IndexedDBBackingStore::ReadCorruptionInfo(path_base, origin, &message));
  EXPECT_FALSE(PathExists(info_path));
  EXPECT_TRUE(message.empty());
  message.clear();

  // Dictionary, message key.
  ASSERT_TRUE(WriteFile(info_path, "{\"message\":\"bar\"}"));
  EXPECT_TRUE(
      IndexedDBBackingStore::ReadCorruptionInfo(path_base, origin, &message));
  EXPECT_FALSE(PathExists(info_path));
  EXPECT_EQ("bar", message);
  message.clear();

  // Dictionary, message key and more.
  ASSERT_TRUE(WriteFile(info_path, "{\"message\":\"foo\",\"bar\":5}"));
  EXPECT_TRUE(
      IndexedDBBackingStore::ReadCorruptionInfo(path_base, origin, &message));
  EXPECT_FALSE(PathExists(info_path));
  EXPECT_EQ("foo", message);
}

// There was a wrong migration from schema 2 to 3, which always delete IDB
// blobs and doesn't actually write the new schema version. This tests the
// upgrade path where the database doesn't have blob entries, so it' safe to
// keep the database.
// https://crbug.com/756447, https://crbug.com/829125, https://crbug.com/829141
TEST_F(IndexedDBBackingStoreTest, SchemaUpgradeWithoutBlobsSurvives) {
  struct TestState {
    int64_t database_id;
    const int64_t object_store_id = 99;
  } state;

  // The database metadata needs to be written so we can verify the blob entry
  // keys are not detected.
  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStore* backing_store, TestState* state) {
            const base::string16 database_name(ASCIIToUTF16("db1"));
            const int64_t version = 9;

            const base::string16 object_store_name(
                ASCIIToUTF16("object_store1"));
            const bool auto_increment = true;
            const IndexedDBKeyPath object_store_key_path(
                ASCIIToUTF16("object_store_key"));

            IndexedDBMetadataCoding metadata_coding;

            {
              IndexedDBDatabaseMetadata database;
              leveldb::Status s = metadata_coding.CreateDatabase(
                  backing_store->db(), backing_store->origin_identifier(),
                  database_name, version, &database);
              EXPECT_TRUE(s.ok());
              EXPECT_GT(database.id, 0);
              state->database_id = database.id;

              IndexedDBBackingStore::Transaction transaction(backing_store);
              transaction.Begin();

              IndexedDBObjectStoreMetadata object_store;
              s = metadata_coding.CreateObjectStore(
                  transaction.transaction(), database.id,
                  state->object_store_id, object_store_name,
                  object_store_key_path, auto_increment, &object_store);
              EXPECT_TRUE(s.ok());

              scoped_refptr<TestCallback> callback(
                  base::MakeRefCounted<TestCallback>());
              EXPECT_TRUE(transaction.CommitPhaseOne(callback).ok());
              EXPECT_TRUE(callback->called);
              EXPECT_TRUE(callback->succeeded);
              EXPECT_TRUE(transaction.CommitPhaseTwo().ok());
            }
          },
          base::Unretained(backing_store()), base::Unretained(&state)));
  RunAllTasksUntilIdle();
  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStore* backing_store, IndexedDBKey key,
             IndexedDBValue value, TestState* state) {
            // Save a value.
            IndexedDBBackingStore::Transaction transaction1(backing_store);
            transaction1.Begin();
            std::vector<std::unique_ptr<storage::BlobDataHandle>> handles;
            IndexedDBBackingStore::RecordIdentifier record;
            leveldb::Status s = backing_store->PutRecord(
                &transaction1, state->database_id, state->object_store_id, key,
                &value, &handles, &record);
            EXPECT_TRUE(s.ok());
            scoped_refptr<TestCallback> callback(
                base::MakeRefCounted<TestCallback>());
            EXPECT_TRUE(transaction1.CommitPhaseOne(callback).ok());
            EXPECT_TRUE(callback->called);
            EXPECT_TRUE(callback->succeeded);
            EXPECT_TRUE(transaction1.CommitPhaseTwo().ok());

            // Set the schema to 2, which was before blob support.
            scoped_refptr<LevelDBTransaction> transaction =
                IndexedDBClassFactory::Get()->CreateLevelDBTransaction(
                    backing_store->db());
            const std::string schema_version_key = SchemaVersionKey::Encode();
            indexed_db::PutInt(transaction.get(), schema_version_key, 2);
            ASSERT_TRUE(transaction->Commit().ok());
          },
          base::Unretained(backing_store()), key1_, value1_,
          base::Unretained(&state)));
  RunAllTasksUntilIdle();

  DestroyFactoryAndBackingStore();
  CreateFactoryAndBackingStore();

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStore* backing_store, IndexedDBKey key,
             IndexedDBValue value, TestState* state) {
            IndexedDBBackingStore::Transaction transaction2(backing_store);
            transaction2.Begin();
            IndexedDBValue result_value;
            EXPECT_TRUE(backing_store
                            ->GetRecord(&transaction2, state->database_id,
                                        state->object_store_id, key,
                                        &result_value)
                            .ok());
            scoped_refptr<TestCallback> callback(
                base::MakeRefCounted<TestCallback>());
            EXPECT_TRUE(transaction2.CommitPhaseOne(callback).ok());
            EXPECT_TRUE(callback->called);
            EXPECT_TRUE(callback->succeeded);
            EXPECT_TRUE(transaction2.CommitPhaseTwo().ok());
            EXPECT_EQ(value.bits, result_value.bits);

            // Test that we upgraded.
            scoped_refptr<LevelDBTransaction> transaction =
                IndexedDBClassFactory::Get()->CreateLevelDBTransaction(
                    backing_store->db());
            const std::string schema_version_key = SchemaVersionKey::Encode();
            int64_t found_int = 0;
            bool found = false;
            bool success =
                indexed_db::GetInt(transaction.get(), schema_version_key,
                                   &found_int, &found)
                    .ok();
            ASSERT_TRUE(success);
            ASSERT_TRUE(transaction->Commit().ok());

            EXPECT_TRUE(found);
            EXPECT_EQ(3, found_int);
          },
          base::Unretained(backing_store()), key1_, value1_,
          base::Unretained(&state)));
  RunAllTasksUntilIdle();
}

// Our v2->v3 schema migration code forgot to bump the on-disk version number.
// This test covers migrating a v3 database mislabeled as v2 to a properly
// labeled v3 database. When the mislabeled database has blob entries, we must
// treat it as corrupt and delete it.
// https://crbug.com/756447, https://crbug.com/829125, https://crbug.com/829141
TEST_F(IndexedDBBackingStoreTestWithBlobs, SchemaUpgradeWithBlobsCorrupt) {
  struct TestState {
    int64_t database_id;
    const int64_t object_store_id = 99;
    std::unique_ptr<IndexedDBBackingStore::Transaction> transaction1;
    scoped_refptr<TestCallback> callback1;
    std::unique_ptr<IndexedDBBackingStore::Transaction> transaction3;
    scoped_refptr<TestCallback> callback3;
  } state;

  // The database metadata needs to be written so the blob entry keys can
  // be detected.
  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStore* backing_store, TestState* state) {
            const base::string16 database_name(ASCIIToUTF16("db1"));
            const int64_t version = 9;

            const base::string16 object_store_name(
                ASCIIToUTF16("object_store1"));
            const bool auto_increment = true;
            const IndexedDBKeyPath object_store_key_path(
                ASCIIToUTF16("object_store_key"));

            IndexedDBMetadataCoding metadata_coding;

            {
              IndexedDBDatabaseMetadata database;
              leveldb::Status s = metadata_coding.CreateDatabase(
                  backing_store->db(), backing_store->origin_identifier(),
                  database_name, version, &database);
              EXPECT_TRUE(s.ok());
              EXPECT_GT(database.id, 0);
              state->database_id = database.id;

              IndexedDBBackingStore::Transaction transaction(backing_store);
              transaction.Begin();

              IndexedDBObjectStoreMetadata object_store;
              s = metadata_coding.CreateObjectStore(
                  transaction.transaction(), database.id,
                  state->object_store_id, object_store_name,
                  object_store_key_path, auto_increment, &object_store);
              EXPECT_TRUE(s.ok());

              scoped_refptr<TestCallback> callback(
                  base::MakeRefCounted<TestCallback>());
              EXPECT_TRUE(transaction.CommitPhaseOne(callback).ok());
              EXPECT_TRUE(callback->called);
              EXPECT_TRUE(callback->succeeded);
              EXPECT_TRUE(transaction.CommitPhaseTwo().ok());
            }
          },
          base::Unretained(backing_store()), base::Unretained(&state)));
  RunAllTasksUntilIdle();

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            // Initiate transaction1 - writing blobs.
            state->transaction1 =
                std::make_unique<IndexedDBBackingStore::Transaction>(
                    test->backing_store());
            state->transaction1->Begin();
            std::vector<std::unique_ptr<storage::BlobDataHandle>> handles;
            IndexedDBBackingStore::RecordIdentifier record;
            EXPECT_TRUE(test->backing_store()
                            ->PutRecord(state->transaction1.get(),
                                        state->database_id,
                                        state->object_store_id, test->key3_,
                                        &test->value3_, &handles, &record)
                            .ok());
            state->callback1 = base::MakeRefCounted<TestCallback>();
            EXPECT_TRUE(
                state->transaction1->CommitPhaseOne(state->callback1).ok());
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();

  idb_context_->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBBackingStoreTestWithBlobs* test, TestState* state) {
            // Finish up transaction1, verifying blob writes.
            EXPECT_TRUE(state->callback1->called);
            EXPECT_TRUE(state->callback1->succeeded);
            EXPECT_TRUE(test->CheckBlobWrites());
            EXPECT_TRUE(state->transaction1->CommitPhaseTwo().ok());

            // Set the schema to 2, which was before blob support.
            scoped_refptr<LevelDBTransaction> transaction =
                IndexedDBClassFactory::Get()->CreateLevelDBTransaction(
                    test->backing_store()->db());
            const std::string schema_version_key = SchemaVersionKey::Encode();
            indexed_db::PutInt(transaction.get(), schema_version_key, 2);
            ASSERT_TRUE(transaction->Commit().ok());
          },
          base::Unretained(this), base::Unretained(&state)));
  RunAllTasksUntilIdle();

  DestroyFactoryAndBackingStore();
  CreateFactoryAndBackingStore();

  // The factory returns a null backing store pointer when there is a corrupt
  // database.
  EXPECT_EQ(nullptr, backing_store());
}

}  // namespace indexed_db_backing_store_unittest
}  // namespace content
