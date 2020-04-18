// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/network/network_state_notifier.h"

#include "ash/public/cpp/vector_icons/vector_icons.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/net/shill_error.h"
#include "chrome/browser/notifications/system_notification_helper.h"
#include "chrome/browser/ui/ash/system_tray_client.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "chromeos/network/network_configuration_handler.h"
#include "chromeos/network/network_connection_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/shill_property_util.h"
#include "components/device_event_log/device_event_log.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/message_center/public/cpp/notification.h"

namespace chromeos {

namespace {

const int kMinTimeBetweenOutOfCreditsNotifySeconds = 10 * 60;

const char kNotifierNetwork[] = "ash.network";
const char kNotifierNetworkError[] = "ash.network.error";

// Ignore in-progress error.
bool ShillErrorIsIgnored(const std::string& shill_error) {
  if (shill_error == shill::kErrorResultInProgress)
    return true;
  return false;
}

// Error messages based on |error_name|, not network_state->error().
base::string16 GetConnectErrorString(const std::string& error_name) {
  if (error_name == NetworkConnectionHandler::kErrorNotFound)
    return l10n_util::GetStringUTF16(IDS_CHROMEOS_NETWORK_ERROR_CONNECT_FAILED);
  if (error_name == NetworkConnectionHandler::kErrorConfigureFailed) {
    return l10n_util::GetStringUTF16(
        IDS_CHROMEOS_NETWORK_ERROR_CONFIGURE_FAILED);
  }
  if (error_name == NetworkConnectionHandler::kErrorCertLoadTimeout) {
    return l10n_util::GetStringUTF16(
        IDS_CHROMEOS_NETWORK_ERROR_CERTIFICATES_NOT_LOADED);
  }
  if (error_name == NetworkConnectionHandler::kErrorActivateFailed) {
    return l10n_util::GetStringUTF16(
        IDS_CHROMEOS_NETWORK_ERROR_ACTIVATION_FAILED);
  }
  return base::string16();
}

const gfx::VectorIcon& GetErrorNotificationVectorIcon(
    const std::string& network_type) {
  if (network_type == shill::kTypeVPN)
    return ash::kNotificationVpnIcon;
  if (network_type == shill::kTypeCellular)
    return ash::kNotificationMobileDataOffIcon;
  return ash::kNotificationWifiOffIcon;
}

void ShowErrorNotification(const std::string& service_path,
                           const std::string& notification_id,
                           const std::string& network_type,
                           const base::string16& title,
                           const base::string16& message,
                           const base::Closure& callback) {
  NET_LOG(ERROR) << "ShowErrorNotification: " << service_path << ": "
                 << base::UTF16ToUTF8(title);
  std::unique_ptr<message_center::Notification> notification =
      message_center::Notification::CreateSystemNotification(
          message_center::NOTIFICATION_TYPE_SIMPLE, notification_id, title,
          message, gfx::Image(), base::string16() /* display_source */, GURL(),
          message_center::NotifierId(
              message_center::NotifierId::SYSTEM_COMPONENT,
              kNotifierNetworkError),
          message_center::RichNotificationData(),
          new message_center::HandleNotificationClickDelegate(callback),
          GetErrorNotificationVectorIcon(network_type),
          message_center::SystemNotificationWarningLevel::CRITICAL_WARNING);
  notification->set_priority(message_center::SYSTEM_PRIORITY);
  SystemNotificationHelper::GetInstance()->Display(*notification);
}

bool ShouldConnectFailedNotificationBeShown(const std::string& error_name,
                                            const NetworkState* network_state) {
  // Only show a notification for certain errors. Other failures are expected
  // to be handled by the UI that initiated the connect request.
  // Note: kErrorConnectFailed may also cause the configure dialog to be
  // displayed, but we rely on the notification system to show additional
  // details if available.
  if (error_name != NetworkConnectionHandler::kErrorConnectFailed &&
      error_name != NetworkConnectionHandler::kErrorNotFound &&
      error_name != NetworkConnectionHandler::kErrorConfigureFailed &&
      error_name != NetworkConnectionHandler::kErrorCertLoadTimeout) {
    return false;
  }

  // When a connection to a Tether network fails, the Tether component shows its
  // own error notification. If this is the case, there is no need to show an
  // additional notification for the failure to connect to the underlying Wi-Fi
  // network.
  if (network_state && !network_state->tether_guid().empty())
    return false;

  // Otherwise, the connection failed notification should be shown.
  return true;
}

const NetworkState* GetNetworkStateForGuid(const std::string& guid) {
  return guid.empty() ? nullptr
                      : NetworkHandler::Get()
                            ->network_state_handler()
                            ->GetNetworkStateFromGuid(guid);
}

}  // namespace

const char NetworkStateNotifier::kNetworkConnectNotificationId[] =
    "chrome://settings/internet/connect";
const char NetworkStateNotifier::kNetworkActivateNotificationId[] =
    "chrome://settings/internet/activate";
const char NetworkStateNotifier::kNetworkOutOfCreditsNotificationId[] =
    "chrome://settings/internet/out-of-credits";

NetworkStateNotifier::NetworkStateNotifier()
    : did_show_out_of_credits_(false), weak_ptr_factory_(this) {
  if (!NetworkHandler::IsInitialized())
    return;
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  handler->AddObserver(this, FROM_HERE);
  UpdateDefaultNetwork(handler->DefaultNetwork());
  NetworkHandler::Get()->network_connection_handler()->AddObserver(this);
}

NetworkStateNotifier::~NetworkStateNotifier() {
  if (!NetworkHandler::IsInitialized())
    return;
  NetworkHandler::Get()->network_state_handler()->RemoveObserver(this,
                                                                 FROM_HERE);
  NetworkHandler::Get()->network_connection_handler()->RemoveObserver(this);
}

void NetworkStateNotifier::ConnectToNetworkRequested(
    const std::string& service_path) {
  const NetworkState* network =
      NetworkHandler::Get()->network_state_handler()->GetNetworkState(
          service_path);
  if (network && network->type() == shill::kTypeVPN)
    connected_vpn_.clear();

  RemoveConnectNotification();
}

void NetworkStateNotifier::ConnectSucceeded(const std::string& service_path) {
  RemoveConnectNotification();
}

void NetworkStateNotifier::ConnectFailed(const std::string& service_path,
                                         const std::string& error_name) {
  const NetworkState* network =
      NetworkHandler::Get()->network_state_handler()->GetNetworkState(
          service_path);
  if (!ShouldConnectFailedNotificationBeShown(error_name, network)) {
    NET_LOG(EVENT) << "Skipping notification for: " << service_path
                   << " Error: " << error_name;
    return;
  }
  ShowNetworkConnectErrorForGuid(error_name, network ? network->guid() : "");
}

void NetworkStateNotifier::DisconnectRequested(
    const std::string& service_path) {
  const NetworkState* network =
      NetworkHandler::Get()->network_state_handler()->GetNetworkState(
          service_path);
  if (network && network->type() == shill::kTypeVPN)
    connected_vpn_.clear();
}

void NetworkStateNotifier::DefaultNetworkChanged(const NetworkState* network) {
  if (!UpdateDefaultNetwork(network))
    return;
  // If the default network changes to another network, allow the out of
  // credits notification to be shown again. A delay prevents the notification
  // from being shown too frequently (see below).
  if (network)
    did_show_out_of_credits_ = false;
}

void NetworkStateNotifier::NetworkConnectionStateChanged(
    const NetworkState* network) {
  if (network->type() == shill::kTypeVPN)
    UpdateVpnConnectionState(network);
}

void NetworkStateNotifier::NetworkPropertiesUpdated(
    const NetworkState* network) {
  if (network->type() != shill::kTypeCellular)
    return;
  UpdateCellularOutOfCredits(network);
  UpdateCellularActivating(network);
}

bool NetworkStateNotifier::UpdateDefaultNetwork(const NetworkState* network) {
  std::string default_network_path;
  if (network)
    default_network_path = network->path();
  if (default_network_path != last_default_network_) {
    last_default_network_ = default_network_path;
    return true;
  }
  return false;
}

void NetworkStateNotifier::UpdateVpnConnectionState(const NetworkState* vpn) {
  if (vpn->path() == connected_vpn_) {
    if (!vpn->IsConnectedState() && !vpn->IsConnectingState()) {
      if (vpn->vpn_provider_type() != shill::kProviderArcVpn) {
        ShowVpnDisconnectedNotification(vpn);
      }
      connected_vpn_.clear();
    }
  } else if (vpn->IsConnectedState()) {
    connected_vpn_ = vpn->path();
  }
}

void NetworkStateNotifier::UpdateCellularOutOfCredits(
    const NetworkState* cellular) {
  // Only display a notification if we are out of credits and have not already
  // shown a notification (or have since connected to another network type).
  if (!cellular->cellular_out_of_credits() || did_show_out_of_credits_)
    return;

  // Only display a notification if not connected, connecting, or waiting to
  // connect to another network.
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  const NetworkState* default_network = handler->DefaultNetwork();
  if (default_network && default_network != cellular)
    return;
  if (handler->ConnectingNetworkByType(NetworkTypePattern::NonVirtual()) ||
      NetworkHandler::Get()
          ->network_connection_handler()
          ->HasPendingConnectRequest())
    return;

  did_show_out_of_credits_ = true;
  base::TimeDelta dtime = base::Time::Now() - out_of_credits_notify_time_;
  if (dtime.InSeconds() > kMinTimeBetweenOutOfCreditsNotifySeconds) {
    out_of_credits_notify_time_ = base::Time::Now();
    base::string16 error_msg = l10n_util::GetStringFUTF16(
        IDS_NETWORK_OUT_OF_CREDITS_BODY, base::UTF8ToUTF16(cellular->name()));
    ShowErrorNotification(
        cellular->path(), kNetworkOutOfCreditsNotificationId, cellular->type(),
        l10n_util::GetStringUTF16(IDS_NETWORK_OUT_OF_CREDITS_TITLE), error_msg,
        base::Bind(&NetworkStateNotifier::ShowNetworkSettings,
                   weak_ptr_factory_.GetWeakPtr(), cellular->guid()));
  }
}

void NetworkStateNotifier::UpdateCellularActivating(
    const NetworkState* cellular) {
  // Keep track of any activating cellular network.
  std::string activation_state = cellular->activation_state();
  if (activation_state == shill::kActivationStateActivating) {
    cellular_activating_.insert(cellular->path());
    return;
  }
  // Only display a notification if this network was activating and is now
  // activated.
  if (!cellular_activating_.count(cellular->path()) ||
      activation_state != shill::kActivationStateActivated)
    return;

  cellular_activating_.erase(cellular->path());
  const gfx::Image& icon =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          cellular->network_technology() == shill::kNetworkTechnologyLte
              ? IDR_NETWORK_ACTIVATED_LTE
              : IDR_NETWORK_ACTIVATED_3G);
  SystemNotificationHelper::GetInstance()->Display(
      *message_center::Notification::CreateSystemNotification(
          kNetworkActivateNotificationId,
          l10n_util::GetStringUTF16(IDS_NETWORK_CELLULAR_ACTIVATED_TITLE),
          l10n_util::GetStringFUTF16(IDS_NETWORK_CELLULAR_ACTIVATED,
                                     base::UTF8ToUTF16((cellular->name()))),
          icon, kNotifierNetwork,
          base::Bind(&NetworkStateNotifier::ShowNetworkSettings,
                     weak_ptr_factory_.GetWeakPtr(), cellular->guid())));
}

