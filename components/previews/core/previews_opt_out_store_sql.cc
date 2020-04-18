// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_opt_out_store_sql.h"

#include <map>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/previews/core/previews_black_list.h"
#include "components/previews/core/previews_black_list_item.h"
#include "components/previews/core/previews_experiments.h"
#include "sql/connection.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace previews {

namespace {

// Command line switch to change the previews per row DB size.
const char kMaxRowsPerHost[] = "previews-max-opt-out-rows-per-host";

// Command line switch to change the previews DB size.
const char kMaxRows[] = "previews-max-opt-out-rows";

// Returns the maximum number of table rows allowed per host for the previews
// opt out store. This is enforced during insertion of new navigation entries.
int MaxRowsPerHostInOptOutDB() {
  std::string max_rows =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          kMaxRowsPerHost);
  int value;
  return base::StringToInt(max_rows, &value) ? value : 32;
}

// Returns the maximum number of table rows allowed for the previews opt out
// store. This is enforced during load time; thus the database can grow
// larger than this temporarily.
int MaxRowsInOptOutDB() {
  std::string max_rows =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(kMaxRows);
  int value;
  return base::StringToInt(max_rows, &value) ? value : 3200;
}

// Table names use a macro instead of a const, so they can be used inline in
// other SQL statements below.

// The Previews OptOut table holds entries for hosts that should not use a
// specified PreviewsType treatment. Also known as the previews blacklist.
#define PREVIEWS_OPT_OUT_TABLE_NAME "previews_v1"

// The Enabled Previews table hold the list of enabled PreviewsType
// treatments with a version for that enabled treatment. If the version
// changes or the type becomes disabled, then any entries in the OptOut
// table for that treatment type should be cleared.
#define ENABLED_PREVIEWS_TABLE_NAME "enabled_previews_v1"

void CreateSchema(sql::Connection* db) {
  const char kSqlCreatePreviewsTable[] =
      "CREATE TABLE IF NOT EXISTS " PREVIEWS_OPT_OUT_TABLE_NAME
      " (host_name VARCHAR NOT NULL,"
      " time INTEGER NOT NULL,"
      " opt_out INTEGER NOT NULL,"
      " type INTEGER NOT NULL,"
      " PRIMARY KEY(host_name, time DESC, opt_out, type))";
  if (!db->Execute(kSqlCreatePreviewsTable))
    return;

  const char kSqlCreateEnabledTypeVersionTable[] =
      "CREATE TABLE IF NOT EXISTS " ENABLED_PREVIEWS_TABLE_NAME
      " (type INTEGER NOT NULL,"
      " version INTEGER NOT NULL,"
      " PRIMARY KEY(type))";
  if (!db->Execute(kSqlCreateEnabledTypeVersionTable))
    return;
}

void DatabaseErrorCallback(sql::Connection* db,
                           const base::FilePath& db_path,
                           int extended_error,
                           sql::Statement* stmt) {
  if (sql::Recovery::ShouldRecover(extended_error)) {
    // Prevent reentrant calls.
    db->reset_error_callback();

    // After this call, the |db| handle is poisoned so that future calls will
    // return errors until the handle is re-opened.
    sql::Recovery::RecoverDatabase(db, db_path);

    // The DLOG(WARNING) below is intended to draw immediate attention to errors
    // in newly-written code.  Database corruption is generally a result of OS
    // or hardware issues, not coding errors at the client level, so displaying
    // the error would probably lead to confusion.  The ignored call signals the
    // test-expectation framework that the error was handled.
    ignore_result(sql::Connection::IsExpectedSqliteError(extended_error));
    return;
  }

  // The default handling is to assert on debug and to ignore on release.
  if (!sql::Connection::IsExpectedSqliteError(extended_error)) {
    DLOG(WARNING) << db->GetErrorMessage();
    base::UmaHistogramSparse("Previews.OptOut.SQLiteLoadError", extended_error);
  }
}

void InitDatabase(sql::Connection* db, base::FilePath path) {
  // The entry size should be between 11 and 10 + x bytes, where x is the the
  // length of the host name string in bytes.
  // The total number of entries per host is bounded at 32, and the total number
  // of hosts is currently unbounded (but typically expected to be under 100).
  // Assuming average of 100 bytes per entry, and 100 hosts, the total size will
  // be 4096 * 78. 250 allows room for extreme cases such as many host names
  // or very long host names.
  // The average case should be much smaller as users rarely visit hosts that
  // are not in their top 20 hosts. It should be closer to 32 * 100 * 20 for
  // most users, which is about 4096 * 15.
  // The total size of the database will be capped at 3200 entries.
  db->set_page_size(4096);
  db->set_cache_size(250);
  db->set_histogram_tag("PreviewsOptOut");
  db->set_exclusive_locking();

  db->set_error_callback(base::Bind(&DatabaseErrorCallback, db, path));

  base::File::Error err;
  if (!base::CreateDirectoryAndGetError(path.DirName(), &err)) {
    return;
  }
  if (!db->Open(path)) {
    return;
  }

  CreateSchema(db);
}

