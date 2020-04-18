// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/network_ui.h"

#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/chromeos/options/network_config_view.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/ui/webui/chromeos/network_element_localized_strings_provider.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/network/device_state.h"
#include "chromeos/network/network_configuration_handler.h"
#include "chromeos/network/network_device_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/onc/onc_utils.h"
#include "components/device_event_log/device_event_log.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/l10n/l10n_util.h"

namespace chromeos {

namespace {

constexpr char kGetNetworkProperties[] = "getShillNetworkProperties";
constexpr char kGetDeviceProperties[] = "getShillDeviceProperties";

bool GetServicePathFromGuid(const std::string& guid,
                            std::string* service_path) {
  const NetworkState* network =
      NetworkHandler::Get()->network_state_handler()->GetNetworkStateFromGuid(
          guid);
  if (!network)
    return false;
  *service_path = network->path();
  return true;
}

void SetDeviceProperties(base::DictionaryValue* dictionary) {
  std::string device;
  dictionary->GetStringWithoutPathExpansion(shill::kDeviceProperty, &device);
  const DeviceState* device_state =
      NetworkHandler::Get()->network_state_handler()->GetDeviceState(device);
  if (!device_state)
    return;

  std::unique_ptr<base::DictionaryValue> device_dictionary(
      device_state->properties().DeepCopy());

  if (!device_state->ip_configs().empty()) {
    // Convert IPConfig dictionary to a ListValue.
    std::unique_ptr<base::ListValue> ip_configs(new base::ListValue);
    for (base::DictionaryValue::Iterator iter(device_state->ip_configs());
         !iter.IsAtEnd(); iter.Advance()) {
      ip_configs->Append(iter.value().CreateDeepCopy());
    }
    device_dictionary->SetWithoutPathExpansion(shill::kIPConfigsProperty,
                                               std::move(ip_configs));
  }
  if (!device_dictionary->empty())
    dictionary->Set(shill::kDeviceProperty, std::move(device_dictionary));
}

class NetworkConfigMessageHandler : public content::WebUIMessageHandler {
 public:
  NetworkConfigMessageHandler() : weak_ptr_factory_(this) {}
  ~NetworkConfigMessageHandler() override {}

  // WebUIMessageHandler implementation.
  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        kGetNetworkProperties,
        base::BindRepeating(
            &NetworkConfigMessageHandler::GetShillNetworkProperties,
            base::Unretained(this)));
    web_ui()->RegisterMessageCallback(
        kGetDeviceProperties,
        base::BindRepeating(
            &NetworkConfigMessageHandler::GetShillDeviceProperties,
            base::Unretained(this)));
    web_ui()->RegisterMessageCallback(
        "addNetwork",
        base::BindRepeating(&NetworkConfigMessageHandler::AddNetwork,
                            base::Unretained(this)));
  }

 private:
  void GetShillNetworkProperties(const base::ListValue* arg_list) {
    std::string guid;
    if (!arg_list->GetString(0, &guid)) {
      NOTREACHED();
      return;
    }
    std::string service_path;
    if (!GetServicePathFromGuid(guid, &service_path)) {
      ErrorCallback(guid, kGetNetworkProperties, "Error.InvalidNetworkGuid",
                    nullptr);
      return;
    }
    NetworkHandler::Get()->network_configuration_handler()->GetShillProperties(
        service_path,
        base::Bind(
            &NetworkConfigMessageHandler::GetShillNetworkPropertiesSuccess,
            weak_ptr_factory_.GetWeakPtr()),
        base::Bind(&NetworkConfigMessageHandler::ErrorCallback,
                   weak_ptr_factory_.GetWeakPtr(), guid,
                   kGetNetworkProperties));
  }

  void GetShillNetworkPropertiesSuccess(
      const std::string& service_path,
      const base::DictionaryValue& dictionary) {
    std::unique_ptr<base::DictionaryValue> dictionary_copy(
        dictionary.DeepCopy());

    // Set the 'service_path' property for debugging.
    dictionary_copy->SetKey("service_path", base::Value(service_path));
    // Set the device properties for debugging.
    SetDeviceProperties(dictionary_copy.get());

    base::ListValue return_arg_list;
    return_arg_list.Append(std::move(dictionary_copy));

    AllowJavascript();
    CallJavascriptFunction(
        base::StringPrintf("NetworkUI.%sResult", kGetNetworkProperties),
        return_arg_list);
  }

  void GetShillDeviceProperties(const base::ListValue* arg_list) {
    std::string type;
    if (!arg_list->GetString(0, &type)) {
      NOTREACHED();
      return;
    }
    const DeviceState* device =
        NetworkHandler::Get()->network_state_handler()->GetDeviceStateByType(
            onc::NetworkTypePatternFromOncType(type));
    if (!device) {
      ErrorCallback(type, kGetDeviceProperties, "Error.InvalidDeviceType",
                    nullptr);
      return;
    }
    NetworkHandler::Get()->network_device_handler()->GetDeviceProperties(
        device->path(),
        base::Bind(
            &NetworkConfigMessageHandler::GetShillDevicePropertiesSuccess,
            weak_ptr_factory_.GetWeakPtr()),
        base::Bind(&NetworkConfigMessageHandler::ErrorCallback,
                   weak_ptr_factory_.GetWeakPtr(), type, kGetDeviceProperties));
  }

