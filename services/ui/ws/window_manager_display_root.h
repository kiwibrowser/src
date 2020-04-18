// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_MANAGER_DISPLAY_ROOT_H_
#define SERVICES_UI_WS_WINDOW_MANAGER_DISPLAY_ROOT_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"

namespace ui {
namespace ws {

class Display;
class ServerWindow;
class WindowManagerState;
class WindowServer;

// Owns the root window of a window manager for one display. Each window manager
// has one WindowManagerDisplayRoot for each Display. The root window is
// parented to the root of a Display.
class WindowManagerDisplayRoot {
 public:
  explicit WindowManagerDisplayRoot(Display* display);
  ~WindowManagerDisplayRoot();

  // NOTE: this window is not necessarily visible to the window manager. When
  // the display roots are automatically created this root is visible to the
  // window manager. When the display roots are not automatically created this
  // root has a single child that is created by the client. Use
  // GetClientVisibleRoot() to get the root that is visible to the client.
  ServerWindow* root() { return root_.get(); }
  const ServerWindow* root() const { return root_.get(); }

  // See root() for details of this. This returns null until the client creates
  // the root.
  ServerWindow* GetClientVisibleRoot() {
    return const_cast<ServerWindow*>(
        const_cast<const WindowManagerDisplayRoot*>(this)
            ->GetClientVisibleRoot());
  }
  const ServerWindow* GetClientVisibleRoot() const;

  Display* display() { return display_; }
  const Display* display() const { return display_; }

  WindowManagerState* window_manager_state() { return window_manager_state_; }
  const WindowManagerState* window_manager_state() const {
    return window_manager_state_;
  }

 private:
  friend class Display;
  friend class WindowManagerState;

  WindowServer* window_server();

  Display* display_;
  // Root ServerWindow of this WindowManagerDisplayRoot. |root_| has a parent,
  // the root ServerWindow of the Display.
  std::unique_ptr<ServerWindow> root_;
  WindowManagerState* window_manager_state_ = nullptr;
  viz::ParentLocalSurfaceIdAllocator allocator_;

  DISALLOW_COPY_AND_ASSIGN(WindowManagerDisplayRoot);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_MANAGER_DISPLAY_ROOT_H_
