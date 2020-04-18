// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_transaction.h"

#include <stdint.h>
#include <memory>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/indexed_db/fake_indexed_db_metadata_coding.h"
#include "content/browser/indexed_db/indexed_db_connection.h"
#include "content/browser/indexed_db/indexed_db_database_error.h"
#include "content/browser/indexed_db/indexed_db_fake_backing_store.h"
#include "content/browser/indexed_db/indexed_db_metadata_coding.h"
#include "content/browser/indexed_db/indexed_db_observer.h"
#include "content/browser/indexed_db/mock_indexed_db_database_callbacks.h"
#include "content/browser/indexed_db/mock_indexed_db_factory.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_database_exception.h"

namespace content {
namespace indexed_db_transaction_unittest {

const int kFakeProcessId = 10;

class AbortObserver {
 public:
  AbortObserver() : abort_task_called_(false) {}

  void AbortTask() { abort_task_called_ = true; }

  bool abort_task_called() const { return abort_task_called_; }

 private:
  bool abort_task_called_;
  DISALLOW_COPY_AND_ASSIGN(AbortObserver);
};

class IndexedDBTransactionTest : public testing::Test {
 public:
  IndexedDBTransactionTest() : factory_(new MockIndexedDBFactory()) {
    backing_store_ = new IndexedDBFakeBackingStore();
    CreateDB();
  }

  void CreateDB() {
    // DB is created here instead of the constructor to workaround a
    // "peculiarity of C++". More info at
    // https://github.com/google/googletest/blob/master/googletest/docs/FAQ.md#my-compiler-complains-that-a-constructor-or-destructor-cannot-return-a-value-whats-going-on
    leveldb::Status s;
    std::tie(db_, s) = IndexedDBDatabase::Create(
        base::ASCIIToUTF16("db"), backing_store_.get(), factory_.get(),
        std::make_unique<FakeIndexedDBMetadataCoding>(),
        IndexedDBDatabase::Identifier());
    ASSERT_TRUE(s.ok());
  }

  void RunPostedTasks() { base::RunLoop().RunUntilIdle(); }
  leveldb::Status DummyOperation(leveldb::Status result,
                                 IndexedDBTransaction* transaction) {
    return result;
  }
  leveldb::Status AbortableOperation(AbortObserver* observer,
                                     IndexedDBTransaction* transaction) {
    transaction->ScheduleAbortTask(
        base::BindOnce(&AbortObserver::AbortTask, base::Unretained(observer)));
    return leveldb::Status::OK();
  }

 protected:
  scoped_refptr<IndexedDBFakeBackingStore> backing_store_;
  scoped_refptr<IndexedDBDatabase> db_;
  scoped_refptr<MockIndexedDBFactory> factory_;

