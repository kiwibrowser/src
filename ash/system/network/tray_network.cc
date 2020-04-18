// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/tray_network.h"

#include "ash/metrics/user_metrics_recorder.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/network/network_icon.h"
#include "ash/system/network/network_icon_animation.h"
#include "ash/system/network/network_icon_animation_observer.h"
#include "ash/system/network/network_list.h"
#include "ash/system/network/network_tray_view.h"
#include "ash/system/network/tray_network_state_observer.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_item_detailed_view_delegate.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_item_more.h"
#include "ash/system/tray/tray_item_view.h"
#include "ash/system/tray/tray_popup_item_style.h"
#include "ash/system/tray/tray_utils.h"
#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/widget/widget.h"

using chromeos::NetworkHandler;
using chromeos::NetworkTypePattern;

namespace ash {
namespace tray {

class NetworkDefaultView : public TrayItemMore,
                           public network_icon::AnimationObserver {
 public:
  explicit NetworkDefaultView(TrayNetwork* network_tray)
      : TrayItemMore(network_tray) {
    Update();
  }

  ~NetworkDefaultView() override {
    network_icon::NetworkIconAnimation::GetInstance()->RemoveObserver(this);
  }

  void Update() {
    gfx::ImageSkia image;
    base::string16 label;
    bool animating = false;
    network_icon::GetDefaultNetworkImageAndLabel(
        network_icon::ICON_TYPE_DEFAULT_VIEW, &image, &label, &animating);
    // We use the inactive icon alpha only if there is no active network and
    // wifi is disabled.
    if (!IsActive() &&
        !NetworkHandler::Get()->network_state_handler()->IsTechnologyEnabled(
            NetworkTypePattern::WiFi())) {
      image = gfx::ImageSkiaOperations::CreateTransparentImage(
          image, TrayPopupItemStyle::kInactiveIconAlpha);
    }

    if (animating)
      network_icon::NetworkIconAnimation::GetInstance()->AddObserver(this);
    else
      network_icon::NetworkIconAnimation::GetInstance()->RemoveObserver(this);
    SetImage(image);
    SetLabel(label);
    SetAccessibleName(label);
    UpdateStyle();
  }

  // network_icon::AnimationObserver
  void NetworkIconChanged() override { Update(); }

 protected:
  // TrayItemMore:
  std::unique_ptr<TrayPopupItemStyle> HandleCreateStyle() const override {
    std::unique_ptr<TrayPopupItemStyle> style =
        TrayItemMore::HandleCreateStyle();
    style->set_color_style(IsActive()
                               ? TrayPopupItemStyle::ColorStyle::ACTIVE
                               : TrayPopupItemStyle::ColorStyle::INACTIVE);
    return style;
  }

  // Determines whether to use the ACTIVE or INACTIVE text style.
  bool IsActive() const { return GetConnectedNetwork() != nullptr; }

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkDefaultView);
};

}  // namespace tray

TrayNetwork::TrayNetwork(SystemTray* system_tray)
    : SystemTrayItem(system_tray, UMA_NETWORK),
      tray_(nullptr),
      default_(nullptr),
      detailed_(nullptr),
      detailed_view_delegate_(
          std::make_unique<SystemTrayItemDetailedViewDelegate>(this)) {
  network_state_observer_.reset(new TrayNetworkStateObserver(this));
}

TrayNetwork::~TrayNetwork() = default;

views::View* TrayNetwork::CreateTrayView(LoginStatus status) {
  CHECK(tray_ == nullptr);
  if (!chromeos::NetworkHandler::IsInitialized())
    return nullptr;
  tray_ = new tray::NetworkTrayView(this);
  return tray_;
}

views::View* TrayNetwork::CreateDefaultView(LoginStatus status) {
  CHECK(default_ == nullptr);
  if (!chromeos::NetworkHandler::IsInitialized())
    return nullptr;
  CHECK(tray_ != nullptr);
  default_ = new tray::NetworkDefaultView(this);
  default_->SetEnabled(status != LoginStatus::LOCKED);
  return default_;
}

views::View* TrayNetwork::CreateDetailedView(LoginStatus status) {
  CHECK(detailed_ == nullptr);
  Shell::Get()->metrics()->RecordUserMetricsAction(
      UMA_STATUS_AREA_DETAILED_NETWORK_VIEW);
  if (!chromeos::NetworkHandler::IsInitialized())
    return nullptr;
  detailed_ = new tray::NetworkListView(detailed_view_delegate_.get(), status);
  detailed_->Init();
  return detailed_;
}

void TrayNetwork::OnTrayViewDestroyed() {
  tray_ = nullptr;
}

void TrayNetwork::OnDefaultViewDestroyed() {
  default_ = nullptr;
}

void TrayNetwork::OnDetailedViewDestroyed() {
  detailed_ = nullptr;
}

void TrayNetwork::NetworkStateChanged(bool notify_a11y) {
  if (tray_) {
    tray_->UpdateNetworkStateHandlerIcon();
    tray_->UpdateConnectionStatus(tray::GetConnectedNetwork(), notify_a11y);
  }
  if (default_)
    default_->Update();
  if (detailed_)
    detailed_->Update();
}

}  // namespace ash
