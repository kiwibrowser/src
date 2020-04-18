// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/request_queue_store_sql.h"

#include <unordered_set>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/background/save_page_request.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace offline_pages {

template class StoreUpdateResult<SavePageRequest>;

namespace {

using SuccessCallback = base::OnceCallback<void(bool)>;

// This is a macro instead of a const so that
// it can be used inline in other SQL statements below.
#define REQUEST_QUEUE_TABLE_NAME "request_queue_v1"
const bool kUserRequested = true;

bool CreateRequestQueueTable(sql::Connection* db) {
  const char kSql[] = "CREATE TABLE IF NOT EXISTS " REQUEST_QUEUE_TABLE_NAME
                      " (request_id INTEGER PRIMARY KEY NOT NULL,"
                      " creation_time INTEGER NOT NULL,"
                      " activation_time INTEGER NOT NULL DEFAULT 0,"
                      " last_attempt_time INTEGER NOT NULL DEFAULT 0,"
                      " started_attempt_count INTEGER NOT NULL,"
                      " completed_attempt_count INTEGER NOT NULL,"
                      " state INTEGER NOT NULL DEFAULT 0,"
                      " url VARCHAR NOT NULL,"
                      " client_namespace VARCHAR NOT NULL,"
                      " client_id VARCHAR NOT NULL,"
                      " original_url VARCHAR NOT NULL DEFAULT '',"
                      " request_origin VARCHAR NOT NULL DEFAULT '',"
                      " fail_state INTEGER NOT NULL DEFAULT 0"
                      ")";
  return db->Execute(kSql);
}

bool UpgradeWithQuery(sql::Connection* db, const char* upgrade_sql) {
  if (!db->Execute("ALTER TABLE " REQUEST_QUEUE_TABLE_NAME
                   " RENAME TO temp_" REQUEST_QUEUE_TABLE_NAME)) {
    return false;
  }
  if (!CreateRequestQueueTable(db))
    return false;
  if (!db->Execute(upgrade_sql))
    return false;
  return db->Execute("DROP TABLE IF EXISTS temp_" REQUEST_QUEUE_TABLE_NAME);
}

bool UpgradeFrom57(sql::Connection* db) {
  const char kSql[] =
      "INSERT INTO " REQUEST_QUEUE_TABLE_NAME
      " (request_id, creation_time, activation_time, last_attempt_time, "
      "started_attempt_count, completed_attempt_count, state, url, "
      "client_namespace, client_id) "
      "SELECT "
      "request_id, creation_time, activation_time, last_attempt_time, "
      "started_attempt_count, completed_attempt_count, state, url, "
      "client_namespace, client_id "
      "FROM temp_" REQUEST_QUEUE_TABLE_NAME;
  return UpgradeWithQuery(db, kSql);
}

bool UpgradeFrom58(sql::Connection* db) {
  const char kSql[] =
      "INSERT INTO " REQUEST_QUEUE_TABLE_NAME
      " (request_id, creation_time, activation_time, last_attempt_time, "
      "started_attempt_count, completed_attempt_count, state, url, "
      "client_namespace, client_id, original_url) "
      "SELECT "
      "request_id, creation_time, activation_time, last_attempt_time, "
      "started_attempt_count, completed_attempt_count, state, url, "
      "client_namespace, client_id, original_url "
      "FROM temp_" REQUEST_QUEUE_TABLE_NAME;
  return UpgradeWithQuery(db, kSql);
}

bool UpgradeFrom61(sql::Connection* db) {
  const char kSql[] =
      "INSERT INTO " REQUEST_QUEUE_TABLE_NAME
      " (request_id, creation_time, activation_time, last_attempt_time, "
      "started_attempt_count, completed_attempt_count, state, url, "
      "client_namespace, client_id, original_url, request_origin) "
      "SELECT "
      "request_id, creation_time, activation_time, last_attempt_time, "
      "started_attempt_count, completed_attempt_count, state, url, "
      "client_namespace, client_id, original_url, request_origin "
      "FROM temp_" REQUEST_QUEUE_TABLE_NAME;
  return UpgradeWithQuery(db, kSql);
}

bool CreateSchema(sql::Connection* db) {
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  if (!db->DoesTableExist(REQUEST_QUEUE_TABLE_NAME)) {
    if (!CreateRequestQueueTable(db))
      return false;
  }

  // If there is not already a state column, we need to drop the old table. We
  // are choosing to drop instead of upgrade since the feature is not yet
  // released, so we don't try to migrate it.
  if (!db->DoesColumnExist(REQUEST_QUEUE_TABLE_NAME, "state")) {
    if (!db->Execute("DROP TABLE IF EXISTS " REQUEST_QUEUE_TABLE_NAME))
      return false;
  }

  if (!db->DoesColumnExist(REQUEST_QUEUE_TABLE_NAME, "original_url")) {
    if (!UpgradeFrom57(db))
      return false;
  } else if (!db->DoesColumnExist(REQUEST_QUEUE_TABLE_NAME, "request_origin")) {
    if (!UpgradeFrom58(db))
      return false;
  } else if (!db->DoesColumnExist(REQUEST_QUEUE_TABLE_NAME, "fail_state")) {
    if (!UpgradeFrom61(db))
      return false;
  }

  // This would be a great place to add indices when we need them.
  return transaction.Commit();
}

// Create a save page request from a SQL result.  Expects complete rows with
// all columns present.  Columns are in order they are defined in select query
// in |GetOneRequest| method.
std::unique_ptr<SavePageRequest> MakeSavePageRequest(
    const sql::Statement& statement) {
  const int64_t id = statement.ColumnInt64(0);
  const base::Time creation_time =
      store_utils::FromDatabaseTime(statement.ColumnInt64(1));
  const base::Time last_attempt_time =
      store_utils::FromDatabaseTime(statement.ColumnInt64(3));
  const int64_t started_attempt_count = statement.ColumnInt64(4);
  const int64_t completed_attempt_count = statement.ColumnInt64(5);
  const SavePageRequest::RequestState state =
      static_cast<SavePageRequest::RequestState>(statement.ColumnInt64(6));
  const GURL url(statement.ColumnString(7));
  const ClientId client_id(statement.ColumnString(8),
                           statement.ColumnString(9));
  const GURL original_url(statement.ColumnString(10));
  const std::string request_origin(statement.ColumnString(11));
  const FailState fail_state =
      static_cast<FailState>(statement.ColumnInt64(12));

  DVLOG(2) << "making save page request - id " << id << " url " << url
           << " client_id " << client_id.name_space << "-" << client_id.id
           << " creation time " << creation_time << " user requested "
           << kUserRequested << " original_url " << original_url
           << " request_origin " << request_origin;

  std::unique_ptr<SavePageRequest> request(
      new SavePageRequest(id, url, client_id, creation_time, kUserRequested));
  request->set_last_attempt_time(last_attempt_time);
  request->set_started_attempt_count(started_attempt_count);
  request->set_completed_attempt_count(completed_attempt_count);
  request->set_request_state(state);
  request->set_original_url(original_url);
  request->set_request_origin(request_origin);
  request->set_fail_state(fail_state);
  return request;
}

// Get a request for a specific id.
std::unique_ptr<SavePageRequest> GetOneRequest(sql::Connection* db,
                                               const int64_t request_id) {
  const char kSql[] =
      "SELECT request_id, creation_time, activation_time,"
      " last_attempt_time, started_attempt_count, completed_attempt_count,"
      " state, url, client_namespace, client_id, original_url, request_origin,"
      " fail_state"
      " FROM " REQUEST_QUEUE_TABLE_NAME " WHERE request_id=?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, request_id);

  if (statement.Step())
    return MakeSavePageRequest(statement);
  return std::unique_ptr<SavePageRequest>(nullptr);
}

ItemActionStatus DeleteRequestById(sql::Connection* db, int64_t request_id) {
  const char kSql[] =
      "DELETE FROM " REQUEST_QUEUE_TABLE_NAME " WHERE request_id=?";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, request_id);
  if (!statement.Run())
    return ItemActionStatus::STORE_ERROR;
  if (db->GetLastChangeCount() == 0)
    return ItemActionStatus::NOT_FOUND;
  return ItemActionStatus::SUCCESS;
}

