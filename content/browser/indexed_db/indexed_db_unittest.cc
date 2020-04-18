// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <utility>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/browser/indexed_db/indexed_db_connection.h"
#include "content/browser/indexed_db/indexed_db_context_impl.h"
#include "content/browser/indexed_db/indexed_db_factory_impl.h"
#include "content/browser/indexed_db/mock_indexed_db_callbacks.h"
#include "content/browser/indexed_db/mock_indexed_db_database_callbacks.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "storage/browser/quota/quota_manager.h"
#include "storage/browser/quota/special_storage_policy.h"
#include "storage/browser/test/mock_quota_manager_proxy.h"
#include "storage/browser/test/mock_special_storage_policy.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

using url::Origin;

namespace content {

class IndexedDBTest : public testing::Test {
 public:
  const Origin kNormalOrigin;
  const Origin kSessionOnlyOrigin;

  IndexedDBTest()
      : kNormalOrigin(url::Origin::Create(GURL("http://normal/"))),
        kSessionOnlyOrigin(url::Origin::Create(GURL("http://session-only/"))),
        special_storage_policy_(
            base::MakeRefCounted<MockSpecialStoragePolicy>()),
        quota_manager_proxy_(
            base::MakeRefCounted<MockQuotaManagerProxy>(nullptr, nullptr)) {
    special_storage_policy_->AddSessionOnly(kSessionOnlyOrigin.GetURL());
  }
  ~IndexedDBTest() override {
    quota_manager_proxy_->SimulateQuotaManagerDestroyed();
  }

 protected:
  scoped_refptr<MockSpecialStoragePolicy> special_storage_policy_;
  scoped_refptr<MockQuotaManagerProxy> quota_manager_proxy_;

 private:
  TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(IndexedDBTest);
};

TEST_F(IndexedDBTest, ClearSessionOnlyDatabases) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::FilePath normal_path;
  base::FilePath session_only_path;

  // Create the scope which will ensure we run the destructor of the context
  // which should trigger the clean up.
  {
    scoped_refptr<IndexedDBContextImpl> idb_context =
        base::MakeRefCounted<IndexedDBContextImpl>(
            temp_dir.GetPath(), special_storage_policy_.get(),
            quota_manager_proxy_.get());

    normal_path = idb_context->GetFilePathForTesting(kNormalOrigin);
    session_only_path = idb_context->GetFilePathForTesting(kSessionOnlyOrigin);
    ASSERT_TRUE(base::CreateDirectory(normal_path));
    ASSERT_TRUE(base::CreateDirectory(session_only_path));
    RunAllTasksUntilIdle();
    quota_manager_proxy_->SimulateQuotaManagerDestroyed();
  }

  RunAllTasksUntilIdle();

  EXPECT_TRUE(base::DirectoryExists(normal_path));
  EXPECT_FALSE(base::DirectoryExists(session_only_path));
}

TEST_F(IndexedDBTest, SetForceKeepSessionState) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::FilePath normal_path;
  base::FilePath session_only_path;

  // Create the scope which will ensure we run the destructor of the context.
  {
    // Create some indexedDB paths.
    // With the levelDB backend, these are directories.
    scoped_refptr<IndexedDBContextImpl> idb_context =
        base::MakeRefCounted<IndexedDBContextImpl>(
            temp_dir.GetPath(), special_storage_policy_.get(),
            quota_manager_proxy_.get());

    // Save session state. This should bypass the destruction-time deletion.
    idb_context->SetForceKeepSessionState();

    normal_path = idb_context->GetFilePathForTesting(kNormalOrigin);
    session_only_path = idb_context->GetFilePathForTesting(kSessionOnlyOrigin);
    ASSERT_TRUE(base::CreateDirectory(normal_path));
    ASSERT_TRUE(base::CreateDirectory(session_only_path));
    base::RunLoop().RunUntilIdle();
  }

  // Make sure we wait until the destructor has run.
  base::RunLoop().RunUntilIdle();

  // No data was cleared because of SetForceKeepSessionState.
  EXPECT_TRUE(base::DirectoryExists(normal_path));
  EXPECT_TRUE(base::DirectoryExists(session_only_path));
}

class ForceCloseDBCallbacks : public IndexedDBCallbacks {
 public:
  ForceCloseDBCallbacks(scoped_refptr<IndexedDBContextImpl> idb_context,
                        const Origin& origin)
      : IndexedDBCallbacks(nullptr, origin, nullptr, idb_context->TaskRunner()),
        idb_context_(idb_context),
        origin_(origin) {}

  void OnSuccess() override {}
  void OnSuccess(const std::vector<base::string16>&) override {}
  void OnSuccess(std::unique_ptr<IndexedDBConnection> connection,
                 const IndexedDBDatabaseMetadata& metadata) override {
    connection_ = std::move(connection);
    idb_context_->ConnectionOpened(origin_, connection_.get());
  }

  IndexedDBConnection* connection() { return connection_.get(); }

 protected:
  ~ForceCloseDBCallbacks() override {}

 private:
  scoped_refptr<IndexedDBContextImpl> idb_context_;
  Origin origin_;
  std::unique_ptr<IndexedDBConnection> connection_;
  DISALLOW_COPY_AND_ASSIGN(ForceCloseDBCallbacks);
};

