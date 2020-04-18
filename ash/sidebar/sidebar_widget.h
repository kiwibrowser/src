// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SIDEBAR_SIDEBAR_WIDGET_H_
#define ASH_SIDEBAR_SIDEBAR_WIDGET_H_

#include "ash/sidebar/sidebar_params.h"
#include "ui/display/display_observer.h"
#include "ui/views/widget/widget.h"

namespace aura {
class Window;
}

namespace ash {
class Sidebar;
class Shelf;

class SidebarWidget : public views::Widget, public display::DisplayObserver {
 public:
  SidebarWidget(aura::Window* sidebar_container,
                Sidebar* sidebar,
                Shelf* shelf,
                SidebarInitMode mode);
  ~SidebarWidget() override;

  void Reinitialize(SidebarInitMode mode);

  // Overridden from display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t metrics) override;

  void SetSystemTrayView(views::View* view);

 private:
  class DelegateView;

  // The sidebar instance managing this widget.
  Sidebar* sidebar_;
  // The shelf which this widget belongs to.
  Shelf* shelf_;
  // DelegateView for this widget. Owned by the hierarchy (it means this is
  // indirectly owned by this widget).
  DelegateView* delegate_view_;

  DISALLOW_COPY_AND_ASSIGN(SidebarWidget);
};

}  // namespace ash

#endif  // ASH_SIDEBAR_SIDEBAR_WIDGET_H_