 private:
  TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(IndexedDBTransactionTest);
};

class IndexedDBTransactionTestMode
    : public IndexedDBTransactionTest,
      public testing::WithParamInterface<blink::WebIDBTransactionMode> {
 public:
  IndexedDBTransactionTestMode() {}
 private:
  DISALLOW_COPY_AND_ASSIGN(IndexedDBTransactionTestMode);
};

TEST_F(IndexedDBTransactionTest, Timeout) {
  const int64_t id = 0;
  const std::set<int64_t> scope;
  const leveldb::Status commit_success = leveldb::Status::OK();
  std::unique_ptr<IndexedDBConnection> connection(
      std::make_unique<IndexedDBConnection>(
          kFakeProcessId, db_, new MockIndexedDBDatabaseCallbacks()));
  std::unique_ptr<IndexedDBTransaction> transaction =
      std::unique_ptr<IndexedDBTransaction>(new IndexedDBTransaction(
          id, connection.get(), scope, blink::kWebIDBTransactionModeReadWrite,
          new IndexedDBFakeBackingStore::FakeTransaction(commit_success)));
  db_->TransactionCreated(transaction.get());

  // No conflicting transactions, so coordinator will start it immediately:
  EXPECT_EQ(IndexedDBTransaction::STARTED, transaction->state());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
  EXPECT_EQ(0, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(0, transaction->diagnostics().tasks_completed);

  // Schedule a task - timer won't be started until it's processed.
  transaction->ScheduleTask(
      base::BindOnce(&IndexedDBTransactionTest::DummyOperation,
                     base::Unretained(this), leveldb::Status::OK()));
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
  EXPECT_EQ(1, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(0, transaction->diagnostics().tasks_completed);

  RunPostedTasks();
  EXPECT_TRUE(transaction->IsTimeoutTimerRunning());

  transaction->Timeout();
  EXPECT_EQ(IndexedDBTransaction::FINISHED, transaction->state());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
  EXPECT_EQ(1, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(1, transaction->diagnostics().tasks_completed);

  // This task will be ignored.
  transaction->ScheduleTask(
      base::BindOnce(&IndexedDBTransactionTest::DummyOperation,
                     base::Unretained(this), leveldb::Status::OK()));
  EXPECT_EQ(IndexedDBTransaction::FINISHED, transaction->state());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
  EXPECT_EQ(1, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(1, transaction->diagnostics().tasks_completed);
}

TEST_F(IndexedDBTransactionTest, NoTimeoutReadOnly) {
  const int64_t id = 0;
  const std::set<int64_t> scope;
  const leveldb::Status commit_success = leveldb::Status::OK();
  std::unique_ptr<IndexedDBConnection> connection(
      std::make_unique<IndexedDBConnection>(
          kFakeProcessId, db_, new MockIndexedDBDatabaseCallbacks()));
  std::unique_ptr<IndexedDBTransaction> transaction =
      std::unique_ptr<IndexedDBTransaction>(new IndexedDBTransaction(
          id, connection.get(), scope, blink::kWebIDBTransactionModeReadOnly,
          new IndexedDBFakeBackingStore::FakeTransaction(commit_success)));
  db_->TransactionCreated(transaction.get());

  // No conflicting transactions, so coordinator will start it immediately:
  EXPECT_EQ(IndexedDBTransaction::STARTED, transaction->state());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());

  // Schedule a task - timer won't be started until it's processed.
  transaction->ScheduleTask(
      base::BindOnce(&IndexedDBTransactionTest::DummyOperation,
                     base::Unretained(this), leveldb::Status::OK()));
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());

  // Transaction is read-only, so no need to time it out.
  RunPostedTasks();
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());

  // Clean up to avoid leaks.
  transaction->Abort(IndexedDBDatabaseError(
      IndexedDBDatabaseError(blink::kWebIDBDatabaseExceptionAbortError,
                             "Transaction aborted by user.")));
  EXPECT_EQ(IndexedDBTransaction::FINISHED, transaction->state());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
}

TEST_P(IndexedDBTransactionTestMode, ScheduleNormalTask) {
  const int64_t id = 0;
  const std::set<int64_t> scope;
  const leveldb::Status commit_success = leveldb::Status::OK();
  std::unique_ptr<IndexedDBConnection> connection(
      std::make_unique<IndexedDBConnection>(
          kFakeProcessId, db_, new MockIndexedDBDatabaseCallbacks()));
  std::unique_ptr<IndexedDBTransaction> transaction =
      std::unique_ptr<IndexedDBTransaction>(new IndexedDBTransaction(
          id, connection.get(), scope, GetParam(),
          new IndexedDBFakeBackingStore::FakeTransaction(commit_success)));

  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());
  EXPECT_EQ(0, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(0, transaction->diagnostics().tasks_completed);

  db_->TransactionCreated(transaction.get());

  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());

  transaction->ScheduleTask(
      blink::kWebIDBTaskTypeNormal,
      base::BindOnce(&IndexedDBTransactionTest::DummyOperation,
                     base::Unretained(this), leveldb::Status::OK()));

  EXPECT_EQ(1, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(0, transaction->diagnostics().tasks_completed);

  EXPECT_TRUE(transaction->HasPendingTasks());
  EXPECT_FALSE(transaction->IsTaskQueueEmpty());
  EXPECT_FALSE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());

  // Pump the message loop so that the transaction completes all pending tasks,
  // otherwise it will defer the commit.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());
  EXPECT_EQ(IndexedDBTransaction::STARTED, transaction->state());
  EXPECT_EQ(1, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(1, transaction->diagnostics().tasks_completed);

  transaction->Commit();

  EXPECT_EQ(IndexedDBTransaction::FINISHED, transaction->state());
  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());
  EXPECT_EQ(1, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(1, transaction->diagnostics().tasks_completed);
}

TEST_P(IndexedDBTransactionTestMode, TaskFails) {
  const int64_t id = 0;
  const std::set<int64_t> scope;
  const leveldb::Status commit_success = leveldb::Status::OK();
  std::unique_ptr<IndexedDBConnection> connection(
      std::make_unique<IndexedDBConnection>(
          kFakeProcessId, db_, new MockIndexedDBDatabaseCallbacks()));
  std::unique_ptr<IndexedDBTransaction> transaction =
      std::unique_ptr<IndexedDBTransaction>(new IndexedDBTransaction(
          id, connection.get(), scope, GetParam(),
          new IndexedDBFakeBackingStore::FakeTransaction(commit_success)));

  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());
  EXPECT_EQ(0, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(0, transaction->diagnostics().tasks_completed);

  db_->TransactionCreated(transaction.get());

  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());

  EXPECT_CALL(*factory_, HandleBackingStoreFailure(testing::_)).Times(1);

  transaction->ScheduleTask(
      blink::kWebIDBTaskTypeNormal,
      base::BindOnce(&IndexedDBTransactionTest::DummyOperation,
                     base::Unretained(this),
                     leveldb::Status::IOError("error")));

  EXPECT_EQ(1, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(0, transaction->diagnostics().tasks_completed);

  EXPECT_TRUE(transaction->HasPendingTasks());
  EXPECT_FALSE(transaction->IsTaskQueueEmpty());
  EXPECT_FALSE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());

  // Pump the message loop so that the transaction completes all pending tasks,
  // otherwise it will defer the commit.
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());
  EXPECT_EQ(IndexedDBTransaction::STARTED, transaction->state());
  EXPECT_EQ(1, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(1, transaction->diagnostics().tasks_completed);

  transaction->Commit();

  EXPECT_EQ(IndexedDBTransaction::FINISHED, transaction->state());
  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());
  EXPECT_EQ(1, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(1, transaction->diagnostics().tasks_completed);
}

TEST_F(IndexedDBTransactionTest, SchedulePreemptiveTask) {
  const int64_t id = 0;
  const std::set<int64_t> scope;
  const leveldb::Status commit_failure = leveldb::Status::Corruption("Ouch.");
  std::unique_ptr<IndexedDBConnection> connection(
      std::make_unique<IndexedDBConnection>(
          kFakeProcessId, db_, new MockIndexedDBDatabaseCallbacks()));
  std::unique_ptr<IndexedDBTransaction> transaction =
      std::unique_ptr<IndexedDBTransaction>(new IndexedDBTransaction(
          id, connection.get(), scope,
          blink::kWebIDBTransactionModeVersionChange,
          new IndexedDBFakeBackingStore::FakeTransaction(commit_failure)));

  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());
  EXPECT_EQ(0, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(0, transaction->diagnostics().tasks_completed);

  db_->TransactionCreated(transaction.get());

  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());

