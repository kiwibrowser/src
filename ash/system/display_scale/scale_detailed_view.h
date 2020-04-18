// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_DISPLAY_SCALE_SCALE_DETAILED_VIEW_H_
#define ASH_SYSTEM_DISPLAY_SCALE_SCALE_DETAILED_VIEW_H_

#include "ash/system/tray/tray_detailed_view.h"
#include "base/macros.h"

namespace views {
class View;
}

namespace ash {
class HoverHighlightView;

namespace tray {

class ScaleDetailedView : public TrayDetailedView {
 public:
  explicit ScaleDetailedView(DetailedViewDelegate* delegate);

  ~ScaleDetailedView() override;

 private:
  HoverHighlightView* AddScrollListItem(const base::string16& text,
                                        bool highlight,
                                        bool checked);

  void UpdateScrollableList();

  // TrayDetailedView:
  void HandleViewClicked(views::View* view) override;

  std::map<views::View*, double> view_to_scale_;

  DISALLOW_COPY_AND_ASSIGN(ScaleDetailedView);
};

}  // namespace tray
}  // namespace ash

#endif  // ASH_SYSTEM_DISPLAY_SCALE_SCALE_DETAILED_VIEW_H_
