// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/file_manager/file_manager_browsertest_base.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chromeos/chromeos_switches.h"
#include "components/session_manager/core/session_manager.h"
#include "components/user_manager/user_manager.h"
#include "services/identity/public/cpp/identity_manager.h"

namespace file_manager {

// TestCase: FilesAppBrowserTest parameters.
struct TestCase {
  explicit TestCase(const char* name)
    : test_name(name) {}

  const char* GetTestName() const {
    CHECK(test_name) << "FATAL: no test name";
    return test_name;
  }

  GuestMode GetGuestMode() const { return guest_mode; }

  TestCase& InGuestMode() {
    guest_mode = IN_GUEST_MODE;
    return *this;
  }

  TestCase& InIncognito() {
    guest_mode = IN_INCOGNITO;
    return *this;
  }

  bool GetTabletMode() const { return tablet_mode; }

  TestCase& TabletMode() {
    tablet_mode = true;
    return *this;
  }

  const char* test_name = nullptr;
  GuestMode guest_mode = NOT_IN_GUEST_MODE;
  bool tablet_mode = false;
};

// FilesApp browser test.
class FilesAppBrowserTest : public FileManagerBrowserTestBase,
                            public ::testing::WithParamInterface<TestCase> {
 public:
  FilesAppBrowserTest() = default;

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    FileManagerBrowserTestBase::SetUpCommandLine(command_line);

    // TODO(noel): describe this.
    if (ShouldEnableLegacyEventDispatch()) {
      command_line->AppendSwitchASCII("disable-blink-features",
                                      "TrustedEventsDefaultAction");
    }

    // Default mode is clamshell: force Ash into tablet mode if requested.
    if (GetParam().GetTabletMode()) {
      command_line->AppendSwitchASCII("force-tablet-mode", "touch_view");
    }
  }

  GuestMode GetGuestMode() const override {
    return GetParam().GetGuestMode();
  }

  const char* GetTestCaseName() const override {
    return GetParam().GetTestName();
  }

  const char* GetTestExtensionManifestName() const override {
    return "file_manager_test_manifest.json";
  }

 private:
  // TODO(noel): remove this and use a bool TestCase<> paramater instead.
  bool ShouldEnableLegacyEventDispatch() {
    const std::string test_case_name = GetTestCaseName();
    // crbug.com/482121 crbug.com/480491
    return test_case_name.find("tabindex") != std::string::npos;
  }

  DISALLOW_COPY_AND_ASSIGN(FilesAppBrowserTest);
};

IN_PROC_BROWSER_TEST_P(FilesAppBrowserTest, Test) {
  StartTest();
}

// INSTANTIATE_TEST_CASE_P expands to code that stringizes the arguments. Thus
// macro parameters such as |prefix| and |test_class| won't be expanded by the
// macro pre-processor. To work around this, indirect INSTANTIATE_TEST_CASE_P,
// as WRAPPED_INSTANTIATE_TEST_CASE_P here, so the pre-processor expands macro
// defines used to disable tests, MAYBE_prefix for example.
#define WRAPPED_INSTANTIATE_TEST_CASE_P(prefix, test_class, generator) \
  INSTANTIATE_TEST_CASE_P(prefix, test_class, generator, &PostTestCaseName)

std::string PostTestCaseName(const ::testing::TestParamInfo<TestCase>& test) {
  std::string name(test.param.GetTestName());

  const GuestMode mode = test.param.GetGuestMode();
  if (mode == IN_GUEST_MODE)
    name.append("_GuestMode");
  else if (mode == IN_INCOGNITO)
    name.append("_Incognito");

  if (test.param.GetTabletMode())
    name.append("_TabletMode");

  return name;
}

