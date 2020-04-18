// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/top_level_window_factory.h"

#include "ash/disconnected_app_handler.h"
#include "ash/frame/detached_title_area_renderer.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/root_window_controller.h"
#include "ash/root_window_settings.h"
#include "ash/shell.h"
#include "ash/window_manager.h"
#include "ash/wm/container_finder.h"
#include "ash/wm/non_client_frame_controller.h"
#include "ash/wm/property_util.h"
#include "ash/wm/window_state.h"
#include "mojo/public/cpp/bindings/type_converter.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/property_utils.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {

// Returns true if a fullscreen window was requested.
bool IsFullscreen(aura::PropertyConverter* property_converter,
                  const std::vector<uint8_t>& transport_data) {
  using ui::mojom::WindowManager;
  aura::PropertyConverter::PrimitiveType show_state = 0;
  return property_converter->GetPropertyValueFromTransportValue(
             WindowManager::kShowState_Property, transport_data, &show_state) &&
         (static_cast<ui::WindowShowState>(show_state) ==
          ui::SHOW_STATE_FULLSCREEN);
}

bool ShouldRenderTitleArea(
    aura::PropertyConverter* property_converter,
    const std::map<std::string, std::vector<uint8_t>>& properties) {
  auto iter = properties.find(
      ui::mojom::WindowManager::kRenderParentTitleArea_Property);
  if (iter == properties.end())
    return false;

  aura::PropertyConverter::PrimitiveType value = 0;
  return property_converter->GetPropertyValueFromTransportValue(
             ui::mojom::WindowManager::kRenderParentTitleArea_Property,
             iter->second, &value) &&
         value == 1;
}

// Returns the RootWindowController where new top levels are created.
// |properties| is the properties supplied during window creation.
RootWindowController* GetRootWindowControllerForNewTopLevelWindow(
    std::map<std::string, std::vector<uint8_t>>* properties) {
  // If a specific display was requested, use it.
  const int64_t display_id = GetInitialDisplayId(*properties);
  if (display_id != display::kInvalidDisplayId) {
    for (RootWindowController* root_window_controller :
         RootWindowController::root_window_controllers()) {
      if (GetRootWindowSettings(root_window_controller->GetRootWindow())
              ->display_id == display_id) {
        return root_window_controller;
      }
    }
  }
  return RootWindowController::ForWindow(Shell::GetRootWindowForNewWindows());
}

// Returns the bounds for the new window.
gfx::Rect CalculateDefaultBounds(
    RootWindowController* root_window_controller,
    aura::Window* container_window,
    aura::PropertyConverter* property_converter,
    const std::map<std::string, std::vector<uint8_t>>* properties) {
  gfx::Rect requested_bounds;
  if (GetInitialBounds(*properties, &requested_bounds))
    return requested_bounds;

  const gfx::Size root_size =
      root_window_controller->GetRootWindow()->bounds().size();
  auto show_state_iter =
      properties->find(ui::mojom::WindowManager::kShowState_Property);
  if (show_state_iter != properties->end()) {
    if (IsFullscreen(property_converter, show_state_iter->second)) {
      gfx::Rect bounds(root_size);
      if (!container_window) {
        const display::Display display =
            display::Screen::GetScreen()->GetDisplayNearestWindow(
                root_window_controller->GetRootWindow());
        bounds.Offset(display.bounds().OffsetFromOrigin());
      }
      return bounds;
    }
  }

  gfx::Size window_size;
  if (GetWindowPreferredSize(*properties, &window_size) &&
      !window_size.IsEmpty()) {
    // TODO(sky): likely want to constrain more than root size.
    window_size.SetToMin(root_size);
  } else {
    static constexpr int kRootSizeDelta = 240;
    window_size.SetSize(root_size.width() - kRootSizeDelta,
                        root_size.height() - kRootSizeDelta);
  }
  // TODO(sky): this should use code in chrome/browser/ui/window_sizer.
  static constexpr int kOriginOffset = 40;
  return gfx::Rect(gfx::Point(kOriginOffset, kOriginOffset), window_size);
}