ItemActionStatus Insert(sql::Connection* db, const SavePageRequest& request) {
  const char kSql[] =
      "INSERT OR IGNORE INTO " REQUEST_QUEUE_TABLE_NAME
      " (request_id, creation_time, activation_time,"
      " last_attempt_time, started_attempt_count, completed_attempt_count,"
      " state, url, client_namespace, client_id, original_url, request_origin,"
      " fail_state)"
      " VALUES "
      " (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, request.request_id());
  statement.BindInt64(1, store_utils::ToDatabaseTime(request.creation_time()));
  statement.BindInt64(2, 0);
  statement.BindInt64(3,
                      store_utils::ToDatabaseTime(request.last_attempt_time()));
  statement.BindInt64(4, request.started_attempt_count());
  statement.BindInt64(5, request.completed_attempt_count());
  statement.BindInt64(6, static_cast<int64_t>(request.request_state()));
  statement.BindString(7, request.url().spec());
  statement.BindString(8, request.client_id().name_space);
  statement.BindString(9, request.client_id().id);
  statement.BindString(10, request.original_url().spec());
  statement.BindString(11, request.request_origin());
  statement.BindInt64(12, static_cast<int64_t>(request.fail_state()));

  if (!statement.Run())
    return ItemActionStatus::STORE_ERROR;
  if (db->GetLastChangeCount() == 0)
    return ItemActionStatus::ALREADY_EXISTS;
  return ItemActionStatus::SUCCESS;
}

