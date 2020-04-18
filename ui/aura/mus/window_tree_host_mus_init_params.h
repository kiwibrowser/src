// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_WINDOW_TREE_HOST_MUS_INIT_PARAMS_H_
#define UI_AURA_MUS_WINDOW_TREE_HOST_MUS_INIT_PARAMS_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "components/viz/common/surfaces/frame_sink_id.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/aura_export.h"

namespace display {
class Display;
}

namespace aura {

class WindowPortMus;
class WindowTreeClient;

// Used for a WindowTreeHost that corresponds to a Display that is manually
// created by the window manager.
struct AURA_EXPORT DisplayInitParams {
  DisplayInitParams();
  ~DisplayInitParams();

  // The display, if not provided then the Display identified by
  // |WindowTreeHostMusInitParams::display_id| must be one of the Displays
  // contained in Screen.
  std::unique_ptr<display::Display> display;

  ui::mojom::WmViewportMetrics viewport_metrics;

  bool is_primary_display = false;

  // |mirrors| contains a list of physical displays presenting contents mirrored
  // from another physical display, or from part of a virtual unified display.
  // See |display::DisplayManager::software_mirroring_display_list_| for info.
  std::vector<display::Display> mirrors;
};

// Used to create a WindowTreeHostMus. The typical case is to use
// CreateInitParamsForTopLevel().
struct AURA_EXPORT WindowTreeHostMusInitParams {
  WindowTreeHostMusInitParams();
  WindowTreeHostMusInitParams(WindowTreeHostMusInitParams&& other);
  ~WindowTreeHostMusInitParams();

  // The WindowTreeClient; must be specified.
  WindowTreeClient* window_tree_client = nullptr;

  // Used to create the Window; must be specified.
  std::unique_ptr<WindowPortMus> window_port;

  // Properties to send to the server as well as to set on the Window.
  std::map<std::string, std::vector<uint8_t>> properties;

  viz::FrameSinkId frame_sink_id;

  // Id of the display the window should be created on.
  int64_t display_id = 0;

  // Used when the WindowTreeHostMus corresponds to a new display manually
  // created by the window manager.
  std::unique_ptr<DisplayInitParams> display_init_params;

  // Use classic IME (i.e. InputMethodChromeOS) instead of servicified IME
  // (i.e. InputMethodMus).
  bool use_classic_ime = false;
};

// Creates a WindowTreeHostMusInitParams that is used when creating a top-level
// window.
AURA_EXPORT WindowTreeHostMusInitParams CreateInitParamsForTopLevel(
    WindowTreeClient* window_tree_client,
    std::map<std::string, std::vector<uint8_t>> properties =
        std::map<std::string, std::vector<uint8_t>>());

}  // namespace aura

#endif  // UI_AURA_MUS_WINDOW_TREE_HOST_MUS_INIT_PARAMS_H_
