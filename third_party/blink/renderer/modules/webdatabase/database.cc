/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/webdatabase/database.h"

#include <memory>

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_database_observer.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/modules/webdatabase/change_version_data.h"
#include "third_party/blink/renderer/modules/webdatabase/change_version_wrapper.h"
#include "third_party/blink/renderer/modules/webdatabase/database_authorizer.h"
#include "third_party/blink/renderer/modules/webdatabase/database_context.h"
#include "third_party/blink/renderer/modules/webdatabase/database_manager.h"
#include "third_party/blink/renderer/modules/webdatabase/database_task.h"
#include "third_party/blink/renderer/modules/webdatabase/database_thread.h"
#include "third_party/blink/renderer/modules/webdatabase/database_tracker.h"
#include "third_party/blink/renderer/modules/webdatabase/sql_error.h"
#include "third_party/blink/renderer/modules/webdatabase/sql_transaction_backend.h"
#include "third_party/blink/renderer/modules/webdatabase/sql_transaction_client.h"
#include "third_party/blink/renderer/modules/webdatabase/sql_transaction_coordinator.h"
#include "third_party/blink/renderer/modules/webdatabase/sqlite/sqlite_statement.h"
#include "third_party/blink/renderer/modules/webdatabase/sqlite/sqlite_transaction.h"
#include "third_party/blink/renderer/modules/webdatabase/storage_log.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/heap/safe_point.h"
#include "third_party/blink/renderer/platform/waitable_event.h"
#include "third_party/blink/renderer/platform/web_task_runner.h"
#include "third_party/blink/renderer/platform/wtf/atomics.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

// Registering "opened" databases with the DatabaseTracker
// =======================================================
// The DatabaseTracker maintains a list of databases that have been
// "opened" so that the client can call interrupt or delete on every database
// associated with a DatabaseContext.
//
// We will only call DatabaseTracker::addOpenDatabase() to add the database
// to the tracker as opened when we've succeeded in opening the database,
// and will set m_opened to true. Similarly, we only call
// DatabaseTracker::removeOpenDatabase() to remove the database from the
// tracker when we set m_opened to false in closeDatabase(). This sets up
// a simple symmetry between open and close operations, and a direct
// correlation to adding and removing databases from the tracker's list,
// thus ensuring that we have a correct list for the interrupt and
// delete operations to work on.
//
// The only databases instances not tracked by the tracker's open database
// list are the ones that have not been added yet, or the ones that we
// attempted an open on but failed to. Such instances only exist in the
// DatabaseServer's factory methods for creating database backends.
//
// The factory methods will either call openAndVerifyVersion() or
// performOpenAndVerify(). These methods will add the newly instantiated
// database backend if they succeed in opening the requested database.
// In the case of failure to open the database, the factory methods will
// simply discard the newly instantiated database backend when they return.
// The ref counting mechanims will automatically destruct the un-added
// (and un-returned) databases instances.

namespace blink {

// Defines static local variable after making sure that guid lock is held.
// (We can't use DEFINE_STATIC_LOCAL for this because it asserts thread
// safety, which is externally guaranteed by the guideMutex lock)
#if DCHECK_IS_ON()
#define DEFINE_STATIC_LOCAL_WITH_LOCK(type, name, arguments) \
  DCHECK(GuidMutex().Locked());                              \
  static type& name = *new type arguments
#else
#define DEFINE_STATIC_LOCAL_WITH_LOCK(type, name, arguments) \
  static type& name = *new type arguments
#endif

static const char kVersionKey[] = "WebKitDatabaseVersionKey";
static const char kInfoTableName[] = "__WebKitDatabaseInfoTable__";

static String FormatErrorMessage(const char* message,
                                 int sqlite_error_code,
                                 const char* sqlite_error_message) {
  return String::Format("%s (%d %s)", message, sqlite_error_code,
                        sqlite_error_message);
}

static bool RetrieveTextResultFromDatabase(SQLiteDatabase& db,
                                           const String& query,
                                           String& result_string) {
  SQLiteStatement statement(db, query);
  int result = statement.Prepare();

  if (result != kSQLResultOk) {
    DLOG(ERROR) << "Error (" << result
                << ") preparing statement to read text result from database ("
                << query << ")";
    return false;
  }

  result = statement.Step();
  if (result == kSQLResultRow) {
    result_string = statement.GetColumnText(0);
    return true;
  }
  if (result == kSQLResultDone) {
    result_string = String();
    return true;
  }

  DLOG(ERROR) << "Error (" << result << ") reading text result from database ("
              << query << ")";
  return false;
}

static bool SetTextValueInDatabase(SQLiteDatabase& db,
                                   const String& query,
                                   const String& value) {
  SQLiteStatement statement(db, query);
  int result = statement.Prepare();

  if (result != kSQLResultOk) {
    DLOG(ERROR) << "Failed to prepare statement to set value in database ("
                << query << ")";
    return false;
  }

  statement.BindText(1, value);

  result = statement.Step();
  if (result != kSQLResultDone) {
    DLOG(ERROR) << "Failed to step statement to set value in database ("
                << query << ")";
    return false;
  }

  return true;
}

// FIXME: move all guid-related functions to a DatabaseVersionTracker class.
static RecursiveMutex& GuidMutex() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(RecursiveMutex, mutex, ());
  return mutex;
}

