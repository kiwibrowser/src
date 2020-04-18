// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WIDGET_FINDER_H_
#define ASH_WIDGET_FINDER_H_

#include "ash/ash_export.h"

namespace aura {
class Window;
}

namespace views {
class Widget;
}

namespace ash {

// Returns the widget associated with |window|, or null if not associated with
// a widget. Only ash system UI widgets are returned, not widgets created
// by the mus window manager code to show a non-client frame.
ASH_EXPORT views::Widget* GetInternalWidgetForWindow(aura::Window* window);

}  // namespace ash

#endif  // ASH_WIDGET_FINDER_H_
