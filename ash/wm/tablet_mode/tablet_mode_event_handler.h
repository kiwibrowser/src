// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_TABLET_MODE_TABLET_MODE_EVENT_HANDLER_H_
#define ASH_WM_TABLET_MODE_TABLET_MODE_EVENT_HANDLER_H_

#include "base/macros.h"

namespace ui {
class TouchEvent;
}

namespace ash {
namespace wm {

// TabletModeEventHandler handles toggling fullscreen when appropriate.
// TabletModeEventHandler installs event handlers in an environment specific
// way, e.g. EventHandler for aura.
class TabletModeEventHandler {
 public:
  TabletModeEventHandler();
  virtual ~TabletModeEventHandler();

 protected:
  // Subclasses call this to toggle fullscreen. If a toggle happened returns
  // true.
  bool ToggleFullscreen(const ui::TouchEvent& event);

 private:
  DISALLOW_COPY_AND_ASSIGN(TabletModeEventHandler);
};

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_TABLET_MODE_TABLET_MODE_EVENT_HANDLER_H_