typedef HashMap<DatabaseGuid, String> GuidVersionMap;
static GuidVersionMap& GuidToVersionMap() {
  DEFINE_STATIC_LOCAL_WITH_LOCK(GuidVersionMap, map, ());
  return map;
}

// NOTE: Caller must lock guidMutex().
static inline void UpdateGuidVersionMap(DatabaseGuid guid, String new_version) {
  // Ensure the the mutex is locked.
#if DCHECK_IS_ON()
  DCHECK(GuidMutex().Locked());
#endif

  // Note: It is not safe to put an empty string into the guidToVersionMap()
  // map. That's because the map is cross-thread, but empty strings are
  // per-thread. The copy() function makes a version of the string you can
  // use on the current thread, but we need a string we can keep in a
  // cross-thread data structure.
  // FIXME: This is a quite-awkward restriction to have to program with.

  // Map null string to empty string (see comment above).
  GuidToVersionMap().Set(
      guid, new_version.IsEmpty() ? String() : new_version.IsolatedCopy());
}

static HashCountedSet<DatabaseGuid>& GuidCount() {
  DEFINE_STATIC_LOCAL_WITH_LOCK(HashCountedSet<DatabaseGuid>, guid_count, ());
  return guid_count;
}

static DatabaseGuid GuidForOriginAndName(const String& origin,
                                         const String& name) {
  // Ensure the the mutex is locked.
#if DCHECK_IS_ON()
  DCHECK(GuidMutex().Locked());
#endif

  String string_id = origin + "/" + name;

  typedef HashMap<String, int> IDGuidMap;
  DEFINE_STATIC_LOCAL_WITH_LOCK(IDGuidMap, string_identifier_to_guid_map, ());
  DatabaseGuid guid = string_identifier_to_guid_map.at(string_id);
  if (!guid) {
    static int current_new_guid = 1;
    guid = current_new_guid++;
    string_identifier_to_guid_map.Set(string_id, guid);
  }

  return guid;
}

Database::Database(DatabaseContext* database_context,
                   const String& name,
                   const String& expected_version,
                   const String& display_name,
                   unsigned estimated_size)
    : database_context_(database_context),
      name_(name.IsolatedCopy()),
      expected_version_(expected_version.IsolatedCopy()),
      display_name_(display_name.IsolatedCopy()),
      estimated_size_(estimated_size),
      guid_(0),
      opened_(0),
      new_(false),
      transaction_in_progress_(false),
      is_transaction_queue_enabled_(true) {
  DCHECK(IsMainThread());
  context_thread_security_origin_ =
      database_context_->GetSecurityOrigin()->IsolatedCopy();

  database_authorizer_ =
      DatabaseAuthorizer::Create(database_context, kInfoTableName);

  if (name_.IsNull())
    name_ = "";

  {
    RecursiveMutexLocker locker(GuidMutex());
    guid_ = GuidForOriginAndName(GetSecurityOrigin()->ToString(), name);
    GuidCount().insert(guid_);
  }

  filename_ = DatabaseManager::Manager().FullPathForDatabase(
      GetSecurityOrigin(), name_);

  database_thread_security_origin_ =
      context_thread_security_origin_->IsolatedCopy();
  DCHECK(database_context_->GetDatabaseThread());
  DCHECK(database_context_->IsContextThread());
  database_task_runner_ =
      GetExecutionContext()->GetTaskRunner(TaskType::kDatabaseAccess);
}

Database::~Database() {
  // SQLite is "multi-thread safe", but each database handle can only be used
  // on a single thread at a time.
  //
  // For Database, we open the SQLite database on the DatabaseThread, and
  // hence we should also close it on that same thread. This means that the
  // SQLite database need to be closed by another mechanism (see
  // DatabaseContext::stopDatabases()). By the time we get here, the SQLite
  // database should have already been closed.

  DCHECK(!opened_);
}

