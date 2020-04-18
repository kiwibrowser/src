// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "ash/detachable_base/detachable_base_pairing_status.h"
#include "ash/login/mock_login_screen_client.h"
#include "ash/login/ui/arrow_button_view.h"
#include "ash/login/ui/fake_login_detachable_base_model.h"
#include "ash/login/ui/lock_contents_view.h"
#include "ash/login/ui/lock_screen.h"
#include "ash/login/ui/login_auth_user_view.h"
#include "ash/login/ui/login_big_user_view.h"
#include "ash/login/ui/login_bubble.h"
#include "ash/login/ui/login_display_style.h"
#include "ash/login/ui/login_expanded_public_account_view.h"
#include "ash/login/ui/login_keyboard_test_base.h"
#include "ash/login/ui/login_pin_view.h"
#include "ash/login/ui/login_public_account_user_view.h"
#include "ash/login/ui/login_test_base.h"
#include "ash/login/ui/login_user_view.h"
#include "ash/login/ui/scrollable_users_list_view.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/shell.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget.h"

using ::testing::_;

namespace ash {

using LockContentsViewUnitTest = LoginTestBase;
using LockContentsViewKeyboardUnitTest = LoginKeyboardTestBase;

TEST_F(LockContentsViewUnitTest, DisplayMode) {
  // Build lock screen with 1 user.
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(1);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  // Verify user list and secondary auth are not shown for one user.
  LockContentsView::TestApi lock_contents(contents);
  EXPECT_EQ(nullptr, lock_contents.users_list());
  EXPECT_FALSE(lock_contents.opt_secondary_big_view());

  // Verify user list is not shown for two users, but secondary auth is.
  SetUserCount(2);
  EXPECT_EQ(nullptr, lock_contents.users_list());
  EXPECT_TRUE(lock_contents.opt_secondary_big_view());

  // Verify user names and pod style is set correctly for 3-25 users. This also
  // sanity checks that LockContentsView can respond to a multiple user change
  // events fired from the data dispatcher, which is needed for the debug UI.
  for (size_t user_count = 3; user_count < 25; ++user_count) {
    SetUserCount(user_count);
    ScrollableUsersListView::TestApi users_list(lock_contents.users_list());
    EXPECT_EQ(user_count - 1, users_list.user_views().size());

    // 1 extra user gets large style.
    LoginDisplayStyle expected_style = LoginDisplayStyle::kLarge;
    // 2-6 extra users get small style.
    if (user_count >= 3)
      expected_style = LoginDisplayStyle::kSmall;
    // 7+ users get get extra small style.
    if (user_count >= 7)
      expected_style = LoginDisplayStyle::kExtraSmall;

    for (size_t i = 0; i < users_list.user_views().size(); ++i) {
      LoginUserView::TestApi user_test_api(users_list.user_views()[i]);
      EXPECT_EQ(expected_style, user_test_api.display_style());

      const mojom::LoginUserInfoPtr& user = users()[i + 1];
      EXPECT_EQ(base::UTF8ToUTF16(user->basic_user_info->display_name),
                user_test_api.displayed_name());
    }
  }
}

// Verifies that the single user view is centered.
TEST_F(LockContentsViewUnitTest, SingleUserCentered) {
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(1);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  LockContentsView::TestApi test_api(contents);
  LoginBigUserView* auth_view = test_api.primary_big_view();
  gfx::Rect widget_bounds = widget->GetWindowBoundsInScreen();
  int expected_margin =
      (widget_bounds.width() - auth_view->GetPreferredSize().width()) / 2;
  gfx::Rect auth_bounds = auth_view->GetBoundsInScreen();

  EXPECT_NE(0, expected_margin);
  EXPECT_EQ(expected_margin, auth_bounds.x());
  EXPECT_EQ(expected_margin,
            widget_bounds.width() - (auth_bounds.x() + auth_bounds.width()));
}

// Verifies that the single user view is centered when lock screen notes are
// enabled.
TEST_F(LockContentsViewUnitTest, SingleUserCenteredNoteActionEnabled) {
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(1);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  LockContentsView::TestApi test_api(contents);
  LoginBigUserView* auth_view = test_api.primary_big_view();
  gfx::Rect widget_bounds = widget->GetWindowBoundsInScreen();
  int expected_margin =
      (widget_bounds.width() - auth_view->GetPreferredSize().width()) / 2;
  gfx::Rect auth_bounds = auth_view->GetBoundsInScreen();

  EXPECT_NE(0, expected_margin);
  EXPECT_EQ(expected_margin, auth_bounds.x());
  EXPECT_EQ(expected_margin,
            widget_bounds.width() - (auth_bounds.x() + auth_bounds.width()));
}

// Verifies that layout dynamically updates after a rotation by checking the
// distance between the auth user and the user list in landscape and portrait
// mode.
TEST_F(LockContentsViewUnitTest, AutoLayoutAfterRotation) {
  // Build lock screen with three users.
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  LockContentsView::TestApi lock_contents(contents);
  SetUserCount(3);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  // Returns the distance between the auth user view and the user view.
  auto calculate_distance = [&]() {
    if (lock_contents.opt_secondary_big_view()) {
      return lock_contents.opt_secondary_big_view()->GetBoundsInScreen().x() -
             lock_contents.primary_big_view()->GetBoundsInScreen().x();
    }
    ScrollableUsersListView::TestApi users_list(lock_contents.users_list());
    return users_list.user_views()[0]->GetBoundsInScreen().x() -
           lock_contents.primary_big_view()->GetBoundsInScreen().x();
  };

  const display::Display& display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(
          widget->GetNativeWindow());
  for (int i = 2; i < 10; ++i) {
    SetUserCount(i);

    // Start at 0 degrees (landscape).
    display_manager()->SetDisplayRotation(
        display.id(), display::Display::ROTATE_0,
        display::Display::RotationSource::ACTIVE);
    int distance_0deg = calculate_distance();
    EXPECT_NE(distance_0deg, 0);

    // Rotate the display to 90 degrees (portrait).
    display_manager()->SetDisplayRotation(
        display.id(), display::Display::ROTATE_90,
        display::Display::RotationSource::ACTIVE);
    int distance_90deg = calculate_distance();
    EXPECT_GT(distance_0deg, distance_90deg);

    // Rotate the display back to 0 degrees (landscape).
    display_manager()->SetDisplayRotation(
        display.id(), display::Display::ROTATE_0,
        display::Display::RotationSource::ACTIVE);
    int distance_180deg = calculate_distance();
    EXPECT_EQ(distance_0deg, distance_180deg);
    EXPECT_NE(distance_0deg, distance_90deg);
  }
}

TEST_F(LockContentsViewUnitTest, AutoLayoutExtraSmallUsersListAfterRotation) {
  // Build lock screen with extra small layout (> 6 users).
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(9);
  ScrollableUsersListView* users_list =
      LockContentsView::TestApi(contents).users_list();
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  // Users list in extra small layout should adjust its height to parent.
  EXPECT_EQ(contents->height(), users_list->height());

  const display::Display& display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(
          widget->GetNativeWindow());

  // Start at 0 degrees (landscape).
  display_manager()->SetDisplayRotation(
      display.id(), display::Display::ROTATE_0,
      display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(contents->height(), users_list->height());

  // Rotate the display to 90 degrees (portrait).
  display_manager()->SetDisplayRotation(
      display.id(), display::Display::ROTATE_90,
      display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(contents->height(), users_list->height());

  // Rotate the display back to 0 degrees (landscape).
  display_manager()->SetDisplayRotation(
      display.id(), display::Display::ROTATE_0,
      display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(contents->height(), users_list->height());
}

TEST_F(LockContentsViewUnitTest, AutoLayoutSmallUsersListAfterRotation) {
  // Build lock screen with small layout (3-6 users).
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(4);
  ScrollableUsersListView* users_list =
      LockContentsView::TestApi(contents).users_list();
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  // Calculate top spacing between users list and lock screen contents.
  auto top_margin = [&]() {
    return users_list->GetBoundsInScreen().y() -
           contents->GetBoundsInScreen().y();
  };

  // Calculate bottom spacing between users list and lock screen contents.
  auto bottom_margin = [&]() {
    return contents->GetBoundsInScreen().bottom() -
           users_list->GetBoundsInScreen().bottom();
  };

  // Users list in small layout should adjust its height to content and be
  // vertical centered in parent.
  EXPECT_EQ(top_margin(), bottom_margin());
  EXPECT_EQ(users_list->height(), users_list->contents()->height());

  const display::Display& display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(
          widget->GetNativeWindow());

  // Start at 0 degrees (landscape).
  display_manager()->SetDisplayRotation(
      display.id(), display::Display::ROTATE_0,
      display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(top_margin(), bottom_margin());
  EXPECT_EQ(users_list->height(), users_list->contents()->height());

  // Rotate the display to 90 degrees (portrait).
  display_manager()->SetDisplayRotation(
      display.id(), display::Display::ROTATE_90,
      display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(top_margin(), bottom_margin());
  EXPECT_EQ(users_list->height(), users_list->contents()->height());

  // Rotate the display back to 0 degrees (landscape).
  display_manager()->SetDisplayRotation(
      display.id(), display::Display::ROTATE_0,
      display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(top_margin(), bottom_margin());
  EXPECT_EQ(users_list->height(), users_list->contents()->height());
}

TEST_F(LockContentsViewKeyboardUnitTest,
       AutoLayoutExtraSmallUsersListForKeyboard) {
  // Build lock screen with extra small layout (> 6 users).
  ASSERT_NO_FATAL_FAILURE(ShowLoginScreen());
  LockContentsView* contents =
      LockScreen::TestApi(LockScreen::Get()).contents_view();
  ASSERT_NE(nullptr, contents);
  LoadUsers(9);

  // Users list in extra small layout should adjust its height to parent.
  ScrollableUsersListView* users_list =
      LockContentsView::TestApi(contents).users_list();
  EXPECT_EQ(contents->height(), users_list->height());

  ASSERT_NO_FATAL_FAILURE(ShowKeyboard());
  gfx::Rect keyboard_bounds = GetKeyboardBoundsInScreen();
  EXPECT_FALSE(users_list->GetBoundsInScreen().Intersects(keyboard_bounds));
  EXPECT_EQ(contents->height(), users_list->height());

  ASSERT_NO_FATAL_FAILURE(HideKeyboard());
  EXPECT_EQ(contents->height(), users_list->height());
}

TEST_F(LockContentsViewKeyboardUnitTest, AutoLayoutSmallUsersListForKeyboard) {
  // Build lock screen with small layout (3-6 users).
  ASSERT_NO_FATAL_FAILURE(ShowLoginScreen());
  LockContentsView* contents =
      LockScreen::TestApi(LockScreen::Get()).contents_view();
  ASSERT_NE(nullptr, contents);
  LoadUsers(4);
  ScrollableUsersListView* users_list =
      LockContentsView::TestApi(contents).users_list();

  // Calculate top spacing between users list and lock screen contents.
  auto top_margin = [&]() {
    return users_list->GetBoundsInScreen().y() -
           contents->GetBoundsInScreen().y();
  };

  // Calculate bottom spacing between users list and lock screen contents.
  auto bottom_margin = [&]() {
    return contents->GetBoundsInScreen().bottom() -
           users_list->GetBoundsInScreen().bottom();
  };

  // Users list in small layout should adjust its height to content and be
  // vertical centered in parent.
  EXPECT_EQ(top_margin(), bottom_margin());
  EXPECT_EQ(users_list->height(), users_list->contents()->height());

  ASSERT_NO_FATAL_FAILURE(ShowKeyboard());
  gfx::Rect keyboard_bounds = GetKeyboardBoundsInScreen();
  EXPECT_FALSE(users_list->GetBoundsInScreen().Intersects(keyboard_bounds));
  EXPECT_EQ(top_margin(), bottom_margin());

  ASSERT_NO_FATAL_FAILURE(HideKeyboard());
  EXPECT_EQ(top_margin(), bottom_margin());
  EXPECT_EQ(users_list->height(), users_list->contents()->height());
}

// Ensures that when swapping between two users, only auth method display swaps.
TEST_F(LockContentsViewUnitTest, SwapAuthUsersInTwoUserLayout) {
  // Build lock screen with two users.
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  LockContentsView::TestApi test_api(contents);
  SetUserCount(2);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  // Capture user info to validate it did not change during the swap.
  AccountId primary_user = test_api.primary_big_view()
                               ->GetCurrentUser()
                               ->basic_user_info->account_id;
  AccountId secondary_user = test_api.opt_secondary_big_view()
                                 ->GetCurrentUser()
                                 ->basic_user_info->account_id;
  EXPECT_NE(primary_user, secondary_user);

  // Primary user starts with auth. Secondary user does not have any auth.
  EXPECT_TRUE(test_api.primary_big_view()->IsAuthEnabled());
  EXPECT_FALSE(test_api.opt_secondary_big_view()->IsAuthEnabled());
  ASSERT_NE(nullptr, test_api.opt_secondary_big_view()->auth_user());

  // Send event to swap users.
  ui::test::EventGenerator& generator = GetEventGenerator();
  LoginAuthUserView::TestApi secondary_test_api(
      test_api.opt_secondary_big_view()->auth_user());
  generator.MoveMouseTo(
      secondary_test_api.user_view()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();

  // User info is not swapped.
  EXPECT_EQ(primary_user, test_api.primary_big_view()
                              ->GetCurrentUser()
                              ->basic_user_info->account_id);
  EXPECT_EQ(secondary_user, test_api.opt_secondary_big_view()
                                ->GetCurrentUser()
                                ->basic_user_info->account_id);

  // Active auth user (ie, which user is showing password) is swapped.
  EXPECT_FALSE(test_api.primary_big_view()->IsAuthEnabled());
  EXPECT_TRUE(test_api.opt_secondary_big_view()->IsAuthEnabled());
}

// Ensures that when swapping from a user list, the entire user info is swapped.
TEST_F(LockContentsViewUnitTest, SwapUserListToPrimaryAuthUser) {
  // Build lock screen with five users.
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  LockContentsView::TestApi lock_contents(contents);
  SetUserCount(5);
  ScrollableUsersListView::TestApi users_list(lock_contents.users_list());
  EXPECT_EQ(users().size() - 1, users_list.user_views().size());
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  LoginBigUserView* auth_view = lock_contents.primary_big_view();

  for (const LoginUserView* const list_user_view : users_list.user_views()) {
    // Capture user info to validate it did not change during the swap.
    AccountId auth_id =
        auth_view->GetCurrentUser()->basic_user_info->account_id;
    AccountId list_user_id =
        list_user_view->current_user()->basic_user_info->account_id;
    EXPECT_NE(auth_id, list_user_id);

    // Send event to swap users.
    ui::test::EventGenerator& generator = GetEventGenerator();
    generator.MoveMouseTo(list_user_view->GetBoundsInScreen().CenterPoint());
    generator.ClickLeftButton();

    // User info is swapped.
    EXPECT_EQ(list_user_id,
              auth_view->GetCurrentUser()->basic_user_info->account_id);
    EXPECT_EQ(auth_id,
              list_user_view->current_user()->basic_user_info->account_id);

    // Validate that every user is still unique.
    std::unordered_set<std::string> emails;
    for (const LoginUserView* const view : users_list.user_views()) {
      std::string email =
          view->current_user()->basic_user_info->account_id.GetUserEmail();
      EXPECT_TRUE(emails.insert(email).second);
    }
  }
}

// Test goes through different lock screen note state changes and tests that
// the note action visibility is updated accordingly.
TEST_F(LockContentsViewUnitTest, NoteActionButtonVisibilityChanges) {
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(1);
  SetWidget(CreateWidgetWithContent(contents));

  LockContentsView::TestApi test_api(contents);
  views::View* note_action_button = test_api.note_action();

  // In kAvailable state, the note action button should be visible.
  EXPECT_TRUE(note_action_button->visible());

  // In kLaunching state, the note action button should not be visible.
  data_dispatcher()->SetLockScreenNoteState(mojom::TrayActionState::kLaunching);
  EXPECT_FALSE(note_action_button->visible());

  // In kActive state, the note action button should not be visible.
  data_dispatcher()->SetLockScreenNoteState(mojom::TrayActionState::kActive);
  EXPECT_FALSE(note_action_button->visible());

  // When moved back to kAvailable state, the note action button should become
  // visible again.
  data_dispatcher()->SetLockScreenNoteState(mojom::TrayActionState::kAvailable);
  EXPECT_TRUE(note_action_button->visible());

  // In kNotAvailable state, the note action button should not be visible.
  data_dispatcher()->SetLockScreenNoteState(
      mojom::TrayActionState::kNotAvailable);
  EXPECT_FALSE(note_action_button->visible());
}

// Verifies note action view bounds.
TEST_F(LockContentsViewUnitTest, NoteActionButtonBounds) {
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(1);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  LockContentsView::TestApi test_api(contents);

  // The note action button should not be visible if the note action is not
  // available.
  EXPECT_FALSE(test_api.note_action()->visible());

  // When the note action becomes available, the note action button should be
  // shown.
  data_dispatcher()->SetLockScreenNoteState(mojom::TrayActionState::kAvailable);
  EXPECT_TRUE(test_api.note_action()->visible());

  // Verify the bounds of the note action button are as expected.
  gfx::Rect widget_bounds = widget->GetWindowBoundsInScreen();
  gfx::Size note_action_size = test_api.note_action()->GetPreferredSize();
  EXPECT_EQ(gfx::Rect(widget_bounds.top_right() -
                          gfx::Vector2d(note_action_size.width(), 0),
                      note_action_size),
            test_api.note_action()->GetBoundsInScreen());

  // If the note action is disabled again, the note action button should be
  // hidden.
  data_dispatcher()->SetLockScreenNoteState(
      mojom::TrayActionState::kNotAvailable);
  EXPECT_FALSE(test_api.note_action()->visible());
}

// Verifies the note action view bounds when note action is available at lock
// contents view creation.
TEST_F(LockContentsViewUnitTest, NoteActionButtonBoundsInitiallyAvailable) {
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(1);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  LockContentsView::TestApi test_api(contents);

  // Verify the note action button is visible and positioned in the top right
  // corner of the screen.
  EXPECT_TRUE(test_api.note_action()->visible());
  gfx::Rect widget_bounds = widget->GetWindowBoundsInScreen();
  gfx::Size note_action_size = test_api.note_action()->GetPreferredSize();
  EXPECT_EQ(gfx::Rect(widget_bounds.top_right() -
                          gfx::Vector2d(note_action_size.width(), 0),
                      note_action_size),
            test_api.note_action()->GetBoundsInScreen());

  // If the note action is disabled, the note action button should be hidden.
  data_dispatcher()->SetLockScreenNoteState(
      mojom::TrayActionState::kNotAvailable);
  EXPECT_FALSE(test_api.note_action()->visible());
}

// Verifies the dev channel info view bounds.
TEST_F(LockContentsViewUnitTest, DevChannelInfoViewBounds) {
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(1);

  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);
  gfx::Rect widget_bounds = widget->GetWindowBoundsInScreen();
  LockContentsView::TestApi test_api(contents);
  // Verify that the dev channel info view is hidden by default.
  EXPECT_FALSE(test_api.dev_channel_info()->visible());

  // Verify that the dev channel info view becomes visible and it doesn't block
  // the note action button.
  data_dispatcher()->SetDevChannelInfo("Best version ever", "Asset ID: 6666",
                                       "Bluetooth adapter");
  EXPECT_TRUE(test_api.dev_channel_info()->visible());
  EXPECT_TRUE(test_api.note_action()->visible());
  gfx::Size note_action_size = test_api.note_action()->GetPreferredSize();
  EXPECT_GE(widget_bounds.right() -
                test_api.dev_channel_info()->GetBoundsInScreen().right(),
            note_action_size.width());

  // Verify that if the note action is disabled, the dev channel info view moves
  // to the right to fill the empty space.
  data_dispatcher()->SetLockScreenNoteState(
      mojom::TrayActionState::kNotAvailable);
  EXPECT_FALSE(test_api.note_action()->visible());
  EXPECT_LT(widget_bounds.right() -
                test_api.dev_channel_info()->GetBoundsInScreen().right(),
            note_action_size.width());
}

// Verifies the easy unlock tooltip is automatically displayed when requested.
TEST_F(LockContentsViewUnitTest, EasyUnlockForceTooltipCreatesTooltipWidget) {
  auto* lock = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));

  SetUserCount(1);
  SetWidget(CreateWidgetWithContent(lock));

  LockContentsView::TestApi test_api(lock);
  // Creating lock screen does not show tooltip bubble.
  EXPECT_FALSE(test_api.tooltip_bubble()->IsVisible());

  // Show an icon with |autoshow_tooltip| is false. Tooltip bubble is not
  // activated.
  auto icon = mojom::EasyUnlockIconOptions::New();
  icon->icon = mojom::EasyUnlockIconId::LOCKED;
  icon->autoshow_tooltip = false;
  data_dispatcher()->ShowEasyUnlockIcon(users()[0]->basic_user_info->account_id,
                                        icon);
  EXPECT_FALSE(test_api.tooltip_bubble()->IsVisible());

  // Show icon with |autoshow_tooltip| set to true. Tooltip bubble is shown.
  icon->autoshow_tooltip = true;
  data_dispatcher()->ShowEasyUnlockIcon(users()[0]->basic_user_info->account_id,
                                        icon);
  EXPECT_TRUE(test_api.tooltip_bubble()->IsVisible());
}

// Verifies that easy unlock icon state persists when changing auth user.
TEST_F(LockContentsViewUnitTest, EasyUnlockIconUpdatedDuringUserSwap) {
  // Build lock screen with two users.
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(2);
  SetWidget(CreateWidgetWithContent(contents));

  LockContentsView::TestApi test_api(contents);
  LoginBigUserView* primary = test_api.primary_big_view();
  LoginBigUserView* secondary = test_api.opt_secondary_big_view();

  // Returns true if the easy unlock icon is displayed for |view|.
  auto showing_easy_unlock_icon = [&](LoginBigUserView* view) {
    if (!view->auth_user())
      return false;

    views::View* icon =
        LoginPasswordView::TestApi(
            LoginAuthUserView::TestApi(view->auth_user()).password_view())
            .easy_unlock_icon();
    return icon->visible();
  };

  // Enables easy unlock icon for |view|.
  auto enable_icon = [&](LoginBigUserView* view) {
    auto icon = mojom::EasyUnlockIconOptions::New();
    icon->icon = mojom::EasyUnlockIconId::LOCKED;
    data_dispatcher()->ShowEasyUnlockIcon(
        view->GetCurrentUser()->basic_user_info->account_id, icon);
  };

  // Disables easy unlock icon for |view|.
  auto disable_icon = [&](LoginBigUserView* view) {
    auto icon = mojom::EasyUnlockIconOptions::New();
    icon->icon = mojom::EasyUnlockIconId::NONE;
    data_dispatcher()->ShowEasyUnlockIcon(
        view->GetCurrentUser()->basic_user_info->account_id, icon);
  };

  // Makes |view| the active auth view so it will can show auth methods.
  auto make_active_auth_view = [&](LoginBigUserView* view) {
    // Send event to swap users.
    ui::test::EventGenerator& generator = GetEventGenerator();
    LoginUserView* user_view = view->GetUserView();
    generator.MoveMouseTo(user_view->GetBoundsInScreen().CenterPoint());
    generator.ClickLeftButton();
  };

  // NOTE: we cannot assert on non-active auth views because the easy unlock
  // icon is lazily updated, ie, if we're not showing the view we will not
  // update icon state.

  // No easy unlock icons are shown.
  make_active_auth_view(primary);
  EXPECT_FALSE(showing_easy_unlock_icon(primary));

  // Activate icon for primary.
  enable_icon(primary);
  EXPECT_TRUE(showing_easy_unlock_icon(primary));

  // Secondary does not have easy unlock enabled; swapping auth hides the icon.
  make_active_auth_view(secondary);
  EXPECT_FALSE(showing_easy_unlock_icon(secondary));

  // Switching back enables the icon again.
  make_active_auth_view(primary);
  EXPECT_TRUE(showing_easy_unlock_icon(primary));

  // Activate icon for secondary. Primary visiblity does not change.
  enable_icon(secondary);
  EXPECT_TRUE(showing_easy_unlock_icon(primary));

  // Swap to secondary, icon still visible.
  make_active_auth_view(secondary);
  EXPECT_TRUE(showing_easy_unlock_icon(secondary));

  // Deactivate secondary, icon hides.
  disable_icon(secondary);
  EXPECT_FALSE(showing_easy_unlock_icon(secondary));

  // Deactivate primary, icon still hidden.
  disable_icon(primary);
  EXPECT_FALSE(showing_easy_unlock_icon(secondary));

  // Enable primary, icon still hidden.
  enable_icon(primary);
  EXPECT_FALSE(showing_easy_unlock_icon(secondary));
}

TEST_F(LockContentsViewUnitTest, ShowErrorBubbleOnAuthFailure) {
  // Build lock screen with a single user.
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(1);
  SetWidget(CreateWidgetWithContent(contents));

  LockContentsView::TestApi test_api(contents);

  // Password submit runs mojo.
  std::unique_ptr<MockLoginScreenClient> client = BindMockLoginScreenClient();
  client->set_authenticate_user_callback_result(false);
  EXPECT_CALL(
      *client,
      AuthenticateUser_(users()[0]->basic_user_info->account_id, _, false, _));

  // Submit password.
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.PressKey(ui::KeyboardCode::VKEY_A, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(test_api.auth_error_bubble()->IsVisible());

  // The error bubble is expected to close on a user action - e.g. if they start
  // typing the password again.
  generator.PressKey(ui::KeyboardCode::VKEY_B, 0);
  EXPECT_FALSE(test_api.auth_error_bubble()->IsVisible());
}

TEST_F(LockContentsViewUnitTest, ErrorBubbleOnUntrustedDetachableBase) {
  auto fake_detachable_base_model =
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher());
  FakeLoginDetachableBaseModel* detachable_base_model =
      fake_detachable_base_model.get();

  // Build lock screen with 2 users.
  auto* contents = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                        data_dispatcher(),
                                        std::move(fake_detachable_base_model));
  SetUserCount(2);

  const AccountId& kFirstUserAccountId =
      users()[0]->basic_user_info->account_id;
  const AccountId& kSecondUserAccountId =
      users()[1]->basic_user_info->account_id;

  // Initialize the detachable base state, so the user 1 has previously used
  // detachable base.
  detachable_base_model->InitLastUsedBases({{kFirstUserAccountId, "1234"}});
  detachable_base_model->SetPairingStatus(
      DetachableBasePairingStatus::kAuthenticated, "1234");
  SetWidget(CreateWidgetWithContent(contents));

  LockContentsView::TestApi test_api(contents);
  ui::test::EventGenerator& generator = GetEventGenerator();

  EXPECT_FALSE(test_api.detachable_base_error_bubble()->IsVisible());

  // Change detachable base to a base different than the one previously used by
  // the user - verify that a detachable base error bubble is shown.
  detachable_base_model->SetPairingStatus(
      DetachableBasePairingStatus::kAuthenticated, "5678");
  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());

  // Verify that the bubble is not hidden if the user starts typing.
  generator.PressKey(ui::KeyboardCode::VKEY_B, 0);
  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());

  // Switching to the user that doesn't have previously used detachable base
  // (and should thus not be warned about the detachable base missmatch) should
  // hide the login bubble.
  LoginAuthUserView::TestApi secondary_test_api(
      test_api.opt_secondary_big_view()->auth_user());
  generator.MoveMouseTo(
      secondary_test_api.user_view()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();

  EXPECT_FALSE(test_api.detachable_base_error_bubble()->IsVisible());

  // The error should be shown again when switching back to the primary user.
  LoginAuthUserView::TestApi primary_test_api(
      test_api.primary_big_view()->auth_user());
  generator.MoveMouseTo(
      primary_test_api.user_view()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();

  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());
  EXPECT_FALSE(primary_test_api.password_view()->HasFocus());

  EXPECT_EQ("1234",
            detachable_base_model->GetLastUsedBase(kFirstUserAccountId));
  EXPECT_EQ("", detachable_base_model->GetLastUsedBase(kSecondUserAccountId));

  // The current detachable base should be set as the last used one by the user
  // after they authenticate - test for this.
  std::unique_ptr<MockLoginScreenClient> client = BindMockLoginScreenClient();
  client->set_authenticate_user_callback_result(true);
  EXPECT_CALL(*client, AuthenticateUser_(kFirstUserAccountId, _, false, _));

  // Submit password.
  primary_test_api.password_view()->RequestFocus();
  generator.PressKey(ui::KeyboardCode::VKEY_A, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ("5678",
            detachable_base_model->GetLastUsedBase(kFirstUserAccountId));
  EXPECT_EQ("", detachable_base_model->GetLastUsedBase(kSecondUserAccountId));
}

TEST_F(LockContentsViewUnitTest, ErrorBubbleForUnauthenticatedDetachableBase) {
  auto fake_detachable_base_model =
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher());
  FakeLoginDetachableBaseModel* detachable_base_model =
      fake_detachable_base_model.get();

  // Build lock screen with 2 users.
  auto* contents = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                        data_dispatcher(),
                                        std::move(fake_detachable_base_model));
  SetUserCount(2);

  const AccountId& kFirstUserAccountId =
      users()[0]->basic_user_info->account_id;
  const AccountId& kSecondUserAccountId =
      users()[1]->basic_user_info->account_id;

  detachable_base_model->InitLastUsedBases({{kSecondUserAccountId, "5678"}});

  SetWidget(CreateWidgetWithContent(contents));

  LockContentsView::TestApi test_api(contents);
  ui::test::EventGenerator& generator = GetEventGenerator();

  EXPECT_FALSE(test_api.detachable_base_error_bubble()->IsVisible());

  // Show notification if unauthenticated base is attached.
  detachable_base_model->SetPairingStatus(
      DetachableBasePairingStatus::kNotAuthenticated, "");
  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());

  // Verify that the bubble is not hidden if the user starts typing.
  generator.PressKey(ui::KeyboardCode::VKEY_B, 0);
  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());

