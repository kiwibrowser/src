// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_util.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/ppapi/ppapi_test.h"
#include "components/download/quarantine/quarantine.h"
#include "ppapi/shared_impl/test_utils.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/shell_dialogs/select_file_dialog_factory.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/shell_dialogs/selected_file_info.h"

#if defined(FULL_SAFE_BROWSING)
#include "chrome/browser/safe_browsing/download_protection/download_protection_service.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "components/safe_browsing/db/test_database_manager.h"

using safe_browsing::DownloadProtectionService;
using safe_browsing::SafeBrowsingService;
#endif

namespace {

class TestSelectFileDialogFactory final : public ui::SelectFileDialogFactory {
 public:
  using SelectedFileInfoList = std::vector<ui::SelectedFileInfo>;

  enum Mode {
    RESPOND_WITH_FILE_LIST,
    CANCEL,
    REPLACE_BASENAME,
    NOT_REACHED,
  };

  TestSelectFileDialogFactory(Mode mode,
                              const SelectedFileInfoList& selected_file_info)
      : selected_file_info_(selected_file_info), mode_(mode) {
    // Only safe because this class is 'final'
    ui::SelectFileDialog::SetFactory(this);
  }

  // SelectFileDialogFactory
  ui::SelectFileDialog* Create(
      ui::SelectFileDialog::Listener* listener,
      std::unique_ptr<ui::SelectFilePolicy> policy) override {
    return new SelectFileDialog(listener, std::move(policy),
                                selected_file_info_, mode_);
  }

 private:
  class SelectFileDialog : public ui::SelectFileDialog {
   public:
    SelectFileDialog(Listener* listener,
                     std::unique_ptr<ui::SelectFilePolicy> policy,
                     const SelectedFileInfoList& selected_file_info,
                     Mode mode)
        : ui::SelectFileDialog(listener, std::move(policy)),
          selected_file_info_(selected_file_info),
          mode_(mode) {}

   protected:
    // ui::SelectFileDialog
    void SelectFileImpl(Type type,
                        const base::string16& title,
                        const base::FilePath& default_path,
                        const FileTypeInfo* file_types,
                        int file_type_index,
                        const base::FilePath::StringType& default_extension,
                        gfx::NativeWindow owning_window,
                        void* params) override {
      switch (mode_) {
        case RESPOND_WITH_FILE_LIST:
          break;

        case CANCEL:
          EXPECT_EQ(0u, selected_file_info_.size());
          break;

        case REPLACE_BASENAME:
          EXPECT_EQ(1u, selected_file_info_.size());
          for (auto& selected_file : selected_file_info_) {
            selected_file =
                ui::SelectedFileInfo(selected_file.file_path.DirName().Append(
                                         default_path.BaseName()),
                                     selected_file.local_path.DirName().Append(
                                         default_path.BaseName()));
          }
          break;

        case NOT_REACHED:
          ADD_FAILURE() << "Unexpected SelectFileImpl invocation.";
      }

      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::Bind(&SelectFileDialog::RespondToFileSelectionRequest, this,
                     params));
    }
    bool HasMultipleFileTypeChoicesImpl() override { return false; }

    // BaseShellDialog
    bool IsRunning(gfx::NativeWindow owning_window) const override {
      return false;
    }
    void ListenerDestroyed() override {}

   private:
    void RespondToFileSelectionRequest(void* params) {
      if (selected_file_info_.size() == 0)
        listener_->FileSelectionCanceled(params);
      else if (selected_file_info_.size() == 1)
        listener_->FileSelectedWithExtraInfo(selected_file_info_.front(), 0,
                                             params);
      else
        listener_->MultiFilesSelectedWithExtraInfo(selected_file_info_, params);
    }

    SelectedFileInfoList selected_file_info_;
    Mode mode_;
  };

  std::vector<ui::SelectedFileInfo> selected_file_info_;
  Mode mode_;
};

class PPAPIFileChooserTest : public OutOfProcessPPAPITest {};

#if defined(FULL_SAFE_BROWSING)

