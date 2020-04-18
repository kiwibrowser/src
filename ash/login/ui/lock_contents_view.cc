// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/lock_contents_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "ash/detachable_base/detachable_base_pairing_status.h"
#include "ash/focus_cycler.h"
#include "ash/ime/ime_controller.h"
#include "ash/keyboard/keyboard_observer_register.h"
#include "ash/login/login_screen_controller.h"
#include "ash/login/ui/layout_util.h"
#include "ash/login/ui/lock_screen.h"
#include "ash/login/ui/login_auth_user_view.h"
#include "ash/login/ui/login_big_user_view.h"
#include "ash/login/ui/login_bubble.h"
#include "ash/login/ui/login_detachable_base_model.h"
#include "ash/login/ui/login_expanded_public_account_view.h"
#include "ash/login/ui/login_public_account_user_view.h"
#include "ash/login/ui/login_user_view.h"
#include "ash/login/ui/non_accessible_view.h"
#include "ash/login/ui/note_action_launch_button.h"
#include "ash/login/ui/scrollable_users_list_view.h"
#include "ash/root_window_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/status_area_widget_delegate.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "components/user_manager/user_type.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/managed_display_info.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/accessibility/ax_aura_obj_cache.h"
#include "ui/views/background.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/focus/focus_search.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"

namespace ash {

namespace {

// Any non-zero value used for separator height. Makes debugging easier; this
// should not affect visual appearance.
constexpr int kNonEmptyHeightDp = 30;

// Horizontal distance between two users in the low density layout.
constexpr int kLowDensityDistanceBetweenUsersInLandscapeDp = 118;
constexpr int kLowDensityDistanceBetweenUsersInPortraitDp = 32;

// Margin left of the auth user in the medium density layout.
constexpr int kMediumDensityMarginLeftOfAuthUserLandscapeDp = 98;
constexpr int kMediumDensityMarginLeftOfAuthUserPortraitDp = 0;

// Horizontal distance between the auth user and the medium density user row.
constexpr int kMediumDensityDistanceBetweenAuthUserAndUsersLandscapeDp = 220;
constexpr int kMediumDensityDistanceBetweenAuthUserAndUsersPortraitDp = 84;

constexpr const char kLockContentsViewName[] = "LockContentsView";

// A view which stores two preferred sizes. The embedder can control which one
// is used.
class MultiSizedView : public views::View {
 public:
  MultiSizedView(const gfx::Size& a, const gfx::Size& b) : a_(a), b_(b) {}
  ~MultiSizedView() override = default;

  void SwapPreferredSizeTo(bool use_a) {
    if (use_a)
      SetPreferredSize(a_);
    else
      SetPreferredSize(b_);
  }

 private:
  gfx::Size a_;
  gfx::Size b_;

  DISALLOW_COPY_AND_ASSIGN(MultiSizedView);
};

// Returns the first or last focusable child of |root|. If |reverse| is false,
// this returns the first focusable child. If |reverse| is true, this returns
// the last focusable child.
views::View* FindFirstOrLastFocusableChild(views::View* root, bool reverse) {
  views::FocusSearch search(root, reverse /*cycle*/,
                            false /*accessibility_mode*/);
  views::FocusTraversable* dummy_focus_traversable;
  views::View* dummy_focus_traversable_view;
  return search.FindNextFocusableView(
      root,
      reverse ? views::FocusSearch::SearchDirection::kBackwards
              : views::FocusSearch::SearchDirection::kForwards,
      views::FocusSearch::TraversalDirection::kDown,
      views::FocusSearch::StartingViewPolicy::kSkipStartingView,
      views::FocusSearch::AnchoredDialogPolicy::kCanGoIntoAnchoredDialog,
      &dummy_focus_traversable, &dummy_focus_traversable_view);
}

// Make a section of the text bold.
// |label|:       The label to apply mixed styles.
// |text|:        The message to display.
// |bold_start|:  The position in |text| to start bolding.
// |bold_length|: The length of bold text.
void MakeSectionBold(views::StyledLabel* label,
                     const base::string16& text,
                     const base::Optional<int>& bold_start,
                     int bold_length) {
  auto create_style = [&](bool is_bold) {
    views::StyledLabel::RangeStyleInfo style;
    if (is_bold) {
      style.custom_font = label->GetDefaultFontList().Derive(
          0, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::BOLD);
    }
    style.override_color = SK_ColorWHITE;
    return style;
  };

  auto add_style = [&](const views::StyledLabel::RangeStyleInfo& style,
                       int start, int end) {
    if (start >= end)
      return;

    label->AddStyleRange(gfx::Range(start, end), style);
  };

  views::StyledLabel::RangeStyleInfo regular_style =
      create_style(false /*is_bold*/);
  views::StyledLabel::RangeStyleInfo bold_style =
      create_style(true /*is_bold*/);
  if (!bold_start || bold_length == 0) {
    add_style(regular_style, 0, text.length());
    return;
  }

  add_style(regular_style, 0, *bold_start - 1);
  add_style(bold_style, *bold_start, *bold_start + bold_length);
  add_style(regular_style, *bold_start + bold_length + 1, text.length());
}

// Helper function to create a label for the dev channel info view.
views::Label* CreateInfoLabel() {
  views::Label* label = new views::Label();
  label->SetAutoColorReadabilityEnabled(false);
  label->SetEnabledColor(SK_ColorWHITE);
  label->SetFontList(views::Label::GetDefaultFontList().Derive(
      -1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
  label->SetSubpixelRenderingEnabled(false);

  return label;
}

keyboard::KeyboardController* GetKeyboardControllerForWidget(
    const views::Widget* widget) {
  keyboard::KeyboardController* keyboard_controller =
      keyboard::KeyboardController::GetInstance();
  if (!keyboard_controller)
    return nullptr;

  aura::Window* keyboard_window =
      keyboard_controller->GetContainerWindow()->GetRootWindow();
  aura::Window* this_window = widget->GetNativeWindow()->GetRootWindow();
  return keyboard_window == this_window ? keyboard_controller : nullptr;
}

bool IsPublicAccountUser(const mojom::LoginUserInfoPtr& user) {
  return user->basic_user_info->type == user_manager::USER_TYPE_PUBLIC_ACCOUNT;
}

}  // namespace

LockContentsView::TestApi::TestApi(LockContentsView* view) : view_(view) {}

LockContentsView::TestApi::~TestApi() = default;

LoginBigUserView* LockContentsView::TestApi::primary_big_view() const {
  return view_->primary_big_view_;
}

LoginBigUserView* LockContentsView::TestApi::opt_secondary_big_view() const {
  return view_->opt_secondary_big_view_;
}

ScrollableUsersListView* LockContentsView::TestApi::users_list() const {
  return view_->users_list_;
}

views::View* LockContentsView::TestApi::note_action() const {
  return view_->note_action_;
}

LoginBubble* LockContentsView::TestApi::tooltip_bubble() const {
  return view_->tooltip_bubble_.get();
}

LoginBubble* LockContentsView::TestApi::auth_error_bubble() const {
  return view_->auth_error_bubble_.get();
}

LoginBubble* LockContentsView::TestApi::detachable_base_error_bubble() const {
  return view_->detachable_base_error_bubble_.get();
}

views::View* LockContentsView::TestApi::dev_channel_info() const {
  return view_->dev_channel_info_;
}

LoginExpandedPublicAccountView* LockContentsView::TestApi::expanded_view()
    const {
  return view_->expanded_view_;
}

views::View* LockContentsView::TestApi::main_view() const {
  return view_->main_view_;
}

LockContentsView::UserState::UserState(AccountId account_id)
    : account_id(account_id) {}

LockContentsView::UserState::UserState(UserState&&) = default;

LockContentsView::UserState::~UserState() = default;

LockContentsView::LockContentsView(
    mojom::TrayActionState initial_note_action_state,
    LoginDataDispatcher* data_dispatcher,
    std::unique_ptr<LoginDetachableBaseModel> detachable_base_model)
    : NonAccessibleView(kLockContentsViewName),
      data_dispatcher_(data_dispatcher),
      detachable_base_model_(std::move(detachable_base_model)),
      display_observer_(this),
      session_observer_(this),
      keyboard_observer_(this) {
  data_dispatcher_->AddObserver(this);
  display_observer_.Add(display::Screen::GetScreen());
  Shell::Get()->login_screen_controller()->AddObserver(this);
  Shell::Get()->system_tray_notifier()->AddSystemTrayFocusObserver(this);
  auth_error_bubble_ = std::make_unique<LoginBubble>();
  detachable_base_error_bubble_ = std::make_unique<LoginBubble>();
  tooltip_bubble_ = std::make_unique<LoginBubble>();

  // We reuse the focusable state on this view as a signal that focus should
  // switch to the system tray. LockContentsView should otherwise not be
  // focusable.
  SetFocusBehavior(FocusBehavior::ALWAYS);

  SetLayoutManager(std::make_unique<views::FillLayout>());

  main_view_ = new NonAccessibleView();
  AddChildView(main_view_);

  // The top header view.
  top_header_ = new views::View();
  auto top_header_layout =
      std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal);
  top_header_layout->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_END);
  top_header_->SetLayoutManager(std::move(top_header_layout));
  AddChildView(top_header_);

