// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/bluetooth/tray_bluetooth.h"

#include <map>
#include <memory>
#include <set>
#include <string>

#include "ash/ash_view_ids.h"
#include "ash/metrics/user_metrics_recorder.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/bluetooth/tray_bluetooth_helper.h"
#include "ash/system/tray/hover_highlight_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/system/tray/system_tray_item_detailed_view_delegate.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_detailed_view.h"
#include "ash/system/tray/tray_info_label.h"
#include "ash/system/tray/tray_item_more.h"
#include "ash/system/tray/tray_popup_item_style.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ash/system/tray/tri_view.h"
#include "device/bluetooth/bluetooth_common.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"

namespace ash {
namespace tray {
namespace {

const int kUpdateFrequencyMs = 1000;

// Updates bluetooth device |device| in the |list|. If it is new, append to the
// end of the |list|; otherwise, keep it at the same place, but update the data
// with new device info provided by |device|.
void UpdateBluetoothDeviceListHelper(BluetoothDeviceList* list,
                                     const BluetoothDeviceInfo& device) {
  for (BluetoothDeviceList::iterator it = list->begin(); it != list->end();
       ++it) {
    if ((*it).address == device.address) {
      *it = device;
      return;
    }
  }

  list->push_back(device);
}

// Removes the obsolete BluetoothDevices from |list|, if they are not in the
// |new_list|.
void RemoveObsoleteBluetoothDevicesFromList(
    BluetoothDeviceList* list,
    const std::set<std::string>& new_list) {
  for (BluetoothDeviceList::iterator it = list->begin(); it != list->end();
       ++it) {
    if (new_list.find((*it).address) == new_list.end()) {
      it = list->erase(it);
      if (it == list->end())
        return;
    }
  }
}

// Returns corresponding device type icons for given Bluetooth device types and
// connection states.
const gfx::VectorIcon& GetBluetoothDeviceIcon(
    device::BluetoothDeviceType device_type,
    bool connected) {
  switch (device_type) {
    case device::BluetoothDeviceType::COMPUTER:
      return ash::kSystemMenuComputerIcon;
    case device::BluetoothDeviceType::PHONE:
      return ash::kSystemMenuPhoneIcon;
    case device::BluetoothDeviceType::AUDIO:
    case device::BluetoothDeviceType::CAR_AUDIO:
      return ash::kSystemMenuHeadsetIcon;
    case device::BluetoothDeviceType::VIDEO:
      return ash::kSystemMenuVideocamIcon;
    case device::BluetoothDeviceType::JOYSTICK:
    case device::BluetoothDeviceType::GAMEPAD:
      return ash::kSystemMenuGamepadIcon;
    case device::BluetoothDeviceType::KEYBOARD:
    case device::BluetoothDeviceType::KEYBOARD_MOUSE_COMBO:
      return ash::kSystemMenuKeyboardIcon;
    case device::BluetoothDeviceType::TABLET:
      return ash::kSystemMenuTabletIcon;
    case device::BluetoothDeviceType::MOUSE:
      return ash::kSystemMenuMouseIcon;
    case device::BluetoothDeviceType::MODEM:
    case device::BluetoothDeviceType::PERIPHERAL:
      return ash::kSystemMenuBluetoothIcon;
    default:
      return connected ? ash::kSystemMenuBluetoothConnectedIcon
                       : ash::kSystemMenuBluetoothIcon;
  }
}

const int kDisabledPanelLabelBaselineY = 20;

}  // namespace

class BluetoothDefaultView : public TrayItemMore {
 public:
  explicit BluetoothDefaultView(SystemTrayItem* owner) : TrayItemMore(owner) {
    set_id(VIEW_ID_BLUETOOTH_DEFAULT_VIEW);
  }
  ~BluetoothDefaultView() override = default;

  void Update() {
    TrayBluetoothHelper* helper = Shell::Get()->tray_bluetooth_helper();
    if (helper->GetBluetoothAvailable()) {
      const base::string16 label = l10n_util::GetStringUTF16(
          helper->GetBluetoothEnabled()
              ? IDS_ASH_STATUS_TRAY_BLUETOOTH_ENABLED
              : IDS_ASH_STATUS_TRAY_BLUETOOTH_DISABLED);
      SetLabel(label);
      SetAccessibleName(label);
      SetVisible(true);
    } else {
      SetVisible(false);
    }
    UpdateStyle();
  }

