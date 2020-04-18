// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/layout_util.h"

#include "ash/login/ui/non_accessible_view.h"
#include "ash/shell.h"
#include "ui/display/manager/display_manager.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace login_layout_util {

views::View* WrapViewForPreferredSize(views::View* view) {
  auto* proxy = new NonAccessibleView();
  auto layout_manager =
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical);
  layout_manager->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_START);
  proxy->SetLayoutManager(std::move(layout_manager));
  proxy->AddChildView(view);
  return proxy;
}

bool ShouldShowLandscape(const views::Widget* widget) {
  // |widget| is null when the view is being constructed. Default to landscape
  // in that case. A new layout will happen when the view is attached to a
  // widget (see LockContentsView::AddedToWidget), which will let us fetch the
  // correct display orientation.
  if (!widget)
    return true;

  // Get the orientation for |widget|.
  const display::Display& display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(
          widget->GetNativeWindow());
  display::ManagedDisplayInfo info =
      Shell::Get()->display_manager()->GetDisplayInfo(display.id());

  // Return true if it is landscape.
  switch (info.GetActiveRotation()) {
    case display::Display::ROTATE_0:
    case display::Display::ROTATE_180:
      return true;
    case display::Display::ROTATE_90:
    case display::Display::ROTATE_270:
      return false;
  }
  NOTREACHED();
  return true;
}

}  // namespace login_layout_util
}  // namespace ash
