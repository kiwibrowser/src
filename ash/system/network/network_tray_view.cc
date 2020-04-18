// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/network_tray_view.h"

#include "ash/strings/grit/ash_strings.h"
#include "ash/system/network/network_icon.h"
#include "ash/system/network/network_icon_animation.h"
#include "ash/system/network/tray_network.h"
#include "base/strings/utf_string_conversions.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/image_view.h"

using chromeos::NetworkHandler;
using chromeos::NetworkState;
using chromeos::NetworkStateHandler;
using chromeos::NetworkTypePattern;

namespace ash {
namespace tray {

const NetworkState* GetConnectedNetwork() {
  NetworkStateHandler* handler = NetworkHandler::Get()->network_state_handler();
  return handler->ConnectedNetworkByType(NetworkTypePattern::NonVirtual());
}

NetworkTrayView::NetworkTrayView(TrayNetwork* network_tray)
    : TrayItemView(network_tray) {
  CreateImageView();
  UpdateNetworkStateHandlerIcon();
  UpdateConnectionStatus(GetConnectedNetwork(), true /* notify_a11y */);
}

NetworkTrayView::~NetworkTrayView() {
  network_icon::NetworkIconAnimation::GetInstance()->RemoveObserver(this);
}

const char* NetworkTrayView::GetClassName() const {
  return "NetworkTrayView";
}

void NetworkTrayView::UpdateNetworkStateHandlerIcon() {
  gfx::ImageSkia image;
  base::string16 name;
  bool animating = false;
  network_icon::GetDefaultNetworkImageAndLabel(network_icon::ICON_TYPE_TRAY,
                                               &image, &name, &animating);
  bool show_in_tray = !image.isNull();
  UpdateIcon(show_in_tray, image);
  if (animating)
    network_icon::NetworkIconAnimation::GetInstance()->AddObserver(this);
  else
    network_icon::NetworkIconAnimation::GetInstance()->RemoveObserver(this);
}

void NetworkTrayView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->SetName(connection_status_string_);
  node_data->role = ax::mojom::Role::kButton;
}

void NetworkTrayView::NetworkIconChanged() {
  UpdateNetworkStateHandlerIcon();
  UpdateConnectionStatus(GetConnectedNetwork(), false /* notify_a11y */);
}

void NetworkTrayView::UpdateConnectionStatus(
    const NetworkState* connected_network,
    bool notify_a11y) {
  using SignalStrength = network_icon::SignalStrength;

  base::string16 new_connection_status_string;
  if (connected_network) {
    new_connection_status_string = l10n_util::GetStringFUTF16(
        IDS_ASH_STATUS_TRAY_NETWORK_CONNECTED,
        base::UTF8ToUTF16(connected_network->name()));

    // Retrieve the string describing the signal strength, if it is applicable
    // to |connected_network|.
    base::string16 signal_strength_string;
    switch (network_icon::GetSignalStrengthForNetwork(connected_network)) {
      case SignalStrength::NONE:
      case SignalStrength::NOT_WIRELESS:
        break;
      case SignalStrength::WEAK:
        signal_strength_string =
            l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_NETWORK_SIGNAL_WEAK);
        break;
      case SignalStrength::MEDIUM:
        signal_strength_string = l10n_util::GetStringUTF16(
            IDS_ASH_STATUS_TRAY_NETWORK_SIGNAL_MEDIUM);
        break;
      case SignalStrength::STRONG:
        signal_strength_string = l10n_util::GetStringUTF16(
            IDS_ASH_STATUS_TRAY_NETWORK_SIGNAL_STRONG);
        break;
    }

    if (!signal_strength_string.empty()) {
      new_connection_status_string = l10n_util::GetStringFUTF16(
          IDS_ASH_STATUS_TRAY_NETWORK_CONNECTED_ACCESSIBLE,
          base::UTF8ToUTF16(connected_network->name()), signal_strength_string);
    }
  }
  if (new_connection_status_string != connection_status_string_) {
    connection_status_string_ = new_connection_status_string;
    if (notify_a11y && !connection_status_string_.empty())
      NotifyAccessibilityEvent(ax::mojom::Event::kAlert, true);
  }
}

void NetworkTrayView::UpdateIcon(bool tray_icon_visible,
                                 const gfx::ImageSkia& image) {
  image_view()->SetImage(image);
  SetVisible(tray_icon_visible);
  SchedulePaint();
}

}  // namespace tray
}  // namespace ash
