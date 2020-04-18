// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/debug.h"

#include "ash/shell.h"
#include "cc/debug/layer_tree_debug_state.h"
#include "ui/aura/window_tree_host.h"
#include "ui/compositor/compositor.h"

namespace ash {
namespace debug {

void ToggleShowDebugBorders() {
  aura::Window::Windows root_windows = Shell::Get()->GetAllRootWindows();
  std::unique_ptr<cc::DebugBorderTypes> value;
  for (aura::Window::Windows::iterator it = root_windows.begin();
       it != root_windows.end(); ++it) {
    ui::Compositor* compositor = (*it)->GetHost()->compositor();
    cc::LayerTreeDebugState state = compositor->GetLayerTreeDebugState();
    if (!value.get())
      value.reset(new cc::DebugBorderTypes(state.show_debug_borders.flip()));
    state.show_debug_borders = *value.get();
    compositor->SetLayerTreeDebugState(state);
  }
}

void ToggleShowFpsCounter() {
  aura::Window::Windows root_windows = Shell::Get()->GetAllRootWindows();
  std::unique_ptr<bool> value;
  for (aura::Window::Windows::iterator it = root_windows.begin();
       it != root_windows.end(); ++it) {
    ui::Compositor* compositor = (*it)->GetHost()->compositor();
    cc::LayerTreeDebugState state = compositor->GetLayerTreeDebugState();
    if (!value.get())
      value.reset(new bool(!state.show_fps_counter));
    state.show_fps_counter = *value.get();
    compositor->SetLayerTreeDebugState(state);
  }
}

void ToggleShowPaintRects() {
  aura::Window::Windows root_windows = Shell::Get()->GetAllRootWindows();
  std::unique_ptr<bool> value;
  for (aura::Window::Windows::iterator it = root_windows.begin();
       it != root_windows.end(); ++it) {
    ui::Compositor* compositor = (*it)->GetHost()->compositor();
    cc::LayerTreeDebugState state = compositor->GetLayerTreeDebugState();
    if (!value.get())
      value.reset(new bool(!state.show_paint_rects));
    state.show_paint_rects = *value.get();
    compositor->SetLayerTreeDebugState(state);
  }
}

}  // namespace debug
}  // namespace ash
