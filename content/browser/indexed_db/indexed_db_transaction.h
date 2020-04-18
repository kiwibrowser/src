// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TRANSACTION_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TRANSACTION_H_

#include <stdint.h>

#include <memory>
#include <set>

#include "base/containers/queue.h"
#include "base/containers/stack.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "content/browser/indexed_db/indexed_db_connection.h"
#include "content/browser/indexed_db/indexed_db_database.h"
#include "content/browser/indexed_db/indexed_db_database_error.h"
#include "content/browser/indexed_db/indexed_db_observer.h"
#include "content/common/indexed_db/indexed_db.mojom.h"
#include "third_party/blink/public/platform/modules/indexeddb/web_idb_types.h"

namespace content {

class BlobWriteCallbackImpl;
class IndexedDBCursor;
class IndexedDBDatabaseCallbacks;

namespace indexed_db_transaction_unittest {
class IndexedDBTransactionTestMode;
class IndexedDBTransactionTest;
FORWARD_DECLARE_TEST(IndexedDBTransactionTestMode, AbortPreemptive);
FORWARD_DECLARE_TEST(IndexedDBTransactionTestMode, AbortTasks);
FORWARD_DECLARE_TEST(IndexedDBTransactionTest, NoTimeoutReadOnly);
FORWARD_DECLARE_TEST(IndexedDBTransactionTest, SchedulePreemptiveTask);
FORWARD_DECLARE_TEST(IndexedDBTransactionTestMode, ScheduleNormalTask);
FORWARD_DECLARE_TEST(IndexedDBTransactionTestMode, TaskFails);
FORWARD_DECLARE_TEST(IndexedDBTransactionTest, Timeout);
FORWARD_DECLARE_TEST(IndexedDBTransactionTest, IndexedDBObserver);
}  // namespace indexed_db_transaction_unittest

class CONTENT_EXPORT IndexedDBTransaction {
 public:
  using Operation = base::OnceCallback<leveldb::Status(IndexedDBTransaction*)>;
  using AbortOperation = base::OnceClosure;

  enum State {
    CREATED,     // Created, but not yet started by coordinator.
    STARTED,     // Started by the coordinator.
    COMMITTING,  // In the process of committing, possibly waiting for blobs
                 // to be written.
    FINISHED,    // Either aborted or committed.
  };

  virtual ~IndexedDBTransaction();

  leveldb::Status Commit();

  // This object is destroyed by this method.
  void Abort(const IndexedDBDatabaseError& error);

  // Called by the transaction coordinator when this transaction is unblocked.
  void Start();

  // Grabs a snapshot from the database immediately, then starts the
  // transaction.
  void GrabSnapshotThenStart();

  blink::WebIDBTransactionMode mode() const { return mode_; }
  const std::set<int64_t>& scope() const { return object_store_ids_; }

  // Tasks cannot call Commit.
  void ScheduleTask(Operation task) {
    ScheduleTask(blink::kWebIDBTaskTypeNormal, std::move(task));
  }
  void ScheduleTask(blink::WebIDBTaskType, Operation task);
  void ScheduleAbortTask(AbortOperation abort_task);
  void RegisterOpenCursor(IndexedDBCursor* cursor);
  void UnregisterOpenCursor(IndexedDBCursor* cursor);
  void AddPreemptiveEvent() { pending_preemptive_events_++; }
  void DidCompletePreemptiveEvent() {
    pending_preemptive_events_--;
    DCHECK_GE(pending_preemptive_events_, 0);
  }
  void AddPendingObserver(int32_t observer_id,
                          const IndexedDBObserver::Options& options);
  // Delete pending observers with ID's listed in |pending_observer_ids|.
  void RemovePendingObservers(const std::vector<int32_t>& pending_observer_ids);

  // Adds observation for the connection.
  void AddObservation(int32_t connection_id,
                      ::indexed_db::mojom::ObservationPtr observation);

  ::indexed_db::mojom::ObserverChangesPtr* GetPendingChangesForConnection(
      int32_t connection_id);

  IndexedDBBackingStore::Transaction* BackingStoreTransaction() {
    return transaction_.get();
  }
  int64_t id() const { return id_; }

  IndexedDBDatabase* database() const { return database_.get(); }
  IndexedDBDatabaseCallbacks* callbacks() const { return callbacks_.get(); }
  IndexedDBConnection* connection() const { return connection_.get(); }

  State state() const { return state_; }
  bool IsTimeoutTimerRunning() const { return timeout_timer_.IsRunning(); }

  struct Diagnostics {
    base::Time creation_time;
    base::Time start_time;
    int tasks_scheduled;
    int tasks_completed;
  };

  const Diagnostics& diagnostics() const { return diagnostics_; }

