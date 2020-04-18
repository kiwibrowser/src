// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NETWORK_NETWORK_STATE_LIST_DETAILED_VIEW_H_
#define ASH_SYSTEM_NETWORK_NETWORK_STATE_LIST_DETAILED_VIEW_H_

#include <string>

#include "ash/login_status.h"
#include "ash/system/tray/tray_detailed_view.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace views {
class Button;
}

namespace ash {

namespace tray {

// Exported for tests.
class ASH_EXPORT NetworkStateListDetailedView
    : public TrayDetailedView,
      public base::SupportsWeakPtr<NetworkStateListDetailedView> {
 public:
  ~NetworkStateListDetailedView() override;

  void Init();

  // Called when the contents of the network list have changed or when any
  // Manager properties (e.g. technology state) have changed.
  void Update();

  void ToggleInfoBubbleForTesting();

 protected:
  enum ListType { LIST_TYPE_NETWORK, LIST_TYPE_VPN };

  NetworkStateListDetailedView(DetailedViewDelegate* delegate,
                               ListType list_type,
                               LoginStatus login);

  // Refreshes the network list.
  virtual void UpdateNetworkList() = 0;

  // Checks whether |view| represents a network in the list. If yes, sets
  // |guid| to the network's guid and returns |true|. Otherwise,
  // leaves |guid| unchanged and returns |false|.
  virtual bool IsNetworkEntry(views::View* view, std::string* guid) const = 0;

 private:
  class InfoBubble;

  // TrayDetailedView:
  void HandleViewClicked(views::View* view) override;
  void HandleButtonPressed(views::Button* sender,
                           const ui::Event& event) override;
  void CreateExtraTitleRowButtons() override;

  // Launches the WebUI settings in a browser and closes the system menu.
  void ShowSettings();

  // Update UI components.
  void UpdateHeaderButtons();

  // Create and manage the network info bubble.
  void ToggleInfoBubble();
  bool ResetInfoBubble();
  void OnInfoBubbleDestroyed();
  views::View* CreateNetworkInfoView();

  // Periodically request a network scan.
  void CallRequestScan();

  // Type of list (all networks or vpn)
  ListType list_type_;

  // Track login state.
  LoginStatus login_;

  views::Button* info_button_;
  views::Button* settings_button_;

  // A small bubble for displaying network info.
  InfoBubble* info_bubble_;

  DISALLOW_COPY_AND_ASSIGN(NetworkStateListDetailedView);
};

}  // namespace tray
}  // namespace ash

#endif  // ASH_SYSTEM_NETWORK_NETWORK_STATE_LIST_DETAILED_VIEW_H_