TEST_F(IndexedDBTest, ForceCloseOpenDatabasesOnDelete) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  scoped_refptr<IndexedDBContextImpl> idb_context =
      base::MakeRefCounted<IndexedDBContextImpl>(temp_dir.GetPath(),
                                                 special_storage_policy_.get(),
                                                 quota_manager_proxy_.get());

  const Origin kTestOrigin = Origin::Create(GURL("http://test/"));
  idb_context->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBContextImpl* idb_context,
             scoped_refptr<MockIndexedDBDatabaseCallbacks> open_db_callbacks,
             scoped_refptr<MockIndexedDBDatabaseCallbacks> closed_db_callbacks,
             scoped_refptr<ForceCloseDBCallbacks> open_callbacks,
             scoped_refptr<ForceCloseDBCallbacks> closed_callbacks,
             const Origin& origin) {

            const int child_process_id = 0;
            const int64_t host_transaction_id = 0;
            const int64_t version = 0;
            const scoped_refptr<net::URLRequestContextGetter> request_context;

            IndexedDBFactory* factory = idb_context->GetIDBFactory();

            base::FilePath test_path =
                idb_context->GetFilePathForTesting(origin);

            factory->Open(base::ASCIIToUTF16("opendb"),
                          std::make_unique<IndexedDBPendingConnection>(
                              open_callbacks, open_db_callbacks,
                              child_process_id, host_transaction_id, version),
                          request_context, origin, idb_context->data_path());
            EXPECT_TRUE(base::DirectoryExists(test_path));

            factory->Open(base::ASCIIToUTF16("closeddb"),
                          std::make_unique<IndexedDBPendingConnection>(
                              closed_callbacks, closed_db_callbacks,
                              child_process_id, host_transaction_id, version),
                          request_context, origin, idb_context->data_path());

            closed_callbacks->connection()->Close();

            idb_context->DeleteForOrigin(origin);

            EXPECT_TRUE(open_db_callbacks->forced_close_called());
            EXPECT_FALSE(closed_db_callbacks->forced_close_called());
            EXPECT_FALSE(base::DirectoryExists(test_path));
          },
          base::Unretained(idb_context.get()),
          base::MakeRefCounted<MockIndexedDBDatabaseCallbacks>(),
          base::MakeRefCounted<MockIndexedDBDatabaseCallbacks>(),
          base::MakeRefCounted<ForceCloseDBCallbacks>(idb_context, kTestOrigin),
          base::MakeRefCounted<ForceCloseDBCallbacks>(idb_context, kTestOrigin),
          kTestOrigin));
  RunAllTasksUntilIdle();
}

TEST_F(IndexedDBTest, DeleteFailsIfDirectoryLocked) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  const Origin kTestOrigin = Origin::Create(GURL("http://test/"));

  scoped_refptr<IndexedDBContextImpl> idb_context =
      base::MakeRefCounted<IndexedDBContextImpl>(temp_dir.GetPath(),
                                                 special_storage_policy_.get(),
                                                 quota_manager_proxy_.get());

  base::FilePath test_path = idb_context->GetFilePathForTesting(kTestOrigin);
  ASSERT_TRUE(base::CreateDirectory(test_path));

  std::unique_ptr<LevelDBLock> lock =
      LevelDBDatabase::LockForTesting(test_path);
  ASSERT_TRUE(lock);

  // TODO(jsbell): Remove static_cast<> when overloads are eliminated.
  void (IndexedDBContextImpl::* delete_for_origin)(const Origin&) =
      &IndexedDBContextImpl::DeleteForOrigin;
  idb_context->TaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(delete_for_origin, idb_context, kTestOrigin));
  RunAllTasksUntilIdle();

  EXPECT_TRUE(base::DirectoryExists(test_path));
}

TEST_F(IndexedDBTest, ForceCloseOpenDatabasesOnCommitFailure) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  scoped_refptr<IndexedDBContextImpl> idb_context =
      base::MakeRefCounted<IndexedDBContextImpl>(temp_dir.GetPath(),
                                                 special_storage_policy_.get(),
                                                 quota_manager_proxy_.get());

  idb_context->TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](IndexedDBContextImpl* idb_context, const base::FilePath temp_path,
             scoped_refptr<MockIndexedDBCallbacks> callbacks,
             scoped_refptr<MockIndexedDBDatabaseCallbacks> db_callbacks) {
            const Origin kTestOrigin = Origin::Create(GURL("http://test/"));

            scoped_refptr<IndexedDBFactoryImpl> factory =
                static_cast<IndexedDBFactoryImpl*>(
                    idb_context->GetIDBFactory());

            const int child_process_id = 0;
            const int64_t transaction_id = 1;
            const scoped_refptr<net::URLRequestContextGetter> request_context;

            std::unique_ptr<IndexedDBPendingConnection> connection(
                std::make_unique<IndexedDBPendingConnection>(
                    callbacks, db_callbacks, child_process_id, transaction_id,
                    IndexedDBDatabaseMetadata::DEFAULT_VERSION));
            factory->Open(base::ASCIIToUTF16("db"), std::move(connection),
                          request_context, Origin(kTestOrigin), temp_path);

            EXPECT_TRUE(callbacks->connection());

            // ConnectionOpened() is usually called by the dispatcher.
            idb_context->ConnectionOpened(kTestOrigin, callbacks->connection());

            EXPECT_TRUE(factory->IsBackingStoreOpen(kTestOrigin));

            // Simulate the write failure.
            leveldb::Status status =
                leveldb::Status::IOError("Simulated failure");
            idb_context->GetIDBFactory()->HandleBackingStoreFailure(
                kTestOrigin);

            EXPECT_TRUE(db_callbacks->forced_close_called());
            EXPECT_FALSE(factory->IsBackingStoreOpen(kTestOrigin));
          },
          base::Unretained(idb_context.get()), temp_dir.GetPath(),

          base::MakeRefCounted<MockIndexedDBCallbacks>(),
          base::MakeRefCounted<MockIndexedDBDatabaseCallbacks>()));
  RunAllTasksUntilIdle();
}

}  // namespace content