  // Switching to another user should not hide the error bubble.
  LoginAuthUserView::TestApi secondary_test_api(
      test_api.opt_secondary_big_view()->auth_user());
  generator.MoveMouseTo(
      secondary_test_api.user_view()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();

  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());
  EXPECT_FALSE(secondary_test_api.password_view()->HasFocus());

  // The last trusted detachable used by the user should not be overriden by
  // user authentication.
  std::unique_ptr<MockLoginScreenClient> client = BindMockLoginScreenClient();
  client->set_authenticate_user_callback_result(true);
  EXPECT_CALL(*client, AuthenticateUser_(kSecondUserAccountId, _, false, _));

  // Submit password.
  secondary_test_api.password_view()->RequestFocus();
  generator.PressKey(ui::KeyboardCode::VKEY_A, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ("", detachable_base_model->GetLastUsedBase(kFirstUserAccountId));
  EXPECT_EQ("5678",
            detachable_base_model->GetLastUsedBase(kSecondUserAccountId));
}
TEST_F(LockContentsViewUnitTest,
       RemovingAttachedBaseHidesDetachableBaseNotification) {
  auto fake_detachable_base_model =
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher());
  FakeLoginDetachableBaseModel* detachable_base_model =
      fake_detachable_base_model.get();

  // Build lock screen with 2 users.
  auto* contents = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                        data_dispatcher(),
                                        std::move(fake_detachable_base_model));
  SetUserCount(1);

  const AccountId& kUserAccountId = users()[0]->basic_user_info->account_id;

  // Initialize the detachable base state, as if the user has previously used
  // detachable base.
  detachable_base_model->InitLastUsedBases({{kUserAccountId, "1234"}});
  detachable_base_model->SetPairingStatus(
      DetachableBasePairingStatus::kAuthenticated, "1234");

  SetWidget(CreateWidgetWithContent(contents));

  LockContentsView::TestApi test_api(contents);

  // Change detachable base to a base different than the one previously used by
  // the user - verify that a detachable base error bubble is shown.
  detachable_base_model->SetPairingStatus(
      DetachableBasePairingStatus::kAuthenticated, "5678");
  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());

  // The notification should be hidden if the base gets detached.
  detachable_base_model->SetPairingStatus(DetachableBasePairingStatus::kNone,
                                          "");
  EXPECT_FALSE(test_api.detachable_base_error_bubble()->IsVisible());
}