// Adds a new OptOut entry to the data base.
void AddPreviewNavigationToDataBase(sql::Connection* db,
                                    bool opt_out,
                                    const std::string& host_name,
                                    PreviewsType type,
                                    base::Time now) {
  // Adds the new entry.
  const char kSqlInsert[] = "INSERT INTO " PREVIEWS_OPT_OUT_TABLE_NAME
                            " (host_name, time, opt_out, type)"
                            " VALUES "
                            " (?, ?, ?, ?)";

  sql::Statement statement_insert(
      db->GetCachedStatement(SQL_FROM_HERE, kSqlInsert));
  statement_insert.BindString(0, host_name);
  statement_insert.BindInt64(1, now.ToInternalValue());
  statement_insert.BindBool(2, opt_out);
  statement_insert.BindInt(3, static_cast<int>(type));
  statement_insert.Run();
}

// Removes OptOut entries for |host_name| if the per-host row limit is exceeded.
// Removes OptOut entries if per data base row limit is exceeded.
void MaybeEvictHostEntryFromDataBase(sql::Connection* db,
                                     const std::string& host_name) {
  // Delete the oldest entries if there are more than |MaxRowsPerHostInOptOutDB|
  // for |host_name|.
  // DELETE ... LIMIT -1 OFFSET x means delete all but the first x entries.
  const char kSqlDeleteByHost[] =
      "DELETE FROM " PREVIEWS_OPT_OUT_TABLE_NAME
      " WHERE ROWID IN"
      " (SELECT ROWID from " PREVIEWS_OPT_OUT_TABLE_NAME
      " WHERE host_name == ?"
      " ORDER BY time DESC"
      " LIMIT -1 OFFSET ?)";

  sql::Statement statement_delete_by_host(
      db->GetCachedStatement(SQL_FROM_HERE, kSqlDeleteByHost));
  statement_delete_by_host.BindString(0, host_name);
  statement_delete_by_host.BindInt(1, MaxRowsPerHostInOptOutDB());
  statement_delete_by_host.Run();
}

// Deletes every preview navigation/OptOut entry for |type|.
void ClearBlacklistForTypeInDataBase(sql::Connection* db, PreviewsType type) {
  const char kSql[] =
      "DELETE FROM " PREVIEWS_OPT_OUT_TABLE_NAME " WHERE type == ?";
  sql::Statement statement(db->GetUniqueStatement(kSql));
  statement.BindInt(0, static_cast<int>(type));
  statement.Run();
}

// Retrieves the list of previously enabled previews types with their version
// from the Enabled Previews table.
std::unique_ptr<std::map<PreviewsType, int>> GetStoredPreviews(
    sql::Connection* db) {
  const char kSqlLoadEnabledPreviewsVersions[] =
      "SELECT type, version FROM " ENABLED_PREVIEWS_TABLE_NAME;

  sql::Statement statement(
      db->GetUniqueStatement(kSqlLoadEnabledPreviewsVersions));

  std::unique_ptr<std::map<PreviewsType, int>> stored_previews(
      new std::map<PreviewsType, int>());
  while (statement.Step()) {
    PreviewsType type = static_cast<PreviewsType>(statement.ColumnInt(0));
    int version = statement.ColumnInt(1);
    stored_previews->insert({type, version});
  }
  return stored_previews;
}

// Adds a newly enabled |type| with its |version| to the Enabled Previews table.
void InsertEnabledPreviewInDataBase(sql::Connection* db,
                                    PreviewsType type,
                                    int version) {
  const char kSqlInsert[] = "INSERT INTO " ENABLED_PREVIEWS_TABLE_NAME
                            " (type, version)"
                            " VALUES "
                            " (?, ?)";

  sql::Statement statement_insert(db->GetUniqueStatement(kSqlInsert));
  statement_insert.BindInt(0, static_cast<int>(type));
  statement_insert.BindInt(1, version);
  statement_insert.Run();
}

