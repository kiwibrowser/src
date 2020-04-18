// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store_schema.h"

#include <limits>
#include <memory>

#include "sql/connection.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

static const char kSomeTableCreationSql[] =
    "CREATE TABLE some_table "
    "(id INTEGER PRIMARY KEY NOT NULL,"
    " value INTEGER NOT NULL)";

static const char kAnotherTableCreationSql[] =
    "CREATE TABLE another_table "
    "(id INTEGER PRIMARY KEY NOT NULL,"
    " name VARCHAR NOT NULL)";

TEST(PrefetchStoreSchemaPreconditionTest,
     TestSqliteCreateTableIsTransactional) {
  sql::Connection db;
  ASSERT_TRUE(db.OpenInMemory());

  sql::Transaction transaction(&db);
  ASSERT_TRUE(transaction.Begin());
  EXPECT_TRUE(db.Execute(kSomeTableCreationSql));
  EXPECT_TRUE(db.Execute(kAnotherTableCreationSql));
  transaction.Rollback();

  EXPECT_FALSE(db.DoesTableExist("some_table"));
  EXPECT_FALSE(db.DoesTableExist("another_table"));
}

TEST(PrefetchStoreSchemaPreconditionTest, TestSqliteDropTableIsTransactional) {
  sql::Connection db;
  ASSERT_TRUE(db.OpenInMemory());
  EXPECT_TRUE(db.Execute(kSomeTableCreationSql));
  EXPECT_TRUE(db.Execute(kAnotherTableCreationSql));

  sql::Transaction transaction(&db);
  ASSERT_TRUE(transaction.Begin());
  EXPECT_TRUE(db.Execute("DROP TABLE some_table"));
  EXPECT_TRUE(db.Execute("DROP TABLE another_table"));
  transaction.Rollback();

  EXPECT_TRUE(db.DoesTableExist("some_table"));
  EXPECT_TRUE(db.DoesTableExist("another_table"));
}

TEST(PrefetchStoreSchemaPreconditionTest, TestSqliteAlterTableIsTransactional) {
  sql::Connection db;
  ASSERT_TRUE(db.OpenInMemory());
  EXPECT_TRUE(db.Execute(kSomeTableCreationSql));

  sql::Transaction transaction(&db);
  ASSERT_TRUE(transaction.Begin());
  EXPECT_TRUE(db.Execute("ALTER TABLE some_table ADD new_column VARCHAR NULL"));
  EXPECT_TRUE(db.Execute("ALTER TABLE some_table RENAME TO another_table"));
  transaction.Rollback();

  EXPECT_TRUE(db.DoesTableExist("some_table"));
  EXPECT_FALSE(db.DoesColumnExist("some_table", "new_column"));
  EXPECT_FALSE(db.DoesTableExist("another_table"));
}

TEST(PrefetchStoreSchemaPreconditionTest,
     TestCommonMigrationCodeIsTransactional) {
  sql::Connection db;
  ASSERT_TRUE(db.OpenInMemory());
  EXPECT_TRUE(db.Execute(kSomeTableCreationSql));

  sql::Transaction transaction(&db);
  ASSERT_TRUE(transaction.Begin());
  EXPECT_TRUE(db.Execute("ALTER TABLE some_table RENAME TO another_table"));
  EXPECT_TRUE(db.Execute(kSomeTableCreationSql));
  EXPECT_TRUE(db.Execute("DROP TABLE another_table"));
  transaction.Rollback();

  EXPECT_TRUE(db.DoesTableExist("some_table"));
  EXPECT_FALSE(db.DoesTableExist("another_table"));
  EXPECT_TRUE(db.DoesColumnExist("some_table", "value"));
}

class PrefetchStoreSchemaTest : public testing::Test {
 public:
  PrefetchStoreSchemaTest() = default;
  ~PrefetchStoreSchemaTest() override = default;

  void SetUp() override {
    db_ = std::make_unique<sql::Connection>();
    ASSERT_TRUE(db_->OpenInMemory());
    ASSERT_FALSE(sql::MetaTable::DoesTableExist(db_.get()));
  }

  void CheckTablesExistence() {
    EXPECT_TRUE(db_->DoesTableExist("prefetch_items"));
    EXPECT_TRUE(db_->DoesTableExist("prefetch_downloader_quota"));
    EXPECT_FALSE(db_->DoesTableExist("prefetch_items_old"));
  }

 protected:
  std::unique_ptr<sql::Connection> db_;
  std::unique_ptr<PrefetchStoreSchema> schema_;
};