void Database::Trace(blink::Visitor* visitor) {
  visitor->Trace(database_context_);
  visitor->Trace(sqlite_database_);
  visitor->Trace(database_authorizer_);
  ScriptWrappable::Trace(visitor);
}

bool Database::OpenAndVerifyVersion(bool set_version_in_new_database,
                                    DatabaseError& error,
                                    String& error_message,
                                    V8DatabaseCallback* creation_callback) {
  WaitableEvent event;
  if (!GetDatabaseContext()->DatabaseThreadAvailable())
    return false;

  DatabaseTracker::Tracker().PrepareToOpenDatabase(this);
  bool success = false;
  std::unique_ptr<DatabaseOpenTask> task = DatabaseOpenTask::Create(
      this, set_version_in_new_database, &event, error, error_message, success);
  GetDatabaseContext()->GetDatabaseThread()->ScheduleTask(std::move(task));
  event.Wait();
  if (creation_callback) {
    if (success && IsNew()) {
      STORAGE_DVLOG(1)
          << "Scheduling DatabaseCreationCallbackTask for database " << this;
      auto* v8persistent_callback =
          ToV8PersistentCallbackFunction(creation_callback);
      probe::AsyncTaskScheduled(GetExecutionContext(), "openDatabase",
                                v8persistent_callback);
      GetExecutionContext()
          ->GetTaskRunner(TaskType::kDatabaseAccess)
          ->PostTask(
              FROM_HERE,
              WTF::Bind(&Database::RunCreationCallback, WrapPersistent(this),
                        WrapPersistent(v8persistent_callback)));
    }
  }

  return success;
}

void Database::RunCreationCallback(
    V8PersistentCallbackFunction<V8DatabaseCallback>* creation_callback) {
  probe::AsyncTask async_task(GetExecutionContext(), creation_callback);
  creation_callback->InvokeAndReportException(nullptr, this);
}

void Database::Close() {
  DCHECK(GetDatabaseContext()->GetDatabaseThread());
  DCHECK(GetDatabaseContext()->GetDatabaseThread()->IsDatabaseThread());

  {
    MutexLocker locker(transaction_in_progress_mutex_);

    // Clean up transactions that have not been scheduled yet:
    // Transaction phase 1 cleanup. See comment on "What happens if a
    // transaction is interrupted?" at the top of SQLTransactionBackend.cpp.
    SQLTransactionBackend* transaction = nullptr;
    while (!transaction_queue_.IsEmpty()) {
      transaction = transaction_queue_.TakeFirst();
      transaction->NotifyDatabaseThreadIsShuttingDown();
    }

    is_transaction_queue_enabled_ = false;
    transaction_in_progress_ = false;
  }

  CloseDatabase();
  GetDatabaseContext()->GetDatabaseThread()->RecordDatabaseClosed(this);
}

SQLTransactionBackend* Database::RunTransaction(SQLTransaction* transaction,
                                                bool read_only,
                                                const ChangeVersionData* data) {
  MutexLocker locker(transaction_in_progress_mutex_);
  if (!is_transaction_queue_enabled_)
    return nullptr;

  SQLTransactionWrapper* wrapper = nullptr;
  if (data)
    wrapper =
        ChangeVersionWrapper::Create(data->OldVersion(), data->NewVersion());

  SQLTransactionBackend* transaction_backend =
      SQLTransactionBackend::Create(this, transaction, wrapper, read_only);
  transaction_queue_.push_back(transaction_backend);
  if (!transaction_in_progress_)
    ScheduleTransaction();

  return transaction_backend;
}

void Database::InProgressTransactionCompleted() {
  MutexLocker locker(transaction_in_progress_mutex_);
  transaction_in_progress_ = false;
  ScheduleTransaction();
}

void Database::ScheduleTransaction() {
#if DCHECK_IS_ON()
  DCHECK(transaction_in_progress_mutex_.Locked());  // Locked by caller.
#endif                                              // DCHECK_IS_ON()
  SQLTransactionBackend* transaction = nullptr;

  if (is_transaction_queue_enabled_ && !transaction_queue_.IsEmpty())
    transaction = transaction_queue_.TakeFirst();

  if (transaction && GetDatabaseContext()->DatabaseThreadAvailable()) {
    std::unique_ptr<DatabaseTransactionTask> task =
        DatabaseTransactionTask::Create(transaction);
    STORAGE_DVLOG(1) << "Scheduling DatabaseTransactionTask " << task.get()
                     << " for transaction " << task->Transaction();
    transaction_in_progress_ = true;
    GetDatabaseContext()->GetDatabaseThread()->ScheduleTask(std::move(task));
  } else {
    transaction_in_progress_ = false;
  }
}

