// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_METADATA_STORE_SQL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_METADATA_STORE_SQL_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_store_types.h"

namespace base {
class SequencedTaskRunner;
}

namespace sql {
class Connection;
}

namespace offline_pages {
typedef StoreUpdateResult<OfflinePageItem> OfflinePagesUpdateResult;

// OfflinePageMetadataStoreSQL keeps metadata for the offline pages in an SQLite
// database.
//
// This store has a history of schema updates in pretty much every release.
// Original schema was delivered in M52. Since then, the following changes
// happened:
// * In M53 expiration_time was added,
// * In M54 title was added,
// * In M55 we dropped the following fields (never used): version, status,
//   offline_url, user_initiated.
// * In M56 original_url was added.
// * In M57 expiration_time was dropped. Existing expired pages would be
//   removed when metadata consistency check happens.
// * In M58-M60 there were no changes.
// * In M61 request_origin was added.
// * In M62 system_download_id, file_missing_time, upgrade_attempt and digest
//   were added to support P2P sharing feature.
//
// Here is a procedure to update the schema for this store:
// * Decide how to detect that the store is on a particular version, which
//   typically means that a certain field exists or is missing. This happens in
//   Upgrade section of |CreateSchema|
// * Work out appropriate change and apply it to all existing upgrade paths. In
//   the interest of performing a single update of the store, it upgrades from a
//   detected version to the current one. This means that when making a change,
//   more than a single query may have to be updated (in case of fields being
//   removed or needed to be initialized to a specific, non-default value).
//   Such approach is preferred to doing N updates for every changed version on
//   a startup after browser update.
// * New upgrade method should specify which version it is upgrading from, e.g.
//   |UpgradeFrom54|.
// * Upgrade should use |UpgradeWithQuery| and simply specify SQL command to
//   move data from old table (prefixed by temp_) to the new one.
class OfflinePageMetadataStoreSQL {
 public:
  // This enum is used in an UMA histogram. Hence the entries here shouldn't
  // be deleted or re-ordered and new ones should be added to the end.
  enum LoadStatus {
    LOAD_SUCCEEDED,
    STORE_INIT_FAILED,
    STORE_LOAD_FAILED,
    DATA_PARSING_FAILED,

    // NOTE: always keep this entry at the end.
    LOAD_STATUS_COUNT
  };

  typedef base::RepeatingCallback<void(bool /* success */)> ResetCallback;

  // Definition of the callback that is going to run the core of the command in
  // the |Execute| method.
  template <typename T>
  using RunCallback = base::OnceCallback<T(sql::Connection*)>;

  // Definition of the callback used to pass the result back to the caller of
  // |Execute| method.
  template <typename T>
  using ResultCallback = base::OnceCallback<void(T)>;

  // This is the first version saved in the meta table, which was introduced in
  // the store in M65. It is set once a legacy upgrade is run successfully for
  // the last time in |UpgradeFromLegacyVersion|.
  static const int kFirstPostLegacyVersion = 1;
  static const int kCurrentVersion = 3;
  static const int kCompatibleVersion = kFirstPostLegacyVersion;

  // Defines inactivity time of DB after which it is going to be closed.
  // TODO(fgorski): Derive appropriate value in a scientific way.
  static constexpr base::TimeDelta kClosingDelay =
      base::TimeDelta::FromSeconds(20);

  // TODO(fgorski): Move to private and expose ForTest factory.
  // Applies in PrefetchStore as well.
  // Creates the store in memory. Should only be used for testing.
  explicit OfflinePageMetadataStoreSQL(
      scoped_refptr<base::SequencedTaskRunner> background_task_runner);

  // Creates the store with database pointing to provided directory.
  OfflinePageMetadataStoreSQL(
      scoped_refptr<base::SequencedTaskRunner> background_task_runner,
      const base::FilePath& database_dir);

  ~OfflinePageMetadataStoreSQL();

