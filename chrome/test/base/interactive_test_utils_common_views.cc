// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Methods compiled on all toolkit-views platforms (including Mac).

#include "chrome/test/base/interactive_test_utils.h"

#include "ui/compositor/layer.h"
#include "ui/compositor/layer_animator.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace ui_test_utils {

void MoveMouseToCenterAndPress(views::View* view,
                               ui_controls::MouseButton button,
                               int state,
                               const base::Closure& closure) {
  DCHECK(view);
  DCHECK(view->GetWidget());
  // Complete any in-progress animation before sending the events so that the
  // mouse-event targetting happens reliably, and does not flake because of
  // unreliable animation state.
  ui::Layer* layer = view->GetWidget()->GetLayer();
  if (layer) {
    ui::LayerAnimator* animator = layer->GetAnimator();
    if (animator && animator->is_animating())
      animator->StopAnimating();
  }

  gfx::Point view_center(view->width() / 2, view->height() / 2);
  views::View::ConvertPointToScreen(view, &view_center);
  ui_controls::SendMouseMoveNotifyWhenDone(
      view_center.x(), view_center.y(),
      base::BindOnce(&internal::ClickTask, button, state, closure));
}

}  // namespace ui_test_utils
