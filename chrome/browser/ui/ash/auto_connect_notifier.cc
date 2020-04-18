// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/auto_connect_notifier.h"

#include "ash/system/network/network_icon.h"
#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/network/network_connection_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"
#include "url/gurl.h"

namespace {

const char kNotifierAutoConnect[] = "ash.auto-connect";

// Signal strength to use for the notification icon. The network icon should be
// at full signal strength (4 out of 4).
const int kSignalStrength = 4;

// Dimensions of notification icon in pixels.
constexpr gfx::Size kSignalIconSize(18, 18);

// Timeout used for connecting to a managed network. When an auto-connection is
// initiated, we expect the connection to occur within this amount of time. If
// a timeout occurs, we assume that no auto-connection occurred and do not show
// a notification.
constexpr const base::TimeDelta kNetworkConnectionTimeout =
    base::TimeDelta::FromSeconds(3);

const char kAutoConnectNotificationId[] =
    "cros_auto_connect_notifier_ids.connected_to_network";

}  // namespace

AutoConnectNotifier::AutoConnectNotifier(
    Profile* profile,
    chromeos::NetworkConnectionHandler* network_connection_handler,
    chromeos::NetworkStateHandler* network_state_handler,
    chromeos::AutoConnectHandler* auto_connect_handler)
    : profile_(profile),
      network_connection_handler_(network_connection_handler),
      network_state_handler_(network_state_handler),
      auto_connect_handler_(auto_connect_handler),
      timer_(std::make_unique<base::OneShotTimer>()) {
  network_connection_handler_->AddObserver(this);
  network_state_handler_->AddObserver(this, FROM_HERE);
  auto_connect_handler_->AddObserver(this);
}

AutoConnectNotifier::~AutoConnectNotifier() {
  network_connection_handler_->RemoveObserver(this);
  network_state_handler_->RemoveObserver(this, FROM_HERE);
  auto_connect_handler_->RemoveObserver(this);
}

void AutoConnectNotifier::ConnectToNetworkRequested(
    const std::string& /* service_path */) {
  has_user_explicitly_requested_connection_ = true;
}

void AutoConnectNotifier::NetworkConnectionStateChanged(
    const chromeos::NetworkState* network) {
  // No notification should be shown unless an auto-connection is underway.
  if (!timer_->IsRunning())
    return;

  // The notification is only shown when a connection has succeeded; if
  // |network| is not connected, there is nothing to do.
  if (!network->IsConnectedState())
    return;

  // An auto-connected network has connected successfully. Display a
  // notification alerting the user that this has occurred.
  DisplayNotification();
  has_user_explicitly_requested_connection_ = false;
}

void AutoConnectNotifier::OnAutoConnectedInitiated(int auto_connect_reasons) {
  // If the user has not explicitly requested a connection to another network,
  // the notification does not need to be shown.
  if (!has_user_explicitly_requested_connection_)
    return;

  // The notification should only be shown if a network is joined due to a
  // policy or certificate. Other reasons (e.g., joining a network due to login)
  // do not require that a notification be shown.
  const int kManagedNetworkReasonsBitmask =
      chromeos::AutoConnectHandler::AUTO_CONNECT_REASON_POLICY_APPLIED |
      chromeos::AutoConnectHandler::AUTO_CONNECT_REASON_CERTIFICATE_RESOLVED;
  if (!(auto_connect_reasons & kManagedNetworkReasonsBitmask))
    return;

  // If a potential connection is already underway, reset the timeout and
  // continue waiting.
  if (timer_->IsRunning()) {
    timer_->Reset();
    return;
  }

  // Auto-connection has been requested, so start a timer. If a network connects
  // successfully before the timer expires, auto-connection has succeeded, so a
  // notification should be shown. If no connection occurs before the timer
  // fires, we assume that auto-connect attempted to search for networks to
  // join but did not succeed in joining one (in that case, no notification
  // should be shown).
  timer_->Start(FROM_HERE, kNetworkConnectionTimeout, base::DoNothing());
}

void AutoConnectNotifier::DisplayNotification() {
  auto notification = message_center::Notification::CreateSystemNotification(
      message_center::NotificationType::NOTIFICATION_TYPE_SIMPLE,
      kAutoConnectNotificationId,
      l10n_util::GetStringUTF16(IDS_NETWORK_AUTOCONNECT_NOTIFICATION_TITLE),
      l10n_util::GetStringUTF16(IDS_NETWORK_AUTOCONNECT_NOTIFICATION_MESSAGE),
      gfx::Image() /* icon */, base::string16() /* display_source */,
      GURL() /* origin_url */,
      message_center::NotifierId(
          message_center::NotifierId::NotifierType::SYSTEM_COMPONENT,
          kNotifierAutoConnect),
      {} /* optional_fields */,
      base::MakeRefCounted<message_center::NotificationDelegate>(),
      gfx::VectorIcon() /* small_image */,
      message_center::SystemNotificationWarningLevel::NORMAL);

  notification->set_small_image(
      gfx::Image(gfx::CanvasImageSource::MakeImageSkia<
                 ash::network_icon::SignalStrengthImageSource>(
          ash::network_icon::ARCS, notification->accent_color(),
          kSignalIconSize, kSignalStrength)));

  NotificationDisplayService::GetForProfile(profile_)->Display(
      NotificationHandler::Type::TRANSIENT, *notification);
}

void AutoConnectNotifier::SetTimerForTesting(
    std::unique_ptr<base::Timer> test_timer) {
  timer_ = std::move(test_timer);
}
