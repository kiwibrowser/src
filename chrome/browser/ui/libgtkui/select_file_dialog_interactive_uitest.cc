// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/libgtkui/select_file_dialog_impl_gtk.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/widget/widget.h"

using BrowserSelectFileDialogTest = InProcessBrowserTest;

namespace libgtkui {

// FilePicker opens a GtkFileChooser.
class FilePicker : public ui::SelectFileDialog::Listener {
 public:
  explicit FilePicker(BrowserWindow* window) {
    select_file_dialog_ = ui::SelectFileDialog::Create(
        this, std::make_unique<ChromeSelectFilePolicy>(nullptr));

    gfx::NativeWindow parent_window = window->GetNativeWindow();
    ui::SelectFileDialog::FileTypeInfo file_types;
    file_types.allowed_paths = ui::SelectFileDialog::FileTypeInfo::ANY_PATH;
    const base::FilePath file_path;
    select_file_dialog_->SelectFile(ui::SelectFileDialog::SELECT_OPEN_FILE,
                                    base::string16(),
                                    file_path,
                                    &file_types,
                                    0,
                                    base::FilePath::StringType(),
                                    parent_window,
                                    nullptr);
  }

  ~FilePicker() override {
    select_file_dialog_->ListenerDestroyed();
  }

  void Close() {
    SelectFileDialogImplGTK* file_dialog =
        static_cast<SelectFileDialogImplGTK*>(select_file_dialog_.get());


    while (!file_dialog->dialogs_.empty())
      gtk_widget_destroy(*(file_dialog->dialogs_.begin()));
  }

  // SelectFileDialog::Listener implementation.
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override {}
 private:
  // Dialog box used for opening and saving files.
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;

  DISALLOW_COPY_AND_ASSIGN(FilePicker);
};

}  // namespace libgtkui

// Leaks in GtkFileChooserDialog. http://crbug.com/537468
// Flaky on Linux. http://crbug.com/700134
#if defined(ADDRESS_SANITIZER) || defined(OS_LINUX)
#define MAYBE_ModalTest DISABLED_ModalTest
#else
#define MAYBE_ModalTest ModalTest
#endif
// Test that the file-picker is modal.
IN_PROC_BROWSER_TEST_F(BrowserSelectFileDialogTest, MAYBE_ModalTest) {
  // Bring the native window to the foreground. Returns true on success.
  ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));
  ASSERT_TRUE(browser()->window()->IsActive());

  libgtkui::FilePicker file_picker(browser()->window());

  gfx::NativeWindow window = browser()->window()->GetNativeWindow();
  views::Widget* widget = views::Widget::GetWidgetForNativeWindow(window);
  ASSERT_NE(nullptr, widget);

  // Run a nested loop until the browser window becomes inactive
  // so that the file-picker can be active.
  views::test::WidgetActivationWaiter waiter_inactive(widget, false);
  waiter_inactive.Wait();
  EXPECT_FALSE(browser()->window()->IsActive());

  ui_test_utils::ClickOnView(browser(), VIEW_ID_TAB_CONTAINER);
  // The window should not get focus due to modal dialog.
  EXPECT_FALSE(browser()->window()->IsActive());

  ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));
  EXPECT_FALSE(browser()->window()->IsActive());

  ASSERT_TRUE(ui_test_utils::ShowAndFocusNativeWindow(window));
  EXPECT_FALSE(browser()->window()->IsActive());

  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      browser(), ui::VKEY_TAB, false, false, true, false));
  EXPECT_FALSE(browser()->window()->IsActive());

  file_picker.Close();

  // Run a nested loop until the browser window becomes active.
  views::test::WidgetActivationWaiter wait_active(widget, true);
  wait_active.Wait();

  ui_test_utils::ClickOnView(browser(), VIEW_ID_TAB_CONTAINER);
  EXPECT_TRUE(browser()->window()->IsActive());
}
