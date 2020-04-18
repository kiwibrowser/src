// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_SERVER_WINDOW_OBSERVER_H_
#define SERVICES_UI_WS_SERVER_WINDOW_OBSERVER_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "services/ui/public/interfaces/cursor/cursor.mojom.h"
#include "services/ui/public/interfaces/mus_constants.mojom.h"
#include "ui/base/ui_base_types.h"

namespace gfx {
class Insets;
class Rect;
class Transform;
}

namespace ui {

struct TextInputState;

namespace ws {

class ServerWindow;

// TODO(sky): rename to OnDid and OnWill everywhere.
class ServerWindowObserver {
 public:
  // Invoked when a window is about to be destroyed; before any of the children
  // have been removed and before the window has been removed from its parent.
  virtual void OnWindowDestroying(ServerWindow* window) {}

  // Invoked at the end of the window's destructor (after it has been removed
  // from the hierarchy.
  virtual void OnWindowDestroyed(ServerWindow* window) {}

  virtual void OnWillChangeWindowHierarchy(ServerWindow* window,
                                           ServerWindow* new_parent,
                                           ServerWindow* old_parent) {}

  virtual void OnWindowHierarchyChanged(ServerWindow* window,
                                        ServerWindow* new_parent,
                                        ServerWindow* old_parent) {}

  virtual void OnWindowBoundsChanged(ServerWindow* window,
                                     const gfx::Rect& old_bounds,
                                     const gfx::Rect& new_bounds) {}

  virtual void OnWindowTransformChanged(ServerWindow* window,
                                        const gfx::Transform& old_transform,
                                        const gfx::Transform& new_transform) {}

  virtual void OnWindowClientAreaChanged(
      ServerWindow* window,
      const gfx::Insets& new_client_area,
      const std::vector<gfx::Rect>& new_additional_client_areas) {}

  virtual void OnWindowReordered(ServerWindow* window,
                                 ServerWindow* relative,
                                 mojom::OrderDirection direction) {}

  virtual void OnWillChangeWindowVisibility(ServerWindow* window) {}
  virtual void OnWindowVisibilityChanged(ServerWindow* window) {}
  virtual void OnWindowOpacityChanged(ServerWindow* window,
                                      float old_opacity,
                                      float new_opacity) {}

  virtual void OnWindowCursorChanged(ServerWindow* window,
                                     const ui::CursorData& cursor_data) {}
  virtual void OnWindowNonClientCursorChanged(
      ServerWindow* window,
      const ui::CursorData& cursor_data) {}

  virtual void OnWindowTextInputStateChanged(ServerWindow* window,
                                             const ui::TextInputState& state) {}

  virtual void OnWindowSharedPropertyChanged(
      ServerWindow* window,
      const std::string& name,
      const std::vector<uint8_t>* new_data) {}

  // Called when the window is no longer an embed root.
  virtual void OnWindowEmbeddedAppDisconnected(ServerWindow* window) {}

  // Called when a transient child is added to |window|.
  virtual void OnTransientWindowAdded(ServerWindow* window,
                                      ServerWindow* transient_child) {}

  // Called when a transient child is removed from |window|.
  virtual void OnTransientWindowRemoved(ServerWindow* window,
                                        ServerWindow* transient_child) {}

  virtual void OnWindowModalTypeChanged(ServerWindow* window,
                                        ModalType old_modal_type) {}

 protected:
  virtual ~ServerWindowObserver() {}
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_SERVER_WINDOW_OBSERVER_H_