TEST_F(LockContentsViewUnitTest, DetachableBaseErrorClearsAuthError) {
  auto fake_detachable_base_model =
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher());
  FakeLoginDetachableBaseModel* detachable_base_model =
      fake_detachable_base_model.get();

  // Build lock screen with a single user.
  auto* contents = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                        data_dispatcher(),
                                        std::move(fake_detachable_base_model));
  SetUserCount(1);

  const AccountId& kUserAccountId = users()[0]->basic_user_info->account_id;

  // Initialize the detachable base state, as if the user has previously used
  // detachable base.
  detachable_base_model->InitLastUsedBases({{kUserAccountId, "1234"}});
  detachable_base_model->SetPairingStatus(
      DetachableBasePairingStatus::kAuthenticated, "1234");

  SetWidget(CreateWidgetWithContent(contents));

  LockContentsView::TestApi test_api(contents);
  ui::test::EventGenerator& generator = GetEventGenerator();

  EXPECT_FALSE(test_api.detachable_base_error_bubble()->IsVisible());

  // Attempt and fail user auth - an auth error is expected to be shown.
  std::unique_ptr<MockLoginScreenClient> client = BindMockLoginScreenClient();
  client->set_authenticate_user_callback_result(false);
  EXPECT_CALL(*client, AuthenticateUser_(kUserAccountId, _, false, _));

  // Submit password.
  generator.PressKey(ui::KeyboardCode::VKEY_A, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(test_api.auth_error_bubble()->IsVisible());
  EXPECT_FALSE(test_api.detachable_base_error_bubble()->IsVisible());

  // Change detachable base to a base different than the one previously used by
  // the user - verify that a detachable base error bubble is shown, and the
  // auth error is hidden.
  detachable_base_model->SetPairingStatus(
      DetachableBasePairingStatus::kAuthenticated, "5678");

  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());
  EXPECT_FALSE(test_api.auth_error_bubble()->IsVisible());
}