  void GetShillDevicePropertiesSuccess(
      const std::string& device_path,
      const base::DictionaryValue& dictionary) {
    std::unique_ptr<base::DictionaryValue> dictionary_copy(
        dictionary.DeepCopy());

    // Set the 'device_path' property for debugging.
    dictionary_copy->SetKey("device_path", base::Value(device_path));

    base::ListValue return_arg_list;
    return_arg_list.Append(std::move(dictionary_copy));

    AllowJavascript();
    CallJavascriptFunction(
        base::StringPrintf("NetworkUI.%sResult", kGetDeviceProperties),
        return_arg_list);
  }

  void ErrorCallback(const std::string& guid_or_type,
                     const std::string& function_name,
                     const std::string& error_name,
                     std::unique_ptr<base::DictionaryValue> /* error_data */) {
    NET_LOG(ERROR) << "Shill Error: " << error_name << " id=" << guid_or_type;
    base::ListValue return_arg_list;
    std::unique_ptr<base::DictionaryValue> dictionary;
    std::string key = function_name == kGetDeviceProperties
                          ? shill::kTypeProperty
                          : shill::kGuidProperty;
    dictionary->SetKey(key, base::Value(guid_or_type));
    dictionary->SetKey("ShillError", base::Value(error_name));
    return_arg_list.Append(std::move(dictionary));

    AllowJavascript();
    CallJavascriptFunction(
        base::StringPrintf("NetworkUI.%sResult", function_name.c_str()),
        return_arg_list);
  }

  void AddNetwork(const base::ListValue* args) {
    std::string onc_type;
    args->GetString(0, &onc_type);
    std::string shill_type = (onc_type == ::onc::network_type::kVPN)
                                 ? shill::kTypeVPN
                                 : shill::kTypeWifi;
    NetworkConfigView::ShowForType(shill_type);
  }

  base::WeakPtrFactory<NetworkConfigMessageHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkConfigMessageHandler);
};

}  // namespace

// static
void NetworkUI::GetLocalizedStrings(base::DictionaryValue* localized_strings) {
  localized_strings->SetString("titleText",
                               l10n_util::GetStringUTF16(IDS_NETWORK_UI_TITLE));

  localized_strings->SetString("titleText",
                               l10n_util::GetStringUTF16(IDS_NETWORK_UI_TITLE));
  localized_strings->SetString(
      "autoRefreshText",
      l10n_util::GetStringUTF16(IDS_NETWORK_UI_AUTO_REFRESH));
  localized_strings->SetString(
      "deviceLogLinkText", l10n_util::GetStringUTF16(IDS_DEVICE_LOG_LINK_TEXT));
  localized_strings->SetString(
      "networkRefreshText", l10n_util::GetStringUTF16(IDS_NETWORK_UI_REFRESH));
  localized_strings->SetString(
      "clickToExpandText", l10n_util::GetStringUTF16(IDS_NETWORK_UI_EXPAND));
  localized_strings->SetString(
      "propertyFormatText",
      l10n_util::GetStringUTF16(IDS_NETWORK_UI_PROPERTY_FORMAT));

  localized_strings->SetString(
      "normalFormatOption",
      l10n_util::GetStringUTF16(IDS_NETWORK_UI_FORMAT_NORMAL));
  localized_strings->SetString(
      "managedFormatOption",
      l10n_util::GetStringUTF16(IDS_NETWORK_UI_FORMAT_MANAGED));
  localized_strings->SetString(
      "stateFormatOption",
      l10n_util::GetStringUTF16(IDS_NETWORK_UI_FORMAT_STATE));
  localized_strings->SetString(
      "shillFormatOption",
      l10n_util::GetStringUTF16(IDS_NETWORK_UI_FORMAT_SHILL));

  localized_strings->SetString(
      "globalPolicyLabel",
      l10n_util::GetStringUTF16(IDS_NETWORK_UI_GLOBAL_POLICY));
  localized_strings->SetString(
      "networkListsLabel",
      l10n_util::GetStringUTF16(IDS_NETWORK_UI_NETWORK_LISTS));
  localized_strings->SetString(
      "visibleNetworksLabel",
      l10n_util::GetStringUTF16(IDS_NETWORK_UI_VISIBLE_NETWORKS));
  localized_strings->SetString(
      "favoriteNetworksLabel",
      l10n_util::GetStringUTF16(IDS_NETWORK_UI_FAVORITE_NETWORKS));
  localized_strings->SetString(
      "devicesLabel", l10n_util::GetStringUTF16(IDS_NETWORK_UI_DEVICES));
}

NetworkUI::NetworkUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<NetworkConfigMessageHandler>());

  // Enable extension API calls in the WebUI.
  extensions::TabHelper::CreateForWebContents(web_ui->GetWebContents());

  base::DictionaryValue localized_strings;
  GetLocalizedStrings(&localized_strings);

  content::WebUIDataSource* html =
      content::WebUIDataSource::Create(chrome::kChromeUINetworkHost);
  html->AddLocalizedStrings(localized_strings);

  network_element::AddLocalizedStrings(html);

  html->SetJsonPath("strings.js");
  html->AddResourcePath("network_ui.css", IDR_NETWORK_UI_CSS);
  html->AddResourcePath("network_ui.js", IDR_NETWORK_UI_JS);
  html->SetDefaultResource(IDR_NETWORK_UI_HTML);

  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                html);
}

NetworkUI::~NetworkUI() {}

}  // namespace chromeos