  void set_size(int64_t size) { size_ = size; }
  int64_t size() const { return size_; }

 protected:
  // Test classes may derive, but most creation should be done via
  // IndexedDBClassFactory.
  IndexedDBTransaction(
      int64_t id,
      IndexedDBConnection* connection,
      const std::set<int64_t>& object_store_ids,
      blink::WebIDBTransactionMode mode,
      IndexedDBBackingStore::Transaction* backing_store_transaction);

  // May be overridden in tests.
  virtual base::TimeDelta GetInactivityTimeout() const;

 private:
  friend class BlobWriteCallbackImpl;
  friend class IndexedDBClassFactory;
  friend class IndexedDBConnection;
  friend class base::RefCounted<IndexedDBTransaction>;

  FRIEND_TEST_ALL_PREFIXES(
      indexed_db_transaction_unittest::IndexedDBTransactionTestMode,
      AbortPreemptive);
  FRIEND_TEST_ALL_PREFIXES(
      indexed_db_transaction_unittest::IndexedDBTransactionTestMode,
      AbortTasks);
  FRIEND_TEST_ALL_PREFIXES(
      indexed_db_transaction_unittest::IndexedDBTransactionTest,
      NoTimeoutReadOnly);
  FRIEND_TEST_ALL_PREFIXES(
      indexed_db_transaction_unittest::IndexedDBTransactionTest,
      SchedulePreemptiveTask);
  FRIEND_TEST_ALL_PREFIXES(
      indexed_db_transaction_unittest::IndexedDBTransactionTestMode,
      ScheduleNormalTask);
  FRIEND_TEST_ALL_PREFIXES(
      indexed_db_transaction_unittest::IndexedDBTransactionTestMode,
      TaskFails);
  FRIEND_TEST_ALL_PREFIXES(
      indexed_db_transaction_unittest::IndexedDBTransactionTest,
      Timeout);
  FRIEND_TEST_ALL_PREFIXES(
      indexed_db_transaction_unittest::IndexedDBTransactionTest,
      IndexedDBObserver);

  void RunTasksIfStarted();

  bool IsTaskQueueEmpty() const;
  bool HasPendingTasks() const;

  leveldb::Status BlobWriteComplete(
      IndexedDBBackingStore::BlobWriteResult result);
  void ProcessTaskQueue();
  void CloseOpenCursors();
  leveldb::Status CommitPhaseTwo();
  void Timeout();

  const int64_t id_;
  const std::set<int64_t> object_store_ids_;
  const blink::WebIDBTransactionMode mode_;

  bool used_ = false;
  State state_ = CREATED;
  bool commit_pending_ = false;
  // We are owned by the connection object, but during force closes sometimes
  // there are issues if there is a pending OpenRequest. So use a WeakPtr.
  base::WeakPtr<IndexedDBConnection> connection_;
  scoped_refptr<IndexedDBDatabaseCallbacks> callbacks_;
  scoped_refptr<IndexedDBDatabase> database_;

  // Observers in pending queue do not listen to changes until activated.
  std::vector<std::unique_ptr<IndexedDBObserver>> pending_observers_;
  std::map<int32_t, ::indexed_db::mojom::ObserverChangesPtr>
      connection_changes_map_;

  // Metrics for quota.
  int64_t size_ = 0;

  class TaskQueue {
   public:
    TaskQueue();
    ~TaskQueue();
    bool empty() const { return queue_.empty(); }
    void push(Operation task) { queue_.push(std::move(task)); }
    Operation pop();
    void clear();

   private:
    base::queue<Operation> queue_;

    DISALLOW_COPY_AND_ASSIGN(TaskQueue);
  };

  class TaskStack {
   public:
    TaskStack();
    ~TaskStack();
    bool empty() const { return stack_.empty(); }
    void push(AbortOperation task) { stack_.push(std::move(task)); }
    AbortOperation pop();
    void clear();

   private:
    base::stack<AbortOperation> stack_;

    DISALLOW_COPY_AND_ASSIGN(TaskStack);
  };

  TaskQueue task_queue_;
  TaskQueue preemptive_task_queue_;
  TaskStack abort_task_stack_;

  std::unique_ptr<IndexedDBBackingStore::Transaction> transaction_;
  bool backing_store_transaction_begun_ = false;

  bool should_process_queue_ = false;
  int pending_preemptive_events_ = 0;
  bool processing_event_queue_ = false;

  std::set<IndexedDBCursor*> open_cursors_;

  // This timer is started after requests have been processed. If no subsequent
  // requests are processed before the timer fires, assume the script is
  // unresponsive and abort to unblock the transaction queue.
  base::OneShotTimer timeout_timer_;

  Diagnostics diagnostics_;

  base::WeakPtrFactory<IndexedDBTransaction> ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TRANSACTION_H_
