// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/keyboard_brightness/tray_keyboard_brightness.h"

#include <algorithm>

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/keyboard_brightness_control_delegate.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/system/tray/tri_view.h"
#include "ash/wm/tablet_mode/tablet_mode_observer.h"
#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager/backlight.pb.h"
#include "chromeos/dbus/power_manager_client.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/display/display.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/slider.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace ash {
namespace tray {

namespace {

// A slider that ignores inputs.
class ReadOnlySlider : public views::Slider {
 public:
  ReadOnlySlider() : Slider(nullptr) {}

 private:
  // views::View:
  bool OnMousePressed(const ui::MouseEvent& event) override { return false; }
  bool OnMouseDragged(const ui::MouseEvent& event) override { return false; }
  void OnMouseReleased(const ui::MouseEvent& event) override {}
  bool OnKeyPressed(const ui::KeyEvent& event) override { return false; }

  // ui::EventHandler:
  void OnGestureEvent(ui::GestureEvent* event) override {}
};

}  // namespace

class KeyboardBrightnessView : public TabletModeObserver, public views::View {
 public:
  explicit KeyboardBrightnessView(double initial_percent);

  // |percent| is in the range [0.0, 100.0].
  void SetKeyboardBrightnessPercent(double percent);

  // TabletModeObserver:
  void OnTabletModeStarted() override;
  void OnTabletModeEnded() override;

 private:
  // views::View:
  void OnBoundsChanged(const gfx::Rect& old_bounds) override;

  ReadOnlySlider* slider_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardBrightnessView);
};

KeyboardBrightnessView::KeyboardBrightnessView(double initial_percent) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  // Use CreateMultiTargetRowView() instead of CreateDefaultRowView() because
  // that's what the audio row uses and we want the two rows to layout with the
  // same insets.
  TriView* tri_view = TrayPopupUtils::CreateMultiTargetRowView();
  AddChildView(tri_view);

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  views::ImageView* icon = TrayPopupUtils::CreateMainImageView();
  icon->SetImage(
      gfx::CreateVectorIcon(kSystemMenuKeyboardBrightnessIcon, kMenuIconColor));
  tri_view->AddView(TriView::Container::START, icon);

  slider_ = new ReadOnlySlider();
  slider_->SetBorder(views::CreateEmptyBorder(
      gfx::Insets(0, kTrayPopupSliderHorizontalPadding)));
  slider_->SetValue(static_cast<float>(initial_percent / 100.0));
  slider_->GetViewAccessibility().OverrideName(
      rb.GetLocalizedString(IDS_ASH_STATUS_TRAY_KEYBOARD_BRIGHTNESS));
  tri_view->AddView(TriView::Container::CENTER, slider_);
  tri_view->SetContainerVisible(TriView::Container::END, false);
}

void KeyboardBrightnessView::SetKeyboardBrightnessPercent(double percent) {
  slider_->SetValue(static_cast<float>(percent / 100.0));
}

void KeyboardBrightnessView::OnTabletModeStarted() {
  SetVisible(true);
}

void KeyboardBrightnessView::OnTabletModeEnded() {
  SetVisible(false);
}

void KeyboardBrightnessView::OnBoundsChanged(const gfx::Rect& old_bounds) {
  int w = width() - slider_->x();
  slider_->SetSize(gfx::Size(w, slider_->height()));
}

}  // namespace tray

TrayKeyboardBrightness::TrayKeyboardBrightness(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_DISPLAY_BRIGHTNESS),
      weak_ptr_factory_(this) {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
}

TrayKeyboardBrightness::~TrayKeyboardBrightness() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
}

views::View* TrayKeyboardBrightness::CreateDetailedView(LoginStatus status) {
  CHECK(!brightness_view_);
  brightness_view_ = new tray::KeyboardBrightnessView(current_percent_);
  return brightness_view_;
}

void TrayKeyboardBrightness::OnDetailedViewDestroyed() {
  brightness_view_ = nullptr;
}

void TrayKeyboardBrightness::UpdateAfterLoginStatusChange(LoginStatus status) {}

bool TrayKeyboardBrightness::ShouldShowShelf() const {
  return false;
}

void TrayKeyboardBrightness::KeyboardBrightnessChanged(
    const power_manager::BacklightBrightnessChange& change) {
  current_percent_ = change.percent();

  if (brightness_view_)
    brightness_view_->SetKeyboardBrightnessPercent(current_percent_);

  if (change.cause() !=
      power_manager::BacklightBrightnessChange_Cause_USER_REQUEST)
    return;

  if (brightness_view_ && brightness_view_->visible())
    SetDetailedViewCloseDelay(kTrayPopupAutoCloseDelayInSeconds);
  else
    ShowDetailedView(kTrayPopupAutoCloseDelayInSeconds);
}

}  // namespace ash
