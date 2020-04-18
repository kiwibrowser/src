// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/display_manager.h"

#include <vector>

#include "base/containers/flat_set.h"
#include "base/trace_event/trace_event.h"
#include "services/ui/display/screen_manager.h"
#include "services/ui/display/viewport_metrics.h"
#include "services/ui/ws/cursor_location_manager.h"
#include "services/ui/ws/display.h"
#include "services/ui/ws/display_binding.h"
#include "services/ui/ws/display_creation_config.h"
#include "services/ui/ws/event_processor.h"
#include "services/ui/ws/frame_generator.h"
#include "services/ui/ws/platform_display_mirror.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/user_display_manager.h"
#include "services/ui/ws/user_display_manager_delegate.h"
#include "services/ui/ws/window_manager_state.h"
#include "services/ui/ws/window_manager_window_tree_factory.h"
#include "services/ui/ws/window_server_delegate.h"
#include "services/ui/ws/window_tree.h"
#include "ui/display/display_list.h"
#include "ui/display/screen_base.h"
#include "ui/display/types/display_constants.h"
#include "ui/events/event_rewriter.h"
#include "ui/gfx/geometry/point_conversions.h"

#if defined(OS_CHROMEOS)
#include "ui/chromeos/events/event_rewriter_chromeos.h"
#endif

