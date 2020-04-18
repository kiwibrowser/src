// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/select_file_dialog_extension.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"
#include "chrome/browser/chromeos/file_manager/volume_manager.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/test/test_utils.h"
#include "extensions/test/extension_test_message_listener.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/shell_dialogs/selected_file_info.h"

using content::BrowserContext;

// Mock listener used by test below.
class MockSelectFileDialogListener : public ui::SelectFileDialog::Listener {
 public:
  MockSelectFileDialogListener()
    : file_selected_(false),
      canceled_(false),
      params_(NULL) {
  }

  bool file_selected() const { return file_selected_; }
  bool canceled() const { return canceled_; }
  base::FilePath path() const { return path_; }
  void* params() const { return params_; }

  // ui::SelectFileDialog::Listener implementation.
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override {
    file_selected_ = true;
    path_ = path;
    params_ = params;
    QuitMessageLoop();
  }
  void FileSelectedWithExtraInfo(const ui::SelectedFileInfo& selected_file_info,
                                 int index,
                                 void* params) override {
    FileSelected(selected_file_info.local_path, index, params);
  }
  void MultiFilesSelected(const std::vector<base::FilePath>& files,
                          void* params) override {
    QuitMessageLoop();
  }
  void FileSelectionCanceled(void* params) override {
    canceled_ = true;
    params_ = params;
    QuitMessageLoop();
  }

  void WaitForCalled() {
    message_loop_runner_ = new content::MessageLoopRunner();
    message_loop_runner_->Run();
  }

 private:
  void QuitMessageLoop() {
    if (message_loop_runner_.get())
      message_loop_runner_->Quit();
  }

  bool file_selected_;
  bool canceled_;
  base::FilePath path_;
  void* params_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(MockSelectFileDialogListener);
};

class SelectFileDialogExtensionBrowserTest
    : public extensions::ExtensionBrowserTest {
 public:
  enum DialogButtonType {
    DIALOG_BTN_OK,
    DIALOG_BTN_CANCEL
  };

  void SetUp() override {
    extensions::ComponentLoader::EnableBackgroundExtensionsForTesting();

    // Create the dialog wrapper object, but don't show it yet.
    listener_.reset(new MockSelectFileDialogListener());
    dialog_ = new SelectFileDialogExtension(listener_.get(), NULL);

    // We have to provide at least one mount point.
    // File manager looks for "Downloads" mount point, so use this name.
    base::FilePath tmp_path;
    base::PathService::Get(base::DIR_TEMP, &tmp_path);
    ASSERT_TRUE(tmp_dir_.CreateUniqueTempDirUnderPath(tmp_path));
    downloads_dir_ = tmp_dir_.GetPath().Append("Downloads");
    base::CreateDirectory(downloads_dir_);

    // Must run after our setup because it actually runs the test.
    extensions::ExtensionBrowserTest::SetUp();
  }

  void TearDown() override {
    extensions::ExtensionBrowserTest::TearDown();

    // Delete the dialog first, as it holds a pointer to the listener.
    dialog_ = NULL;
    listener_.reset();

    second_dialog_ = NULL;
    second_listener_.reset();
  }

  // Creates a file system mount point for a directory.
  void AddMountPoint(const base::FilePath& path) {
    EXPECT_TRUE(file_manager::VolumeManager::Get(
        browser()->profile())->RegisterDownloadsDirectoryForTesting(path));
    browser()->profile()->GetPrefs()->SetFilePath(
        prefs::kDownloadDefaultDirectory, downloads_dir_);
  }

  void CheckJavascriptErrors() {
    content::RenderFrameHost* host =
        dialog_->GetRenderViewHost()->GetMainFrame();
    std::unique_ptr<base::Value> value =
        content::ExecuteScriptAndGetValue(host, "window.JSErrorCount");
    int js_error_count = 0;
    ASSERT_TRUE(value->GetAsInteger(&js_error_count));
    ASSERT_EQ(0, js_error_count);
  }

  void OpenDialog(ui::SelectFileDialog::Type dialog_type,
                  const base::FilePath& file_path,
                  const gfx::NativeWindow& owning_window,
                  const std::string& additional_message) {
    // Spawn a dialog to open a file.  The dialog will signal that it is ready
    // via chrome.test.sendMessage() in the extension JavaScript.
    ExtensionTestMessageListener init_listener("ready", false /* will_reply */);

    std::unique_ptr<ExtensionTestMessageListener> additional_listener;
    if (!additional_message.empty()) {
      additional_listener.reset(
          new ExtensionTestMessageListener(additional_message, false));
    }

    dialog_->SelectFile(dialog_type,
                        base::string16() /* title */,
                        file_path,
                        NULL /* file_types */,
                         0 /* file_type_index */,
                        FILE_PATH_LITERAL("") /* default_extension */,
                        owning_window,
                        this /* params */);

    LOG(INFO) << "Waiting for JavaScript ready message.";
    ASSERT_TRUE(init_listener.WaitUntilSatisfied());

    if (additional_listener.get()) {
      LOG(INFO) << "Waiting for JavaScript " << additional_message
                << " message.";
      ASSERT_TRUE(additional_listener->WaitUntilSatisfied());
    }

    // Dialog should be running now.
    ASSERT_TRUE(dialog_->IsRunning(owning_window));

    ASSERT_NO_FATAL_FAILURE(CheckJavascriptErrors());
  }

  void TryOpeningSecondDialog(const gfx::NativeWindow& owning_window) {
    second_listener_.reset(new MockSelectFileDialogListener());
    second_dialog_ = new SelectFileDialogExtension(second_listener_.get(),
                                                   NULL);

    // At the moment we don't really care about dialog type, but we have to put
    // some dialog type.
    second_dialog_->SelectFile(ui::SelectFileDialog::SELECT_OPEN_FILE,
                               base::string16() /* title */,
                               base::FilePath() /* default_path */,
                               NULL /* file_types */,
                               0 /* file_type_index */,
                               FILE_PATH_LITERAL("") /* default_extension */,
                               owning_window,
                               this /* params */);
  }

  void CloseDialog(DialogButtonType button_type,
                   const gfx::NativeWindow& owning_window) {
    // Inject JavaScript to click the cancel button and wait for notification
    // that the window has closed.
    content::RenderViewHost* host = dialog_->GetRenderViewHost();
    std::string button_class =
        (button_type == DIALOG_BTN_OK) ? ".button-panel .ok" :
                                         ".button-panel .cancel";
    base::string16 script = base::ASCIIToUTF16(
        "console.log(\'Test JavaScript injected.\');"
        "document.querySelector(\'" + button_class + "\').click();");
    // The file selection handler closes the dialog and does not return control
    // to JavaScript, so do not wait for return values.
    host->GetMainFrame()->ExecuteJavaScriptForTests(script);
    LOG(INFO) << "Waiting for window close notification.";
    listener_->WaitForCalled();

    // Dialog no longer believes it is running.
    ASSERT_FALSE(dialog_->IsRunning(owning_window));
  }

  std::unique_ptr<MockSelectFileDialogListener> listener_;
  scoped_refptr<SelectFileDialogExtension> dialog_;

  std::unique_ptr<MockSelectFileDialogListener> second_listener_;
  scoped_refptr<SelectFileDialogExtension> second_dialog_;

  base::ScopedTempDir tmp_dir_;
  base::FilePath downloads_dir_;
};

