// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NETWORK_DROPDOWN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NETWORK_DROPDOWN_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/observer_list.h"
#include "chrome/browser/ui/webui/chromeos/login/base_webui_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/network_dropdown.h"

namespace chromeos {

class NetworkDropdownHandler : public BaseWebUIHandler,
                               public NetworkDropdown::View {
 public:
  class Observer {
   public:
    virtual ~Observer() {}
    virtual void OnConnectToNetworkRequested() = 0;
  };

  NetworkDropdownHandler();
  ~NetworkDropdownHandler() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void Initialize() override;

  // WebUIMessageHandler implementation:
  void RegisterMessages() override;

 private:
  // NetworkDropdown::Actor implementation:
  void OnConnectToNetworkRequested() override;

  // Handles choosing of the network menu item.
  void HandleNetworkItemChosen(double id);
  // Handles network drop-down showing.
  void HandleNetworkDropdownShow(const std::string& element_id,
                                 bool oobe);
  // Handles network drop-down hiding.
  void HandleNetworkDropdownHide();
  // Handles network drop-down refresh.
  void HandleNetworkDropdownRefresh();

  void HandleLaunchInternetDetailDialog();
  void HandleLaunchAddWiFiNetworkDialog();
  void HandleShowNetworkDetails(const base::ListValue* args);
  void HandleShowNetworkConfig(const base::ListValue* args);

  std::unique_ptr<NetworkDropdown> dropdown_;

  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(NetworkDropdownHandler);
};

}  // namespace chromeos
#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_NETWORK_DROPDOWN_HANDLER_H_
