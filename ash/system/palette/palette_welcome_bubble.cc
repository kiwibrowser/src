// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/palette_welcome_bubble.h"

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/palette/palette_tray.h"
#include "chromeos/chromeos_switches.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "ui/aura/window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {

namespace {

// Width of the bubble content label size.
constexpr int kBubbleContentLabelPreferredWidthDp = 380;

}  // namespace

// Controlled by PaletteWelcomeBubble and anchored to a PaletteTray.
class PaletteWelcomeBubble::WelcomeBubbleView
    : public views::BubbleDialogDelegateView {
 public:
  WelcomeBubbleView(views::View* anchor, views::BubbleBorder::Arrow arrow)
      : views::BubbleDialogDelegateView(anchor, arrow) {
    set_close_on_deactivate(true);
    set_can_activate(false);
    set_accept_events(true);
    set_parent_window(
        anchor_widget()->GetNativeWindow()->GetRootWindow()->GetChildById(
            kShellWindowId_SettingBubbleContainer));
    views::BubbleDialogDelegateView::CreateBubble(this);
  }

  ~WelcomeBubbleView() override = default;

  // ui::BubbleDialogDelegateView:
  base::string16 GetWindowTitle() const override {
    return l10n_util::GetStringUTF16(IDS_ASH_STYLUS_WARM_WELCOME_BUBBLE_TITLE);
  }

  bool ShouldShowWindowTitle() const override { return true; }

  bool ShouldShowCloseButton() const override { return true; }

  void Init() override {
    SetLayoutManager(std::make_unique<views::FillLayout>());
    auto* label = new views::Label(l10n_util::GetStringUTF16(
        chromeos::switches::IsVoiceInteractionEnabled()
            ? IDS_ASH_STYLUS_WARM_WELCOME_BUBBLE_WITH_ASSISTANT_DESCRIPTION
            : IDS_ASH_STYLUS_WARM_WELCOME_BUBBLE_DESCRIPTION));
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    label->SetMultiLine(true);
    label->SizeToFit(kBubbleContentLabelPreferredWidthDp);
    AddChildView(label);
  }

  int GetDialogButtons() const override { return ui::DIALOG_BUTTON_NONE; }

 private:
  DISALLOW_COPY_AND_ASSIGN(WelcomeBubbleView);
};

PaletteWelcomeBubble::PaletteWelcomeBubble(PaletteTray* tray) : tray_(tray) {
  Shell::Get()->session_controller()->AddObserver(this);
}

PaletteWelcomeBubble::~PaletteWelcomeBubble() {
  if (bubble_view_) {
    bubble_view_->GetWidget()->RemoveObserver(this);
    ShellPort::Get()->RemovePointerWatcher(this);
  }
  Shell::Get()->session_controller()->RemoveObserver(this);
}

// static
void PaletteWelcomeBubble::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kShownPaletteWelcomeBubble, false);
}

void PaletteWelcomeBubble::OnWidgetClosing(views::Widget* widget) {
  widget->RemoveObserver(this);
  bubble_view_ = nullptr;
  ShellPort::Get()->RemovePointerWatcher(this);
}

void PaletteWelcomeBubble::OnActiveUserPrefServiceChanged(
    PrefService* pref_service) {
  active_user_pref_service_ = pref_service;
}

void PaletteWelcomeBubble::ShowIfNeeded(bool shown_by_stylus) {
  if (!active_user_pref_service_)
    return;

  if (Shell::Get()->session_controller()->GetSessionState() !=
      session_manager::SessionState::ACTIVE) {
    return;
  }

  base::Optional<user_manager::UserType> user_type =
      Shell::Get()->session_controller()->GetUserType();
  if (user_type && *user_type == user_manager::USER_TYPE_GUEST)
    return;

  if (!active_user_pref_service_->GetBoolean(
          prefs::kShownPaletteWelcomeBubble) &&
      !bubble_shown()) {
    Show(shown_by_stylus);
  }
}

void PaletteWelcomeBubble::MarkAsShown() {
  DCHECK(active_user_pref_service_);
  active_user_pref_service_->SetBoolean(prefs::kShownPaletteWelcomeBubble,
                                        true);
}

base::Optional<gfx::Rect> PaletteWelcomeBubble::GetBubbleBoundsForTest() {
  if (bubble_view_)
    return base::make_optional(bubble_view_->GetBoundsInScreen());

  return base::nullopt;
}

void PaletteWelcomeBubble::Show(bool shown_by_stylus) {
  if (!bubble_view_) {
    DCHECK(tray_);
    bubble_view_ =
        new WelcomeBubbleView(tray_, views::BubbleBorder::BOTTOM_RIGHT);
  }
  active_user_pref_service_->SetBoolean(prefs::kShownPaletteWelcomeBubble,
                                        true);
  bubble_view_->GetWidget()->Show();
  bubble_view_->GetWidget()->AddObserver(this);
  ShellPort::Get()->AddPointerWatcher(this,
                                      views::PointerWatcherEventTypes::BASIC);

  // If the bubble is shown after the device first reads a stylus, ignore the
  // first up event so the event responsible for showing the bubble does not
  // also cause the bubble to close.
  if (shown_by_stylus)
    ignore_stylus_event_ = true;
}

void PaletteWelcomeBubble::Hide() {
  if (bubble_view_)
    bubble_view_->GetWidget()->Close();
}

void PaletteWelcomeBubble::OnPointerEventObserved(
    const ui::PointerEvent& event,
    const gfx::Point& location_in_screen,
    gfx::NativeView target) {
  if (!bubble_view_)
    return;

  // For devices with external stylus, the bubble is activated after it receives
  // a pen pointer event. However, on attaching |this| as a pointer watcher, it
  // receives the same set of events which activated the bubble in the first
  // place (ET_POINTER_DOWN, ET_POINTER_UP, ET_POINTER_CAPTURE_CHANGED). By only
  // looking at ET_POINTER_UP events and skipping the first up event if the
  // bubble is shown because of a stylus event, we ensure the same event that
  // opened the bubble does not close it as well.
  if (event.type() != ui::ET_POINTER_UP)
    return;

  if (ignore_stylus_event_) {
    ignore_stylus_event_ = false;
    return;
  }

  if (!bubble_view_->GetBoundsInScreen().Contains(location_in_screen))
    bubble_view_->GetWidget()->Close();
}

}  // namespace ash
