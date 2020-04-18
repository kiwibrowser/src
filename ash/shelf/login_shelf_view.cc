// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/login_shelf_view.h"

#include <memory>
#include <utility>

#include "ash/focus_cycler.h"
#include "ash/lock_screen_action/lock_screen_action_background_controller.h"
#include "ash/lock_screen_action/lock_screen_action_background_state.h"
#include "ash/login/login_screen_controller.h"
#include "ash/login/ui/lock_screen.h"
#include "ash/login/ui/lock_window.h"
#include "ash/public/cpp/ash_constants.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_constants.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/shutdown_controller.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/status_area_widget_delegate.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wm/lock_state_controller.h"
#include "base/metrics/user_metrics.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/accessibility/ax_aura_obj_cache.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/focus/focus_search.h"
#include "ui/views/layout/box_layout.h"

using session_manager::SessionState;

namespace ash {
namespace {

LoginMetricsRecorder::ShelfButtonClickTarget GetUserClickTarget(int button_id) {
  switch (button_id) {
    case LoginShelfView::kShutdown:
      return LoginMetricsRecorder::ShelfButtonClickTarget::kShutDownButton;
    case LoginShelfView::kRestart:
      return LoginMetricsRecorder::ShelfButtonClickTarget::kRestartButton;
    case LoginShelfView::kSignOut:
      return LoginMetricsRecorder::ShelfButtonClickTarget::kSignOutButton;
    case LoginShelfView::kCloseNote:
      return LoginMetricsRecorder::ShelfButtonClickTarget::kCloseNoteButton;
    case LoginShelfView::kBrowseAsGuest:
      return LoginMetricsRecorder::ShelfButtonClickTarget::kBrowseAsGuestButton;
    case LoginShelfView::kAddUser:
      return LoginMetricsRecorder::ShelfButtonClickTarget::kAddUserButton;
    case LoginShelfView::kCancel:
      return LoginMetricsRecorder::ShelfButtonClickTarget::kCancelButton;
  }
  return LoginMetricsRecorder::ShelfButtonClickTarget::kTargetCount;
}

// Spacing between the button image and label.
constexpr int kImageLabelSpacingDp = 8;

// The width of the four margins of each button.
constexpr int kButtonMarginDp = 13;

// The color of the button image and label.
constexpr SkColor kButtonColor = SK_ColorWHITE;

class LoginShelfButton : public views::LabelButton {
 public:
  LoginShelfButton(views::ButtonListener* listener,
                   const base::string16& text,
                   const gfx::ImageSkia& image)
      : LabelButton(listener, text) {
    SetAccessibleName(text);
    SetImage(views::Button::STATE_NORMAL, image);
    SetFocusBehavior(FocusBehavior::ALWAYS);
    SetFocusPainter(views::Painter::CreateSolidFocusPainter(
        kFocusBorderColor, kFocusBorderThickness, gfx::InsetsF()));
    SetInkDropMode(InkDropMode::ON);
    set_has_ink_drop_action_on_click(true);
    set_ink_drop_base_color(kShelfInkDropBaseColor);
    set_ink_drop_visible_opacity(kShelfInkDropVisibleOpacity);
    SetTextSubpixelRenderingEnabled(false);

    SetImageLabelSpacing(kImageLabelSpacingDp);
    SetTextColor(views::Button::STATE_NORMAL, kButtonColor);
    SetTextColor(views::Button::STATE_HOVERED, kButtonColor);
    SetTextColor(views::Button::STATE_PRESSED, kButtonColor);
    label()->SetFontList(views::Label::GetDefaultFontList().Derive(
        1, gfx::Font::FontStyle::NORMAL, gfx::Font::Weight::NORMAL));
  }

  ~LoginShelfButton() override = default;

  // views::View:
  gfx::Insets GetInsets() const override {
    return gfx::Insets(kButtonMarginDp);
  }

