// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/unified_system_tray.h"

#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/date/date_view.h"
#include "ash/system/message_center/ash_popup_alignment_delegate.h"
#include "ash/system/model/clock_model.h"
#include "ash/system/model/system_tray_model.h"
#include "ash/system/network/network_tray_view.h"
#include "ash/system/network/tray_network_state_observer.h"
#include "ash/system/power/tray_power.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/tray_container.h"
#include "ash/system/unified/notification_counter_view.h"
#include "ash/system/unified/unified_system_tray_bubble.h"
#include "ash/system/unified/unified_system_tray_model.h"
#include "chromeos/network/network_handler.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/ui_controller.h"
#include "ui/message_center/ui_delegate.h"
#include "ui/message_center/views/message_popup_collection.h"

namespace ash {

class UnifiedSystemTray::UiDelegate : public message_center::UiDelegate {
 public:
  UiDelegate(UnifiedSystemTray* owner);
  ~UiDelegate() override;

  // message_center::UiDelegate:
  void OnMessageCenterContentsChanged() override;
  bool ShowPopups() override;
  void HidePopups() override;
  bool ShowMessageCenter(bool show_by_click) override;
  void HideMessageCenter() override;
  bool ShowNotifierSettings() override;

  message_center::UiController* ui_controller() { return ui_controller_.get(); }

 private:
  std::unique_ptr<message_center::UiController> ui_controller_;
  std::unique_ptr<AshPopupAlignmentDelegate> popup_alignment_delegate_;
  std::unique_ptr<message_center::MessagePopupCollection>
      message_popup_collection_;

  UnifiedSystemTray* const owner_;