  dev_channel_info_ = new views::View();
  auto dev_channel_info_layout = std::make_unique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(5, 8));
  dev_channel_info_layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_END);
  dev_channel_info_->SetLayoutManager(std::move(dev_channel_info_layout));
  dev_channel_info_->SetVisible(false);
  top_header_->AddChildView(dev_channel_info_);

  note_action_ = new NoteActionLaunchButton(initial_note_action_state);
  top_header_->AddChildView(note_action_);

  // Public Session expanded view.
  expanded_view_ = new LoginExpandedPublicAccountView(
      base::BindRepeating(&LockContentsView::SetDisplayStyle,
                          base::Unretained(this), DisplayStyle::kAll));
  expanded_view_->SetVisible(false);
  AddChildView(expanded_view_);

  OnLockScreenNoteStateChanged(initial_note_action_state);
  Shell::Get()->AddShellObserver(this);
}

LockContentsView::~LockContentsView() {
  data_dispatcher_->RemoveObserver(this);
  Shell::Get()->login_screen_controller()->RemoveObserver(this);
  Shell::Get()->system_tray_notifier()->RemoveSystemTrayFocusObserver(this);

  if (unlock_attempt_ > 0) {
    // Times a password was incorrectly entered until user gives up (sign out
    // current session or shutdown the device). For a successful unlock,
    // unlock_attempt_ should already be reset by OnLockStateChanged.
    Shell::Get()->metrics()->login_metrics_recorder()->RecordNumLoginAttempts(
        unlock_attempt_, false /*success*/);
  }
  Shell::Get()->RemoveShellObserver(this);
  keyboard_observer_.RemoveAll();
}

void LockContentsView::Layout() {
  View::Layout();
  LayoutTopHeader();
  LayoutPublicSessionView();

  if (users_list_)
    users_list_->Layout();
}

void LockContentsView::AddedToWidget() {
  // Register keyboard observer after view has been added to the widget. If
  // virtual keyboard is activated before displaying lock screen we do not
  // receive OnVirtualKeyboardStateChanged() callback and we need to register
  // keyboard observer here.
  keyboard::KeyboardController* keyboard_controller = GetKeyboardController();
  if (keyboard_controller)
    keyboard_observer_.Add(keyboard_controller);

  DoLayout();

  // Focus the primary user when showing the UI. This will focus the password.
  if (primary_big_view_)
    primary_big_view_->RequestFocus();
}

void LockContentsView::OnFocus() {
  // If LockContentsView somehow gains focus (ie, a test, but it should not
  // under typical circumstances), immediately forward the focus to the
  // primary_big_view_ since LockContentsView has no real focusable content by
  // itself.
  if (primary_big_view_)
    primary_big_view_->RequestFocus();
}

void LockContentsView::AboutToRequestFocusFromTabTraversal(bool reverse) {
  // The LockContentsView itself doesn't have anything to focus. If it gets
  // focused we should change the currently focused widget (ie, to the shelf or
  // status area, or lock screen apps, if they are active).
  if (reverse && lock_screen_apps_active_) {
    Shell::Get()->login_screen_controller()->FocusLockScreenApps(reverse);
    return;
  }

  FocusNextWidget(reverse);
}