// Does the real work of CreateAndParentTopLevelWindow() once the appropriate
// RootWindowController was found.
aura::Window* CreateAndParentTopLevelWindowInRoot(
    aura::WindowManagerClient* window_manager_client,
    RootWindowController* root_window_controller,
    ui::mojom::WindowType window_type,
    aura::PropertyConverter* property_converter,
    std::map<std::string, std::vector<uint8_t>>* properties) {
  // TODO(sky): constrain and validate properties.

  int32_t container_id = kShellWindowId_Invalid;
  aura::Window* context = nullptr;
  aura::Window* container_window = nullptr;
  if (GetInitialContainerId(*properties, &container_id)) {
    container_window =
        root_window_controller->GetRootWindow()->GetChildById(container_id);
  } else {
    context = root_window_controller->GetRootWindow();
  }

  gfx::Rect bounds = CalculateDefaultBounds(
      root_window_controller, container_window, property_converter, properties);

  const bool provide_non_client_frame =
      window_type == ui::mojom::WindowType::WINDOW ||
      window_type == ui::mojom::WindowType::PANEL;
  if (provide_non_client_frame) {
    // See NonClientFrameController for details on lifetime.
    NonClientFrameController* non_client_frame_controller =
        new NonClientFrameController(container_window, context, bounds,
                                     window_type, property_converter,
                                     properties, window_manager_client);
    return non_client_frame_controller->window();
  }

  if (window_type == ui::mojom::WindowType::POPUP &&
      ShouldRenderTitleArea(property_converter, *properties)) {
    // Pick a parent so display information is obtained. Will pick the real one
    // once transient parent found.
    aura::Window* unparented_control_container =
        root_window_controller->GetRootWindow()->GetChildById(
            kShellWindowId_UnparentedControlContainer);
    // DetachedTitleAreaRendererForClient is owned by the client.
    DetachedTitleAreaRendererForClient* renderer =
        new DetachedTitleAreaRendererForClient(unparented_control_container,
                                               property_converter, properties);
    return renderer->widget()->GetNativeView();
  }

  aura::Window* window = new aura::Window(nullptr);
  aura::SetWindowType(window, window_type);
  window->SetProperty(aura::client::kEmbedType,
                      aura::client::WindowEmbedType::TOP_LEVEL_IN_WM);
  ApplyProperties(window, property_converter, *properties);
  window->Init(ui::LAYER_TEXTURED);
  window->SetBounds(bounds);

  if (container_window) {
    container_window->AddChild(window);
  } else {
    aura::Window* root = root_window_controller->GetRootWindow();
    gfx::Point origin;
    aura::Window::ConvertPointToTarget(root, root->GetRootWindow(), &origin);
    const display::Display display =
        display::Screen::GetScreen()->GetDisplayNearestWindow(
            root_window_controller->GetRootWindow());
    origin += display.bounds().OffsetFromOrigin();
    gfx::Rect bounds_in_screen(origin, bounds.size());
    ash::wm::GetDefaultParent(window, bounds_in_screen)->AddChild(window);
  }
  return window;
}

}  // namespace

aura::Window* CreateAndParentTopLevelWindow(
    WindowManager* window_manager,
    ui::mojom::WindowType window_type,
    aura::PropertyConverter* property_converter,
    std::map<std::string, std::vector<uint8_t>>* properties) {
  RootWindowController* root_window_controller =
      GetRootWindowControllerForNewTopLevelWindow(properties);
  aura::Window* window = CreateAndParentTopLevelWindowInRoot(
      window_manager ? window_manager->window_manager_client() : nullptr,
      root_window_controller, window_type, property_converter, properties);
  DisconnectedAppHandler::Create(window);

  auto ignored_by_shelf_iter = properties->find(
      ui::mojom::WindowManager::kWindowIgnoredByShelf_InitProperty);
  if (ignored_by_shelf_iter != properties->end()) {
    wm::WindowState* window_state = wm::GetWindowState(window);
    window_state->set_ignored_by_shelf(
        mojo::ConvertTo<bool>(ignored_by_shelf_iter->second));
    // No need to persist this value.
    properties->erase(ignored_by_shelf_iter);
  }

  auto focusable_iter =
      properties->find(ui::mojom::WindowManager::kFocusable_InitProperty);
  if (focusable_iter != properties->end()) {
    bool can_focus = mojo::ConvertTo<bool>(focusable_iter->second);
    // TODO(crbug.com/842301): Add support for window-service as a library.
    if (window_manager)
      window_manager->window_tree_client()->SetCanFocus(window, can_focus);
    NonClientFrameController* non_client_frame_controller =
        NonClientFrameController::Get(window);
    if (non_client_frame_controller)
      non_client_frame_controller->set_can_activate(can_focus);
    // No need to persist this value.
    properties->erase(focusable_iter);
  }

  auto translucent_iter =
      properties->find(ui::mojom::WindowManager::kTranslucent_InitProperty);
  if (translucent_iter != properties->end()) {
    bool translucent = mojo::ConvertTo<bool>(translucent_iter->second);
    window->SetTransparent(translucent);
    // No need to persist this value.
    properties->erase(translucent_iter);
  }

  return window;
}

}  // namespace ash