void NetworkStateNotifier::ShowNetworkConnectErrorForGuid(
    const std::string& error_name,
    const std::string& guid) {
  const NetworkState* network = GetNetworkStateForGuid(guid);
  if (!network) {
    base::DictionaryValue shill_properties;
    ShowConnectErrorNotification(error_name, "", shill_properties);
    return;
  }
  // Get the up-to-date properties for the network and display the error.
  NetworkHandler::Get()->network_configuration_handler()->GetShillProperties(
      network->path(),
      base::Bind(&NetworkStateNotifier::ConnectErrorPropertiesSucceeded,
                 weak_ptr_factory_.GetWeakPtr(), error_name),
      base::Bind(&NetworkStateNotifier::ConnectErrorPropertiesFailed,
                 weak_ptr_factory_.GetWeakPtr(), error_name, network->path()));
}

void NetworkStateNotifier::ShowMobileActivationErrorForGuid(
    const std::string& guid) {
  const NetworkState* cellular = GetNetworkStateForGuid(guid);
  if (!cellular || cellular->type() != shill::kTypeCellular) {
    NET_LOG(ERROR) << "ShowMobileActivationError without Cellular network: "
                   << guid;
    return;
  }
  SystemNotificationHelper::GetInstance()->Display(
      *message_center::Notification::CreateSystemNotification(
          kNetworkActivateNotificationId,
          l10n_util::GetStringUTF16(IDS_NETWORK_ACTIVATION_ERROR_TITLE),
          l10n_util::GetStringFUTF16(IDS_NETWORK_ACTIVATION_NEEDS_CONNECTION,
                                     base::UTF8ToUTF16(cellular->name())),
          ui::ResourceBundle::GetSharedInstance().GetImageNamed(
              IDR_NETWORK_FAILED_CELLULAR),
          kNotifierNetworkError,
          base::Bind(&NetworkStateNotifier::ShowNetworkSettings,
                     weak_ptr_factory_.GetWeakPtr(), cellular->guid())));
}

