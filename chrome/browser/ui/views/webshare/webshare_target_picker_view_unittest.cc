// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/webshare/webshare_target_picker_view.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/chrome_constrained_window_views_client.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/constrained_window/constrained_window_views.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/controls/table/table_view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"
#include "ui/views/window/dialog_delegate.h"
#include "url/gurl.h"

constexpr char kAppName1[] = "App One";
constexpr char kAppName2[] = "App Two";
constexpr char kTemplate[] = "share?title={title}";
constexpr char kUrl1[] = "https://appone.com/path/bits";
constexpr char kUrl2[] = "https://apptwo.xyz";

class WebShareTargetPickerViewTest : public ChromeViewsTestBase {
 public:
  WebShareTargetPickerViewTest() {}

  void SetUp() override {
    ChromeViewsTestBase::SetUp();

    SetConstrainedWindowViewsClient(CreateChromeConstrainedWindowViewsClient());

    // Create the parent widget.
    views::Widget::InitParams params =
        CreateParams(views::Widget::InitParams::TYPE_WINDOW);
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;

    parent_widget_.reset(new views::Widget());
    parent_widget_->Init(params);
    parent_widget_->Show();
  }

  void TearDown() override {
    if (view_)
      view_->GetWidget()->CloseNow();
    parent_widget_->CloseNow();
    quit_closure_ = base::Closure();
    constrained_window::SetConstrainedWindowViewsClient(nullptr);

    ChromeViewsTestBase::TearDown();
  }

 protected:
  // Creates the WebShareTargetPickerView (available as view()).
  void CreateView(std::vector<WebShareTarget> targets) {
    view_ = new WebShareTargetPickerView(
        std::move(targets),
        base::BindOnce(&WebShareTargetPickerViewTest::OnCallback,
                       base::Unretained(this)));
    constrained_window::CreateBrowserModalDialogViews(
        view_, parent_widget_->GetNativeWindow())
        ->Show();
  }

  // Sets the closure that will be called when the dialog is closed. This is
  // used in tests to quit the RunLoop.
  void SetQuitClosure(base::Closure&& quit_closure) {
    quit_closure_ = std::move(quit_closure);
  }

  // The view under test.
  WebShareTargetPickerView* view() { return view_; }
  // The table inside the view (for inspection).
  views::TableView* table() { return view_->table_; }

  // The result that was returned to the dialog's callback.
  const WebShareTarget* result() { return result_; }

 private:
  void OnCallback(const WebShareTarget* result) {
    result_ = result;
    if (quit_closure_)
      quit_closure_.Run();
  }

  std::unique_ptr<views::Widget> parent_widget_;
  WebShareTargetPickerView* view_ = nullptr;

  const WebShareTarget* result_;

  base::Closure quit_closure_;

  DISALLOW_COPY_AND_ASSIGN(WebShareTargetPickerViewTest);
};

// Table with 0 targets. Choose to cancel.
TEST_F(WebShareTargetPickerViewTest, EmptyListCancel) {
  CreateView(std::vector<WebShareTarget>());
  EXPECT_EQ(0, table()->RowCount());
  EXPECT_EQ(-1, table()->FirstSelectedRow());  // Nothing selected.
  EXPECT_FALSE(view()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));

  base::RunLoop run_loop;
  SetQuitClosure(run_loop.QuitClosure());

  view()->Cancel();

  run_loop.Run();

  EXPECT_EQ(nullptr, result());
}

// Table with 2 targets. Choose second target and share.
TEST_F(WebShareTargetPickerViewTest, ChooseItem) {
  std::vector<WebShareTarget> targets;
  targets.emplace_back(GURL(kUrl1), kAppName1, GURL(kTemplate));
  targets.emplace_back(GURL(kUrl2), kAppName2, GURL(kTemplate));

  CreateView(std::move(targets));
  EXPECT_EQ(2, table()->RowCount());
  EXPECT_EQ(base::ASCIIToUTF16("App One (https://appone.com/)"),
            table()->model()->GetText(0, 0));
  EXPECT_EQ(base::ASCIIToUTF16("App Two (https://apptwo.xyz/)"),
            table()->model()->GetText(1, 0));
  EXPECT_EQ(0, table()->FirstSelectedRow());
  EXPECT_TRUE(view()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));

  // Deselect and ensure OK button is disabled.
  table()->Select(-1);
  EXPECT_FALSE(view()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));

  // Select the second app and ensure the OK button is enabled.
  table()->Select(1);
  EXPECT_TRUE(view()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));

  base::RunLoop run_loop;
  SetQuitClosure(run_loop.QuitClosure());

  view()->Accept();

  run_loop.Run();

  EXPECT_EQ(WebShareTarget(GURL(kUrl2), kAppName2, GURL(kTemplate)), *result());
}

// Table with 1 target. Select using double-click.
TEST_F(WebShareTargetPickerViewTest, ChooseItemWithDoubleClick) {
  std::vector<WebShareTarget> targets;
  targets.emplace_back(GURL(kUrl1), kAppName1, GURL(kTemplate));

  CreateView(std::move(targets));
  EXPECT_EQ(1, table()->RowCount());
  EXPECT_EQ(base::ASCIIToUTF16("App One (https://appone.com/)"),
            table()->model()->GetText(0, 0));
  EXPECT_EQ(0, table()->FirstSelectedRow());
  EXPECT_TRUE(view()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));

  base::RunLoop run_loop;
  SetQuitClosure(run_loop.QuitClosure());

  view()->OnDoubleClick();

  run_loop.Run();

  EXPECT_EQ(WebShareTarget(GURL(kUrl1), kAppName1, GURL(kTemplate)), *result());
}

// Table with 1 target. Select, share and GetText.
TEST_F(WebShareTargetPickerViewTest, GetTextAfterAccept) {
  std::vector<WebShareTarget> targets;
  targets.emplace_back(GURL(kUrl1), kAppName1, GURL(kTemplate));

  CreateView(std::move(targets));
  EXPECT_EQ(1, table()->RowCount());
  EXPECT_EQ(base::ASCIIToUTF16("App One (https://appone.com/)"),
            table()->model()->GetText(0, 0));
  EXPECT_EQ(0, table()->FirstSelectedRow());
  EXPECT_TRUE(view()->IsDialogButtonEnabled(ui::DIALOG_BUTTON_OK));

  base::RunLoop run_loop;
  SetQuitClosure(run_loop.QuitClosure());

  view()->Accept();

  run_loop.Run();

  EXPECT_EQ(base::ASCIIToUTF16("App One (https://appone.com/)"),
            table()->model()->GetText(0, 0));

  EXPECT_EQ(WebShareTarget(GURL(kUrl1), kAppName1, GURL(kTemplate)), *result());
}
