// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/screen_mus.h"

#include "base/stl_util.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/display/types/display_constants.h"
#include "ui/views/mus/screen_mus_delegate.h"
#include "ui/views/mus/window_manager_frame_values.h"

namespace mojo {

template <>
struct TypeConverter<views::WindowManagerFrameValues,
                     ui::mojom::FrameDecorationValuesPtr> {
  static views::WindowManagerFrameValues Convert(
      const ui::mojom::FrameDecorationValuesPtr& input) {
    views::WindowManagerFrameValues result;
    result.normal_insets = input->normal_client_area_insets;
    result.maximized_insets = input->maximized_client_area_insets;
    result.max_title_bar_button_width = input->max_title_bar_button_width;
    return result;
  }
};

}  // namespace mojo

namespace views {

using Type = display::DisplayList::Type;

ScreenMus::ScreenMus(ScreenMusDelegate* delegate)
    : delegate_(delegate), screen_provider_observer_binding_(this) {
  DCHECK(delegate);
  display::Screen::SetScreenInstance(this);
}

ScreenMus::~ScreenMus() {
  DCHECK_EQ(this, display::Screen::GetScreen());
  display::Screen::SetScreenInstance(nullptr);
}

void ScreenMus::Init(service_manager::Connector* connector) {
  connector->BindInterface(ui::mojom::kServiceName, &screen_provider_);

  ui::mojom::ScreenProviderObserverPtr observer;
  screen_provider_observer_binding_.Bind(mojo::MakeRequest(&observer));
  screen_provider_->AddObserver(std::move(observer));

  // We need the set of displays before we can continue. Wait for it.
  //
  // TODO(rockot): Do something better here. This should not have to block tasks
  // from running on the calling thread. http://crbug.com/594852.
  bool success = screen_provider_observer_binding_.WaitForIncomingMethodCall();

  // The WaitForIncomingMethodCall() should have supplied the set of Displays,
  // unless mus is going down, in which case encountered_error() is true, or the
  // call to WaitForIncomingMethodCall() failed.
  if (display_list().displays().empty()) {
    DCHECK(screen_provider_.encountered_error() || !success);
    // In this case we install a default display and assume the process is
    // going to exit shortly so that the real value doesn't matter.
    display_list().AddDisplay(
        display::Display(0xFFFFFFFF, gfx::Rect(0, 0, 801, 802)), Type::PRIMARY);
  }
}

display::Display ScreenMus::GetDisplayNearestWindow(
    gfx::NativeWindow window) const {
  aura::WindowTreeHostMus* window_tree_host_mus =
      aura::WindowTreeHostMus::ForWindow(window);
  if (!window_tree_host_mus)
    return GetPrimaryDisplay();
  return window_tree_host_mus->GetDisplay();
}

gfx::Point ScreenMus::GetCursorScreenPoint() {
  return aura::Env::GetInstance()->last_mouse_location();
}

bool ScreenMus::IsWindowUnderCursor(gfx::NativeWindow window) {
  return window && window->IsVisible() &&
         window->GetBoundsInScreen().Contains(GetCursorScreenPoint());
}

aura::Window* ScreenMus::GetWindowAtScreenPoint(const gfx::Point& point) {
  return delegate_->GetWindowAtScreenPoint(point);
}

void ScreenMus::OnDisplaysChanged(
    std::vector<ui::mojom::WsDisplayPtr> ws_displays,
    int64_t primary_display_id,
    int64_t internal_display_id) {
  const bool primary_changed = primary_display_id != GetPrimaryDisplay().id();
  int64_t handled_display_id = display::kInvalidDisplayId;
  const WindowManagerFrameValues initial_frame_values =
      WindowManagerFrameValues::instance();

  if (internal_display_id != display::kInvalidDisplayId)
    display::Display::SetInternalDisplayId(internal_display_id);

  if (primary_changed) {
    handled_display_id = primary_display_id;
    for (auto& ws_display_ptr : ws_displays) {
      if (ws_display_ptr->display.id() == primary_display_id) {
        // TODO(sky): Make WindowManagerFrameValues per display.
        WindowManagerFrameValues frame_values =
            ws_display_ptr->frame_decoration_values
                .To<WindowManagerFrameValues>();
        WindowManagerFrameValues::SetInstance(frame_values);

        const bool is_primary = true;
        ProcessDisplayChanged(ws_display_ptr->display, is_primary);
        break;
      }
    }
  }

  // Add new displays and update existing ones.
  std::set<int64_t> display_ids;
  for (auto& ws_display_ptr : ws_displays) {
    display_ids.insert(ws_display_ptr->display.id());
    if (handled_display_id == ws_display_ptr->display.id())
      continue;

    const bool is_primary = false;
    ProcessDisplayChanged(ws_display_ptr->display, is_primary);
  }

  // Remove any displays no longer in |ws_displays|.
  std::set<int64_t> existing_display_ids;
  for (const auto& display : display_list().displays())
    existing_display_ids.insert(display.id());
  std::set<int64_t> removed_display_ids =
      base::STLSetDifference<std::set<int64_t>>(existing_display_ids,
                                                display_ids);
  for (int64_t display_id : removed_display_ids) {
    // TODO(kylechar): DisplayList would need to change to handle having no
    // primary display.
    if (display_id != GetPrimaryDisplay().id())
      display_list().RemoveDisplay(display_id);
  }

  if (primary_changed &&
      initial_frame_values != WindowManagerFrameValues::instance()) {
    delegate_->OnWindowManagerFrameValuesChanged();
  }
}

}  // namespace views