TEST_F(LockContentsViewUnitTest, AuthErrorDoesNotRemoveDetachableBaseError) {
  auto fake_detachable_base_model =
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher());
  FakeLoginDetachableBaseModel* detachable_base_model =
      fake_detachable_base_model.get();

  // Build lock screen with a single user.
  auto* contents = new LockContentsView(mojom::TrayActionState::kNotAvailable,
                                        data_dispatcher(),
                                        std::move(fake_detachable_base_model));
  SetUserCount(1);

  const AccountId& kUserAccountId = users()[0]->basic_user_info->account_id;

  // Initialize the detachable base state, as if the user has previously used
  // detachable base.
  detachable_base_model->InitLastUsedBases({{kUserAccountId, "1234"}});
  SetWidget(CreateWidgetWithContent(contents));

  detachable_base_model->SetPairingStatus(
      DetachableBasePairingStatus::kAuthenticated, "1234");

  LockContentsView::TestApi test_api(contents);
  ui::test::EventGenerator& generator = GetEventGenerator();

  EXPECT_FALSE(test_api.detachable_base_error_bubble()->IsVisible());

  // Change detachable base to a base different than the one previously used by
  // the user - verify that a detachable base error bubble is shown, and the
  // auth error is hidden.
  detachable_base_model->SetPairingStatus(
      DetachableBasePairingStatus::kAuthenticated, "5678");

  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());

  // Attempt and fail user auth - an auth error is expected to be shown.
  // Detachable base error should not be hidden.
  std::unique_ptr<MockLoginScreenClient> client = BindMockLoginScreenClient();
  client->set_authenticate_user_callback_result(false);
  EXPECT_CALL(*client, AuthenticateUser_(kUserAccountId, _, false, _));

  // Submit password.
  LoginAuthUserView::TestApi(test_api.primary_big_view()->auth_user())
      .password_view()
      ->RequestFocus();
  generator.PressKey(ui::KeyboardCode::VKEY_A, 0);
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(test_api.auth_error_bubble()->IsVisible());
  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());

  // User action, like pressing a key should close the auth error bubble, but
  // not the detachable base error bubble.
  generator.PressKey(ui::KeyboardCode::VKEY_A, 0);

  EXPECT_TRUE(test_api.detachable_base_error_bubble()->IsVisible());
  EXPECT_FALSE(test_api.auth_error_bubble()->IsVisible());
}

