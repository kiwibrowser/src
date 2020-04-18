// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_STATUS_NETWORK_MENU_H_
#define CHROME_BROWSER_CHROMEOS_STATUS_NETWORK_MENU_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "ui/gfx/native_widget_types.h"  // gfx::NativeWindow

namespace ui {
class MenuModel;
}

namespace chromeos {

class NetworkMenuModel;

// This class builds and manages a ui::MenuModel used to build network
// menus. It does not represent an actual menu widget. The menu is populated
// with the list of wifi and mobile networks, and handles connecting to
// existing networks or spawning UI to configure a new network.
//
// This class is now only used to build the dropdown menu in the oobe and
// other web based login screen UI. See ash::NetworkStateListDetailedView for
// the status area network list. TODO(stevenjb): Integrate this with the
// login UI code.
//
// The network menu model looks like this:
//
// <icon>  Ethernet
// <icon>  Wifi Network A
// <icon>  Wifi Network B
// <icon>  Wifi Network C
// <icon>  Cellular Network A
// <icon>  Cellular Network B
// <icon>  Cellular Network C
// <icon>  Other Wi-Fi network...
// --------------------------------
//         Disable Wifi
//         Disable Celluar
// --------------------------------
//         <IP Address>
//         Proxy settings...
//
// <icon> will show the strength of the wifi/cellular networks.
// The label will be BOLD if the network is currently connected.

class NetworkMenu {
 public:
  class Delegate {
   public:
    virtual ~Delegate();
    virtual gfx::NativeWindow GetNativeWindow() const = 0;
    virtual void OpenButtonOptions() = 0;
    virtual bool ShouldOpenButtonOptions() const = 0;
    virtual void OnConnectToNetworkRequested() = 0;
  };

  explicit NetworkMenu(Delegate* delegate);
  virtual ~NetworkMenu();

  // Access to menu definition.
  ui::MenuModel* GetMenuModel();

  // Update the menu (e.g. when the network list or status has changed).
  virtual void UpdateMenu();

 private:
  friend class NetworkMenuModel;

  // Getters.
  Delegate* delegate() const { return delegate_; }

  // Weak ptr to delegate.
  Delegate* delegate_;

  // Set to true if we are currently refreshing the menu.
  bool refreshing_menu_;

  // The network menu.
  std::unique_ptr<NetworkMenuModel> main_menu_model_;

  // Weak pointer factory so we can start connections at a later time
  // without worrying that they will actually try to happen after the lifetime
  // of this object.
  base::WeakPtrFactory<NetworkMenu> weak_pointer_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkMenu);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_STATUS_NETWORK_MENU_H_
