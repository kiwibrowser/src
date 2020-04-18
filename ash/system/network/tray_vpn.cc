// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/tray_vpn.h"

#include "ash/metrics/user_metrics_recorder.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/network/network_icon.h"
#include "ash/system/network/network_icon_animation.h"
#include "ash/system/network/network_icon_animation_observer.h"
#include "ash/system/network/vpn_list.h"
#include "ash/system/network/vpn_list_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_item_detailed_view_delegate.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_item_more.h"
#include "ash/system/tray/tray_popup_item_style.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"

using chromeos::NetworkHandler;
using chromeos::NetworkState;
using chromeos::NetworkStateHandler;
using chromeos::NetworkTypePattern;

namespace ash {
namespace tray {

bool IsVPNVisibleInSystemTray() {
  LoginStatus login_status = Shell::Get()->session_controller()->login_status();
  if (login_status == LoginStatus::NOT_LOGGED_IN)
    return false;

  // Show the VPN entry in the ash tray bubble if at least one third-party VPN
  // provider is installed.
  if (Shell::Get()->vpn_list()->HaveThirdPartyOrArcVPNProviders())
    return true;

  // Also show the VPN entry if at least one VPN network is configured.
  NetworkStateHandler* const handler =
      NetworkHandler::Get()->network_state_handler();
  if (handler->FirstNetworkByType(NetworkTypePattern::VPN()))
    return true;
  return false;
}

bool IsVPNEnabled() {
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  return handler->FirstNetworkByType(NetworkTypePattern::VPN());
}

bool IsVPNConnected() {
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  const NetworkState* vpn =
      handler->FirstNetworkByType(NetworkTypePattern::VPN());
  return IsVPNEnabled() &&
         (vpn->IsConnectedState() || vpn->IsConnectingState());
}

class VpnDefaultView : public TrayItemMore,
                       public network_icon::AnimationObserver {
 public:
  explicit VpnDefaultView(SystemTrayItem* owner) : TrayItemMore(owner) {}

  ~VpnDefaultView() override {
    network_icon::NetworkIconAnimation::GetInstance()->RemoveObserver(this);
  }

  void Update() {
    gfx::ImageSkia image;
    base::string16 label;
    bool animating = false;
    GetNetworkStateHandlerImageAndLabel(&image, &label, &animating);
    if (animating)
      network_icon::NetworkIconAnimation::GetInstance()->AddObserver(this);
    else
      network_icon::NetworkIconAnimation::GetInstance()->RemoveObserver(this);
    SetImage(image);
    SetLabel(label);
    SetAccessibleName(label);
  }

  // network_icon::AnimationObserver
  void NetworkIconChanged() override { Update(); }

 protected:
  // TrayItemMore:
  std::unique_ptr<TrayPopupItemStyle> HandleCreateStyle() const override {
    std::unique_ptr<TrayPopupItemStyle> style =
        TrayItemMore::HandleCreateStyle();
    style->set_color_style(
        !IsVPNEnabled()
            ? TrayPopupItemStyle::ColorStyle::DISABLED
            : IsVPNConnected() ? TrayPopupItemStyle::ColorStyle::ACTIVE
                               : TrayPopupItemStyle::ColorStyle::INACTIVE);
    return style;
  }

  void UpdateStyle() override {
    TrayItemMore::UpdateStyle();
    Update();
  }

 private:
  void GetNetworkStateHandlerImageAndLabel(gfx::ImageSkia* image,
                                           base::string16* label,
                                           bool* animating) {
    NetworkStateHandler* handler =
        NetworkHandler::Get()->network_state_handler();
    const NetworkState* vpn =
        handler->FirstNetworkByType(NetworkTypePattern::VPN());
    *image = gfx::CreateVectorIcon(
        kNetworkVpnIcon, TrayPopupItemStyle::GetIconColor(
                             vpn && vpn->IsConnectedState()
                                 ? TrayPopupItemStyle::ColorStyle::ACTIVE
                                 : TrayPopupItemStyle::ColorStyle::INACTIVE));
    if (!IsVPNConnected()) {
      if (label) {
        *label =
            l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_VPN_DISCONNECTED);
      }
      *animating = false;
      return;
    }
    *animating = vpn->IsConnectingState();
    if (label) {
      *label = network_icon::GetLabelForNetwork(
          vpn, network_icon::ICON_TYPE_DEFAULT_VIEW);
    }
  }

  DISALLOW_COPY_AND_ASSIGN(VpnDefaultView);
};

}  // namespace tray

TrayVPN::TrayVPN(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_VPN),
      default_(nullptr),
      detailed_(nullptr),
      detailed_view_delegate_(
          std::make_unique<SystemTrayItemDetailedViewDelegate>(this)) {
  network_state_observer_.reset(new TrayNetworkStateObserver(this));
}

TrayVPN::~TrayVPN() = default;

views::View* TrayVPN::CreateDefaultView(LoginStatus status) {
  CHECK(default_ == nullptr);
  if (!chromeos::NetworkHandler::IsInitialized())
    return nullptr;
  if (!tray::IsVPNVisibleInSystemTray())
    return nullptr;

  const bool is_in_secondary_login_screen =
      Shell::Get()->session_controller()->IsInSecondaryLoginScreen();

  default_ = new tray::VpnDefaultView(this);
  default_->SetEnabled(status != LoginStatus::LOCKED &&
                       !is_in_secondary_login_screen);

  return default_;
}

views::View* TrayVPN::CreateDetailedView(LoginStatus status) {
  CHECK(detailed_ == nullptr);
  if (!chromeos::NetworkHandler::IsInitialized())
    return nullptr;

  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_DETAILED_VPN_VIEW);
  detailed_ = new tray::VPNListView(detailed_view_delegate_.get(), status);
  detailed_->Init();
  return detailed_;
}

void TrayVPN::OnDefaultViewDestroyed() {
  default_ = nullptr;
}

void TrayVPN::OnDetailedViewDestroyed() {
  detailed_ = nullptr;
}

void TrayVPN::NetworkStateChanged(bool /* notify_a11y */) {
  if (default_)
    default_->Update();
  if (detailed_)
    detailed_->Update();
}

}  // namespace ash