WRAPPED_INSTANTIATE_TEST_CASE_P(
    FileDisplay, /* file_display.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("fileDisplayDownloads"),
                      TestCase("fileDisplayDownloads").InGuestMode(),
                      TestCase("fileDisplayDownloads").TabletMode(),
                      TestCase("fileDisplayDrive"),
                      TestCase("fileDisplayDrive").TabletMode(),
                      TestCase("fileDisplayMtp"),
                      TestCase("fileDisplayUsb"),
                      TestCase("fileSearch"),
                      TestCase("fileSearchCaseInsensitive"),
                      TestCase("fileSearchNotFound")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    OpenVideoFiles, /* open_video_files.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("videoOpenDownloads").InGuestMode(),
                      TestCase("videoOpenDownloads"),
                      TestCase("videoOpenDrive")));

// TIMEOUT PASS on MSAN, https://crbug.com/836254
#if defined(MEMORY_SANITIZER)
#define MAYBE_OpenAudioFiles DISABLED_OpenAudioFiles
#else
#define MAYBE_OpenAudioFiles OpenAudioFiles
#endif
WRAPPED_INSTANTIATE_TEST_CASE_P(
    MAYBE_OpenAudioFiles, /* open_audio_files.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("audioOpenDownloads").InGuestMode(),
                      TestCase("audioOpenDownloads"),
                      TestCase("audioOpenDrive"),
                      TestCase("audioAutoAdvanceDrive"),
                      TestCase("audioRepeatAllModeSingleFileDrive"),
                      TestCase("audioNoRepeatModeSingleFileDrive"),
                      TestCase("audioRepeatOneModeSingleFileDrive"),
                      TestCase("audioRepeatAllModeMultipleFileDrive"),
                      TestCase("audioNoRepeatModeMultipleFileDrive"),
                      TestCase("audioRepeatOneModeMultipleFileDrive")));

// Fails on the MSAN bots, https://crbug.com/837551
#if defined(MEMORY_SANITIZER)
#define MAYBE_OpenImageFiles DISABLED_OpenImageFiles
#else
#define MAYBE_OpenImageFiles OpenImageFiles
#endif
WRAPPED_INSTANTIATE_TEST_CASE_P(
    MAYBE_OpenImageFiles, /* open_image_files.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("imageOpenDownloads").InGuestMode(),
                      TestCase("imageOpenDownloads"),
                      TestCase("imageOpenDrive")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    CreateNewFolder, /* create_new_folder.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("selectCreateFolderDownloads"),
                      TestCase("createFolderDownloads").InGuestMode(),
                      TestCase("createFolderDownloads"),
                      TestCase("createFolderDrive")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    KeyboardOperations, /* keyboard_operations.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("keyboardDeleteDownloads").InGuestMode(),
                      TestCase("keyboardDeleteDownloads"),
                      TestCase("keyboardDeleteDrive"),
                      TestCase("keyboardCopyDownloads").InGuestMode(),
                      TestCase("keyboardCopyDownloads"),
                      TestCase("keyboardCopyDrive"),
                      TestCase("renameFileDownloads").InGuestMode(),
                      TestCase("renameFileDownloads"),
                      TestCase("renameFileDrive"),
                      TestCase("renameNewFolderDownloads").InGuestMode(),
                      TestCase("renameNewFolderDownloads"),
                      TestCase("renameNewFolderDrive")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    Delete, /* delete.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("deleteMenuItemNoEntrySelected"),
                      TestCase("deleteEntryWithToolbar")));

WRAPPED_INSTANTIATE_TEST_CASE_P(QuickView, /* quick_view.js */
                                FilesAppBrowserTest,
                                ::testing::Values(TestCase("openQuickView"),
                                                  TestCase("closeQuickView")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    DirectoryTreeContextMenu, /* directory_tree_context_menu.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("dirCopyWithContextMenu"),
                      TestCase("dirCopyWithContextMenu").InGuestMode(),
                      TestCase("dirCopyWithKeyboard"),
                      TestCase("dirCopyWithKeyboard").InGuestMode(),
                      TestCase("dirCopyWithoutChangingCurrent"),
                      TestCase("dirCutWithContextMenu"),
                      TestCase("dirCutWithContextMenu").InGuestMode(),
                      TestCase("dirCutWithKeyboard"),
                      TestCase("dirCutWithKeyboard").InGuestMode(),
                      TestCase("dirPasteWithContextMenu"),
                      TestCase("dirPasteWithContextMenu").InGuestMode(),
                      TestCase("dirPasteWithoutChangingCurrent"),
                      TestCase("dirRenameWithContextMenu"),
                      TestCase("dirRenameWithContextMenu").InGuestMode(),
                      TestCase("dirRenameWithKeyboard"),
                      TestCase("dirRenameWithKeyboard").InGuestMode(),
                      TestCase("dirRenameWithoutChangingCurrent"),
                      TestCase("dirRenameToEmptyString"),
                      TestCase("dirRenameToEmptyString").InGuestMode(),
                      TestCase("dirRenameToExisting"),
                      TestCase("dirRenameToExisting").InGuestMode(),
                      TestCase("dirCreateWithContextMenu"),
                      TestCase("dirCreateWithKeyboard"),
                      TestCase("dirCreateWithoutChangingCurrent")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    DriveSpecific, /* drive_specific.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("driveOpenSidebarOffline"),
                      TestCase("driveOpenSidebarSharedWithMe"),
                      TestCase("driveAutoCompleteQuery"),
                      TestCase("drivePinFileMobileNetwork"),
                      TestCase("driveClickFirstSearchResult"),
                      TestCase("drivePressEnterToSearch")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    Transfer, /* transfer.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("transferFromDriveToDownloads"),
                      TestCase("transferFromDownloadsToDrive"),
                      TestCase("transferFromSharedToDownloads"),
                      TestCase("transferFromSharedToDrive"),
                      TestCase("transferFromOfflineToDownloads"),
                      TestCase("transferFromOfflineToDrive")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    RestorePrefs, /* restore_prefs.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("restoreSortColumn").InGuestMode(),
                      TestCase("restoreSortColumn"),
                      TestCase("restoreCurrentView").InGuestMode(),
                      TestCase("restoreCurrentView")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    RestoreGeometry, /* restore_geometry.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("restoreGeometry"),
                      TestCase("restoreGeometry").InGuestMode(),
                      TestCase("restoreGeometryMaximized")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    ShareAndManageDialog, /* share_and_manage_dialog.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("shareFileDrive"),
                      TestCase("shareDirectoryDrive"),
                      TestCase("manageHostedFileDrive"),
                      TestCase("manageFileDrive"),
                      TestCase("manageDirectoryDrive")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    SuggestAppDialog, /* suggest_app_dialog.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("suggestAppDialog")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    Traverse, /* traverse.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("traverseDownloads").InGuestMode(),
                      TestCase("traverseDownloads"),
                      TestCase("traverseDrive")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    Tasks, /* tasks.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("executeDefaultTaskDownloads"),
                      TestCase("executeDefaultTaskDownloads").InGuestMode(),
                      TestCase("executeDefaultTaskDrive"),
                      TestCase("defaultTaskDialogDownloads"),
                      TestCase("defaultTaskDialogDownloads").InGuestMode(),
                      TestCase("defaultTaskDialogDrive"),
                      TestCase("genericTaskIsNotExecuted"),
                      TestCase("genericTaskAndNonGenericTask")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    FolderShortcuts, /* folder_shortcuts.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("traverseFolderShortcuts"),
                      TestCase("addRemoveFolderShortcuts")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    SortColumns, /* sort_columns.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("sortColumns"),
                      TestCase("sortColumns").InGuestMode()));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    TabIndex, /* tab_index.js */
    FilesAppBrowserTest,
    ::testing::Values(
        TestCase("tabindexSearchBoxFocus"),
        TestCase("tabindexFocus"),
        TestCase("tabindexFocusDownloads"),
        TestCase("tabindexFocusDownloads").InGuestMode(),
        TestCase("tabindexFocusDirectorySelected"),
        TestCase("tabindexOpenDialogDrive"),
        TestCase("tabindexOpenDialogDownloads"),
        TestCase("tabindexOpenDialogDownloads").InGuestMode(),
        TestCase("tabindexSaveFileDialogDrive"),
        TestCase("tabindexSaveFileDialogDownloads"),
        TestCase("tabindexSaveFileDialogDownloads").InGuestMode()));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    FileDialog, /* file_dialog.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("openFileDialogUnload"),
                      TestCase("openFileDialogDownloads"),
                      TestCase("openFileDialogDownloads").InGuestMode(),
                      TestCase("openFileDialogDownloads").InIncognito(),
                      TestCase("openFileDialogCancelDownloads"),
                      TestCase("openFileDialogEscapeDownloads"),
                      TestCase("openFileDialogDrive"),
                      TestCase("openFileDialogDrive").InIncognito(),
                      TestCase("openFileDialogCancelDrive"),
                      TestCase("openFileDialogEscapeDrive")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    CopyBetweenWindows, /* copy_between_windows.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("copyBetweenWindowsLocalToDrive"),
                      TestCase("copyBetweenWindowsLocalToUsb"),
                      TestCase("copyBetweenWindowsUsbToDrive"),
                      TestCase("copyBetweenWindowsDriveToLocal"),
                      TestCase("copyBetweenWindowsDriveToUsb"),
                      TestCase("copyBetweenWindowsUsbToLocal")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    GridView, /* grid_view.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("showGridViewDownloads"),
                      TestCase("showGridViewDownloads").InGuestMode(),
                      TestCase("showGridViewDrive")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    Providers, /* providers.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("requestMount"),
                      TestCase("requestMountMultipleMounts"),
                      TestCase("requestMountSourceDevice"),
                      TestCase("requestMountSourceFile")));

WRAPPED_INSTANTIATE_TEST_CASE_P(
    GearMenu, /* gear_menu.js */
    FilesAppBrowserTest,
    ::testing::Values(TestCase("showHiddenFilesDownloads"),
                      TestCase("showHiddenFilesDrive"),
                      TestCase("toogleGoogleDocsDrive"),
                      TestCase("showPasteIntoCurrentFolder"),
                      TestCase("showSelectAllInCurrentFolder")));