void LockContentsView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  Shelf* shelf = Shelf::ForWindow(GetWidget()->GetNativeWindow());
  ShelfWidget* shelf_widget = shelf->shelf_widget();
  int next_id = views::AXAuraObjCache::GetInstance()->GetID(shelf_widget);
  node_data->AddIntAttribute(ax::mojom::IntAttribute::kNextFocusId, next_id);

  int previous_id =
      views::AXAuraObjCache::GetInstance()->GetID(shelf->GetStatusAreaWidget());
  node_data->AddIntAttribute(ax::mojom::IntAttribute::kPreviousFocusId,
                             previous_id);
  node_data->SetNameExplicitlyEmpty();
}

void LockContentsView::OnUsersChanged(
    const std::vector<mojom::LoginUserInfoPtr>& users) {
  // The debug view will potentially call this method many times. Make sure to
  // invalidate any child references.
  main_view_->RemoveAllChildViews(true /*delete_children*/);
  opt_secondary_big_view_ = nullptr;
  users_list_ = nullptr;
  rotation_actions_.clear();
  users_.clear();

  // If there are no users we have no UI to build.
  if (users.empty()) {
    LOG(ERROR) << "Empty user list received";
    return;
  }

  // Build user state list.
  for (const mojom::LoginUserInfoPtr& user : users) {
    UserState state(user->basic_user_info->account_id);
    state.fingerprint_state = user->allow_fingerprint_unlock
                                  ? mojom::FingerprintUnlockState::AVAILABLE
                                  : mojom::FingerprintUnlockState::UNAVAILABLE;
    users_.push_back(std::move(state));
  }

  auto box_layout =
      std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal);
  main_layout_ = box_layout.get();
  main_layout_->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  main_layout_->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  main_view_->SetLayoutManager(std::move(box_layout));

  // Add big user.
  primary_big_view_ = AllocateLoginBigUserView(users[0], true /*is_primary*/);
  main_view_->AddChildView(primary_big_view_);

  // Build layout for additional users.
  if (users.size() == 2)
    CreateLowDensityLayout(users);
  else if (users.size() >= 3 && users.size() <= 6)
    CreateMediumDensityLayout(users);
  else if (users.size() >= 7)
    CreateHighDensityLayout(users);

  LayoutAuth(primary_big_view_, opt_secondary_big_view_, false /*animate*/);

  // Big user may be the same if we already built lock screen.
  OnBigUserChanged();

  // Force layout.
  PreferredSizeChanged();
  Layout();
}

void LockContentsView::OnPinEnabledForUserChanged(const AccountId& user,
                                                  bool enabled) {
  LockContentsView::UserState* state = FindStateForUser(user);
  if (!state) {
    LOG(ERROR) << "Unable to find user when changing PIN state to " << enabled;
    return;
  }

  state->show_pin = enabled;

  LoginBigUserView* big_user =
      TryToFindBigUser(user, true /*require_auth_active*/);
  if (big_user && big_user->auth_user())
    LayoutAuth(big_user, nullptr /*opt_to_hide*/, true /*animate*/);
}

void LockContentsView::OnAuthEnabledForUserChanged(
    const AccountId& user,
    bool enabled,
    const base::Optional<base::Time>& auth_reenabled_time) {
  LockContentsView::UserState* state = FindStateForUser(user);
  if (!state) {
    LOG(ERROR) << "Unable to find user when changing auth enabled state to "
               << enabled;
    return;
  }

  DCHECK(enabled || auth_reenabled_time);
  state->disable_auth = !enabled;
  // TODO(crbug.com/845287): Reenable lock screen note when auth is reenabled.
  if (state->disable_auth)
    DisableLockScreenNote();

  LoginBigUserView* big_user =
      TryToFindBigUser(user, true /*require_auth_active*/);
  if (big_user && big_user->auth_user()) {
    LayoutAuth(big_user, nullptr /*opt_to_hide*/, true /*animate*/);
    if (auth_reenabled_time)
      big_user->auth_user()->SetAuthReenabledTime(auth_reenabled_time.value());
  }
}

void LockContentsView::OnClickToUnlockEnabledForUserChanged(
    const AccountId& user,
    bool enabled) {
  LockContentsView::UserState* state = FindStateForUser(user);
  if (!state) {
    LOG(ERROR) << "Unable to find user enabling click to auth";
    return;
  }
  state->enable_tap_auth = enabled;

  LoginBigUserView* big_user =
      TryToFindBigUser(user, true /*require_auth_active*/);
  if (big_user && big_user->auth_user())
    LayoutAuth(big_user, nullptr /*opt_to_hide*/, true /*animate*/);
}

void LockContentsView::OnForceOnlineSignInForUser(const AccountId& user) {
  LockContentsView::UserState* state = FindStateForUser(user);
  if (!state) {
    LOG(ERROR) << "Unable to find user forcing online sign in";
    return;
  }
  state->force_online_sign_in = true;

  LoginBigUserView* big_user =
      TryToFindBigUser(user, true /*require_auth_active*/);
  if (big_user && big_user->auth_user())
    LayoutAuth(big_user, nullptr /*opt_to_hide*/, true /*animate*/);
}

void LockContentsView::OnShowEasyUnlockIcon(
    const AccountId& user,
    const mojom::EasyUnlockIconOptionsPtr& icon) {
  UserState* state = FindStateForUser(user);
  if (!state)
    return;

  state->easy_unlock_state = icon->Clone();
  UpdateEasyUnlockIconForUser(user);

  // Show tooltip only if the user is actively showing auth.
  LoginBigUserView* big_user =
      TryToFindBigUser(user, true /*require_auth_active*/);
  if (!big_user || !big_user->auth_user())
    return;

  tooltip_bubble_->Close();
  if (icon->autoshow_tooltip) {
    tooltip_bubble_->ShowTooltip(
        icon->tooltip, big_user->auth_user()->password_view() /*anchor_view*/);
  }
}

void LockContentsView::OnLockScreenNoteStateChanged(
    mojom::TrayActionState state) {
  if (disable_lock_screen_note_)
    state = mojom::TrayActionState::kNotAvailable;

  bool old_lock_screen_apps_active = lock_screen_apps_active_;
  lock_screen_apps_active_ = state == mojom::TrayActionState::kActive;
  note_action_->UpdateVisibility(state);
  LayoutTopHeader();

  // If lock screen apps just got deactivated - request focus for primary auth,
  // which should focus the password field.
  if (old_lock_screen_apps_active && !lock_screen_apps_active_ &&
      primary_big_view_) {
    primary_big_view_->RequestFocus();
  }
}

