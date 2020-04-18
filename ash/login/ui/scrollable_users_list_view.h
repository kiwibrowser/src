// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_SCROLLABLE_USERS_LIST_VIEW_H_
#define ASH_LOGIN_UI_SCROLLABLE_USERS_LIST_VIEW_H_

#include <vector>

#include "ash/ash_export.h"
#include "ash/login/ui/login_display_style.h"
#include "ash/login/ui/login_user_view.h"
#include "ash/public/interfaces/login_user_info.mojom.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/scroll_view.h"

namespace views {
class View;
class BoxLayout;
}  // namespace views

namespace ash {

class HoverNotifier;
class ScrollBar;

// Scrollable list of the users. Stores the list of login user views. Can be
// styled with GradientParams that define gradient tinting at the top and at the
// bottom. Can be styled with LayoutParams that define spacing and sizing.
class ASH_EXPORT ScrollableUsersListView : public views::ScrollView {
 public:
  // TestApi is used for tests to get internal implementation details.
  class ASH_EXPORT TestApi {
   public:
    explicit TestApi(ScrollableUsersListView* view);
    ~TestApi();

    const std::vector<LoginUserView*>& user_views() const;

   private:
    ScrollableUsersListView* const view_;
  };

  // TODO(jdufault): Pass AccountId or LoginUserView* instead of index.
  using ActionWithUser = base::RepeatingCallback<void(int)>;

  // Initializes users list with rows for all |users|. The |display_style| is
  // used to determine layout and sizings. |on_user_view_tap| callback is
  // invoked whenever user row is tapped.
  ScrollableUsersListView(const std::vector<mojom::LoginUserInfoPtr>& users,
                          const ActionWithUser& on_tap_user,
                          LoginDisplayStyle display_style);
  ~ScrollableUsersListView() override;

  // Returns user view at |index| if it exists or nullptr otherwise.
  int user_count() const { return static_cast<int>(user_views_.size()); }
  LoginUserView* user_view_at(int index) {
    DCHECK_GE(index, 0);
    DCHECK_LT(index, user_count());
    return user_views_[index];
  }

  // Returns user view with |account_id| if it exists or nullptr otherwise.
  LoginUserView* GetUserView(const AccountId& account_id);

  // views::View:
  void Layout() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;

 private:
  struct GradientParams {
    // Start color for drawing linear gradient.
    SkColor color_from;
    // End color for drawing linear gradient.
    SkColor color_to;
    // Height of linear gradient.
    SkScalar height;
  };

  struct LayoutParams {
    // Display style to determine layout and sizing of users list.
    LoginDisplayStyle display_style;
    // Spacing between user entries on users list.
    int between_child_spacing;
    // Insets around users list used in landscape orientation.
    gfx::Insets insets_landscape;
    // Insets around users list used in portrait orientation.
    gfx::Insets insets_portrait;
  };

  // Updates visibility of scroll bar thumb. Called when hover state changes.
  void OnHover(bool has_hover);

  // Returns parameters of the layout for given display |style|.
  LayoutParams GetLayoutParams(LoginDisplayStyle style);
  // Returns parameters of the gradient for given display |style|.
  GradientParams GetGradientParams(LoginDisplayStyle style);

  views::BoxLayout* layout_ = nullptr;

  // Owned by ScrollView.
  ScrollBar* scroll_bar_ = nullptr;

  std::vector<LoginUserView*> user_views_;

  std::unique_ptr<HoverNotifier> hover_notifier_;

  GradientParams gradient_params_;
  LayoutParams layout_params_;

  DISALLOW_COPY_AND_ASSIGN(ScrollableUsersListView);
};

}  // namespace ash

#endif  // ASH_LOGIN_UI_SCROLLABLE_USERS_LIST_VIEW_H_
