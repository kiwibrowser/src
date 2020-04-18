// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/tablet_mode/tablet_mode_event_handler_classic.h"

#include "ash/shell.h"
#include "ui/events/event.h"

namespace ash {
namespace wm {

TabletModeEventHandlerClassic::TabletModeEventHandlerClassic() {
  Shell::Get()->AddPreTargetHandler(this);
}

TabletModeEventHandlerClassic::~TabletModeEventHandlerClassic() {
  Shell::Get()->RemovePreTargetHandler(this);
}

void TabletModeEventHandlerClassic::OnTouchEvent(ui::TouchEvent* event) {
  if (ToggleFullscreen(*event))
    event->StopPropagation();
}

}  // namespace wm
}  // namespace ash