void NetworkStateNotifier::RemoveConnectNotification() {
  SystemNotificationHelper::GetInstance()->Close(kNetworkConnectNotificationId);
}

void NetworkStateNotifier::ConnectErrorPropertiesSucceeded(
    const std::string& error_name,
    const std::string& service_path,
    const base::DictionaryValue& shill_properties) {
  std::string state;
  shill_properties.GetStringWithoutPathExpansion(shill::kStateProperty, &state);
  if (NetworkState::StateIsConnected(state) ||
      NetworkState::StateIsConnecting(state)) {
    NET_LOG(EVENT) << "Skipping connect error notification. State: " << state;
    // Network is no longer in an error state. This can happen if an
    // unexpected idle state transition occurs, see http://crbug.com/333955.
    return;
  }
  ShowConnectErrorNotification(error_name, service_path, shill_properties);
}

void NetworkStateNotifier::ConnectErrorPropertiesFailed(
    const std::string& error_name,
    const std::string& service_path,
    const std::string& shill_connect_error,
    std::unique_ptr<base::DictionaryValue> shill_error_data) {
  base::DictionaryValue shill_properties;
  ShowConnectErrorNotification(error_name, service_path, shill_properties);
}

void NetworkStateNotifier::ShowConnectErrorNotification(
    const std::string& error_name,
    const std::string& service_path,
    const base::DictionaryValue& shill_properties) {
  base::string16 error = GetConnectErrorString(error_name);
  NET_LOG(DEBUG) << "Notify: " << service_path
                 << ": Connect error: " << error_name << ": "
                 << base::UTF16ToUTF8(error);

  const NetworkState* network =
      NetworkHandler::Get()->network_state_handler()->GetNetworkState(
          service_path);
  std::string guid = network ? network->guid() : "";

  if (error.empty()) {
    std::string shill_error;
    shill_properties.GetStringWithoutPathExpansion(shill::kErrorProperty,
                                                   &shill_error);
    if (!NetworkState::ErrorIsValid(shill_error)) {
      shill_properties.GetStringWithoutPathExpansion(
          shill::kPreviousErrorProperty, &shill_error);
      NET_LOG(DEBUG) << "Notify: " << service_path
                     << ": Service.PreviousError: " << shill_error;
      if (!NetworkState::ErrorIsValid(shill_error))
        shill_error.clear();
    } else {
      NET_LOG(DEBUG) << "Notify: " << service_path
                     << ": Service.Error: " << shill_error;
    }

    if (network) {
      // Always log last_error, but only use it if shill_error is empty.
      // TODO(stevenjb): This shouldn't ever be necessary, but is kept here as
      // a failsafe since more information is better than less when debugging
      // and we have encountered some strange edge cases before.
      NET_LOG(DEBUG) << "Notify: " << service_path
                     << ": Network.last_error: " << network->last_error();
      if (shill_error.empty())
        shill_error = network->last_error();
    }

    if (ShillErrorIsIgnored(shill_error)) {
      NET_LOG(DEBUG) << "Notify: " << service_path
                     << ": Ignoring error: " << error_name;
      return;
    }
    error = shill_error::GetShillErrorString(shill_error, guid);
    if (error.empty()) {
      if (error_name == NetworkConnectionHandler::kErrorConnectFailed &&
          network && !network->connectable()) {
        // Connect failure on non connectable network with no additional
        // information. We expect the UI to show configuration UI so do not
        // show an additional (and unhelpful) notification.
        return;
      }
      error = l10n_util::GetStringUTF16(IDS_CHROMEOS_NETWORK_ERROR_UNKNOWN);
    }
  }
  NET_LOG(ERROR) << "Notify: " << service_path
                 << ": Connect error: " + base::UTF16ToUTF8(error);

  std::string network_name = shill_property_util::GetNameFromProperties(
      service_path, shill_properties);
  std::string network_error_details;
  shill_properties.GetStringWithoutPathExpansion(shill::kErrorDetailsProperty,
                                                 &network_error_details);

  base::string16 error_msg;
  if (!network_error_details.empty()) {
    // network_name should't be empty if network_error_details is set.
    error_msg = l10n_util::GetStringFUTF16(
        IDS_NETWORK_CONNECTION_ERROR_MESSAGE_WITH_SERVER_MESSAGE,
        base::UTF8ToUTF16(network_name), error,
        base::UTF8ToUTF16(network_error_details));
  } else if (network_name.empty()) {
    error_msg = l10n_util::GetStringFUTF16(
        IDS_NETWORK_CONNECTION_ERROR_MESSAGE_NO_NAME, error);
  } else {
    error_msg =
        l10n_util::GetStringFUTF16(IDS_NETWORK_CONNECTION_ERROR_MESSAGE,
                                   base::UTF8ToUTF16(network_name), error);
  }

  std::string network_type;
  shill_properties.GetStringWithoutPathExpansion(shill::kTypeProperty,
                                                 &network_type);

  ShowErrorNotification(
      service_path, kNetworkConnectNotificationId, network_type,
      l10n_util::GetStringUTF16(IDS_NETWORK_CONNECTION_ERROR_TITLE), error_msg,
      base::Bind(&NetworkStateNotifier::ShowNetworkSettings,
                 weak_ptr_factory_.GetWeakPtr(), guid));
}