void LockContentsView::OnDevChannelInfoChanged(
    const std::string& os_version_label_text,
    const std::string& enterprise_info_text,
    const std::string& bluetooth_name) {
  DCHECK(!os_version_label_text.empty() || !enterprise_info_text.empty() ||
         !bluetooth_name.empty());

  if (!dev_channel_info_->visible()) {
    // Initialize the dev channel info view.
    dev_channel_info_->SetVisible(true);
    for (int i = 0; i < 3; ++i)
      dev_channel_info_->AddChildView(CreateInfoLabel());
  }

  views::Label* version_label =
      static_cast<views::Label*>(dev_channel_info_->child_at(0));
  version_label->SetVisible(!os_version_label_text.empty());
  version_label->SetText(base::UTF8ToUTF16(os_version_label_text));

  views::Label* enterprise_label =
      static_cast<views::Label*>(dev_channel_info_->child_at(1));
  enterprise_label->SetVisible(!enterprise_info_text.empty());
  enterprise_label->SetText(base::UTF8ToUTF16(enterprise_info_text));

  views::Label* bluetooth_label =
      static_cast<views::Label*>(dev_channel_info_->child_at(2));
  bluetooth_label->SetVisible(!bluetooth_name.empty());
  bluetooth_label->SetText(base::UTF8ToUTF16(bluetooth_name));

  LayoutTopHeader();
}

void LockContentsView::OnPublicSessionDisplayNameChanged(
    const AccountId& account_id,
    const std::string& display_name) {
  LoginUserView* user_view = TryToFindUserView(account_id);
  if (!user_view || !IsPublicAccountUser(user_view->current_user()))
    return;

  mojom::LoginUserInfoPtr user_info = user_view->current_user()->Clone();
  user_info->basic_user_info->display_name = display_name;
  user_view->UpdateForUser(user_info, false /*animate*/);
}

void LockContentsView::OnPublicSessionLocalesChanged(
    const AccountId& account_id,
    const std::vector<mojom::LocaleItemPtr>& locales,
    const std::string& default_locale,
    bool show_advanced_view) {
  LoginUserView* user_view = TryToFindUserView(account_id);
  if (!user_view || !IsPublicAccountUser(user_view->current_user()))
    return;

  mojom::LoginUserInfoPtr user_info = user_view->current_user()->Clone();
  user_info->public_account_info->available_locales = mojo::Clone(locales);
  user_info->public_account_info->default_locale = default_locale;
  user_info->public_account_info->show_advanced_view = show_advanced_view;
  user_view->UpdateForUser(user_info, false /*animate*/);
}

void LockContentsView::OnPublicSessionKeyboardLayoutsChanged(
    const AccountId& account_id,
    const std::string& locale,
    const std::vector<mojom::InputMethodItemPtr>& keyboard_layouts) {
  // Update expanded view because keyboard layouts is user interactive content.
  // I.e. user selects a language locale and the corresponding keyboard layouts
  // will be changed.
  if (expanded_view_->visible() &&
      expanded_view_->current_user()->basic_user_info->account_id ==
          account_id) {
    mojom::LoginUserInfoPtr user_info = expanded_view_->current_user()->Clone();
    user_info->public_account_info->default_locale = locale;
    user_info->public_account_info->keyboard_layouts =
        mojo::Clone(keyboard_layouts);
    expanded_view_->UpdateForUser(user_info);
  }

  LoginUserView* user_view = TryToFindUserView(account_id);
  if (!user_view || !IsPublicAccountUser(user_view->current_user())) {
    LOG(ERROR) << "Unable to find public account user.";
    return;
  }

  mojom::LoginUserInfoPtr user_info = user_view->current_user()->Clone();
  // Skip updating keyboard layouts if |locale| is not the default locale
  // of the user. I.e. user changed the default locale in the expanded view,
  // and it should be handled by expanded view.
  if (user_info->public_account_info->default_locale != locale)
    return;

  user_info->public_account_info->keyboard_layouts =
      mojo::Clone(keyboard_layouts);
  user_view->UpdateForUser(user_info, false /*animate*/);
}

void LockContentsView::OnDetachableBasePairingStatusChanged(
    DetachableBasePairingStatus pairing_status) {
  const mojom::UserInfoPtr& user_info =
      CurrentBigUserView()->GetCurrentUser()->basic_user_info;
  // If the current big user is public account user, or the base is not paired,
  // or the paired base matches the last used by the current user, the
  // detachable base error bubble should be hidden. Otherwise, the bubble should
  // be shown.
  if (!CurrentBigUserView()->auth_user() ||
      pairing_status == DetachableBasePairingStatus::kNone ||
      (pairing_status == DetachableBasePairingStatus::kAuthenticated &&
       detachable_base_model_->PairedBaseMatchesLastUsedByUser(*user_info))) {
    detachable_base_error_bubble_->Close();
    return;
  }

  auth_error_bubble_->Close();

  base::string16 error_text =
      l10n_util::GetStringUTF16(IDS_ASH_LOGIN_ERROR_DETACHABLE_BASE_CHANGED);

  views::Label* label =
      new views::Label(error_text, views::style::CONTEXT_MESSAGE_BOX_BODY_TEXT,
                       views::style::STYLE_PRIMARY);
  label->SetMultiLine(true);
  label->SetAutoColorReadabilityEnabled(false);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetEnabledColor(SK_ColorWHITE);

  detachable_base_error_bubble_->ShowErrorBubble(
      label, CurrentBigUserView()->auth_user()->password_view() /*anchor_view*/,
      LoginBubble::kFlagPersistent);

  // Remove the focus from the password field, to make user less likely to enter
  // the password without seeing the warning about detachable base change.
  if (GetWidget()->IsActive())
    GetWidget()->GetFocusManager()->ClearFocus();
}

void LockContentsView::OnFingerprintUnlockStateChanged(
    const AccountId& account_id,
    mojom::FingerprintUnlockState state) {
  UserState* user_state = FindStateForUser(account_id);
  if (!user_state)
    return;

  user_state->fingerprint_state = state;
  LoginBigUserView* big_view =
      TryToFindBigUser(account_id, true /*require_auth_active*/);
  if (!big_view || !big_view->auth_user())
    return;

  big_view->auth_user()->SetFingerprintState(user_state->fingerprint_state);
  LayoutAuth(big_view, nullptr /*opt_to_hide*/, true /*animate*/);
}