 protected:
  // TrayItemMore:
  std::unique_ptr<TrayPopupItemStyle> HandleCreateStyle() const override {
    TrayBluetoothHelper* helper = Shell::Get()->tray_bluetooth_helper();
    std::unique_ptr<TrayPopupItemStyle> style =
        TrayItemMore::HandleCreateStyle();
    style->set_color_style(
        helper->GetBluetoothEnabled()
            ? TrayPopupItemStyle::ColorStyle::ACTIVE
            : helper->GetBluetoothAvailable()
                  ? TrayPopupItemStyle::ColorStyle::INACTIVE
                  : TrayPopupItemStyle::ColorStyle::DISABLED);

    return style;
  }

  void UpdateStyle() override {
    TrayItemMore::UpdateStyle();
    std::unique_ptr<TrayPopupItemStyle> style = CreateStyle();
    SetImage(gfx::CreateVectorIcon(GetCurrentIcon(), style->GetIconColor()));
  }

 private:
  const gfx::VectorIcon& GetCurrentIcon() {
    TrayBluetoothHelper* helper = Shell::Get()->tray_bluetooth_helper();
    if (!helper->GetBluetoothEnabled())
      return kSystemMenuBluetoothDisabledIcon;

    bool has_connected_device = false;
    BluetoothDeviceList list = helper->GetAvailableBluetoothDevices();
    for (const auto& device : list) {
      if (device.connected) {
        has_connected_device = true;
        break;
      }
    }
    return has_connected_device ? kSystemMenuBluetoothConnectedIcon
                                : kSystemMenuBluetoothIcon;
  }

  DISALLOW_COPY_AND_ASSIGN(BluetoothDefaultView);
};

class BluetoothDetailedView : public TrayDetailedView {
 public:
  BluetoothDetailedView(DetailedViewDelegate* delegate, LoginStatus login)
      : TrayDetailedView(delegate),
        login_(login),
        toggle_(nullptr),
        settings_(nullptr),
        disabled_panel_(nullptr) {
    CreateItems();
  }

  ~BluetoothDetailedView() override {
    // Stop discovering bluetooth devices when exiting BT detailed view.
    BluetoothStopDiscovering();
  }

  void Update() {
    // Update immediately for initial device list and
    // when bluetooth is disabled.
    if (device_map_.size() == 0 ||
        !Shell::Get()->tray_bluetooth_helper()->GetBluetoothEnabled()) {
      DoUpdate();
      return;
    }

    // Return here since an update is already queued.
    if (timer_.IsRunning())
      return;

    // Update the detailed view after kUpdateFrequencyMs.
    timer_.Start(FROM_HERE,
                 base::TimeDelta::FromMilliseconds(kUpdateFrequencyMs), this,
                 &BluetoothDetailedView::DoUpdate);
  }

 private:
  void CreateItems() {
    CreateScrollableList();
    CreateTitleRow(IDS_ASH_STATUS_TRAY_BLUETOOTH);
  }

  void BluetoothStartDiscovering() {
    TrayBluetoothHelper* helper = Shell::Get()->tray_bluetooth_helper();
    if (helper->HasBluetoothDiscoverySession()) {
      ShowLoadingIndicator();
      return;
    }
    HideLoadingIndicator();
    if (helper->GetBluetoothEnabled())
      helper->StartBluetoothDiscovering();
  }

  void BluetoothStopDiscovering() {
    TrayBluetoothHelper* helper = Shell::Get()->tray_bluetooth_helper();
    if (helper && helper->HasBluetoothDiscoverySession()) {
      helper->StopBluetoothDiscovering();
      HideLoadingIndicator();
    }
  }

