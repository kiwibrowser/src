// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_test_base.h"

#include <string>

#include "ash/login/ui/login_test_utils.h"
#include "ash/public/cpp/config.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/shell.h"
#include "base/strings/strcat.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

// A WidgetDelegate which ensures that |initially_focused| gets focus.
class LoginTestBase::WidgetDelegate : public views::WidgetDelegate {
 public:
  explicit WidgetDelegate(views::View* content) : content_(content) {}
  ~WidgetDelegate() override = default;

  // views::WidgetDelegate:
  void DeleteDelegate() override { delete this; }
  views::View* GetInitiallyFocusedView() override { return content_; }
  views::Widget* GetWidget() override { return content_->GetWidget(); }
  const views::Widget* GetWidget() const override {
    return content_->GetWidget();
  }

 private:
  views::View* content_;

  DISALLOW_COPY_AND_ASSIGN(WidgetDelegate);
};

LoginTestBase::LoginTestBase() = default;

LoginTestBase::~LoginTestBase() = default;

void LoginTestBase::SetWidget(std::unique_ptr<views::Widget> widget) {
  EXPECT_FALSE(widget_) << "SetWidget can only be called once.";
  widget_ = std::move(widget);
}

std::unique_ptr<views::Widget> LoginTestBase::CreateWidgetWithContent(
    views::View* content) {
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.bounds = gfx::Rect(0, 0, 800, 800);
  params.delegate = new WidgetDelegate(content);

  // Set the widget to the lock screen container, since a test may change the
  // session state to locked, which will hide all widgets not associated with
  // the lock screen.
  params.parent = Shell::GetContainer(Shell::GetPrimaryRootWindow(),
                                      kShellWindowId_LockScreenContainer);

  auto new_widget = std::make_unique<views::Widget>();
  new_widget->Init(params);
  new_widget->SetContentsView(content);
  new_widget->Show();
  return new_widget;
}

void LoginTestBase::SetUserCount(size_t count) {
  if (count > users_.size()) {
    AddUsers(count - users_.size());
    return;
  }

  users_.erase(users_.begin() + count, users_.end());
  // Notify any listeners that the user count has changed.
  data_dispatcher_.NotifyUsers(users_);
}

void LoginTestBase::AddUsers(size_t num_users) {
  for (size_t i = 0; i < num_users; i++) {
    std::string email =
        base::StrCat({"user", std::to_string(users_.size()), "@domain.com"});
    users_.push_back(CreateUser(email));
  }

  // Notify any listeners that the user count has changed.
  data_dispatcher_.NotifyUsers(users_);
}

void LoginTestBase::AddPublicAccountUsers(size_t num_public_accounts) {
  for (size_t i = 0; i < num_public_accounts; i++) {
    std::string email =
        base::StrCat({"user", std::to_string(users_.size()), "@domain.com"});
    users_.push_back(CreatePublicAccountUser(email));
  }

  // Notify any listeners that the user count has changed.
  data_dispatcher_.NotifyUsers(users_);
}

void LoginTestBase::TearDown() {
  widget_.reset();

  AshTestBase::TearDown();
}

}  // namespace ash