IN_PROC_BROWSER_TEST_F(SelectFileDialogExtensionBrowserTest, CreateAndDestroy) {
  // Browser window must be up for us to test dialog window parent.
  gfx::NativeWindow native_window = browser()->window()->GetNativeWindow();
  ASSERT_TRUE(native_window != NULL);

  // Before we call SelectFile, dialog is not running/visible.
  ASSERT_FALSE(dialog_->IsRunning(native_window));
}

IN_PROC_BROWSER_TEST_F(SelectFileDialogExtensionBrowserTest, DestroyListener) {
  // Some users of SelectFileDialog destroy their listener before cleaning
  // up the dialog.  Make sure we don't crash.
  dialog_->ListenerDestroyed();
  listener_.reset();
}

// TODO(jamescook): Add a test for selecting a file for an <input type='file'/>
// page element, as that uses different memory management pathways.
// crbug.com/98791

// Flaky on Chrome OS, see: http://crbug.com/477360
#if defined(OS_CHROMEOS)
#define MAYBE_SelectFileAndCancel DISABLED_SelectFileAndCancel
#else
#define MAYBE_SelectFileAndCancel SelectFileAndCancel
#endif
IN_PROC_BROWSER_TEST_F(SelectFileDialogExtensionBrowserTest,
                       MAYBE_SelectFileAndCancel) {
  AddMountPoint(downloads_dir_);

  gfx::NativeWindow owning_window = browser()->window()->GetNativeWindow();

  // base::FilePath() for default path.
  ASSERT_NO_FATAL_FAILURE(OpenDialog(ui::SelectFileDialog::SELECT_OPEN_FILE,
                                     base::FilePath(), owning_window, ""));

  // Press cancel button.
  CloseDialog(DIALOG_BTN_CANCEL, owning_window);

  // Listener should have been informed of the cancellation.
  ASSERT_FALSE(listener_->file_selected());
  ASSERT_TRUE(listener_->canceled());
  ASSERT_EQ(this, listener_->params());
}