void LockContentsView::SetAvatarForUser(const AccountId& account_id,
                                        const mojom::UserAvatarPtr& avatar) {
  auto replace = [&](const ash::mojom::LoginUserInfoPtr& user) {
    auto changed = user->Clone();
    changed->basic_user_info->avatar = avatar->Clone();
    return changed;
  };

  LoginBigUserView* big =
      TryToFindBigUser(account_id, false /*require_auth_active*/);
  if (big) {
    big->UpdateForUser(replace(big->GetCurrentUser()));
    return;
  }

  LoginUserView* user =
      users_list_ ? users_list_->GetUserView(account_id) : nullptr;
  if (user) {
    user->UpdateForUser(replace(user->current_user()), false /*animate*/);
    return;
  }
}

void LockContentsView::OnFocusLeavingLockScreenApps(bool reverse) {
  if (!reverse || lock_screen_apps_active_)
    FocusNextWidget(reverse);
  else
    FindFirstOrLastFocusableChild(this, reverse)->RequestFocus();
}

void LockContentsView::OnFocusLeavingSystemTray(bool reverse) {
  // This function is called when the system tray is losing focus. We want to
  // focus the first or last child in this view, or a lock screen app window if
  // one is active (in which case lock contents should not have focus). In the
  // later case, still focus lock screen first, to synchronously take focus away
  // from the system shelf (or tray) - lock shelf view expect the focus to be
  // taken when it passes it to lock screen view, and can misbehave in case the
  // focus is kept in it.
  FindFirstOrLastFocusableChild(this, reverse)->RequestFocus();

  if (lock_screen_apps_active_) {
    Shell::Get()->login_screen_controller()->FocusLockScreenApps(reverse);
    return;
  }
}

void LockContentsView::OnDisplayMetricsChanged(const display::Display& display,
                                               uint32_t changed_metrics) {
  // Ignore all metric changes except rotation.
  if ((changed_metrics & DISPLAY_METRIC_ROTATION) == 0)
    return;

  DoLayout();
}

void LockContentsView::OnLockStateChanged(bool locked) {
  if (!locked) {
    // Successfully unlock the screen.
    Shell::Get()->metrics()->login_metrics_recorder()->RecordNumLoginAttempts(
        unlock_attempt_, true /*success*/);
    unlock_attempt_ = 0;
  }
}

void LockContentsView::OnVirtualKeyboardStateChanged(
    bool activated,
    aura::Window* root_window) {
  const views::Widget* widget = GetWidget();
  if (widget) {
    UpdateKeyboardObserverFromStateChanged(
        activated, root_window, widget->GetNativeWindow()->GetRootWindow(),
        &keyboard_observer_);
  }
}

void LockContentsView::OnStateChanged(
    const keyboard::KeyboardControllerState state) {
  if (state == keyboard::KeyboardControllerState::SHOWN ||
      state == keyboard::KeyboardControllerState::HIDDEN) {
    LayoutAuth(primary_big_view_, opt_secondary_big_view_, false /*animate*/);
  }
}

void LockContentsView::FocusNextWidget(bool reverse) {
  Shelf* shelf = Shelf::ForWindow(GetWidget()->GetNativeWindow());
  // Tell the focus direction to the status area or the shelf so they can focus
  // the correct child view.
  if (reverse) {
    shelf->GetStatusAreaWidget()
        ->status_area_widget_delegate()
        ->set_default_last_focusable_child(reverse);
    Shell::Get()->focus_cycler()->FocusWidget(shelf->GetStatusAreaWidget());
  } else {
    shelf->shelf_widget()->set_default_last_focusable_child(reverse);
    Shell::Get()->focus_cycler()->FocusWidget(shelf->shelf_widget());
  }
}

void LockContentsView::CreateLowDensityLayout(
    const std::vector<mojom::LoginUserInfoPtr>& users) {
  DCHECK_EQ(users.size(), 2u);

  // Space between auth user and alternative user.
  main_view_->AddChildView(MakeOrientationViewWithWidths(
      kLowDensityDistanceBetweenUsersInLandscapeDp,
      kLowDensityDistanceBetweenUsersInPortraitDp));

  // Build auth user.
  opt_secondary_big_view_ =
      AllocateLoginBigUserView(users[1], false /*is_primary*/);
  main_view_->AddChildView(opt_secondary_big_view_);
}

void LockContentsView::CreateMediumDensityLayout(
    const std::vector<mojom::LoginUserInfoPtr>& users) {
  // Insert spacing before (left of) auth.
  main_view_->AddChildViewAt(MakeOrientationViewWithWidths(
                                 kMediumDensityMarginLeftOfAuthUserLandscapeDp,
                                 kMediumDensityMarginLeftOfAuthUserPortraitDp),
                             0);
  // Insert spacing between auth and user list.
  main_view_->AddChildView(MakeOrientationViewWithWidths(
      kMediumDensityDistanceBetweenAuthUserAndUsersLandscapeDp,
      kMediumDensityDistanceBetweenAuthUserAndUsersPortraitDp));

  users_list_ = BuildScrollableUsersListView(users, LoginDisplayStyle::kSmall);
  main_view_->AddChildView(users_list_);

  // Insert dynamic spacing on left/right of the content which changes based on
  // screen rotation and display size.
  auto* left = new NonAccessibleView();
  main_view_->AddChildViewAt(left, 0);
  auto* right = new NonAccessibleView();
  main_view_->AddChildView(right);
  AddRotationAction(base::BindRepeating(
      [](views::BoxLayout* layout, views::View* left, views::View* right,
         bool landscape) {
        if (landscape) {
          layout->SetFlexForView(left, 1);
          layout->SetFlexForView(right, 1);
        } else {
          layout->SetFlexForView(left, 2);
          layout->SetFlexForView(right, 1);
        }
      },
      main_layout_, left, right));
}

