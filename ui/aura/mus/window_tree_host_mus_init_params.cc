// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/window_tree_host_mus_init_params.h"

#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace aura {

DisplayInitParams::DisplayInitParams() = default;

DisplayInitParams::~DisplayInitParams() = default;

WindowTreeHostMusInitParams::WindowTreeHostMusInitParams() = default;

WindowTreeHostMusInitParams::WindowTreeHostMusInitParams(
    WindowTreeHostMusInitParams&& other) = default;

WindowTreeHostMusInitParams::~WindowTreeHostMusInitParams() = default;

WindowTreeHostMusInitParams CreateInitParamsForTopLevel(
    WindowTreeClient* window_tree_client,
    std::map<std::string, std::vector<uint8_t>> properties) {
  WindowTreeHostMusInitParams params;
  params.window_tree_client = window_tree_client;
  params.display_id = display::Screen::GetScreen()->GetPrimaryDisplay().id();
  // Pass |properties| to CreateWindowPortForTopLevel() so that |properties|
  // are passed to the server *and* pass |properties| to the WindowTreeHostMus
  // constructor (above) which applies the properties to the Window. Some of the
  // properties may be server specific and not applied to the Window.
  params.window_port =
      static_cast<WindowTreeHostMusDelegate*>(window_tree_client)
          ->CreateWindowPortForTopLevel(&properties);
  params.properties = std::move(properties);
  return params;
}

}  // namespace aura
