// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/status/network_menu.h"

#include <stddef.h>

#include <algorithm>

#include "ash/system/network/network_icon.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/mobile_config.h"
#include "chrome/browser/chromeos/options/network_config_view.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/webui/chromeos/internet_config_dialog.h"
#include "chrome/browser/ui/webui/chromeos/internet_detail_dialog.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/network/device_state.h"
#include "chromeos/network/network_connect.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_util.h"
#include "components/onc/onc_constants.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia.h"

namespace chromeos {

namespace {

// Offsets for views menu ids (main menu and submenu ids use the same
// namespace).
const int kMainIndexMask = 0x1000;
const int kMoreIndexMask = 0x4000;

const ui::ResourceBundle::FontStyle kAssociatedNetworkFontStyle =
    ui::ResourceBundle::BoldFont;

// Replace '&' in a string with "&&" to allow it to be a menu item label.
std::string EscapeAmpersands(const std::string& input) {
  std::string str = input;
  size_t found = str.find('&');
  while (found != std::string::npos) {
    str.replace(found, 1, "&&");
    found = str.find('&', found + 2);
  }
  return str;
}

// Highlight any connected or connecting networks in the UI.
bool ShouldHighlightNetwork(const NetworkState* network) {
  return network->IsConnectedState() || network->IsConnectingState();
}

void ToggleTechnology(const NetworkTypePattern& technology) {
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  bool is_enabled = handler->IsTechnologyEnabled(technology);
  NetworkConnect::Get()->SetTechnologyEnabled(technology, !is_enabled);
}

}  // namespace

class NetworkMenuModel : public ui::MenuModel {
 public:
  struct MenuItem {
    MenuItem()
        : type(ui::MenuModel::TYPE_SEPARATOR),
          sub_menu_model(NULL),
          flags(0) {
    }
    MenuItem(ui::MenuModel::ItemType type,
             base::string16 label,
             gfx::ImageSkia icon,
             const std::string& service_path,
             int flags)
        : type(type),
          label(label),
          icon(icon),
          service_path(service_path),
          sub_menu_model(NULL),
          flags(flags) {
    }
    MenuItem(ui::MenuModel::ItemType type,
             base::string16 label,
             gfx::ImageSkia icon,
             NetworkMenuModel* sub_menu_model,
             int flags)
        : type(type),
          label(label),
          icon(icon),
          sub_menu_model(sub_menu_model),
          flags(flags) {
    }

    ui::MenuModel::ItemType type;
    base::string16 label;
    gfx::ImageSkia icon;
    std::string service_path;
    NetworkMenuModel* sub_menu_model;  // Weak ptr.
    int flags;
  };
  typedef std::vector<MenuItem> MenuItemVector;

  explicit NetworkMenuModel(const base::WeakPtr<NetworkMenu>& owner)
    : owner_(owner) {}
  ~NetworkMenuModel() override {}

  // Connect or reconnect to the network at |index|.
  void ConnectToNetworkAt(int index);

  // Called by NetworkMenu::UpdateMenu to initialize menu items.
  virtual void InitMenuItems(bool should_open_button_options) = 0;

  // Menu item field accessors.
  const MenuItemVector& menu_items() const { return menu_items_; }

  // ui::MenuModel implementation
  // GetCommandIdAt() must be implemented by subclasses.
  bool HasIcons() const override;
  int GetItemCount() const override;
  ui::MenuModel::ItemType GetTypeAt(int index) const override;
  ui::MenuSeparatorType GetSeparatorTypeAt(int index) const override;
  base::string16 GetLabelAt(int index) const override;
  bool IsItemDynamicAt(int index) const override;
  const gfx::FontList* GetLabelFontListAt(int index) const override;
  bool GetAcceleratorAt(int index, ui::Accelerator* accelerator) const override;
  bool IsItemCheckedAt(int index) const override;
  int GetGroupIdAt(int index) const override;
  bool GetIconAt(int index, gfx::Image* icon) override;
  ui::ButtonMenuItemModel* GetButtonMenuItemAt(int index) const override;
  bool IsEnabledAt(int index) const override;
  bool IsVisibleAt(int index) const override;
  ui::MenuModel* GetSubmenuModelAt(int index) const override;
  void HighlightChangedTo(int index) override;
  void ActivatedAt(int index) override;
  void SetMenuModelDelegate(ui::MenuModelDelegate* delegate) override;
  ui::MenuModelDelegate* GetMenuModelDelegate() const override;

