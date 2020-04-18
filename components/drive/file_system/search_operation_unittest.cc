// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/search_operation.h"

#include <stddef.h>

#include "base/callback_helpers.h"
#include "components/drive/chromeos/loader_controller.h"
#include "components/drive/file_system/operation_test_base.h"
#include "components/drive/service/fake_drive_service.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace file_system {

typedef OperationTestBase SearchOperationTest;

TEST_F(SearchOperationTest, ContentSearch) {
  SearchOperation operation(blocking_task_runner(), scheduler(), metadata(),
                            loader_controller());

  std::set<std::string> expected_results;
  expected_results.insert(
      "drive/root/Directory 1/Sub Directory Folder/Sub Sub Directory Folder");
  expected_results.insert("drive/root/Directory 1/Sub Directory Folder");
  expected_results.insert("drive/root/Directory 1/SubDirectory File 1.txt");
  expected_results.insert("drive/root/Directory 1");
  expected_results.insert("drive/root/Directory 2 excludeDir-test");

  FileError error = FILE_ERROR_FAILED;
  GURL next_link;
  std::unique_ptr<std::vector<SearchResultInfo>> results;

  operation.Search("Directory", GURL(),
                   google_apis::test_util::CreateCopyResultCallback(
                       &error, &next_link, &results));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_TRUE(next_link.is_empty());
  EXPECT_EQ(expected_results.size(), results->size());
  for (size_t i = 0; i < results->size(); i++) {
    EXPECT_TRUE(expected_results.count(results->at(i).path.AsUTF8Unsafe()))
        << results->at(i).path.AsUTF8Unsafe();
  }
}

TEST_F(SearchOperationTest, ContentSearchWithNewEntry) {
  SearchOperation operation(blocking_task_runner(), scheduler(), metadata(),
                            loader_controller());

  // Create a new directory in the drive service.
  google_apis::DriveApiErrorCode status = google_apis::DRIVE_OTHER_ERROR;
  std::unique_ptr<google_apis::FileResource> server_entry;
  fake_service()->AddNewDirectory(
      fake_service()->GetRootResourceId(), "New Directory 1!",
      AddNewDirectoryOptions(),
      google_apis::test_util::CreateCopyResultCallback(&status, &server_entry));
  content::RunAllTasksUntilIdle();
  ASSERT_EQ(google_apis::HTTP_CREATED, status);

  // As the result of the first Search(), only entries in the current file
  // system snapshot are expected to be returned in the "right" path. New
  // entries like "New Directory 1!" is temporarily added to "drive/other".
  std::set<std::string> expected_results;
  expected_results.insert("drive/root/Directory 1");
  expected_results.insert("drive/other/New Directory 1!");

  FileError error = FILE_ERROR_FAILED;
  GURL next_link;
  std::unique_ptr<std::vector<SearchResultInfo>> results;

  operation.Search("\"Directory 1\"", GURL(),
                   google_apis::test_util::CreateCopyResultCallback(
                       &error, &next_link, &results));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_TRUE(next_link.is_empty());
  ASSERT_EQ(expected_results.size(), results->size());
  for (size_t i = 0; i < results->size(); i++) {
    EXPECT_TRUE(expected_results.count(results->at(i).path.AsUTF8Unsafe()))
        << results->at(i).path.AsUTF8Unsafe();
  }

  // Load the change from FakeDriveService.
  ASSERT_EQ(FILE_ERROR_OK, CheckForUpdates());

  // Now the new entry must be reported to be in the right directory.
  expected_results.clear();
  expected_results.insert("drive/root/Directory 1");
  expected_results.insert("drive/root/New Directory 1!");
  error = FILE_ERROR_FAILED;
  operation.Search("\"Directory 1\"", GURL(),
                   google_apis::test_util::CreateCopyResultCallback(
                       &error, &next_link, &results));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_TRUE(next_link.is_empty());
  ASSERT_EQ(expected_results.size(), results->size());
  for (size_t i = 0; i < results->size(); i++) {
    EXPECT_TRUE(expected_results.count(results->at(i).path.AsUTF8Unsafe()))
        << results->at(i).path.AsUTF8Unsafe();
  }
}

TEST_F(SearchOperationTest, ContentSearchEmptyResult) {
  SearchOperation operation(blocking_task_runner(), scheduler(), metadata(),
                            loader_controller());

  FileError error = FILE_ERROR_FAILED;
  GURL next_link;
  std::unique_ptr<std::vector<SearchResultInfo>> results;

  operation.Search("\"no-match query\"", GURL(),
                   google_apis::test_util::CreateCopyResultCallback(
                       &error, &next_link, &results));
  content::RunAllTasksUntilIdle();

  EXPECT_EQ(FILE_ERROR_OK, error);
  EXPECT_TRUE(next_link.is_empty());
  EXPECT_EQ(0U, results->size());
}

TEST_F(SearchOperationTest, Lock) {
  SearchOperation operation(blocking_task_runner(), scheduler(), metadata(),
                            loader_controller());

  // Lock.
  std::unique_ptr<base::ScopedClosureRunner> lock =
      loader_controller()->GetLock();

  // Search does not return the result as long as lock is alive.
  FileError error = FILE_ERROR_FAILED;
  GURL next_link;
  std::unique_ptr<std::vector<SearchResultInfo>> results;

  operation.Search("\"Directory 1\"", GURL(),
                   google_apis::test_util::CreateCopyResultCallback(
                       &error, &next_link, &results));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_FAILED, error);
  EXPECT_FALSE(results);

  // Unlock, this should resume the pending search.
  lock.reset();
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(results);
  EXPECT_EQ(1u, results->size());
}

}  // namespace file_system
}  // namespace drive