TEST_F(PrefetchStoreSchemaTest, TestSchemaCreationFromNothing) {
  EXPECT_TRUE(PrefetchStoreSchema::CreateOrUpgradeIfNeeded(db_.get()));
  CheckTablesExistence();
  sql::MetaTable meta_table;
  EXPECT_TRUE(meta_table.Init(db_.get(), std::numeric_limits<int>::max(),
                              std::numeric_limits<int>::max()));
  EXPECT_EQ(PrefetchStoreSchema::kCurrentVersion,
            meta_table.GetVersionNumber());
  EXPECT_EQ(PrefetchStoreSchema::kCompatibleVersion,
            meta_table.GetCompatibleVersionNumber());
}

TEST_F(PrefetchStoreSchemaTest, TestMissingTablesAreCreatedAtLatestVersion) {
  sql::MetaTable meta_table;
  EXPECT_TRUE(meta_table.Init(db_.get(), PrefetchStoreSchema::kCurrentVersion,
                              PrefetchStoreSchema::kCompatibleVersion));
  EXPECT_EQ(PrefetchStoreSchema::kCurrentVersion,
            meta_table.GetVersionNumber());
  EXPECT_EQ(PrefetchStoreSchema::kCompatibleVersion,
            meta_table.GetCompatibleVersionNumber());

  EXPECT_TRUE(PrefetchStoreSchema::CreateOrUpgradeIfNeeded(db_.get()));
  CheckTablesExistence();
}

TEST_F(PrefetchStoreSchemaTest, TestMissingTablesAreRecreated) {
  EXPECT_TRUE(PrefetchStoreSchema::CreateOrUpgradeIfNeeded(db_.get()));
  CheckTablesExistence();

  EXPECT_TRUE(db_->Execute("DROP TABLE prefetch_items"));
  EXPECT_TRUE(PrefetchStoreSchema::CreateOrUpgradeIfNeeded(db_.get()));
  CheckTablesExistence();

  EXPECT_TRUE(db_->Execute("DROP TABLE prefetch_downloader_quota"));
  EXPECT_TRUE(PrefetchStoreSchema::CreateOrUpgradeIfNeeded(db_.get()));
  CheckTablesExistence();
}

void CreateVersion1TablesWithSampleRows(sql::Connection* db) {
  // Create version 1 tables.
  const char kV0ItemsTableCreationSql[] =
      "CREATE TABLE prefetch_items"
      "(offline_id INTEGER PRIMARY KEY NOT NULL,"
      " state INTEGER NOT NULL DEFAULT 0,"
      " generate_bundle_attempts INTEGER NOT NULL DEFAULT 0,"
      " get_operation_attempts INTEGER NOT NULL DEFAULT 0,"
      " download_initiation_attempts INTEGER NOT NULL DEFAULT 0,"
      " archive_body_length INTEGER_NOT_NULL DEFAULT -1,"
      " creation_time INTEGER NOT NULL,"
      " freshness_time INTEGER NOT NULL,"
      " error_code INTEGER NOT NULL DEFAULT 0,"
      " file_size INTEGER NOT NULL DEFAULT 0,"
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
  EXPECT_TRUE(db->Execute(kV0ItemsTableCreationSql));
  const char kV0QuotaTableCreationSql[] =
      "CREATE TABLE prefetch_downloader_quota"
      "(quota_id INTEGER PRIMARY KEY NOT NULL DEFAULT 1,"
      " update_time INTEGER NOT NULL,"
      " available_quota INTEGER NOT NULL DEFAULT 0)";
  EXPECT_TRUE(db->Execute(kV0QuotaTableCreationSql));

  // Insert one row with artificial values into the items table.
  const char kV0ItemInsertSql[] =
      "INSERT INTO prefetch_items"
      " (offline_id, state, generate_bundle_attempts, get_operation_attempts,"
      "  download_initiation_attempts, archive_body_length, creation_time,"
      "  freshness_time, error_code, file_size, guid, client_namespace,"
      "  client_id, requested_url, final_archived_url, operation_name,"
      "  archive_body_name, title, file_path)"
      " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
  sql::Statement insertStatement1(db->GetUniqueStatement(kV0ItemInsertSql));
  // Generates fake values for all integer columns starting at 1.
  for (int i = 0; i <= 9; ++i)
    insertStatement1.BindInt(i, i + 1);
  // Generates fake values for all string columns starting at "a".
  for (int i = 10; i <= 18; ++i)
    insertStatement1.BindString(i, std::string(1, 'a' + i - 10));
  EXPECT_TRUE(insertStatement1.Run());

  // Insert one row with artificial values into the quota table.
  const char kV0QuotaInsertSql[] =
      "INSERT INTO prefetch_downloader_quota"
      " (quota_id, update_time, available_quota)"
      " VALUES (?, ?, ?)";
  sql::Statement insertStatement2(db->GetUniqueStatement(kV0QuotaInsertSql));
  // Generates fake values for all columns.
  insertStatement2.BindInt(0, 1);
  insertStatement2.BindInt(1, 2);
  insertStatement2.BindInt(2, 3);
  EXPECT_TRUE(insertStatement2.Run());
}

