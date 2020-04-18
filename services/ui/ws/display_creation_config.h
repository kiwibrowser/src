// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_DISPLAY_CREATION_CONFIG_H_
#define SERVICES_UI_WS_DISPLAY_CREATION_CONFIG_H_

namespace ui {
namespace ws {

enum class DisplayCreationConfig {
  // Initial state.
  UNKNOWN,

  // Displays are created automatically based on the system.
  AUTOMATIC,

  // Used when the window manager manually creates displays.
  MANUAL,
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_DISPLAY_CREATION_CONFIG_H_