namespace ui {
namespace ws {

DisplayManager::DisplayManager(WindowServer* window_server, bool is_hosting_viz)
    : window_server_(window_server),
      cursor_location_manager_(std::make_unique<CursorLocationManager>()) {
#if defined(OS_CHROMEOS)
  if (is_hosting_viz) {
    // TODO: http://crbug.com/701468 fix function key preferences and sticky
    // keys.
    ui::EventRewriterChromeOS::Delegate* delegate = nullptr;
    ui::EventRewriter* sticky_keys_controller = nullptr;
    event_rewriter_ = std::make_unique<ui::EventRewriterChromeOS>(
        delegate, sticky_keys_controller);
  }
#endif
}

DisplayManager::~DisplayManager() {
  DestroyAllDisplays();
}

void DisplayManager::OnDisplayCreationConfigSet() {
  if (window_server_->display_creation_config() ==
      DisplayCreationConfig::MANUAL) {
    if (user_display_manager_)
      user_display_manager_->DisableAutomaticNotification();
  } else {
    // In AUTOMATIC mode SetDisplayConfiguration() is never called.
    got_initial_config_from_window_manager_ = true;
  }
}

bool DisplayManager::SetDisplayConfiguration(
    const std::vector<display::Display>& displays,
    const std::vector<display::ViewportMetrics>& viewport_metrics,
    int64_t primary_display_id,
    int64_t internal_display_id,
    const std::vector<display::Display>& mirrors) {
  DCHECK_NE(display::kUnifiedDisplayId, internal_display_id);
  if (window_server_->display_creation_config() !=
      DisplayCreationConfig::MANUAL) {
    LOG(ERROR) << "SetDisplayConfiguration is only valid when roots manually "
                  "created";
    return false;
  }
  if (displays.size() + mirrors.size() != viewport_metrics.size()) {
    LOG(ERROR) << "SetDisplayConfiguration called with mismatch in sizes";
    return false;
  }

  size_t primary_display_index = std::numeric_limits<size_t>::max();

  // Check the mirrors before potentially passing them to a unified display.
  base::flat_set<int64_t> mirror_ids;
  for (const auto& mirror : mirrors) {
    if (mirror.id() == display::kInvalidDisplayId) {
      LOG(ERROR) << "SetDisplayConfiguration passed invalid display id";
      return false;
    }
    if (mirror.id() == display::kUnifiedDisplayId) {
      LOG(ERROR) << "SetDisplayConfiguration passed unified display in mirrors";
      return false;
    }
    if (!mirror_ids.insert(mirror.id()).second) {
      LOG(ERROR) << "SetDisplayConfiguration passed duplicate display id";
      return false;
    }
    if (mirror.id() == primary_display_id) {
      LOG(ERROR) << "SetDisplayConfiguration passed primary display in mirrors";
      return false;
    }
  }

  base::flat_set<int64_t> display_ids;
  for (size_t i = 0; i < displays.size(); ++i) {
    const display::Display& display = displays[i];
    if (display.id() == display::kInvalidDisplayId) {
      LOG(ERROR) << "SetDisplayConfiguration passed invalid display id";
      return false;
    }
    if (!display_ids.insert(display.id()).second) {
      LOG(ERROR) << "SetDisplayConfiguration passed duplicate display id";
      return false;
    }
    if (display.id() == primary_display_id)
      primary_display_index = i;
    Display* ws_display = GetDisplayById(display.id());
    if (!ws_display && display.id() != display::kUnifiedDisplayId) {
      LOG(ERROR) << "SetDisplayConfiguration passed unknown display id "
                 << display.id();
      return false;
    }
  }
  if (primary_display_index == std::numeric_limits<size_t>::max()) {
    LOG(ERROR) << "SetDisplayConfiguration primary id not in displays";
    return false;
  }

  // See comment in header as to why this doesn't use Display::SetInternalId().
  // NOTE: the internal display might not be listed in |displays| or |mirrors|.
  internal_display_id_ = internal_display_id;

  display::DisplayList& display_list =
      display::ScreenManager::GetInstance()->GetScreen()->display_list();
  // Update the primary display first, in case the old primary has been removed.
  display_list.AddOrUpdateDisplay(displays[primary_display_index],
                                  display::DisplayList::Type::PRIMARY);

  // Remove any existing displays that are not included in the configuration.
  for (int i = display_list.displays().size() - 1; i >= 0; --i) {
    const int64_t id = display_list.displays()[i].id();
    if (display_ids.count(id) == 0) {
      // The display list also contains mirrors, which do not have ws::Displays.
      // If the destroyed display still has a root window, it is orphaned here.
      // Root windows are destroyed by explicit window manager instruction.
      if (Display* ws_display = GetDisplayById(id))
        DestroyDisplay(ws_display);
      // Do not remove display::Display entries for mirroring displays.
      if (mirror_ids.count(id) == 0)
        display_list.RemoveDisplay(id);
    }
  }

  // Remove any existing mirrors that are not included in the configuration.
  for (int i = mirrors_.size() - 1; i >= 0; --i) {
    if (mirror_ids.count(mirrors_[i]->display().id()) == 0) {
      // Do not remove display::Display entries for extended displays.
      if (display_ids.count(mirrors_[i]->display().id()) == 0)
        display_list.RemoveDisplay(mirrors_[i]->display().id());
      mirrors_.erase(mirrors_.begin() + i);
    }
  }

  // Add or update any displays that are included in the configuration.
  for (size_t i = 0; i < displays.size(); ++i) {
    Display* ws_display = GetDisplayById(displays[i].id());
    DCHECK(ws_display);
    ws_display->SetDisplay(displays[i]);
    ws_display->OnViewportMetricsChanged(viewport_metrics[i]);
    if (i != primary_display_index) {
      display_list.AddOrUpdateDisplay(displays[i],
                                      display::DisplayList::Type::NOT_PRIMARY);
    }
  }

  // Add or update any mirrors that are included in the configuration.
  for (size_t i = 0; i < mirrors.size(); ++i) {
    DCHECK(!GetDisplayById(mirrors[i].id()));
    Display* ws_display_to_mirror = GetDisplayById(displays[0].id());

    const auto& metrics = viewport_metrics[displays.size() + i];
    PlatformDisplayMirror* mirror = nullptr;
    for (size_t i = 0; i < mirrors_.size(); ++i) {
      if (mirrors_[i]->display().id() == mirrors[i].id()) {
        mirror = mirrors_[i].get();
        break;
      }
    }
    if (mirror) {
      mirror->set_display(mirrors[i]);
      mirror->UpdateViewportMetrics(metrics);
    } else {
      mirrors_.push_back(std::make_unique<PlatformDisplayMirror>(
          mirrors[i], metrics, window_server_, ws_display_to_mirror));
    }
  }

  std::set<int64_t> existing_display_ids;
  for (const display::Display& display : display_list.displays())
    existing_display_ids.insert(display.id());
  std::set<int64_t> removed_display_ids =
      base::STLSetDifference<std::set<int64_t>>(existing_display_ids,
                                                display_ids);
  for (int64_t display_id : removed_display_ids)
    display_list.RemoveDisplay(display_id);

  if (user_display_manager_)
    user_display_manager_->CallOnDisplaysChanged();

  if (!got_initial_config_from_window_manager_) {
    got_initial_config_from_window_manager_ = true;
    window_server_->delegate()->OnFirstDisplayReady();
  }

  return true;
}

UserDisplayManager* DisplayManager::GetUserDisplayManager() {
  if (!user_display_manager_) {
    user_display_manager_ =
        std::make_unique<UserDisplayManager>(window_server_);
    if (window_server_->display_creation_config() ==
        DisplayCreationConfig::MANUAL) {
      user_display_manager_->DisableAutomaticNotification();
    }
  }
  return user_display_manager_.get();
}

void DisplayManager::AddDisplay(Display* display) {
  DCHECK_EQ(0u, pending_displays_.count(display));
  pending_displays_.insert(display);
}

Display* DisplayManager::AddDisplayForWindowManager(
    bool is_primary_display,
    const display::Display& display,
    const display::ViewportMetrics& metrics) {
  DCHECK_EQ(DisplayCreationConfig::MANUAL,
            window_server_->display_creation_config());
  const display::DisplayList::Type display_type =
      is_primary_display ? display::DisplayList::Type::PRIMARY
                         : display::DisplayList::Type::NOT_PRIMARY;
  display::DisplayList& display_list =
      display::ScreenManager::GetInstance()->GetScreen()->display_list();
  // The display may already be listed as a mirror destination display.
  display_list.AddOrUpdateDisplay(display, display_type);
  OnDisplayAdded(display, metrics);
  return GetDisplayById(display.id());
}

void DisplayManager::DestroyDisplay(Display* display) {
  const bool is_pending = pending_displays_.count(display) > 0;
  if (is_pending) {
    pending_displays_.erase(display);
  } else {
    DCHECK(displays_.count(display));
    displays_.erase(display);
    window_server_->OnDisplayDestroyed(display);
  }
  const int64_t display_id = display->GetId();
  delete display;

  // If we have no more roots left, let the app know so it can terminate.
  // TODO(sky): move to delegate/observer.
  if (displays_.empty() && pending_displays_.empty())
    window_server_->OnNoMoreDisplays();
  else if (user_display_manager_)
    user_display_manager_->OnDisplayDestroyed(display_id);
}

void DisplayManager::DestroyAllDisplays() {
  while (!pending_displays_.empty())
    DestroyDisplay(*pending_displays_.begin());
  DCHECK(pending_displays_.empty());

  while (!displays_.empty())
    DestroyDisplay(*displays_.begin());
  DCHECK(displays_.empty());
}

std::set<const Display*> DisplayManager::displays() const {
  std::set<const Display*> ret_value(displays_.begin(), displays_.end());
  return ret_value;
}

void DisplayManager::OnDisplayUpdated(const display::Display& display) {
  if (user_display_manager_)
    user_display_manager_->OnDisplayUpdated(display);
}

Display* DisplayManager::GetDisplayContaining(const ServerWindow* window) {
  return const_cast<Display*>(
      static_cast<const DisplayManager*>(this)->GetDisplayContaining(window));
}

const Display* DisplayManager::GetDisplayContaining(
    const ServerWindow* window) const {
  while (window && window->parent())
    window = window->parent();
  for (Display* display : displays_) {
    if (window == display->root_window())
      return display;
  }
  return nullptr;
}

const Display* DisplayManager::GetDisplayById(int64_t display_id) const {
  for (Display* display : displays_) {
    if (display->GetId() == display_id)
      return display;
  }
  return nullptr;
}

const WindowManagerDisplayRoot* DisplayManager::GetWindowManagerDisplayRoot(
    const ServerWindow* window) const {
  const ServerWindow* last = window;
  while (window && window->parent()) {
    last = window;
    window = window->parent();
  }
  for (Display* display : displays_) {
    if (window == display->root_window())
      return display->GetWindowManagerDisplayRootWithRoot(last);
  }
  return nullptr;
}

WindowManagerDisplayRoot* DisplayManager::GetWindowManagerDisplayRoot(
    const ServerWindow* window) {
  return const_cast<WindowManagerDisplayRoot*>(
      const_cast<const DisplayManager*>(this)->GetWindowManagerDisplayRoot(
          window));
}

bool DisplayManager::InUnifiedDisplayMode() const {
  return GetDisplayById(display::kUnifiedDisplayId) != nullptr;
}

ClientWindowId DisplayManager::GetAndAdvanceNextRootId() {
  const ClientSpecificId id = next_root_id_++;
  CHECK_NE(0u, next_root_id_);
  return ClientWindowId(kWindowServerClientId, id);
}

void DisplayManager::OnDisplayAcceleratedWidgetAvailable(Display* display) {
  DCHECK_NE(0u, pending_displays_.count(display));
  DCHECK_EQ(0u, displays_.count(display));
  const bool is_first_display =
      displays_.empty() && got_initial_config_from_window_manager_;
  displays_.insert(display);
  pending_displays_.erase(display);
  if (event_rewriter_)
    display->platform_display()->AddEventRewriter(event_rewriter_.get());
  window_server_->OnDisplayReady(display, is_first_display);
}

void DisplayManager::OnWindowManagerSurfaceActivation(
    Display* display,
    const viz::SurfaceInfo& surface_info) {
  display->platform_display()->GetFrameGenerator()->SetEmbeddedSurface(
      surface_info);
  // Also pass the surface info to any displays mirroring the given display.
  for (const auto& mirror : mirrors_) {
    if (mirror->display_to_mirror() == display)
      mirror->GetFrameGenerator()->SetEmbeddedSurface(surface_info);
  }
}

void DisplayManager::SetHighContrastMode(bool enabled) {
  for (Display* display : displays_) {
    display->platform_display()->GetFrameGenerator()->SetHighContrastMode(
        enabled);
  }
}

bool DisplayManager::IsInternalDisplay(const display::Display& display) const {
  return display.id() == GetInternalDisplayId();
}

int64_t DisplayManager::GetInternalDisplayId() const {
  if (window_server_->display_creation_config() ==
      DisplayCreationConfig::MANUAL) {
    return internal_display_id_;
  }
  return display::Display::HasInternalDisplay()
             ? display::Display::InternalDisplayId()
             : display::kInvalidDisplayId;
}

void DisplayManager::CreateDisplay(const display::Display& display,
                                   const display::ViewportMetrics& metrics) {
  ws::Display* ws_display = new ws::Display(window_server_);
  ws_display->SetDisplay(display);
  ws_display->Init(metrics, nullptr);
}

void DisplayManager::OnDisplayAdded(const display::Display& display,
                                    const display::ViewportMetrics& metrics) {
  DVLOG(3) << "OnDisplayAdded: " << display.ToString();
  CreateDisplay(display, metrics);
}

void DisplayManager::OnDisplayRemoved(int64_t display_id) {
  DVLOG(3) << "OnDisplayRemoved: " << display_id;
  Display* display = GetDisplayById(display_id);
  if (display)
    DestroyDisplay(display);
}

void DisplayManager::OnDisplayModified(
    const display::Display& display,
    const display::ViewportMetrics& metrics) {
  DVLOG(3) << "OnDisplayModified: " << display.ToString();

  Display* ws_display = GetDisplayById(display.id());
  DCHECK(ws_display);

  // Update the cached display information.
  ws_display->SetDisplay(display);

  // Send IPC to WMs with new display information.
  WindowManagerWindowTreeFactory* factory =
      window_server_->window_manager_window_tree_factory();
  if (factory->window_tree())
    factory->window_tree()->OnWmDisplayModified(display);

  // Update the PlatformWindow and ServerWindow size. This must happen after
  // OnWmDisplayModified() so the WM has updated the display size.
  ws_display->OnViewportMetricsChanged(metrics);

  OnDisplayUpdated(display);
}

void DisplayManager::OnPrimaryDisplayChanged(int64_t primary_display_id) {
  DVLOG(3) << "OnPrimaryDisplayChanged: " << primary_display_id;
  // TODO(kylechar): Send IPCs to WM clients first.

  // Send IPCs to any DisplayManagerObservers.
  if (user_display_manager_)
    user_display_manager_->OnPrimaryDisplayChanged(primary_display_id);
}

}  // namespace ws
}  // namespace ui
