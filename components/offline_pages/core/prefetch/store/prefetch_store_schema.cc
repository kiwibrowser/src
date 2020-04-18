// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store_schema.h"

#include "sql/connection.h"
#include "sql/meta_table.h"
#include "sql/transaction.h"

namespace offline_pages {

// Schema versions changelog:
// * 1: Initial version with prefetch_items and prefetch_downloader_quota
//   tables.
// * 2: Changes prefetch_items.file_size to have a default value of -1 (instead
//   of 0).

// static
const int PrefetchStoreSchema::kCurrentVersion = 2;
// static
const int PrefetchStoreSchema::kCompatibleVersion = 1;

namespace {

// TODO(https://crbug.com/765282): remove MetaTable internal values and helper
// methods once its getters and setters for version information allow the caller
// to be informed about internal errors.

// From MetaTable internals.
const char kVersionKey[] = "version";
const char kCompatibleVersionKey[] = "last_compatible_version";

const int kVersionError = -1;

bool SetVersionNumber(sql::MetaTable* meta_table, int version) {
  return meta_table->SetValue(kVersionKey, version);
}

bool SetCompatibleVersionNumber(sql::MetaTable* meta_table, int version) {
  return meta_table->SetValue(kCompatibleVersionKey, version);
}

int GetVersionNumber(sql::MetaTable* meta_table) {
  int version;
  if (meta_table->GetValue(kVersionKey, &version))
    return version;
  return kVersionError;
}

int GetCompatibleVersionNumber(sql::MetaTable* meta_table) {
  int version;
  if (meta_table->GetValue(kCompatibleVersionKey, &version))
    return version;
  return kVersionError;
}

// IMPORTANT #1: when making changes to these columns please also reflect them
// into:
// - PrefetchItem: update existing fields and all method implementations
//   (operator=, operator<<, ToString, etc).
// - PrefetchItemTest, PrefetchStoreTestUtil: update test related code to cover
//   the changed set of columns and PrefetchItem members.
// - MockPrefetchItemGenerator: so that its generated items consider all fields.
// IMPORTANT #2: the ordering of column types is important in SQLite 3 tables to
// simplify data retrieval. Columns with fixed length types must come first and
// variable length types must come later.
static const char kItemsTableCreationSql[] =
    "CREATE TABLE IF NOT EXISTS prefetch_items "
    // Fixed length columns come first.
    "(offline_id INTEGER PRIMARY KEY NOT NULL,"
    " state INTEGER NOT NULL DEFAULT 0,"
    " generate_bundle_attempts INTEGER NOT NULL DEFAULT 0,"
    " get_operation_attempts INTEGER NOT NULL DEFAULT 0,"
    " download_initiation_attempts INTEGER NOT NULL DEFAULT 0,"
    " archive_body_length INTEGER_NOT_NULL DEFAULT -1,"
    " creation_time INTEGER NOT NULL,"
    " freshness_time INTEGER NOT NULL,"
    " error_code INTEGER NOT NULL DEFAULT 0,"
    " file_size INTEGER NOT NULL DEFAULT -1,"
    // Variable length columns come later.
    " guid VARCHAR NOT NULL DEFAULT '',"
    " client_namespace VARCHAR NOT NULL DEFAULT '',"
    " client_id VARCHAR NOT NULL DEFAULT '',"
    " requested_url VARCHAR NOT NULL DEFAULT '',"
    " final_archived_url VARCHAR NOT NULL DEFAULT '',"
    " operation_name VARCHAR NOT NULL DEFAULT '',"
    " archive_body_name VARCHAR NOT NULL DEFAULT '',"
    " title VARCHAR NOT NULL DEFAULT '',"
    " file_path VARCHAR NOT NULL DEFAULT ''"
    ")";

bool CreatePrefetchItemsTable(sql::Connection* db) {
  return db->Execute(kItemsTableCreationSql);
}

static const char kQuotaTableCreationSql[] =
    "CREATE TABLE IF NOT EXISTS prefetch_downloader_quota "
    "(quota_id INTEGER PRIMARY KEY NOT NULL DEFAULT 1,"
    " update_time INTEGER NOT NULL,"
    " available_quota INTEGER NOT NULL DEFAULT 0)";

bool CreatePrefetchQuotaTable(sql::Connection* db) {
  return db->Execute(kQuotaTableCreationSql);
}

bool CreateLatestSchema(sql::Connection* db) {
  sql::Transaction transaction(db);
  if (!transaction.Begin())
    return false;

  if (!CreatePrefetchItemsTable(db) || !CreatePrefetchQuotaTable(db))
    return false;

  // This would be a great place to add indices when we need them.
  return transaction.Commit();
}

int MigrateFromVersion1To2(sql::Connection* db, sql::MetaTable* meta_table) {
  const int target_version = 2;
  const int target_compatible_version = 1;
  const char kVersion1ToVersion2MigrationSql[] =
      // Rename the existing items table.
      "ALTER TABLE prefetch_items RENAME TO prefetch_items_old; "
      // Creates the new items table.
      "CREATE TABLE prefetch_items "
      "(offline_id INTEGER PRIMARY KEY NOT NULL,"
      " state INTEGER NOT NULL DEFAULT 0,"
      " generate_bundle_attempts INTEGER NOT NULL DEFAULT 0,"
      " get_operation_attempts INTEGER NOT NULL DEFAULT 0,"
      " download_initiation_attempts INTEGER NOT NULL DEFAULT 0,"
      " archive_body_length INTEGER_NOT_NULL DEFAULT -1,"
      " creation_time INTEGER NOT NULL,"
      " freshness_time INTEGER NOT NULL,"
      " error_code INTEGER NOT NULL DEFAULT 0,"
      // Note: default value changed from 0 to -1.
      " file_size INTEGER NOT NULL DEFAULT -1,"
      " guid VARCHAR NOT NULL DEFAULT '',"
      " client_namespace VARCHAR NOT NULL DEFAULT '',"
      " client_id VARCHAR NOT NULL DEFAULT '',"
      " requested_url VARCHAR NOT NULL DEFAULT '',"
      " final_archived_url VARCHAR NOT NULL DEFAULT '',"
      " operation_name VARCHAR NOT NULL DEFAULT '',"
      " archive_body_name VARCHAR NOT NULL DEFAULT '',"
      " title VARCHAR NOT NULL DEFAULT '',"
      " file_path VARCHAR NOT NULL DEFAULT ''); "
      // Copy existing rows to the new items table.
      "INSERT INTO prefetch_items "
      " (offline_id, state, generate_bundle_attempts, get_operation_attempts,"
      "  download_initiation_attempts, archive_body_length, creation_time,"
      "  freshness_time, error_code, file_size, guid, client_namespace,"
      "  client_id, requested_url, final_archived_url, operation_name,"
      "  archive_body_name, title, file_path)"
      " SELECT "
      "  offline_id, state, generate_bundle_attempts, get_operation_attempts,"
      "  download_initiation_attempts, archive_body_length, creation_time,"
      "  freshness_time, error_code, file_size, guid, client_namespace,"
      "  client_id, requested_url, final_archived_url, operation_name,"
      "  archive_body_name, title, file_path"
      " FROM prefetch_items_old; "
      // Drops the old items table.
      "DROP TABLE prefetch_items_old; ";

  sql::Transaction transaction(db);
  if (transaction.Begin() && db->Execute(kVersion1ToVersion2MigrationSql) &&
      SetVersionNumber(meta_table, target_version) &&
      SetCompatibleVersionNumber(meta_table, target_compatible_version) &&
      transaction.Commit()) {
    return target_version;
  }

  return kVersionError;
}

}  // namespace

// static
bool PrefetchStoreSchema::CreateOrUpgradeIfNeeded(sql::Connection* db) {
  DCHECK_GE(kCurrentVersion, kCompatibleVersion);
  DCHECK(db);
  if (!db)
    return false;

  sql::MetaTable meta_table;
  if (!meta_table.Init(db, kCurrentVersion, kCompatibleVersion))
    return false;

  const int compatible_version = GetCompatibleVersionNumber(&meta_table);
  int current_version = GetVersionNumber(&meta_table);
  if (current_version == kVersionError || compatible_version == kVersionError)
    return false;
  DCHECK_GE(current_version, compatible_version);

  // Stored database version is newer and incompatible with the current running
  // code (Chrome was downgraded). The DB will never work until Chrome is
  // re-upgraded.
  if (compatible_version > kCurrentVersion)
    return false;

  // Database is already at the latest version or has just been created. Create
  // any missing tables and return.
  if (current_version == kCurrentVersion)
    return CreateLatestSchema(db);

  // Versions 0 and below are unexpected.
  if (current_version <= 0)
    return false;

  // Schema upgrade code starts here.
  //
  // Note #1: A series of if-else blocks was chosen to allow for more
  // flexibility in the upgrade logic than a single switch-case block would.
  // Note #2: Be very mindful when referring any constants from inside upgrade
  // code as they might change as the schema evolve whereas the upgrade code
  // should not. For instance, one should never refer to kCurrentVersion or
  // kCompatibleVersion when setting values for the current and compatible
  // versions as these are definitely going to change with each schema change.
  if (current_version == 1) {
    current_version = MigrateFromVersion1To2(db, &meta_table);
  }

  return current_version == kCurrentVersion;
}

// static
std::string PrefetchStoreSchema::GetItemTableCreationSqlForTesting() {
  return kItemsTableCreationSql;
}

}  // namespace offline_pages