// Updates the |version| of an enabled previews |type| in the Enabled Previews
// table.
void UpdateEnabledPreviewInDataBase(sql::Connection* db,
                                    PreviewsType type,
                                    int version) {
  const char kSqlUpdate[] = "UPDATE " ENABLED_PREVIEWS_TABLE_NAME
                            " SET version = ?"
                            " WHERE type = ?";

  sql::Statement statement_update(
      db->GetCachedStatement(SQL_FROM_HERE, kSqlUpdate));
  statement_update.BindInt(0, version);
  statement_update.BindInt(1, static_cast<int>(type));
  statement_update.Run();
}

// Deletes a previously enabled previews |type| from the Enabled Previews table.
void DeleteEnabledPreviewInDataBase(sql::Connection* db, PreviewsType type) {
  const char kSqlDelete[] =
      "DELETE FROM " ENABLED_PREVIEWS_TABLE_NAME " WHERE type == ?";

  sql::Statement statement_delete(db->GetUniqueStatement(kSqlDelete));
  statement_delete.BindInt(0, static_cast<int>(type));
  statement_delete.Run();
}

// Checks the current set of enabled previews (with their current version)
// and where a preview is now disabled or has a different version, cleans up
// any associated blacklist entries.
void CheckAndReconcileEnabledPreviewsWithDataBase(
    sql::Connection* db,
    PreviewsTypeList* enabled_previews) {
  std::unique_ptr<std::map<PreviewsType, int>> stored_previews(
      GetStoredPreviews(db));

  for (auto enabled_it = enabled_previews->begin();
       enabled_it != enabled_previews->end(); ++enabled_it) {
    PreviewsType type = enabled_it->first;
    int current_version = enabled_it->second;
    auto stored_it = stored_previews->find(type);
    if (stored_it == stored_previews->end()) {
      InsertEnabledPreviewInDataBase(db, type, current_version);
    } else {
      if (stored_it->second != current_version) {
        DCHECK_GE(current_version, stored_it->second);
        ClearBlacklistForTypeInDataBase(db, type);
        UpdateEnabledPreviewInDataBase(db, type, current_version);
      }
      // Erase entry from the local map to detect any newly disabled types.
      stored_previews->erase(stored_it);
    }
  }

  // Now check for any types that are no longer enabled.
  for (auto stored_it = stored_previews->begin();
       stored_it != stored_previews->end(); ++stored_it) {
    PreviewsType type = stored_it->first;
    ClearBlacklistForTypeInDataBase(db, type);
    DeleteEnabledPreviewInDataBase(db, type);
  }
}

void LoadBlackListFromDataBase(
    sql::Connection* db,
    PreviewsTypeList* enabled_previews,
    scoped_refptr<base::SingleThreadTaskRunner> runner,
    LoadBlackListCallback callback) {
  // First handle any update needed wrt enabled previews and their versions.
  CheckAndReconcileEnabledPreviewsWithDataBase(db, enabled_previews);

  // Gets the table sorted by host and time. Limits the number of hosts using
  // most recent opt_out time as the limiting function. Sorting is free due to
  // the table structure, and it improves performance in the loop below.
  const char kSql[] =
      "SELECT host_name, time, opt_out"
      " FROM " PREVIEWS_OPT_OUT_TABLE_NAME " ORDER BY host_name, time DESC";

  sql::Statement statement(db->GetUniqueStatement(kSql));

  std::unique_ptr<BlackListItemMap> black_list_item_map(new BlackListItemMap());
  std::unique_ptr<PreviewsBlackListItem> host_indifferent_black_list_item =
      PreviewsBlackList::CreateHostIndifferentBlackListItem();
  int count = 0;
  // Add the host name, the visit time, and opt out history to
  // |black_list_item_map|.
  while (statement.Step()) {
    ++count;
    std::string host_name = statement.ColumnString(0);
    PreviewsBlackListItem* black_list_item =
        PreviewsBlackList::GetOrCreateBlackListItemForMap(
            black_list_item_map.get(), host_name);
    DCHECK_LE(black_list_item_map->size(),
              params::MaxInMemoryHostsInBlackList());
    // Allows the internal logic of PreviewsBlackListItem to determine how to
    // evict entries when there are more than
    // |StoredHistoryLengthForBlackList()| for the host.
    black_list_item->AddPreviewNavigation(
        statement.ColumnBool(2),
        base::Time::FromInternalValue(statement.ColumnInt64(1)));
    // Allows the internal logic of PreviewsBlackListItem to determine what
    // items to evict.
    host_indifferent_black_list_item->AddPreviewNavigation(
        statement.ColumnBool(2),
        base::Time::FromInternalValue(statement.ColumnInt64(1)));
  }

  UMA_HISTOGRAM_COUNTS_10000("Previews.OptOut.DBRowCount", count);

  if (count > MaxRowsInOptOutDB()) {
    // Delete the oldest entries if there are more than |kMaxEntriesInDB|.
    // DELETE ... LIMIT -1 OFFSET x means delete all but the first x entries.
    const char kSqlDeleteByDBSize[] =
        "DELETE FROM " PREVIEWS_OPT_OUT_TABLE_NAME
        " WHERE ROWID IN"
        " (SELECT ROWID from " PREVIEWS_OPT_OUT_TABLE_NAME
        " ORDER BY time DESC"
        " LIMIT -1 OFFSET ?)";

    sql::Statement statement_delete(
        db->GetCachedStatement(SQL_FROM_HERE, kSqlDeleteByDBSize));
    statement_delete.BindInt(0, MaxRowsInOptOutDB());
    statement_delete.Run();
  }

  runner->PostTask(FROM_HERE,
                   base::BindOnce(callback, std::move(black_list_item_map),
                                  std::move(host_indifferent_black_list_item)));
}

