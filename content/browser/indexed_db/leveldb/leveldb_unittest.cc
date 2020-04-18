// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <algorithm>
#include <cstring>
#include <string>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_piece.h"
#include "base/test/simple_test_clock.h"
#include "content/browser/indexed_db/leveldb/leveldb_comparator.h"
#include "content/browser/indexed_db/leveldb/leveldb_database.h"
#include "content/browser/indexed_db/leveldb/leveldb_env.h"
#include "content/browser/indexed_db/leveldb/leveldb_write_batch.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/leveldb_chrome.h"

namespace content {
namespace leveldb_unittest {

static const size_t kDefaultMaxOpenIteratorsPerDatabase = 50;

class SimpleComparator : public LevelDBComparator {
 public:
  int Compare(const base::StringPiece& a,
              const base::StringPiece& b) const override {
    size_t len = std::min(a.size(), b.size());
    return memcmp(a.begin(), b.begin(), len);
  }
  const char* Name() const override { return "temp_comparator"; }
};

TEST(LevelDBDatabaseTest, CorruptionTest) {
  base::ScopedTempDir temp_directory;
  ASSERT_TRUE(temp_directory.CreateUniqueTempDir());

  const std::string key("key");
  const std::string value("value");
  std::string put_value;
  std::string got_value;
  SimpleComparator comparator;

  std::unique_ptr<LevelDBDatabase> leveldb;
  LevelDBDatabase::Open(temp_directory.GetPath(), &comparator,
                        kDefaultMaxOpenIteratorsPerDatabase, &leveldb);
  EXPECT_TRUE(leveldb);
  put_value = value;
  leveldb::Status status = leveldb->Put(key, &put_value);
  EXPECT_TRUE(status.ok());
  leveldb.reset();
  EXPECT_FALSE(leveldb);

  LevelDBDatabase::Open(temp_directory.GetPath(), &comparator,
                        kDefaultMaxOpenIteratorsPerDatabase, &leveldb);
  EXPECT_TRUE(leveldb);
  bool found = false;
  status = leveldb->Get(key, &got_value, &found);
  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(found);
  EXPECT_EQ(value, got_value);
  leveldb.reset();
  EXPECT_FALSE(leveldb);

  EXPECT_TRUE(
      leveldb_chrome::CorruptClosedDBForTesting(temp_directory.GetPath()));

  status = LevelDBDatabase::Open(temp_directory.GetPath(), &comparator,
                                 kDefaultMaxOpenIteratorsPerDatabase, &leveldb);
  EXPECT_FALSE(leveldb);
  EXPECT_FALSE(status.ok());
  EXPECT_TRUE(status.IsCorruption());

  status = LevelDBDatabase::Destroy(temp_directory.GetPath());
  EXPECT_TRUE(status.ok());

  status = LevelDBDatabase::Open(temp_directory.GetPath(), &comparator,
                                 kDefaultMaxOpenIteratorsPerDatabase, &leveldb);
  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(leveldb);
  status = leveldb->Get(key, &got_value, &found);
  EXPECT_TRUE(status.ok());
  EXPECT_FALSE(found);
}

TEST(LevelDB, Locking) {
  base::ScopedTempDir temp_directory;
  ASSERT_TRUE(temp_directory.CreateUniqueTempDir());

  leveldb::Env* env = LevelDBEnv::Get();
  base::FilePath file = temp_directory.GetPath().AppendASCII("LOCK");
  leveldb::FileLock* lock;
  leveldb::Status status = env->LockFile(file.AsUTF8Unsafe(), &lock);
  EXPECT_TRUE(status.ok());

  status = env->UnlockFile(lock);
  EXPECT_TRUE(status.ok());

  status = env->LockFile(file.AsUTF8Unsafe(), &lock);
  EXPECT_TRUE(status.ok());

  leveldb::FileLock* lock2;
  status = env->LockFile(file.AsUTF8Unsafe(), &lock2);
  EXPECT_FALSE(status.ok());

  status = env->UnlockFile(lock);
  EXPECT_TRUE(status.ok());
}

TEST(LevelDBDatabaseTest, LastModified) {
  const std::string key("key");
  const std::string value("value");
  std::string put_value;
  SimpleComparator comparator;
  auto test_clock = std::make_unique<base::SimpleTestClock>();
  base::SimpleTestClock* clock_ptr = test_clock.get();
  clock_ptr->Advance(base::TimeDelta::FromHours(2));
  std::unique_ptr<LevelDBDatabase> leveldb =
      LevelDBDatabase::OpenInMemory(&comparator);
  ASSERT_TRUE(leveldb);
  leveldb->SetClockForTesting(std::move(test_clock));
  // Calling |Put| sets time modified.
  put_value = value;
  base::Time now_time = clock_ptr->Now();
  leveldb::Status status = leveldb->Put(key, &put_value);
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(now_time, leveldb->LastModified());

  // Calling |Remove| sets time modified.
  clock_ptr->Advance(base::TimeDelta::FromSeconds(200));
  now_time = clock_ptr->Now();
  status = leveldb->Remove(key);
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(now_time, leveldb->LastModified());

  // Calling |Write| sets time modified
  clock_ptr->Advance(base::TimeDelta::FromMinutes(15));
  now_time = clock_ptr->Now();
  auto batch = LevelDBWriteBatch::Create();
  batch->Put(key, value);
  batch->Remove(key);
  status = leveldb->Write(*batch);
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(now_time, leveldb->LastModified());
}

}  // namespace leveldb_unittest
}  // namespace content
