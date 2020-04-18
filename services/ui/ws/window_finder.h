// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_FINDER_H_
#define SERVICES_UI_WS_WINDOW_FINDER_H_

#include "components/viz/host/hit_test/hit_test_query.h"

namespace gfx {
class Point;
}

namespace ui {
namespace ws {

class ServerWindow;

struct DeepestWindow {
  ServerWindow* window = nullptr;
  bool in_non_client_area = false;
};

using EventSource = viz::EventSource;

// Finds the deepest visible child of |root| that should receive an event at
// |location|. |location| is in the coordinate space of |root_window|. The
// |window| field in the returned structure is set to the child window. If no
// valid child window is found |window| is set to null.
DeepestWindow FindDeepestVisibleWindowForLocation(ServerWindow* root_window,
                                                  EventSource event_source,
                                                  const gfx::Point& location);

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_FINDER_H_
