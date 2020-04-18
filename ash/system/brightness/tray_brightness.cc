// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/brightness/tray_brightness.h"

#include <algorithm>

#include "ash/metrics/user_metrics_recorder.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/brightness_control_delegate.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/system/tray/tri_view.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
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
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/slider.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace ash {
namespace tray {

class BrightnessView : public TabletModeObserver,
                       public views::View,
                       public views::SliderListener {
 public:
  BrightnessView(bool default_view, double initial_percent);
  ~BrightnessView() override;

  bool is_default_view() const { return is_default_view_; }

  // |percent| is in the range [0.0, 100.0].
  void SetBrightnessPercent(double percent);

  // TabletModeObserver:
  void OnTabletModeStarted() override;
  void OnTabletModeEnded() override;

 private:
  // views::View:
  void OnBoundsChanged(const gfx::Rect& old_bounds) override;

  // views:SliderListener:
  void SliderValueChanged(views::Slider* sender,
                          float value,
                          float old_value,
                          views::SliderChangeReason reason) override;

  // views:SliderListener:
  void SliderDragStarted(views::Slider* slider) override;
  void SliderDragEnded(views::Slider* slider) override;

  views::Slider* slider_;

  // Is |slider_| currently being dragged?
  bool dragging_;

  // True if this view is for the default tray view. Used to control hide/show
  // behaviour of the default view when entering or leaving Maximize Mode.
  bool is_default_view_;

  // Last brightness level that we observed, in the range [0.0, 100.0].
  double last_percent_;

  DISALLOW_COPY_AND_ASSIGN(BrightnessView);
};

BrightnessView::BrightnessView(bool default_view, double initial_percent)
    : dragging_(false),
      is_default_view_(default_view),
      last_percent_(initial_percent) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  // Use CreateMultiTargetRowView() instead of CreateDefaultRowView() because
  // that's what the audio row uses and we want the two rows to layout with the
  // same insets.
  TriView* tri_view = TrayPopupUtils::CreateMultiTargetRowView();
  AddChildView(tri_view);

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  views::ImageView* icon = TrayPopupUtils::CreateMainImageView();
  icon->SetImage(
      gfx::CreateVectorIcon(kSystemMenuBrightnessIcon, kMenuIconColor));
  tri_view->AddView(TriView::Container::START, icon);

  slider_ = TrayPopupUtils::CreateSlider(this);
  slider_->SetValue(static_cast<float>(initial_percent / 100.0));
  slider_->GetViewAccessibility().OverrideName(
      rb.GetLocalizedString(IDS_ASH_STATUS_TRAY_BRIGHTNESS));
  tri_view->AddView(TriView::Container::CENTER, slider_);

  if (is_default_view_) {
    Shell::Get()->tablet_mode_controller()->AddObserver(this);
    SetVisible(Shell::Get()
                   ->tablet_mode_controller()
                   ->IsTabletModeWindowManagerEnabled());
  }
  tri_view->SetContainerVisible(TriView::Container::END, false);
}

BrightnessView::~BrightnessView() {
  if (is_default_view_ && Shell::Get()->tablet_mode_controller())
    Shell::Get()->tablet_mode_controller()->RemoveObserver(this);
}

void BrightnessView::SetBrightnessPercent(double percent) {
  last_percent_ = percent;
  if (!dragging_)
    slider_->SetValue(static_cast<float>(percent / 100.0));
}

void BrightnessView::OnTabletModeStarted() {
  SetVisible(true);
}

void BrightnessView::OnTabletModeEnded() {
  SetVisible(false);
}

void BrightnessView::OnBoundsChanged(const gfx::Rect& old_bounds) {
  int w = width() - slider_->x();
  slider_->SetSize(gfx::Size(w, slider_->height()));
}

void BrightnessView::SliderValueChanged(views::Slider* sender,
                                        float value,
                                        float old_value,
                                        views::SliderChangeReason reason) {
  DCHECK_EQ(sender, slider_);
  if (reason != views::VALUE_CHANGED_BY_USER)
    return;
  BrightnessControlDelegate* brightness_control_delegate =
      Shell::Get()->brightness_control_delegate();
  if (brightness_control_delegate) {
    double percent = std::max(value * 100.0, kMinBrightnessPercent);
    brightness_control_delegate->SetBrightnessPercent(percent, true);
  }
}

void BrightnessView::SliderDragStarted(views::Slider* slider) {
  DCHECK_EQ(slider, slider_);
  dragging_ = true;
}

void BrightnessView::SliderDragEnded(views::Slider* slider) {
  DCHECK_EQ(slider, slider_);
  dragging_ = false;
  slider_->SetValue(static_cast<float>(last_percent_ / 100.0));
}

}  // namespace tray

