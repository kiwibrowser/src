// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/file_change.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace drive {

TEST(ChangeListTest, FileChange_Change) {
  FileChange::ChangeType change_type = FileChange::CHANGE_TYPE_ADD_OR_UPDATE;
  FileChange::FileType file_type = FileChange::FILE_TYPE_FILE;

  FileChange::Change change1(change_type, file_type);
  EXPECT_EQ(change_type, change1.change());
  EXPECT_EQ(file_type, change1.file_type());

  FileChange::Change change2(change_type, file_type);
  EXPECT_EQ(change_type, change1.change());
  EXPECT_EQ(file_type, change1.file_type());
  EXPECT_EQ(change1, change2);

  FileChange::Change change3(change_type, FileChange::FILE_TYPE_DIRECTORY);
  EXPECT_EQ(change_type, change3.change());
  EXPECT_EQ(FileChange::FILE_TYPE_DIRECTORY, change3.file_type());
  EXPECT_TRUE(!(change1 == change3));
}

TEST(ChangeListTest, FileChangeChangeList) {
  FileChange::ChangeList changes;
  EXPECT_TRUE(changes.empty());
  EXPECT_EQ(0u, changes.size());

  changes.Update(FileChange::Change(FileChange::CHANGE_TYPE_ADD_OR_UPDATE,
                                    FileChange::FILE_TYPE_FILE));
  EXPECT_EQ(1u, changes.size());
}

TEST(ChangeListTest, FileChange) {
  base::FilePath change_path1(FILE_PATH_LITERAL("test"));
  base::FilePath change_path2(FILE_PATH_LITERAL("a/b/c/d"));
  base::FilePath change_path3(FILE_PATH_LITERAL("a/b/c/e"));
  base::FilePath change_dir(FILE_PATH_LITERAL("a/b/c"));

  FileChange changed_files;
  changed_files.Update(change_path1, FileChange::FILE_TYPE_FILE,
                       FileChange::CHANGE_TYPE_ADD_OR_UPDATE);
  changed_files.Update(change_path2, FileChange::FILE_TYPE_FILE,
                       FileChange::CHANGE_TYPE_ADD_OR_UPDATE);
  changed_files.Update(change_path2, FileChange::FILE_TYPE_FILE,
                       FileChange::CHANGE_TYPE_ADD_OR_UPDATE);
  changed_files.Update(change_path3, FileChange::FILE_TYPE_FILE,
                       FileChange::CHANGE_TYPE_ADD_OR_UPDATE);

  ASSERT_EQ(3u, changed_files.size());
  ASSERT_EQ(2u, changed_files.CountDirectory(change_dir));
}

}  // namespace drive
