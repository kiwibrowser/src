// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LOGIN_EXPANDED_PUBLIC_ACCOUNT_VIEW_H_
#define ASH_LOGIN_UI_LOGIN_EXPANDED_PUBLIC_ACCOUNT_VIEW_H_

#include "ash/ash_export.h"
#include "ash/login/ui/non_accessible_view.h"
#include "ash/public/interfaces/login_user_info.mojom.h"
#include "ui/events/event_handler.h"
#include "ui/views/view.h"

namespace ash {

class ArrowButtonView;
class LoginUserView;
class RightPaneView;

// Implements an expanded view for the public acount user to select language
// and keyboard options.
class ASH_EXPORT LoginExpandedPublicAccountView : public NonAccessibleView {
 public:
  // TestApi is used for tests to get internal implementation details.
  class ASH_EXPORT TestApi {
   public:
    explicit TestApi(LoginExpandedPublicAccountView* view);
    ~TestApi();

    views::View* advanced_view_button();
    ArrowButtonView* submit_button();
    views::View* advanced_view();

   private:
    LoginExpandedPublicAccountView* const view_;
  };

  using OnPublicSessionViewDismissed = base::RepeatingClosure;
  explicit LoginExpandedPublicAccountView(
      const OnPublicSessionViewDismissed& on_dismissed);
  ~LoginExpandedPublicAccountView() override;

  void ProcessPressedEvent(const ui::LocatedEvent* event);
  void UpdateForUser(const mojom::LoginUserInfoPtr& user);
  const mojom::LoginUserInfoPtr& current_user() const;
  void Hide();

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;

  // ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  void OnKeyEvent(ui::KeyEvent* event) override;

 private:
  LoginUserView* user_view_ = nullptr;
  RightPaneView* right_pane_ = nullptr;
  OnPublicSessionViewDismissed on_dismissed_;

  DISALLOW_COPY_AND_ASSIGN(LoginExpandedPublicAccountView);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_LOGIN_EXPANDED_PUBLIC_ACCOUNT_VIEW_H_
