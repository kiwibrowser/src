// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/pointer_watcher_adapter_classic.h"

#include "ash/shell.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/window.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/pointer_watcher.h"
#include "ui/views/widget/widget.h"

namespace ash {

PointerWatcherAdapterClassic::PointerWatcherAdapterClassic() {
  Shell::Get()->AddPreTargetHandler(this);
}

PointerWatcherAdapterClassic::~PointerWatcherAdapterClassic() {
  Shell::Get()->RemovePreTargetHandler(this);
}

void PointerWatcherAdapterClassic::AddPointerWatcher(
    views::PointerWatcher* watcher,
    views::PointerWatcherEventTypes events) {
  // We only allow a watcher to be added once. That is, we don't consider
  // the pair of |watcher| and |events| unique, just |watcher|.
  DCHECK(!non_move_watchers_.HasObserver(watcher));
  DCHECK(!move_watchers_.HasObserver(watcher));
  DCHECK(!drag_watchers_.HasObserver(watcher));
  if (events == views::PointerWatcherEventTypes::DRAGS)
    drag_watchers_.AddObserver(watcher);
  else if (events == views::PointerWatcherEventTypes::MOVES)
    move_watchers_.AddObserver(watcher);
  else
    non_move_watchers_.AddObserver(watcher);
}

void PointerWatcherAdapterClassic::RemovePointerWatcher(
    views::PointerWatcher* watcher) {
  non_move_watchers_.RemoveObserver(watcher);
  move_watchers_.RemoveObserver(watcher);
  drag_watchers_.RemoveObserver(watcher);
}

void PointerWatcherAdapterClassic::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() != ui::ET_MOUSE_PRESSED &&
      event->type() != ui::ET_MOUSE_RELEASED &&
      event->type() != ui::ET_MOUSE_MOVED &&
      event->type() != ui::ET_MOUSEWHEEL &&
      event->type() != ui::ET_MOUSE_CAPTURE_CHANGED &&
      event->type() != ui::ET_MOUSE_DRAGGED)
    return;

  DCHECK(ui::PointerEvent::CanConvertFrom(*event));
  NotifyWatchers(ui::PointerEvent(*event), *event);
}

void PointerWatcherAdapterClassic::OnTouchEvent(ui::TouchEvent* event) {
  if (event->type() != ui::ET_TOUCH_PRESSED &&
      event->type() != ui::ET_TOUCH_RELEASED &&
      event->type() != ui::ET_TOUCH_MOVED)
    return;

  DCHECK(ui::PointerEvent::CanConvertFrom(*event));
  NotifyWatchers(ui::PointerEvent(*event), *event);
}

gfx::Point PointerWatcherAdapterClassic::GetLocationInScreen(
    const ui::LocatedEvent& event) const {
  gfx::Point location_in_screen;
  if (event.type() == ui::ET_MOUSE_CAPTURE_CHANGED) {
    location_in_screen = display::Screen::GetScreen()->GetCursorScreenPoint();
  } else {
    aura::Window* target = static_cast<aura::Window*>(event.target());
    location_in_screen = event.location();
    aura::client::GetScreenPositionClient(target->GetRootWindow())
        ->ConvertPointToScreen(target, &location_in_screen);
  }
  return location_in_screen;
}

void PointerWatcherAdapterClassic::NotifyWatchers(
    const ui::PointerEvent& event,
    const ui::LocatedEvent& original_event) {
  const gfx::Point screen_location(GetLocationInScreen(original_event));
  aura::Window* target = static_cast<aura::Window*>(original_event.target());
  for (auto& observer : drag_watchers_)
    observer.OnPointerEventObserved(event, screen_location, target);
  if (original_event.type() != ui::ET_TOUCH_MOVED &&
      original_event.type() != ui::ET_MOUSE_DRAGGED) {
    for (auto& observer : move_watchers_)
      observer.OnPointerEventObserved(event, screen_location, target);
  }
  if (event.type() != ui::ET_POINTER_MOVED) {
    for (auto& observer : non_move_watchers_)
      observer.OnPointerEventObserved(event, screen_location, target);
  }
}

}  // namespace ash