void NetworkStateNotifier::ShowVpnDisconnectedNotification(
    const NetworkState* vpn) {
  base::string16 error_msg = l10n_util::GetStringFUTF16(
      IDS_NETWORK_VPN_CONNECTION_LOST_BODY, base::UTF8ToUTF16(vpn->name()));
  ShowErrorNotification(
      vpn->path(), kNetworkConnectNotificationId, shill::kTypeVPN,
      l10n_util::GetStringUTF16(IDS_NETWORK_VPN_CONNECTION_LOST_TITLE),
      error_msg,
      base::Bind(&NetworkStateNotifier::ShowNetworkSettings,
                 weak_ptr_factory_.GetWeakPtr(), vpn->guid()));
}

void NetworkStateNotifier::ShowNetworkSettings(const std::string& network_id) {
  if (!SystemTrayClient::Get())
    return;
  const NetworkState* network = GetNetworkStateForGuid(network_id);
  if (!network)
    return;
  std::string error = network->GetErrorState();
  if (!error.empty()) {
    NET_LOG(ERROR) << "Notify ShowNetworkSettings: " << network_id
                   << ": Error: " << error;
  }
  if (!NetworkTypePattern::Primitive(network->type())
           .MatchesPattern(NetworkTypePattern::Mobile()) &&
      shill_error::IsConfigurationError(error)) {
    SystemTrayClient::Get()->ShowNetworkConfigure(network_id);
  } else {
    SystemTrayClient::Get()->ShowNetworkSettings(network_id);
  }
}

}  // namespace chromeos
