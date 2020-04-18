// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/touch_operation.h"

#include "base/files/file_path.h"
#include "base/time/time.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_errors.h"
#include "components/drive/file_system/operation_test_base.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace file_system {

typedef OperationTestBase TouchOperationTest;

TEST_F(TouchOperationTest, TouchFile) {
  TouchOperation operation(blocking_task_runner(),
                           delegate(),
                           metadata());

  const base::FilePath kTestPath(FILE_PATH_LITERAL("drive/root/File 1.txt"));
  const base::Time::Exploded kLastAccessTime = {
    2012, 7, 0, 19, 15, 59, 13, 123
  };
  const base::Time::Exploded kLastModifiedTime = {
    2013, 7, 0, 19, 15, 59, 13, 123
  };

  FileError error = FILE_ERROR_FAILED;
  base::Time last_access_time_utc;
  base::Time last_modified_time_utc;
  EXPECT_TRUE(
      base::Time::FromUTCExploded(kLastAccessTime, &last_access_time_utc));
  EXPECT_TRUE(
      base::Time::FromUTCExploded(kLastModifiedTime, &last_modified_time_utc));
  operation.TouchFile(kTestPath, last_access_time_utc, last_modified_time_utc,
                      google_apis::test_util::CreateCopyResultCallback(&error));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(kTestPath, &entry));
  EXPECT_EQ(last_access_time_utc,
            base::Time::FromInternalValue(entry.file_info().last_accessed()));
  EXPECT_EQ(last_modified_time_utc,
            base::Time::FromInternalValue(entry.file_info().last_modified()));
  EXPECT_EQ(last_modified_time_utc,
            base::Time::FromInternalValue(entry.last_modified_by_me()));
  EXPECT_EQ(ResourceEntry::DIRTY, entry.metadata_edit_state());

  EXPECT_EQ(1U, delegate()->get_changed_files().size());
  EXPECT_TRUE(delegate()->get_changed_files().count(kTestPath));

  EXPECT_EQ(1U, delegate()->updated_local_ids().size());
  EXPECT_TRUE(delegate()->updated_local_ids().count(entry.local_id()));
}

}  // namespace file_system
}  // namespace drive
