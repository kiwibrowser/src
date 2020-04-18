// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_WINDOW_EVENT_DISPATCHER_OBSERVER_H_
#define UI_AURA_WINDOW_EVENT_DISPATCHER_OBSERVER_H_

#include "ui/aura/aura_export.h"

namespace ui {
class Event;
}

namespace aura {

class WindowEventDispatcher;

// WindowEventDispatcherObservers are added to Env and observe *all*
// WindowEventDispatchers.
class AURA_EXPORT WindowEventDispatcherObserver {
 public:
  // Called when WindowEventDispatcher starts processing an event.
  //
  // NOTE: this function is called *after* the location has been transformed
  // (assuming the event is a located event). In other words, the coordinates
  // are DIPs when this is called.
  virtual void OnWindowEventDispatcherStartedProcessing(
      WindowEventDispatcher* dispatcher,
      const ui::Event& event) {}

  // Called when WindowEventDispatcher finishes processing an event.
  virtual void OnWindowEventDispatcherFinishedProcessingEvent(
      WindowEventDispatcher* dispatcher) {}

 protected:
  virtual ~WindowEventDispatcherObserver() {}
};

}  // namespace aura

#endif  // UI_AURA_WINDOW_EVENT_DISPATCHER_OBSERVER_H_
