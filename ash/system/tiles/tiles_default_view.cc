// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tiles/tiles_default_view.h"

#include "ash/metrics/user_metrics_action.h"
#include "ash/metrics/user_metrics_recorder.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/shutdown_controller.h"
#include "ash/shutdown_reason.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/night_light/night_light_controller.h"
#include "ash/system/night_light/night_light_toggle_button.h"
#include "ash/system/tray/system_menu_button.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/system/tray/system_tray_item.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/wm/lock_state_controller.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"

namespace ash {

namespace {

// The ISO-639 code for the Hebrew locale. The help icon asset is a '?' which is
// not mirrored in this locale.
const char kHebrewLocale[] = "he";

}  // namespace

TilesDefaultView::TilesDefaultView(SystemTrayItem* owner)
    : owner_(owner),
      settings_button_(nullptr),
      help_button_(nullptr),
      night_light_button_(nullptr),
      lock_button_(nullptr),
      power_button_(nullptr) {
  DCHECK(owner_);
}

TilesDefaultView::~TilesDefaultView() = default;

void TilesDefaultView::Init() {
  auto box_layout = std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal, gfx::Insets(0, 4));
  box_layout->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
  box_layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  SetLayoutManager(std::move(box_layout));

  // Show the buttons in this row as disabled if the user is at the login
  // screen, lock screen, or in a secondary account flow. The exception is
  // |power_button_| which is always shown as enabled.
  const bool can_show_web_ui = TrayPopupUtils::CanOpenWebUISettings();

  settings_button_ = new SystemMenuButton(this, kSystemMenuSettingsIcon,
                                          IDS_ASH_STATUS_TRAY_SETTINGS);
  settings_button_->SetEnabled(can_show_web_ui);
  AddChildView(settings_button_);
  AddChildView(TrayPopupUtils::CreateVerticalSeparator());

  help_button_ =
      new SystemMenuButton(this, kSystemMenuHelpIcon, IDS_ASH_STATUS_TRAY_HELP);
  if (base::i18n::IsRTL() &&
      base::i18n::GetConfiguredLocale() == kHebrewLocale) {
    // The asset for the help button is a question mark '?'. Normally this asset
    // is flipped in RTL locales, however Hebrew uses the LTR '?'. So the
    // flipping must be disabled. (crbug.com/475237)
    help_button_->EnableCanvasFlippingForRTLUI(false);
  }
  help_button_->SetEnabled(can_show_web_ui);
  AddChildView(help_button_);
  AddChildView(TrayPopupUtils::CreateVerticalSeparator());

  if (switches::IsNightLightEnabled()) {
    night_light_button_ = new NightLightToggleButton(this);
    night_light_button_->SetEnabled(can_show_web_ui);
    AddChildView(night_light_button_);
    AddChildView(TrayPopupUtils::CreateVerticalSeparator());
  }

  lock_button_ =
      new SystemMenuButton(this, kSystemMenuLockIcon, IDS_ASH_STATUS_TRAY_LOCK);
  lock_button_->SetEnabled(can_show_web_ui &&
                           Shell::Get()->session_controller()->CanLockScreen());

  AddChildView(lock_button_);
  AddChildView(TrayPopupUtils::CreateVerticalSeparator());

  power_button_ = new SystemMenuButton(this, kSystemMenuPowerIcon,
                                       IDS_ASH_STATUS_TRAY_SHUTDOWN);
  AddChildView(power_button_);
  // This object is recreated every time the menu opens. Don't bother updating
  // the tooltip if the shutdown policy changes while the menu is open.
  bool reboot = Shell::Get()->shutdown_controller()->reboot_on_shutdown();
  power_button_->SetTooltipText(l10n_util::GetStringUTF16(
      reboot ? IDS_ASH_STATUS_TRAY_REBOOT : IDS_ASH_STATUS_TRAY_SHUTDOWN));
}

void TilesDefaultView::ButtonPressed(views::Button* sender,
                                     const ui::Event& event) {
  DCHECK(sender);
  if (sender == settings_button_) {
    Shell::Get()->metrics()->RecordUserMetricsAction(UMA_TRAY_SETTINGS);
    Shell::Get()->system_tray_controller()->ShowSettings();
  } else if (sender == help_button_) {
    Shell::Get()->metrics()->RecordUserMetricsAction(UMA_TRAY_HELP);
    Shell::Get()->system_tray_controller()->ShowHelp();
  } else if (switches::IsNightLightEnabled() && sender == night_light_button_) {
    Shell::Get()->metrics()->RecordUserMetricsAction(UMA_TRAY_NIGHT_LIGHT);
    night_light_button_->Toggle();
  } else if (sender == lock_button_) {
    Shell::Get()->metrics()->RecordUserMetricsAction(UMA_TRAY_LOCK_SCREEN);
    chromeos::DBusThreadManager::Get()
        ->GetSessionManagerClient()
        ->RequestLockScreen();
  } else if (sender == power_button_) {
    Shell::Get()->metrics()->RecordUserMetricsAction(UMA_TRAY_SHUT_DOWN);
    Shell::Get()->lock_state_controller()->RequestShutdown(
        ShutdownReason::TRAY_SHUT_DOWN_BUTTON);
  }
}

views::View* TilesDefaultView::GetHelpButtonView() const {
  return help_button_;
}

const views::Button* TilesDefaultView::GetShutdownButtonViewForTest() const {
  return power_button_;
}

}  // namespace ash