TEST_F(LockContentsViewKeyboardUnitTest, SwitchPinAndVirtualKeyboard) {
  ASSERT_NO_FATAL_FAILURE(ShowLockScreen());
  LockContentsView* contents =
      LockScreen::TestApi(LockScreen::Get()).contents_view();
  ASSERT_NE(nullptr, contents);

  // Add user with enabled pin method of authentication.
  const std::string email = "user@domain.com";
  LoadUser(email);
  contents->OnPinEnabledForUserChanged(AccountId::FromUserEmail(email), true);
  LoginBigUserView* big_view =
      LockContentsView::TestApi(contents).primary_big_view();
  ASSERT_NE(nullptr, big_view);
  ASSERT_NE(nullptr, big_view->auth_user());

  // Pin keyboard should only be visible when there is no virtual keyboard
  // shown.
  LoginPinView* pin_view =
      LoginAuthUserView::TestApi(big_view->auth_user()).pin_view();
  EXPECT_TRUE(pin_view->visible());

  ASSERT_NO_FATAL_FAILURE(ShowKeyboard());
  EXPECT_FALSE(pin_view->visible());

  ASSERT_NO_FATAL_FAILURE(HideKeyboard());
  EXPECT_TRUE(pin_view->visible());
}

// Verify that swapping works in two user layout between one regular auth user
// and one public account user.
TEST_F(LockContentsViewUnitTest, SwapAuthAndPublicAccountUserInTwoUserLayout) {
  // Build lock screen with two users: one public account user and one regular
  // user.
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  LockContentsView::TestApi test_api(contents);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);
  AddPublicAccountUsers(1);
  AddUsers(1);

  // Capture user info to validate it did not change during the swap.
  AccountId primary_user = test_api.primary_big_view()
                               ->GetCurrentUser()
                               ->basic_user_info->account_id;
  AccountId secondary_user = test_api.opt_secondary_big_view()
                                 ->GetCurrentUser()
                                 ->basic_user_info->account_id;
  EXPECT_NE(primary_user, secondary_user);

  // Primary user starts with auth. Secondary user does not have any auth.
  EXPECT_TRUE(test_api.primary_big_view()->IsAuthEnabled());
  EXPECT_FALSE(test_api.opt_secondary_big_view()->IsAuthEnabled());

  // Verify the LoginBigUserView has built the child view correctly.
  ASSERT_TRUE(test_api.primary_big_view()->public_account());
  ASSERT_FALSE(test_api.primary_big_view()->auth_user());
  ASSERT_FALSE(test_api.opt_secondary_big_view()->public_account());
  ASSERT_TRUE(test_api.opt_secondary_big_view()->auth_user());

  // Send event to swap users.
  ui::test::EventGenerator& generator = GetEventGenerator();
  LoginAuthUserView::TestApi secondary_test_api(
      test_api.opt_secondary_big_view()->auth_user());
  generator.MoveMouseTo(
      secondary_test_api.user_view()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();

  // User info is not swapped.
  EXPECT_EQ(primary_user, test_api.primary_big_view()
                              ->GetCurrentUser()
                              ->basic_user_info->account_id);
  EXPECT_EQ(secondary_user, test_api.opt_secondary_big_view()
                                ->GetCurrentUser()
                                ->basic_user_info->account_id);

  // Child view of LoginBigUserView stays the same.
  ASSERT_TRUE(test_api.primary_big_view()->public_account());
  ASSERT_FALSE(test_api.primary_big_view()->auth_user());
  ASSERT_FALSE(test_api.opt_secondary_big_view()->public_account());
  ASSERT_TRUE(test_api.opt_secondary_big_view()->auth_user());

  // Active auth (ie, which user is showing password) is swapped.
  EXPECT_FALSE(test_api.primary_big_view()->IsAuthEnabled());
  EXPECT_TRUE(test_api.opt_secondary_big_view()->IsAuthEnabled());
}

