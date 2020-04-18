// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MASH_SIMPLE_WM_MOVE_EVENT_HANDLER_H_
#define MASH_SIMPLE_WM_MOVE_EVENT_HANDLER_H_

#include <memory>

#include "base/macros.h"
#include "ui/aura/window_observer.h"
#include "ui/events/event_handler.h"

namespace aura {
class Window;
}

namespace ui {
class LocatedEvent;
}

namespace simple_wm {

class MoveLoop;

// EventHandler attached to the root. Starts a MoveLoop as necessary.
class MoveEventHandler : public ui::EventHandler, public aura::WindowObserver {
 public:
  explicit MoveEventHandler(aura::Window* window);
  ~MoveEventHandler() override;

 private:
  void ProcessLocatedEvent(ui::LocatedEvent* event);
  int GetNonClientComponentForEvent(const ui::LocatedEvent* event);

  // Removes observer and EventHandler installed on |window_|.
  void Detach();

  // Overridden from ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnTouchEvent(ui::TouchEvent* event) override;
  void OnCancelMode(ui::CancelModeEvent* event) override;

  // Overridden from aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

  aura::Window* window_;
  std::unique_ptr<MoveLoop> move_loop_;

  DISALLOW_COPY_AND_ASSIGN(MoveEventHandler);
};

}  // namespace simple_wm

#endif  // MASH_SIMPLE_WM_MOVE_EVENT_HANDLER_H_