// Structure to describe an account info.
struct TestAccountInfo {
  const char* const gaia_id;
  const char* const email;
  const char* const hash;
  const char* const display_name;
};

enum {
  DUMMY_ACCOUNT_INDEX = 0,
  PRIMARY_ACCOUNT_INDEX = 1,
  SECONDARY_ACCOUNT_INDEX_START = 2,
};

static const TestAccountInfo kTestAccounts[] = {
    {"gaia-id-d", "__dummy__@invalid.domain", "hashdummy", "Dummy Account"},
    {"gaia-id-a", "alice@invalid.domain", "hashalice", "Alice"},
    {"gaia-id-b", "bob@invalid.domain", "hashbob", "Bob"},
    {"gaia-id-c", "charlie@invalid.domain", "hashcharlie", "Charlie"},
};

// Test fixture class for testing multi-profile features.
class MultiProfileFileManagerBrowserTest : public FileManagerBrowserTestBase {
 public:
  MultiProfileFileManagerBrowserTest() = default;

 protected:
  // Enables multi-profiles.
  void SetUpCommandLine(base::CommandLine* command_line) override {
    FileManagerBrowserTestBase::SetUpCommandLine(command_line);
    // Logs in to a dummy profile (For making MultiProfileWindowManager happy;
    // browser test creates a default window and the manager tries to assign a
    // user for it, and we need a profile connected to a user.)
    command_line->AppendSwitchASCII(chromeos::switches::kLoginUser,
                                    kTestAccounts[DUMMY_ACCOUNT_INDEX].email);
    command_line->AppendSwitchASCII(chromeos::switches::kLoginProfile,
                                    kTestAccounts[DUMMY_ACCOUNT_INDEX].hash);
    // Don't require policy for our sessions - this is required because
    // this test creates a secondary profile synchronously, so we need to
    // let the policy code know not to expect cached policy.
    command_line->AppendSwitchASCII(chromeos::switches::kProfileRequiresPolicy,
                                    "false");
  }