  DISALLOW_COPY_AND_ASSIGN(UiDelegate);
};

UnifiedSystemTray::UiDelegate::UiDelegate(UnifiedSystemTray* owner)
    : owner_(owner) {
  ui_controller_ = std::make_unique<message_center::UiController>(this);
  popup_alignment_delegate_ =
      std::make_unique<AshPopupAlignmentDelegate>(owner->shelf());
  message_popup_collection_ =
      std::make_unique<message_center::MessagePopupCollection>(
          message_center::MessageCenter::Get(), ui_controller_.get(),
          popup_alignment_delegate_.get());
  display::Screen* screen = display::Screen::GetScreen();
  popup_alignment_delegate_->StartObserving(
      screen, screen->GetDisplayNearestWindow(
                  owner->shelf()->GetStatusAreaWidget()->GetNativeWindow()));
}

UnifiedSystemTray::UiDelegate::~UiDelegate() = default;

void UnifiedSystemTray::UiDelegate::OnMessageCenterContentsChanged() {
  owner_->UpdateNotificationInternal();
}

bool UnifiedSystemTray::UiDelegate::ShowPopups() {
  if (owner_->IsBubbleShown())
    return false;
  message_popup_collection_->DoUpdate();
  return true;
}

void UnifiedSystemTray::UiDelegate::HidePopups() {
  message_popup_collection_->MarkAllPopupsShown();
}

bool UnifiedSystemTray::UiDelegate::ShowMessageCenter(bool show_by_click) {
  if (owner_->IsBubbleShown())
    return false;

  owner_->ShowBubbleInternal(show_by_click);
  return true;
}

void UnifiedSystemTray::UiDelegate::HideMessageCenter() {
  owner_->HideBubbleInternal();
}

bool UnifiedSystemTray::UiDelegate::ShowNotifierSettings() {
  return false;
}

class UnifiedSystemTray::NetworkStateDelegate
    : public TrayNetworkStateObserver::Delegate {
 public:
  explicit NetworkStateDelegate(tray::NetworkTrayView* tray_view);
  ~NetworkStateDelegate() override;

  // TrayNetworkStateObserver::Delegate
  void NetworkStateChanged(bool notify_a11y) override;

 private:
  tray::NetworkTrayView* const tray_view_;
  const std::unique_ptr<TrayNetworkStateObserver> network_state_observer_;

  DISALLOW_COPY_AND_ASSIGN(NetworkStateDelegate);
};

UnifiedSystemTray::NetworkStateDelegate::NetworkStateDelegate(
    tray::NetworkTrayView* tray_view)
    : tray_view_(tray_view),
      network_state_observer_(
          std::make_unique<TrayNetworkStateObserver>(this)) {}

UnifiedSystemTray::NetworkStateDelegate::~NetworkStateDelegate() = default;

void UnifiedSystemTray::NetworkStateDelegate::NetworkStateChanged(
    bool notify_a11y) {
  tray_view_->UpdateNetworkStateHandlerIcon();
  tray_view_->UpdateConnectionStatus(tray::GetConnectedNetwork(), notify_a11y);
}

UnifiedSystemTray::UnifiedSystemTray(Shelf* shelf)
    : TrayBackgroundView(shelf),
      ui_delegate_(std::make_unique<UiDelegate>(this)),
      model_(std::make_unique<UnifiedSystemTrayModel>()),
      notification_counter_item_(new NotificationCounterView()),
      quiet_mode_view_(new QuietModeView()) {
  tray_container()->AddChildView(notification_counter_item_);
  tray_container()->AddChildView(quiet_mode_view_);

  // It is possible in unit tests that it's missing.
  if (chromeos::NetworkHandler::IsInitialized()) {
    tray::NetworkTrayView* network_item = new tray::NetworkTrayView(nullptr);
    network_state_delegate_ =
        std::make_unique<NetworkStateDelegate>(network_item);
    tray_container()->AddChildView(network_item);
  }

  tray_container()->AddChildView(new tray::PowerTrayView(nullptr));

  TrayItemView* time_item = new TrayItemView(nullptr);
  time_item->AddChildView(
      new tray::TimeView(tray::TimeView::ClockLayout::HORIZONTAL_CLOCK,
                         Shell::Get()->system_tray_model()->clock()));
  tray_container()->AddChildView(time_item);

  SetInkDropMode(InkDropMode::ON);
  SetVisible(true);
}

UnifiedSystemTray::~UnifiedSystemTray() = default;

bool UnifiedSystemTray::IsBubbleShown() const {
  return !!bubble_;
}

gfx::Rect UnifiedSystemTray::GetBubbleBoundsInScreen() const {
  return bubble_ ? bubble_->GetBoundsInScreen() : gfx::Rect();
}

bool UnifiedSystemTray::PerformAction(const ui::Event& event) {
  if (bubble_)
    CloseBubble();
  else
    ShowBubble(true /* show_by_click */);
  return true;
}

void UnifiedSystemTray::ShowBubble(bool show_by_click) {
  // ShowBubbleInternal will be called from UiDelegate.
  if (!bubble_)
    ui_delegate_->ui_controller()->ShowMessageCenterBubble(show_by_click);
}

void UnifiedSystemTray::CloseBubble() {
  // HideBubbleInternal will be called from UiDelegate.
  ui_delegate_->ui_controller()->HideMessageCenterBubble();
}

base::string16 UnifiedSystemTray::GetAccessibleNameForTray() {
  base::string16 time = base::TimeFormatTimeOfDayWithHourClockType(
      base::Time::Now(),
      Shell::Get()->system_tray_model()->clock()->hour_clock_type(),
      base::kKeepAmPm);
  base::string16 battery = PowerStatus::Get()->GetAccessibleNameString(false);
  return l10n_util::GetStringFUTF16(IDS_ASH_STATUS_TRAY_ACCESSIBLE_DESCRIPTION,
                                    time, battery);
}

void UnifiedSystemTray::HideBubbleWithView(
    const views::TrayBubbleView* bubble_view) {}

void UnifiedSystemTray::ClickedOutsideBubble() {
  CloseBubble();
}

void UnifiedSystemTray::ShowBubbleInternal(bool show_by_click) {
  bubble_ = std::make_unique<UnifiedSystemTrayBubble>(this, show_by_click);
  SetIsActive(true);
}

void UnifiedSystemTray::HideBubbleInternal() {
  bubble_.reset();
  SetIsActive(false);
}

void UnifiedSystemTray::UpdateNotificationInternal() {
  notification_counter_item_->Update();
  quiet_mode_view_->Update();
}

}  // namespace ash
