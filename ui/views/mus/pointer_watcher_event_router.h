// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_POINTER_WATCHER_EVENT_ROUTER_H_
#define UI_VIEWS_MUS_POINTER_WATCHER_EVENT_ROUTER_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/observer_list.h"
#include "ui/aura/client/capture_client_observer.h"
#include "ui/aura/mus/window_tree_client_observer.h"
#include "ui/views/mus/mus_export.h"

namespace aura {
class WindowTreeClient;

namespace client {
class CaptureClient;
}
}

namespace ui {
class PointerEvent;
}

namespace views {

class PointerWatcher;
class PointerWatcherEventRouterTest;

// PointerWatcherEventRouter is responsible for maintaining the list of
// PointerWatchers and notifying appropriately. It is expected the owner of
// PointerWatcherEventRouter is a WindowTreeClientDelegate and calls
// OnPointerEventObserved().
class VIEWS_MUS_EXPORT PointerWatcherEventRouter
    : public aura::WindowTreeClientObserver,
      public aura::client::CaptureClientObserver {
 public:
  // Public solely for tests.
  enum EventTypes {
    // No PointerWatchers have been added.
    NONE,

    // Used when the only PointerWatchers added do not want moves.
    NON_MOVE_EVENTS,

    // Used when at least one PointerWatcher has been added that wants moves.
    MOVE_EVENTS,
  };

  explicit PointerWatcherEventRouter(
      aura::WindowTreeClient* window_tree_client);
  ~PointerWatcherEventRouter() override;

  void AddPointerWatcher(PointerWatcher* watcher, bool wants_moves);
  void RemovePointerWatcher(PointerWatcher* watcher);

  // Called by WindowTreeClientDelegate to notify PointerWatchers appropriately.
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              int64_t display_id,
                              aura::Window* target);

  // Called when the |capture_client| has been set or will be unset.
  void AttachToCaptureClient(aura::client::CaptureClient* capture_client);
  void DetachFromCaptureClient(aura::client::CaptureClient* capture_client);

 private:
  friend class PointerWatcherEventRouterTest;

  // Determines EventTypes based on the number and type of PointerWatchers.
  EventTypes DetermineEventTypes();

  // aura::WindowTreeClientObserver:
  void OnWillDestroyClient(aura::WindowTreeClient* client) override;

  // aura::client::CaptureClientObserver:
  void OnCaptureChanged(aura::Window* lost_capture,
                        aura::Window* gained_capture) override;

  aura::WindowTreeClient* window_tree_client_;
  // The true parameter to ObserverList indicates the list must be empty on
  // destruction. Two sets of observers are maintained, one for observers not
  // needing moves |non_move_watchers_| and |move_watchers_| for those
  // observers wanting moves too.
  base::ObserverList<views::PointerWatcher, true> non_move_watchers_;
  base::ObserverList<views::PointerWatcher, true> move_watchers_;

  EventTypes event_types_ = EventTypes::NONE;

  DISALLOW_COPY_AND_ASSIGN(PointerWatcherEventRouter);
};

}  // namespace views

#endif  // UI_VIEWS_MUS_POINTER_WATCHER_EVENT_ROUTER_H_
