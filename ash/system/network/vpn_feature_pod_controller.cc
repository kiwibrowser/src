// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/vpn_feature_pod_controller.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/network/network_icon.h"
#include "ash/system/network/tray_vpn.h"
#include "ash/system/network/vpn_list.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/unified/feature_pod_button.h"
#include "ash/system/unified/unified_system_tray_controller.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"

using chromeos::NetworkHandler;
using chromeos::NetworkState;
using chromeos::NetworkStateHandler;
using chromeos::NetworkTypePattern;

namespace ash {
namespace {

base::string16 GetNetworkStateHandlerLabel() {
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  const NetworkState* vpn =
      handler->FirstNetworkByType(NetworkTypePattern::VPN());
  if (!tray::IsVPNConnected())
    return l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_VPN_DISCONNECTED);
  return network_icon::GetLabelForNetwork(vpn,
                                          network_icon::ICON_TYPE_DEFAULT_VIEW);
}

}  // namespace

VPNFeaturePodController::VPNFeaturePodController(
    UnifiedSystemTrayController* tray_controller)
    : tray_controller_(tray_controller) {}

VPNFeaturePodController::~VPNFeaturePodController() = default;

FeaturePodButton* VPNFeaturePodController::CreateButton() {
  DCHECK(!button_);
  button_ = new FeaturePodButton(this);
  button_->SetVectorIcon(kNetworkVpnIcon);
  Update();
  return button_;
}

void VPNFeaturePodController::OnIconPressed() {
  tray_controller_->ShowVPNDetailedView();
}

void VPNFeaturePodController::Update() {
  button_->SetVisible(tray::IsVPNVisibleInSystemTray());
  if (!button_->visible())
    return;

  button_->SetLabel(GetNetworkStateHandlerLabel());
  button_->SetToggled(tray::IsVPNEnabled() && tray::IsVPNConnected());
}

}  // namespace ash