  transaction->ScheduleTask(
      blink::kWebIDBTaskTypePreemptive,
      base::BindOnce(&IndexedDBTransactionTest::DummyOperation,
                     base::Unretained(this), leveldb::Status::OK()));
  transaction->AddPreemptiveEvent();

  EXPECT_TRUE(transaction->HasPendingTasks());
  EXPECT_FALSE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_FALSE(transaction->preemptive_task_queue_.empty());

  // Pump the message loop so that the transaction completes all pending tasks,
  // otherwise it will defer the commit.
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(transaction->HasPendingTasks());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());
  EXPECT_EQ(IndexedDBTransaction::STARTED, transaction->state());
  EXPECT_EQ(0, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(0, transaction->diagnostics().tasks_completed);

  transaction->DidCompletePreemptiveEvent();
  transaction->Commit();

  EXPECT_EQ(IndexedDBTransaction::FINISHED, transaction->state());
  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
  EXPECT_TRUE(transaction->IsTaskQueueEmpty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());
  EXPECT_EQ(0, transaction->diagnostics().tasks_scheduled);
  EXPECT_EQ(0, transaction->diagnostics().tasks_completed);
}

TEST_P(IndexedDBTransactionTestMode, AbortTasks) {
  const int64_t id = 0;
  const std::set<int64_t> scope;
  const leveldb::Status commit_failure = leveldb::Status::Corruption("Ouch.");
  std::unique_ptr<IndexedDBConnection> connection(
      std::make_unique<IndexedDBConnection>(
          kFakeProcessId, db_, new MockIndexedDBDatabaseCallbacks()));
  std::unique_ptr<IndexedDBTransaction> transaction =
      std::unique_ptr<IndexedDBTransaction>(new IndexedDBTransaction(
          id, connection.get(), scope, GetParam(),
          new IndexedDBFakeBackingStore::FakeTransaction(commit_failure)));
  db_->TransactionCreated(transaction.get());

  AbortObserver observer;
  transaction->ScheduleTask(
      base::BindOnce(&IndexedDBTransactionTest::AbortableOperation,
                     base::Unretained(this), base::Unretained(&observer)));

  // Pump the message loop so that the transaction completes all pending tasks,
  // otherwise it will defer the commit.
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(observer.abort_task_called());
  transaction->Commit();
  EXPECT_TRUE(observer.abort_task_called());
  EXPECT_EQ(IndexedDBTransaction::FINISHED, transaction->state());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
}

TEST_P(IndexedDBTransactionTestMode, AbortPreemptive) {
  const int64_t id = 0;
  const std::set<int64_t> scope;
  const leveldb::Status commit_success = leveldb::Status::OK();
  std::unique_ptr<IndexedDBConnection> connection(
      std::make_unique<IndexedDBConnection>(
          kFakeProcessId, db_, new MockIndexedDBDatabaseCallbacks()));
  std::unique_ptr<IndexedDBTransaction> transaction =
      std::unique_ptr<IndexedDBTransaction>(new IndexedDBTransaction(
          id, connection.get(), scope, GetParam(),
          new IndexedDBFakeBackingStore::FakeTransaction(commit_success)));
  db_->TransactionCreated(transaction.get());

  // No conflicting transactions, so coordinator will start it immediately:
  EXPECT_EQ(IndexedDBTransaction::STARTED, transaction->state());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());