void LockContentsView::CreateHighDensityLayout(
    const std::vector<mojom::LoginUserInfoPtr>& users) {
  // Insert spacing before and after the auth view.
  auto* fill = new NonAccessibleView();
  main_view_->AddChildViewAt(fill, 0);
  main_layout_->SetFlexForView(fill, 1);

  fill = new NonAccessibleView();
  main_view_->AddChildView(fill);
  main_layout_->SetFlexForView(fill, 1);

  users_list_ =
      BuildScrollableUsersListView(users, LoginDisplayStyle::kExtraSmall);
  main_view_->AddChildView(users_list_);
}

void LockContentsView::DoLayout() {
  bool landscape = login_layout_util::ShouldShowLandscape(GetWidget());
  for (auto& action : rotation_actions_)
    action.Run(landscape);

  const display::Display& display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(
          GetWidget()->GetNativeWindow());
  SetPreferredSize(display.size());
  SizeToPreferredSize();
  Layout();
}

void LockContentsView::LayoutTopHeader() {
  int preferred_width = dev_channel_info_->GetPreferredSize().width() +
                        note_action_->GetPreferredSize().width();
  int preferred_height =
      std::max(dev_channel_info_->GetPreferredSize().height(),
               note_action_->GetPreferredSize().height());
  top_header_->SetPreferredSize(gfx::Size(preferred_width, preferred_height));
  top_header_->SizeToPreferredSize();
  top_header_->Layout();
  // Position the top header - the origin is offset to the left from the top
  // right corner of the entire view by the width of this top header view.
  top_header_->SetPosition(GetLocalBounds().top_right() -
                           gfx::Vector2d(preferred_width, 0));
}

void LockContentsView::LayoutPublicSessionView() {
  gfx::Rect bounds = GetContentsBounds();
  bounds.ClampToCenteredSize(expanded_view_->GetPreferredSize());
  expanded_view_->SetBoundsRect(bounds);
}

views::View* LockContentsView::MakeOrientationViewWithWidths(int landscape,
                                                             int portrait) {
  auto* view = new MultiSizedView(gfx::Size(landscape, kNonEmptyHeightDp),
                                  gfx::Size(portrait, kNonEmptyHeightDp));
  AddRotationAction(base::BindRepeating(&MultiSizedView::SwapPreferredSizeTo,
                                        base::Unretained(view)));
  return view;
}

void LockContentsView::AddRotationAction(const OnRotate& on_rotate) {
  on_rotate.Run(login_layout_util::ShouldShowLandscape(GetWidget()));
  rotation_actions_.push_back(on_rotate);
}

void LockContentsView::SwapActiveAuthBetweenPrimaryAndSecondary(
    bool is_primary) {
  if (is_primary && !primary_big_view_->IsAuthEnabled()) {
    LayoutAuth(primary_big_view_, opt_secondary_big_view_, true /*animate*/);
    OnBigUserChanged();
  } else if (!is_primary && opt_secondary_big_view_ &&
             !opt_secondary_big_view_->IsAuthEnabled()) {
    LayoutAuth(opt_secondary_big_view_, primary_big_view_, true /*animate*/);
    OnBigUserChanged();
  }
}

void LockContentsView::OnAuthenticate(bool auth_success) {
  if (auth_success) {
    auth_error_bubble_->Close();
    detachable_base_error_bubble_->Close();

    // Now that the user has been authenticated, update the user's last used
    // detachable base (if one is attached). This will prevent further
    // detachable base change notifications from appearing for this base (until
    // the user uses another detachable base).
    if (CurrentBigUserView()->auth_user() &&
        detachable_base_model_->GetPairingStatus() ==
            DetachableBasePairingStatus::kAuthenticated) {
      detachable_base_model_->SetPairedBaseAsLastUsedByUser(
          *CurrentBigUserView()->GetCurrentUser()->basic_user_info);
    }
  } else {
    ShowAuthErrorMessage();
    ++unlock_attempt_;
  }
}

LockContentsView::UserState* LockContentsView::FindStateForUser(
    const AccountId& user) {
  for (UserState& state : users_) {
    if (state.account_id == user)
      return &state;
  }

  return nullptr;
}

void LockContentsView::LayoutAuth(LoginBigUserView* to_update,
                                  LoginBigUserView* opt_to_hide,
                                  bool animate) {
  DCHECK(to_update);
  UpdateAuthForAuthUser(to_update->auth_user(),
                        opt_to_hide ? opt_to_hide->auth_user() : nullptr,
                        animate);
  UpdateAuthForPublicAccount(
      to_update->public_account(),
      opt_to_hide ? opt_to_hide->public_account() : nullptr, animate);
}

void LockContentsView::SwapToBigUser(int user_index) {
  DCHECK(users_list_);
  LoginUserView* view = users_list_->user_view_at(user_index);
  DCHECK(view);
  mojom::LoginUserInfoPtr previous_big_user =
      primary_big_view_->GetCurrentUser()->Clone();
  mojom::LoginUserInfoPtr new_big_user = view->current_user()->Clone();

  view->UpdateForUser(previous_big_user, true /*animate*/);
  primary_big_view_->UpdateForUser(new_big_user);
  LayoutAuth(primary_big_view_, nullptr, true /*animate*/);
  OnBigUserChanged();
}

void LockContentsView::OnRemoveUserWarningShown(bool is_primary) {
  Shell::Get()->login_screen_controller()->OnRemoveUserWarningShown();
}

void LockContentsView::RemoveUser(bool is_primary) {
  LoginBigUserView* to_remove =
      is_primary ? primary_big_view_ : opt_secondary_big_view_;
  DCHECK(to_remove->GetCurrentUser()->can_remove);
  AccountId user = to_remove->GetCurrentUser()->basic_user_info->account_id;

  // Ask chrome to remove the user.
  Shell::Get()->login_screen_controller()->RemoveUser(user);

  // Display the new user list less |user|.
  std::vector<mojom::LoginUserInfoPtr> new_users;
  if (!is_primary)
    new_users.push_back(primary_big_view_->GetCurrentUser()->Clone());
  if (is_primary && opt_secondary_big_view_)
    new_users.push_back(opt_secondary_big_view_->GetCurrentUser()->Clone());
  if (users_list_) {
    for (int i = 0; i < users_list_->user_count(); ++i) {
      new_users.push_back(
          users_list_->user_view_at(i)->current_user()->Clone());
    }
  }
  data_dispatcher_->NotifyUsers(new_users);
}

