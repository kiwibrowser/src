// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/cursor_state.h"

#include "services/ui/ws/display.h"
#include "services/ui/ws/display_manager.h"
#include "ui/base/cursor/cursor.h"

namespace ui {
namespace ws {

class CursorState::StateSnapshot {
 public:
  StateSnapshot() = default;
  StateSnapshot(const StateSnapshot& rhs) = default;
  ~StateSnapshot() = default;

  const base::Optional<ui::CursorData>& global_override_cursor() const {
    return global_override_cursor_;
  }
  void SetGlobalOverrideCursor(const base::Optional<ui::CursorData>& cursor) {
    global_override_cursor_ = cursor;
  }

  const ui::CursorData& cursor_data() const { return cursor_data_; }
  void SetCursorData(const ui::CursorData& data) { cursor_data_ = data; }

  bool visible() const { return visible_; }
  void set_visible(bool visible) { visible_ = visible; }

  ui::CursorSize cursor_size() const { return cursor_size_; }
  void set_cursor_size(ui::CursorSize cursor_size) {
    cursor_size_ = cursor_size;
  }

  bool cursor_touch_visible() const { return cursor_touch_visible_; }
  void set_cursor_touch_visible(bool enabled) {
    cursor_touch_visible_ = enabled;
  }

 private:
  // An optional cursor set by the window manager which overrides per-window
  // requests.
  base::Optional<ui::CursorData> global_override_cursor_;

  // The last cursor set. Used to track whether we need to change the cursor.
  ui::CursorData cursor_data_ = ui::CursorData(ui::CursorType::kNull);

  // Which cursor set to use.
  ui::CursorSize cursor_size_ = CursorSize::kNormal;

  // Whether the cursor is visible.
  bool visible_ = true;

  // An extra visibility check separate from user control which shows/hides
  // based on whether the last event was a touch or a mouse event.
  bool cursor_touch_visible_ = true;
};

CursorState::CursorState(DisplayManager* display_manager,
                         CursorStateDelegate* delegate)
    : display_manager_(display_manager),
      delegate_(delegate),
      current_state_(std::make_unique<StateSnapshot>()),
      state_on_unlock_(std::make_unique<StateSnapshot>()) {}

CursorState::~CursorState() {}

void CursorState::SetCurrentWindowCursor(const ui::CursorData& cursor) {
  if (!state_on_unlock_->cursor_data().IsSameAs(cursor))
    state_on_unlock_->SetCursorData(cursor);

  if (cursor_lock_count_ == 0 &&
      !current_state_->cursor_data().IsSameAs(cursor)) {
    current_state_->SetCursorData(cursor);
    SetPlatformCursor();
  }
}

void CursorState::LockCursor() {
  cursor_lock_count_++;
}

void CursorState::UnlockCursor() {
  cursor_lock_count_--;
  DCHECK_GE(cursor_lock_count_, 0);
  if (cursor_lock_count_ > 0)
    return;

  if (current_state_->cursor_touch_visible() !=
      state_on_unlock_->cursor_touch_visible()) {
    NotifyCursorTouchVisibleChanged(state_on_unlock_->cursor_touch_visible());
  }

  *current_state_ = *state_on_unlock_;
  SetPlatformCursorSize();
  SetPlatformCursor();
}

void CursorState::SetCursorVisible(bool visible) {
  state_on_unlock_->set_visible(visible);
  if (cursor_lock_count_ == 0 &&
      current_state_->visible() != state_on_unlock_->visible()) {
    current_state_->set_visible(visible);
    SetPlatformCursor();
  }
}

void CursorState::SetGlobalOverrideCursor(
    const base::Optional<ui::CursorData>& cursor) {
  state_on_unlock_->SetGlobalOverrideCursor(cursor);
  if (cursor_lock_count_ == 0) {
    current_state_->SetGlobalOverrideCursor(cursor);
    SetPlatformCursor();
  }
}

void CursorState::SetCursorSize(ui::CursorSize cursor_size) {
  state_on_unlock_->set_cursor_size(cursor_size);
  if (cursor_lock_count_ == 0 &&
      current_state_->cursor_size() != state_on_unlock_->cursor_size()) {
    current_state_->set_cursor_size(cursor_size);
    SetPlatformCursorSize();
    SetPlatformCursor();
  }
}

void CursorState::SetCursorTouchVisible(bool enabled) {
  state_on_unlock_->set_cursor_touch_visible(enabled);
  if (cursor_lock_count_ == 0 && current_state_->cursor_touch_visible() !=
                                     state_on_unlock_->cursor_touch_visible()) {
    current_state_->set_cursor_touch_visible(enabled);
    NotifyCursorTouchVisibleChanged(enabled);
    SetPlatformCursor();
  }
}

void CursorState::NotifyCursorTouchVisibleChanged(bool enabled) {
  delegate_->OnCursorTouchVisibleChanged(enabled);
}

void CursorState::SetPlatformCursorSize() {
  DisplayManager* manager = display_manager_;
  for (Display* display : manager->displays())
    display->SetNativeCursorSize(current_state_->cursor_size());
}

void CursorState::SetPlatformCursor() {
  DisplayManager* manager = display_manager_;
  auto set_on_all = [manager](const ui::CursorData& cursor) {
    for (Display* display : manager->displays())
      display->SetNativeCursor(cursor);
  };

  if (current_state_->visible() && current_state_->cursor_touch_visible()) {
    if (current_state_->global_override_cursor().has_value())
      set_on_all(current_state_->global_override_cursor().value());
    else
      set_on_all(current_state_->cursor_data());
  } else {
    set_on_all(ui::CursorData(ui::CursorType::kNone));
  }
}

}  // namespace ws
}  // namespace ui
