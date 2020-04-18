// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell_port_mus.h"

#include <memory>
#include <utility>

#include "ash/accelerators/accelerator_controller.h"
#include "ash/display/display_synchronizer.h"
#include "ash/host/ash_window_tree_host_init_params.h"
#include "ash/host/ash_window_tree_host_mus.h"
#include "ash/host/ash_window_tree_host_mus_mirroring_unified.h"
#include "ash/host/ash_window_tree_host_mus_unified.h"
#include "ash/keyboard/keyboard_ui_mash.h"
#include "ash/pointer_watcher_adapter_classic.h"
#include "ash/public/cpp/config.h"
#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/root_window_controller.h"
#include "ash/root_window_settings.h"
#include "ash/shell.h"
#include "ash/touch/touch_transform_setter_mus.h"
#include "ash/virtual_keyboard_controller.h"
#include "ash/window_manager.h"
#include "ash/wm/drag_window_resizer.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/tablet_mode/tablet_mode_event_handler_classic.h"
#include "ash/wm/window_cycle_event_filter_classic.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/window_util.h"
#include "ash/wm/workspace/workspace_event_handler_classic.h"
#include "base/memory/ptr_util.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/public/interfaces/video_detector.mojom.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/focus_synchronizer.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/mus/window_tree_host_mus_init_params.h"
#include "ui/aura/window.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches_util.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/forwarding_display_delegate.h"
#include "ui/display/types/native_display_delegate.h"