// Ensures that when swapping from a user list, the entire user info is swapped
// and the primary big user will rebuild its child view when necessary.
TEST_F(LockContentsViewUnitTest, SwapUserListToPrimaryBigUser) {
  // Build lock screen with 4 users: two public account users and two regular
  // users.
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  LockContentsView::TestApi lock_contents(contents);
  AddPublicAccountUsers(2);
  AddUsers(2);
  ScrollableUsersListView::TestApi users_list(lock_contents.users_list());
  EXPECT_EQ(users().size() - 1, users_list.user_views().size());
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);

  LoginBigUserView* primary_big_view = lock_contents.primary_big_view();

  // Verify that primary_big_view is public account user.
  ASSERT_TRUE(primary_big_view->public_account());
  ASSERT_FALSE(primary_big_view->auth_user());

  const LoginUserView* user_view0 = users_list.user_views().at(0);
  const LoginUserView* user_view1 = users_list.user_views().at(1);
  const LoginUserView* user_view2 = users_list.user_views().at(2);

  // Clicks on |view| to make it swap with the primary big user.
  auto click_view = [&](const LoginUserView* view) {
    ui::test::EventGenerator& generator = GetEventGenerator();
    generator.MoveMouseTo(view->GetBoundsInScreen().CenterPoint());
    generator.ClickLeftButton();
  };

  auto is_public_account = [](const LoginUserView* view) -> bool {
    return view->current_user()->basic_user_info->type ==
           user_manager::USER_TYPE_PUBLIC_ACCOUNT;
  };

  // Case 1: Swap user_view0 (public account user) with primary big user (public
  // account user).
  EXPECT_TRUE(is_public_account(user_view0));
  AccountId primary_id =
      primary_big_view->GetCurrentUser()->basic_user_info->account_id;
  AccountId list_user_id =
      user_view0->current_user()->basic_user_info->account_id;
  EXPECT_NE(primary_id, list_user_id);

  // Send event to swap users.
  click_view(user_view0);

  // User info is swapped.
  EXPECT_EQ(list_user_id,
            primary_big_view->GetCurrentUser()->basic_user_info->account_id);
  EXPECT_EQ(primary_id,
            user_view0->current_user()->basic_user_info->account_id);

  // Child view of primary big user stays the same.
  ASSERT_TRUE(primary_big_view->public_account());
  ASSERT_FALSE(primary_big_view->auth_user());
  // user_view0 is still public account user.
  EXPECT_TRUE(is_public_account(user_view0));

  // Case 2: Swap user_view1 (auth user) with primary big user (public account
  // user).
  EXPECT_FALSE(is_public_account(user_view1));
  primary_id = primary_big_view->GetCurrentUser()->basic_user_info->account_id;
  list_user_id = user_view1->current_user()->basic_user_info->account_id;
  EXPECT_NE(primary_id, list_user_id);

  // Send event to swap users.
  click_view(user_view1);

  // User info is swapped.
  EXPECT_EQ(list_user_id,
            primary_big_view->GetCurrentUser()->basic_user_info->account_id);
  EXPECT_EQ(primary_id,
            user_view1->current_user()->basic_user_info->account_id);

  // Primary big user becomes auth user and its child view is rebuilt.
  ASSERT_FALSE(primary_big_view->public_account());
  ASSERT_TRUE(primary_big_view->auth_user());
  // user_view1 becomes public account user.
  EXPECT_TRUE(is_public_account(user_view1));

  // Case 3: Swap user_view2 (auth user) with primary big user (auth user).
  EXPECT_FALSE(is_public_account(user_view2));
  primary_id = primary_big_view->GetCurrentUser()->basic_user_info->account_id;
  list_user_id = user_view2->current_user()->basic_user_info->account_id;
  EXPECT_NE(primary_id, list_user_id);

  // Send event to swap users.
  click_view(user_view2);

  // User info is swapped.
  EXPECT_EQ(list_user_id,
            primary_big_view->GetCurrentUser()->basic_user_info->account_id);
  EXPECT_EQ(primary_id,
            user_view2->current_user()->basic_user_info->account_id);

  // Child view of primary big user stays the same.
  ASSERT_FALSE(primary_big_view->public_account());
  ASSERT_TRUE(primary_big_view->auth_user());
  // user_view2 is still auth user.
  EXPECT_FALSE(is_public_account(user_view2));

  // Case 4: Swap user_view0 (public account user) with with primary big user
  // (auth user).
  EXPECT_TRUE(is_public_account(user_view0));
  primary_id = primary_big_view->GetCurrentUser()->basic_user_info->account_id;
  list_user_id = user_view0->current_user()->basic_user_info->account_id;
  EXPECT_NE(primary_id, list_user_id);

  // Send event to swap users.
  click_view(user_view0);

  // User info is swapped.
  EXPECT_EQ(list_user_id,
            primary_big_view->GetCurrentUser()->basic_user_info->account_id);
  EXPECT_EQ(primary_id,
            user_view0->current_user()->basic_user_info->account_id);

  // Primary big user becomes public account user and its child view is rebuilt.
  ASSERT_TRUE(primary_big_view->public_account());
  ASSERT_FALSE(primary_big_view->auth_user());
  // user_view0 becomes auth user.
  EXPECT_FALSE(is_public_account(user_view0));
}