  void UpdateBluetoothDeviceList() {
    std::set<std::string> new_connecting_devices;
    std::set<std::string> new_connected_devices;
    std::set<std::string> new_paired_not_connected_devices;
    std::set<std::string> new_discovered_not_paired_devices;

    BluetoothDeviceList list =
        Shell::Get()->tray_bluetooth_helper()->GetAvailableBluetoothDevices();
    for (const auto& device : list) {
      if (device.connecting) {
        new_connecting_devices.insert(device.address);
        UpdateBluetoothDeviceListHelper(&connecting_devices_, device);
      } else if (device.connected && device.paired) {
        new_connected_devices.insert(device.address);
        UpdateBluetoothDeviceListHelper(&connected_devices_, device);
      } else if (device.paired) {
        new_paired_not_connected_devices.insert(device.address);
        UpdateBluetoothDeviceListHelper(&paired_not_connected_devices_, device);
      } else {
        new_discovered_not_paired_devices.insert(device.address);
        UpdateBluetoothDeviceListHelper(&discovered_not_paired_devices_,
                                        device);
      }
    }
    RemoveObsoleteBluetoothDevicesFromList(&connecting_devices_,
                                           new_connecting_devices);
    RemoveObsoleteBluetoothDevicesFromList(&connected_devices_,
                                           new_connected_devices);
    RemoveObsoleteBluetoothDevicesFromList(&paired_not_connected_devices_,
                                           new_paired_not_connected_devices);
    RemoveObsoleteBluetoothDevicesFromList(&discovered_not_paired_devices_,
                                           new_discovered_not_paired_devices);
  }

  void UpdateHeaderEntry() {
    const bool is_bluetooth_enabled =
        Shell::Get()->tray_bluetooth_helper()->GetBluetoothEnabled();
    if (toggle_)
      toggle_->SetIsOn(is_bluetooth_enabled, true);
  }

  void UpdateDeviceScrollList() {
    std::string focused_device_address = GetFocusedDeviceAddress();

    device_map_.clear();
    scroll_content()->RemoveAllChildViews(true);

    TrayBluetoothHelper* helper = Shell::Get()->tray_bluetooth_helper();
    const bool bluetooth_enabled = helper->GetBluetoothEnabled();
    const bool bluetooth_available = helper->GetBluetoothAvailable();

    // If Bluetooth is disabled, show a panel which only indicates that it is
    // disabled, instead of the scroller with Bluetooth devices.
    if (bluetooth_enabled) {
      HideDisabledPanel();
    } else {
      ShowDisabledPanel();
      return;
    }

    // Add paired devices and their section header to the list.
    size_t num_paired_devices = connected_devices_.size() +
                                connecting_devices_.size() +
                                paired_not_connected_devices_.size();
    if (num_paired_devices > 0) {
      AddScrollListSubHeader(IDS_ASH_STATUS_TRAY_BLUETOOTH_PAIRED_DEVICES);
      AppendSameTypeDevicesToScrollList(connected_devices_, true, true,
                                        bluetooth_enabled);
      AppendSameTypeDevicesToScrollList(connecting_devices_, true, false,
                                        bluetooth_enabled);
      AppendSameTypeDevicesToScrollList(paired_not_connected_devices_, false,
                                        false, bluetooth_enabled);
    }

    // Add unpaired devices to the list. If at least one paired device is
    // present, also add a section header above the unpaired devices.
    if (discovered_not_paired_devices_.size() > 0) {
      if (num_paired_devices > 0)
        AddScrollListSubHeader(IDS_ASH_STATUS_TRAY_BLUETOOTH_UNPAIRED_DEVICES);
      AppendSameTypeDevicesToScrollList(discovered_not_paired_devices_, false,
                                        false, bluetooth_enabled);
    }

    // Show user Bluetooth state if there is no bluetooth devices in list.
    if (device_map_.size() == 0 && bluetooth_available && bluetooth_enabled) {
      scroll_content()->AddChildView(new TrayInfoLabel(
          nullptr /* delegate */, IDS_ASH_STATUS_TRAY_BLUETOOTH_DISCOVERING));
    }

    // Focus the device which was focused before the device-list update.
    if (!focused_device_address.empty())
      FocusDeviceByAddress(focused_device_address);

    scroll_content()->InvalidateLayout();
  }

  void AppendSameTypeDevicesToScrollList(const BluetoothDeviceList& list,
                                         bool highlight,
                                         bool checked,
                                         bool enabled) {
    for (const auto& device : list) {
      const gfx::VectorIcon& icon =
          GetBluetoothDeviceIcon(device.device_type, device.connected);
      HoverHighlightView* container =
          AddScrollListItem(icon, device.display_name);
      if (device.connected)
        SetupConnectedScrollListItem(container);
      else if (device.connecting)
        SetupConnectingScrollListItem(container);
      device_map_[container] = device.address;
    }
  }

