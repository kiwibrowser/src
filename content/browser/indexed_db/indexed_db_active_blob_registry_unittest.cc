// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <set>

#include "base/macros.h"
#include "base/test/test_simple_task_runner.h"
#include "content/browser/indexed_db/indexed_db_active_blob_registry.h"
#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "content/browser/indexed_db/indexed_db_fake_backing_store.h"
#include "content/browser/indexed_db/mock_indexed_db_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

namespace content {

namespace {

class RegistryTestMockFactory : public MockIndexedDBFactory {
 public:
  RegistryTestMockFactory() : duplicate_calls_(false) {}

  void ReportOutstandingBlobs(const url::Origin& origin,
                              bool blobs_outstanding) override {
    if (blobs_outstanding) {
      if (origins_.count(origin)) {
        duplicate_calls_ = true;
      } else {
        origins_.insert(origin);
      }
    } else {
      if (!origins_.count(origin)) {
        duplicate_calls_ = true;
      } else {
        origins_.erase(origin);
      }
    }
  }

  bool CheckNoOriginsInUse() const {
    return !duplicate_calls_ && origins_.empty();
  }

  bool CheckSingleOriginInUse(const url::Origin& origin) const {
    return !duplicate_calls_ && origins_.size() == 1 && origins_.count(origin);
  }

 private:
  ~RegistryTestMockFactory() override {}

  std::set<url::Origin> origins_;
  bool duplicate_calls_;

  DISALLOW_COPY_AND_ASSIGN(RegistryTestMockFactory);
};

class MockIDBBackingStore : public IndexedDBFakeBackingStore {
 public:
  typedef std::pair<int64_t, int64_t> KeyPair;
  typedef std::set<KeyPair> KeyPairSet;

  MockIDBBackingStore(IndexedDBFactory* factory,
                      base::SequencedTaskRunner* task_runner)
      : IndexedDBFakeBackingStore(factory, task_runner),
        duplicate_calls_(false) {}

  void ReportBlobUnused(int64_t database_id, int64_t blob_key) override {
    unused_blobs_.insert(std::make_pair(database_id, blob_key));
  }

  bool CheckUnusedBlobsEmpty() const {
    return !duplicate_calls_ && !unused_blobs_.size();
  }
  bool CheckSingleUnusedBlob(int64_t database_id, int64_t blob_key) const {
    return !duplicate_calls_ && unused_blobs_.size() == 1 &&
           unused_blobs_.count(std::make_pair(database_id, blob_key));
  }

  const KeyPairSet& unused_blobs() const { return unused_blobs_; }

 protected:
  ~MockIDBBackingStore() override {}

 private:
  KeyPairSet unused_blobs_;
  bool duplicate_calls_;

  DISALLOW_COPY_AND_ASSIGN(MockIDBBackingStore);
};

// Base class for our test fixtures.
class IndexedDBActiveBlobRegistryTest : public testing::Test {
 public:
  typedef IndexedDBBlobInfo::ReleaseCallback ReleaseCallback;

  static const int64_t kDatabaseId0 = 7;
  static const int64_t kDatabaseId1 = 12;
  static const int64_t kBlobKey0 = 77;
  static const int64_t kBlobKey1 = 14;

  IndexedDBActiveBlobRegistryTest()
      : task_runner_(new base::TestSimpleTaskRunner),
        factory_(new RegistryTestMockFactory),
        backing_store_(
            new MockIDBBackingStore(factory_.get(), task_runner_.get())),
        registry_(std::make_unique<IndexedDBActiveBlobRegistry>(
            backing_store_.get())) {}

  void RunUntilIdle() { task_runner_->RunUntilIdle(); }
  RegistryTestMockFactory* factory() const { return factory_.get(); }
  MockIDBBackingStore* backing_store() const { return backing_store_.get(); }
  IndexedDBActiveBlobRegistry* registry() const { return registry_.get(); }

 private:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  scoped_refptr<RegistryTestMockFactory> factory_;
  scoped_refptr<MockIDBBackingStore> backing_store_;
  std::unique_ptr<IndexedDBActiveBlobRegistry> registry_;

  DISALLOW_COPY_AND_ASSIGN(IndexedDBActiveBlobRegistryTest);
};

TEST_F(IndexedDBActiveBlobRegistryTest, DeleteUnused) {
  EXPECT_TRUE(factory()->CheckNoOriginsInUse());
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  EXPECT_FALSE(registry()->MarkDeletedCheckIfUsed(kDatabaseId0, kBlobKey0));
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckNoOriginsInUse());
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());
}

TEST_F(IndexedDBActiveBlobRegistryTest, SimpleUse) {
  EXPECT_TRUE(factory()->CheckNoOriginsInUse());
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  base::Closure add_ref =
      registry()->GetAddBlobRefCallback(kDatabaseId0, kBlobKey0);
  ReleaseCallback release =
      registry()->GetFinalReleaseCallback(kDatabaseId0, kBlobKey0);
  std::move(add_ref).Run();
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckSingleOriginInUse(backing_store()->origin()));
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  std::move(release).Run(base::FilePath());
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckNoOriginsInUse());
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());
}

