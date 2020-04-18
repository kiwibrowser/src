// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/display.h"

#include <set>
#include <utility>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "services/service_manager/public/mojom/connector.mojom.h"
#include "services/ui/common/types.h"
#include "services/ui/display/viewport_metrics.h"
#include "services/ui/public/interfaces/cursor/cursor.mojom.h"
#include "services/ui/ws/debug_utils.h"
#include "services/ui/ws/display_binding.h"
#include "services/ui/ws/display_manager.h"
#include "services/ui/ws/focus_controller.h"
#include "services/ui/ws/platform_display.h"
#include "services/ui/ws/user_activity_monitor.h"
#include "services/ui/ws/window_manager_display_root.h"
#include "services/ui/ws/window_manager_state.h"
#include "services/ui/ws/window_manager_window_tree_factory.h"
#include "services/ui/ws/window_server.h"
#include "services/ui/ws/window_server_delegate.h"
#include "services/ui/ws/window_tree.h"
#include "services/ui/ws/window_tree_binding.h"
#include "ui/base/cursor/cursor.h"
#include "ui/display/screen.h"

namespace ui {
namespace ws {

Display::Display(WindowServer* window_server) : window_server_(window_server) {
  window_server_->window_manager_window_tree_factory()->AddObserver(this);
}

Display::~Display() {
  window_server_->window_manager_window_tree_factory()->RemoveObserver(this);

  if (focus_controller_) {
    focus_controller_->RemoveObserver(this);
    focus_controller_.reset();
  }

  if (!binding_) {
    if (window_manager_display_root_) {
      window_manager_display_root_->window_manager_state()->OnDisplayDestroying(
          this);
    }
  } else if (window_manager_display_root_) {
    // If there is a |binding_| then the tree was created specifically for this
    // display (which corresponds to a WindowTreeHost).
    window_server_->DestroyTree(
        window_manager_display_root_->window_manager_state()->window_tree());
  }
}

void Display::Init(const display::ViewportMetrics& metrics,
                   std::unique_ptr<DisplayBinding> binding) {
  binding_ = std::move(binding);
  display_manager()->AddDisplay(this);

  CreateRootWindow(metrics.bounds_in_pixels.size());

  platform_display_ = PlatformDisplay::Create(
      root_.get(), metrics, window_server_->GetThreadedImageCursorsFactory());
  platform_display_->Init(this);
  UpdateCursorConfig();
}

void Display::InitWindowManagerDisplayRoots() {
  if (binding_) {
    std::unique_ptr<WindowManagerDisplayRoot> display_root_ptr(
        new WindowManagerDisplayRoot(this));
    window_manager_display_root_ = display_root_ptr.get();
    WindowManagerDisplayRoot* display_root = display_root_ptr.get();
    WindowTree* window_tree = binding_->CreateWindowTree(display_root->root());
    display_root->window_manager_state_ = window_tree->window_manager_state();
    window_tree->window_manager_state()->AddWindowManagerDisplayRoot(
        std::move(display_root_ptr));
  } else {
    CreateWindowManagerDisplayRootFromFactory();
  }
  display_manager()->OnDisplayUpdated(display_);
}

int64_t Display::GetId() const {
  // TODO(tonikitoo): Implement a different ID for external window mode.
  return display_.id();
}

void Display::SetDisplay(const display::Display& display) {
  display_ = display;

  UpdateCursorConfig();
}

const display::Display& Display::GetDisplay() {
  return display_;
}

const display::ViewportMetrics& Display::GetViewportMetrics() const {
  return platform_display_->GetViewportMetrics();
}

DisplayManager* Display::display_manager() {
  return window_server_->display_manager();
}

const DisplayManager* Display::display_manager() const {
  return window_server_->display_manager();
}

gfx::Size Display::GetSize() const {
  DCHECK(root_);
  return root_->bounds().size();
}

WindowManagerDisplayRoot* Display::GetWindowManagerDisplayRootWithRoot(
    const ServerWindow* window) {
  return (window_manager_display_root_ &&
          window_manager_display_root_->root() == window)
             ? window_manager_display_root_
             : nullptr;
}

bool Display::SetFocusedWindow(ServerWindow* new_focused_window) {
  DVLOG(3) << "Display::SetFocusedWindow id="
           << DebugWindowId(new_focused_window);
  ServerWindow* old_focused_window = focus_controller_->GetFocusedWindow();
  if (old_focused_window == new_focused_window)
    return true;
  DCHECK(!new_focused_window || root_->Contains(new_focused_window));
  return focus_controller_->SetFocusedWindow(new_focused_window);
}

ServerWindow* Display::GetFocusedWindow() {
  return focus_controller_->GetFocusedWindow();
}

void Display::UpdateTextInputState(ServerWindow* window,
                                   const ui::TextInputState& state) {
  // Do not need to update text input for unfocused windows.
  if (!platform_display_ || focus_controller_->GetFocusedWindow() != window)
    return;
  platform_display_->UpdateTextInputState(state);
}

void Display::SetImeVisibility(ServerWindow* window, bool visible) {
  // Do not need to show or hide IME for unfocused window.
  if (focus_controller_->GetFocusedWindow() != window)
    return;
  platform_display_->SetImeVisibility(visible);
}

void Display::OnWillDestroyTree(WindowTree* tree) {
  if (window_manager_display_root_ &&
      window_manager_display_root_->window_manager_state()->window_tree() ==
          tree) {
    window_manager_display_root_ = nullptr;
  }
}

void Display::RemoveWindowManagerDisplayRoot(
    WindowManagerDisplayRoot* display_root) {
  DCHECK_EQ(window_manager_display_root_, display_root);
  window_manager_display_root_ = nullptr;
  display_manager()->DestroyDisplay(this);
}

void Display::SetNativeCursor(const ui::CursorData& cursor) {
  platform_display_->SetCursor(cursor);
}

void Display::SetNativeCursorSize(ui::CursorSize cursor_size) {
  platform_display_->SetCursorSize(cursor_size);
}

void Display::SetSize(const gfx::Size& size) {
  platform_display_->SetViewportSize(size);
}

void Display::SetTitle(const std::string& title) {
  platform_display_->SetTitle(base::UTF8ToUTF16(title));
}

void Display::CreateWindowManagerDisplayRootFromFactory() {
  WindowManagerWindowTreeFactory* factory =
      window_server_->window_manager_window_tree_factory();
  if (!factory->window_tree())
    return;

  std::unique_ptr<WindowManagerDisplayRoot> display_root_ptr =
      std::make_unique<WindowManagerDisplayRoot>(this);
  window_manager_display_root_ = display_root_ptr.get();
  WindowManagerState* window_manager_state =
      factory->window_tree()->window_manager_state();
  window_manager_display_root_->window_manager_state_ = window_manager_state;
  window_manager_display_root_->root()->SetVisible(true);
  window_manager_state->window_tree()->AddRootForWindowManager(
      window_manager_display_root_->root());
  window_manager_state->AddWindowManagerDisplayRoot(
      std::move(display_root_ptr));
}

void Display::CreateRootWindow(const gfx::Size& size) {
  DCHECK(!root_);

  const ClientWindowId client_window_id =
      display_manager()->GetAndAdvanceNextRootId();
  root_.reset(window_server_->CreateServerWindow(client_window_id,
                                                 ServerWindow::Properties()));
  root_->set_event_targeting_policy(
      mojom::EventTargetingPolicy::DESCENDANTS_ONLY);
  root_->SetBounds(gfx::Rect(size), allocator_.GenerateId());
  root_->SetVisible(true);
  focus_controller_ = std::make_unique<FocusController>(root_.get());
  focus_controller_->AddObserver(this);
}

void Display::UpdateCursorConfig() {
  float scale = display_.device_scale_factor();

  if (!display_manager()->IsInternalDisplay(display_))
    scale *= ui::mojom::kCursorMultiplierForExternalDisplays;

  if (platform_display_)
    platform_display_->SetCursorConfig(display_.rotation(), scale);
}

ServerWindow* Display::GetRootWindow() {
  return root_.get();
}

EventSink* Display::GetEventSink() {
  return this;
}

void Display::OnAcceleratedWidgetAvailable() {
  display_manager()->OnDisplayAcceleratedWidgetAvailable(this);
  InitWindowManagerDisplayRoots();
}

void Display::OnNativeCaptureLost() {
  if (window_manager_display_root_) {
    window_manager_display_root_->window_manager_state()->SetCapture(
        nullptr, kInvalidClientId);
  }
}

bool Display::IsHostingViz() const {
  return window_server_->is_hosting_viz();
}

void Display::OnViewportMetricsChanged(
    const display::ViewportMetrics& metrics) {
  platform_display_->UpdateViewportMetrics(metrics);

  SetBoundsInPixels(metrics.bounds_in_pixels);
}

void Display::SetBoundsInPixels(const gfx::Rect& bounds_in_pixels) {
  if (root_->bounds().size() == bounds_in_pixels.size())
    return;

  gfx::Rect new_bounds(bounds_in_pixels.size());
  root_->SetBounds(new_bounds, allocator_.GenerateId());
  if (window_manager_display_root_) {
    window_manager_display_root_->root()->SetBounds(new_bounds,
                                                    allocator_.GenerateId());
  }
}

ServerWindow* Display::GetActiveRootWindow() {
  if (window_manager_display_root_)
    return window_manager_display_root_->root();
  return nullptr;
}

void Display::ProcessEvent(ui::Event* event,
                           base::OnceClosure event_processed_callback) {
  if (window_manager_display_root_) {
    WindowManagerState* wm_state =
        window_manager_display_root_->window_manager_state();
    wm_state->ProcessEvent(event, GetId());
    if (event_processed_callback) {
      wm_state->ScheduleCallbackWhenDoneProcessingEvents(
          std::move(event_processed_callback));
    }
  } else if (event_processed_callback) {
    std::move(event_processed_callback).Run();
  }

  window_server_->user_activity_monitor()->OnUserActivity();
}

void Display::OnActivationChanged(ServerWindow* old_active_window,
                                  ServerWindow* new_active_window) {
  // Don't do anything here. We assume the window manager handles restacking. If
  // we did attempt to restack then we would be reordering windows owned by
  // the window-manager, which breaks the assumption that only the owner of a
  // window reorders the children.
}

void Display::OnFocusChanged(FocusControllerChangeSource change_source,
                             ServerWindow* old_focused_window,
                             ServerWindow* new_focused_window) {
  // TODO(sky): focus is global, not per windowtreehost. Move.

  // There are up to four clients that need to be notified:
  // . the client containing |old_focused_window|.
  // . the client with |old_focused_window| as its root.
  // . the client containing |new_focused_window|.
  // . the client with |new_focused_window| as its root.
  // Some of these client may be the same. The following takes care to notify
  // each only once.
  WindowTree* owning_tree_old = nullptr;
  WindowTree* embedded_tree_old = nullptr;

  if (old_focused_window) {
    owning_tree_old =
        window_server_->GetTreeWithId(old_focused_window->owning_tree_id());
    if (owning_tree_old) {
      owning_tree_old->ProcessFocusChanged(old_focused_window,
                                           new_focused_window);
    }
    embedded_tree_old = window_server_->GetTreeWithRoot(old_focused_window);
    if (embedded_tree_old) {
      DCHECK_NE(owning_tree_old, embedded_tree_old);
      embedded_tree_old->ProcessFocusChanged(old_focused_window,
                                             new_focused_window);
    }
  }
  WindowTree* owning_tree_new = nullptr;
  WindowTree* embedded_tree_new = nullptr;
  if (new_focused_window) {
    owning_tree_new =
        window_server_->GetTreeWithId(new_focused_window->owning_tree_id());
    if (owning_tree_new && owning_tree_new != owning_tree_old &&
        owning_tree_new != embedded_tree_old) {
      owning_tree_new->ProcessFocusChanged(old_focused_window,
                                           new_focused_window);
    }
    embedded_tree_new = window_server_->GetTreeWithRoot(new_focused_window);
    if (embedded_tree_new && embedded_tree_new != owning_tree_old &&
        embedded_tree_new != embedded_tree_old) {
      DCHECK_NE(owning_tree_new, embedded_tree_new);
      embedded_tree_new->ProcessFocusChanged(old_focused_window,
                                             new_focused_window);
    }
  }

  // WindowManagers are always notified of focus changes.
  if (window_manager_display_root_) {
    WindowTree* wm_tree =
        window_manager_display_root_->window_manager_state()->window_tree();
    if (wm_tree != owning_tree_old && wm_tree != embedded_tree_old &&
        wm_tree != owning_tree_new && wm_tree != embedded_tree_new) {
      wm_tree->ProcessFocusChanged(old_focused_window, new_focused_window);
    }
  }

  if (new_focused_window) {
    UpdateTextInputState(new_focused_window,
                         new_focused_window->text_input_state());
  }
}

void Display::OnWindowManagerWindowTreeFactoryReady(
    WindowManagerWindowTreeFactory* factory) {
  if (!binding_)
    CreateWindowManagerDisplayRootFromFactory();
}

EventDispatchDetails Display::OnEventFromSource(Event* event) {
  ProcessEvent(event);
  return EventDispatchDetails();
}

}  // namespace ws
}  // namespace ui
