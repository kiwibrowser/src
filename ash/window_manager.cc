// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/window_manager.h"

#include <stdint.h>

#include <limits>
#include <memory>
#include <utility>

#include "ash/accelerators/accelerator_handler.h"
#include "ash/accelerators/accelerator_ids.h"
#include "ash/display/window_tree_host_manager.h"
#include "ash/drag_drop/drag_image_view.h"
#include "ash/event_matcher_util.h"
#include "ash/host/ash_window_tree_host.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/public/cpp/window_state_type.h"
#include "ash/public/interfaces/window_actions.mojom.h"
#include "ash/root_window_controller.h"
#include "ash/root_window_settings.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/shell_delegate_mus.h"
#include "ash/shell_init_params.h"
#include "ash/shell_port_mash.h"
#include "ash/shell_port_mus.h"
#include "ash/wm/ash_focus_rules.h"
#include "ash/wm/move_event_handler.h"
#include "ash/wm/non_client_frame_controller.h"
#include "ash/wm/property_util.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ash/wm/top_level_window_factory.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_state.h"
#include "components/exo/file_helper.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/common/accelerator_util.h"
#include "services/ui/common/types.h"
#include "services/ui/public/cpp/input_devices/input_device_client.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/window_parenting_client.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/capture_synchronizer.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/window.h"
#include "ui/base/class_property.h"
#include "ui/base/hit_test.h"
#include "ui/base/ui_base_features.h"
#include "ui/display/display_observer.h"
#include "ui/events/mojo/event.mojom.h"
#include "ui/views/mus/pointer_watcher_event_router.h"
#include "ui/wm/core/capture_controller.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/core/wm_state.h"
#include "ui/wm/public/activation_client.h"

namespace ash {

struct WindowManager::DragState {
  // An image representation of the contents of the current drag and drop
  // clipboard.
  std::unique_ptr<ash::DragImageView> view;

