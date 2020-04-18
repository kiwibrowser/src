// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_UI_LAYOUT_UTIL_H_
#define ASH_LOGIN_UI_LAYOUT_UTIL_H_

#include "ash/ash_export.h"

namespace views {
class View;
class Widget;
}

namespace ash {

namespace login_layout_util {

// Wraps view in another view so the original view is sized to it's preferred
// size, regardless of the view's parent's layout manager.
ASH_EXPORT views::View* WrapViewForPreferredSize(views::View* view);

// Returns true if landscape constants should be used for UI shown in |widget|.
ASH_EXPORT bool ShouldShowLandscape(const views::Widget* widget);
}  // namespace login_layout_util

}  // namespace ash

#endif  // ASH_LOGIN_UI_LAYOUT_UTIL_H_
