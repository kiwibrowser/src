// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_TRAY_EVENT_FILTER_H_
#define ASH_SYSTEM_TRAY_TRAY_EVENT_FILTER_H_

#include <set>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/views/pointer_watcher.h"

namespace gfx {
class Point;
}

namespace ui {
class PointerEvent;
}

namespace ash {
class TrayBubbleBase;

// Handles events for a tray bubble, e.g. to close the system tray bubble when
// the user clicks outside it.
class ASH_EXPORT TrayEventFilter : public views::PointerWatcher {
 public:
  TrayEventFilter();
  ~TrayEventFilter() override;

  void AddBubble(TrayBubbleBase* bubble);
  void RemoveBubble(TrayBubbleBase* bubble);

  // views::PointerWatcher:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override;

 private:
  void ProcessPressedEvent(const gfx::Point& location_in_screen,
                           gfx::NativeView target);

  std::set<TrayBubbleBase*> bubbles_;

  DISALLOW_COPY_AND_ASSIGN(TrayEventFilter);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_TRAY_EVENT_FILTER_H_
