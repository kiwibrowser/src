// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NETWORK_TRAY_VPN_H_
#define ASH_SYSTEM_NETWORK_TRAY_VPN_H_

#include <memory>

#include "ash/system/network/tray_network_state_observer.h"
#include "ash/system/tray/system_tray_item.h"
#include "base/macros.h"

namespace ash {
class TrayNetworkStateObserver;
class DetailedViewDelegate;

namespace tray {
class VPNListView;
class VpnDefaultView;

extern bool IsVPNVisibleInSystemTray();
extern bool IsVPNEnabled();
extern bool IsVPNConnected();

}  // namespace tray

class TrayVPN : public SystemTrayItem,
                public TrayNetworkStateObserver::Delegate {
 public:
  explicit TrayVPN(SystemTray* system_tray);
  ~TrayVPN() override;

  // SystemTrayItem
  views::View* CreateDefaultView(LoginStatus status) override;
  views::View* CreateDetailedView(LoginStatus status) override;
  void OnDefaultViewDestroyed() override;
  void OnDetailedViewDestroyed() override;

  // TrayNetworkStateObserver::Delegate
  void NetworkStateChanged(bool notify_a11y) override;

 private:
  tray::VpnDefaultView* default_;
  tray::VPNListView* detailed_;
  std::unique_ptr<TrayNetworkStateObserver> network_state_observer_;

  const std::unique_ptr<DetailedViewDelegate> detailed_view_delegate_;

  DISALLOW_COPY_AND_ASSIGN(TrayVPN);
};

}  // namespace ash

#endif  // ASH_SYSTEM_NETWORK_TRAY_VPN_H_
