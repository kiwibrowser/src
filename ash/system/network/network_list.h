// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NETWORK_NETWORK_LIST_H_
#define ASH_SYSTEM_NETWORK_NETWORK_LIST_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ash/system/network/network_icon_animation_observer.h"
#include "ash/system/network/network_info.h"
#include "ash/system/network/network_state_list_detailed_view.h"
#include "base/macros.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_type_pattern.h"

namespace views {
class Separator;
class View;
}

namespace ash {
class HoverHighlightView;
class TrayInfoLabel;
class TriView;

namespace tray {

// A list of available networks of a given type. This class is used for all
// network types except VPNs. For VPNs, see the |VPNList| class.
class NetworkListView : public NetworkStateListDetailedView,
                        public network_icon::AnimationObserver {
 public:
  class SectionHeaderRowView;

  NetworkListView(DetailedViewDelegate* delegate, LoginStatus login);
  ~NetworkListView() override;

  // NetworkStateListDetailedView:
  void UpdateNetworkList() override;
  bool IsNetworkEntry(views::View* view, std::string* guid) const override;

 private:
  // Clears |network_list_| and adds to it |networks| that match |delegate_|'s
  // network type pattern.
  void UpdateNetworks(
      const chromeos::NetworkStateHandler::NetworkStateList& networks);

  // Updates |network_list_| entries and sets |this| to observe network icon
  // animations when any of the networks are in connecting state.
  void UpdateNetworkIcons();

  // Orders entries in |network_list_| such that higher priority network types
  // are at the top of the list.
  void OrderNetworks();

  // Refreshes a list of child views, updates |network_map_| and
  // |network_guid_map_| and performs layout making sure selected view if any is
  // scrolled into view.
  void UpdateNetworkListInternal();

  // Adds new or updates existing child views including header row and messages.
  // Returns a set of guids for the added network connections.
  std::unique_ptr<std::set<std::string>> UpdateNetworkListEntries();

  bool ShouldMobileDataSectionBeShown();

  // Creates the view which displays a warning message, if a VPN or proxy is
  // being used.
  TriView* CreateConnectionWarning();

  // Updates |view| with the information in |info|.
  void UpdateViewForNetwork(HoverHighlightView* view, const NetworkInfo& info);

  // Creates the a battery icon next to the name of Tether networks indicating
  // the battery percentage of the mobile device that is being used as a
  // hotspot.
  views::View* CreatePowerStatusView(const NetworkInfo& info);

  // Creates the view of an extra icon appearing next to the network name
  // indicating that the network is controlled by an extension. If no extension
  // is registered for this network, returns |nullptr|.
  views::View* CreateControlledByExtensionView(const NetworkInfo& info);

  // Adds or updates child views representing the network connections when
  // |is_wifi| is matching the attribute of a network connection starting at
  // |child_index|. Returns a set of guids for the added network
  // connections.
  std::unique_ptr<std::set<std::string>> UpdateNetworkChildren(
      NetworkInfo::Type type,
      int child_index);
  void UpdateNetworkChild(int index, const NetworkInfo* info);

  // Reorders children of |scroll_content()| as necessary placing |view| at
  // |index|.
  void PlaceViewAtIndex(views::View* view, int index);

  // Creates an info label with text specified by |message_id| and adds it to
  // |scroll_content()| if necessary or updates the text and reorders the
  // |scroll_content()| placing the info label at |insertion_index|. When
  // |message_id| is zero removes the |*info_label_ptr| from the
  // |scroll_content()| and destroys it. |info_label_ptr| is an in/out parameter
  // and is only modified if the info label is created or destroyed.
  void UpdateInfoLabel(int message_id,
                       int insertion_index,
                       TrayInfoLabel** info_label_ptr);

  // Creates a cellular/tether/Wi-Fi header row |view| and adds it to
  // |scroll_content()| if necessary and reorders the |scroll_content()| placing
  // the |view| at |child_index|. Returns the index where the next child should
  // be inserted, i.e., the index directly after the last inserted child.
  int UpdateSectionHeaderRow(chromeos::NetworkTypePattern pattern,
                             bool enabled,
                             int child_index,
                             SectionHeaderRowView** view,
                             views::Separator** separator_view);

  // network_icon::AnimationObserver:
  void NetworkIconChanged() override;

  // Returns true if the info should be updated to the view for network,
  // otherwise false.
  bool NeedUpdateViewForNetwork(const NetworkInfo& info) const;

  bool needs_relayout_;

  TrayInfoLabel* no_wifi_networks_view_;
  SectionHeaderRowView* mobile_header_view_;
  SectionHeaderRowView* wifi_header_view_;
  views::Separator* mobile_separator_view_;
  views::Separator* wifi_separator_view_;
  TriView* connection_warning_;

  // An owned list of network info.
  std::vector<std::unique_ptr<NetworkInfo>> network_list_;

  using NetworkMap = std::map<views::View*, std::string>;
  NetworkMap network_map_;

  // A map of network guids to their view.
  using NetworkGuidMap = std::map<std::string, HoverHighlightView*>;
  NetworkGuidMap network_guid_map_;

  // Save a map of network guids to their infos against current |network_list_|.
  using NetworkInfoMap = std::map<std::string, std::unique_ptr<NetworkInfo>>;
  NetworkInfoMap last_network_info_map_;

  DISALLOW_COPY_AND_ASSIGN(NetworkListView);
};

}  // namespace tray
}  // namespace ash

#endif  // ASH_SYSTEM_NETWORK_NETWORK_LIST_H_
