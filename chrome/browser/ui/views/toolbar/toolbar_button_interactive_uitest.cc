// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/toolbar/toolbar_button.h"

#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/view_event_test_base.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace test {

// Friend of ToolbarButton to access private members.
class ToolbarButtonTestApi {
 public:
  explicit ToolbarButtonTestApi(ToolbarButton* button) : button_(button) {}

  views::MenuRunner* menu_runner() { return button_->menu_runner_.get(); }
  bool menu_showing() const { return button_->menu_showing_; }

 private:
  ToolbarButton* button_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarButtonTestApi);
};

}  // namespace test

class ToolbarButtonUITest : public ViewEventTestBase {
 public:
  ToolbarButtonUITest() {}

  // ViewEventTestBase:
  views::View* CreateContentsView() override {
    // Usually a BackForwardMenuModel is used, but that needs a Browser*. Make
    // something simple with at least one item so a menu gets shown. Note that
    // ToolbarButton takes ownership of the |model|.
    auto model = std::make_unique<ui::SimpleMenuModel>(nullptr);
    model->AddItem(0, base::string16());
    button_ = new ToolbarButton(&profile_, nullptr, std::move(model));
    return button_;
  }
  void DoTestOnMessageLoop() override {}

 protected:
  TestingProfile profile_;
  ToolbarButton* button_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(ToolbarButtonUITest);
};

// Test showing and dismissing a menu to verify menu delegate lifetime.
TEST_F(ToolbarButtonUITest, ShowMenu) {
  test::ToolbarButtonTestApi test_api(button_);

  EXPECT_FALSE(test_api.menu_showing());
  EXPECT_FALSE(test_api.menu_runner());
  EXPECT_EQ(views::Button::STATE_NORMAL, button_->state());

  // Show the menu. Note that it is asynchronous.
  button_->ShowContextMenuForView(nullptr, gfx::Point(), ui::MENU_SOURCE_MOUSE);

  EXPECT_TRUE(test_api.menu_showing());
  EXPECT_TRUE(test_api.menu_runner());
  EXPECT_TRUE(test_api.menu_runner()->IsRunning());

  // Button should appear pressed when the menu is showing.
  EXPECT_EQ(views::Button::STATE_PRESSED, button_->state());

  test_api.menu_runner()->Cancel();

  // Ensure the ToolbarButton's |menu_runner_| member is reset to null.
  EXPECT_FALSE(test_api.menu_showing());
  EXPECT_FALSE(test_api.menu_runner());
  EXPECT_EQ(views::Button::STATE_NORMAL, button_->state());
}

// Test deleting a ToolbarButton while its menu is showing.
TEST_F(ToolbarButtonUITest, DeleteWithMenu) {
  button_->ShowContextMenuForView(nullptr, gfx::Point(), ui::MENU_SOURCE_MOUSE);
  EXPECT_TRUE(test::ToolbarButtonTestApi(button_).menu_runner());
  delete button_;
}