void Database::ScheduleTransactionStep(SQLTransactionBackend* transaction) {
  if (!GetDatabaseContext()->DatabaseThreadAvailable())
    return;

  std::unique_ptr<DatabaseTransactionTask> task =
      DatabaseTransactionTask::Create(transaction);
  STORAGE_DVLOG(1) << "Scheduling DatabaseTransactionTask " << task.get()
                   << " for the transaction step";
  GetDatabaseContext()->GetDatabaseThread()->ScheduleTask(std::move(task));
}

SQLTransactionClient* Database::TransactionClient() const {
  return GetDatabaseContext()->GetDatabaseThread()->TransactionClient();
}

SQLTransactionCoordinator* Database::TransactionCoordinator() const {
  return GetDatabaseContext()->GetDatabaseThread()->TransactionCoordinator();
}

// static
const char* Database::DatabaseInfoTableName() {
  return kInfoTableName;
}

void Database::CloseDatabase() {
  if (!opened_)
    return;

  ReleaseStore(&opened_, 0);
  sqlite_database_.Close();
  // See comment at the top this file regarding calling removeOpenDatabase().
  DatabaseTracker::Tracker().RemoveOpenDatabase(this);
  {
    RecursiveMutexLocker locker(GuidMutex());

    DCHECK(GuidCount().Contains(guid_));
    if (GuidCount().erase(guid_)) {
      GuidToVersionMap().erase(guid_);
    }
  }
}

String Database::version() const {
  // Note: In multi-process browsers the cached value may be accurate, but we
  // cannot read the actual version from the database without potentially
  // inducing a deadlock.
  // FIXME: Add an async version getter to the DatabaseAPI.
  return GetCachedVersion();
}

class DoneCreatingDatabaseOnExitCaller {
  STACK_ALLOCATED();

 public:
  DoneCreatingDatabaseOnExitCaller(Database* database)
      : database_(database), open_succeeded_(false) {}
  ~DoneCreatingDatabaseOnExitCaller() {
    if (!open_succeeded_)
      DatabaseTracker::Tracker().FailedToOpenDatabase(database_);
  }

  void SetOpenSucceeded() { open_succeeded_ = true; }

 private:
  CrossThreadPersistent<Database> database_;
  bool open_succeeded_;
};

