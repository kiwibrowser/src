// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_DISPLAY_H_
#define SERVICES_UI_WS_DISPLAY_H_

#include <stdint.h>

#include <memory>
#include <queue>
#include <set>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "services/ui/common/types.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "services/ui/public/interfaces/window_tree_host.mojom.h"
#include "services/ui/ws/focus_controller_observer.h"
#include "services/ui/ws/platform_display.h"
#include "services/ui/ws/platform_display_delegate.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/server_window_observer.h"
#include "services/ui/ws/window_manager_window_tree_factory_observer.h"
#include "ui/display/display.h"
#include "ui/events/event_sink.h"

namespace display {
struct ViewportMetrics;
}

namespace ui {
namespace ws {

class DisplayBinding;
class DisplayManager;
class FocusController;
class WindowManagerDisplayRoot;
class WindowServer;
class WindowTree;

namespace test {
class DisplayTestApi;
}

// Displays manages the state associated with a single display. Display has a
// single root window whose children are the roots for a per-user
// WindowManager. Display is configured in two distinct ways:
// . with a DisplayBinding. In this mode there is only ever one WindowManager
//   for the display, which comes from the client that created the
//   Display.
// . without a DisplayBinding. In this mode a WindowManager is automatically
//   created per user.
class Display : public PlatformDisplayDelegate,
                public mojom::WindowTreeHost,
                public FocusControllerObserver,
                public WindowManagerWindowTreeFactoryObserver,
                public EventSink {
 public:
  explicit Display(WindowServer* window_server);
  ~Display() override;

  // Initializes the display root ServerWindow and PlatformDisplay. Adds this to
  // DisplayManager as a pending display, until accelerated widget is available.
  void Init(const display::ViewportMetrics& metrics,
            std::unique_ptr<DisplayBinding> binding);

  // Initialize the display's root window to host window manager content.
  void InitWindowManagerDisplayRoots();

  // Returns the ID for this display. In internal mode this is the
  // display::Display ID. In external mode this hasn't been defined yet.
  int64_t GetId() const;

  // Sets the display::Display corresponding to this ws::Display.
  void SetDisplay(const display::Display& display);

  // PlatformDisplayDelegate:
  const display::Display& GetDisplay() override;

  const display::ViewportMetrics& GetViewportMetrics() const;

  DisplayManager* display_manager();
  const DisplayManager* display_manager() const;

  PlatformDisplay* platform_display() { return platform_display_.get(); }

  // Returns the size of the display in physical pixels.
  gfx::Size GetSize() const;

  WindowServer* window_server() { return window_server_; }

  // Returns the root of the Display. The root's children are the roots
  // of the corresponding WindowManagers.
  ServerWindow* root_window() { return root_.get(); }
  const ServerWindow* root_window() const { return root_.get(); }

  WindowManagerDisplayRoot* GetWindowManagerDisplayRootWithRoot(
      const ServerWindow* window);

  WindowManagerDisplayRoot* window_manager_display_root() {
    return window_manager_display_root_;
  }

  const WindowManagerDisplayRoot* window_manager_display_root() const {
    return window_manager_display_root_;
  }

  // TODO(sky): this should only be called by WindowServer, move to interface
  // used by WindowServer.
  // See description of WindowServer::SetFocusedWindow() for details on return
  // value.
  bool SetFocusedWindow(ServerWindow* window);
  // NOTE: this returns the focused window only if the focused window is in this
  // display. If this returns null focus may be in another display.
  ServerWindow* GetFocusedWindow();

  void UpdateTextInputState(ServerWindow* window,
                            const ui::TextInputState& state);
  void SetImeVisibility(ServerWindow* window, bool visible);

  // Called just before |tree| is destroyed.
  void OnWillDestroyTree(WindowTree* tree);

  // Called prior to |display_root| being destroyed.
  void RemoveWindowManagerDisplayRoot(WindowManagerDisplayRoot* display_root);

  // Sets the native cursor to |cursor|.
  void SetNativeCursor(const ui::CursorData& curosor);

  // Sets the native cursor size to |cursor_size|.
  void SetNativeCursorSize(ui::CursorSize cursor_size);

  // mojom::WindowTreeHost:
  void SetSize(const gfx::Size& size) override;
  void SetTitle(const std::string& title) override;

  // Updates the size of display root ServerWindow and WM root ServerWindow(s).
  void OnViewportMetricsChanged(const display::ViewportMetrics& metrics);

  void SetBoundsInPixels(const gfx::Rect& bounds_in_pixels);

  // Returns the root window of the active user.
  ServerWindow* GetActiveRootWindow();

  // Processes an event. |event_processed_callback| is run once the appropriate
  // client processes the event.
  // TODO(sky): move event processing code into standalone classes. Display
  // shouldn't have logic like this.
  void ProcessEvent(
      ui::Event* event,
      base::OnceClosure event_processed_callback = base::OnceClosure());

 private:
  friend class test::DisplayTestApi;

  class CursorState;

  // Creates a WindowManagerDisplayRoot from the
  // WindowManagerWindowTreeFactory.
  void CreateWindowManagerDisplayRootFromFactory();

  // Creates the root ServerWindow for this display, where |size| is in physical
  // pixels.
  void CreateRootWindow(const gfx::Size& size);

  // Applyes the cursor scale and rotation to the PlatformDisplay.
  void UpdateCursorConfig();

  // PlatformDisplayDelegate:
  ServerWindow* GetRootWindow() override;
  EventSink* GetEventSink() override;
  void OnAcceleratedWidgetAvailable() override;
  void OnNativeCaptureLost() override;
  bool IsHostingViz() const override;

  // FocusControllerObserver:
  void OnActivationChanged(ServerWindow* old_active_window,
                           ServerWindow* new_active_window) override;
  void OnFocusChanged(FocusControllerChangeSource change_source,
                      ServerWindow* old_focused_window,
                      ServerWindow* new_focused_window) override;

  // WindowManagerWindowTreeFactoryObserver:
  void OnWindowManagerWindowTreeFactoryReady(
      WindowManagerWindowTreeFactory* factory) override;

  // EventSink:
  EventDispatchDetails OnEventFromSource(Event* event) override;

  std::unique_ptr<DisplayBinding> binding_;
  WindowServer* const window_server_;
  std::unique_ptr<ServerWindow> root_;
  std::unique_ptr<PlatformDisplay> platform_display_;
  std::unique_ptr<FocusController> focus_controller_;

  // In internal window mode this contains information about the display. In
  // external window mode this will be invalid.
  display::Display display_;

  viz::ParentLocalSurfaceIdAllocator allocator_;

  // This is owned by WindowManagerState.
  WindowManagerDisplayRoot* window_manager_display_root_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Display);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_DISPLAY_H_
