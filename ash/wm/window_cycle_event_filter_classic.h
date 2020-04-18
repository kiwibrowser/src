// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WINDOW_CYCLE_EVENT_FILTER_CLASSIC_H_
#define ASH_WM_WINDOW_CYCLE_EVENT_FILTER_CLASSIC_H_

#include "ash/ash_export.h"
#include "ash/wm/window_cycle_event_filter.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "ui/events/event_handler.h"

namespace ash {

class ASH_EXPORT WindowCycleEventFilterClassic : public ui::EventHandler,
                                                 public WindowCycleEventFilter {
 public:
  WindowCycleEventFilterClassic();
  ~WindowCycleEventFilterClassic() override;

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;

 private:
  class AltReleaseHandler : public ui::EventHandler {
   public:
    AltReleaseHandler();
    ~AltReleaseHandler() override;

    void OnKeyEvent(ui::KeyEvent* event) override;

   private:
    DISALLOW_COPY_AND_ASSIGN(AltReleaseHandler);
  };

  // When the user holds Alt+Tab, this timer is used to send repeated
  // cycle commands to WindowCycleController. Note this is not accomplished
  // by marking the Alt+Tab accelerator as "repeatable" in the accelerator
  // table because we wish to control the repeat interval.
  base::RepeatingTimer repeat_timer_;

  AltReleaseHandler alt_release_handler_;

  DISALLOW_COPY_AND_ASSIGN(WindowCycleEventFilterClassic);
};

}  // namespace ash

#endif  // ASH_WM_WINDOW_CYCLE_EVENT_FILTER_CLASSIC_H_
