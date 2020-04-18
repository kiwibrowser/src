// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mash/simple_wm/move_loop.h"

#include "base/auto_reset.h"
#include "base/memory/ptr_util.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/base/ui_base_types.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/rect.h"

namespace simple_wm {
namespace {

int MouseOnlyEventFlags(int flags) {
  return flags & (ui::EF_LEFT_MOUSE_BUTTON | ui::EF_MIDDLE_MOUSE_BUTTON |
                  ui::EF_RIGHT_MOUSE_BUTTON);
}

}  // namespace

MoveLoop::~MoveLoop() {
  if (target_)
    target_->RemoveObserver(this);
}

// static
std::unique_ptr<MoveLoop> MoveLoop::Create(aura::Window* target,
                                           int ht_location,
                                           const ui::PointerEvent& event) {
  DCHECK_EQ(event.type(), ui::ET_POINTER_DOWN);
  // Start a move on left mouse, or any other type of pointer.
  if (event.IsMousePointerEvent() &&
      MouseOnlyEventFlags(event.flags()) != ui::EF_LEFT_MOUSE_BUTTON) {
    return nullptr;
  }

  Type type;
  HorizontalLocation h_loc;
  VerticalLocation v_loc;
  if (!DetermineType(ht_location, &type, &h_loc, &v_loc))
    return nullptr;

  return base::WrapUnique(new MoveLoop(target, event, type, h_loc, v_loc));
}

MoveLoop::MoveResult MoveLoop::Move(const ui::PointerEvent& event) {
  switch (event.type()) {
    case ui::ET_POINTER_CANCELLED:
      if (event.pointer_details().id == pointer_id_) {
        if (target_)
          Revert();
        return MoveResult::DONE;
      }
      return MoveResult::CONTINUE;

    case ui::ET_POINTER_MOVED:
      if (target_ && event.pointer_details().id == pointer_id_)
        MoveImpl(event);
      return MoveResult::CONTINUE;

    case ui::ET_POINTER_UP:
      if (event.pointer_details().id == pointer_id_) {
        // TODO(sky): need to support changed_flags.
        if (target_)
          MoveImpl(event);
        return MoveResult::DONE;
      }
      return MoveResult::CONTINUE;

    default:
      break;
  }
  return MoveResult::CONTINUE;
}

void MoveLoop::Revert() {
  if (!target_)
    return;

  base::AutoReset<bool> resetter(&changing_bounds_, true);
  target_->SetBounds(initial_window_bounds_);
}

MoveLoop::MoveLoop(aura::Window* target,
                   const ui::PointerEvent& event,
                   Type type,
                   HorizontalLocation h_loc,
                   VerticalLocation v_loc)
    : target_(target),
      type_(type),
      h_loc_(h_loc),
      v_loc_(v_loc),
      pointer_id_(event.pointer_details().id),
      initial_event_screen_location_(event.root_location()),
      initial_window_bounds_(target->bounds()),
      initial_user_set_bounds_(target->bounds()),
      changing_bounds_(false) {
  target->AddObserver(this);
}

// static
bool MoveLoop::DetermineType(int ht_location,
                             Type* type,
                             HorizontalLocation* h_loc,
                             VerticalLocation* v_loc) {
  *h_loc = HorizontalLocation::OTHER;
  *v_loc = VerticalLocation::OTHER;
  switch (ht_location) {
    case HTCAPTION:
      *type = Type::MOVE;
      *v_loc = VerticalLocation::TOP;
      return true;
    case HTTOPLEFT:
      *type = Type::RESIZE;
      *v_loc = VerticalLocation::TOP;
      *h_loc = HorizontalLocation::LEFT;
      return true;
    case HTTOP:
      *type = Type::RESIZE;
      *v_loc = VerticalLocation::TOP;
      return true;
    case HTTOPRIGHT:
      *type = Type::RESIZE;
      *v_loc = VerticalLocation::TOP;
      *h_loc = HorizontalLocation::RIGHT;
      return true;
    case HTRIGHT:
      *type = Type::RESIZE;
      *h_loc = HorizontalLocation::RIGHT;
      return true;
    case HTBOTTOMRIGHT:
      *type = Type::RESIZE;
      *v_loc = VerticalLocation::BOTTOM;
      *h_loc = HorizontalLocation::RIGHT;
      return true;
    case HTBOTTOM:
      *type = Type::RESIZE;
      *v_loc = VerticalLocation::BOTTOM;
      return true;
    case HTBOTTOMLEFT:
      *type = Type::RESIZE;
      *v_loc = VerticalLocation::BOTTOM;
      *h_loc = HorizontalLocation::LEFT;
      return true;
    case HTLEFT:
      *type = Type::RESIZE;
      *h_loc = HorizontalLocation::LEFT;
      return true;
    default:
      break;
  }
  return false;
}

void MoveLoop::MoveImpl(const ui::PointerEvent& event) {
  ui::WindowShowState show_state =
      target_->GetProperty(aura::client::kShowStateKey);
  // TODO(beng): figure out if there might not be another place to put this,
  //             perhaps prior to move loop creation.
  if (show_state == ui::SHOW_STATE_MAXIMIZED) {
    base::AutoReset<bool> resetter(&changing_bounds_, true);
    target_->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
    gfx::Rect restored_bounds =
        *target_->GetProperty(aura::client::kRestoreBoundsKey);
    // TODO(beng): Not just enough to adjust width and height, probably also
    //             need to take some action to recenter the window relative to
    //             the pointer position within the titlebar.
    initial_window_bounds_.set_width(restored_bounds.width());
    initial_window_bounds_.set_height(restored_bounds.height());
  }
  const gfx::Vector2d delta =
      event.root_location() - initial_event_screen_location_;
  const gfx::Rect new_bounds(DetermineBoundsFromDelta(delta));
  base::AutoReset<bool> resetter(&changing_bounds_, true);
  target_->SetBounds(new_bounds);
}

void MoveLoop::Cancel() {
  target_->RemoveObserver(this);
  target_ = nullptr;
}

gfx::Rect MoveLoop::DetermineBoundsFromDelta(const gfx::Vector2d& delta) {
  if (type_ == Type::MOVE) {
    return gfx::Rect(initial_window_bounds_.origin() + delta,
                     initial_window_bounds_.size());
  }

  // TODO(sky): support better min sizes, make sure doesn't get bigger than
  // screen and max. Also make sure keep some portion on screen.
  gfx::Rect bounds(initial_window_bounds_);
  if (h_loc_ == HorizontalLocation::LEFT) {
    const int x = std::min(bounds.right() - 1, bounds.x() + delta.x());
    const int width = bounds.right() - x;
    bounds.set_x(x);
    bounds.set_width(width);
  } else if (h_loc_ == HorizontalLocation::RIGHT) {
    bounds.set_width(std::max(1, bounds.width() + delta.x()));
  }

  if (v_loc_ == VerticalLocation::TOP) {
    const int y = std::min(bounds.bottom() - 1, bounds.y() + delta.y());
    const int height = bounds.bottom() - y;
    bounds.set_y(y);
    bounds.set_height(height);
  } else if (v_loc_ == VerticalLocation::BOTTOM) {
    bounds.set_height(std::max(1, bounds.height() + delta.y()));
  }

  return bounds;
}

void MoveLoop::OnWindowHierarchyChanged(const HierarchyChangeParams& params) {
  if (params.target == target_)
    Cancel();
}

void MoveLoop::OnWindowBoundsChanged(aura::Window* window,
                                     const gfx::Rect& old_bounds,
                                     const gfx::Rect& new_bounds,
                                     ui::PropertyChangeReason reason) {
  DCHECK_EQ(window, target_);
  if (!changing_bounds_)
    Cancel();
}

void MoveLoop::OnWindowVisibilityChanged(aura::Window* window, bool visible) {
  DCHECK_EQ(window, target_);
  Cancel();
}

}  // namespace simple_wm