ItemActionStatus Update(sql::Connection* db, const SavePageRequest& request) {
  const char kSql[] =
      "UPDATE OR IGNORE " REQUEST_QUEUE_TABLE_NAME
      " SET creation_time = ?, activation_time = ?, last_attempt_time = ?,"
      " started_attempt_count = ?, completed_attempt_count = ?, state = ?,"
      " url = ?, client_namespace = ?, client_id = ?, original_url = ?,"
      " request_origin = ?, fail_state = ?"
      " WHERE request_id = ?";

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  statement.BindInt64(0, store_utils::ToDatabaseTime(request.creation_time()));
  statement.BindInt64(1, 0);
  statement.BindInt64(2,
                      store_utils::ToDatabaseTime(request.last_attempt_time()));
  statement.BindInt64(3, request.started_attempt_count());
  statement.BindInt64(4, request.completed_attempt_count());
  statement.BindInt64(5, static_cast<int64_t>(request.request_state()));
  statement.BindString(6, request.url().spec());
  statement.BindString(7, request.client_id().name_space);
  statement.BindString(8, request.client_id().id);
  statement.BindString(9, request.original_url().spec());
  statement.BindString(10, request.request_origin());
  statement.BindInt64(11, static_cast<int64_t>(request.fail_state()));
  statement.BindInt64(12, request.request_id());

  if (!statement.Run())
    return ItemActionStatus::STORE_ERROR;
  if (db->GetLastChangeCount() == 0)
    return ItemActionStatus::NOT_FOUND;
  return ItemActionStatus::SUCCESS;
}

void PostStoreUpdateResultForIds(
    scoped_refptr<base::SingleThreadTaskRunner> runner,
    StoreState store_state,
    const std::vector<int64_t>& item_ids,
    ItemActionStatus action_status,
    RequestQueueStore::UpdateCallback callback) {
  std::unique_ptr<UpdateRequestsResult> result(
      new UpdateRequestsResult(store_state));
  for (const auto& item_id : item_ids)
    result->item_statuses.push_back(std::make_pair(item_id, action_status));
  runner->PostTask(FROM_HERE,
                   base::BindOnce(std::move(callback), std::move(result)));
}

void PostStoreErrorForAllRequests(
    scoped_refptr<base::SingleThreadTaskRunner> runner,
    const std::vector<SavePageRequest>& items,
    RequestQueueStore::UpdateCallback callback) {
  std::vector<int64_t> item_ids;
  for (const auto& item : items)
    item_ids.push_back(item.request_id());
  PostStoreUpdateResultForIds(runner, StoreState::LOADED, item_ids,
                              ItemActionStatus::STORE_ERROR,
                              std::move(callback));
}

void PostStoreErrorForAllIds(scoped_refptr<base::SingleThreadTaskRunner> runner,
                             const std::vector<int64_t>& item_ids,
                             RequestQueueStore::UpdateCallback callback) {
  PostStoreUpdateResultForIds(runner, StoreState::LOADED, item_ids,
                              ItemActionStatus::STORE_ERROR,
                              std::move(callback));
}

bool InitDatabase(sql::Connection* db, const base::FilePath& path) {
  db->set_page_size(4096);
  db->set_cache_size(500);
  db->set_histogram_tag("BackgroundRequestQueue");
  db->set_exclusive_locking();

  base::File::Error err;
  if (!base::CreateDirectoryAndGetError(path.DirName(), &err))
    return false;
  if (!db->Open(path))
    return false;
  db->Preload();

  return CreateSchema(db);
}

void GetRequestsSync(sql::Connection* db,
                     scoped_refptr<base::SingleThreadTaskRunner> runner,
                     RequestQueueStore::GetRequestsCallback callback) {
  const char kSql[] =
      "SELECT request_id, creation_time, activation_time,"
      " last_attempt_time, started_attempt_count, completed_attempt_count,"
      " state, url, client_namespace, client_id, original_url, request_origin,"
      " fail_state"
      " FROM " REQUEST_QUEUE_TABLE_NAME;

  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));

  std::vector<std::unique_ptr<SavePageRequest>> requests;
  while (statement.Step())
    requests.push_back(MakeSavePageRequest(statement));

  runner->PostTask(FROM_HERE,
                   base::BindOnce(std::move(callback), statement.Succeeded(),
                                  std::move(requests)));
}