// Flaky on Chrome OS, see: http://crbug.com/477360
#if defined(OS_CHROMEOS)
#define MAYBE_SelectFileAndOpen DISABLED_SelectFileAndOpen
#else
#define MAYBE_SelectFileAndOpen SelectFileAndOpen
#endif
IN_PROC_BROWSER_TEST_F(SelectFileDialogExtensionBrowserTest,
                       MAYBE_SelectFileAndOpen) {
  AddMountPoint(downloads_dir_);

  base::FilePath test_file =
      downloads_dir_.AppendASCII("file_manager_test.html");

  // Create an empty file to give us something to select.
  FILE* fp = base::OpenFile(test_file, "w");
  ASSERT_TRUE(fp != NULL);
  ASSERT_TRUE(base::CloseFile(fp));

  gfx::NativeWindow owning_window = browser()->window()->GetNativeWindow();

  // Spawn a dialog to open a file.  Provide the path to the file so the dialog
  // will automatically select it.  Ensure that the OK button is enabled by
  // waiting for chrome.test.sendMessage('selection-change-complete').
  ASSERT_NO_FATAL_FAILURE(OpenDialog(ui::SelectFileDialog::SELECT_OPEN_FILE,
                                     test_file, owning_window,
                                     "dialog-ready"));

  // Click open button.
  CloseDialog(DIALOG_BTN_OK, owning_window);

  // Listener should have been informed that the file was opened.
  ASSERT_TRUE(listener_->file_selected());
  ASSERT_FALSE(listener_->canceled());
  ASSERT_EQ(test_file, listener_->path());
  ASSERT_EQ(this, listener_->params());
}

// Flaky on Chrome OS, see: http://crbug.com/477360
#if defined(OS_CHROMEOS)
#define MAYBE_SelectFileAndSave DISABLED_SelectFileAndSave
#else
#define MAYBE_SelectFileAndSave SelectFileAndSave
#endif
IN_PROC_BROWSER_TEST_F(SelectFileDialogExtensionBrowserTest,
                       MAYBE_SelectFileAndSave) {
  AddMountPoint(downloads_dir_);

  base::FilePath test_file =
      downloads_dir_.AppendASCII("file_manager_test.html");

  gfx::NativeWindow owning_window = browser()->window()->GetNativeWindow();

  // Spawn a dialog to save a file, providing a suggested path.
  // Ensure "Save" button is enabled by waiting for notification from
  // chrome.test.sendMessage().
  ASSERT_NO_FATAL_FAILURE(OpenDialog(ui::SelectFileDialog::SELECT_SAVEAS_FILE,
                                     test_file, owning_window,
                                     "dialog-ready"));

  // Click save button.
  CloseDialog(DIALOG_BTN_OK, owning_window);

  // Listener should have been informed that the file was selected.
  ASSERT_TRUE(listener_->file_selected());
  ASSERT_FALSE(listener_->canceled());
  ASSERT_EQ(test_file, listener_->path());
  ASSERT_EQ(this, listener_->params());
}

// Flaky on Chrome OS, see: http://crbug.com/477360
#if defined(OS_CHROMEOS)
#define MAYBE_OpenSingletonTabAndCancel DISABLED_OpenSingletonTabAndCancel
#else
#define MAYBE_OpenSingletonTabAndCancel OpenSingletonTabAndCancel
#endif
IN_PROC_BROWSER_TEST_F(SelectFileDialogExtensionBrowserTest,
                       MAYBE_OpenSingletonTabAndCancel) {
  AddMountPoint(downloads_dir_);

  gfx::NativeWindow owning_window = browser()->window()->GetNativeWindow();

  ASSERT_NO_FATAL_FAILURE(OpenDialog(ui::SelectFileDialog::SELECT_OPEN_FILE,
                                     base::FilePath(), owning_window, ""));

  // Open a singleton tab in background.
  NavigateParams p(browser(), GURL("http://www.google.com"),
                   ui::PAGE_TRANSITION_LINK);
  p.window_action = NavigateParams::SHOW_WINDOW;
  p.disposition = WindowOpenDisposition::SINGLETON_TAB;
  Navigate(&p);

  // Press cancel button.
  CloseDialog(DIALOG_BTN_CANCEL, owning_window);

  // Listener should have been informed of the cancellation.
  ASSERT_FALSE(listener_->file_selected());
  ASSERT_TRUE(listener_->canceled());
  ASSERT_EQ(this, listener_->params());
}

// Flaky on Chrome OS, see: http://crbug.com/477360
#if defined(OS_CHROMEOS)
#define MAYBE_OpenTwoDialogs DISABLED_OpenTwoDialogs
#else
#define MAYBE_OpenTwoDialogs OpenTwoDialogs
#endif
IN_PROC_BROWSER_TEST_F(SelectFileDialogExtensionBrowserTest,
                       MAYBE_OpenTwoDialogs) {
  AddMountPoint(downloads_dir_);

  gfx::NativeWindow owning_window = browser()->window()->GetNativeWindow();

  ASSERT_NO_FATAL_FAILURE(OpenDialog(ui::SelectFileDialog::SELECT_OPEN_FILE,
                                     base::FilePath(), owning_window, ""));

  TryOpeningSecondDialog(owning_window);

  // Second dialog should not be running.
  ASSERT_FALSE(second_dialog_->IsRunning(owning_window));

  // Click cancel button.
  CloseDialog(DIALOG_BTN_CANCEL, owning_window);

  // Listener should have been informed of the cancellation.
  ASSERT_FALSE(listener_->file_selected());
  ASSERT_TRUE(listener_->canceled());
  ASSERT_EQ(this, listener_->params());
}
