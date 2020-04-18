// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/test/test_browser_dialog.h"

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_CHROMEOS)
#include "ash/public/cpp/config.h"
#include "ash/shell.h"  // mash-ok
#include "chrome/browser/chromeos/ash_config.h"
#endif

#if defined(OS_MACOSX)
#include "chrome/browser/ui/test/test_browser_dialog_mac.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "ui/views/test/widget_test.h"
#include "ui/views/widget/widget_observer.h"
#endif

namespace {

#if defined(TOOLKIT_VIEWS)
// Helper to return when a Widget has been closed.
// TODO(pkasting): This is pretty similar to views::test::WidgetClosingObserver
// in ui/views/test/widget_test.h but keys off widget destruction rather than
// closing.  Can the two be combined?
class WidgetCloseObserver : public views::WidgetObserver {
 public:
  explicit WidgetCloseObserver(views::Widget* widget) : widget_(widget) {
    widget->AddObserver(this);
  }

  // views::WidgetObserver:
  void OnWidgetDestroyed(views::Widget* widget) override {
    widget_->RemoveObserver(this);
    widget_ = nullptr;
    run_loop_.Quit();
  }

  void WaitForDestroy() { run_loop_.Run(); }

 protected:
  views::Widget* widget() { return widget_; }

 private:
  views::Widget* widget_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(WidgetCloseObserver);
};

// Helper to close a Widget.  Inherits from WidgetCloseObserver since regardless
// of whether the close is done synchronously, we always want callers to wait
// for it to complete.
class WidgetCloser : public WidgetCloseObserver {
 public:
  WidgetCloser(views::Widget* widget, bool async)
      : WidgetCloseObserver(widget) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&WidgetCloser::CloseWidget,
                                  weak_ptr_factory_.GetWeakPtr(), async));
  }

 private:
  void CloseWidget(bool async) {
    if (!widget())
      return;

    if (async)
      widget()->Close();
    else
      widget()->CloseNow();
  }

  base::WeakPtrFactory<WidgetCloser> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(WidgetCloser);
};
#endif  // defined(TOOLKIT_VIEWS)

}  // namespace

TestBrowserDialog::TestBrowserDialog() = default;
TestBrowserDialog::~TestBrowserDialog() = default;

void TestBrowserDialog::PreShow() {
// The rest of this class assumes the child dialog is toolkit-views. So, for
// Mac, it will only work when MD for secondary UI is enabled. Without this, a
// Cocoa dialog will be created, which TestBrowserDialog doesn't support.
// Force kSecondaryUiMd on Mac to get coverage on the bots. Leave it optional
// elsewhere so that the non-MD dialog can be invoked to compare.
#if defined(OS_MACOSX)
  // Note that since SetUp() has already been called, some parts of the toolkit
  // may already be initialized without MD - this is just to ensure Cocoa
  // dialogs are not selected.
  UseMdOnly();
#endif

  UpdateWidgets();
}

// This can return false if no dialog was shown, if the dialog shown wasn't a
// toolkit-views dialog, or if more than one child dialog was shown.
bool TestBrowserDialog::VerifyUi() {
#if defined(TOOLKIT_VIEWS)
  views::Widget::Widgets widgets_before = widgets_;
  UpdateWidgets();

  auto added =
      base::STLSetDifference<views::Widget::Widgets>(widgets_, widgets_before);

  if (added.size() > 1) {
    // Some tests create a standalone window to anchor a dialog. In those cases,
    // ignore added Widgets that are not dialogs.
    base::EraseIf(added, [](views::Widget* widget) {
      return !widget->widget_delegate()->AsDialogDelegate();
    });
  }
  widgets_ = added;

  return added.size() == 1;
#else
  NOTIMPLEMENTED();
  return false;
#endif
}

void TestBrowserDialog::WaitForUserDismissal() {
#if defined(OS_MACOSX)
  internal::TestBrowserDialogInteractiveSetUp();
#endif

#if defined(TOOLKIT_VIEWS)
  ASSERT_FALSE(widgets_.empty());
  WidgetCloseObserver observer(*widgets_.begin());
  observer.WaitForDestroy();
#else
  NOTIMPLEMENTED();
#endif
}

void TestBrowserDialog::DismissUi() {
#if defined(TOOLKIT_VIEWS)
  ASSERT_FALSE(widgets_.empty());
  WidgetCloser closer(*widgets_.begin(), AlwaysCloseAsynchronously());
  closer.WaitForDestroy();
#else
  NOTIMPLEMENTED();
#endif
}

bool TestBrowserDialog::AlwaysCloseAsynchronously() {
  // TODO(tapted): Iterate over close methods for greater test coverage.
  return false;
}

void TestBrowserDialog::UpdateWidgets() {
  widgets_.clear();
#if defined(OS_CHROMEOS)
  // Under mash, GetAllWidgets() uses MusClient to get the list of root windows.
  // Otherwise, GetAllWidgets() relies on AuraTestHelper to get the root window,
  // but that is not available in browser_tests, so use ash::Shell directly.
  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    widgets_ = views::test::WidgetTest::GetAllWidgets();
  } else {
    for (aura::Window* root_window : ash::Shell::GetAllRootWindows())
      views::Widget::GetAllChildWidgets(root_window, &widgets_);
  }
#elif defined(TOOLKIT_VIEWS)
  widgets_ = views::test::WidgetTest::GetAllWidgets();
#else
  NOTIMPLEMENTED();
#endif
}
