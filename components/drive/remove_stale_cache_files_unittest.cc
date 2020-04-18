// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/remove_stale_cache_files.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/drive/chromeos/drive_test_util.h"
#include "components/drive/chromeos/fake_free_disk_space_getter.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_system_core_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {

class RemoveStaleCacheFilesTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    fake_free_disk_space_getter_.reset(new FakeFreeDiskSpaceGetter);

    metadata_storage_.reset(new ResourceMetadataStorage(
        temp_dir_.GetPath(), base::ThreadTaskRunnerHandle::Get().get()));

    cache_.reset(new FileCache(metadata_storage_.get(), temp_dir_.GetPath(),
                               base::ThreadTaskRunnerHandle::Get().get(),
                               fake_free_disk_space_getter_.get()));

    resource_metadata_.reset(new ResourceMetadata(
        metadata_storage_.get(), cache_.get(),
        base::ThreadTaskRunnerHandle::Get()));

    ASSERT_TRUE(metadata_storage_->Initialize());
    ASSERT_TRUE(cache_->Initialize());
    ASSERT_EQ(FILE_ERROR_OK, resource_metadata_->Initialize());
  }

  content::TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir temp_dir_;

  std::unique_ptr<ResourceMetadataStorage, test_util::DestroyHelperForTests>
      metadata_storage_;
  std::unique_ptr<FileCache, test_util::DestroyHelperForTests> cache_;
  std::unique_ptr<ResourceMetadata, test_util::DestroyHelperForTests>
      resource_metadata_;
  std::unique_ptr<FakeFreeDiskSpaceGetter> fake_free_disk_space_getter_;
};

TEST_F(RemoveStaleCacheFilesTest, RemoveStaleCacheFiles) {
  base::FilePath dummy_file;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir_.GetPath(), &dummy_file));
  std::string md5_metadata("abcdef0123456789"), md5_cache("ABCDEF9876543210");

  // Create a stale cache file.
  ResourceEntry entry;
  std::string local_id;
  entry.mutable_file_specific_info()->set_md5(md5_metadata);
  entry.set_parent_local_id(util::kDriveGrandRootLocalId);
  entry.set_title("File.txt");
  EXPECT_EQ(FILE_ERROR_OK, resource_metadata_->AddEntry(entry, &local_id));

  EXPECT_EQ(FILE_ERROR_OK,
            cache_->Store(local_id, md5_cache, dummy_file,
                          FileCache::FILE_OPERATION_COPY));

  // Remove stale cache files.
  RemoveStaleCacheFiles(cache_.get(), resource_metadata_.get());

  // Verify that the cache is deleted.
  EXPECT_EQ(FILE_ERROR_OK,
            resource_metadata_->GetResourceEntryById(local_id, &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_present());
}

TEST_F(RemoveStaleCacheFilesTest, DirtyCacheFiles) {
  base::FilePath dummy_file;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir_.GetPath(), &dummy_file));

  // Dirty entry.
  std::string md5_metadata("abcdef0123456789");

  ResourceEntry entry;
  std::string local_id;
  entry.mutable_file_specific_info()->set_md5(md5_metadata);
  entry.set_parent_local_id(util::kDriveGrandRootLocalId);
  entry.set_title("file.txt");
  EXPECT_EQ(FILE_ERROR_OK, resource_metadata_->AddEntry(entry, &local_id));

  EXPECT_EQ(FILE_ERROR_OK,
            cache_->Store(local_id, std::string(), dummy_file,
                          FileCache::FILE_OPERATION_COPY));

  // Remove stale cache files.
  RemoveStaleCacheFiles(cache_.get(), resource_metadata_.get());

  // Dirty cache should not be removed even though its MD5 doesn't match.
  EXPECT_EQ(FILE_ERROR_OK,
            resource_metadata_->GetResourceEntryById(local_id, &entry));
  EXPECT_TRUE(entry.file_specific_info().cache_state().is_present());
}

}  // namespace internal
}  // namespace drive