bool Database::PerformOpenAndVerify(bool should_set_version_in_new_database,
                                    DatabaseError& error,
                                    String& error_message) {
  double call_start_time = WTF::CurrentTimeTicksInSeconds();
  DoneCreatingDatabaseOnExitCaller on_exit_caller(this);
  DCHECK(error_message.IsEmpty());
  DCHECK_EQ(error,
            DatabaseError::kNone);  // Better not have any errors already.
  // Presumed failure. We'll clear it if we succeed below.
  error = DatabaseError::kInvalidDatabaseState;

  const int kMaxSqliteBusyWaitTime = 30000;

  if (!sqlite_database_.Open(filename_)) {
    ReportOpenDatabaseResult(
        1, kInvalidStateError, sqlite_database_.LastError(),
        WTF::CurrentTimeTicksInSeconds() - call_start_time);
    error_message = FormatErrorMessage("unable to open database",
                                       sqlite_database_.LastError(),
                                       sqlite_database_.LastErrorMsg());
    return false;
  }
  if (!sqlite_database_.TurnOnIncrementalAutoVacuum())
    DLOG(ERROR) << "Unable to turn on incremental auto-vacuum ("
                << sqlite_database_.LastError() << " "
                << sqlite_database_.LastErrorMsg() << ")";

  sqlite_database_.SetBusyTimeout(kMaxSqliteBusyWaitTime);

  String current_version;
  {
    RecursiveMutexLocker locker(GuidMutex());

    GuidVersionMap::iterator entry = GuidToVersionMap().find(guid_);
    if (entry != GuidToVersionMap().end()) {
      // Map null string to empty string (see updateGuidVersionMap()).
      current_version =
          entry->value.IsNull() ? g_empty_string : entry->value.IsolatedCopy();
      STORAGE_DVLOG(1) << "Current cached version for guid " << guid_ << " is "
                       << current_version;

      // Note: In multi-process browsers the cached value may be
      // inaccurate, but we cannot read the actual version from the
      // database without potentially inducing a form of deadlock, a
      // busytimeout error when trying to access the database. So we'll
      // use the cached value if we're unable to read the value from the
      // database file without waiting.
      // FIXME: Add an async openDatabase method to the DatabaseAPI.
      const int kNoSqliteBusyWaitTime = 0;
      sqlite_database_.SetBusyTimeout(kNoSqliteBusyWaitTime);
      String version_from_database;
      if (GetVersionFromDatabase(version_from_database, false)) {
        current_version = version_from_database;
        UpdateGuidVersionMap(guid_, current_version);
      }
      sqlite_database_.SetBusyTimeout(kMaxSqliteBusyWaitTime);
    } else {
      STORAGE_DVLOG(1) << "No cached version for guid " << guid_;

      SQLiteTransaction transaction(sqlite_database_);
      transaction.begin();
      if (!transaction.InProgress()) {
        ReportOpenDatabaseResult(
            2, kInvalidStateError, sqlite_database_.LastError(),
            WTF::CurrentTimeTicksInSeconds() - call_start_time);
        error_message = FormatErrorMessage(
            "unable to open database, failed to start transaction",
            sqlite_database_.LastError(), sqlite_database_.LastErrorMsg());
        sqlite_database_.Close();
        return false;
      }

      String table_name(kInfoTableName);
      if (!sqlite_database_.TableExists(table_name)) {
        new_ = true;

        if (!sqlite_database_.ExecuteCommand(
                "CREATE TABLE " + table_name +
                " (key TEXT NOT NULL ON CONFLICT FAIL UNIQUE ON CONFLICT "
                "REPLACE,value TEXT NOT NULL ON CONFLICT FAIL);")) {
          ReportOpenDatabaseResult(
              3, kInvalidStateError, sqlite_database_.LastError(),
              WTF::CurrentTimeTicksInSeconds() - call_start_time);
          error_message = FormatErrorMessage(
              "unable to open database, failed to create 'info' table",
              sqlite_database_.LastError(), sqlite_database_.LastErrorMsg());
          transaction.Rollback();
          sqlite_database_.Close();
          return false;
        }
      } else if (!GetVersionFromDatabase(current_version, false)) {
        ReportOpenDatabaseResult(
            4, kInvalidStateError, sqlite_database_.LastError(),
            WTF::CurrentTimeTicksInSeconds() - call_start_time);
        error_message = FormatErrorMessage(
            "unable to open database, failed to read current version",
            sqlite_database_.LastError(), sqlite_database_.LastErrorMsg());
        transaction.Rollback();
        sqlite_database_.Close();
        return false;
      }

      if (current_version.length()) {
        STORAGE_DVLOG(1) << "Retrieved current version " << current_version
                         << " from database " << DatabaseDebugName();
      } else if (!new_ || should_set_version_in_new_database) {
        STORAGE_DVLOG(1) << "Setting version " << expected_version_
                         << " in database " << DatabaseDebugName()
                         << " that was just created";
        if (!SetVersionInDatabase(expected_version_, false)) {
          ReportOpenDatabaseResult(
              5, kInvalidStateError, sqlite_database_.LastError(),
              WTF::CurrentTimeTicksInSeconds() - call_start_time);
          error_message = FormatErrorMessage(
              "unable to open database, failed to write current version",
              sqlite_database_.LastError(), sqlite_database_.LastErrorMsg());
          transaction.Rollback();
          sqlite_database_.Close();
          return false;
        }
        current_version = expected_version_;
      }
      UpdateGuidVersionMap(guid_, current_version);
      transaction.Commit();
    }
  }

  if (current_version.IsNull()) {
    STORAGE_DVLOG(1) << "Database " << DatabaseDebugName()
                     << " does not have its version set";
    current_version = "";
  }

  // If the expected version isn't the empty string, ensure that the current
  // database version we have matches that version. Otherwise, set an
  // exception.
  // If the expected version is the empty string, then we always return with
  // whatever version of the database we have.
  if ((!new_ || should_set_version_in_new_database) &&
      expected_version_.length() && expected_version_ != current_version) {
    ReportOpenDatabaseResult(
        6, kInvalidStateError, 0,
        WTF::CurrentTimeTicksInSeconds() - call_start_time);
    error_message =
        "unable to open database, version mismatch, '" + expected_version_ +
        "' does not match the currentVersion of '" + current_version + "'";
    sqlite_database_.Close();
    return false;
  }

  DCHECK(database_authorizer_);
  sqlite_database_.SetAuthorizer(database_authorizer_.Get());

  // See comment at the top this file regarding calling addOpenDatabase().
  DatabaseTracker::Tracker().AddOpenDatabase(this);
  opened_ = 1;

  // Declare success:
  error = DatabaseError::kNone;  // Clear the presumed error from above.
  on_exit_caller.SetOpenSucceeded();

  if (new_ && !should_set_version_in_new_database) {
    // The caller provided a creationCallback which will set the expected
    // version.
    expected_version_ = "";
  }

  ReportOpenDatabaseResult(
      0, -1, 0, WTF::CurrentTimeTicksInSeconds() - call_start_time);  // OK

  if (GetDatabaseContext()->GetDatabaseThread())
    GetDatabaseContext()->GetDatabaseThread()->RecordDatabaseOpen(this);
  return true;
}

