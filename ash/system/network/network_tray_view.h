// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NETWORK_NETWORK_TRAY_VIEW_H_
#define ASH_SYSTEM_NETWORK_NETWORK_TRAY_VIEW_H_

#include "ash/system/network/network_icon_animation_observer.h"
#include "ash/system/tray/tray_item_view.h"
#include "base/macros.h"

namespace chromeos {
class NetworkState;
}  // namespace chromeos

namespace ash {
class TrayNetwork;

namespace tray {

// Returns the connected, non-virtual (aka VPN), network.
const chromeos::NetworkState* GetConnectedNetwork();

class NetworkTrayView : public TrayItemView,
                        public network_icon::AnimationObserver {
 public:
  explicit NetworkTrayView(TrayNetwork* network_tray);

  ~NetworkTrayView() override;

  const char* GetClassName() const override;

  void UpdateNetworkStateHandlerIcon();

  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // network_icon::AnimationObserver:
  void NetworkIconChanged() override;

  // Updates connection status and notifies accessibility event when necessary.
  void UpdateConnectionStatus(const chromeos::NetworkState* connected_network,
                              bool notify_a11y);

 private:
  void UpdateIcon(bool tray_icon_visible, const gfx::ImageSkia& image);

  base::string16 connection_status_string_;

  DISALLOW_COPY_AND_ASSIGN(NetworkTrayView);
};

}  // namespace tray
}  // namespace ash

#endif  // ASH_SYSTEM_NETWORK_NETWORK_TRAY_VIEW_H_
