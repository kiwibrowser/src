// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/network_dropdown_handler.h"

#include "chrome/browser/chromeos/login/ui/login_display_webui.h"
#include "chrome/browser/chromeos/options/network_config_view.h"
#include "chrome/browser/ui/webui/chromeos/internet_config_dialog.h"
#include "chrome/browser/ui/webui/chromeos/internet_detail_dialog.h"
#include "chrome/browser/ui/webui/chromeos/login/network_dropdown.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_type_pattern.h"
#include "components/login/localized_values_builder.h"
#include "components/onc/onc_constants.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace {

const char kJsScreenPath[] = "cr.ui.DropDown";

// JS API callbacks names.
const char kJsApiNetworkItemChosen[] = "networkItemChosen";
const char kJsApiNetworkDropdownShow[] = "networkDropdownShow";
const char kJsApiNetworkDropdownHide[] = "networkDropdownHide";
const char kJsApiNetworkDropdownRefresh[] = "networkDropdownRefresh";
const char kJsApiLaunchInternetDetailDialog[] = "launchInternetDetailDialog";
const char kJsApiLaunchAddWiFiNetworkDialog[] = "launchAddWiFiNetworkDialog";
const char kJsApiShowNetworkConfig[] = "showNetworkConfig";
const char kJsApiShowNetworkDetails[] = "showNetworkDetails";

}  // namespace

namespace chromeos {

NetworkDropdownHandler::NetworkDropdownHandler() {
  set_call_js_prefix(kJsScreenPath);
}

NetworkDropdownHandler::~NetworkDropdownHandler() {}

void NetworkDropdownHandler::AddObserver(Observer* observer) {
  if (observer && !observers_.HasObserver(observer))
    observers_.AddObserver(observer);
}

void NetworkDropdownHandler::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void NetworkDropdownHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("selectNetwork", IDS_NETWORK_SELECTION_SELECT);
  builder->Add("selectAnotherNetwork", IDS_ANOTHER_NETWORK_SELECTION_SELECT);
}

void NetworkDropdownHandler::Initialize() {}

void NetworkDropdownHandler::RegisterMessages() {
  AddCallback(kJsApiNetworkItemChosen,
              &NetworkDropdownHandler::HandleNetworkItemChosen);
  AddCallback(kJsApiNetworkDropdownShow,
              &NetworkDropdownHandler::HandleNetworkDropdownShow);
  AddCallback(kJsApiNetworkDropdownHide,
              &NetworkDropdownHandler::HandleNetworkDropdownHide);
  AddCallback(kJsApiNetworkDropdownRefresh,
              &NetworkDropdownHandler::HandleNetworkDropdownRefresh);

  // MD-OOBE
  AddCallback(kJsApiLaunchInternetDetailDialog,
              &NetworkDropdownHandler::HandleLaunchInternetDetailDialog);
  AddCallback(kJsApiLaunchAddWiFiNetworkDialog,
              &NetworkDropdownHandler::HandleLaunchAddWiFiNetworkDialog);
  AddRawCallback(kJsApiShowNetworkDetails,
                 &NetworkDropdownHandler::HandleShowNetworkDetails);
  AddRawCallback(kJsApiShowNetworkConfig,
                 &NetworkDropdownHandler::HandleShowNetworkConfig);
}

void NetworkDropdownHandler::HandleLaunchInternetDetailDialog() {
  // Empty string opens the internet detail dialog for the default network.
  InternetDetailDialog::ShowDialog("");
}

void NetworkDropdownHandler::HandleLaunchAddWiFiNetworkDialog() {
  // Make sure WiFi is enabled.
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  if (handler->GetTechnologyState(NetworkTypePattern::WiFi()) !=
      NetworkStateHandler::TECHNOLOGY_ENABLED) {
    handler->SetTechnologyEnabled(NetworkTypePattern::WiFi(), true,
                                  network_handler::ErrorCallback());
  }
  if (chromeos::switches::IsNetworkSettingsConfigEnabled()) {
    chromeos::InternetConfigDialog::ShowDialogForNetworkType(
        ::onc::network_type::kWiFi);
  } else {
    NetworkConfigView::ShowForType(shill::kTypeWifi);
  }
}

void NetworkDropdownHandler::HandleShowNetworkDetails(
    const base::ListValue* args) {
  std::string type, guid;
  args->GetString(0, &type);
  args->GetString(1, &guid);
  if (type == ::onc::network_type::kCellular) {
    // Make sure Cellular is enabled.
    NetworkStateHandler* handler =
        NetworkHandler::Get()->network_state_handler();
    if (handler->GetTechnologyState(NetworkTypePattern::Cellular()) !=
        NetworkStateHandler::TECHNOLOGY_ENABLED) {
      handler->SetTechnologyEnabled(NetworkTypePattern::Cellular(), true,
                                    network_handler::ErrorCallback());
    }
  }
  InternetDetailDialog::ShowDialog(guid);
}

void NetworkDropdownHandler::HandleShowNetworkConfig(
    const base::ListValue* args) {
  std::string guid;
  args->GetString(0, &guid);
  if (chromeos::switches::IsNetworkSettingsConfigEnabled())
    chromeos::InternetConfigDialog::ShowDialogForNetworkId(guid);
  else
    NetworkConfigView::ShowForNetworkId(guid);
}

void NetworkDropdownHandler::OnConnectToNetworkRequested() {
  for (Observer& observer : observers_)
    observer.OnConnectToNetworkRequested();
}

void NetworkDropdownHandler::HandleNetworkItemChosen(double id) {
  if (dropdown_.get()) {
    dropdown_->OnItemChosen(static_cast<int>(id));
  } else {
    // It could happen with very low probability but still keep NOTREACHED to
    // detect if it starts happening all the time.
    NOTREACHED();
  }
}

void NetworkDropdownHandler::HandleNetworkDropdownShow(
    const std::string& element_id,
    bool oobe) {
  dropdown_.reset(new NetworkDropdown(this, web_ui(), oobe));
}

void NetworkDropdownHandler::HandleNetworkDropdownHide() {
  dropdown_.reset();
}

void NetworkDropdownHandler::HandleNetworkDropdownRefresh() {
  // Since language change is async,
  // we may in theory be on another screen during this call.
  if (dropdown_.get())
    dropdown_->Refresh();
}

}  // namespace chromeos
