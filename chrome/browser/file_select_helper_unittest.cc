// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "build/build_config.h"
#include "chrome/browser/file_select_helper.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::FileChooserParams;

class FileSelectHelperTest : public testing::Test {
 public:
  FileSelectHelperTest() {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &data_dir_));
    data_dir_ = data_dir_.AppendASCII("file_select_helper");
    ASSERT_TRUE(base::PathExists(data_dir_));
  }

  // The path to input data used in tests.
  base::FilePath data_dir_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileSelectHelperTest);
};

TEST_F(FileSelectHelperTest, IsAcceptTypeValid) {
  EXPECT_TRUE(FileSelectHelper::IsAcceptTypeValid("a/b"));
  EXPECT_TRUE(FileSelectHelper::IsAcceptTypeValid("abc/def"));
  EXPECT_TRUE(FileSelectHelper::IsAcceptTypeValid("abc/*"));
  EXPECT_TRUE(FileSelectHelper::IsAcceptTypeValid(".a"));
  EXPECT_TRUE(FileSelectHelper::IsAcceptTypeValid(".abc"));

  EXPECT_FALSE(FileSelectHelper::IsAcceptTypeValid("."));
  EXPECT_FALSE(FileSelectHelper::IsAcceptTypeValid("/"));
  EXPECT_FALSE(FileSelectHelper::IsAcceptTypeValid("ABC/*"));
  EXPECT_FALSE(FileSelectHelper::IsAcceptTypeValid("abc/def "));
}

#if defined(OS_MACOSX)
TEST_F(FileSelectHelperTest, ZipPackage) {
  // Zip the package.
  const char app_name[] = "CalculatorFake.app";
  base::FilePath src = data_dir_.Append(app_name);
  base::FilePath dest = FileSelectHelper::ZipPackage(src);
  ASSERT_FALSE(dest.empty());
  ASSERT_TRUE(base::PathExists(dest));

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  // Unzip the package into a temporary directory.
  base::CommandLine cl(base::FilePath("/usr/bin/unzip"));
  cl.AppendArg(dest.value().c_str());
  cl.AppendArg("-d");
  cl.AppendArg(temp_dir.GetPath().value().c_str());
  std::string output;
  EXPECT_TRUE(base::GetAppOutput(cl, &output));

  // Verify that several key files haven't changed.
  const char* files_to_verify[] = {"Contents/Info.plist",
                                   "Contents/MacOS/Calculator",
                                   "Contents/_CodeSignature/CodeResources"};
  size_t file_count = arraysize(files_to_verify);
  for (size_t i = 0; i < file_count; i++) {
    const char* relative_path = files_to_verify[i];
    base::FilePath orig_file = src.Append(relative_path);
    base::FilePath final_file =
        temp_dir.GetPath().Append(app_name).Append(relative_path);
    EXPECT_TRUE(base::ContentsEqual(orig_file, final_file));
  }
}
#endif  // defined(OS_MACOSX)

TEST_F(FileSelectHelperTest, GetSanitizedFileName) {
  // The empty path should be preserved.
  EXPECT_EQ(base::FilePath(FILE_PATH_LITERAL("")),
            FileSelectHelper::GetSanitizedFileName(base::FilePath()));

  EXPECT_EQ(base::FilePath(FILE_PATH_LITERAL("ascii.txt")),
            FileSelectHelper::GetSanitizedFileName(
                base::FilePath(FILE_PATH_LITERAL("ascii.txt"))));
  EXPECT_EQ(base::FilePath(FILE_PATH_LITERAL("trailing-spaces_")),
            FileSelectHelper::GetSanitizedFileName(
                base::FilePath(FILE_PATH_LITERAL("trailing-spaces "))));
  EXPECT_EQ(base::FilePath(FILE_PATH_LITERAL("path_components_in_name")),
            FileSelectHelper::GetSanitizedFileName(
                base::FilePath(FILE_PATH_LITERAL("path/components/in/name"))));

#if defined(OS_WIN)
  // Invalid UTF-16. However, note that on Windows, the invalid UTF-16 will pass
  // through without error.
  base::FilePath::CharType kBadName[] = {0xd801, 0xdc37, 0xdc17, 0};
#else
  // Invalid UTF-8
  base::FilePath::CharType kBadName[] = {0xe3, 0x81, 0x81, 0x81, 0x82, 0};
#endif
  base::FilePath bad_filename(kBadName);
  ASSERT_FALSE(bad_filename.empty());
  // The only thing we are testing is that if the source filename was non-empty,
  // the resulting filename is also not empty. Invalid encoded filenames can
  // cause conversions to fail. Such failures shouldn't cause the resulting
  // filename to disappear.
  EXPECT_FALSE(FileSelectHelper::GetSanitizedFileName(bad_filename).empty());
}