TrayBrightness::TrayBrightness(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_DISPLAY_BRIGHTNESS),
      brightness_view_(nullptr),
      current_percent_(100.0),
      got_current_percent_(false),
      weak_ptr_factory_(this) {
  // Post a task to get the initial brightness; the BrightnessControlDelegate
  // isn't created yet.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&TrayBrightness::GetInitialBrightness,
                            weak_ptr_factory_.GetWeakPtr()));
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
}

TrayBrightness::~TrayBrightness() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
}

void TrayBrightness::GetInitialBrightness() {
  BrightnessControlDelegate* brightness_control_delegate =
      Shell::Get()->brightness_control_delegate();
  // Worrisome, but happens in unit tests, so don't log anything.
  if (!brightness_control_delegate)
    return;
  brightness_control_delegate->GetBrightnessPercent(
      base::BindOnce(&TrayBrightness::HandleInitialBrightness,
                     weak_ptr_factory_.GetWeakPtr()));
}

void TrayBrightness::HandleInitialBrightness(base::Optional<double> percent) {
  if (!got_current_percent_ && percent.has_value())
    HandleBrightnessChanged(percent.value(), false);
}

views::View* TrayBrightness::CreateDefaultView(LoginStatus status) {
  CHECK(brightness_view_ == nullptr);
  brightness_view_ = new tray::BrightnessView(true, current_percent_);
  return brightness_view_;
}

views::View* TrayBrightness::CreateDetailedView(LoginStatus status) {
  CHECK(brightness_view_ == nullptr);
  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_DETAILED_BRIGHTNESS_VIEW);
  brightness_view_ = new tray::BrightnessView(false, current_percent_);
  return brightness_view_;
}

void TrayBrightness::OnDefaultViewDestroyed() {
  if (brightness_view_ && brightness_view_->is_default_view())
    brightness_view_ = nullptr;
}

void TrayBrightness::OnDetailedViewDestroyed() {
  if (brightness_view_ && !brightness_view_->is_default_view())
    brightness_view_ = nullptr;
}

void TrayBrightness::UpdateAfterLoginStatusChange(LoginStatus status) {}

bool TrayBrightness::ShouldShowShelf() const {
  return false;
}

void TrayBrightness::ScreenBrightnessChanged(
    const power_manager::BacklightBrightnessChange& change) {
  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_BRIGHTNESS_CHANGED);
  const bool user_initiated =
      change.cause() ==
      power_manager::BacklightBrightnessChange_Cause_USER_REQUEST;
  HandleBrightnessChanged(change.percent(), user_initiated);
}

void TrayBrightness::HandleBrightnessChanged(double percent,
                                             bool user_initiated) {
  current_percent_ = percent;
  got_current_percent_ = true;

  if (brightness_view_)
    brightness_view_->SetBrightnessPercent(percent);

  if (!user_initiated)
    return;

  // Never show the bubble on systems that lack internal displays: if an
  // external display's brightness is changed, it may already display the new
  // level via an on-screen display.
  if (!display::Display::HasInternalDisplay())
    return;

  // Do not show bubble when UnifiedSystemTray bubble is already shown.
  if (IsUnifiedBubbleShown())
    return;

  if (brightness_view_ && brightness_view_->visible())
    SetDetailedViewCloseDelay(kTrayPopupAutoCloseDelayInSeconds);
  else
    ShowDetailedView(kTrayPopupAutoCloseDelayInSeconds);
}

}  // namespace ash
