// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MASH_SIMPLE_WM_MOVE_LOOP_H_
#define MASH_SIMPLE_WM_MOVE_LOOP_H_

#include <stdint.h>
#include <memory>

#include "base/macros.h"
#include "ui/aura/window_observer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {
class PointerEvent;
}

namespace simple_wm {

// MoveLoop is responsible for moving/resizing windows.
class MoveLoop : public aura::WindowObserver {
 public:
  enum MoveResult {
    // The move is still ongoing.
    CONTINUE,
    // The move is done and the MoveLoop should be destroyed.
    DONE,
  };

  enum class HorizontalLocation {
    LEFT,
    RIGHT,
    OTHER,
  };

  enum class VerticalLocation {
    TOP,
    BOTTOM,
    OTHER,
  };

  ~MoveLoop() override;

  // If a move/resize loop should occur for the specified parameters creates
  // and returns a new MoveLoop. All events should be funneled to the MoveLoop
  // until done (Move()). |ht_location| is one of the constants defined by
  // HitTestCompat.
  static std::unique_ptr<MoveLoop> Create(aura::Window* target,
                                          int ht_location,
                                          const ui::PointerEvent& event);

  // Processes an event for a move/resize loop.
  MoveResult Move(const ui::PointerEvent& event);

  // If possible reverts any changes made during the move loop.
  void Revert();

 private:
  enum class Type {
    MOVE,
    RESIZE,
  };

  MoveLoop(aura::Window* target,
           const ui::PointerEvent& event,
           Type type,
           HorizontalLocation h_loc,
           VerticalLocation v_loc);

  // Determines the type of move from the specified HitTestCompat value.
  // Returns true if a move/resize should occur.
  static bool DetermineType(int ht_location,
                            Type* type,
                            HorizontalLocation* h_loc,
                            VerticalLocation* v_loc);

  // Does the actual move/resize.
  void MoveImpl(const ui::PointerEvent& event);

  // Cancels the loop. This sets |target_| to null and removes the observer.
  // After this the MoveLoop is still ongoing and won't stop until the
  // appropriate event is received.
  void Cancel();

  gfx::Rect DetermineBoundsFromDelta(const gfx::Vector2d& delta);

  // aura::WindowObserver:
  void OnWindowHierarchyChanged(const HierarchyChangeParams& params) override;
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;
  void OnWindowVisibilityChanged(aura::Window* window, bool visible) override;

  // The window this MoveLoop is acting on. |target_| is set to null if the
  // window unexpectedly changes while the move is in progress.
  aura::Window* target_;

  const Type type_;
  const HorizontalLocation h_loc_;
  const VerticalLocation v_loc_;

  // The id of the pointer that triggered the move.
  const int32_t pointer_id_;

  // Location of the event (in screen coordinates) that triggered the move.
  const gfx::Point initial_event_screen_location_;

  // Original bounds of the window.
  gfx::Rect initial_window_bounds_;
  const gfx::Rect initial_user_set_bounds_;

  // Set to true when MoveLoop changes the bounds of |target_|. The move is
  // canceled if the bounds change unexpectedly during the move.
  bool changing_bounds_;

  DISALLOW_COPY_AND_ASSIGN(MoveLoop);
};

}  // namespace simple_wm

#endif  // MASH_SIMPLE_WM_MOVE_LOOP_H_
