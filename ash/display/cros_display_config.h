// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_CROS_DISPLAY_CONFIG_H_
#define ASH_DISPLAY_CROS_DISPLAY_CONFIG_H_

#include <map>
#include <memory>
#include <string>

#include "ash/ash_export.h"
#include "ash/public/interfaces/cros_display_config.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace ash {

class OverscanCalibrator;
class TouchCalibratorController;

// ASH_EXPORT for use in chrome unit_tests for DisplayInfoProviderChromeOS.
class ASH_EXPORT CrosDisplayConfig : public mojom::CrosDisplayConfigController {
 public:
  CrosDisplayConfig();
  ~CrosDisplayConfig() override;

  void BindRequest(mojom::CrosDisplayConfigControllerRequest request);

  // mojom::CrosDisplayConfigController:
  void AddObserver(
      mojom::CrosDisplayConfigObserverAssociatedPtrInfo observer) override;
  void GetDisplayLayoutInfo(GetDisplayLayoutInfoCallback callback) override;
  void SetDisplayLayoutInfo(mojom::DisplayLayoutInfoPtr info,
                            SetDisplayLayoutInfoCallback callback) override;
  void GetDisplayUnitInfoList(bool single_unified,
                              GetDisplayUnitInfoListCallback callback) override;
  void SetDisplayProperties(const std::string& id,
                            mojom::DisplayConfigPropertiesPtr properties,
                            SetDisplayPropertiesCallback callback) override;
  void SetUnifiedDesktopEnabled(bool enabled) override;
  void OverscanCalibration(const std::string& display_id,
                           mojom::DisplayConfigOperation op,
                           const base::Optional<gfx::Insets>& delta,
                           OverscanCalibrationCallback callback) override;
  void TouchCalibration(const std::string& display_id,
                        mojom::DisplayConfigOperation op,
                        mojom::TouchCalibrationPtr calibration,
                        TouchCalibrationCallback callback) override;

  TouchCalibratorController* touch_calibrator_for_test() {
    return touch_calibrator_.get();
  }

 private:
  class DisplayObserver;
  void NotifyObserversDisplayConfigChanged();
  OverscanCalibrator* GetOverscanCalibrator(const std::string& id);

  std::unique_ptr<DisplayObserver> display_observer_;
  mojo::BindingSet<mojom::CrosDisplayConfigController> bindings_;
  mojo::AssociatedInterfacePtrSet<mojom::CrosDisplayConfigObserver> observers_;
  std::map<std::string, std::unique_ptr<OverscanCalibrator>>
      overscan_calibrators_;
  std::unique_ptr<TouchCalibratorController> touch_calibrator_;

  DISALLOW_COPY_AND_ASSIGN(CrosDisplayConfig);
};

}  // namespace ash

#endif  // ASH_DISPLAY_CROS_DISPLAY_CONFIG_H_