  // Logs in to the primary profile of this test.
  void SetUpOnMainThread() override {
    const TestAccountInfo& info = kTestAccounts[PRIMARY_ACCOUNT_INDEX];

    AddUser(info, true);
    FileManagerBrowserTestBase::SetUpOnMainThread();
  }

  // Loads all users to the current session and sets up necessary fields.
  // This is used for preparing all accounts in PRE_ test setup, and for testing
  // actual login behavior.
  void AddAllUsers() {
    for (size_t i = 0; i < arraysize(kTestAccounts); ++i) {
      // The primary account was already set up in SetUpOnMainThread, so skip it
      // here.
      if (i == PRIMARY_ACCOUNT_INDEX)
        continue;
      AddUser(kTestAccounts[i], i >= SECONDARY_ACCOUNT_INDEX_START);
    }
  }

  // Returns primary profile (if it is already created.)
  Profile* profile() override {
    Profile* const profile =
        chromeos::ProfileHelper::GetProfileByUserIdHashForTest(
            kTestAccounts[PRIMARY_ACCOUNT_INDEX].hash);
    return profile ? profile : FileManagerBrowserTestBase::profile();
  }

  // Adds a new user for testing to the current session.
  void AddUser(const TestAccountInfo& info, bool log_in) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    const AccountId account_id(
        AccountId::FromUserEmailGaiaId(info.email, info.gaia_id));
    if (log_in) {
      session_manager::SessionManager::Get()->CreateSession(account_id,
                                                            info.hash, false);
    }
    user_manager::UserManager::Get()->SaveUserDisplayName(
        account_id, base::UTF8ToUTF16(info.display_name));
    Profile* profile =
        chromeos::ProfileHelper::GetProfileByUserIdHashForTest(info.hash);
    // TODO(https://crbug.com/814307): We can't use
    // identity::MakePrimaryAccountAvailable from identity_test_utils.h here
    // because that DCHECKs that the SigninManager isn't authenticated yet.
    // Here, it *can* be already authenticated if a PRE_ test previously set up
    // the user.
    IdentityManagerFactory::GetForProfile(profile)
        ->SetPrimaryAccountSynchronouslyForTests(info.gaia_id, info.email,
                                                 "refresh_token");
  }

  GuestMode GetGuestMode() const override { return NOT_IN_GUEST_MODE; }

  const char* GetTestCaseName() const override {
    return test_case_name_.c_str();
  }

  const char* GetTestExtensionManifestName() const override {
    return "file_manager_test_manifest.json";
  }

  void set_test_case_name(const std::string& name) { test_case_name_ = name; }

 private:
  std::string test_case_name_;

  DISALLOW_COPY_AND_ASSIGN(MultiProfileFileManagerBrowserTest);
};

IN_PROC_BROWSER_TEST_F(MultiProfileFileManagerBrowserTest, PRE_BasicDownloads) {
  AddAllUsers();
}

IN_PROC_BROWSER_TEST_F(MultiProfileFileManagerBrowserTest, BasicDownloads) {
  AddAllUsers();
  // Sanity check that normal operations work in multi-profile.
  set_test_case_name("keyboardCopyDownloads");
  StartTest();
}

IN_PROC_BROWSER_TEST_F(MultiProfileFileManagerBrowserTest, PRE_BasicDrive) {
  AddAllUsers();
}

IN_PROC_BROWSER_TEST_F(MultiProfileFileManagerBrowserTest, BasicDrive) {
  AddAllUsers();
  // Sanity check that normal operations work in multi-profile.
  set_test_case_name("keyboardCopyDrive");
  StartTest();
}

}  // namespace file_manager
