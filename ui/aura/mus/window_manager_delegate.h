// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_WINDOW_MANAGER_DELEGATE_H_
#define UI_AURA_MUS_WINDOW_MANAGER_DELEGATE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/flat_map.h"
#include "services/ui/public/interfaces/cursor/cursor.mojom.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "services/ui/public/interfaces/window_tree_constants.mojom.h"
#include "ui/aura/aura_export.h"
#include "ui/events/mojo/event.mojom.h"
#include "ui/gfx/native_widget_types.h"

namespace display {
class Display;
}

namespace gfx {
class Insets;
class Rect;
}

namespace ui {
class Event;
}

namespace aura {

class Window;
class WindowTreeHostMus;

struct WindowTreeHostMusInitParams;

// This mirrors ui::mojom::BlockingContainers. See it for details.
struct BlockingContainers {
  aura::Window* system_modal_container = nullptr;
  aura::Window* min_container = nullptr;
};

// See the mojom with the same name for details on the functions in this
// interface.
class AURA_EXPORT WindowManagerClient {
 public:
  virtual void SetFrameDecorationValues(
      ui::mojom::FrameDecorationValuesPtr values) = 0;
  virtual void SetNonClientCursor(Window* window,
                                  const ui::CursorData& non_client_cursor) = 0;

  virtual void AddAccelerators(
      std::vector<ui::mojom::WmAcceleratorPtr> accelerators,
      const base::Callback<void(bool)>& callback) = 0;
  virtual void RemoveAccelerator(uint32_t id) = 0;
  virtual void AddActivationParent(Window* window) = 0;
  virtual void RemoveActivationParent(Window* window) = 0;
  virtual void SetExtendedHitRegionForChildren(
      Window* window,
      const gfx::Insets& mouse_area,
      const gfx::Insets& touch_area) = 0;

  // Queues changes to the cursor instead of applying them instantly. Queued
  // changes will be executed on UnlockCursor().
  virtual void LockCursor() = 0;

  // Executes queued changes.
  virtual void UnlockCursor() = 0;

  // Globally shows or hides the cursor.
  virtual void SetCursorVisible(bool visible) = 0;

  // Globally sets whether we use normal or large cursors.
  virtual void SetCursorSize(ui::CursorSize cursor_size) = 0;

  // Sets a cursor which is used instead of the per window cursors. Pass a
  // nullopt in |cursor| to clear the override.
  virtual void SetGlobalOverrideCursor(
      base::Optional<ui::CursorData> cursor) = 0;

  // Sets whether the cursor is visible because the user touched the
  // screen. This bit is separate from SetCursorVisible(), as it implicitly is
  // set in the window server when a touch event occurs, and is implicitly
  // cleared when the mouse moves.
  virtual void SetCursorTouchVisible(bool enabled) = 0;

  // Sends |event| to mus to be dispatched.
  virtual void InjectEvent(const ui::Event& event, int64_t display_id) = 0;

  // Sets the list of keys which don't hide the cursor.
  virtual void SetKeyEventsThatDontHideCursor(
      std::vector<ui::mojom::EventMatcherPtr> cursor_key_list) = 0;

  // Requests the client embedded in |window| to close the window. Only
  // applicable to top-level windows. If a client is not embedded in |window|,
  // this does nothing.
  virtual void RequestClose(Window* window) = 0;

  // See mojom::WindowManager::SetBlockingContainers() and
  // mojom::BlockingContainers for details on what this does.
  virtual void SetBlockingContainers(
      const std::vector<BlockingContainers>& all_blocking_containers) = 0;

  // Blocks until the initial displays have been received, or if displays are
  // not automatically created until the connection to mus has been
  // established.
  virtual bool WaitForInitialDisplays() = 0;

  // Used by the window manager to create a new display. This is only useful if
  // the WindowTreeClient was configured not to automatically create displays
  // (see ConnectAsWindowManager()). The caller needs to configure
  // DisplayInitParams on the returned object.
  virtual WindowTreeHostMusInitParams CreateInitParamsForNewDisplay() = 0;

  // Configures the displays. This is used when the window manager manually
  // configures display roots.
  virtual void SetDisplayConfiguration(
      const std::vector<display::Display>& displays,
      std::vector<ui::mojom::WmViewportMetricsPtr> viewport_metrics,
      int64_t primary_display_id,
      const std::vector<display::Display>& mirrors) = 0;

  // Adds |display| as a new display moving |window_tree_host| to the new
  // display. This results in closing the previous display |window_tree_host|
  // was associated with.
  virtual void AddDisplayReusingWindowTreeHost(
      WindowTreeHostMus* window_tree_host,
      const display::Display& display,
      ui::mojom::WmViewportMetricsPtr viewport_metrics) = 0;

  // Swaps the roots of the two displays.
  virtual void SwapDisplayRoots(WindowTreeHostMus* window_tree_host1,
                                WindowTreeHostMus* window_tree_host2) = 0;

 protected:
  virtual ~WindowManagerClient() {}
};

// Used by clients implementing a window manager.
// TODO(sky): this should be called WindowManager, but that's rather confusing
// currently.
class AURA_EXPORT WindowManagerDelegate {
 public:
  // Called once to give the delegate access to functions only exposed to
  // the WindowManager.
  virtual void SetWindowManagerClient(WindowManagerClient* client) = 0;

