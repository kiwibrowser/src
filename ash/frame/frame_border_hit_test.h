// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_FRAME_FRAME_BORDER_HIT_TEST_H_
#define ASH_FRAME_FRAME_BORDER_HIT_TEST_H_

#include "ash/ash_export.h"

namespace gfx {
class Point;
}

namespace views {
class NonClientFrameView;
class View;
}

namespace ash {

class FrameCaptionButtonContainerView;

// Returns the HitTestCompat for the specified point.
ASH_EXPORT int FrameBorderNonClientHitTest(
    views::NonClientFrameView* view,
    const views::View* back_button,
    const FrameCaptionButtonContainerView* caption_button_container,
    const gfx::Point& point_in_widget);

}  // namespace ash

#endif  // ASH_FRAME_FRAME_BORDER_HIT_TEST_H_
