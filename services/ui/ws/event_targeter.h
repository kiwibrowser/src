// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_EVENT_TARGETER_H_
#define SERVICES_UI_WS_EVENT_TARGETER_H_

#include <stdint.h>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "services/ui/ws/window_finder.h"
#include "ui/gfx/geometry/point.h"

namespace ui {
namespace ws {

class EventTargeterDelegate;
struct EventLocation;

namespace test {
class EventTargeterTestApi;
}

using HitTestCallback =
    base::OnceCallback<void(const EventLocation&, const DeepestWindow&)>;

// Finds the target window for a location.
class EventTargeter {
 public:
  explicit EventTargeter(EventTargeterDelegate* event_targeter_delegate);
  ~EventTargeter();

  // Calls WindowFinder to find the target for |display_location|. |callback| is
  // called with the found target.
  void FindTargetForLocation(EventSource event_source,
                             const EventLocation& event_location,
                             HitTestCallback callback);

 private:
  friend class test::EventTargeterTestApi;

  void FindTargetForLocationNow(EventSource event_source,
                                const EventLocation& display_location,
                                HitTestCallback callback);

  EventTargeterDelegate* event_targeter_delegate_;

  base::WeakPtrFactory<EventTargeter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(EventTargeter);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_EVENT_TARGETER_H_