TEST_F(IndexedDBActiveBlobRegistryTest, DeleteWhileInUse) {
  EXPECT_TRUE(factory()->CheckNoOriginsInUse());
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  base::Closure add_ref =
      registry()->GetAddBlobRefCallback(kDatabaseId0, kBlobKey0);
  ReleaseCallback release =
      registry()->GetFinalReleaseCallback(kDatabaseId0, kBlobKey0);

  std::move(add_ref).Run();
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckSingleOriginInUse(backing_store()->origin()));
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  EXPECT_TRUE(registry()->MarkDeletedCheckIfUsed(kDatabaseId0, kBlobKey0));
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckSingleOriginInUse(backing_store()->origin()));
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  std::move(release).Run(base::FilePath());
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckNoOriginsInUse());
  EXPECT_TRUE(backing_store()->CheckSingleUnusedBlob(kDatabaseId0, kBlobKey0));
}

TEST_F(IndexedDBActiveBlobRegistryTest, MultipleBlobs) {
  EXPECT_TRUE(factory()->CheckNoOriginsInUse());
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  base::Closure add_ref_00 =
      registry()->GetAddBlobRefCallback(kDatabaseId0, kBlobKey0);
  ReleaseCallback release_00 =
      registry()->GetFinalReleaseCallback(kDatabaseId0, kBlobKey0);
  base::Closure add_ref_01 =
      registry()->GetAddBlobRefCallback(kDatabaseId0, kBlobKey1);
  ReleaseCallback release_01 =
      registry()->GetFinalReleaseCallback(kDatabaseId0, kBlobKey1);
  base::Closure add_ref_10 =
      registry()->GetAddBlobRefCallback(kDatabaseId1, kBlobKey0);
  ReleaseCallback release_10 =
      registry()->GetFinalReleaseCallback(kDatabaseId1, kBlobKey0);
  base::Closure add_ref_11 =
      registry()->GetAddBlobRefCallback(kDatabaseId1, kBlobKey1);
  ReleaseCallback release_11 =
      registry()->GetFinalReleaseCallback(kDatabaseId1, kBlobKey1);

  std::move(add_ref_00).Run();
  std::move(add_ref_01).Run();
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckSingleOriginInUse(backing_store()->origin()));
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  std::move(release_00).Run(base::FilePath());
  std::move(add_ref_10).Run();
  std::move(add_ref_11).Run();
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckSingleOriginInUse(backing_store()->origin()));
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  EXPECT_TRUE(registry()->MarkDeletedCheckIfUsed(kDatabaseId0, kBlobKey1));
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckSingleOriginInUse(backing_store()->origin()));
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  std::move(release_01).Run(base::FilePath());
  std::move(release_11).Run(base::FilePath());
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckSingleOriginInUse(backing_store()->origin()));
  EXPECT_TRUE(backing_store()->CheckSingleUnusedBlob(kDatabaseId0, kBlobKey1));

  std::move(release_10).Run(base::FilePath());
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckNoOriginsInUse());
  EXPECT_TRUE(backing_store()->CheckSingleUnusedBlob(kDatabaseId0, kBlobKey1));
}

TEST_F(IndexedDBActiveBlobRegistryTest, ForceShutdown) {
  EXPECT_TRUE(factory()->CheckNoOriginsInUse());
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  base::Closure add_ref_0 =
      registry()->GetAddBlobRefCallback(kDatabaseId0, kBlobKey0);
  ReleaseCallback release_0 =
      registry()->GetFinalReleaseCallback(kDatabaseId0, kBlobKey0);
  base::Closure add_ref_1 =
      registry()->GetAddBlobRefCallback(kDatabaseId0, kBlobKey1);
  ReleaseCallback release_1 =
      registry()->GetFinalReleaseCallback(kDatabaseId0, kBlobKey1);

  std::move(add_ref_0).Run();
  RunUntilIdle();

  EXPECT_TRUE(factory()->CheckSingleOriginInUse(backing_store()->origin()));
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  registry()->ForceShutdown();

  std::move(add_ref_1).Run();
  RunUntilIdle();

  // Nothing changes.
  EXPECT_TRUE(factory()->CheckSingleOriginInUse(backing_store()->origin()));
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());

  std::move(release_0).Run(base::FilePath());
  std::move(release_1).Run(base::FilePath());
  RunUntilIdle();

  // Nothing changes.
  EXPECT_TRUE(factory()->CheckSingleOriginInUse(backing_store()->origin()));
  EXPECT_TRUE(backing_store()->CheckUnusedBlobsEmpty());
}

}  // namespace

}  // namespace content