// Synchronous implementations, these are run on the background thread
// and actually do the work to access the SQL data base.
void LoadBlackListSync(sql::Connection* db,
                       const base::FilePath& path,
                       std::unique_ptr<PreviewsTypeList> enabled_previews,
                       scoped_refptr<base::SingleThreadTaskRunner> runner,
                       LoadBlackListCallback callback) {
  if (!db->is_open())
    InitDatabase(db, path);

  LoadBlackListFromDataBase(db, enabled_previews.get(), runner, callback);
}

// Deletes every row in the table that has entry time between |begin_time| and
// |end_time|.
void ClearBlackListSync(sql::Connection* db,
                        base::Time begin_time,
                        base::Time end_time) {
  const char kSql[] = "DELETE FROM " PREVIEWS_OPT_OUT_TABLE_NAME
                      " WHERE time >= ? and time <= ?";

  sql::Statement statement(db->GetUniqueStatement(kSql));
  statement.BindInt64(0, begin_time.ToInternalValue());
  statement.BindInt64(1, end_time.ToInternalValue());
  statement.Run();
}

void AddPreviewNavigationSync(bool opt_out,
                              const std::string& host_name,
                              PreviewsType type,
                              base::Time now,
                              sql::Connection* db) {
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return;
  AddPreviewNavigationToDataBase(db, opt_out, host_name, type, now);
  MaybeEvictHostEntryFromDataBase(db, host_name);
  transaction.Commit();
}

}  // namespace

PreviewsOptOutStoreSQL::PreviewsOptOutStoreSQL(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::SequencedTaskRunner> background_task_runner,
    const base::FilePath& path,
    std::unique_ptr<PreviewsTypeList> enabled_previews)
    : io_task_runner_(io_task_runner),
      background_task_runner_(background_task_runner),
      db_file_path_(path),
      enabled_previews_(std::move(enabled_previews)) {
  DCHECK(enabled_previews_);
}

PreviewsOptOutStoreSQL::~PreviewsOptOutStoreSQL() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (db_) {
    background_task_runner_->DeleteSoon(FROM_HERE, db_.release());
  }
}

void PreviewsOptOutStoreSQL::AddPreviewNavigation(bool opt_out,
                                                  const std::string& host_name,
                                                  PreviewsType type,
                                                  base::Time now) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(db_);
  background_task_runner_->PostTask(
      FROM_HERE, base::Bind(&AddPreviewNavigationSync, opt_out, host_name, type,
                            now, db_.get()));
}

void PreviewsOptOutStoreSQL::ClearBlackList(base::Time begin_time,
                                            base::Time end_time) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(db_);
  background_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&ClearBlackListSync, db_.get(), begin_time, end_time));
}

void PreviewsOptOutStoreSQL::LoadBlackList(LoadBlackListCallback callback) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (!db_)
    db_ = std::make_unique<sql::Connection>();
  std::unique_ptr<PreviewsTypeList> enabled_previews =
      std::make_unique<PreviewsTypeList>(*enabled_previews_);
  background_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&LoadBlackListSync, db_.get(), db_file_path_,
                                std::move(enabled_previews),
                                base::ThreadTaskRunnerHandle::Get(), callback));
}

}  // namespace previews