String Database::StringIdentifier() const {
  // Return a deep copy for ref counting thread safety
  return name_.IsolatedCopy();
}

String Database::DisplayName() const {
  // Return a deep copy for ref counting thread safety
  return display_name_.IsolatedCopy();
}

unsigned Database::EstimatedSize() const {
  return estimated_size_;
}

String Database::FileName() const {
  // Return a deep copy for ref counting thread safety
  return filename_.IsolatedCopy();
}

bool Database::GetVersionFromDatabase(String& version,
                                      bool should_cache_version) {
  String query(String("SELECT value FROM ") + kInfoTableName +
               " WHERE key = '" + kVersionKey + "';");

  database_authorizer_->Disable();

  bool result =
      RetrieveTextResultFromDatabase(sqlite_database_, query, version);
  if (result) {
    if (should_cache_version)
      SetCachedVersion(version);
  } else {
    DLOG(ERROR) << "Failed to retrieve version from database "
                << DatabaseDebugName();
  }

  database_authorizer_->Enable();

  return result;
}

bool Database::SetVersionInDatabase(const String& version,
                                    bool should_cache_version) {
  // The INSERT will replace an existing entry for the database with the new
  // version number, due to the UNIQUE ON CONFLICT REPLACE clause in the
  // CREATE statement (see Database::performOpenAndVerify()).
  String query(String("INSERT INTO ") + kInfoTableName +
               " (key, value) VALUES ('" + kVersionKey + "', ?);");

  database_authorizer_->Disable();

  bool result = SetTextValueInDatabase(sqlite_database_, query, version);
  if (result) {
    if (should_cache_version)
      SetCachedVersion(version);
  } else {
    DLOG(ERROR) << "Failed to set version " << version << " in database ("
                << query << ")";
  }

  database_authorizer_->Enable();

  return result;
}

void Database::SetExpectedVersion(const String& version) {
  expected_version_ = version.IsolatedCopy();
}

String Database::GetCachedVersion() const {
  RecursiveMutexLocker locker(GuidMutex());
  return GuidToVersionMap().at(guid_).IsolatedCopy();
}

void Database::SetCachedVersion(const String& actual_version) {
  // Update the in memory database version map.
  RecursiveMutexLocker locker(GuidMutex());
  UpdateGuidVersionMap(guid_, actual_version);
}

bool Database::GetActualVersionForTransaction(String& actual_version) {
  DCHECK(sqlite_database_.TransactionInProgress());
  // Note: In multi-process browsers the cached value may be inaccurate. So we
  // retrieve the value from the database and update the cached value here.
  return GetVersionFromDatabase(actual_version, true);
}

void Database::DisableAuthorizer() {
  DCHECK(database_authorizer_);
  database_authorizer_->Disable();
}

void Database::EnableAuthorizer() {
  DCHECK(database_authorizer_);
  database_authorizer_->Enable();
}

void Database::SetAuthorizerPermissions(int permissions) {
  DCHECK(database_authorizer_);
  database_authorizer_->SetPermissions(permissions);
}

bool Database::LastActionChangedDatabase() {
  DCHECK(database_authorizer_);
  return database_authorizer_->LastActionChangedDatabase();
}

bool Database::LastActionWasInsert() {
  DCHECK(database_authorizer_);
  return database_authorizer_->LastActionWasInsert();
}

void Database::ResetDeletes() {
  DCHECK(database_authorizer_);
  database_authorizer_->ResetDeletes();
}

bool Database::HadDeletes() {
  DCHECK(database_authorizer_);
  return database_authorizer_->HadDeletes();
}

void Database::ResetAuthorizer() {
  if (database_authorizer_)
    database_authorizer_->Reset();
}

unsigned long long Database::MaximumSize() const {
  return DatabaseTracker::Tracker().GetMaxSizeForDatabase(this);
}

void Database::IncrementalVacuumIfNeeded() {
  int64_t free_space_size = sqlite_database_.FreeSpaceSize();
  int64_t total_size = sqlite_database_.TotalSize();
  if (total_size <= 10 * free_space_size) {
    int result = sqlite_database_.RunIncrementalVacuumCommand();
    ReportVacuumDatabaseResult(result);
    if (result != kSQLResultOk)
      LogErrorMessage(FormatErrorMessage("error vacuuming database", result,
                                         sqlite_database_.LastErrorMsg()));
  }
}

