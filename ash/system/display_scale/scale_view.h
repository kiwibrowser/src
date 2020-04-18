// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_DISPLAY_SCALE_SCALE_VIEW_H_
#define ASH_SYSTEM_DISPLAY_SCALE_SCALE_VIEW_H_

#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/slider.h"
#include "ui/views/view.h"

namespace views {
class Button;
class Label;
}

namespace ash {
class SystemTrayItem;
class TriView;

namespace tray {
class ScaleView : public views::View,
                  public views::SliderListener,
                  public views::ButtonListener {
 public:
  ScaleView(SystemTrayItem* owner, bool is_default_view);

  ~ScaleView() override;

 private:
  // SliderListener:
  void SliderValueChanged(views::Slider* sender,
                          float value,
                          float old_value,
                          views::SliderChangeReason reason) override;
  void SliderDragEnded(views::Slider* sender) override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  SystemTrayItem* owner_;
  // The only immediate child view of |this|. All other view elements are added
  // to the |tri_view_| to handle layout.
  TriView* tri_view_;
  views::Button* more_button_;
  views::Label* label_;
  views::Slider* slider_;
  bool is_default_view_;

  DISALLOW_COPY_AND_ASSIGN(ScaleView);
};

}  // namespace tray
}  // namespace ash

#endif  // ASH_SYSTEM_DISPLAY_SCALE_SCALE_VIEW_H_