struct SafeBrowsingTestConfiguration {
  std::map<base::FilePath::StringType, safe_browsing::DownloadCheckResult>
      result_map;
  safe_browsing::DownloadCheckResult default_result =
      safe_browsing::DownloadCheckResult::SAFE;
};

class FakeDatabaseManager
    : public safe_browsing::TestSafeBrowsingDatabaseManager {
 public:
  bool IsSupported() const override { return true; }
  bool MatchDownloadWhitelistUrl(const GURL& url) override {
    // This matches the behavior in RunTestViaHTTP().
    return url.SchemeIsHTTPOrHTTPS() && url.has_path() &&
           base::StartsWith(url.path(), "/test_case.html",
                            base::CompareCase::SENSITIVE);
  }

 protected:
  ~FakeDatabaseManager() override {}
};

class FakeDownloadProtectionService : public DownloadProtectionService {
 public:
  explicit FakeDownloadProtectionService(
      const SafeBrowsingTestConfiguration* test_config)
      : DownloadProtectionService(nullptr), test_configuration_(test_config) {}

  void CheckPPAPIDownloadRequest(
      const GURL& requestor_url,
      const GURL& initiating_frame_url_unused,
      content::WebContents* web_contents_unused,
      const base::FilePath& default_file_path,
      const std::vector<base::FilePath::StringType>& alternate_extensions,
      Profile* /* profile */,
      const safe_browsing::CheckDownloadCallback& callback) override {
    const auto iter =
        test_configuration_->result_map.find(default_file_path.Extension());
    if (iter != test_configuration_->result_map.end()) {
      callback.Run(iter->second);
      return;
    }

    for (const auto extension : alternate_extensions) {
      EXPECT_EQ(base::FilePath::kExtensionSeparator, extension[0]);
      const auto iter = test_configuration_->result_map.find(extension);
      if (iter != test_configuration_->result_map.end()) {
        callback.Run(iter->second);
        return;
      }
    }

    callback.Run(test_configuration_->default_result);
  }

 private:
  const SafeBrowsingTestConfiguration* test_configuration_;
};

class TestSafeBrowsingService
    : public safe_browsing::ServicesDelegate::ServicesCreator,
      public safe_browsing::SafeBrowsingService {
 public:
  explicit TestSafeBrowsingService(const SafeBrowsingTestConfiguration* config)
      : test_configuration_(config) {
    services_delegate_ =
        safe_browsing::ServicesDelegate::CreateForTest(this, this);
  }

 private:
  // safe_browsing::ServicesDelegate::ServicesCreator
  bool CanCreateDownloadProtectionService() override { return true; }
  bool CanCreateIncidentReportingService() override { return false; }
  bool CanCreateResourceRequestDetector() override { return false; }
  DownloadProtectionService* CreateDownloadProtectionService() override {
    return new FakeDownloadProtectionService(test_configuration_);
  }
  safe_browsing::IncidentReportingService* CreateIncidentReportingService()
      override {
    return nullptr;
  }
  safe_browsing::ResourceRequestDetector* CreateResourceRequestDetector()
      override {
    return nullptr;
  }

  const SafeBrowsingTestConfiguration* test_configuration_;
};

class TestSafeBrowsingServiceFactory
    : public safe_browsing::SafeBrowsingServiceFactory {
 public:
  explicit TestSafeBrowsingServiceFactory(
      const SafeBrowsingTestConfiguration* config)
      : test_configuration_(config) {}

  SafeBrowsingService* CreateSafeBrowsingService() override {
    SafeBrowsingService* service =
        new TestSafeBrowsingService(test_configuration_);
    return service;
  }

 private:
  const SafeBrowsingTestConfiguration* test_configuration_;
};

class PPAPIFileChooserTestWithSBService : public PPAPIFileChooserTest {
 public:
  PPAPIFileChooserTestWithSBService()
      : safe_browsing_service_factory_(&safe_browsing_test_configuration_) {}

  void SetUp() override {
    SafeBrowsingService::RegisterFactory(&safe_browsing_service_factory_);
    PPAPIFileChooserTest::SetUp();
  }
  void TearDown() override {
    PPAPIFileChooserTest::TearDown();
    SafeBrowsingService::RegisterFactory(nullptr);
  }