namespace ash {

ShellPortMus::ShellPortMus(WindowManager* window_manager)
    : window_manager_(window_manager) {
  DCHECK_EQ(Config::MUS, GetAshConfig());
}

ShellPortMus::~ShellPortMus() = default;

// static
ShellPortMus* ShellPortMus::Get() {
  const ash::Config config = ShellPort::Get()->GetAshConfig();
  CHECK(config == Config::MUS);
  return static_cast<ShellPortMus*>(ShellPort::Get());
}

RootWindowController* ShellPortMus::GetRootWindowControllerWithDisplayId(
    int64_t id) {
  for (RootWindowController* root_window_controller :
       RootWindowController::root_window_controllers()) {
    RootWindowSettings* settings =
        GetRootWindowSettings(root_window_controller->GetRootWindow());
    DCHECK(settings);
    if (settings->display_id == id)
      return root_window_controller;
  }
  return nullptr;
}

aura::WindowTreeClient* ShellPortMus::window_tree_client() {
  return window_manager_->window_tree_client();
}

void ShellPortMus::Shutdown() {
  display_synchronizer_.reset();

  pointer_watcher_adapter_.reset();

  ShellPort::Shutdown();
}

Config ShellPortMus::GetAshConfig() const {
  return Config::MUS;
}

std::unique_ptr<display::TouchTransformSetter>
ShellPortMus::CreateTouchTransformDelegate() {
  return std::make_unique<TouchTransformSetterMus>(
      window_manager_->connector());
}

void ShellPortMus::LockCursor() {
  // When we are running in mus, we need to keep track of state not just in the
  // window server, but also locally in ash because ash treats the cursor
  // manager as the canonical state for now. NativeCursorManagerAsh will keep
  // this state, while also forwarding it to the window manager for us.
  Shell::Get()->cursor_manager()->LockCursor();
}

void ShellPortMus::UnlockCursor() {
  Shell::Get()->cursor_manager()->UnlockCursor();
}

void ShellPortMus::ShowCursor() {
  Shell::Get()->cursor_manager()->ShowCursor();
}

void ShellPortMus::HideCursor() {
  Shell::Get()->cursor_manager()->HideCursor();
}

void ShellPortMus::SetCursorSize(ui::CursorSize cursor_size) {
  Shell::Get()->cursor_manager()->SetCursorSize(cursor_size);
}

void ShellPortMus::SetGlobalOverrideCursor(
    base::Optional<ui::CursorData> cursor) {
  NOTREACHED();
}

bool ShellPortMus::IsMouseEventsEnabled() {
  return Shell::Get()->cursor_manager()->IsMouseEventsEnabled();
}

void ShellPortMus::SetCursorTouchVisible(bool enabled) {
  NOTREACHED();
}

std::unique_ptr<WindowResizer> ShellPortMus::CreateDragWindowResizer(
    std::unique_ptr<WindowResizer> next_window_resizer,
    wm::WindowState* window_state) {
  return base::WrapUnique(ash::DragWindowResizer::Create(
      next_window_resizer.release(), window_state));
}

std::unique_ptr<WindowCycleEventFilter>
ShellPortMus::CreateWindowCycleEventFilter() {
  return std::make_unique<WindowCycleEventFilterClassic>();
}

std::unique_ptr<wm::TabletModeEventHandler>
ShellPortMus::CreateTabletModeEventHandler() {
  return std::make_unique<wm::TabletModeEventHandlerClassic>();
}

std::unique_ptr<WorkspaceEventHandler>
ShellPortMus::CreateWorkspaceEventHandler(aura::Window* workspace_window) {
  return std::make_unique<WorkspaceEventHandlerClassic>(workspace_window);
}

std::unique_ptr<KeyboardUI> ShellPortMus::CreateKeyboardUI() {
  return KeyboardUI::Create();
}

void ShellPortMus::AddPointerWatcher(views::PointerWatcher* watcher,
                                     views::PointerWatcherEventTypes events) {
  pointer_watcher_adapter_->AddPointerWatcher(watcher, events);
}

void ShellPortMus::RemovePointerWatcher(views::PointerWatcher* watcher) {
  pointer_watcher_adapter_->RemovePointerWatcher(watcher);
}

bool ShellPortMus::IsTouchDown() {
  return aura::Env::GetInstance()->is_touch_down();
}

void ShellPortMus::ToggleIgnoreExternalKeyboard() {
  Shell::Get()->virtual_keyboard_controller()->ToggleIgnoreExternalKeyboard();
}

void ShellPortMus::CreatePointerWatcherAdapter() {
  // In Config::MUS PointerWatcherAdapterClassic must be created when this
  // function is called (it is order dependent), that is not the case with
  // Config::MASH.
  pointer_watcher_adapter_ = std::make_unique<PointerWatcherAdapterClassic>();
}

std::unique_ptr<AshWindowTreeHost> ShellPortMus::CreateAshWindowTreeHost(
    const AshWindowTreeHostInitParams& init_params) {
  std::unique_ptr<aura::DisplayInitParams> display_params =
      std::make_unique<aura::DisplayInitParams>();
  display_params->viewport_metrics.bounds_in_pixels =
      init_params.initial_bounds;
  display_params->viewport_metrics.device_scale_factor =
      init_params.device_scale_factor;
  display_params->viewport_metrics.ui_scale_factor =
      init_params.ui_scale_factor;
  display::DisplayManager* display_manager = Shell::Get()->display_manager();
  display::Display mirrored_display =
      display_manager->GetMirroringDisplayById(init_params.display_id);
  if (mirrored_display.is_valid()) {
    display_params->display =
        std::make_unique<display::Display>(mirrored_display);
  }
  display_params->is_primary_display = true;
  display_params->mirrors = display_manager->software_mirroring_display_list();
  aura::WindowTreeHostMusInitParams aura_init_params =
      window_manager_->window_manager_client()->CreateInitParamsForNewDisplay();
  aura_init_params.display_id = init_params.display_id;
  aura_init_params.display_init_params = std::move(display_params);
  aura_init_params.use_classic_ime = !Shell::ShouldUseIMEService();

  if (!base::FeatureList::IsEnabled(features::kMash)) {
    if (init_params.mirroring_unified) {
      return std::make_unique<AshWindowTreeHostMusMirroringUnified>(
          std::move(aura_init_params), init_params.display_id,
          init_params.mirroring_delegate);
    }
    if (init_params.offscreen) {
      return std::make_unique<AshWindowTreeHostMusUnified>(
          std::move(aura_init_params), init_params.mirroring_delegate);
    }
  }

  return std::make_unique<AshWindowTreeHostMus>(std::move(aura_init_params));
}

void ShellPortMus::OnCreatedRootWindowContainers(
    RootWindowController* root_window_controller) {
  // TODO: To avoid lots of IPC AddActivationParent() should take an array.
  // http://crbug.com/682048.
  aura::Window* root_window = root_window_controller->GetRootWindow();
  for (size_t i = 0; i < kNumActivatableShellWindowIds; ++i) {
    window_manager_->window_manager_client()->AddActivationParent(
        root_window->GetChildById(kActivatableShellWindowIds[i]));
  }

  UpdateSystemModalAndBlockingContainers();
}

void ShellPortMus::UpdateSystemModalAndBlockingContainers() {
  std::vector<aura::BlockingContainers> all_blocking_containers;
  for (RootWindowController* root_window_controller :
       Shell::GetAllRootWindowControllers()) {
    aura::BlockingContainers blocking_containers;
    wm::GetBlockingContainersForRoot(
        root_window_controller->GetRootWindow(),
        &blocking_containers.min_container,
        &blocking_containers.system_modal_container);
    all_blocking_containers.push_back(blocking_containers);
  }
  window_manager_->window_manager_client()->SetBlockingContainers(
      all_blocking_containers);
}

void ShellPortMus::OnHostsInitialized() {
  display_synchronizer_ = std::make_unique<DisplaySynchronizer>(
      window_manager_->window_manager_client());
}

std::unique_ptr<display::NativeDisplayDelegate>
ShellPortMus::CreateNativeDisplayDelegate() {
  display::mojom::NativeDisplayDelegatePtr native_display_delegate;
  if (window_manager_->connector()) {
    window_manager_->connector()->BindInterface(ui::mojom::kServiceName,
                                                &native_display_delegate);
  }
  return std::make_unique<display::ForwardingDisplayDelegate>(
      std::move(native_display_delegate));
}

std::unique_ptr<AcceleratorController>
ShellPortMus::CreateAcceleratorController() {
  return std::make_unique<AcceleratorController>(nullptr);
}

void ShellPortMus::AddVideoDetectorObserver(
    viz::mojom::VideoDetectorObserverPtr observer) {
  if (base::FeatureList::IsEnabled(features::kMash)) {
    // We may not have access to the connector in unit tests.
    if (!window_manager_->connector())
      return;

    ui::mojom::VideoDetectorPtr video_detector;
    window_manager_->connector()->BindInterface(ui::mojom::kServiceName,
                                                &video_detector);
    video_detector->AddObserver(std::move(observer));
  } else {
    aura::Env::GetInstance()
        ->context_factory_private()
        ->GetHostFrameSinkManager()
        ->AddVideoDetectorObserver(std::move(observer));
  }
}

}  // namespace ash
