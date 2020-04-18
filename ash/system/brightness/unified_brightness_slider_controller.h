// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_BRIGHTNESS_UNIFIED_BRIGHTNESS_SLIDER_CONTROLLER_H_
#define ASH_SYSTEM_BRIGHTNESS_UNIFIED_BRIGHTNESS_SLIDER_CONTROLLER_H_

#include "ash/system/unified/unified_slider_view.h"

namespace ash {

class UnifiedSystemTrayModel;

// Controller of a slider that can change display brightness.
class UnifiedBrightnessSliderController : public UnifiedSliderListener {
 public:
  explicit UnifiedBrightnessSliderController(UnifiedSystemTrayModel* model);
  ~UnifiedBrightnessSliderController() override;

  // Instantiates UnifiedSliderView. The view will be onwed by views hierarchy.
  // The view should be always deleted after the controller is destructed.
  views::View* CreateView();

  // UnifiedSliderListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;
  void SliderValueChanged(views::Slider* sender,
                          float value,
                          float old_value,
                          views::SliderChangeReason reason) override;

 private:
  UnifiedSystemTrayModel* const model_;
  UnifiedSliderView* slider_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(UnifiedBrightnessSliderController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_BRIGHTNESS_UNIFIED_BRIGHTNESS_SLIDER_CONTROLLER_H_