 protected:
  SafeBrowsingTestConfiguration safe_browsing_test_configuration_;

 private:
  TestSafeBrowsingServiceFactory safe_browsing_service_factory_;
};

#endif

}  // namespace

IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTest, FileChooser_Open_Success) {
  const char kContents[] = "Hello from browser";
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::FilePath existing_filename = temp_dir.GetPath().AppendASCII("foo");
  ASSERT_EQ(
      static_cast<int>(sizeof(kContents) - 1),
      base::WriteFile(existing_filename, kContents, sizeof(kContents) - 1));

  TestSelectFileDialogFactory::SelectedFileInfoList file_info_list;
  file_info_list.push_back(
      ui::SelectedFileInfo(existing_filename, existing_filename));
  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::RESPOND_WITH_FILE_LIST, file_info_list);
  RunTestViaHTTP("FileChooser_OpenSimple");
}

IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTest, FileChooser_Open_Cancel) {
  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::CANCEL,
      TestSelectFileDialogFactory::SelectedFileInfoList());
  RunTestViaHTTP("FileChooser_OpenCancel");
}

IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTest, FileChooser_SaveAs_Success) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath suggested_filename = temp_dir.GetPath().AppendASCII("foo");

  TestSelectFileDialogFactory::SelectedFileInfoList file_info_list;
  file_info_list.push_back(
      ui::SelectedFileInfo(suggested_filename, suggested_filename));
  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::RESPOND_WITH_FILE_LIST, file_info_list);

  RunTestViaHTTP("FileChooser_SaveAsSafeDefaultName");
  ASSERT_TRUE(base::PathExists(suggested_filename));
}

IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTest,
                       FileChooser_SaveAs_SafeDefaultName) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath suggested_filename = temp_dir.GetPath().AppendASCII("foo");

  TestSelectFileDialogFactory::SelectedFileInfoList file_info_list;
  file_info_list.push_back(
      ui::SelectedFileInfo(suggested_filename, suggested_filename));
  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::REPLACE_BASENAME, file_info_list);

  RunTestViaHTTP("FileChooser_SaveAsSafeDefaultName");
  base::FilePath actual_filename =
      temp_dir.GetPath().AppendASCII("innocuous.txt");

  ASSERT_TRUE(base::PathExists(actual_filename));
  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(actual_filename, &file_contents));
  EXPECT_EQ("Hello from PPAPI", file_contents);
}

IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTest,
                       FileChooser_SaveAs_UnsafeDefaultName) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath suggested_filename = temp_dir.GetPath().AppendASCII("foo");

  TestSelectFileDialogFactory::SelectedFileInfoList file_info_list;
  file_info_list.push_back(
      ui::SelectedFileInfo(suggested_filename, suggested_filename));
  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::REPLACE_BASENAME, file_info_list);

  RunTestViaHTTP("FileChooser_SaveAsUnsafeDefaultName");
  base::FilePath actual_filename =
      temp_dir.GetPath().AppendASCII("unsafe.txt_");

  ASSERT_TRUE(base::PathExists(actual_filename));
  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(actual_filename, &file_contents));
  EXPECT_EQ("Hello from PPAPI", file_contents);
}

IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTest, FileChooser_SaveAs_Cancel) {
  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::CANCEL,
      TestSelectFileDialogFactory::SelectedFileInfoList());
  RunTestViaHTTP("FileChooser_SaveAsCancel");
}

#if defined(OS_WIN) || defined(OS_LINUX)
// On Windows, tests that a file downloaded via PPAPI FileChooser API has the
// mark-of-the-web. The PPAPI FileChooser implementation invokes QuarantineFile
// in order to mark the file as being downloaded from the web as soon as the
// file is created. This MOTW prevents the file being opened without due
// security warnings if the file is executable.
IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTest, FileChooser_Quarantine) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath suggested_filename = temp_dir.GetPath().AppendASCII("foo");

  TestSelectFileDialogFactory::SelectedFileInfoList file_info_list;
  file_info_list.push_back(
      ui::SelectedFileInfo(suggested_filename, suggested_filename));
  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::REPLACE_BASENAME, file_info_list);

  RunTestViaHTTP("FileChooser_SaveAsDangerousExecutableAllowed");
  base::FilePath actual_filename =
      temp_dir.GetPath().AppendASCII("dangerous.exe");

  ASSERT_TRUE(base::PathExists(actual_filename));
  EXPECT_TRUE(download::IsFileQuarantined(actual_filename, GURL(), GURL()));
}
#endif  // defined(OS_WIN) || defined(OS_LINUX)

