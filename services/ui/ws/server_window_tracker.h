// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_SERVER_WINDOW_TRACKER_H_
#define SERVICES_UI_WS_SERVER_WINDOW_TRACKER_H_

#include <stdint.h>
#include <set>

#include "base/macros.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/server_window_observer.h"
#include "ui/base/window_tracker_template.h"

namespace ui {
namespace ws {

using ServerWindowTracker =
    ui::WindowTrackerTemplate<ServerWindow, ServerWindowObserver>;

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_SERVER_WINDOW_TRACKER_H_
