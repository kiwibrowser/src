// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NIGHT_LIGHT_NIGHT_LIGHT_FEATURE_POD_CONTROLLER_H_
#define ASH_SYSTEM_NIGHT_LIGHT_NIGHT_LIGHT_FEATURE_POD_CONTROLLER_H_

#include "ash/system/unified/feature_pod_controller_base.h"
#include "base/macros.h"

namespace ash {

// Controller of a feature pod button that toggles night light mode.
class NightLightFeaturePodController : public FeaturePodControllerBase {
 public:
  NightLightFeaturePodController();
  ~NightLightFeaturePodController() override;

  // FeaturePodControllerBase:
  FeaturePodButton* CreateButton() override;
  void OnIconPressed() override;

 private:
  void UpdateButton();

  FeaturePodButton* button_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(NightLightFeaturePodController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_NIGHT_LIGHT_NIGHT_LIGHT_FEATURE_POD_CONTROLLER_H_