  // Executes a |run_callback| on SQL store on the blocking thread, and posts
  // its result back to calling thread through |result_callback|.
  // Calling |Execute| when store is NOT_LOADED will cause the store
  // initialization to start.
  // Store state needs to be LOADED for test task to run, or FAILURE, in which
  // case the |db| pointer passed to |run_callback| will be null and such case
  // should be gracefully handled.
  template <typename T>
  void Execute(RunCallback<T> run_callback, ResultCallback<T> result_callback) {
    // TODO(fgorski): Add a proper state indicating in progress initialization
    // and CHECK that state.

    if (state_ == StoreState::NOT_LOADED) {
      InitializeInternal(
          base::BindOnce(&OfflinePageMetadataStoreSQL::Execute<T>,
                         weak_ptr_factory_.GetWeakPtr(),
                         std::move(run_callback), std::move(result_callback)));
      return;
    }

    TRACE_EVENT_ASYNC_BEGIN1("offline_pages", "Metadata Store: task execution",
                             this, "is store loaded",
                             state_ == StoreState::LOADED);
    // This if allows to run commands later, after store was given a chance to
    // initialize. They would be failing immediately otherwise.
    if (state_ == StoreState::INITIALIZING) {
      pending_commands_.push_back(
          base::BindOnce(&OfflinePageMetadataStoreSQL::Execute<T>,
                         weak_ptr_factory_.GetWeakPtr(),
                         std::move(run_callback), std::move(result_callback)));
      TRACE_EVENT_ASYNC_END1("offline_pages", "Metadata Store: task execution",
                             this, "postponed", true);
      return;
    }

    // Ensure that any scheduled close operations are canceled.
    closing_weak_ptr_factory_.InvalidateWeakPtrs();

    sql::Connection* db = state_ == StoreState::LOADED ? db_.get() : nullptr;

    base::PostTaskAndReplyWithResult(
        background_task_runner_.get(), FROM_HERE,
        base::BindOnce(std::move(run_callback), db),
        base::BindOnce(&OfflinePageMetadataStoreSQL::RescheduleClosing<T>,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(result_callback)));
  }

  // Helper function used to force incorrect state for testing purposes.
  void SetStateForTesting(StoreState state, bool reset_db);
  StoreState GetStateForTesting() const;

 private:
  // Initializes database and calls callback.
  void InitializeInternal(base::OnceClosure pending_command);

  // Used to conclude opening/resetting DB connection.
  void OnInitializeInternalDone(base::OnceClosure pending_command,
                                bool success);

  // Reschedules the closing with a delay. Ensures that |result_callback| is
  // called.
  template <typename T>
  void RescheduleClosing(ResultCallback<T> result_callback, T result) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&OfflinePageMetadataStoreSQL::CloseInternal,
                       closing_weak_ptr_factory_.GetWeakPtr()),
        kClosingDelay);

    // Note: the time recorded for this trace step will include thread hop wait
    // times to the background thread and back.
    TRACE_EVENT_ASYNC_STEP_PAST0(
        "offline_pages", "Metadata Store: task execution", this, "Task");
    std::move(result_callback).Run(std::move(result));
    TRACE_EVENT_ASYNC_STEP_PAST0(
        "offline_pages", "Metadata Store: task execution", this, "Callback");
    TRACE_EVENT_ASYNC_END0("offline_pages", "Metadata Store: task execution",
                           this);
  }

  // Internal function initiating the closing.
  void CloseInternal();

  // Completes the closing. Main purpose is to destroy the db pointer.
  void CloseInternalDone(std::unique_ptr<sql::Connection> db);

  // Background thread where all SQL access should be run.
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // Whether store is opened in memory (for testing) or using a file.
  bool in_memory_;

  // Path to the database on disk.
  base::FilePath db_file_path_;

  // Database connection.
  std::unique_ptr<sql::Connection> db_;

  // State of the store.
  StoreState state_;

  // Pending commands.
  std::vector<base::OnceClosure> pending_commands_;

  // Time of the last time the store was closed. Kept for metrics reporting.
  base::Time last_closing_time_;

  base::WeakPtrFactory<OfflinePageMetadataStoreSQL> weak_ptr_factory_;
  base::WeakPtrFactory<OfflinePageMetadataStoreSQL> closing_weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OfflinePageMetadataStoreSQL);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_METADATA_STORE_SQL_H_
