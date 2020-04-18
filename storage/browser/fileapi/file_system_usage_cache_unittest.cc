// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/file_system_usage_cache.h"

#include <stdint.h>

#include <limits>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

using storage::FileSystemUsageCache;

namespace content {

class FileSystemUsageCacheTest : public testing::Test {
 public:
  FileSystemUsageCacheTest() = default;

  void SetUp() override { ASSERT_TRUE(data_dir_.CreateUniqueTempDir()); }

 protected:
  base::FilePath GetUsageFilePath() {
    return data_dir_.GetPath().Append(FileSystemUsageCache::kUsageFileName);
  }

  FileSystemUsageCache* usage_cache() {
    return &usage_cache_;
  }

 private:
  base::MessageLoop message_loop_;
  base::ScopedTempDir data_dir_;
  FileSystemUsageCache usage_cache_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemUsageCacheTest);
};

TEST_F(FileSystemUsageCacheTest, CreateTest) {
  base::FilePath usage_file_path = GetUsageFilePath();
  EXPECT_TRUE(usage_cache()->UpdateUsage(usage_file_path, 0));
}

TEST_F(FileSystemUsageCacheTest, SetSizeTest) {
  static const int64_t size = 240122;
  base::FilePath usage_file_path = GetUsageFilePath();
  int64_t usage = 0;
  ASSERT_TRUE(usage_cache()->UpdateUsage(usage_file_path, size));
  EXPECT_TRUE(usage_cache()->GetUsage(usage_file_path, &usage));
  EXPECT_EQ(size, usage);
}

TEST_F(FileSystemUsageCacheTest, SetLargeSizeTest) {
  static const int64_t size = std::numeric_limits<int64_t>::max();
  base::FilePath usage_file_path = GetUsageFilePath();
  int64_t usage = 0;
  ASSERT_TRUE(usage_cache()->UpdateUsage(usage_file_path, size));
  EXPECT_TRUE(usage_cache()->GetUsage(usage_file_path, &usage));
  EXPECT_EQ(size, usage);
}

TEST_F(FileSystemUsageCacheTest, IncAndGetSizeTest) {
  base::FilePath usage_file_path = GetUsageFilePath();
  uint32_t dirty = 0;
  int64_t usage = 0;
  ASSERT_TRUE(usage_cache()->UpdateUsage(usage_file_path, 98214));
  ASSERT_TRUE(usage_cache()->IncrementDirty(usage_file_path));
  EXPECT_TRUE(usage_cache()->GetDirty(usage_file_path, &dirty));
  EXPECT_EQ(1u, dirty);
  EXPECT_TRUE(usage_cache()->GetUsage(usage_file_path, &usage));
  EXPECT_EQ(98214, usage);
}

TEST_F(FileSystemUsageCacheTest, DecAndGetSizeTest) {
  static const int64_t size = 71839;
  base::FilePath usage_file_path = GetUsageFilePath();
  int64_t usage = 0;
  ASSERT_TRUE(usage_cache()->UpdateUsage(usage_file_path, size));
  // DecrementDirty for dirty = 0 is invalid. It returns false.
  ASSERT_FALSE(usage_cache()->DecrementDirty(usage_file_path));
  EXPECT_TRUE(usage_cache()->GetUsage(usage_file_path, &usage));
  EXPECT_EQ(size, usage);
}

TEST_F(FileSystemUsageCacheTest, IncDecAndGetSizeTest) {
  static const int64_t size = 198491;
  base::FilePath usage_file_path = GetUsageFilePath();
  int64_t usage = 0;
  ASSERT_TRUE(usage_cache()->UpdateUsage(usage_file_path, size));
  ASSERT_TRUE(usage_cache()->IncrementDirty(usage_file_path));
  ASSERT_TRUE(usage_cache()->DecrementDirty(usage_file_path));
  EXPECT_TRUE(usage_cache()->GetUsage(usage_file_path, &usage));
  EXPECT_EQ(size, usage);
}

TEST_F(FileSystemUsageCacheTest, DecIncAndGetSizeTest) {
  base::FilePath usage_file_path = GetUsageFilePath();
  uint32_t dirty = 0;
  int64_t usage = 0;
  ASSERT_TRUE(usage_cache()->UpdateUsage(usage_file_path, 854238));
  // DecrementDirty for dirty = 0 is invalid. It returns false.
  ASSERT_FALSE(usage_cache()->DecrementDirty(usage_file_path));
  ASSERT_TRUE(usage_cache()->IncrementDirty(usage_file_path));
  // It tests DecrementDirty (which returns false) has no effect, i.e
  // does not make dirty = -1 after DecrementDirty.
  EXPECT_TRUE(usage_cache()->GetDirty(usage_file_path, &dirty));
  EXPECT_EQ(1u, dirty);
  EXPECT_TRUE(usage_cache()->GetUsage(usage_file_path, &usage));
  EXPECT_EQ(854238, usage);
}

TEST_F(FileSystemUsageCacheTest, ManyIncsSameDecsAndGetSizeTest) {
  static const int64_t size = 82412;
  base::FilePath usage_file_path = GetUsageFilePath();
  int64_t usage = 0;
  ASSERT_TRUE(usage_cache()->UpdateUsage(usage_file_path, size));
  for (int i = 0; i < 20; i++)
    ASSERT_TRUE(usage_cache()->IncrementDirty(usage_file_path));
  for (int i = 0; i < 20; i++)
    ASSERT_TRUE(usage_cache()->DecrementDirty(usage_file_path));
  EXPECT_TRUE(usage_cache()->GetUsage(usage_file_path, &usage));
  EXPECT_EQ(size, usage);
}

TEST_F(FileSystemUsageCacheTest, ManyIncsLessDecsAndGetSizeTest) {
  uint32_t dirty = 0;
  int64_t usage = 0;
  base::FilePath usage_file_path = GetUsageFilePath();
  ASSERT_TRUE(usage_cache()->UpdateUsage(usage_file_path, 19319));
  for (int i = 0; i < 20; i++)
    ASSERT_TRUE(usage_cache()->IncrementDirty(usage_file_path));
  for (int i = 0; i < 19; i++)
    ASSERT_TRUE(usage_cache()->DecrementDirty(usage_file_path));
  EXPECT_TRUE(usage_cache()->GetDirty(usage_file_path, &dirty));
  EXPECT_EQ(1u, dirty);
  EXPECT_TRUE(usage_cache()->GetUsage(usage_file_path, &usage));
  EXPECT_EQ(19319, usage);
}

TEST_F(FileSystemUsageCacheTest, GetSizeWithoutCacheFileTest) {
  int64_t usage = 0;
  base::FilePath usage_file_path = GetUsageFilePath();
  EXPECT_FALSE(usage_cache()->GetUsage(usage_file_path, &usage));
}

TEST_F(FileSystemUsageCacheTest, IncrementDirtyWithoutCacheFileTest) {
  base::FilePath usage_file_path = GetUsageFilePath();
  EXPECT_FALSE(usage_cache()->IncrementDirty(usage_file_path));
}

TEST_F(FileSystemUsageCacheTest, DecrementDirtyWithoutCacheFileTest) {
  base::FilePath usage_file_path = GetUsageFilePath();
  EXPECT_FALSE(usage_cache()->IncrementDirty(usage_file_path));
}

}  // namespace content