// These are used to generate histograms of errors seen with websql.
// See about:histograms in chromium.
void Database::ReportOpenDatabaseResult(int error_site,
                                        int web_sql_error_code,
                                        int sqlite_error_code,
                                        double duration) {
  if (Platform::Current()->DatabaseObserver()) {
    Platform::Current()->DatabaseObserver()->ReportOpenDatabaseResult(
        WebSecurityOrigin(GetSecurityOrigin()), StringIdentifier(), error_site,
        web_sql_error_code, sqlite_error_code, duration);
  }
}

void Database::ReportChangeVersionResult(int error_site,
                                         int web_sql_error_code,
                                         int sqlite_error_code) {
  if (Platform::Current()->DatabaseObserver()) {
    Platform::Current()->DatabaseObserver()->ReportChangeVersionResult(
        WebSecurityOrigin(GetSecurityOrigin()), StringIdentifier(), error_site,
        web_sql_error_code, sqlite_error_code);
  }
}

void Database::ReportStartTransactionResult(int error_site,
                                            int web_sql_error_code,
                                            int sqlite_error_code) {
  if (Platform::Current()->DatabaseObserver()) {
    Platform::Current()->DatabaseObserver()->ReportStartTransactionResult(
        WebSecurityOrigin(GetSecurityOrigin()), StringIdentifier(), error_site,
        web_sql_error_code, sqlite_error_code);
  }
}

void Database::ReportCommitTransactionResult(int error_site,
                                             int web_sql_error_code,
                                             int sqlite_error_code) {
  if (Platform::Current()->DatabaseObserver()) {
    Platform::Current()->DatabaseObserver()->ReportCommitTransactionResult(
        WebSecurityOrigin(GetSecurityOrigin()), StringIdentifier(), error_site,
        web_sql_error_code, sqlite_error_code);
  }
}

void Database::ReportExecuteStatementResult(int error_site,
                                            int web_sql_error_code,
                                            int sqlite_error_code) {
  if (Platform::Current()->DatabaseObserver()) {
    Platform::Current()->DatabaseObserver()->ReportExecuteStatementResult(
        WebSecurityOrigin(GetSecurityOrigin()), StringIdentifier(), error_site,
        web_sql_error_code, sqlite_error_code);
  }
}

void Database::ReportVacuumDatabaseResult(int sqlite_error_code) {
  if (Platform::Current()->DatabaseObserver()) {
    Platform::Current()->DatabaseObserver()->ReportVacuumDatabaseResult(
        WebSecurityOrigin(GetSecurityOrigin()), StringIdentifier(),
        sqlite_error_code);
  }
}

void Database::LogErrorMessage(const String& message) {
  GetExecutionContext()->AddConsoleMessage(ConsoleMessage::Create(
      kStorageMessageSource, kErrorMessageLevel, message));
}

ExecutionContext* Database::GetExecutionContext() const {
  return GetDatabaseContext()->GetExecutionContext();
}

void Database::CloseImmediately() {
  DCHECK(GetExecutionContext()->IsContextThread());
  if (GetDatabaseContext()->DatabaseThreadAvailable() && Opened()) {
    LogErrorMessage("forcibly closing database");
    GetDatabaseContext()->GetDatabaseThread()->ScheduleTask(
        DatabaseCloseTask::Create(this, nullptr));
  }
}

void Database::changeVersion(const String& old_version,
                             const String& new_version,
                             V8SQLTransactionCallback* callback,
                             V8SQLTransactionErrorCallback* error_callback,
                             V8VoidCallback* success_callback) {
  ChangeVersionData data(old_version, new_version);
  RunTransaction(SQLTransaction::OnProcessV8Impl::Create(callback),
                 SQLTransaction::OnErrorV8Impl::Create(error_callback),
                 SQLTransaction::OnSuccessV8Impl::Create(success_callback),
                 false, &data);
}

void Database::transaction(V8SQLTransactionCallback* callback,
                           V8SQLTransactionErrorCallback* error_callback,
                           V8VoidCallback* success_callback) {
  RunTransaction(SQLTransaction::OnProcessV8Impl::Create(callback),
                 SQLTransaction::OnErrorV8Impl::Create(error_callback),
                 SQLTransaction::OnSuccessV8Impl::Create(success_callback),
                 false);
}

void Database::readTransaction(V8SQLTransactionCallback* callback,
                               V8SQLTransactionErrorCallback* error_callback,
                               V8VoidCallback* success_callback) {
  RunTransaction(SQLTransaction::OnProcessV8Impl::Create(callback),
                 SQLTransaction::OnErrorV8Impl::Create(error_callback),
                 SQLTransaction::OnSuccessV8Impl::Create(success_callback),
                 true);
}