  transaction->ScheduleTask(
      blink::kWebIDBTaskTypePreemptive,
      base::BindOnce(&IndexedDBTransactionTest::DummyOperation,
                     base::Unretained(this), leveldb::Status::OK()));
  EXPECT_EQ(0, transaction->pending_preemptive_events_);
  transaction->AddPreemptiveEvent();
  EXPECT_EQ(1, transaction->pending_preemptive_events_);

  RunPostedTasks();

  transaction->Abort(
      IndexedDBDatabaseError(blink::kWebIDBDatabaseExceptionAbortError,
                             "Transaction aborted by user."));
  EXPECT_EQ(IndexedDBTransaction::FINISHED, transaction->state());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
  EXPECT_EQ(0, transaction->pending_preemptive_events_);
  EXPECT_TRUE(transaction->preemptive_task_queue_.empty());
  EXPECT_TRUE(transaction->task_queue_.empty());
  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_EQ(transaction->diagnostics().tasks_completed,
            transaction->diagnostics().tasks_scheduled);
  EXPECT_FALSE(transaction->should_process_queue_);
  EXPECT_TRUE(transaction->backing_store_transaction_begun_);
  EXPECT_TRUE(transaction->used_);
  EXPECT_FALSE(transaction->commit_pending_);

  // This task will be ignored.
  transaction->ScheduleTask(
      base::BindOnce(&IndexedDBTransactionTest::DummyOperation,
                     base::Unretained(this), leveldb::Status::OK()));
  EXPECT_EQ(IndexedDBTransaction::FINISHED, transaction->state());
  EXPECT_FALSE(transaction->IsTimeoutTimerRunning());
  EXPECT_FALSE(transaction->HasPendingTasks());
  EXPECT_EQ(transaction->diagnostics().tasks_completed,
            transaction->diagnostics().tasks_scheduled);
}