void CheckSampleRowsAtCurrentVersion(sql::Connection* db) {
  // Checks the previously inserted item row was migrated correctly.
  const char kV0ItemSelectSql[] =
      "SELECT "
      " offline_id, state, generate_bundle_attempts, get_operation_attempts,"
      "  download_initiation_attempts, archive_body_length, creation_time,"
      "  freshness_time, error_code, file_size, guid, client_namespace,"
      "  client_id, requested_url, final_archived_url, operation_name,"
      "  archive_body_name, title, file_path"
      " FROM prefetch_items";
  sql::Statement selectStatement1(db->GetUniqueStatement(kV0ItemSelectSql));
  ASSERT_TRUE(selectStatement1.Step());
  // Checks fake values for all integer columns.
  for (int i = 0; i <= 9; ++i)
    EXPECT_EQ(i + 1, selectStatement1.ColumnInt(i))
        << "Wrong integer value at items table's column " << i;
  // Checks fake values for all string columns.
  for (int i = 10; i <= 18; ++i)
    EXPECT_EQ(std::string(1, 'a' + i - 10), selectStatement1.ColumnString(i))
        << "Wrong string value at items table's column " << i;
  ;
  EXPECT_FALSE(selectStatement1.Step());

  // Checks the previously inserted quota row was migrated correctly.
  const char kV0QuotaSelectSql[] =
      "SELECT quota_id, update_time, available_quota"
      " FROM prefetch_downloader_quota";
  sql::Statement selectStatement2(db->GetUniqueStatement(kV0QuotaSelectSql));
  ASSERT_TRUE(selectStatement2.Step());
  // Checks fake values for all columns.
  EXPECT_EQ(1, selectStatement2.ColumnInt(0));
  EXPECT_EQ(2, selectStatement2.ColumnInt(1));
  EXPECT_EQ(3, selectStatement2.ColumnInt(2));
  EXPECT_FALSE(selectStatement2.Step());
}

// Tests that a migration from the initially deployed version of the schema,
// as it was for chromium/src at 90113a2c01ca9ff77042daacd8282a4c16aade85, is
// correctly migrated to the final, current version without losing data.
TEST_F(PrefetchStoreSchemaTest, TestMigrationFromV0) {
  // Set version numbers to 1.
  sql::MetaTable meta_table;
  EXPECT_TRUE(meta_table.Init(db_.get(), 1, 1));
  EXPECT_EQ(1, meta_table.GetVersionNumber());
  EXPECT_EQ(1, meta_table.GetCompatibleVersionNumber());

  CreateVersion1TablesWithSampleRows(db_.get());

  // Executes the migration.
  EXPECT_TRUE(PrefetchStoreSchema::CreateOrUpgradeIfNeeded(db_.get()));
  EXPECT_EQ(2, meta_table.GetVersionNumber());
  EXPECT_EQ(1, meta_table.GetCompatibleVersionNumber());
  CheckTablesExistence();

  CheckSampleRowsAtCurrentVersion(db_.get());

  // Tests that the default value for file size is now -1.
  sql::Statement fileSizeInsertStatement(db_->GetUniqueStatement(
      "INSERT INTO prefetch_items (offline_id, creation_time, freshness_time)"
      " VALUES (?, ?, ?)"));
  fileSizeInsertStatement.BindInt(0, 100);
  fileSizeInsertStatement.BindInt(1, 101);
  fileSizeInsertStatement.BindInt(2, 102);
  EXPECT_TRUE(fileSizeInsertStatement.Run());

  sql::Statement fileSizeSelectStatement(db_->GetUniqueStatement(
      "SELECT file_size FROM prefetch_items WHERE offline_id = ?"));
  fileSizeSelectStatement.BindInt(0, 100);
  ASSERT_TRUE(fileSizeSelectStatement.Step());
  EXPECT_EQ(-1, fileSizeSelectStatement.ColumnInt(0));
  EXPECT_FALSE(fileSizeSelectStatement.Step());
}

}  // namespace offline_pages