 protected:
  enum MenuItemFlags {
    FLAG_NONE              = 0,
    FLAG_DISABLED          = 1 << 0,
    FLAG_TOGGLE_WIFI       = 1 << 2,
    FLAG_TOGGLE_MOBILE     = 1 << 3,
    FLAG_ASSOCIATED        = 1 << 5,
    FLAG_ETHERNET          = 1 << 6,
    FLAG_WIFI              = 1 << 7,
    FLAG_WIMAX             = 1 << 8,
    FLAG_CELLULAR          = 1 << 9,
    FLAG_OPTIONS           = 1 << 10,
    FLAG_ADD_WIFI          = 1 << 11,
    FLAG_ADD_CELLULAR      = 1 << 12,
  };

  // Our menu items.
  MenuItemVector menu_items_;

  // Weak pointer to NetworkMenu that owns this MenuModel.
  base::WeakPtr<NetworkMenu> owner_;

 private:
  // Open a dialog to set up and connect to a network.
  void ShowOther(const std::string& type) const;

  DISALLOW_COPY_AND_ASSIGN(NetworkMenuModel);
};

class MoreMenuModel : public NetworkMenuModel {
 public:
  explicit MoreMenuModel(const base::WeakPtr<NetworkMenu> owner)
    : NetworkMenuModel(owner) {}
  ~MoreMenuModel() override {}

  // NetworkMenuModel implementation.
  void InitMenuItems(bool should_open_button_options) override;

  // ui::MenuModel implementation
  int GetCommandIdAt(int index) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MoreMenuModel);
};

class MainMenuModel : public NetworkMenuModel {
 public:
  explicit MainMenuModel(const base::WeakPtr<NetworkMenu>& owner)
      : NetworkMenuModel(owner),
        more_menu_model_(new MoreMenuModel(owner)) {
  }
  ~MainMenuModel() override {}

  // NetworkMenuModel implementation.
  void InitMenuItems(bool should_open_button_options) override;

  // ui::MenuModel implementation
  int GetCommandIdAt(int index) const override;

 private:
  void AddWirelessNetworkMenuItem(const NetworkState* wifi_network, int flag);
  void AddMessageItem(const base::string16& msg);

  std::unique_ptr<MoreMenuModel> more_menu_model_;

  DISALLOW_COPY_AND_ASSIGN(MainMenuModel);
};

////////////////////////////////////////////////////////////////////////////////
// NetworkMenuModel, public methods:

void NetworkMenuModel::ConnectToNetworkAt(int index) {
  const std::string& service_path = menu_items_[index].service_path;
  const NetworkState* network =
      NetworkHandler::Get()->network_state_handler()->GetNetworkState(
          service_path);
  if (network)
    NetworkConnect::Get()->ConnectToNetworkId(network->guid());
}

////////////////////////////////////////////////////////////////////////////////
// NetworkMenuModel, ui::MenuModel implementation:

bool NetworkMenuModel::HasIcons() const {
  return true;
}

int NetworkMenuModel::GetItemCount() const {
  return static_cast<int>(menu_items_.size());
}

ui::MenuModel::ItemType NetworkMenuModel::GetTypeAt(int index) const {
  return menu_items_[index].type;
}

ui::MenuSeparatorType NetworkMenuModel::GetSeparatorTypeAt(int index) const {
  return ui::NORMAL_SEPARATOR;
}