void LockContentsView::OnBigUserChanged() {
  const AccountId new_big_user =
      CurrentBigUserView()->GetCurrentUser()->basic_user_info->account_id;

  Shell::Get()->login_screen_controller()->OnFocusPod(new_big_user);
  UpdateEasyUnlockIconForUser(new_big_user);

  if (unlock_attempt_ > 0) {
    // Times a password was incorrectly entered until user gives up (change
    // user pod).
    Shell::Get()->metrics()->login_metrics_recorder()->RecordNumLoginAttempts(
        unlock_attempt_, false /*success*/);

    // Reset unlock attempt when the auth user changes.
    unlock_attempt_ = 0;
  }

  // The new auth user might have different last used detachable base - make
  // sure the detachable base pairing error is updated if needed.
  OnDetachableBasePairingStatusChanged(
      detachable_base_model_->GetPairingStatus());
}

void LockContentsView::UpdateEasyUnlockIconForUser(const AccountId& user) {
  // Try to find an big view for |user|. If there is none, there is no state to
  // update.
  LoginBigUserView* big_view =
      TryToFindBigUser(user, false /*require_auth_active*/);
  if (!big_view || !big_view->auth_user())
    return;

  UserState* state = FindStateForUser(user);
  DCHECK(state);

  // Hide easy unlock icon if there is no data is available.
  if (!state->easy_unlock_state) {
    big_view->auth_user()->SetEasyUnlockIcon(mojom::EasyUnlockIconId::NONE,
                                             base::string16());
    return;
  }

  // TODO(jdufault): Make easy unlock backend always send aria_label, right now
  // it is only sent if there is no tooltip.
  base::string16 accessibility_label = state->easy_unlock_state->aria_label;
  if (accessibility_label.empty())
    accessibility_label = state->easy_unlock_state->tooltip;

  big_view->auth_user()->SetEasyUnlockIcon(state->easy_unlock_state->icon,
                                           accessibility_label);
}

LoginBigUserView* LockContentsView::CurrentBigUserView() {
  if (opt_secondary_big_view_ && opt_secondary_big_view_->IsAuthEnabled()) {
    DCHECK(!primary_big_view_->IsAuthEnabled());
    return opt_secondary_big_view_;
  }

  return primary_big_view_;
}

void LockContentsView::ShowAuthErrorMessage() {
  LoginBigUserView* big_view = CurrentBigUserView();
  if (!big_view->auth_user())
    return;

  base::string16 error_text = l10n_util::GetStringUTF16(
      unlock_attempt_ ? IDS_ASH_LOGIN_ERROR_AUTHENTICATING_2ND_TIME
                      : IDS_ASH_LOGIN_ERROR_AUTHENTICATING);
  ImeController* ime_controller = Shell::Get()->ime_controller();
  if (ime_controller->IsCapsLockEnabled()) {
    error_text += base::ASCIIToUTF16(" ") +
                  l10n_util::GetStringUTF16(IDS_ASH_LOGIN_ERROR_CAPS_LOCK_HINT);
  }

  base::Optional<int> bold_start;
  int bold_length = 0;
  // Display a hint to switch keyboards if there are other active input
  // methods.
  if (ime_controller->available_imes().size() > 1) {
    error_text += base::ASCIIToUTF16(" ");
    bold_start = error_text.length();
    base::string16 shortcut =
        l10n_util::GetStringUTF16(IDS_ASH_LOGIN_KEYBOARD_SWITCH_SHORTCUT);
    bold_length = shortcut.length();

    size_t shortcut_offset_in_string;
    error_text +=
        l10n_util::GetStringFUTF16(IDS_ASH_LOGIN_ERROR_KEYBOARD_SWITCH_HINT,
                                   shortcut, &shortcut_offset_in_string);
    *bold_start += shortcut_offset_in_string;
  }

  views::StyledLabel* label = new views::StyledLabel(error_text, this);
  MakeSectionBold(label, error_text, bold_start, bold_length);
  label->set_auto_color_readability_enabled(false);

  auth_error_bubble_->ShowErrorBubble(
      label, big_view->auth_user()->password_view() /*anchor_view*/,
      LoginBubble::kFlagsNone);
}

void LockContentsView::OnEasyUnlockIconHovered() {
  LoginBigUserView* big_view = CurrentBigUserView();
  if (!big_view->auth_user())
    return;

  UserState* state =
      FindStateForUser(big_view->GetCurrentUser()->basic_user_info->account_id);
  DCHECK(state);
  mojom::EasyUnlockIconOptionsPtr& easy_unlock_state = state->easy_unlock_state;
  DCHECK(easy_unlock_state);

  if (!easy_unlock_state->tooltip.empty()) {
    tooltip_bubble_->ShowTooltip(
        easy_unlock_state->tooltip,
        big_view->auth_user()->password_view() /*anchor_view*/);
  }
}

void LockContentsView::OnEasyUnlockIconTapped() {
  UserState* state = FindStateForUser(
      CurrentBigUserView()->GetCurrentUser()->basic_user_info->account_id);
  DCHECK(state);
  mojom::EasyUnlockIconOptionsPtr& easy_unlock_state = state->easy_unlock_state;
  DCHECK(easy_unlock_state);

  if (easy_unlock_state->hardlock_on_click) {
    AccountId user =
        CurrentBigUserView()->GetCurrentUser()->basic_user_info->account_id;
    Shell::Get()->login_screen_controller()->HardlockPod(user);
    // TODO(jdufault): This should get called as a result of HardlockPod.
    OnClickToUnlockEnabledForUserChanged(user, false /*enabled*/);
  }
}

keyboard::KeyboardController* LockContentsView::GetKeyboardController() const {
  return GetWidget() ? GetKeyboardControllerForWidget(GetWidget()) : nullptr;
}

void LockContentsView::OnPublicAccountTapped() {
  // Update expanded_view_ in case CurrentBigUserView has changed.
  // 1. It happens when the active big user is changed. For example both
  // primary and secondary big user are public account and user switches from
  // primary to secondary.
  // 2. LoginUserInfo in the big user could be changed if we get updates from
  // OnPublicSessionDisplayNameChanged and OnPublicSessionLocalesChanged.
  expanded_view_->UpdateForUser(CurrentBigUserView()->GetCurrentUser());
  SetDisplayStyle(DisplayStyle::kExclusivePublicAccountExpandedView);
}

