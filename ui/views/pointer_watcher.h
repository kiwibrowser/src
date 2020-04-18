// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_POINTER_WATCHER_H_
#define UI_VIEWS_POINTER_WATCHER_H_

#include "base/macros.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/views_export.h"

namespace gfx {
class Point;
}

namespace ui {
class PointerEvent;
}

namespace views {

// When a PointerWatcher is added the types of events desired is specified by
// way of PointerWatcherEventTypes.
enum class PointerWatcherEventTypes {
  // The PointerWatcher is interested in press, release, capture and mouse
  // wheel.
  BASIC,
  // The PointerWatcher is interested in BASIC events, as well as move
  // events.
  MOVES,
  // The PointerWatcher is interested in MOVE events, as well as drag
  // events.
  DRAGS
};

// An interface for read-only observation of pointer events (in particular, the
// events cannot be marked as handled). Only certain event types are supported.
// The |target| is the native window that will receive the event, if any.
// To reduce IPC traffic from the window server, move events are not provided
// unless the app specifically requests them.
// NOTE: On mus this allows observation of events outside of windows owned
// by the current process, in which case the |target| will be null. On mus
// event.target() is always null.
// NOTE: Mouse capture change events are sent through OnPointerEventObserved and
// its |target| is always null.
// NOTE: |target| may or may not have an associated views::Widget that may not
// be a top-level Widget.
class VIEWS_EXPORT PointerWatcher {
 public:
  PointerWatcher() {}

  virtual void OnPointerEventObserved(const ui::PointerEvent& event,
                                      const gfx::Point& location_in_screen,
                                      gfx::NativeView target) = 0;

 protected:
  virtual ~PointerWatcher() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PointerWatcher);
};

}  // namespace views

#endif  // UI_VIEWS_POINTER_WATCHER_H_