void Database::PerformTransaction(
    SQLTransaction::OnProcessCallback* callback,
    SQLTransaction::OnErrorCallback* error_callback,
    SQLTransaction::OnSuccessCallback* success_callback) {
  RunTransaction(callback, error_callback, success_callback, false);
}

static void CallTransactionErrorCallback(
    SQLTransaction::OnErrorCallback* callback,
    std::unique_ptr<SQLErrorData> error_data) {
  callback->OnError(SQLError::Create(*error_data));
}

void Database::RunTransaction(
    SQLTransaction::OnProcessCallback* callback,
    SQLTransaction::OnErrorCallback* error_callback,
    SQLTransaction::OnSuccessCallback* success_callback,
    bool read_only,
    const ChangeVersionData* change_version_data) {
  if (!GetExecutionContext())
    return;

  DCHECK(GetExecutionContext()->IsContextThread());
// FIXME: Rather than passing errorCallback to SQLTransaction and then
// sometimes firing it ourselves, this code should probably be pushed down
// into Database so that we only create the SQLTransaction if we're
// actually going to run it.
#if DCHECK_IS_ON()
  SQLTransaction::OnErrorCallback* original_error_callback = error_callback;
#endif
  SQLTransaction* transaction = SQLTransaction::Create(
      this, callback, success_callback, error_callback, read_only);
  SQLTransactionBackend* transaction_backend =
      RunTransaction(transaction, read_only, change_version_data);
  if (!transaction_backend) {
    SQLTransaction::OnErrorCallback* callback =
        transaction->ReleaseErrorCallback();
#if DCHECK_IS_ON()
    DCHECK_EQ(callback, original_error_callback);
#endif
    if (callback) {
      std::unique_ptr<SQLErrorData> error = SQLErrorData::Create(
          SQLError::kUnknownErr, "database has been closed");
      GetDatabaseTaskRunner()->PostTask(
          FROM_HERE,
          WTF::Bind(&CallTransactionErrorCallback, WrapPersistent(callback),
                    WTF::Passed(std::move(error))));
    }
  }
}

void Database::ScheduleTransactionCallback(SQLTransaction* transaction) {
  // The task is constructed in a database thread, and destructed in the
  // context thread.
  PostCrossThreadTask(*GetDatabaseTaskRunner(), FROM_HERE,
                      CrossThreadBind(&SQLTransaction::PerformPendingCallback,
                                      WrapCrossThreadPersistent(transaction)));
}

Vector<String> Database::PerformGetTableNames() {
  DisableAuthorizer();

  SQLiteStatement statement(
      SqliteDatabase(), "SELECT name FROM sqlite_master WHERE type='table';");
  if (statement.Prepare() != kSQLResultOk) {
    DLOG(ERROR) << "Unable to retrieve list of tables for database "
                << DatabaseDebugName();
    EnableAuthorizer();
    return Vector<String>();
  }

  Vector<String> table_names;
  int result;
  while ((result = statement.Step()) == kSQLResultRow) {
    String name = statement.GetColumnText(0);
    if (name != DatabaseInfoTableName())
      table_names.push_back(name);
  }

  EnableAuthorizer();

  if (result != kSQLResultDone) {
    DLOG(ERROR) << "Error getting tables for database " << DatabaseDebugName();
    return Vector<String>();
  }

  return table_names;
}

Vector<String> Database::TableNames() {
  // FIXME: Not using isolatedCopy on these strings looks ok since threads
  // take strict turns in dealing with them. However, if the code changes,
  // this may not be true anymore.
  Vector<String> result;
  WaitableEvent event;
  if (!GetDatabaseContext()->DatabaseThreadAvailable())
    return result;

  std::unique_ptr<DatabaseTableNamesTask> task =
      DatabaseTableNamesTask::Create(this, &event, result);
  GetDatabaseContext()->GetDatabaseThread()->ScheduleTask(std::move(task));
  event.Wait();

  return result;
}

const SecurityOrigin* Database::GetSecurityOrigin() const {
  if (!GetExecutionContext())
    return nullptr;
  if (GetExecutionContext()->IsContextThread())
    return context_thread_security_origin_.get();
  if (GetDatabaseContext()->GetDatabaseThread()->IsDatabaseThread())
    return database_thread_security_origin_.get();
  return nullptr;
}

bool Database::Opened() {
  return static_cast<bool>(AcquireLoad(&opened_));
}

base::SingleThreadTaskRunner* Database::GetDatabaseTaskRunner() const {
  return database_task_runner_.get();
}

}  // namespace blink
