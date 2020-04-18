// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_AUTOFILL_POPUP_VIEW_COMMON_H_
#define CHROME_BROWSER_UI_AUTOFILL_POPUP_VIEW_COMMON_H_

#include "base/macros.h"
#include "ui/gfx/native_widget_types.h"

namespace display {
class Display;
}

namespace gfx {
class Point;
class Rect;
}

namespace autofill {

// Provides utility functions for popup-style views.
class PopupViewCommon {
 public:
  // Returns the bounds that the popup should be placed at, given the desired
  // width and height. By default this places the popup below |element_bounds|
  // but it will be placed above if there isn't enough space.
  gfx::Rect CalculatePopupBounds(int desired_width,
                                 int desired_height,
                                 const gfx::Rect& element_bounds,
                                 gfx::NativeView container_view,
                                 bool is_rtl);

 protected:
  // A helper function to get the display closest to the given point (virtual
  // for testing).
  virtual display::Display GetDisplayNearestPoint(
      const gfx::Point& point,
      gfx::NativeView container_view);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_AUTOFILL_POPUP_VIEW_COMMON_H_