#if defined(FULL_SAFE_BROWSING)
// These tests only make sense when SafeBrowsing is enabled. They verify
// that files written via the FileChooser_Trusted API are properly passed
// through Safe Browsing.

IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTestWithSBService,
                       FileChooser_SaveAs_DangerousExecutable_Allowed) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  safe_browsing_test_configuration_.default_result =
      safe_browsing::DownloadCheckResult::DANGEROUS;
  safe_browsing_test_configuration_.result_map.insert(
      std::make_pair(base::FilePath::StringType(FILE_PATH_LITERAL(".exe")),
                     safe_browsing::DownloadCheckResult::SAFE));

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath suggested_filename = temp_dir.GetPath().AppendASCII("foo");

  TestSelectFileDialogFactory::SelectedFileInfoList file_info_list;
  file_info_list.push_back(
      ui::SelectedFileInfo(suggested_filename, suggested_filename));
  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::REPLACE_BASENAME, file_info_list);

  RunTestViaHTTP("FileChooser_SaveAsDangerousExecutableAllowed");
  base::FilePath actual_filename =
      temp_dir.GetPath().AppendASCII("dangerous.exe");

  ASSERT_TRUE(base::PathExists(actual_filename));
  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(actual_filename, &file_contents));
  EXPECT_EQ("Hello from PPAPI", file_contents);
}

IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTestWithSBService,
                       FileChooser_SaveAs_DangerousExecutable_Disallowed) {
  safe_browsing_test_configuration_.default_result =
      safe_browsing::DownloadCheckResult::SAFE;
  safe_browsing_test_configuration_.result_map.insert(
      std::make_pair(base::FilePath::StringType(FILE_PATH_LITERAL(".exe")),
                     safe_browsing::DownloadCheckResult::DANGEROUS));

  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::NOT_REACHED,
      TestSelectFileDialogFactory::SelectedFileInfoList());
  RunTestViaHTTP("FileChooser_SaveAsDangerousExecutableDisallowed");
}

IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTestWithSBService,
                       FileChooser_SaveAs_DangerousExtensionList_Disallowed) {
  safe_browsing_test_configuration_.default_result =
      safe_browsing::DownloadCheckResult::SAFE;
  safe_browsing_test_configuration_.result_map.insert(
      std::make_pair(base::FilePath::StringType(FILE_PATH_LITERAL(".exe")),
                     safe_browsing::DownloadCheckResult::DANGEROUS));

  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::NOT_REACHED,
      TestSelectFileDialogFactory::SelectedFileInfoList());
  RunTestViaHTTP("FileChooser_SaveAsDangerousExtensionListDisallowed");
}

IN_PROC_BROWSER_TEST_F(PPAPIFileChooserTestWithSBService,
                       FileChooser_Open_NotBlockedBySafeBrowsing) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  const char kContents[] = "Hello from browser";
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::FilePath existing_filename = temp_dir.GetPath().AppendASCII("foo");
  ASSERT_EQ(
      static_cast<int>(sizeof(kContents) - 1),
      base::WriteFile(existing_filename, kContents, sizeof(kContents) - 1));

  safe_browsing_test_configuration_.default_result =
      safe_browsing::DownloadCheckResult::DANGEROUS;

  TestSelectFileDialogFactory::SelectedFileInfoList file_info_list;
  file_info_list.push_back(
      ui::SelectedFileInfo(existing_filename, existing_filename));
  TestSelectFileDialogFactory test_dialog_factory(
      TestSelectFileDialogFactory::RESPOND_WITH_FILE_LIST, file_info_list);
  RunTestViaHTTP("FileChooser_OpenSimple");
}

#endif  // FULL_SAFE_BROWSING
