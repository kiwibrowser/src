// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/network_feature_pod_button.h"

#include "ash/shell.h"
#include "ash/system/network/network_icon.h"
#include "ash/system/network/network_icon_animation.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "chromeos/network/network_state_handler.h"

namespace ash {

namespace {

bool IsActive() {
  return chromeos::NetworkHandler::Get()
             ->network_state_handler()
             ->ConnectedNetworkByType(
                 chromeos::NetworkTypePattern::NonVirtual()) != nullptr;
}

}  // namespace

NetworkFeaturePodButton::NetworkFeaturePodButton(
    FeaturePodControllerBase* controller)
    : FeaturePodButton(controller) {
  network_state_observer_ = std::make_unique<TrayNetworkStateObserver>(this);
  Update();
}

NetworkFeaturePodButton::~NetworkFeaturePodButton() {
  network_icon::NetworkIconAnimation::GetInstance()->RemoveObserver(this);
}

void NetworkFeaturePodButton::NetworkIconChanged() {
  Update();
}

void NetworkFeaturePodButton::NetworkStateChanged(bool notify_a11y) {
  Update();
}

void NetworkFeaturePodButton::Update() {
  gfx::ImageSkia image;
  base::string16 label;
  bool animating = false;
  network_icon::GetDefaultNetworkImageAndLabel(
      network_icon::ICON_TYPE_DEFAULT_VIEW, &image, &label, &animating);

  if (animating)
    network_icon::NetworkIconAnimation::GetInstance()->AddObserver(this);
  else
    network_icon::NetworkIconAnimation::GetInstance()->RemoveObserver(this);

  SetToggled(IsActive() ||
             chromeos::NetworkHandler::Get()
                 ->network_state_handler()
                 ->IsTechnologyEnabled(chromeos::NetworkTypePattern::WiFi()));
  icon_button()->SetImage(views::Button::STATE_NORMAL, image);
  SetLabel(label);
}

}  // namespace ash
