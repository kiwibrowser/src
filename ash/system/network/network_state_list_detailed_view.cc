// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/network_state_list_detailed_view.h"

#include <algorithm>

#include "ash/metrics/user_metrics_recorder.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/system_menu_button.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/system/tray/tri_view.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chromeos/network/device_state.h"
#include "chromeos/network/network_connect.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/widget/widget.h"

using chromeos::DeviceState;
using chromeos::NetworkHandler;
using chromeos::NetworkState;
using chromeos::NetworkStateHandler;
using chromeos::NetworkTypePattern;

namespace ash {
namespace tray {
namespace {

// Delay between scan requests.
constexpr int kRequestScanDelaySeconds = 10;

// This margin value is used throughout the bubble:
// - margins inside the border
// - horizontal spacing between bubble border and parent bubble border
// - distance between top of this bubble's border and the bottom of the anchor
//   view (horizontal rule).
constexpr int kBubbleMargin = 8;

// Elevation used for the bubble shadow effect (tiny).
constexpr int kBubbleShadowElevation = 2;

}  // namespace

// A bubble which displays network info.
class NetworkStateListDetailedView::InfoBubble
    : public views::BubbleDialogDelegateView {
 public:
  InfoBubble(views::View* anchor,
             views::View* content,
             NetworkStateListDetailedView* detailed_view)
      : views::BubbleDialogDelegateView(anchor, views::BubbleBorder::TOP_RIGHT),
        detailed_view_(detailed_view) {
    set_margins(gfx::Insets(kBubbleMargin));
    set_arrow(views::BubbleBorder::NONE);
    set_shadow(views::BubbleBorder::NO_ASSETS);
    set_anchor_view_insets(gfx::Insets(0, 0, kBubbleMargin, 0));
    set_notify_enter_exit_on_child(true);
    SetLayoutManager(std::make_unique<views::FillLayout>());
    AddChildView(content);
  }

  ~InfoBubble() override {
    // The detailed view can be destructed before info bubble is destructed.
    // Call OnInfoBubbleDestroyed only if the detailed view is live.
    if (detailed_view_)
      detailed_view_->OnInfoBubbleDestroyed();
  }

  void OnNetworkStateListDetailedViewIsDeleting() { detailed_view_ = nullptr; }

 private:
  // View:
  gfx::Size CalculatePreferredSize() const override {
    // This bubble should be inset by kBubbleMargin on both left and right
    // relative to the parent bubble.
    const gfx::Size anchor_size = GetAnchorView()->size();
    int contents_width =
        anchor_size.width() - 2 * kBubbleMargin - margins().width();
    return gfx::Size(contents_width, GetHeightForWidth(contents_width));
  }

  void OnMouseExited(const ui::MouseEvent& event) override {
    // Like the user switching bubble/menu, hide the bubble when the mouse
    // exits.
    if (detailed_view_)
      detailed_view_->ResetInfoBubble();
  }

  // BubbleDialogDelegateView:
  int GetDialogButtons() const override { return ui::DIALOG_BUTTON_NONE; }

  void OnBeforeBubbleWidgetInit(views::Widget::InitParams* params,
                                views::Widget* widget) const override {
    params->shadow_type = views::Widget::InitParams::SHADOW_TYPE_DROP;
    params->shadow_elevation = kBubbleShadowElevation;
    params->name = "NetworkStateListDetailedView::InfoBubble";
  }

  // Not owned.
  NetworkStateListDetailedView* detailed_view_;

  DISALLOW_COPY_AND_ASSIGN(InfoBubble);
};

//------------------------------------------------------------------------------

// Special layout to overlap the scanning throbber and the info button.
class InfoThrobberLayout : public views::LayoutManager {
 public:
  InfoThrobberLayout() = default;
  ~InfoThrobberLayout() override = default;

  // views::LayoutManager
  void Layout(views::View* host) override {
    gfx::Size max_size(GetMaxChildSize(host));
    // Center each child view within |max_size|.
    for (int i = 0; i < host->child_count(); ++i) {
      views::View* child = host->child_at(i);
      if (!child->visible())
        continue;
      gfx::Size child_size = child->GetPreferredSize();
      gfx::Point origin;
      origin.set_x((max_size.width() - child_size.width()) / 2);
      origin.set_y((max_size.height() - child_size.height()) / 2);
      gfx::Rect bounds(origin, child_size);
      bounds.Inset(-host->GetInsets());
      child->SetBoundsRect(bounds);
    }
  }

  gfx::Size GetPreferredSize(const views::View* host) const override {
    gfx::Point origin;
    gfx::Rect rect(origin, GetMaxChildSize(host));
    rect.Inset(-host->GetInsets());
    return rect.size();
  }