base::string16 NetworkMenuModel::GetLabelAt(int index) const {
  return menu_items_[index].label;
}

bool NetworkMenuModel::IsItemDynamicAt(int index) const {
  return false;
}

const gfx::FontList* NetworkMenuModel::GetLabelFontListAt(int index) const {
  const gfx::FontList* font_list = NULL;
  if (menu_items_[index].flags & FLAG_ASSOCIATED) {
    ui::ResourceBundle* rb = &ui::ResourceBundle::GetSharedInstance();
    font_list = &rb->GetFontList(kAssociatedNetworkFontStyle);
  }

  return font_list;
}

bool NetworkMenuModel::GetAcceleratorAt(int index,
                                        ui::Accelerator* accelerator) const {
  return false;
}

bool NetworkMenuModel::IsItemCheckedAt(int index) const {
  // All ui::MenuModel::TYPE_CHECK menu items are checked.
  return true;
}

int NetworkMenuModel::GetGroupIdAt(int index) const {
  return 0;
}

bool NetworkMenuModel::GetIconAt(int index, gfx::Image* icon) {
  if (!menu_items_[index].icon.isNull()) {
    *icon = gfx::Image(menu_items_[index].icon);
    return true;
  }
  return false;
}

ui::ButtonMenuItemModel* NetworkMenuModel::GetButtonMenuItemAt(
    int index) const {
  return NULL;
}

bool NetworkMenuModel::IsEnabledAt(int index) const {
  return !(menu_items_[index].flags & FLAG_DISABLED);
}

bool NetworkMenuModel::IsVisibleAt(int index) const {
  return true;
}

ui::MenuModel* NetworkMenuModel::GetSubmenuModelAt(int index) const {
  return menu_items_[index].sub_menu_model;
}

void NetworkMenuModel::HighlightChangedTo(int index) {
}

void NetworkMenuModel::ActivatedAt(int index) {
  // When we are refreshing the menu, ignore menu item activation.
  if (owner_->refreshing_menu_)
    return;

  int flags = menu_items_[index].flags;
  if (flags & FLAG_OPTIONS) {
    owner_->delegate()->OpenButtonOptions();
  } else if (flags & FLAG_TOGGLE_WIFI) {
    ToggleTechnology(NetworkTypePattern::WiFi());
  } else if (flags & FLAG_TOGGLE_MOBILE) {
    ToggleTechnology(NetworkTypePattern::Mobile());
  } else if (flags & FLAG_ETHERNET) {
    owner_->delegate()->OnConnectToNetworkRequested();
  } else if (flags & (FLAG_WIFI | FLAG_WIMAX | FLAG_CELLULAR)) {
    ConnectToNetworkAt(index);
    owner_->delegate()->OnConnectToNetworkRequested();
  } else if (flags & FLAG_ADD_WIFI) {
    ShowOther(shill::kTypeWifi);
  } else if (flags & FLAG_ADD_CELLULAR) {
    ShowOther(shill::kTypeCellular);
  }
}

void NetworkMenuModel::SetMenuModelDelegate(ui::MenuModelDelegate* delegate) {
}

