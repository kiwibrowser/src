// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/status_area_widget.h"

#include "ash/public/cpp/ash_features.h"
#include "ash/public/cpp/config.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/system/accessibility/dictation_button_tray.h"
#include "ash/system/accessibility/select_to_speak_tray.h"
#include "ash/system/flag_warning/flag_warning_tray.h"
#include "ash/system/ime_menu/ime_menu_tray.h"
#include "ash/system/message_center/notification_tray.h"
#include "ash/system/overview/overview_button_tray.h"
#include "ash/system/palette/palette_tray.h"
#include "ash/system/session/logout_button_tray.h"
#include "ash/system/status_area_widget_delegate.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/unified/unified_system_tray.h"
#include "ash/system/virtual_keyboard/virtual_keyboard_tray.h"
#include "base/command_line.h"
#include "base/i18n/time_formatting.h"
#include "ui/display/display.h"
#include "ui/native_theme/native_theme_dark_aura.h"

namespace ash {

StatusAreaWidget::StatusAreaWidget(aura::Window* status_container, Shelf* shelf)
    : status_area_widget_delegate_(new StatusAreaWidgetDelegate(shelf)),
      shelf_(shelf) {
  DCHECK(status_container);
  DCHECK(shelf);
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.delegate = status_area_widget_delegate_;
  params.name = "StatusAreaWidget";
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.parent = status_container;
  Init(params);
  set_focus_on_creation(false);
  SetContentsView(status_area_widget_delegate_);
}

void StatusAreaWidget::Initialize() {
  // Create the child views, right to left.
  overview_button_tray_ = std::make_unique<OverviewButtonTray>(shelf_);
  status_area_widget_delegate_->AddChildView(overview_button_tray_.get());

  system_tray_ = std::make_unique<SystemTray>(shelf_);
  status_area_widget_delegate_->AddChildView(system_tray_.get());

  if (features::IsSystemTrayUnifiedEnabled()) {
    unified_system_tray_ = std::make_unique<UnifiedSystemTray>(shelf_);
    status_area_widget_delegate_->AddChildView(unified_system_tray_.get());
  }

  // Must happen after the widget is initialized so the native window exists.
  if (!features::IsSystemTrayUnifiedEnabled()) {
    notification_tray_ =
        std::make_unique<NotificationTray>(shelf_, GetNativeWindow());
    status_area_widget_delegate_->AddChildView(notification_tray_.get());
  }

  palette_tray_ = std::make_unique<PaletteTray>(shelf_);
  status_area_widget_delegate_->AddChildView(palette_tray_.get());

  virtual_keyboard_tray_ = std::make_unique<VirtualKeyboardTray>(shelf_);
  status_area_widget_delegate_->AddChildView(virtual_keyboard_tray_.get());

  ime_menu_tray_ = std::make_unique<ImeMenuTray>(shelf_);
  status_area_widget_delegate_->AddChildView(ime_menu_tray_.get());

  select_to_speak_tray_ = std::make_unique<SelectToSpeakTray>(shelf_);
  status_area_widget_delegate_->AddChildView(select_to_speak_tray_.get());

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kEnableExperimentalAccessibilityFeatures)) {
    // Dictation is currently only available behind the experimental
    // accessibility features flag.
    dictation_button_tray_ = std::make_unique<DictationButtonTray>(shelf_);
    status_area_widget_delegate_->AddChildView(dictation_button_tray_.get());
  }

  logout_button_tray_ = std::make_unique<LogoutButtonTray>(shelf_);
  status_area_widget_delegate_->AddChildView(logout_button_tray_.get());

  if (Shell::GetAshConfig() == ash::Config::MASH) {
    // Flag warning tray is not currently used in non-MASH environments, because
    // mus will roll out via experiment/Finch trial and showing the tray would
    // reveal the experiment state to users.
    flag_warning_tray_ = std::make_unique<FlagWarningTray>(shelf_);
    status_area_widget_delegate_->AddChildView(flag_warning_tray_.get());
  }

  // The layout depends on the number of children, so build it once after
  // adding all of them.
  status_area_widget_delegate_->UpdateLayout();

  // Initialize after all trays have been created.
  if (notification_tray_) {
    system_tray_->InitializeTrayItems(notification_tray_.get());
    notification_tray_->Initialize();
  } else {
    system_tray_->InitializeTrayItems(nullptr);
  }
  palette_tray_->Initialize();
  virtual_keyboard_tray_->Initialize();
  ime_menu_tray_->Initialize();
  select_to_speak_tray_->Initialize();
  if (dictation_button_tray_)
    dictation_button_tray_->Initialize();
  overview_button_tray_->Initialize();
  UpdateAfterShelfAlignmentChange();
  UpdateAfterLoginStatusChange(
      Shell::Get()->session_controller()->login_status());

  // NOTE: Container may be hidden depending on login/display state.
  Show();
}

