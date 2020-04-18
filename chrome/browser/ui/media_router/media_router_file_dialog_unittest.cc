// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/media_router/media_router_file_dialog.h"

#include <memory>

#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/common/media_router/issue.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using testing::_;
using testing::ContainsRegex;
using testing::Field;
using testing::InvokeWithoutArgs;
using testing::Return;
using testing::Test;

namespace media_router {

namespace {

// Clears out async tasks
void FlushTasks() {
  base::TaskScheduler::GetInstance()->FlushForTesting();
  base::RunLoop().RunUntilIdle();
}

}  // namespace

class MockDelegate
    : public MediaRouterFileDialog::MediaRouterFileDialogDelegate {
 public:
  MOCK_METHOD1(FileDialogFileSelected,
               void(const ui::SelectedFileInfo& file_info));
  MOCK_METHOD1(FileDialogSelectionFailed, void(const IssueInfo& issue));
  MOCK_METHOD0(FileDialogSelectionCanceled, void());
};

class MockFileSystemDelegate
    : public MediaRouterFileDialog::FileSystemDelegate {
 public:
  MockFileSystemDelegate() : MediaRouterFileDialog::FileSystemDelegate() {}
  ~MockFileSystemDelegate() override {}

  MOCK_CONST_METHOD1(FileExists, bool(const base::FilePath& file_path));
  MOCK_CONST_METHOD1(IsFileReadable, bool(const base::FilePath& file_path));
  MOCK_CONST_METHOD1(IsFileTypeSupported,
                     bool(const base::FilePath& file_path));
  MOCK_CONST_METHOD1(GetFileSize, int64_t(const base::FilePath& file_path));
  MOCK_CONST_METHOD1(GetLastSelectedDirectory,
                     base::FilePath(Browser* browser));
  MOCK_METHOD4(OpenFileDialog,
               void(ui::SelectFileDialog::Listener* listener,
                    const Browser* browser,
                    const base::FilePath& default_directory,
                    const ui::SelectFileDialog::FileTypeInfo* file_type_info));
};

class MediaRouterFileDialogTest : public Test {
 public:
  MediaRouterFileDialogTest() {
    fake_path = base::FilePath(FILE_PATH_LITERAL("im/a/fake_path.mp3"));

    scoped_feature_list_.InitFromCommandLine(
        "EnableCastLocalMedia" /* enabled features */,
        std::string() /* disabled features */);
  }

  void SetUp() override {
    mock_delegate_ = std::make_unique<MockDelegate>();

    auto temp_mock = std::make_unique<MockFileSystemDelegate>();
    mock_file_system_delegate = temp_mock.get();

    dialog_ = std::make_unique<MediaRouterFileDialog>(mock_delegate_.get(),
                                                      std::move(temp_mock));
    dialog_as_listener_ = dialog_.get();

    // Setup default file checks to all pass
    ON_CALL(*mock_file_system_delegate, FileExists(_))
        .WillByDefault(Return(true));
    ON_CALL(*mock_file_system_delegate, IsFileReadable(_))
        .WillByDefault(Return(true));
    ON_CALL(*mock_file_system_delegate, IsFileTypeSupported(_))
        .WillByDefault(Return(true));
    ON_CALL(*mock_file_system_delegate, GetFileSize(_))
        .WillByDefault(Return(1));
  }

  void FileSelectedExpectFailure(base::FilePath fake_path) {
    fake_path_name = fake_path.BaseName().LossyDisplayName();
    std::string error_title = l10n_util::GetStringFUTF8(
        IDS_MEDIA_ROUTER_ISSUE_FILE_CAST_ERROR, fake_path_name);

    EXPECT_CALL(*mock_delegate_, FileDialogSelectionFailed(
                                     Field(&IssueInfo::title, error_title)));

    dialog_as_listener_->FileSelected(fake_path, 0, 0);

    // Flush out the async file validation calls
    FlushTasks();
  }

 protected:
  std::unique_ptr<MockDelegate> mock_delegate_;
  MockFileSystemDelegate* mock_file_system_delegate = nullptr;
  std::unique_ptr<MediaRouterFileDialog> dialog_;

  // Used for simulating calls from a SelectFileDialog.
  ui::SelectFileDialog::Listener* dialog_as_listener_ = nullptr;

  base::FilePath fake_path;
  base::string16 fake_path_name;

  base::test::ScopedFeatureList scoped_feature_list_;
  content::TestBrowserThreadBundle thread_bundle_;
};

TEST_F(MediaRouterFileDialogTest, SelectFileSuccess) {
  // File selection succeeds, success callback called with the right file info.
  // Selected file URL is set properly.

  // Expect all the checks and return passes
  EXPECT_CALL(*mock_file_system_delegate, FileExists(fake_path))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_file_system_delegate, IsFileReadable(fake_path))
      .WillOnce(Return(true));
  EXPECT_CALL(*mock_file_system_delegate, GetFileSize(fake_path))
      .WillOnce(Return(1));

  EXPECT_CALL(*mock_delegate_,
              FileDialogFileSelected(
                  Field(&ui::SelectedFileInfo::local_path, fake_path)));

  dialog_as_listener_->FileSelected(fake_path, 0, 0);

  FlushTasks();

  ASSERT_THAT(dialog_->GetLastSelectedFileUrl().GetContent(),
              ContainsRegex(base::UTF16ToUTF8(fake_path.LossyDisplayName())));
}

TEST_F(MediaRouterFileDialogTest, SelectFileCanceled) {
  // File selection gets cancelled, failure callback called
  EXPECT_CALL(*mock_delegate_, FileDialogSelectionCanceled());

  dialog_as_listener_->FileSelectionCanceled(0);
}

TEST_F(MediaRouterFileDialogTest, SelectFailureFileDoesNotExist) {
  EXPECT_CALL(*mock_file_system_delegate, FileExists(fake_path))
      .WillOnce(Return(false));

  FileSelectedExpectFailure(fake_path);
}

TEST_F(MediaRouterFileDialogTest, SelectFailureFileDoesNotContainContent) {
  EXPECT_CALL(*mock_file_system_delegate, GetFileSize(fake_path))
      .WillOnce(Return(0));

  FileSelectedExpectFailure(fake_path);
}

TEST_F(MediaRouterFileDialogTest, SelectFailureCannotReadGetFileSize) {
  EXPECT_CALL(*mock_file_system_delegate, GetFileSize(fake_path))
      .WillOnce(Return(-1));

  FileSelectedExpectFailure(fake_path);
}

TEST_F(MediaRouterFileDialogTest, SelectFailureCannotReadFile) {
  EXPECT_CALL(*mock_file_system_delegate, IsFileReadable(fake_path))
      .WillOnce(Return(false));

  FileSelectedExpectFailure(fake_path);
}

TEST_F(MediaRouterFileDialogTest, SelectFailureFileNotSupported) {
  EXPECT_CALL(*mock_file_system_delegate, IsFileTypeSupported(fake_path))
      .WillOnce(Return(false));

  FileSelectedExpectFailure(fake_path);
}

}  // namespace media_router
