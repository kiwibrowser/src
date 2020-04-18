// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_TABLET_MODE_TABLET_MODE_EVENT_HANDLER_CLASSIC_H_
#define ASH_WM_TABLET_MODE_TABLET_MODE_EVENT_HANDLER_CLASSIC_H_

#include "ash/ash_export.h"
#include "ash/wm/tablet_mode/tablet_mode_event_handler.h"
#include "ui/events/event_handler.h"

namespace ash {
namespace wm {

// Implementation of TabletModeEventHandler for aura. Uses ui::EventHandler.
class ASH_EXPORT TabletModeEventHandlerClassic : public TabletModeEventHandler,
                                                 public ui::EventHandler {
 public:
  TabletModeEventHandlerClassic();
  ~TabletModeEventHandlerClassic() override;

 private:
  // ui::EventHandler override:
  void OnTouchEvent(ui::TouchEvent* event) override;

  DISALLOW_COPY_AND_ASSIGN(TabletModeEventHandlerClassic);
};

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_TABLET_MODE_TABLET_MODE_EVENT_HANDLER_CLASSIC_H_