void GetRequestsByIdsSync(sql::Connection* db,
                          scoped_refptr<base::SingleThreadTaskRunner> runner,
                          const std::vector<int64_t>& request_ids,
                          RequestQueueStore::UpdateCallback callback) {
  std::unique_ptr<UpdateRequestsResult> result(
      new UpdateRequestsResult(StoreState::LOADED));

  // If you create a transaction but don't Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    PostStoreErrorForAllIds(runner, request_ids, std::move(callback));
    return;
  }

  // Make sure not to include the same request multiple times, preserving the
  // order of non-duplicated IDs in the result.
  std::unordered_set<int64_t> processed_ids;
  for (int64_t request_id : request_ids) {
    if (!processed_ids.insert(request_id).second)
      continue;
    std::unique_ptr<SavePageRequest> request = GetOneRequest(db, request_id);
    if (request)
      result->updated_items.push_back(*request);
    ItemActionStatus status =
        request ? ItemActionStatus::SUCCESS : ItemActionStatus::NOT_FOUND;
    result->item_statuses.push_back(std::make_pair(request_id, status));
  }

  if (!transaction.Commit()) {
    PostStoreErrorForAllIds(runner, request_ids, std::move(callback));
    return;
  }

  runner->PostTask(FROM_HERE,
                   base::BindOnce(std::move(callback), std::move(result)));
}

void AddRequestSync(sql::Connection* db,
                    scoped_refptr<base::SingleThreadTaskRunner> runner,
                    const SavePageRequest& request,
                    RequestQueueStore::AddCallback callback) {
  ItemActionStatus status = Insert(db, request);
  runner->PostTask(FROM_HERE, base::BindOnce(std::move(callback), status));
}

void UpdateRequestsSync(sql::Connection* db,
                        scoped_refptr<base::SingleThreadTaskRunner> runner,
                        const std::vector<SavePageRequest>& requests,
                        RequestQueueStore::UpdateCallback callback) {
  std::unique_ptr<UpdateRequestsResult> result(
      new UpdateRequestsResult(StoreState::LOADED));

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    PostStoreErrorForAllRequests(runner, requests, std::move(callback));
    return;
  }

  for (const auto& request : requests) {
    ItemActionStatus status = Update(db, request);
    result->item_statuses.push_back(
        std::make_pair(request.request_id(), status));
    if (status == ItemActionStatus::SUCCESS)
      result->updated_items.push_back(request);
  }

  if (!transaction.Commit()) {
    PostStoreErrorForAllRequests(runner, requests, std::move(callback));
    return;
  }

  runner->PostTask(FROM_HERE,
                   base::BindOnce(std::move(callback), std::move(result)));
}

void RemoveRequestsSync(sql::Connection* db,
                        scoped_refptr<base::SingleThreadTaskRunner> runner,
                        const std::vector<int64_t>& request_ids,
                        RequestQueueStore::UpdateCallback callback) {
  std::unique_ptr<UpdateRequestsResult> result(
      new UpdateRequestsResult(StoreState::LOADED));

  // If you create a transaction but don't Commit() it is automatically
  // rolled back by its destructor when it falls out of scope.
  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    PostStoreErrorForAllIds(runner, request_ids, std::move(callback));
    return;
  }

  // Read the request before we delete it, and if the delete worked, put it on
  // the queue of requests that got deleted.
  for (int64_t request_id : request_ids) {
    std::unique_ptr<SavePageRequest> request = GetOneRequest(db, request_id);
    ItemActionStatus status = DeleteRequestById(db, request_id);
    result->item_statuses.push_back(std::make_pair(request_id, status));
    if (status == ItemActionStatus::SUCCESS)
      result->updated_items.push_back(*request);
  }

  if (!transaction.Commit()) {
    PostStoreErrorForAllIds(runner, request_ids, std::move(callback));
    return;
  }

  runner->PostTask(FROM_HERE,
                   base::BindOnce(std::move(callback), std::move(result)));
}

void OpenConnectionSync(sql::Connection* db,
                        scoped_refptr<base::SingleThreadTaskRunner> runner,
                        const base::FilePath& path,
                        SuccessCallback callback) {
  bool success = InitDatabase(db, path);
  runner->PostTask(FROM_HERE, base::BindOnce(std::move(callback), success));
}