TEST_F(LockContentsViewUnitTest, ExpandedPublicSessionView) {
  // Build lock screen with 3 users: one public account user and two regular
  // users.
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kNotAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  LockContentsView::TestApi lock_contents(contents);
  std::unique_ptr<views::Widget> widget = CreateWidgetWithContent(contents);
  AddPublicAccountUsers(1);
  AddUsers(2);

  views::View* main_view = lock_contents.main_view();
  LoginExpandedPublicAccountView* expanded_view = lock_contents.expanded_view();
  EXPECT_TRUE(main_view->visible());
  EXPECT_FALSE(expanded_view->visible());

  LoginBigUserView* primary_big_view = lock_contents.primary_big_view();
  AccountId primary_id =
      primary_big_view->GetCurrentUser()->basic_user_info->account_id;

  // Open the expanded public session view.
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.PressKey(ui::KeyboardCode::VKEY_RETURN, 0);

  EXPECT_FALSE(main_view->visible());
  EXPECT_TRUE(expanded_view->visible());
  EXPECT_EQ(expanded_view->current_user()->basic_user_info->account_id,
            primary_id);

  // Expect LanuchPublicSession mojo call when the submit button is clicked.
  std::unique_ptr<MockLoginScreenClient> client = BindMockLoginScreenClient();
  EXPECT_CALL(*client, LaunchPublicSession(primary_id, _, _));

  // Click on the submit button.
  LoginExpandedPublicAccountView::TestApi expanded_view_api(expanded_view);
  generator.MoveMouseTo(
      expanded_view_api.submit_button()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  base::RunLoop().RunUntilIdle();
}

TEST_F(LockContentsViewUnitTest, OnUnlockAllowedForUserChanged) {
  auto* contents = new LockContentsView(
      mojom::TrayActionState::kAvailable, data_dispatcher(),
      std::make_unique<FakeLoginDetachableBaseModel>(data_dispatcher()));
  SetUserCount(1);
  SetWidget(CreateWidgetWithContent(contents));

  const AccountId& kFirstUserAccountId =
      users()[0]->basic_user_info->account_id;
  LockContentsView::TestApi contents_test_api(contents);
  LoginAuthUserView::TestApi auth_test_api(
      contents_test_api.primary_big_view()->auth_user());
  views::View* note_action_button = contents_test_api.note_action();
  LoginPasswordView* password_view = auth_test_api.password_view();
  LoginPinView* pin_view = auth_test_api.pin_view();
  views::View* disabled_auth_message = auth_test_api.disabled_auth_message();

  // The password field is shown by default, and the note action button is
  // shown because the lock screen note state is |kAvailable|.
  EXPECT_TRUE(note_action_button->visible());
  EXPECT_TRUE(password_view->visible());
  EXPECT_FALSE(pin_view->visible());
  EXPECT_FALSE(disabled_auth_message->visible());
  // Setting auth disabled will hide the password field and the note action
  // button, and show the message.
  data_dispatcher()->SetAuthEnabledForUser(
      kFirstUserAccountId, false,
      base::Time::Now() + base::TimeDelta::FromHours(8));
  EXPECT_FALSE(note_action_button->visible());
  EXPECT_FALSE(password_view->visible());
  EXPECT_FALSE(pin_view->visible());
  EXPECT_TRUE(disabled_auth_message->visible());
  // Setting auth enabled will hide the message and show the password field.
  data_dispatcher()->SetAuthEnabledForUser(kFirstUserAccountId, true,
                                           base::nullopt);
  EXPECT_FALSE(note_action_button->visible());
  EXPECT_TRUE(password_view->visible());
  EXPECT_FALSE(pin_view->visible());
  EXPECT_FALSE(disabled_auth_message->visible());

  // Set auth disabled again.
  data_dispatcher()->SetAuthEnabledForUser(
      kFirstUserAccountId, false,
      base::Time::Now() + base::TimeDelta::FromHours(8));
  EXPECT_FALSE(note_action_button->visible());
  EXPECT_FALSE(password_view->visible());
  EXPECT_FALSE(pin_view->visible());
  EXPECT_TRUE(disabled_auth_message->visible());
  // Enable PIN. There's no UI change because auth is currently disabled.
  data_dispatcher()->SetPinEnabledForUser(kFirstUserAccountId, true);
  EXPECT_FALSE(note_action_button->visible());
  EXPECT_FALSE(password_view->visible());
  EXPECT_FALSE(pin_view->visible());
  EXPECT_TRUE(disabled_auth_message->visible());
  // Set auth enabled again. Both password field and PIN keyboard are shown.
  data_dispatcher()->SetAuthEnabledForUser(kFirstUserAccountId, true,
                                           base::nullopt);
  EXPECT_FALSE(note_action_button->visible());
  EXPECT_TRUE(password_view->visible());
  EXPECT_TRUE(pin_view->visible());
  EXPECT_FALSE(disabled_auth_message->visible());
}

}  // namespace ash
