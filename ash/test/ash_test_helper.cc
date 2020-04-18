// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/ash_test_helper.h"

#include <algorithm>
#include <memory>
#include <set>
#include <utility>

#include "ash/display/display_configuration_controller_test_api.h"
#include "ash/display/screen_ash.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/shell_init_params.h"
#include "ash/shell_port.h"
#include "ash/shell_port_classic.h"
#include "ash/shell_port_mash.h"
#include "ash/shell_port_mus.h"
#include "ash/system/screen_layout_observer.h"
#include "ash/test/ash_test_environment.h"
#include "ash/test/ash_test_views_delegate.h"
#include "ash/test_shell_delegate.h"
#include "ash/window_manager.h"
#include "ash/window_manager_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "chromeos/audio/cras_audio_handler.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_policy_controller.h"
#include "chromeos/network/network_handler.h"
#include "components/prefs/testing_pref_service.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "ui/aura/env.h"
#include "ui/aura/input_state_lookup.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/test/env_test_helper.h"
#include "ui/aura/test/event_generator_delegate_aura.h"
#include "ui/aura/test/mus/window_tree_client_private.h"
#include "ui/base/ime/input_method_initializer.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/platform_window_defaults.h"
#include "ui/base/test/material_design_controller_test_api.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches_util.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/compositor/test/context_factories_for_test.h"
#include "ui/display/display.h"
#include "ui/display/display_switches.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/wm/core/capture_controller.h"
#include "ui/wm/core/cursor_manager.h"
#include "ui/wm/core/wm_state.h"

