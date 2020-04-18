// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/wifi_toggle_notification_controller.h"

#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/network/network_icon.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "chromeos/network/network_state_handler.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/notification.h"

using chromeos::NetworkHandler;
using chromeos::NetworkStateHandler;
using chromeos::NetworkTypePattern;
using message_center::Notification;

namespace ash {

namespace {

constexpr char kWifiToggleNotificationId[] = "wifi-toggle";
constexpr char kNotifierWifiToggle[] = "ash.wifi-toggle";

std::unique_ptr<Notification> CreateNotification(bool wifi_enabled) {
  const int string_id = wifi_enabled
                            ? IDS_ASH_STATUS_TRAY_NETWORK_WIFI_ENABLED
                            : IDS_ASH_STATUS_TRAY_NETWORK_WIFI_DISABLED;
  std::unique_ptr<Notification> notification = std::make_unique<Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE, kWifiToggleNotificationId,
      base::string16(), l10n_util::GetStringUTF16(string_id),
      gfx::Image(network_icon::GetImageForWiFiEnabledState(wifi_enabled)),
      base::string16() /* display_source */, GURL(),
      message_center::NotifierId(message_center::NotifierId::SYSTEM_COMPONENT,
                                 kNotifierWifiToggle),
      message_center::RichNotificationData(), nullptr);
  return notification;
}

}  // namespace

WifiToggleNotificationController::WifiToggleNotificationController() {
  Shell::Get()->system_tray_notifier()->AddNetworkObserver(this);
}

WifiToggleNotificationController::~WifiToggleNotificationController() {
  Shell::Get()->system_tray_notifier()->RemoveNetworkObserver(this);
}

void WifiToggleNotificationController::RequestToggleWifi() {
  // This will always be triggered by a user action (e.g. keyboard shortcut)
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  bool enabled = handler->IsTechnologyEnabled(NetworkTypePattern::WiFi());
  Shell::Get()->metrics()->RecordUserMetricsAction(
      enabled ? UMA_STATUS_AREA_DISABLE_WIFI : UMA_STATUS_AREA_ENABLE_WIFI);
  handler->SetTechnologyEnabled(NetworkTypePattern::WiFi(), !enabled,
                                chromeos::network_handler::ErrorCallback());
  message_center::MessageCenter* message_center =
      message_center::MessageCenter::Get();
  if (message_center->FindVisibleNotificationById(kWifiToggleNotificationId))
    message_center->RemoveNotification(kWifiToggleNotificationId, false);
  message_center->AddNotification(CreateNotification(!enabled));
}

}  // namespace ash