TEST_F(FileSelectHelperTest, LastSelectedDirectory) {
  content::TestBrowserThreadBundle browser_thread_bundle;
  TestingProfile profile;
  scoped_refptr<FileSelectHelper> file_select_helper =
      new FileSelectHelper(&profile);

  const int index = 0;
  void* params = nullptr;

  const base::FilePath dir_path_1 = data_dir_.AppendASCII("dir1");
  const base::FilePath dir_path_2 = data_dir_.AppendASCII("dir2");
  const base::FilePath file_path_1 = dir_path_1.AppendASCII("file1.txt");
  const base::FilePath file_path_2 = dir_path_1.AppendASCII("file2.txt");
  const base::FilePath file_path_3 = dir_path_2.AppendASCII("file3.txt");
  std::vector<base::FilePath> files;  // Both in dir1.
  files.push_back(file_path_1);
  files.push_back(file_path_2);
  std::vector<base::FilePath> dirs;
  dirs.push_back(dir_path_1);
  dirs.push_back(dir_path_2);

  // Modes where the parent of the selection is remembered.
  const std::vector<FileChooserParams::Mode> modes = {
    FileChooserParams::Open,
    FileChooserParams::OpenMultiple,
    FileChooserParams::Save,
  };

  for (const auto& mode : modes) {
    file_select_helper->dialog_mode_ = mode;

    file_select_helper->AddRef();  // Normally called by RunFileChooser().
    file_select_helper->FileSelected(file_path_1, index, params);
    EXPECT_EQ(dir_path_1, profile.last_selected_directory());

    file_select_helper->AddRef();  // Normally called by RunFileChooser().
    file_select_helper->FileSelected(file_path_2, index, params);
    EXPECT_EQ(dir_path_1, profile.last_selected_directory());

    file_select_helper->AddRef();  // Normally called by RunFileChooser().
    file_select_helper->FileSelected(file_path_3, index, params);
    EXPECT_EQ(dir_path_2, profile.last_selected_directory());

    file_select_helper->AddRef();  // Normally called by RunFileChooser().
    file_select_helper->MultiFilesSelected(files, params);
    EXPECT_EQ(dir_path_1, profile.last_selected_directory());
  }

  // Type where the selected folder itself is remembered.
  file_select_helper->dialog_mode_ = FileChooserParams::UploadFolder;

  file_select_helper->AddRef();  // Normally called by RunFileChooser().
  file_select_helper->FileSelected(dir_path_1, index, params);
  EXPECT_EQ(dir_path_1, profile.last_selected_directory());

  file_select_helper->AddRef();  // Normally called by RunFileChooser().
  file_select_helper->FileSelected(dir_path_2, index, params);
  EXPECT_EQ(dir_path_2, profile.last_selected_directory());

  file_select_helper->AddRef();  // Normally called by RunFileChooser().
  file_select_helper->MultiFilesSelected(dirs, params);
  EXPECT_EQ(dir_path_1, profile.last_selected_directory());
}
