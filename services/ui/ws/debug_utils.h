// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_DEBUG_UTILS_H_
#define SERVICES_UI_WS_DEBUG_UTILS_H_

#include <string>

namespace ui {
namespace ws {

class ServerWindow;

// Returns a debug string for |window|, returns "null" if |window| is null.
std::string DebugWindowId(const ServerWindow* window);

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_DEBUG_UTILS_H_