  // Returns true if the device with |device_id| is found in |device_list|.
  bool FoundDevice(const std::string& device_id,
                   const BluetoothDeviceList& device_list) {
    for (const auto& device : device_list) {
      if (device.address == device_id)
        return true;
    }
    return false;
  }

  // Updates UI of the clicked bluetooth device to show it is being connected
  // or disconnected if such an operation is going to be performed underway.
  void UpdateClickedDevice(const std::string& device_id,
                           views::View* item_container) {
    if (FoundDevice(device_id, paired_not_connected_devices_)) {
      HoverHighlightView* container =
          static_cast<HoverHighlightView*>(item_container);
      SetupConnectingScrollListItem(container);
      scroll_content()->SizeToPreferredSize();
      scroller()->Layout();
    }
  }

  // TrayDetailedView:
  void HandleViewClicked(views::View* view) override {
    TrayBluetoothHelper* helper = Shell::Get()->tray_bluetooth_helper();
    if (!helper->GetBluetoothEnabled())
      return;

    std::map<views::View*, std::string>::iterator find;
    find = device_map_.find(view);
    if (find == device_map_.end())
      return;

    const std::string device_id = find->second;
    if (FoundDevice(device_id, connecting_devices_))
      return;

    UpdateClickedDevice(device_id, view);
    helper->ConnectToBluetoothDevice(device_id);
  }

  void HandleButtonPressed(views::Button* sender,
                           const ui::Event& event) override {
    if (sender == toggle_) {
      Shell::Get()->tray_bluetooth_helper()->SetBluetoothEnabled(
          toggle_->is_on());
    } else if (sender == settings_) {
      ShowSettings();
    } else {
      NOTREACHED();
    }
  }

  void CreateExtraTitleRowButtons() override {
    if (login_ == LoginStatus::LOCKED)
      return;

    DCHECK(!toggle_);
    DCHECK(!settings_);

    tri_view()->SetContainerVisible(TriView::Container::END, true);

    toggle_ =
        TrayPopupUtils::CreateToggleButton(this, IDS_ASH_STATUS_TRAY_BLUETOOTH);
    tri_view()->AddView(TriView::Container::END, toggle_);

    settings_ = CreateSettingsButton(IDS_ASH_STATUS_TRAY_BLUETOOTH_SETTINGS);
    tri_view()->AddView(TriView::Container::END, settings_);
  }

  void ShowSettings() {
    if (TrayPopupUtils::CanOpenWebUISettings()) {
      Shell::Get()->system_tray_controller()->ShowBluetoothSettings();
      CloseBubble();
    }
  }

  void ShowLoadingIndicator() {
    // Setting a value of -1 gives progress_bar an infinite-loading behavior.
    ShowProgress(-1, true);
  }

  void HideLoadingIndicator() { ShowProgress(0, false); }

  void ShowDisabledPanel() {
    DCHECK(scroller());
    if (!disabled_panel_) {
      disabled_panel_ = CreateDisabledPanel();
      // Insert |disabled_panel_| before the scroller, since the scroller will
      // have unnecessary bottom border when it is not the last child.
      AddChildViewAt(disabled_panel_, GetIndexOf(scroller()));
      // |disabled_panel_| need to fill the remaining space below the title row
      // so that the inner contents of |disabled_panel_| are placed properly.
      box_layout()->SetFlexForView(disabled_panel_, 1);
    }
    disabled_panel_->SetVisible(true);
    scroller()->SetVisible(false);
  }

  void HideDisabledPanel() {
    DCHECK(scroller());
    if (disabled_panel_)
      disabled_panel_->SetVisible(false);
    scroller()->SetVisible(true);
  }

  views::View* CreateDisabledPanel() {
    views::View* container = new views::View;
    auto box_layout =
        std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical);
    box_layout->set_main_axis_alignment(
        views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
    container->SetLayoutManager(std::move(box_layout));

    TrayPopupItemStyle style(
        TrayPopupItemStyle::FontStyle::DETAILED_VIEW_LABEL);
    style.set_color_style(TrayPopupItemStyle::ColorStyle::DISABLED);

    views::ImageView* image_view = new views::ImageView;
    image_view->SetImage(gfx::CreateVectorIcon(kSystemMenuBluetoothDisabledIcon,
                                               style.GetIconColor()));
    image_view->SetVerticalAlignment(views::ImageView::TRAILING);
    container->AddChildView(image_view);

    views::Label* label = new views::Label(
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_BLUETOOTH_DISABLED));
    style.SetupLabel(label);
    label->SetBorder(views::CreateEmptyBorder(
        kDisabledPanelLabelBaselineY - label->GetBaseline(), 0, 0, 0));
    container->AddChildView(label);