  // views::InkDropHostView:
  std::unique_ptr<views::InkDrop> CreateInkDrop() override {
    std::unique_ptr<views::InkDropImpl> ink_drop =
        std::make_unique<views::InkDropImpl>(this, size());
    ink_drop->SetShowHighlightOnHover(false);
    ink_drop->SetShowHighlightOnFocus(false);
    return std::move(ink_drop);
  }
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override {
    gfx::InsetsF insets(ash::kHitRegionPadding, ash::kHitRegionPadding);
    return std::make_unique<views::RoundRectInkDropMask>(
        size(), insets, kTrayRoundedBorderRadius);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginShelfButton);
};

}  // namespace

LoginShelfView::LoginShelfView(
    LockScreenActionBackgroundController* lock_screen_action_background)
    : lock_screen_action_background_(lock_screen_action_background),
      tray_action_observer_(this),
      lock_screen_action_background_observer_(this),
      shutdown_controller_observer_(this) {
  // We reuse the focusable state on this view as a signal that focus should
  // switch to the lock screen or status area. This view should otherwise not
  // be focusable.
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal));

  auto add_button = [this](ButtonId id, int text_resource_id,
                           const gfx::VectorIcon& icon) {
    const base::string16 text = l10n_util::GetStringUTF16(text_resource_id);
    gfx::ImageSkia image = CreateVectorIcon(icon, kButtonColor);
    LoginShelfButton* button = new LoginShelfButton(this, text, image);
    button->set_id(id);
    AddChildView(button);
  };
  add_button(kShutdown, IDS_ASH_SHELF_SHUTDOWN_BUTTON,
             kShelfShutdownButtonIcon);
  add_button(kRestart, IDS_ASH_SHELF_RESTART_BUTTON, kShelfShutdownButtonIcon);
  add_button(kSignOut, IDS_ASH_SHELF_SIGN_OUT_BUTTON, kShelfSignOutButtonIcon);
  add_button(kCloseNote, IDS_ASH_SHELF_UNLOCK_BUTTON, kShelfUnlockButtonIcon);
  add_button(kCancel, IDS_ASH_SHELF_CANCEL_BUTTON, kShelfCancelButtonIcon);
  add_button(kBrowseAsGuest, IDS_ASH_BROWSE_AS_GUEST_BUTTON,
             kShelfBrowseAsGuestButtonIcon);
  add_button(kAddUser, IDS_ASH_ADD_USER_BUTTON, kShelfAddPersonButtonIcon);
  add_button(kShowWebUiLogin, IDS_ASH_SHOW_WEBUI_LOGIN_BUTTON,
             kShelfSignOutButtonIcon);

  // Adds observers for states that affect the visiblity of different buttons.
  tray_action_observer_.Add(Shell::Get()->tray_action());
  shutdown_controller_observer_.Add(Shell::Get()->shutdown_controller());
  lock_screen_action_background_observer_.Add(lock_screen_action_background);
  UpdateUi();
}

LoginShelfView::~LoginShelfView() = default;

void LoginShelfView::UpdateAfterSessionStateChange(SessionState state) {
  UpdateUi();
}

const char* LoginShelfView::GetClassName() const {
  return "LoginShelfView";
}

void LoginShelfView::OnFocus() {
  LOG(WARNING) << "LoginShelfView was focused, but this should never happen. "
                  "Forwarded focus to shelf widget with an unknown direction.";
  Shell::Get()->focus_cycler()->FocusWidget(
      Shelf::ForWindow(GetWidget()->GetNativeWindow())->shelf_widget());
}

void LoginShelfView::AboutToRequestFocusFromTabTraversal(bool reverse) {
  if (reverse) {
    // Focus should leave the system tray.
    Shell::Get()->system_tray_notifier()->NotifyFocusOut(reverse);
  } else {
    // Focus goes to status area.
    Shelf::ForWindow(GetWidget()->GetNativeWindow())
        ->GetStatusAreaWidget()
        ->status_area_widget_delegate()
        ->set_default_last_focusable_child(reverse);
    Shell::Get()->focus_cycler()->FocusWidget(
        Shelf::ForWindow(GetWidget()->GetNativeWindow())
            ->GetStatusAreaWidget());
  }
}

void LoginShelfView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  if (LockScreen::IsShown()) {
    int previous_id = views::AXAuraObjCache::GetInstance()->GetID(
        static_cast<views::Widget*>(LockScreen::Get()->window()));
    node_data->AddIntAttribute(ax::mojom::IntAttribute::kPreviousFocusId,
                               previous_id);
  }

  Shelf* shelf = Shelf::ForWindow(GetWidget()->GetNativeWindow());
  int next_id =
      views::AXAuraObjCache::GetInstance()->GetID(shelf->GetStatusAreaWidget());
  node_data->AddIntAttribute(ax::mojom::IntAttribute::kNextFocusId, next_id);
  node_data->role = ax::mojom::Role::kToolbar;
  node_data->SetName(l10n_util::GetStringUTF8(IDS_ASH_SHELF_ACCESSIBLE_NAME));
}