  // Called if the window server requires the window manager to manage the real
  // accelerated widget. This is the case when mus expects the window manager to
  // set up viz (instead of mus itself hosting viz).
  virtual void OnWmAcceleratedWidgetAvailableForDisplay(
      int64_t display_id,
      gfx::AcceleratedWidget widget) = 0;

  // Called when the connection to mus has been fully established.
  virtual void OnWmConnected();

  // A client requested the bounds of |window| to change to |bounds|.
  virtual void OnWmSetBounds(Window* window, const gfx::Rect& bounds) = 0;

  // A client requested the shared property named |name| to change to
  // |new_data|. Return true to allow the change to |new_data|, false
  // otherwise. If true is returned the property is set via
  // PropertyConverter::SetPropertyFromTransportValue().
  virtual bool OnWmSetProperty(
      Window* window,
      const std::string& name,
      std::unique_ptr<std::vector<uint8_t>>* new_data) = 0;

  // A client requested the modal type to be changed to |type|.
  virtual void OnWmSetModalType(Window* window, ui::ModalType type) = 0;

  // A client requested to change focusibility of |window|. We currently assume
  // this always succeeds.
  virtual void OnWmSetCanFocus(Window* window, bool can_focus) = 0;

  // A client has requested a new top level window. The delegate should create
  // and parent the window appropriately and return it. |properties| is the
  // supplied properties from the client requesting the new window. The
  // delegate may modify |properties| before calling NewWindow(), but the
  // delegate does *not* own |properties|, they are valid only for the life
  // of OnWmCreateTopLevelWindow(). |window_type| is the type of window
  // requested by the client. Use SetWindowType() with |window_type| (in
  // property_utils.h) to configure the type on the newly created window.
  virtual Window* OnWmCreateTopLevelWindow(
      ui::mojom::WindowType window_type,
      std::map<std::string, std::vector<uint8_t>>* properties) = 0;

  // Called when a Mus client's jankiness changes. |windows| is the set of
  // windows owned by the window manager in which the client is embedded.
  virtual void OnWmClientJankinessChanged(
      const std::set<Window*>& client_windows,
      bool janky) = 0;

  // Called when a Mus client has started a drag, and wants this image to be
  // the drag representation.
  virtual void OnWmBuildDragImage(const gfx::Point& screen_location,
                                  const SkBitmap& drag_image,
                                  const gfx::Vector2d& drag_image_offset,
                                  ui::mojom::PointerKind source) = 0;

  // Called during drags when the drag location has changed and the drag
  // representation must be moved.
  virtual void OnWmMoveDragImage(const gfx::Point& screen_location) = 0;

  // Called when a drag is complete or canceled, and signals that the drag image
  // should be removed.
  virtual void OnWmDestroyDragImage() = 0;

  // When a new display is added OnWmWillCreateDisplay() is called, and then
  // OnWmNewDisplay(). OnWmWillCreateDisplay() is intended to add the display
  // to the set of displays (see Screen).
  virtual void OnWmWillCreateDisplay(const display::Display& display) = 0;

  // Called when a WindowTreeHostMus is created for a new display
  // Called when a display is added. |window_tree_host| is the WindowTreeHost
  // for the new display.
  virtual void OnWmNewDisplay(
      std::unique_ptr<WindowTreeHostMus> window_tree_host,
      const display::Display& display) = 0;

  // Called when a display is removed. |window_tree_host| is the WindowTreeHost
  // for the display.
  virtual void OnWmDisplayRemoved(WindowTreeHostMus* window_tree_host) = 0;

  // Called when a display is modified.
  virtual void OnWmDisplayModified(const display::Display& display) = 0;

  // Called when an accelerator is received. |id| is the id previously
  // registered via AddAccelerators(). For pre-target accelerators the delegate
  // may add key/value pairs to |properties| that are then added to the
  // KeyEvent that is sent to the client with the focused window (only if this
  // returns UNHANDLED). |properties| may be used to pass around state from the
  // window manager to clients.
  virtual ui::mojom::EventResult OnAccelerator(
      uint32_t id,
      const ui::Event& event,
      base::flat_map<std::string, std::vector<uint8_t>>* properties);

  // Called when the mouse cursor is shown or hidden in response to a touch
  // event or window manager call.
  virtual void OnCursorTouchVisibleChanged(bool enabled) = 0;

  virtual void OnWmPerformMoveLoop(
      Window* window,
      ui::mojom::MoveLoopSource source,
      const gfx::Point& cursor_location,
      const base::Callback<void(bool)>& on_done) = 0;

  virtual void OnWmCancelMoveLoop(Window* window) = 0;

  // Called when then client changes the client area of a window.
  virtual void OnWmSetClientArea(
      Window* window,
      const gfx::Insets& insets,
      const std::vector<gfx::Rect>& additional_client_areas) = 0;

  // Returns whether |window| is the current active window.
  virtual bool IsWindowActive(Window* window) = 0;

  // Called when a client requests that its activation be given to another
  // window.
  virtual void OnWmDeactivateWindow(Window* window) = 0;

  // Called when a client requests that a generic action be performed. |window|
  // can never be null.
  virtual void OnWmPerformAction(Window* window, const std::string& action);

  // Called when an event is blocked by a modal window. |window| is the modal
  // window that blocked the event.
  virtual void OnEventBlockedByModalWindow(Window* window);

 protected:
  virtual ~WindowManagerDelegate() {}
};

}  // namespace ui

#endif  // UI_AURA_MUS_WINDOW_MANAGER_DELEGATE_H_
