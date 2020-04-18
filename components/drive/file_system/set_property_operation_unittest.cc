// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_system/set_property_operation.h"

#include "base/files/file_path.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_errors.h"
#include "components/drive/file_system/operation_test_base.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/drive_api_requests.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace file_system {
namespace {

const base::FilePath::CharType kTestPath[] =
    FILE_PATH_LITERAL("drive/root/File 1.txt");
const char kTestKey[] = "key";
const char kTestValue[] = "value";
const char kTestAnotherValue[] = "another-value";

}  // namespace

typedef OperationTestBase SetPropertyOperationTest;

TEST_F(SetPropertyOperationTest, SetProperty) {
  SetPropertyOperation operation(blocking_task_runner(), delegate(),
                                 metadata());

  const base::FilePath test_path(kTestPath);
  FileError result = FILE_ERROR_FAILED;
  operation.SetProperty(
      test_path, google_apis::drive::Property::Visibility::VISIBILITY_PRIVATE,
      kTestKey, kTestValue,
      google_apis::test_util::CreateCopyResultCallback(&result));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, result);

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(test_path, &entry));
  EXPECT_EQ(ResourceEntry::DIRTY, entry.metadata_edit_state());
  ASSERT_EQ(1, entry.new_properties().size());
  const drive::Property property = entry.new_properties().Get(0);
  EXPECT_EQ(Property_Visibility_PRIVATE, property.visibility());
  EXPECT_EQ(kTestKey, property.key());
  EXPECT_EQ(kTestValue, property.value());

  EXPECT_EQ(0u, delegate()->get_changed_files().size());
  EXPECT_FALSE(delegate()->get_changed_files().count(test_path));

  EXPECT_EQ(1u, delegate()->updated_local_ids().size());
  EXPECT_TRUE(delegate()->updated_local_ids().count(entry.local_id()));
}

TEST_F(SetPropertyOperationTest, SetProperty_Duplicate) {
  SetPropertyOperation operation(blocking_task_runner(), delegate(),
                                 metadata());

  const base::FilePath test_path(kTestPath);
  FileError result = FILE_ERROR_FAILED;
  operation.SetProperty(
      test_path, google_apis::drive::Property::Visibility::VISIBILITY_PRIVATE,
      kTestKey, kTestValue,
      google_apis::test_util::CreateCopyResultCallback(&result));
  operation.SetProperty(
      test_path, google_apis::drive::Property::Visibility::VISIBILITY_PRIVATE,
      kTestKey, kTestValue,
      google_apis::test_util::CreateCopyResultCallback(&result));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, result);

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(test_path, &entry));
  EXPECT_EQ(1, entry.new_properties().size());
}

TEST_F(SetPropertyOperationTest, SetProperty_Overwrite) {
  SetPropertyOperation operation(blocking_task_runner(), delegate(),
                                 metadata());

  const base::FilePath test_path(kTestPath);
  FileError result = FILE_ERROR_FAILED;
  operation.SetProperty(
      test_path, google_apis::drive::Property::Visibility::VISIBILITY_PUBLIC,
      kTestKey, kTestValue,
      google_apis::test_util::CreateCopyResultCallback(&result));
  operation.SetProperty(
      test_path, google_apis::drive::Property::Visibility::VISIBILITY_PUBLIC,
      kTestKey, kTestAnotherValue,
      google_apis::test_util::CreateCopyResultCallback(&result));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, result);

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(test_path, &entry));
  ASSERT_EQ(1, entry.new_properties().size());
  const drive::Property property = entry.new_properties().Get(0);
  EXPECT_EQ(Property_Visibility_PUBLIC, property.visibility());
  EXPECT_EQ(kTestKey, property.key());
  EXPECT_EQ(kTestAnotherValue, property.value());
}

TEST_F(SetPropertyOperationTest, SetProperty_DifferentVisibilities) {
  SetPropertyOperation operation(blocking_task_runner(), delegate(),
                                 metadata());

  {
    const base::FilePath test_path(kTestPath);
    FileError result = FILE_ERROR_FAILED;
    operation.SetProperty(
        test_path, google_apis::drive::Property::Visibility::VISIBILITY_PRIVATE,
        kTestKey, kTestValue,
        google_apis::test_util::CreateCopyResultCallback(&result));
    content::RunAllTasksUntilIdle();
    EXPECT_EQ(FILE_ERROR_OK, result);

    ResourceEntry entry;
    EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(test_path, &entry));
    ASSERT_EQ(1, entry.new_properties().size());
    const drive::Property property = entry.new_properties().Get(0);
    EXPECT_EQ(Property_Visibility_PRIVATE, property.visibility());
    EXPECT_EQ(kTestKey, property.key());
    EXPECT_EQ(kTestValue, property.value());
  }

  // Insert another property with the same key, same value but different
  // visibility.
  {
    const base::FilePath test_path(kTestPath);
    FileError result = FILE_ERROR_FAILED;
    operation.SetProperty(
        test_path, google_apis::drive::Property::Visibility::VISIBILITY_PUBLIC,
        kTestKey, kTestAnotherValue,
        google_apis::test_util::CreateCopyResultCallback(&result));
    content::RunAllTasksUntilIdle();
    EXPECT_EQ(FILE_ERROR_OK, result);

    ResourceEntry entry;
    EXPECT_EQ(FILE_ERROR_OK, GetLocalResourceEntry(test_path, &entry));
    ASSERT_EQ(2, entry.new_properties().size());
    const drive::Property property = entry.new_properties().Get(1);
    EXPECT_EQ(Property_Visibility_PUBLIC, property.visibility());
    EXPECT_EQ(kTestKey, property.key());
    EXPECT_EQ(kTestAnotherValue, property.value());
  }
}

}  // namespace file_system
}  // namespace drive
