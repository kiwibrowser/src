// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_DISPLAY_MANAGER_H_
#define SERVICES_UI_WS_DISPLAY_MANAGER_H_

#include <map>
#include <memory>
#include <set>

#include "base/macros.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "services/ui/display/screen_manager_delegate.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "services/ui/ws/ids.h"
#include "ui/display/display.h"

namespace display {
struct ViewportMetrics;
}
namespace ui {
class EventRewriter;
namespace ws {

class CursorLocationManager;
class Display;
class PlatformDisplayMirror;
class ServerWindow;
class UserDisplayManager;
class WindowManagerDisplayRoot;
class WindowServer;

// DisplayManager manages the set of Displays. DisplayManager distinguishes
// between displays that do not yet have an accelerated widget (pending), vs
// those that do.
class DisplayManager : public display::ScreenManagerDelegate {
 public:
  DisplayManager(WindowServer* window_server, bool is_hosting_viz);
  ~DisplayManager() override;

  // Called once WindowServer::display_creation_config() has been determined.
  void OnDisplayCreationConfigSet();

  // Indicates the display configuration is valid. Set to true when the
  // display_creation_config() has been determined and the config is
  // AUTOMATIC, or MANUAL and the SetDisplayConfiguration() has been called.
  bool got_initial_config_from_window_manager() const {
    return got_initial_config_from_window_manager_;
  }

  // Sets the display configuration from the window manager. Returns true
  // on success, false if the arguments aren't valid.
  bool SetDisplayConfiguration(
      const std::vector<display::Display>& displays,
      const std::vector<display::ViewportMetrics>& viewport_metrics,
      int64_t primary_display_id,
      int64_t internal_display_id,
      const std::vector<display::Display>& mirrors);

  // Returns the UserDisplayManager. DisplayManager owns the return value.
  UserDisplayManager* GetUserDisplayManager();

  // Returns the CursorLocationManager.
  CursorLocationManager* cursor_location_manager() {
    return cursor_location_manager_.get();
  }

  // Adds/removes a Display. DisplayManager owns the Displays.
  // TODO(sky): make add take a scoped_ptr.
  void AddDisplay(Display* display);
  // Called when the window manager explicitly adds a new display.
  Display* AddDisplayForWindowManager(bool is_primary_display,
                                      const display::Display& display,
                                      const display::ViewportMetrics& metrics);
  void DestroyDisplay(Display* display);
  void DestroyAllDisplays();
  const std::set<Display*>& displays() { return displays_; }
  std::set<const Display*> displays() const;

  // Notifies when something about the Display changes.
  void OnDisplayUpdated(const display::Display& display);

  // Returns the Display that contains |window|, or null if |window| is not
  // attached to a display.
  Display* GetDisplayContaining(const ServerWindow* window);
  const Display* GetDisplayContaining(const ServerWindow* window) const;

  // Returns the display with the specified display id, or null if there is no
  // display with that id.
  const Display* GetDisplayById(int64_t display_id) const;
  Display* GetDisplayById(int64_t display_id) {
    return const_cast<Display*>(
        const_cast<const DisplayManager*>(this)->GetDisplayById(display_id));
  }

  const WindowManagerDisplayRoot* GetWindowManagerDisplayRoot(
      const ServerWindow* window) const;
  // TODO(sky): constness here is wrong! fix!
  WindowManagerDisplayRoot* GetWindowManagerDisplayRoot(
      const ServerWindow* window);

  bool IsReady() const {
    return !displays_.empty() && got_initial_config_from_window_manager_;
  }
  bool has_active_or_pending_displays() const {
    return !displays_.empty() || !pending_displays_.empty();
  }

  bool InUnifiedDisplayMode() const;

  // Returns the id for the next root window (both for the root of a Display
  // as well as the root of WindowManagers).
  ClientWindowId GetAndAdvanceNextRootId();

  // Called when the AcceleratedWidget is available for |display|.
  void OnDisplayAcceleratedWidgetAvailable(Display* display);

  // Called when a new surface for a WindowManager root has been created.
  void OnWindowManagerSurfaceActivation(Display* display,
                                        const viz::SurfaceInfo& surface_info);

  // Switch the high contrast mode of all Displays to |enabled|.
  void SetHighContrastMode(bool enabled);

  bool IsInternalDisplay(const display::Display& display) const;
  int64_t GetInternalDisplayId() const;

 private:
  void CreateDisplay(const display::Display& display,
                     const display::ViewportMetrics& metrics);

  // NOTE: these functions are *not* called when the WindowManager manually
  // creates roots.
  // display::ScreenManagerDelegate:
  void OnDisplayAdded(const display::Display& display,
                      const display::ViewportMetrics& metrics) override;
  void OnDisplayRemoved(int64_t id) override;
  void OnDisplayModified(const display::Display& display,
                         const display::ViewportMetrics& metrics) override;
  void OnPrimaryDisplayChanged(int64_t primary_display_id) override;

  WindowServer* window_server_;

  // For rewriting ChromeOS function keys.
  std::unique_ptr<ui::EventRewriter> event_rewriter_;

  // Displays are initially added to |pending_displays_|. When the display is
  // initialized it is moved to |displays_|. WindowServer owns the Displays.
  std::set<Display*> pending_displays_;
  std::set<Display*> displays_;

  // Displays that mirror the contents of another display in |displays_|.
  std::vector<std::unique_ptr<PlatformDisplayMirror>> mirrors_;

  std::unique_ptr<UserDisplayManager> user_display_manager_;

  std::unique_ptr<CursorLocationManager> cursor_location_manager_;

  // ID to use for next root node.
  // TODO(sky): figure out why this starts at 2.
  ClientSpecificId next_root_id_ = 2;

  bool got_initial_config_from_window_manager_ = false;

  // Id of the internal display, or kInvalidDisplayId. This is only set when
  // WindowManager is configured to manually create display roots. This is
  // not mirrored in Display::SetInternalId() as mus may be running in the
  // same process as the window-manager, so that if this did set the value in
  // Display there could be race conditions.
  int64_t internal_display_id_ = display::kInvalidDisplayId;

  DISALLOW_COPY_AND_ASSIGN(DisplayManager);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_DISPLAY_MANAGER_H_