 private:
  gfx::Size GetMaxChildSize(const views::View* host) const {
    int width = 0, height = 0;
    for (int i = 0; i < host->child_count(); ++i) {
      const views::View* child = host->child_at(i);
      if (!child->visible())
        continue;
      gfx::Size child_size = child->GetPreferredSize();
      width = std::max(width, child_size.width());
      height = std::max(height, child_size.width());
    }
    return gfx::Size(width, height);
  }

  DISALLOW_COPY_AND_ASSIGN(InfoThrobberLayout);
};

//------------------------------------------------------------------------------
// NetworkStateListDetailedView

NetworkStateListDetailedView::NetworkStateListDetailedView(
    DetailedViewDelegate* delegate,
    ListType list_type,
    LoginStatus login)
    : TrayDetailedView(delegate),
      list_type_(list_type),
      login_(login),
      info_button_(nullptr),
      settings_button_(nullptr),
      info_bubble_(nullptr) {}

NetworkStateListDetailedView::~NetworkStateListDetailedView() {
  if (info_bubble_)
    info_bubble_->OnNetworkStateListDetailedViewIsDeleting();
  ResetInfoBubble();
}

void NetworkStateListDetailedView::Update() {
  UpdateNetworkList();
  UpdateHeaderButtons();
  Layout();
}

void NetworkStateListDetailedView::ToggleInfoBubbleForTesting() {
  ToggleInfoBubble();
}

void NetworkStateListDetailedView::Init() {
  CreateScrollableList();
  CreateTitleRow(list_type_ == ListType::LIST_TYPE_NETWORK
                     ? IDS_ASH_STATUS_TRAY_NETWORK
                     : IDS_ASH_STATUS_TRAY_VPN);

  Update();

  if (list_type_ == LIST_TYPE_NETWORK)
    CallRequestScan();
}

void NetworkStateListDetailedView::HandleButtonPressed(views::Button* sender,
                                                       const ui::Event& event) {
  if (sender == info_button_) {
    ToggleInfoBubble();
    return;
  }

  if (sender == settings_button_)
    ShowSettings();

  CloseBubble();
}

void NetworkStateListDetailedView::HandleViewClicked(views::View* view) {
  if (login_ == LoginStatus::LOCKED)
    return;

  std::string guid;
  if (!IsNetworkEntry(view, &guid))
    return;

  const NetworkState* network =
      NetworkHandler::Get()->network_state_handler()->GetNetworkStateFromGuid(
          guid);
  // TODO(stevenjb): Test network->connectable() here instead of
  // IsDefaultCellular once network configuration is integrated into Settings.
  // crbug.com/380937.
  if (!network || network->IsConnectingOrConnected() ||
      network->IsDefaultCellular()) {
    Shell::Get()->metrics()->RecordUserMetricsAction(
        list_type_ == LIST_TYPE_VPN
            ? UMA_STATUS_AREA_SHOW_VPN_CONNECTION_DETAILS
            : UMA_STATUS_AREA_SHOW_NETWORK_CONNECTION_DETAILS);
    Shell::Get()->system_tray_controller()->ShowNetworkSettings(
        network ? network->guid() : std::string());
  } else {
    Shell::Get()->metrics()->RecordUserMetricsAction(
        list_type_ == LIST_TYPE_VPN
            ? UMA_STATUS_AREA_CONNECT_TO_VPN
            : UMA_STATUS_AREA_CONNECT_TO_CONFIGURED_NETWORK);
    chromeos::NetworkConnect::Get()->ConnectToNetworkId(network->guid());
  }
}

void NetworkStateListDetailedView::CreateExtraTitleRowButtons() {
  if (login_ == LoginStatus::LOCKED)
    return;

  DCHECK(!info_button_);
  tri_view()->SetContainerVisible(TriView::Container::END, true);

  info_button_ = new SystemMenuButton(this, kSystemMenuInfoIcon,
                                      IDS_ASH_STATUS_TRAY_NETWORK_INFO);
  tri_view()->AddView(TriView::Container::END, info_button_);

  DCHECK(!settings_button_);
  settings_button_ = new SystemMenuButton(this, kSystemMenuSettingsIcon,
                                          IDS_ASH_STATUS_TRAY_NETWORK_SETTINGS);
  tri_view()->AddView(TriView::Container::END, settings_button_);
}

void NetworkStateListDetailedView::ShowSettings() {
  Shell::Get()->metrics()->RecordUserMetricsAction(
      list_type_ == LIST_TYPE_VPN ? UMA_STATUS_AREA_VPN_SETTINGS_OPENED
                                  : UMA_STATUS_AREA_NETWORK_SETTINGS_OPENED);
  Shell::Get()->system_tray_controller()->ShowNetworkSettings(std::string());
}

void NetworkStateListDetailedView::UpdateHeaderButtons() {
  if (settings_button_) {
    if (login_ == LoginStatus::NOT_LOGGED_IN) {
      // When not logged in, only enable the settings button if there is a
      // default (i.e. connected or connecting) network to show settings for.
      settings_button_->SetEnabled(
          !!NetworkHandler::Get()->network_state_handler()->DefaultNetwork());
    } else {
      // Otherwise, enable if showing settings is allowed. There are situations
      // (supervised user creation flow) when the session is started but UI flow
      // continues within login UI, i.e., no browser window is yet available.
      settings_button_->SetEnabled(
          Shell::Get()->session_controller()->ShouldEnableSettings());
    }
  }
  if (list_type_ == LIST_TYPE_NETWORK) {
    NetworkStateHandler* network_state_handler =
        NetworkHandler::Get()->network_state_handler();
    const bool scanning = network_state_handler->GetScanningByType(
        NetworkTypePattern::WiFi() | NetworkTypePattern::Tether());
    ShowProgress(-1, scanning);
  }
}

void NetworkStateListDetailedView::ToggleInfoBubble() {
  if (ResetInfoBubble())
    return;

  info_bubble_ = new InfoBubble(tri_view(), CreateNetworkInfoView(), this);
  views::BubbleDialogDelegateView::CreateBubble(info_bubble_)->Show();
  info_bubble_->NotifyAccessibilityEvent(ax::mojom::Event::kAlert, false);
}

bool NetworkStateListDetailedView::ResetInfoBubble() {
  if (!info_bubble_)
    return false;

  info_bubble_->GetWidget()->Close();
  return true;
}

void NetworkStateListDetailedView::OnInfoBubbleDestroyed() {
  info_bubble_ = nullptr;

  // Widget of info bubble is activated while info bubble is shown. To move
  // focus back to the widget of this view, activate it again here.
  GetWidget()->Activate();
}

views::View* NetworkStateListDetailedView::CreateNetworkInfoView() {
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();

  std::string ip_address, ipv6_address;
  const NetworkState* network = handler->DefaultNetwork();
  if (network) {
    ip_address = network->GetIpAddress();
    const DeviceState* device = handler->GetDeviceState(network->device_path());
    if (device)
      ipv6_address = device->GetIpAddressByType(shill::kTypeIPv6);
  }

  std::string ethernet_address, wifi_address;
  if (list_type_ == LIST_TYPE_NETWORK) {
    ethernet_address = handler->FormattedHardwareAddressForType(
        NetworkTypePattern::Ethernet());
    wifi_address =
        handler->FormattedHardwareAddressForType(NetworkTypePattern::WiFi());
  }

  base::string16 bubble_text;
  auto add_line = [&bubble_text](const std::string& address, int ids) {
    if (!address.empty()) {
      if (!bubble_text.empty())
        bubble_text += base::ASCIIToUTF16("\n");

      bubble_text +=
          l10n_util::GetStringFUTF16(ids, base::UTF8ToUTF16(address));
    }
  };

  add_line(ip_address, IDS_ASH_STATUS_TRAY_IP_ADDRESS);
  add_line(ipv6_address, IDS_ASH_STATUS_TRAY_IPV6_ADDRESS);
  add_line(ethernet_address, IDS_ASH_STATUS_TRAY_ETHERNET_ADDRESS);
  add_line(wifi_address, IDS_ASH_STATUS_TRAY_WIFI_ADDRESS);

  // Avoid an empty bubble in the unlikely event that there is no network
  // information at all.
  if (bubble_text.empty())
    bubble_text = l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_NO_NETWORKS);

  auto* label = new views::Label(bubble_text);
  label->SetMultiLine(true);
  label->SetHorizontalAlignment(gfx::ALIGN_TO_HEAD);
  label->SetSelectable(true);
  return label;
}

void NetworkStateListDetailedView::CallRequestScan() {
  VLOG(1) << "Requesting Network Scan.";
  NetworkHandler::Get()->network_state_handler()->RequestScan(
      NetworkTypePattern::WiFi());
  // Periodically request a scan while this UI is open.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&NetworkStateListDetailedView::CallRequestScan, AsWeakPtr()),
      base::TimeDelta::FromSeconds(kRequestScanDelaySeconds));
}

}  // namespace tray
}  // namespace ash
