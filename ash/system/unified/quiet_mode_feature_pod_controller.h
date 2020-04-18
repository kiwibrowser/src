// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_QUIET_MODE_FEATURE_POD_CONTROLLER_H_
#define ASH_SYSTEM_UNIFIED_QUIET_MODE_FEATURE_POD_CONTROLLER_H_

#include "ash/system/unified/feature_pod_controller_base.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/message_center/message_center_observer.h"

namespace ash {

// Controller of a feature pod button that toggles do-not-disturb mode.
// If the do-not-disturb mode is enabled, the button indicates it by bright
// background color and different icon.
class QuietModeFeaturePodController
    : public FeaturePodControllerBase,
      public message_center::MessageCenterObserver {
 public:
  QuietModeFeaturePodController();
  ~QuietModeFeaturePodController() override;

  // FeaturePodControllerBase:
  FeaturePodButton* CreateButton() override;
  void OnIconPressed() override;

  // message_center::MessageCenterObserver:
  void OnQuietModeChanged(bool in_quiet_mode) override;

 private:
  FeaturePodButton* button_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(QuietModeFeaturePodController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_QUIET_MODE_FEATURE_POD_CONTROLLER_H_