void LoginShelfView::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  // Intentional crash. session_manager will add --show-webui-login.
  CHECK(sender->id() != kShowWebUiLogin);

  UserMetricsRecorder::RecordUserClickOnShelfButton(
      GetUserClickTarget(sender->id()));
  switch (sender->id()) {
    case kShutdown:
    case kRestart:
      // |ShutdownController| will further distinguish the two cases based on
      // shutdown policy.
      Shell::Get()->lock_state_controller()->RequestShutdown(
          ShutdownReason::LOGIN_SHUT_DOWN_BUTTON);
      break;
    case kSignOut:
      base::RecordAction(base::UserMetricsAction("ScreenLocker_Signout"));
      Shell::Get()->session_controller()->RequestSignOut();
      break;
    case kCloseNote:
      Shell::Get()->tray_action()->CloseLockScreenNote(
          mojom::CloseLockScreenNoteReason::kUnlockButtonPressed);
      break;
    case kCancel:
      // If the Cancel button has focus, clear it. Otherwise the shelf within
      // active session may still be focused.
      GetFocusManager()->ClearFocus();
      Shell::Get()->login_screen_controller()->CancelAddUser();
      break;
    case kBrowseAsGuest:
      Shell::Get()->login_screen_controller()->LoginAsGuest();
      break;
    case kAddUser:
      Shell::Get()->login_screen_controller()->ShowGaiaSignin(base::nullopt);
      break;
    default:
      NOTREACHED();
  }
}

void LoginShelfView::OnLockScreenNoteStateChanged(
    mojom::TrayActionState state) {
  UpdateUi();
}

void LoginShelfView::OnLockScreenActionBackgroundStateChanged(
    LockScreenActionBackgroundState state) {
  UpdateUi();
}

void LoginShelfView::OnShutdownPolicyChanged(bool reboot_on_shutdown) {
  UpdateUi();
}

bool LoginShelfView::LockScreenActionBackgroundAnimating() const {
  return lock_screen_action_background_->state() ==
             LockScreenActionBackgroundState::kShowing ||
         lock_screen_action_background_->state() ==
             LockScreenActionBackgroundState::kHiding;
}

void LoginShelfView::UpdateUi() {
  SessionState session_state =
      Shell::Get()->session_controller()->GetSessionState();
  if (session_state == SessionState::ACTIVE) {
    // The entire view was set invisible. The buttons are also set invisible
    // to avoid affecting calculation of the shelf size.
    for (int i = 0; i < child_count(); ++i)
      child_at(i)->SetVisible(false);
    return;
  }
  bool show_reboot = Shell::Get()->shutdown_controller()->reboot_on_shutdown();
  mojom::TrayActionState tray_action_state =
      Shell::Get()->tray_action()->GetLockScreenNoteState();
  bool is_lock_screen_note_in_foreground =
      (tray_action_state == mojom::TrayActionState::kActive ||
       tray_action_state == mojom::TrayActionState::kLaunching) &&
      !LockScreenActionBackgroundAnimating();

  // The following should be kept in sync with |updateUI_| in md_header_bar.js.
  GetViewByID(kShutdown)->SetVisible(!show_reboot &&
                                     !is_lock_screen_note_in_foreground);
  GetViewByID(kRestart)->SetVisible(show_reboot &&
                                    !is_lock_screen_note_in_foreground);
  GetViewByID(kSignOut)->SetVisible(session_state == SessionState::LOCKED &&
                                    !is_lock_screen_note_in_foreground);
  GetViewByID(kCloseNote)
      ->SetVisible(session_state == SessionState::LOCKED &&
                   is_lock_screen_note_in_foreground);
  GetViewByID(kCancel)->SetVisible(session_state ==
                                   SessionState::LOGIN_SECONDARY);
  // TODO(agawronska): Implement full list of conditions for buttons visibility,
  // when views based shelf if enabled during OOBE. https://crbug.com/798869
  bool is_login_primary = (session_state == SessionState::LOGIN_PRIMARY);
  GetViewByID(kBrowseAsGuest)->SetVisible(is_login_primary);
  GetViewByID(kAddUser)->SetVisible(is_login_primary);
  Layout();
}

}  // namespace ash