TEST_F(IndexedDBTransactionTest, IndexedDBObserver) {
  const int64_t id = 0;
  const std::set<int64_t> scope;
  const leveldb::Status commit_success = leveldb::Status::OK();
  std::unique_ptr<IndexedDBConnection> connection(
      std::make_unique<IndexedDBConnection>(
          kFakeProcessId, db_, new MockIndexedDBDatabaseCallbacks()));

  base::WeakPtr<IndexedDBTransaction> transaction =
      connection->AddTransactionForTesting(
          std::unique_ptr<IndexedDBTransaction>(new IndexedDBTransaction(
              id, connection.get(), scope,
              blink::kWebIDBTransactionModeReadWrite,
              new IndexedDBFakeBackingStore::FakeTransaction(commit_success))));
  ASSERT_TRUE(transaction);
  db_->TransactionCreated(transaction.get());

  EXPECT_EQ(0UL, transaction->pending_observers_.size());
  EXPECT_EQ(0UL, connection->active_observers().size());

  // Add observers to pending observer list.
  const int32_t observer_id1 = 1, observer_id2 = 2;
  IndexedDBObserver::Options options(false, false, false, 0U);
  transaction->AddPendingObserver(observer_id1, options);
  transaction->AddPendingObserver(observer_id2, options);
  EXPECT_EQ(2UL, transaction->pending_observers_.size());
  EXPECT_EQ(0UL, connection->active_observers().size());

  // Before commit, observer would be in pending list of transaction.
  std::vector<int32_t> observer_to_remove1 = {observer_id1};
  connection->RemoveObservers(observer_to_remove1);
  EXPECT_EQ(1UL, transaction->pending_observers_.size());
  EXPECT_EQ(0UL, connection->active_observers().size());

  // After commit, observer moved to connection's active observer.
  transaction->Commit();
  EXPECT_FALSE(transaction);
  EXPECT_EQ(1UL, connection->active_observers().size());

  // Observer does not exist, so no change to active_observers.
  connection->RemoveObservers(observer_to_remove1);
  EXPECT_EQ(1UL, connection->active_observers().size());

  // Observer removed from connection's active observer.
  std::vector<int32_t> observer_to_remove2 = {observer_id2};
  connection->RemoveObservers(observer_to_remove2);
  EXPECT_EQ(0UL, connection->active_observers().size());
}

static const blink::WebIDBTransactionMode kTestModes[] = {
    blink::kWebIDBTransactionModeReadOnly,
    blink::kWebIDBTransactionModeReadWrite,
    blink::kWebIDBTransactionModeVersionChange};

INSTANTIATE_TEST_CASE_P(IndexedDBTransactions,
                        IndexedDBTransactionTestMode,
                        ::testing::ValuesIn(kTestModes));

}  // namespace indexed_db_transaction_unittest
}  // namespace content