namespace ash {

// static
Config AshTestHelper::config_ = Config::CLASSIC;

AshTestHelper::AshTestHelper(AshTestEnvironment* ash_test_environment)
    : ash_test_environment_(ash_test_environment),
      command_line_(std::make_unique<base::test::ScopedCommandLine>()) {
  ui::test::EnableTestConfigForPlatformWindows();
  aura::test::InitializeAuraEventGeneratorDelegate();
}

AshTestHelper::~AshTestHelper() {
  // Ensure the next test starts with a null display::Screen. Done here because
  // some tests use Screen after TearDown().
  ScreenAsh::DeleteScreenForShutdown();
}

void AshTestHelper::SetUp(bool start_session, bool provide_local_state) {
  // TODO(jamescook): Can we do this without changing command line?
  // Use the origin (1,1) so that it doesn't over
  // lap with the native mouse cursor.
  if (!command_line_->GetProcessCommandLine()->HasSwitch(
          ::switches::kHostWindowBounds)) {
    command_line_->GetProcessCommandLine()->AppendSwitchASCII(
        ::switches::kHostWindowBounds, "1+1-800x600");
  }

  // TODO(wutao): We enabled a smooth screen rotation animation, which is using
  // an asynchronous method. However for some tests require to evaluate the
  // screen rotation immediately after the operation of setting display
  // rotation, we need to append a slow screen rotation animation flag to pass
  // the tests. When we remove the flag "ash-disable-smooth-screen-rotation", we
  // need to disable the screen rotation animation in the test.
  if (!command_line_->GetProcessCommandLine()->HasSwitch(
          switches::kAshDisableSmoothScreenRotation)) {
    command_line_->GetProcessCommandLine()->AppendSwitch(
        switches::kAshDisableSmoothScreenRotation);
  }

  display::ResetDisplayIdForTest();
  if (config_ != Config::CLASSIC)
    aura::test::EnvTestHelper().SetAlwaysUseLastMouseLocation(true);
  // WindowManager creates WMState for mash.
  if (config_ == Config::CLASSIC)
    wm_state_ = std::make_unique<::wm::WMState>();
  test_views_delegate_ = ash_test_environment_->CreateViewsDelegate();

  // Disable animations during tests.
  zero_duration_mode_.reset(new ui::ScopedAnimationDurationScaleMode(
      ui::ScopedAnimationDurationScaleMode::ZERO_DURATION));
  ui::InitializeInputMethodForTesting();

  // Creates Shell and hook with Desktop.
  if (!test_shell_delegate_)
    test_shell_delegate_ = new TestShellDelegate;

  if (config_ == Config::CLASSIC) {
    // All of this initialization is done in WindowManagerService for mash.

    if (!chromeos::DBusThreadManager::IsInitialized()) {
      chromeos::DBusThreadManager::Initialize(
          chromeos::DBusThreadManager::kShared);
      dbus_thread_manager_initialized_ = true;
    }

    if (!bluez::BluezDBusManager::IsInitialized()) {
      bluez::BluezDBusManager::Initialize(
          chromeos::DBusThreadManager::Get()->GetSystemBus(),
          chromeos::DBusThreadManager::Get()->IsUsingFakes());
      bluez_dbus_manager_initialized_ = true;
    }

    if (!chromeos::PowerPolicyController::IsInitialized()) {
      chromeos::PowerPolicyController::Initialize(
          chromeos::DBusThreadManager::Get()->GetPowerManagerClient());
      power_policy_controller_initialized_ = true;
    }

    // Create CrasAudioHandler for testing since g_browser_process is not
    // created in AshTestBase tests.
    chromeos::CrasAudioHandler::InitializeForTesting();
    chromeos::SystemSaltGetter::Initialize();
  }

  ash_test_environment_->SetUp();
  // Reset the global state for the cursor manager. This includes the
  // last cursor visibility state, etc.
  ::wm::CursorManager::ResetCursorVisibilityStateForTest();

  // ContentTestSuiteBase might have already initialized
  // MaterialDesignController in unit_tests suite.
  ui::test::MaterialDesignControllerTestAPI::Uninitialize();
  ui::MaterialDesignController::Initialize();

  if (config_ != Config::CLASSIC)
    CreateMashWindowManager();
  else
    CreateShell();

  aura::test::EnvTestHelper().SetInputStateLookup(
      std::unique_ptr<aura::InputStateLookup>());

  Shell* shell = Shell::Get();

  // Cursor is visible by default in tests.
  // CursorManager is null on MASH.
  if (shell->cursor_manager())
    shell->cursor_manager()->ShowCursor();

  if (provide_local_state) {
    auto pref_service = std::make_unique<TestingPrefServiceSimple>();
    Shell::RegisterLocalStatePrefs(pref_service->registry(), true);
    Shell::Get()->OnLocalStatePrefServiceInitialized(std::move(pref_service));
  }

  session_controller_client_.reset(
      new TestSessionControllerClient(shell->session_controller()));
  session_controller_client_->InitializeAndBind();

  if (start_session) {
    session_controller_client_->CreatePredefinedUserSessions(1);
  }

  // Tests that change the display configuration generally don't care about
  // the notifications and the popup UI can interfere with things like
  // cursors.
  shell->screen_layout_observer()->set_show_notifications_for_testing(false);

  display::test::DisplayManagerTestApi(shell->display_manager())
      .DisableChangeDisplayUponHostResize();
  DisplayConfigurationControllerTestApi(
      shell->display_configuration_controller())
      .DisableDisplayAnimator();

  app_list_test_helper_ = std::make_unique<AppListTestHelper>();
}

void AshTestHelper::TearDown() {
  app_list_test_helper_.reset();

  window_manager_service_.reset();

  // WindowManger owns the Shell in mash.
  if (config_ == Config::CLASSIC)
    Shell::DeleteInstance();

  // Suspend the tear down until all resources are returned via
  // CompositorFrameSinkClient::ReclaimResources()
  RunAllPendingInMessageLoop();
  ash_test_environment_->TearDown();

  if (config_ == Config::CLASSIC) {
    chromeos::SystemSaltGetter::Shutdown();
    chromeos::CrasAudioHandler::Shutdown();
  }

  if (power_policy_controller_initialized_) {
    chromeos::PowerPolicyController::Shutdown();
    power_policy_controller_initialized_ = false;
  }

  if (bluez_dbus_manager_initialized_) {
    device::BluetoothAdapterFactory::Shutdown();
    bluez::BluezDBusManager::Shutdown();
    bluez_dbus_manager_initialized_ = false;
  }

  if (dbus_thread_manager_initialized_) {
    chromeos::DBusThreadManager::Shutdown();
    dbus_thread_manager_initialized_ = false;
  }

  if (config_ == Config::CLASSIC)
    ui::TerminateContextFactoryForTests();

  ui::ShutdownInputMethodForTesting();
  zero_duration_mode_.reset();

  test_views_delegate_.reset();
  wm_state_.reset();

  command_line_.reset();

  display::Display::ResetForceDeviceScaleFactorForTesting();
  env_window_tree_client_setter_.reset();

  // WindowManager owns the CaptureController for mus/mash.
  CHECK(config_ != Config::CLASSIC || !::wm::CaptureController::Get());
}

void AshTestHelper::RunAllPendingInMessageLoop() {
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();
}

void AshTestHelper::NotifyClientAboutAcceleratedWidgets() {
  if (config_ == Config::CLASSIC)
    return;
  if (base::FeatureList::IsEnabled(features::kMash))
    return;
  // TODO(crbug.com/841941): Remove this. Config::MUS is deprecated.
  Shell* shell = Shell::Get();
  window_tree_client_setup_.NotifyClientAboutAcceleratedWidgets(
      shell->display_manager());
}

PrefService* AshTestHelper::GetLocalStatePrefService() {
  return Shell::Get()->local_state_.get();
}

aura::Window* AshTestHelper::CurrentContext() {
  aura::Window* root_window = Shell::GetRootWindowForNewWindows();
  if (!root_window)
    root_window = Shell::GetPrimaryRootWindow();
  DCHECK(root_window);
  return root_window;
}

display::Display AshTestHelper::GetSecondaryDisplay() {
  return Shell::Get()->display_manager()->GetSecondaryDisplay();
}

void AshTestHelper::CreateMashWindowManager() {
  CHECK_EQ(config_, Config::MASH);
  const bool show_primary_root_on_connect = false;
  window_manager_service_ =
      std::make_unique<WindowManagerService>(show_primary_root_on_connect);

  window_manager_service_->window_manager_.reset(
      new WindowManager(nullptr, show_primary_root_on_connect));
  window_manager_service_->window_manager()->shell_delegate_.reset(
      test_shell_delegate_);

  window_tree_client_setup_.InitForWindowManager(
      window_manager_service_->window_manager_.get(),
      window_manager_service_->window_manager_.get());
  env_window_tree_client_setter_ =
      std::make_unique<aura::test::EnvWindowTreeClientSetter>(
          window_tree_client_setup_.window_tree_client());
  // Classic ash does not start the NetworkHandler in tests, so don't start it
  // for mash either. The NetworkHandler may cause subtle side effects (such as
  // additional tray items) that can make for flaky tests.
  const bool init_network_handler = false;
  window_manager_service_->InitWindowManager(
      window_tree_client_setup_.OwnWindowTreeClient(), init_network_handler);

  aura::WindowTreeClient* window_tree_client =
      window_manager_service_->window_manager()->window_tree_client();
  window_tree_client_private_ =
      std::make_unique<aura::WindowTreeClientPrivate>(window_tree_client);
  window_tree_client_private_->CallOnConnect();
  NotifyClientAboutAcceleratedWidgets();
}

void AshTestHelper::CreateShell() {
  CHECK(config_ == Config::CLASSIC);
  ui::ContextFactory* context_factory = nullptr;
  ui::ContextFactoryPrivate* context_factory_private = nullptr;
  bool enable_pixel_output = false;
  ui::InitializeContextFactoryForTests(enable_pixel_output, &context_factory,
                                       &context_factory_private);
  ShellInitParams init_params;
  init_params.shell_port = std::make_unique<ash::ShellPortClassic>();
  init_params.delegate.reset(test_shell_delegate_);
  init_params.context_factory = context_factory;
  init_params.context_factory_private = context_factory_private;
  Shell::CreateInstance(std::move(init_params));
}

}  // namespace ash
