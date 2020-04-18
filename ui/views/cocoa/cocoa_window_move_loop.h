// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COCOA_COCOA_WINDOW_MOVE_LOOP_H_
#define UI_VIEWS_COCOA_COCOA_WINDOW_MOVE_LOOP_H_

#import <Cocoa/Cocoa.h>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "ui/views/widget/widget.h"

namespace views {
class BridgedNativeWidget;

// Used by views::BridgedNativeWidget when dragging detached tabs.
class CocoaWindowMoveLoop {
 public:
  CocoaWindowMoveLoop(BridgedNativeWidget* owner,
                      const NSPoint& initial_mouse_in_screen);
  ~CocoaWindowMoveLoop();

  // Initiates the drag until a mouse up event is observed, or End() is called.
  Widget::MoveLoopResult Run();
  void End();

 private:
  enum LoopExitReason {
    ENDED_EXTERNALLY,
    MOUSE_UP,
    WINDOW_DESTROYED,
  };

  BridgedNativeWidget* owner_;  // Weak. Owns this.

  // Initial mouse location at the time before the CocoaWindowMoveLoop is
  // created.
  NSPoint initial_mouse_in_screen_;

  // Pointer to a stack variable holding the exit reason.
  LoopExitReason* exit_reason_ref_ = nullptr;
  base::Closure quit_closure_;

  // WeakPtrFactory for event monitor safety.
  base::WeakPtrFactory<CocoaWindowMoveLoop> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CocoaWindowMoveLoop);
};

}  // namespace views

#endif  // UI_VIEWS_COCOA_COCOA_WINDOW_MOVE_LOOP_H_