    // Make top padding of the icon equal to the height of the label so that the
    // icon is vertically aligned to center of the container.
    image_view->SetBorder(
        views::CreateEmptyBorder(label->GetPreferredSize().height(), 0, 0, 0));
    return container;
  }

  std::string GetFocusedDeviceAddress() {
    for (auto& view_and_address : device_map_) {
      if (view_and_address.first->HasFocus())
        return view_and_address.second;
    }
    return std::string();
  }

  void FocusDeviceByAddress(const std::string& address) {
    for (auto& view_and_address : device_map_) {
      if (view_and_address.second == address) {
        view_and_address.first->RequestFocus();
        return;
      }
    }
  }

  void DoUpdate() {
    BluetoothStartDiscovering();
    UpdateBluetoothDeviceList();

    // Update UI.
    UpdateDeviceScrollList();
    UpdateHeaderEntry();
    Layout();
  }

  // TODO(jamescook): Don't cache this.
  LoginStatus login_;

  std::map<views::View*, std::string> device_map_;

  BluetoothDeviceList connected_devices_;
  BluetoothDeviceList connecting_devices_;
  BluetoothDeviceList paired_not_connected_devices_;
  BluetoothDeviceList discovered_not_paired_devices_;

  views::ToggleButton* toggle_;
  views::Button* settings_;

  // The container of the message "Bluetooth is disabled" and an icon. It should
  // be shown instead of Bluetooth device list when Bluetooth is disabled.
  views::View* disabled_panel_;

  // Timer used to limit the update frequency.
  base::OneShotTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothDetailedView);
};

}  // namespace tray

TrayBluetooth::TrayBluetooth(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_BLUETOOTH),
      default_(nullptr),
      detailed_(nullptr),
      detailed_view_delegate_(
          std::make_unique<SystemTrayItemDetailedViewDelegate>(this)) {
  Shell::Get()->system_tray_notifier()->AddBluetoothObserver(this);
}

TrayBluetooth::~TrayBluetooth() {
  Shell::Get()->system_tray_notifier()->RemoveBluetoothObserver(this);
}

views::View* TrayBluetooth::CreateDefaultView(LoginStatus status) {
  CHECK(default_ == nullptr);
  SessionController* session_controller = Shell::Get()->session_controller();
  default_ = new tray::BluetoothDefaultView(this);
  if (!session_controller->IsActiveUserSessionStarted()) {
    // Bluetooth power setting is always mutable in login screen before any
    // user logs in. The changes will affect local state preferences.
    default_->SetEnabled(true);
  } else {
    // The bluetooth setting should be mutable only if:
    // * the active user is the primary user, and
    // * the session is not in lock screen
    // The changes will affect the primary user's preferences.
    default_->SetEnabled(session_controller->IsUserPrimary() &&
                         status != LoginStatus::LOCKED);
  }
  default_->Update();
  return default_;
}

views::View* TrayBluetooth::CreateDetailedView(LoginStatus status) {
  if (!Shell::Get()->tray_bluetooth_helper()->GetBluetoothAvailable())
    return nullptr;
  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_DETAILED_BLUETOOTH_VIEW);
  CHECK(detailed_ == nullptr);
  detailed_ =
      new tray::BluetoothDetailedView(detailed_view_delegate_.get(), status);
  detailed_->Update();
  return detailed_;
}

void TrayBluetooth::OnDefaultViewDestroyed() {
  default_ = nullptr;
}

void TrayBluetooth::OnDetailedViewDestroyed() {
  detailed_ = nullptr;
}

void TrayBluetooth::UpdateAfterLoginStatusChange(LoginStatus status) {}

void TrayBluetooth::OnBluetoothRefresh() {
  if (default_)
    default_->Update();
  else if (detailed_)
    detailed_->Update();
}

void TrayBluetooth::OnBluetoothDiscoveringChanged() {
  if (!detailed_)
    return;
  detailed_->Update();
}

}  // namespace ash