LoginBigUserView* LockContentsView::AllocateLoginBigUserView(
    const mojom::LoginUserInfoPtr& user,
    bool is_primary) {
  LoginAuthUserView::Callbacks auth_user_callbacks;
  auth_user_callbacks.on_auth = base::BindRepeating(
      &LockContentsView::OnAuthenticate, base::Unretained(this)),
  auth_user_callbacks.on_tap = base::BindRepeating(
      &LockContentsView::SwapActiveAuthBetweenPrimaryAndSecondary,
      base::Unretained(this), is_primary),
  auth_user_callbacks.on_remove_warning_shown =
      base::BindRepeating(&LockContentsView::OnRemoveUserWarningShown,
                          base::Unretained(this), is_primary);
  auth_user_callbacks.on_remove = base::BindRepeating(
      &LockContentsView::RemoveUser, base::Unretained(this), is_primary);
  auth_user_callbacks.on_easy_unlock_icon_hovered = base::BindRepeating(
      &LockContentsView::OnEasyUnlockIconHovered, base::Unretained(this));
  auth_user_callbacks.on_easy_unlock_icon_tapped = base::BindRepeating(
      &LockContentsView::OnEasyUnlockIconTapped, base::Unretained(this));

  LoginPublicAccountUserView::Callbacks public_account_callbacks;
  public_account_callbacks.on_tap = auth_user_callbacks.on_tap;
  public_account_callbacks.on_public_account_tapped = base::BindRepeating(
      &LockContentsView::OnPublicAccountTapped, base::Unretained(this));
  return new LoginBigUserView(user, auth_user_callbacks,
                              public_account_callbacks);
}

LoginBigUserView* LockContentsView::TryToFindBigUser(const AccountId& user,
                                                     bool require_auth_active) {
  LoginBigUserView* view = nullptr;

  // Find auth instance.
  if (primary_big_view_->GetCurrentUser()->basic_user_info->account_id ==
      user) {
    view = primary_big_view_;
  } else if (opt_secondary_big_view_ &&
             opt_secondary_big_view_->GetCurrentUser()
                     ->basic_user_info->account_id == user) {
    view = opt_secondary_big_view_;
  }

  // Make sure auth instance is active if required.
  if (require_auth_active && view && !view->IsAuthEnabled())
    view = nullptr;

  return view;
}

LoginUserView* LockContentsView::TryToFindUserView(const AccountId& user) {
  // Try to find |user| in big user view first.
  LoginBigUserView* big_view =
      TryToFindBigUser(user, false /*require_auth_active*/);
  if (big_view)
    return big_view->GetUserView();

  // Try to find |user| in users_list_.
  return users_list_->GetUserView(user);
}

ScrollableUsersListView* LockContentsView::BuildScrollableUsersListView(
    const std::vector<mojom::LoginUserInfoPtr>& users,
    LoginDisplayStyle display_style) {
  auto* view = new ScrollableUsersListView(
      users,
      base::BindRepeating(&LockContentsView::SwapToBigUser,
                          base::Unretained(this)),
      display_style);
  view->ClipHeightTo(view->contents()->size().height(), size().height());
  return view;
}

void LockContentsView::UpdateAuthForPublicAccount(
    LoginPublicAccountUserView* opt_to_update,
    LoginPublicAccountUserView* opt_to_hide,
    bool animate) {
  if (opt_to_update)
    opt_to_update->SetAuthEnabled(true /*enabled*/, animate);
  if (opt_to_hide)
    opt_to_hide->SetAuthEnabled(false /*enabled*/, animate);
}

void LockContentsView::UpdateAuthForAuthUser(LoginAuthUserView* opt_to_update,
                                             LoginAuthUserView* opt_to_hide,
                                             bool animate) {
  // Capture animation metadata before we changing state.
  if (animate) {
    if (opt_to_update)
      opt_to_update->CaptureStateForAnimationPreLayout();
    if (opt_to_hide)
      opt_to_hide->CaptureStateForAnimationPreLayout();
  }

  // Update auth methods for |opt_to_update|. Disable auth on |opt_to_hide|.
  if (opt_to_update) {
    UserState* state = FindStateForUser(
        opt_to_update->current_user()->basic_user_info->account_id);
    uint32_t to_update_auth;
    if (state->force_online_sign_in) {
      to_update_auth = LoginAuthUserView::AUTH_ONLINE_SIGN_IN;
    } else if (state->disable_auth) {
      to_update_auth = LoginAuthUserView::AUTH_DISABLED;
    } else {
      to_update_auth = LoginAuthUserView::AUTH_PASSWORD;
      keyboard::KeyboardController* keyboard_controller =
          GetKeyboardController();
      const bool keyboard_visible =
          keyboard_controller ? keyboard_controller->keyboard_visible() : false;
      if (state->show_pin && !keyboard_visible &&
          state->fingerprint_state ==
              mojom::FingerprintUnlockState::UNAVAILABLE) {
        to_update_auth |= LoginAuthUserView::AUTH_PIN;
      }
      if (state->enable_tap_auth)
        to_update_auth |= LoginAuthUserView::AUTH_TAP;
      if (state->fingerprint_state !=
          mojom::FingerprintUnlockState::UNAVAILABLE) {
        to_update_auth |= LoginAuthUserView::AUTH_FINGERPRINT;
      }
    }
    opt_to_update->SetAuthMethods(to_update_auth);
  }
  if (opt_to_hide)
    opt_to_hide->SetAuthMethods(LoginAuthUserView::AUTH_NONE);

  Layout();

  // Apply animations.
  if (animate) {
    if (opt_to_update)
      opt_to_update->ApplyAnimationPostLayout();
    if (opt_to_hide)
      opt_to_hide->ApplyAnimationPostLayout();
  }
}

void LockContentsView::SetDisplayStyle(DisplayStyle style) {
  const bool show_expanded_view =
      style == DisplayStyle::kExclusivePublicAccountExpandedView;
  expanded_view_->SetVisible(show_expanded_view);
  main_view_->SetVisible(!show_expanded_view);
  top_header_->SetVisible(!show_expanded_view);
  Layout();
}

void LockContentsView::DisableLockScreenNote() {
  disable_lock_screen_note_ = true;
  OnLockScreenNoteStateChanged(mojom::TrayActionState::kNotAvailable);
}

}  // namespace ash