  // The cursor offset of the dragged item.
  gfx::Vector2d image_offset;
};

// TODO: need to register OSExchangeDataProviderMus. http://crbug.com/665077.
WindowManager::WindowManager(service_manager::Connector* connector,
                             bool show_primary_host_on_connect)
    : connector_(connector),
      show_primary_host_on_connect_(show_primary_host_on_connect),
      wm_state_(std::make_unique<::wm::WMState>()),
      property_converter_(std::make_unique<aura::PropertyConverter>()) {
  DCHECK(features::IsMashEnabled());
  RegisterWindowProperties(property_converter_.get());
}

WindowManager::~WindowManager() {
  Shutdown();
  ash::Shell::set_window_tree_client(nullptr);
  ash::Shell::set_window_manager_client(nullptr);
}

void WindowManager::Init(
    std::unique_ptr<aura::WindowTreeClient> window_tree_client,
    std::unique_ptr<base::Value> initial_display_prefs) {
  input_device_client_ = std::make_unique<ui::InputDeviceClient>();

  // |connector_| can be nullptr in tests.
  if (connector_) {
    ui::mojom::InputDeviceServerPtr server;
    connector_->BindInterface(ui::mojom::kServiceName, &server);
    input_device_client_->Connect(std::move(server));
  }

  DCHECK(window_manager_client_);
  DCHECK(!window_tree_client_);
  window_tree_client_ = std::move(window_tree_client);

  DCHECK_EQ(nullptr, ash::Shell::window_tree_client());
  ash::Shell::set_window_tree_client(window_tree_client_.get());

  // TODO(jamescook): Maybe not needed in Config::MUS?
  pointer_watcher_event_router_ =
      std::make_unique<views::PointerWatcherEventRouter>(
          window_tree_client_.get());

  // Notify PointerWatcherEventRouter and CaptureSynchronizer that the capture
  // client has been set.
  aura::client::CaptureClient* capture_client = wm_state_->capture_controller();
  pointer_watcher_event_router_->AttachToCaptureClient(capture_client);
  window_tree_client_->capture_synchronizer()->AttachToCaptureClient(
      capture_client);

  initial_display_prefs_ = std::move(initial_display_prefs);

  InitCursorOnKeyList();
}

bool WindowManager::GetNextAcceleratorNamespaceId(uint16_t* id) {
  if (accelerator_handlers_.size() == std::numeric_limits<uint16_t>::max())
    return false;
  while (accelerator_handlers_.count(next_accelerator_namespace_id_) > 0)
    ++next_accelerator_namespace_id_;
  *id = next_accelerator_namespace_id_;
  ++next_accelerator_namespace_id_;
  return true;
}

void WindowManager::AddAcceleratorHandler(uint16_t id_namespace,
                                          AcceleratorHandler* handler) {
  DCHECK_EQ(0u, accelerator_handlers_.count(id_namespace));
  accelerator_handlers_[id_namespace] = handler;
}

void WindowManager::RemoveAcceleratorHandler(uint16_t id_namespace) {
  accelerator_handlers_.erase(id_namespace);
}

display::mojom::DisplayController* WindowManager::GetDisplayController() {
  return display_controller_ ? display_controller_.get() : nullptr;
}

void WindowManager::CreateShell() {
  DCHECK(!created_shell_);
  created_shell_ = true;
  ShellInitParams init_params;
  init_params.shell_port = std::make_unique<ShellPortMash>(
      this, pointer_watcher_event_router_.get());
  init_params.delegate = shell_delegate_
                             ? std::move(shell_delegate_)
                             : std::make_unique<ShellDelegateMus>(connector_);
  init_params.initial_display_prefs = std::move(initial_display_prefs_);
  Shell::CreateInstance(std::move(init_params));
}

void WindowManager::InitCursorOnKeyList() {
  DCHECK(window_manager_client_);
  // In Mash, we build a list of keys and send them to the window
  // server. This controls which keys *don't* hide the cursors.

  // TODO(erg): This needs to also check the case of the accessibility
  // keyboard being shown, since clicking a key on the keyboard shouldn't
  // hide the cursor.
  std::vector<ui::mojom::EventMatcherPtr> cursor_key_list;
  cursor_key_list.push_back(BuildKeyReleaseMatcher());
  cursor_key_list.push_back(BuildAltMatcher());
  cursor_key_list.push_back(BuildControlMatcher());

  BuildKeyMatcherRange(ui::mojom::KeyboardCode::F1,
                       ui::mojom::KeyboardCode::F24, &cursor_key_list);
  BuildKeyMatcherRange(ui::mojom::KeyboardCode::BROWSER_BACK,
                       ui::mojom::KeyboardCode::MEDIA_LAUNCH_APP2,
                       &cursor_key_list);
  BuildKeyMatcherList(
      {ui::mojom::KeyboardCode::SHIFT, ui::mojom::KeyboardCode::CONTROL,
       ui::mojom::KeyboardCode::MENU,
       ui::mojom::KeyboardCode::LWIN,  // Search key == VKEY_LWIN.
       ui::mojom::KeyboardCode::WLAN, ui::mojom::KeyboardCode::POWER,
       ui::mojom::KeyboardCode::BRIGHTNESS_DOWN,
       ui::mojom::KeyboardCode::BRIGHTNESS_UP,
       ui::mojom::KeyboardCode::KBD_BRIGHTNESS_DOWN,
       ui::mojom::KeyboardCode::KBD_BRIGHTNESS_UP},
      &cursor_key_list);

  window_manager_client_->SetKeyEventsThatDontHideCursor(
      std::move(cursor_key_list));
}

void WindowManager::InstallFrameDecorationValues() {
  ui::mojom::FrameDecorationValuesPtr frame_decoration_values =
      ui::mojom::FrameDecorationValues::New();
  const gfx::Insets client_area_insets =
      NonClientFrameController::GetPreferredClientAreaInsets();
  frame_decoration_values->normal_client_area_insets = client_area_insets;
  frame_decoration_values->maximized_client_area_insets = client_area_insets;
  frame_decoration_values->max_title_bar_button_width =
      NonClientFrameController::GetMaxTitleBarButtonWidth();
  window_manager_client_->SetFrameDecorationValues(
      std::move(frame_decoration_values));
}

void WindowManager::Shutdown() {
  if (!window_tree_client_)
    return;

  aura::client::CaptureClient* capture_client = wm_state_->capture_controller();
  pointer_watcher_event_router_->DetachFromCaptureClient(capture_client);
  window_tree_client_->capture_synchronizer()->DetachFromCaptureClient(
      capture_client);

  Shell::DeleteInstance();

  pointer_watcher_event_router_.reset();

  window_tree_client_.reset();
  window_manager_client_ = nullptr;
}

void WindowManager::OnEmbed(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) {
  // WindowManager should never see this, instead OnWmNewDisplay() is called.
  NOTREACHED();
}

void WindowManager::OnEmbedRootDestroyed(
    aura::WindowTreeHostMus* window_tree_host) {
  // WindowManager should never see this.
  NOTREACHED();
}

void WindowManager::OnLostConnection(aura::WindowTreeClient* client) {
  DCHECK_EQ(client, window_tree_client_.get());
  Shutdown();
  // TODO(sky): this case should trigger shutting down WindowManagerService too.
}

void WindowManager::OnPointerEventObserved(const ui::PointerEvent& event,
                                           int64_t display_id,
                                           aura::Window* target) {
  pointer_watcher_event_router_->OnPointerEventObserved(event, display_id,
                                                        target);
}

aura::PropertyConverter* WindowManager::GetPropertyConverter() {
  return property_converter_.get();
}

void WindowManager::SetWindowManagerClient(aura::WindowManagerClient* client) {
  window_manager_client_ = client;
  ash::Shell::set_window_manager_client(client);
}

void WindowManager::OnWmConnected() {
  // InstallFrameDecorationValues() must be called before the shell is created,
  // otherwise Mus attempts to notify clients with no frame decorations, which
  // triggers validation errors.
  InstallFrameDecorationValues();
  CreateShell();
  if (show_primary_host_on_connect_)
    Shell::GetPrimaryRootWindow()->GetHost()->Show();

  // TODO(hirono): wire up the file helper. http://crbug.com/768395
  Shell::Get()->InitWaylandServer(nullptr);
}

void WindowManager::OnWmAcceleratedWidgetAvailableForDisplay(
    int64_t display_id,
    gfx::AcceleratedWidget widget) {
  WindowTreeHostManager* manager = Shell::Get()->window_tree_host_manager();
  AshWindowTreeHost* host =
      manager->GetAshWindowTreeHostForDisplayId(display_id);
  // The display may have been destroyed before getting this async callback.
  if (host && host->AsWindowTreeHost()) {
    static_cast<aura::WindowTreeHostMus*>(host->AsWindowTreeHost())
        ->OverrideAcceleratedWidget(widget);
  }
}

void WindowManager::OnWmSetBounds(aura::Window* window,
                                  const gfx::Rect& bounds) {
  // TODO(sky): this indirectly sets bounds, which is against what
  // OnWmSetBounds() recommends doing. Remove that restriction, or fix this.
  window->SetBounds(bounds);
}

bool WindowManager::OnWmSetProperty(
    aura::Window* window,
    const std::string& name,
    std::unique_ptr<std::vector<uint8_t>>* new_data) {
  if (property_converter_->IsTransportNameRegistered(name))
    return true;
  DVLOG(1) << "unknown property changed, ignoring " << name;
  return false;
}

void WindowManager::OnWmSetModalType(aura::Window* window, ui::ModalType type) {
  ui::ModalType old_type = window->GetProperty(aura::client::kModalKey);
  if (type == old_type)
    return;

  window->SetProperty(aura::client::kModalKey, type);
  // Assume the client has positioned the window in the right container and
  // don't attempt to change the parent. This is currently true for Chrome
  // code, but likely there should be checks. Adding checks is complicated by
  // the fact that assumptions in GetSystemModalContainer() are not always
  // valid for Chrome code. In particular Chrome code may create system modal
  // dialogs without a transient parent that Chrome wants shown on the lock
  // screen.
}

void WindowManager::OnWmSetCanFocus(aura::Window* window, bool can_focus) {
  NonClientFrameController* non_client_frame_controller =
      NonClientFrameController::Get(window);
  if (non_client_frame_controller)
    non_client_frame_controller->set_can_activate(can_focus);
}

aura::Window* WindowManager::OnWmCreateTopLevelWindow(
    ui::mojom::WindowType window_type,
    std::map<std::string, std::vector<uint8_t>>* properties) {
  if (window_type == ui::mojom::WindowType::UNKNOWN) {
    LOG(WARNING) << "Request to create top level of unknown type, failing";
    return nullptr;
  }

  return CreateAndParentTopLevelWindow(this, window_type,
                                       property_converter_.get(), properties);
}

void WindowManager::OnWmClientJankinessChanged(
    const std::set<aura::Window*>& client_windows,
    bool janky) {
  for (auto* window : client_windows)
    window->SetProperty(kWindowIsJanky, janky);
}

void WindowManager::OnWmBuildDragImage(const gfx::Point& screen_location,
                                       const SkBitmap& drag_image,
                                       const gfx::Vector2d& drag_image_offset,
                                       ui::mojom::PointerKind source) {
  if (drag_image.isNull())
    return;

  // TODO(erg): Get the right display for this drag image. Right now, none of
  // the drag drop code is multidisplay aware.
  aura::Window* root_window = Shell::GetPrimaryRootWindow();

  // TODO(erg): SkBitmap is the wrong data type for the drag image; we should
  // be passing ImageSkias once http://crbug.com/655874 is implemented.

  ui::DragDropTypes::DragEventSource ui_source =
      source == ui::mojom::PointerKind::MOUSE
          ? ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE
          : ui::DragDropTypes::DRAG_EVENT_SOURCE_TOUCH;
  std::unique_ptr<DragImageView> drag_view =
      std::make_unique<DragImageView>(root_window, ui_source);
  drag_view->SetImage(gfx::ImageSkia::CreateFrom1xBitmap(drag_image));
  gfx::Size size = drag_view->GetPreferredSize();
  gfx::Rect drag_image_bounds(screen_location - drag_image_offset, size);
  drag_view->SetBoundsInScreen(drag_image_bounds);
  drag_view->SetWidgetVisible(true);

  drag_state_ = std::make_unique<DragState>();
  drag_state_->view = std::move(drag_view);
  drag_state_->image_offset = drag_image_offset;
}

void WindowManager::OnWmMoveDragImage(const gfx::Point& screen_location) {
  if (drag_state_) {
    drag_state_->view->SetScreenPosition(screen_location -
                                         drag_state_->image_offset);
  }
}

void WindowManager::OnWmDestroyDragImage() {
  drag_state_.reset();
}

void WindowManager::OnWmWillCreateDisplay(const display::Display& display) {
  // Ash connects such that |automatically_create_display_roots| is false, which
  // means this should never be hit.
  NOTREACHED();
}

void WindowManager::OnWmNewDisplay(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host,
    const display::Display& display) {
  // Ash connects such that |automatically_create_display_roots| is false, which
  // means this should never be hit.
  NOTREACHED();
}

void WindowManager::OnWmDisplayRemoved(
    aura::WindowTreeHostMus* window_tree_host) {
  // Ash connects such that |automatically_create_display_roots| is false, which
  // means this should never be hit.
  NOTREACHED();
}

void WindowManager::OnWmDisplayModified(const display::Display& display) {
  // TODO(sky): this shouldn't be called as we're passing false for
  // |automatically_create_display_roots|, but it currently is. Update mus.
}

void WindowManager::OnWmPerformMoveLoop(
    aura::Window* window,
    ui::mojom::MoveLoopSource source,
    const gfx::Point& cursor_location,
    const base::Callback<void(bool)>& on_done) {
  MoveEventHandler* handler = MoveEventHandler::GetForWindow(window);
  if (!handler) {
    on_done.Run(false);
    return;
  }

  DCHECK(!handler->IsDragInProgress());
  ::wm::WindowMoveSource aura_source =
      source == ui::mojom::MoveLoopSource::MOUSE
          ? ::wm::WINDOW_MOVE_SOURCE_MOUSE
          : ::wm::WINDOW_MOVE_SOURCE_TOUCH;
  handler->AttemptToStartDrag(cursor_location, HTCAPTION, aura_source, on_done);
}

void WindowManager::OnWmCancelMoveLoop(aura::Window* window) {
  MoveEventHandler* handler = MoveEventHandler::GetForWindow(window);
  if (handler)
    handler->RevertDrag();
}

ui::mojom::EventResult WindowManager::OnAccelerator(
    uint32_t id,
    const ui::Event& event,
    base::flat_map<std::string, std::vector<uint8_t>>* properties) {
  auto iter = accelerator_handlers_.find(GetAcceleratorNamespaceId(id));
  if (iter == accelerator_handlers_.end())
    return ui::mojom::EventResult::HANDLED;

  return iter->second->OnAccelerator(id, event, properties);
}

void WindowManager::OnCursorTouchVisibleChanged(bool enabled) {
  ShellPortMash::Get()->OnCursorTouchVisibleChanged(enabled);
}

void WindowManager::OnWmSetClientArea(
    aura::Window* window,
    const gfx::Insets& insets,
    const std::vector<gfx::Rect>& additional_client_areas) {
  NonClientFrameController* non_client_frame_controller =
      NonClientFrameController::Get(window);
  if (!non_client_frame_controller)
    return;
  non_client_frame_controller->SetClientArea(insets, additional_client_areas);
}

bool WindowManager::IsWindowActive(aura::Window* window) {
  return Shell::Get()->activation_client()->GetActiveWindow() == window;
}

void WindowManager::OnWmDeactivateWindow(aura::Window* window) {
  Shell::Get()->activation_client()->DeactivateWindow(window);
}

void WindowManager::OnWmPerformAction(aura::Window* window,
                                      const std::string& action) {
  if (action == mojom::kAddWindowToTabletMode)
    ash::Shell::Get()->tablet_mode_controller()->AddWindow(window);
}

void WindowManager::OnEventBlockedByModalWindow(aura::Window* window) {
  AnimateWindow(window, ::wm::WINDOW_ANIMATION_TYPE_BOUNCE);
}

}  // namespace ash
