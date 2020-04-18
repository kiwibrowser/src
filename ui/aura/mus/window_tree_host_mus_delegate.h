// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_WINDOW_TREE_HOST_MUS_DELEGATE_H_
#define UI_AURA_MUS_WINDOW_TREE_HOST_MUS_DELEGATE_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "ui/aura/aura_export.h"

namespace gfx {
class Rect;
}

namespace aura {

class WindowPortMus;
class WindowTreeHostMus;

class AURA_EXPORT WindowTreeHostMusDelegate {
 public:
  // Called when the bounds of a WindowTreeHostMus is about to change.
  // |bounds| is the bounds supplied to WindowTreeHostMus::SetBounds() and is
  // in screen pixel coordinates.
  virtual void OnWindowTreeHostBoundsWillChange(
      WindowTreeHostMus* window_tree_host,
      const gfx::Rect& bounds) = 0;

  // Called when the client area of a WindowTreeHostMus is about to change.
  virtual void OnWindowTreeHostClientAreaWillChange(
      WindowTreeHostMus* window_tree_host,
      const gfx::Insets& client_area,
      const std::vector<gfx::Rect>& additional_client_areas) = 0;

  // Called when the opacity is changed client side.
  virtual void OnWindowTreeHostSetOpacity(WindowTreeHostMus* window_tree_host,
                                          float opacity) = 0;

  // Called to clear the focus of the current window.
  virtual void OnWindowTreeHostDeactivateWindow(
      WindowTreeHostMus* window_tree_host) = 0;

  // Called to stack the native window above the native window of |window|.
  virtual void OnWindowTreeHostStackAbove(
      WindowTreeHostMus* window_tree_host,
      Window* window) = 0;

  // Called to stack the native window above other native windows.
  virtual void OnWindowTreeHostStackAtTop(
      WindowTreeHostMus* window_tree_host) = 0;

  // Called to signal to the window manager to take an action.
  virtual void OnWindowTreeHostPerformWmAction(
      WindowTreeHostMus* window_tree_host,
      const std::string& action) = 0;

  // Called to start a move loop, where the window manager will take over
  // moving a window during a drag.
  virtual void OnWindowTreeHostPerformWindowMove(
      WindowTreeHostMus* window_tree_host,
      ui::mojom::MoveLoopSource mus_source,
      const gfx::Point& cursor_location,
      const base::Callback<void(bool)>& callback) = 0;

  // Called to cancel a move loop.
  virtual void OnWindowTreeHostCancelWindowMove(
      WindowTreeHostMus* window_tree_host) = 0;

  // Called to move the location of the cursor.
  virtual void OnWindowTreeHostMoveCursorToDisplayLocation(
      const gfx::Point& location_in_pixels,
      int64_t display_id) = 0;

  // Called to confine the cursor to a set of bounds in pixels. Only available
  // to the window manager.
  virtual void OnWindowTreeHostConfineCursorToBounds(
      const gfx::Rect& bounds_in_pixels,
      int64_t display_id) = 0;

  // Called when a WindowTreeHostMus is created without a WindowPort.
  // TODO: this should take an unordered_map, see http://crbug.com/670515.
  virtual std::unique_ptr<WindowPortMus> CreateWindowPortForTopLevel(
      const std::map<std::string, std::vector<uint8_t>>* properties) = 0;

  // Called from WindowTreeHostMus's constructor once the Window has been
  // created.
  virtual void OnWindowTreeHostCreated(WindowTreeHostMus* window_tree_host) = 0;

 protected:
  virtual ~WindowTreeHostMusDelegate() {}
};

}  // namespace aura

#endif  // UI_AURA_MUS_WINDOW_TREE_HOST_MUS_DELEGATE_H_
