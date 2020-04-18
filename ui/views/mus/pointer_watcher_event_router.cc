// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/pointer_watcher_event_router.h"

#include "ui/aura/client/capture_client.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/views/pointer_watcher.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"

using display::Display;
using display::Screen;

namespace views {
namespace {

bool HasPointerWatcher(
    base::ObserverList<views::PointerWatcher, true>* observer_list) {
  return observer_list->begin() != observer_list->end();
}

}  // namespace

PointerWatcherEventRouter::PointerWatcherEventRouter(
    aura::WindowTreeClient* window_tree_client)
    : window_tree_client_(window_tree_client) {
  window_tree_client->AddObserver(this);
}

PointerWatcherEventRouter::~PointerWatcherEventRouter() {
  if (window_tree_client_)
    window_tree_client_->RemoveObserver(this);
}

void PointerWatcherEventRouter::AddPointerWatcher(PointerWatcher* watcher,
                                                  bool wants_moves) {
  // Pointer watchers cannot be added multiple times.
  DCHECK(!move_watchers_.HasObserver(watcher));
  DCHECK(!non_move_watchers_.HasObserver(watcher));
  if (wants_moves) {
    move_watchers_.AddObserver(watcher);
    if (event_types_ != EventTypes::MOVE_EVENTS) {
      event_types_ = EventTypes::MOVE_EVENTS;
      const bool wants_moves = true;
      window_tree_client_->StartPointerWatcher(wants_moves);
    }
  } else {
    non_move_watchers_.AddObserver(watcher);
    if (event_types_ == EventTypes::NONE) {
      event_types_ = EventTypes::NON_MOVE_EVENTS;
      const bool wants_moves = false;
      window_tree_client_->StartPointerWatcher(wants_moves);
    }
  }
}

void PointerWatcherEventRouter::RemovePointerWatcher(PointerWatcher* watcher) {
  if (non_move_watchers_.HasObserver(watcher)) {
    non_move_watchers_.RemoveObserver(watcher);
  } else {
    DCHECK(move_watchers_.HasObserver(watcher));
    move_watchers_.RemoveObserver(watcher);
  }
  const EventTypes types = DetermineEventTypes();
  if (types == event_types_)
    return;

  event_types_ = types;
  switch (types) {
    case EventTypes::NONE:
      window_tree_client_->StopPointerWatcher();
      break;
    case EventTypes::NON_MOVE_EVENTS:
      window_tree_client_->StartPointerWatcher(false);
      break;
    case EventTypes::MOVE_EVENTS:
      // It isn't possible to remove an observer and transition to wanting move
      // events. This could only happen if there is a bug in the add logic.
      NOTREACHED();
      break;
  }
}

void PointerWatcherEventRouter::OnPointerEventObserved(
    const ui::PointerEvent& event,
    int64_t display_id,
    aura::Window* target) {
  Widget* target_widget = nullptr;
  ui::PointerEvent updated_event(event);
  if (target) {
    aura::Window* window = target;
    while (window && !target_widget) {
      target_widget = Widget::GetWidgetForNativeView(window);
      if (!target_widget) {
        // Widget::GetWidgetForNativeView() uses NativeWidgetAura. Views with
        // aura-mus may also create DesktopNativeWidgetAura.
        DesktopNativeWidgetAura* desktop_native_widget_aura =
            DesktopNativeWidgetAura::ForWindow(target);
        if (desktop_native_widget_aura) {
          target_widget = static_cast<internal::NativeWidgetPrivate*>(
                              desktop_native_widget_aura)
                              ->GetWidget();
        }
      }
      window = window->parent();
    }
    if (target_widget) {
      gfx::Point widget_relative_location(event.location());
      aura::Window::ConvertPointToTarget(target, target_widget->GetNativeView(),
                                         &widget_relative_location);
      updated_event.set_location(widget_relative_location);
    }
  }

  // Compute screen coordinates via |display_id| because there may not be a
  // |target| that can be used to find a ScreenPositionClient.
  gfx::Point location_in_screen = event.location();
  Display display;
  if (Screen::GetScreen()->GetDisplayWithDisplayId(display_id, &display))
    location_in_screen.Offset(display.bounds().x(), display.bounds().y());

  for (PointerWatcher& observer : move_watchers_) {
    observer.OnPointerEventObserved(
        updated_event, location_in_screen,
        target_widget ? target_widget->GetNativeWindow() : target);
  }
  if (event.type() != ui::ET_POINTER_MOVED) {
    for (PointerWatcher& observer : non_move_watchers_) {
      observer.OnPointerEventObserved(
          updated_event, location_in_screen,
          target_widget ? target_widget->GetNativeWindow() : target);
    }
  }
}

void PointerWatcherEventRouter::AttachToCaptureClient(
    aura::client::CaptureClient* capture_client) {
  capture_client->AddObserver(this);
}

void PointerWatcherEventRouter::DetachFromCaptureClient(
    aura::client::CaptureClient* capture_client) {
  capture_client->RemoveObserver(this);
}

PointerWatcherEventRouter::EventTypes
PointerWatcherEventRouter::DetermineEventTypes() {
  if (HasPointerWatcher(&move_watchers_))
    return EventTypes::MOVE_EVENTS;

  if (HasPointerWatcher(&non_move_watchers_))
    return EventTypes::NON_MOVE_EVENTS;

  return EventTypes::NONE;
}

void PointerWatcherEventRouter::OnCaptureChanged(aura::Window* lost_capture,
                                                 aura::Window* gained_capture) {
  const ui::MouseEvent mouse_event(ui::ET_MOUSE_CAPTURE_CHANGED, gfx::Point(),
                                   gfx::Point(), ui::EventTimeForNow(), 0, 0);
  const ui::PointerEvent event(mouse_event);
  gfx::Point location_in_screen = Screen::GetScreen()->GetCursorScreenPoint();
  for (PointerWatcher& observer : move_watchers_)
    observer.OnPointerEventObserved(event, location_in_screen, nullptr);
  for (PointerWatcher& observer : non_move_watchers_)
    observer.OnPointerEventObserved(event, location_in_screen, nullptr);
}

void PointerWatcherEventRouter::OnWillDestroyClient(
    aura::WindowTreeClient* client) {
  // We expect that all observers have been removed by this time.
  DCHECK_EQ(event_types_, EventTypes::NONE);
  DCHECK_EQ(client, window_tree_client_);
  window_tree_client_->RemoveObserver(this);
  window_tree_client_ = nullptr;
}

}  // namespace views
