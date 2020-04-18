// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/simple/simple_version_upgrade.h"

#include <stdint.h>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/format_macros.h"
#include "base/strings/stringprintf.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/simple/simple_backend_version.h"
#include "net/disk_cache/simple/simple_entry_format_history.h"
#include "net/disk_cache/simple/simple_experiment.h"
#include "testing/gtest/include/gtest/gtest.h"

// The migration process relies on ability to rename newly created files, which
// could be problematic on Windows XP.
#if defined(OS_POSIX)

namespace {

// Same as |disk_cache::kSimpleInitialMagicNumber|.
const uint64_t kSimpleInitialMagicNumber = UINT64_C(0xfcfb6d1ba7725c30);

// The "fake index" file that cache backends use to distinguish whether the
// cache belongs to one backend or another.
const char kFakeIndexFileName[] = "index";

// Same as |SimpleIndexFile::kIndexFileName|.
const char kIndexFileName[] = "the-real-index";

bool WriteFakeIndexFileV5(const base::FilePath& cache_path) {
  disk_cache::FakeIndexData data;
  data.version = 5;
  data.initial_magic_number = kSimpleInitialMagicNumber;
  data.experiment_type = disk_cache::SimpleExperimentType::NONE;
  data.experiment_param = 0;
  const base::FilePath file_name = cache_path.AppendASCII("index");
  return sizeof(data) ==
         base::WriteFile(
             file_name, reinterpret_cast<const char*>(&data), sizeof(data));
}

TEST(SimpleVersionUpgradeTest, FailsToMigrateBackwards) {
  base::ScopedTempDir cache_dir;
  ASSERT_TRUE(cache_dir.CreateUniqueTempDir());
  const base::FilePath cache_path = cache_dir.GetPath();

  disk_cache::FakeIndexData data;
  data.version = 100500;
  data.initial_magic_number = kSimpleInitialMagicNumber;
  data.experiment_type = disk_cache::SimpleExperimentType::NONE;
  data.experiment_param = 0;
  const base::FilePath file_name = cache_path.AppendASCII(kFakeIndexFileName);
  ASSERT_EQ(static_cast<int>(sizeof(data)),
            base::WriteFile(file_name, reinterpret_cast<const char*>(&data),
                            sizeof(data)));
  EXPECT_FALSE(disk_cache::UpgradeSimpleCacheOnDisk(
      cache_dir.GetPath(), disk_cache::SimpleExperiment()));
}

TEST(SimpleVersionUpgradeTest, ExperimentFromDefault) {
  base::ScopedTempDir cache_dir;
  ASSERT_TRUE(cache_dir.CreateUniqueTempDir());
  const base::FilePath cache_path = cache_dir.GetPath();

  disk_cache::FakeIndexData data;
  data.version = disk_cache::kSimpleVersion;
  data.initial_magic_number = kSimpleInitialMagicNumber;
  data.experiment_type = disk_cache::SimpleExperimentType::NONE;
  data.experiment_param = 0;
  const base::FilePath file_name = cache_path.AppendASCII(kFakeIndexFileName);
  ASSERT_EQ(static_cast<int>(sizeof(data)),
            base::WriteFile(file_name, reinterpret_cast<const char*>(&data),
                            sizeof(data)));

  disk_cache::SimpleExperiment experiment;

  // No change in experiment, so no cache rewrite necessary.
  EXPECT_TRUE(
      disk_cache::UpgradeSimpleCacheOnDisk(cache_dir.GetPath(), experiment));

  // Changing the experiment type causes a cache rewrite.
  experiment.type = disk_cache::SimpleExperimentType::SIZE;
  EXPECT_FALSE(
      disk_cache::UpgradeSimpleCacheOnDisk(cache_dir.GetPath(), experiment));

  // Changing the experiment parameter causes a cache rewrite.
  experiment = disk_cache::SimpleExperiment();
  experiment.param = 1;
  EXPECT_FALSE(
      disk_cache::UpgradeSimpleCacheOnDisk(cache_dir.GetPath(), experiment));

  // Changing the experiment type and parameter causes a cache rewrite.
  experiment = disk_cache::SimpleExperiment();
  experiment.type = disk_cache::SimpleExperimentType::SIZE;
  experiment.param = 2;
  EXPECT_FALSE(
      disk_cache::UpgradeSimpleCacheOnDisk(cache_dir.GetPath(), experiment));
}

TEST(SimpleVersionUpgradeTest, ExperimentBacktoDefault) {
  base::ScopedTempDir cache_dir;
  ASSERT_TRUE(cache_dir.CreateUniqueTempDir());
  const base::FilePath cache_path = cache_dir.GetPath();

  disk_cache::FakeIndexData data;
  data.version = disk_cache::kSimpleVersion;
  data.initial_magic_number = kSimpleInitialMagicNumber;
  data.experiment_type = disk_cache::SimpleExperimentType::SIZE;
  data.experiment_param = 4;
  const base::FilePath file_name = cache_path.AppendASCII(kFakeIndexFileName);
  ASSERT_EQ(static_cast<int>(sizeof(data)),
            base::WriteFile(file_name, reinterpret_cast<const char*>(&data),
                            sizeof(data)));

  // The cache needs to transition from SIZE experiment back to NONE.
  EXPECT_FALSE(disk_cache::UpgradeSimpleCacheOnDisk(
      cache_dir.GetPath(), disk_cache::SimpleExperiment()));
}

TEST(SimpleVersionUpgradeTest, ExperimentStoredInNewFakeIndex) {
  base::ScopedTempDir cache_dir;
  ASSERT_TRUE(cache_dir.CreateUniqueTempDir());
  const base::FilePath cache_path = cache_dir.GetPath();
  const base::FilePath file_name = cache_path.AppendASCII(kFakeIndexFileName);

  disk_cache::SimpleExperiment experiment;
  experiment.type = disk_cache::SimpleExperimentType::SIZE;
  experiment.param = 100u;

  // There is no index on disk, so the upgrade should write a new one and return
  // true.
  EXPECT_TRUE(
      disk_cache::UpgradeSimpleCacheOnDisk(cache_dir.GetPath(), experiment));

  std::string new_fake_index_contents;
  ASSERT_TRUE(base::ReadFileToString(cache_path.AppendASCII(kFakeIndexFileName),
                                     &new_fake_index_contents));
  const disk_cache::FakeIndexData* fake_index_header;
  fake_index_header = reinterpret_cast<const disk_cache::FakeIndexData*>(
      new_fake_index_contents.data());

  EXPECT_EQ(disk_cache::SimpleExperimentType::SIZE,
            fake_index_header->experiment_type);
  EXPECT_EQ(100u, fake_index_header->experiment_param);
}

TEST(SimpleVersionUpgradeTest, FakeIndexVersionGetsUpdated) {
  base::ScopedTempDir cache_dir;
  ASSERT_TRUE(cache_dir.CreateUniqueTempDir());
  const base::FilePath cache_path = cache_dir.GetPath();

  WriteFakeIndexFileV5(cache_path);
  const std::string file_contents("incorrectly serialized data");
  const base::FilePath index_file = cache_path.AppendASCII(kIndexFileName);
  ASSERT_EQ(
      static_cast<int>(file_contents.size()),
      base::WriteFile(index_file, file_contents.data(), file_contents.size()));

  // Upgrade.
  ASSERT_TRUE(disk_cache::UpgradeSimpleCacheOnDisk(
      cache_path, disk_cache::SimpleExperiment()));

  // Check that the version in the fake index file is updated.
  std::string new_fake_index_contents;
  ASSERT_TRUE(base::ReadFileToString(cache_path.AppendASCII(kFakeIndexFileName),
                                     &new_fake_index_contents));
  const disk_cache::FakeIndexData* fake_index_header;
  EXPECT_EQ(sizeof(*fake_index_header), new_fake_index_contents.size());
  fake_index_header = reinterpret_cast<const disk_cache::FakeIndexData*>(
      new_fake_index_contents.data());
  EXPECT_EQ(disk_cache::kSimpleVersion, fake_index_header->version);
  EXPECT_EQ(kSimpleInitialMagicNumber, fake_index_header->initial_magic_number);
}

TEST(SimpleVersionUpgradeTest, UpgradeV5V6IndexMustDisappear) {
  base::ScopedTempDir cache_dir;
  ASSERT_TRUE(cache_dir.CreateUniqueTempDir());
  const base::FilePath cache_path = cache_dir.GetPath();

  WriteFakeIndexFileV5(cache_path);
  const std::string file_contents("incorrectly serialized data");
  const base::FilePath index_file = cache_path.AppendASCII(kIndexFileName);
  ASSERT_EQ(
      static_cast<int>(file_contents.size()),
      base::WriteFile(index_file, file_contents.data(), file_contents.size()));

  // Create a few entry-like files.
  const uint64_t kEntries = 5;
  for (uint64_t entry_hash = 0; entry_hash < kEntries; ++entry_hash) {
    for (int index = 0; index < 3; ++index) {
      std::string file_name =
          base::StringPrintf("%016" PRIx64 "_%1d", entry_hash, index);
      std::string entry_contents =
          file_contents +
          base::StringPrintf(" %" PRIx64, static_cast<uint64_t>(entry_hash));
      ASSERT_EQ(static_cast<int>(entry_contents.size()),
                base::WriteFile(cache_path.AppendASCII(file_name),
                                entry_contents.data(), entry_contents.size()));
    }
  }

  // Upgrade.
  ASSERT_TRUE(disk_cache::UpgradeIndexV5V6(cache_path));

  // Check that the old index disappeared but the files remain unchanged.
  EXPECT_FALSE(base::PathExists(index_file));
  for (uint64_t entry_hash = 0; entry_hash < kEntries; ++entry_hash) {
    for (int index = 0; index < 3; ++index) {
      std::string file_name =
          base::StringPrintf("%016" PRIx64 "_%1d", entry_hash, index);
      std::string expected_contents =
          file_contents +
          base::StringPrintf(" %" PRIx64, static_cast<uint64_t>(entry_hash));
      std::string real_contents;
      EXPECT_TRUE(base::ReadFileToString(cache_path.AppendASCII(file_name),
                                         &real_contents));
      EXPECT_EQ(expected_contents, real_contents);
    }
  }
}

}  // namespace

#endif  // defined(OS_POSIX)