ui::MenuModelDelegate* NetworkMenuModel::GetMenuModelDelegate() const {
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// NetworkMenuModel, private methods:

void NetworkMenuModel::ShowOther(const std::string& type) const {
  // Note: this UI is deprecated and only used in the login screen when no
  // network is available.
  if (type == shill::kTypeCellular) {
    // 'Other mobile network...' shows the detail dialog for a Cellular
    // network (if it exists) so that the mobile data network and access point
    // can be configured.
    const NetworkState* network =
        NetworkHandler::Get()->network_state_handler()->FirstNetworkByType(
            NetworkTypePattern::Cellular());
    if (network)
      InternetDetailDialog::ShowDialog(network->guid());
    return;
  }
  if (chromeos::switches::IsNetworkSettingsConfigEnabled()) {
    chromeos::InternetConfigDialog::ShowDialogForNetworkType(
        chromeos::network_util::TranslateShillTypeToONC(type));
  } else {
    NetworkConfigView::ShowForType(type);
  }
}

////////////////////////////////////////////////////////////////////////////////
// MainMenuModel

void MainMenuModel::AddWirelessNetworkMenuItem(const NetworkState* network,
                                               int flag) {
  base::string16 label;
  // Ampersand is a valid character in an SSID, but menu2 uses it to mark
  // "mnemonics" for keyboard shortcuts.
  std::string wifi_name = EscapeAmpersands(network->name());
  if (network->IsConnectingState()) {
    label = l10n_util::GetStringFUTF16(
        IDS_STATUSBAR_NETWORK_DEVICE_STATUS,
        base::UTF8ToUTF16(wifi_name),
        l10n_util::GetStringUTF16(IDS_STATUSBAR_NETWORK_DEVICE_CONNECTING));
  } else {
    label = base::UTF8ToUTF16(wifi_name);
  }

  // We do not have convenient access to whether or not it might be possible
  // to connect to a wireless network (e.g. whether certs are required), so all
  // entries are enabled.

  if (ShouldHighlightNetwork(network))
    flag |= FLAG_ASSOCIATED;
  const gfx::ImageSkia icon = ash::network_icon::GetImageForNetwork(
      network, ash::network_icon::ICON_TYPE_LIST);
  menu_items_.push_back(
      MenuItem(ui::MenuModel::TYPE_COMMAND,
               label, icon, network->path(), flag));
}

void MainMenuModel::AddMessageItem(const base::string16& msg) {
  menu_items_.push_back(MenuItem(
      ui::MenuModel::TYPE_COMMAND, msg,
      gfx::ImageSkia(), std::string(), FLAG_DISABLED));
}

void MainMenuModel::InitMenuItems(bool should_open_button_options) {
  menu_items_.clear();

  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();

  // Populate our MenuItems with the current list of networks.
  base::string16 label;

  // Ethernet
  // Only display an ethernet icon if enabled, and an ethernet network exists.
  bool ethernet_enabled =
      handler->IsTechnologyEnabled(NetworkTypePattern::Ethernet());
  const NetworkState* ethernet_network =
      handler->FirstNetworkByType(NetworkTypePattern::Ethernet());
  if (ethernet_enabled && ethernet_network) {
    bool ethernet_connecting = ethernet_network->IsConnectingState();
    if (ethernet_connecting) {
      label = l10n_util::GetStringFUTF16(
          IDS_STATUSBAR_NETWORK_DEVICE_STATUS,
          l10n_util::GetStringUTF16(IDS_STATUSBAR_NETWORK_DEVICE_ETHERNET),
          l10n_util::GetStringUTF16(IDS_STATUSBAR_NETWORK_DEVICE_CONNECTING));
    } else {
      label = l10n_util::GetStringUTF16(IDS_STATUSBAR_NETWORK_DEVICE_ETHERNET);
    }
    int flag = FLAG_ETHERNET;
    if (ShouldHighlightNetwork(ethernet_network))
      flag |= FLAG_ASSOCIATED;
    const gfx::ImageSkia icon = ash::network_icon::GetImageForNetwork(
        ethernet_network, ash::network_icon::ICON_TYPE_LIST);
    menu_items_.push_back(MenuItem(ui::MenuModel::TYPE_COMMAND,
                                   label, icon, std::string(), flag));
  }

  // Get the list of all networks.
  NetworkStateHandler::NetworkStateList network_list;
  handler->GetVisibleNetworkList(&network_list);

  // Cellular Networks
  if (handler->IsTechnologyEnabled(NetworkTypePattern::Cellular())) {
    // List Cellular networks.
    for (NetworkStateHandler::NetworkStateList::const_iterator iter =
             network_list.begin(); iter != network_list.end(); ++iter) {
      const NetworkState* network = *iter;
      if (network->type() != shill::kTypeCellular)
        continue;
      std::string activation_state = network->activation_state();

      // This is only used in the login screen; do not show unactivated
      // networks.
      if (activation_state != shill::kActivationStateActivated)
        continue;

      // Ampersand is a valid character in a network name, but menu2 uses it
      // to mark "mnemonics" for keyboard shortcuts.  http://crosbug.com/14697
      std::string network_name = EscapeAmpersands(network->name());
      if (network->IsConnectingState()) {
        label = l10n_util::GetStringFUTF16(
            IDS_STATUSBAR_NETWORK_DEVICE_STATUS,
            base::UTF8ToUTF16(network_name),
            l10n_util::GetStringUTF16(IDS_STATUSBAR_NETWORK_DEVICE_CONNECTING));
      } else {
        label = base::UTF8ToUTF16(network_name);
      }

      int flag = FLAG_CELLULAR;
      bool isActive = ShouldHighlightNetwork(network);
      if (isActive)
        flag |= FLAG_ASSOCIATED;
      const gfx::ImageSkia icon = ash::network_icon::GetImageForNetwork(
          network, ash::network_icon::ICON_TYPE_LIST);
      menu_items_.push_back(
          MenuItem(ui::MenuModel::TYPE_COMMAND,
                   label, icon, network->path(), flag));
    }

    // For GSM add cellular network scan.
    const DeviceState* cellular_device =
        handler->GetDeviceStateByType(NetworkTypePattern::Cellular());
    if (cellular_device && cellular_device->support_network_scan()) {
      const gfx::ImageSkia icon =
          ash::network_icon::GetImageForDisconnectedCellNetwork();
      menu_items_.push_back(MenuItem(
          ui::MenuModel::TYPE_COMMAND,
          l10n_util::GetStringUTF16(IDS_OPTIONS_SETTINGS_CONFIGURE_CELLULAR),
          icon, std::string(), FLAG_ADD_CELLULAR));
    }
  } else {
    int initializing_message_id =
        ash::network_icon::GetCellularUninitializedMsg();
    if (initializing_message_id) {
      // Initializing cellular modem...
      AddMessageItem(l10n_util::GetStringUTF16(initializing_message_id));
    }
  }

  // Wimax Networks
  if (handler->IsTechnologyEnabled(NetworkTypePattern::Wimax())) {
    // List Wimax networks.
    for (NetworkStateHandler::NetworkStateList::const_iterator iter =
             network_list.begin(); iter != network_list.end(); ++iter) {
      const NetworkState* network = *iter;
      if (network->type() != shill::kTypeWimax)
        continue;
      AddWirelessNetworkMenuItem(network, FLAG_WIMAX);
    }
  }

  // Wifi Networks
  if (handler->IsTechnologyEnabled(NetworkTypePattern::WiFi())) {
    // List Wifi networks.
    int scanning_msg = handler->GetScanningByType(NetworkTypePattern::WiFi())
                           ? IDS_WIFI_SCANNING_MESSAGE
                           : 0;
    for (NetworkStateHandler::NetworkStateList::const_iterator iter =
             network_list.begin(); iter != network_list.end(); ++iter) {
      const NetworkState* network = *iter;
      if (network->type() != shill::kTypeWifi)
        continue;
      // Add 'Searching for Wi-Fi networks...' after connected networks.
      if (scanning_msg && !network->IsConnectedState()) {
        AddMessageItem(l10n_util::GetStringUTF16(scanning_msg));
        scanning_msg = 0;
      }
      AddWirelessNetworkMenuItem(network, FLAG_WIFI);
    }
    if (scanning_msg)
      AddMessageItem(l10n_util::GetStringUTF16(scanning_msg));
    const gfx::ImageSkia icon =
        ash::network_icon::GetImageForWiFiEnabledState(true);
    menu_items_.push_back(MenuItem(
        ui::MenuModel::TYPE_COMMAND,
        l10n_util::GetStringUTF16(IDS_OPTIONS_SETTINGS_OTHER_WIFI_NETWORKS),
        icon, std::string(), FLAG_ADD_WIFI));
  }

  if (menu_items_.empty()) {
    // No networks available (and not initializing cellular or wifi scanning)
    AddMessageItem(l10n_util::GetStringFUTF16(
        IDS_STATUSBAR_NETWORK_MENU_ITEM_INDENT,
        l10n_util::GetStringUTF16(IDS_STATUSBAR_NO_NETWORKS_MESSAGE)));
  }

  // Enable / Disable Technology
  NetworkStateHandler::TechnologyState wifi_state =
      handler->GetTechnologyState(NetworkTypePattern::WiFi());
  bool wifi_available =
      wifi_state != NetworkStateHandler::TECHNOLOGY_UNAVAILABLE;
  bool wifi_enabled = wifi_state == NetworkStateHandler::TECHNOLOGY_ENABLED;

  NetworkStateHandler::TechnologyState mobile_state =
      handler->GetTechnologyState(NetworkTypePattern::Mobile());
  bool mobile_available =
      mobile_state != NetworkStateHandler::TECHNOLOGY_UNAVAILABLE;
  bool mobile_enabled = mobile_state == NetworkStateHandler::TECHNOLOGY_ENABLED;

  // Do not show disable wifi or cellular during oobe.
  bool show_toggle_wifi = wifi_available &&
      (should_open_button_options || !wifi_enabled);
  bool show_toggle_mobile = mobile_available &&
      (should_open_button_options || !mobile_enabled);

  if (show_toggle_wifi || show_toggle_mobile) {
    menu_items_.push_back(MenuItem());  // Separator

    if (show_toggle_wifi) {
      int id = wifi_enabled ? IDS_STATUSBAR_NETWORK_DEVICE_DISABLE :
                              IDS_STATUSBAR_NETWORK_DEVICE_ENABLE;
      label = l10n_util::GetStringFUTF16(id,
          l10n_util::GetStringUTF16(IDS_STATUSBAR_NETWORK_DEVICE_WIFI));
      int flag = FLAG_TOGGLE_WIFI;
      if (wifi_state == NetworkStateHandler::TECHNOLOGY_ENABLING)
        flag |= FLAG_DISABLED;
      menu_items_.push_back(MenuItem(ui::MenuModel::TYPE_COMMAND, label,
          gfx::ImageSkia(), std::string(), flag));
    }

    if (show_toggle_mobile) {
      const DeviceState* mobile_device =
          handler->GetDeviceStateByType(NetworkTypePattern::Mobile());
      bool is_locked = mobile_device && !mobile_device->sim_lock_type().empty();
      int id = (mobile_enabled && !is_locked)
          ? IDS_STATUSBAR_NETWORK_DEVICE_DISABLE
          : IDS_STATUSBAR_NETWORK_DEVICE_ENABLE;
      label = l10n_util::GetStringFUTF16(id,
          l10n_util::GetStringUTF16(IDS_STATUSBAR_NETWORK_DEVICE_CELLULAR));
      gfx::ImageSkia icon;
      int flag = FLAG_TOGGLE_MOBILE;
      if (mobile_state == NetworkStateHandler::TECHNOLOGY_ENABLING)
        flag |= FLAG_DISABLED;
      menu_items_.push_back(MenuItem(ui::MenuModel::TYPE_COMMAND, label,
          icon, std::string(), flag));
    }
  }

  // Additional links like:
  // * IP Address on active interface;
  // * Hardware addresses for wifi and ethernet.
  more_menu_model_->InitMenuItems(should_open_button_options);
  if (!more_menu_model_->menu_items().empty()) {
    menu_items_.push_back(MenuItem());  // Separator
    menu_items_.push_back(MenuItem(
        ui::MenuModel::TYPE_SUBMENU,
        l10n_util::GetStringUTF16(IDS_STATUSBAR_NETWORK_MORE),
        gfx::ImageSkia(), more_menu_model_.get(), FLAG_NONE));
  }
}

int MainMenuModel::GetCommandIdAt(int index) const {
  return index + kMainIndexMask;
}

////////////////////////////////////////////////////////////////////////////////
// MoreMenuModel

void MoreMenuModel::InitMenuItems(bool should_open_button_options) {
  menu_items_.clear();
  MenuItemVector link_items;
  MenuItemVector address_items;

  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  const NetworkState* default_network = handler->DefaultNetwork();

  int message_id = -1;
  if (default_network)
    message_id = IDS_STATUSBAR_NETWORK_OPEN_PROXY_SETTINGS_DIALOG;
  if (message_id != -1) {
    link_items.push_back(MenuItem(ui::MenuModel::TYPE_COMMAND,
                                  l10n_util::GetStringUTF16(message_id),
                                  gfx::ImageSkia(),
                                  std::string(),
                                  FLAG_OPTIONS));
  }

  if (default_network) {
    std::string ip_address = default_network->GetIpAddress();
    if (!ip_address.empty()) {
      address_items.push_back(MenuItem(
          ui::MenuModel::TYPE_COMMAND, base::ASCIIToUTF16(ip_address),
          gfx::ImageSkia(), std::string(), FLAG_DISABLED));
    }
  }

  std::string ethernet_address =
      handler->FormattedHardwareAddressForType(NetworkTypePattern::Ethernet());
  if (!ethernet_address.empty()) {
    std::string label = l10n_util::GetStringUTF8(
        IDS_STATUSBAR_NETWORK_DEVICE_ETHERNET) + " " + ethernet_address;
    address_items.push_back(MenuItem(
        ui::MenuModel::TYPE_COMMAND, base::UTF8ToUTF16(label),
        gfx::ImageSkia(), std::string(), FLAG_DISABLED));
  }

  std::string wifi_address =
      handler->FormattedHardwareAddressForType(NetworkTypePattern::WiFi());
  if (!wifi_address.empty()) {
    std::string label = l10n_util::GetStringUTF8(
        IDS_STATUSBAR_NETWORK_DEVICE_WIFI) + " " + wifi_address;
    address_items.push_back(MenuItem(
        ui::MenuModel::TYPE_COMMAND, base::UTF8ToUTF16(label),
        gfx::ImageSkia(), std::string(), FLAG_DISABLED));
  }

  menu_items_ = link_items;
  if (!menu_items_.empty() && address_items.size() > 1)
    menu_items_.push_back(MenuItem());  // Separator
  menu_items_.insert(menu_items_.end(),
      address_items.begin(), address_items.end());
}

int MoreMenuModel::GetCommandIdAt(int index) const {
  return index + kMoreIndexMask;
}

////////////////////////////////////////////////////////////////////////////////
// NetworkMenu

NetworkMenu::Delegate::~Delegate() {
}

NetworkMenu::NetworkMenu(Delegate* delegate)
    : delegate_(delegate),
      refreshing_menu_(false),
      weak_pointer_factory_(this) {
  main_menu_model_.reset(new MainMenuModel(weak_pointer_factory_.GetWeakPtr()));
}

NetworkMenu::~NetworkMenu() {
}

ui::MenuModel* NetworkMenu::GetMenuModel() {
  return main_menu_model_.get();
}

void NetworkMenu::UpdateMenu() {
  refreshing_menu_ = true;
  main_menu_model_->InitMenuItems(delegate_->ShouldOpenButtonOptions());
  refreshing_menu_ = false;
}

}  // namespace chromeos