StatusAreaWidget::~StatusAreaWidget() {
  system_tray_->Shutdown();

  notification_tray_.reset();
  // Must be destroyed after |notification_tray_|.
  system_tray_.reset();
  unified_system_tray_.reset();
  ime_menu_tray_.reset();
  select_to_speak_tray_.reset();
  dictation_button_tray_.reset();
  virtual_keyboard_tray_.reset();
  palette_tray_.reset();
  logout_button_tray_.reset();
  overview_button_tray_.reset();
  flag_warning_tray_.reset();

  // All child tray views have been removed.
  DCHECK_EQ(0, GetContentsView()->child_count());
}

void StatusAreaWidget::UpdateAfterShelfAlignmentChange() {
  system_tray_->UpdateAfterShelfAlignmentChange();
  if (notification_tray_)
    notification_tray_->UpdateAfterShelfAlignmentChange();
  logout_button_tray_->UpdateAfterShelfAlignmentChange();
  virtual_keyboard_tray_->UpdateAfterShelfAlignmentChange();
  ime_menu_tray_->UpdateAfterShelfAlignmentChange();
  select_to_speak_tray_->UpdateAfterShelfAlignmentChange();
  if (dictation_button_tray_)
    dictation_button_tray_->UpdateAfterShelfAlignmentChange();
  palette_tray_->UpdateAfterShelfAlignmentChange();
  overview_button_tray_->UpdateAfterShelfAlignmentChange();
  if (flag_warning_tray_)
    flag_warning_tray_->UpdateAfterShelfAlignmentChange();
  status_area_widget_delegate_->UpdateLayout();
}

void StatusAreaWidget::UpdateAfterLoginStatusChange(LoginStatus login_status) {
  if (login_status_ == login_status)
    return;
  login_status_ = login_status;

  system_tray_->UpdateAfterLoginStatusChange(login_status);
  logout_button_tray_->UpdateAfterLoginStatusChange();
  overview_button_tray_->UpdateAfterLoginStatusChange(login_status);
}

void StatusAreaWidget::SetSystemTrayVisibility(bool visible) {
  TrayBackgroundView* tray =
      unified_system_tray_
          ? static_cast<TrayBackgroundView*>(unified_system_tray_.get())
          : static_cast<TrayBackgroundView*>(system_tray_.get());
  tray->SetVisible(visible);
  // Opacity is set to prevent flakiness in kiosk browser tests. See
  // https://crbug.com/624584.
  SetOpacity(visible ? 1.f : 0.f);
  if (visible) {
    Show();
  } else {
    tray->CloseBubble();
    Hide();
  }
}

TrayBackgroundView* StatusAreaWidget::GetSystemTrayAnchor() const {
  if (overview_button_tray_->visible())
    return overview_button_tray_.get();
  if (unified_system_tray_)
    return unified_system_tray_.get();
  return system_tray_.get();
}

bool StatusAreaWidget::ShouldShowShelf() const {
  // The system tray bubble may or may not want to force the shelf to be
  // visible.
  if (system_tray_->IsSystemBubbleVisible())
    return system_tray_->ShouldShowShelf();

  // All other tray bubbles will force the shelf to be visible.
  return views::TrayBubbleView::IsATrayBubbleOpen();
}

bool StatusAreaWidget::IsMessageBubbleShown() const {
  return (unified_system_tray_ && unified_system_tray_->IsBubbleShown()) ||
         (!unified_system_tray_ && system_tray_->IsSystemBubbleVisible()) ||
         (notification_tray_ && notification_tray_->IsMessageCenterVisible());
}

void StatusAreaWidget::SchedulePaint() {
  status_area_widget_delegate_->SchedulePaint();
  if (notification_tray_)
    notification_tray_->SchedulePaint();
  system_tray_->SchedulePaint();
  virtual_keyboard_tray_->SchedulePaint();
  logout_button_tray_->SchedulePaint();
  ime_menu_tray_->SchedulePaint();
  select_to_speak_tray_->SchedulePaint();
  if (dictation_button_tray_)
    dictation_button_tray_->SchedulePaint();
  palette_tray_->SchedulePaint();
  overview_button_tray_->SchedulePaint();
  if (flag_warning_tray_)
    flag_warning_tray_->SchedulePaint();
}

const ui::NativeTheme* StatusAreaWidget::GetNativeTheme() const {
  return ui::NativeThemeDarkAura::instance();
}

bool StatusAreaWidget::OnNativeWidgetActivationChanged(bool active) {
  if (!Widget::OnNativeWidgetActivationChanged(active))
    return false;
  if (active)
    status_area_widget_delegate_->SetPaneFocusAndFocusDefault();
  return true;
}

void StatusAreaWidget::UpdateShelfItemBackground(SkColor color) {
  if (notification_tray_)
    notification_tray_->UpdateShelfItemBackground(color);
  system_tray_->UpdateShelfItemBackground(color);
  virtual_keyboard_tray_->UpdateShelfItemBackground(color);
  ime_menu_tray_->UpdateShelfItemBackground(color);
  select_to_speak_tray_->UpdateShelfItemBackground(color);
  if (dictation_button_tray_)
    dictation_button_tray_->UpdateShelfItemBackground(color);
  palette_tray_->UpdateShelfItemBackground(color);
  overview_button_tray_->UpdateShelfItemBackground(color);
}

}  // namespace ash
