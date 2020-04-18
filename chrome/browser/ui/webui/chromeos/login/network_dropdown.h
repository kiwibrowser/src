// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NETWORK_DROPDOWN_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NETWORK_DROPDOWN_H_

#include <memory>

#include "ash/system/network/network_icon_animation_observer.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/status/network_menu.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "ui/gfx/native_widget_types.h"

namespace content {
class WebUI;
}

namespace chromeos {

class NetworkMenuWebUI;
class NetworkState;

// Class which implements network dropdown menu using WebUI.
class NetworkDropdown : public NetworkMenu::Delegate,
                        public NetworkStateHandlerObserver,
                        public ash::network_icon::AnimationObserver {
 public:
  class View {
   public:
    virtual ~View() {}
    virtual void OnConnectToNetworkRequested() = 0;
  };

  NetworkDropdown(View* view, content::WebUI* web_ui, bool oobe);
  ~NetworkDropdown() override;

  // This method should be called, when item with the given id is chosen.
  void OnItemChosen(int id);

  // NetworkMenu::Delegate
  gfx::NativeWindow GetNativeWindow() const override;
  void OpenButtonOptions() override;
  bool ShouldOpenButtonOptions() const override;
  void OnConnectToNetworkRequested() override;

  // NetworkStateHandlerObserver
  void DefaultNetworkChanged(const NetworkState* network) override;
  void NetworkConnectionStateChanged(const NetworkState* network) override;
  void NetworkListChanged() override;

  // network_icon::AnimationObserver
  void NetworkIconChanged() override;

  // Refreshes control state. Usually there's no need to do it manually
  // as control refreshes itself on network state change.
  // Should be called on language change.
  void Refresh();

 private:
  void SetNetworkIconAndText();

  // Request a network scan and refreshes control state. Should be called
  // by |network_scan_timer_| only.
  void RequestNetworkScan();

  // The Network menu.
  std::unique_ptr<NetworkMenuWebUI> network_menu_;

  View* view_;

  content::WebUI* web_ui_;

  // Is the dropdown shown on one of the OOBE screens.
  bool oobe_;

  // Timer used to periodically force network scan.
  base::RepeatingTimer network_scan_timer_;

  DISALLOW_COPY_AND_ASSIGN(NetworkDropdown);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NETWORK_DROPDOWN_H_