void ResetSync(sql::Connection* db,
               const base::FilePath& db_file_path,
               scoped_refptr<base::SingleThreadTaskRunner> runner,
               SuccessCallback callback) {
  // This method deletes the content of the whole store and reinitializes it.
  bool success = true;
  if (db) {
    success = db->Raze();
    db->Close();
  }
  success = base::DeleteFile(db_file_path, true /* recursive */) && success;
  runner->PostTask(FROM_HERE, base::BindOnce(std::move(callback), success));
}

}  // anonymous namespace

RequestQueueStoreSQL::RequestQueueStoreSQL(
    scoped_refptr<base::SequencedTaskRunner> background_task_runner,
    const base::FilePath& path)
    : background_task_runner_(std::move(background_task_runner)),
      db_file_path_(path.AppendASCII("RequestQueue.db")),
      state_(StoreState::NOT_LOADED),
      weak_ptr_factory_(this) {}

RequestQueueStoreSQL::~RequestQueueStoreSQL() {
  if (db_)
    background_task_runner_->DeleteSoon(FROM_HERE, db_.release());
}

void RequestQueueStoreSQL::Initialize(InitializeCallback callback) {
  DCHECK(!db_);
  db_.reset(new sql::Connection());
  background_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &OpenConnectionSync, db_.get(), base::ThreadTaskRunnerHandle::Get(),
          db_file_path_,
          base::BindOnce(&RequestQueueStoreSQL::OnOpenConnectionDone,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
}

void RequestQueueStoreSQL::GetRequests(GetRequestsCallback callback) {
  DCHECK(db_);
  if (!CheckDb()) {
    std::vector<std::unique_ptr<SavePageRequest>> requests;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), false, std::move(requests)));
    return;
  }

  background_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&GetRequestsSync, db_.get(),
                     base::ThreadTaskRunnerHandle::Get(), std::move(callback)));
}

void RequestQueueStoreSQL::GetRequestsByIds(
    const std::vector<int64_t>& request_ids,
    UpdateCallback callback) {
  if (!CheckDb()) {
    PostStoreErrorForAllIds(base::ThreadTaskRunnerHandle::Get(), request_ids,
                            std::move(callback));
    return;
  }

  background_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&GetRequestsByIdsSync, db_.get(),
                                base::ThreadTaskRunnerHandle::Get(),
                                request_ids, std::move(callback)));
}

void RequestQueueStoreSQL::AddRequest(const SavePageRequest& request,
                                      AddCallback callback) {
  if (!CheckDb()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), ItemActionStatus::STORE_ERROR));
    return;
  }

  background_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AddRequestSync, db_.get(),
                                base::ThreadTaskRunnerHandle::Get(), request,
                                std::move(callback)));
}

void RequestQueueStoreSQL::UpdateRequests(
    const std::vector<SavePageRequest>& requests,
    UpdateCallback callback) {
  if (!CheckDb()) {
    PostStoreErrorForAllRequests(base::ThreadTaskRunnerHandle::Get(), requests,
                                 std::move(callback));
    return;
  }

  background_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&UpdateRequestsSync, db_.get(),
                                base::ThreadTaskRunnerHandle::Get(), requests,
                                std::move(callback)));
}

void RequestQueueStoreSQL::RemoveRequests(
    const std::vector<int64_t>& request_ids,
    UpdateCallback callback) {
  if (!CheckDb()) {
    PostStoreErrorForAllIds(base::ThreadTaskRunnerHandle::Get(), request_ids,
                            std::move(callback));
    return;
  }

  background_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&RemoveRequestsSync, db_.get(),
                                base::ThreadTaskRunnerHandle::Get(),
                                request_ids, std::move(callback)));
}

void RequestQueueStoreSQL::Reset(ResetCallback callback) {
  background_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ResetSync, db_.get(), db_file_path_,
          base::ThreadTaskRunnerHandle::Get(),
          base::BindOnce(&RequestQueueStoreSQL::OnResetDone,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback))));
}

StoreState RequestQueueStoreSQL::state() const {
  return state_;
}

void RequestQueueStoreSQL::OnOpenConnectionDone(InitializeCallback callback,
                                                bool success) {
  DCHECK(db_);
  state_ = success ? StoreState::LOADED : StoreState::FAILED_LOADING;
  std::move(callback).Run(success);
}

void RequestQueueStoreSQL::OnResetDone(ResetCallback callback, bool success) {
  state_ = success ? StoreState::NOT_LOADED : StoreState::FAILED_RESET;
  db_.reset();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), success));
}

bool RequestQueueStoreSQL::CheckDb() const {
  return db_ && state_ == StoreState::LOADED;
}

}  // namespace offline_pages
