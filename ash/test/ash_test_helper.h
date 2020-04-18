// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TEST_ASH_TEST_HELPER_H_
#define ASH_TEST_ASH_TEST_HELPER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ash/app_list/test/app_list_test_helper.h"
#include "ash/session/test_session_controller_client.h"
#include "base/macros.h"
#include "base/test/scoped_command_line.h"
#include "ui/aura/test/mus/test_window_tree_client_setup.h"

class PrefService;

namespace aura {
class Window;
class WindowTreeClientPrivate;
namespace test {
class EnvWindowTreeClientSetter;
}
}

namespace display {
class Display;
}

namespace mash {
namespace test {
class MashTestSuite;
}
}

namespace ui {
class ScopedAnimationDurationScaleMode;
}

namespace wm {
class WMState;
}

namespace ash {

class AshTestEnvironment;
class AshTestViewsDelegate;
class RootWindowController;
class TestShellDelegate;
class TestSessionControllerClient;
class WindowManagerService;

enum class Config;

// A helper class that does common initialization required for Ash. Creates a
// root window and an ash::Shell instance with a test delegate.
class AshTestHelper {
 public:
  explicit AshTestHelper(AshTestEnvironment* ash_test_environment);
  ~AshTestHelper();

  // This is intended to be called from TestSuites, not individual configs.
  static void set_config(Config config) { config_ = config; }

  // Returns the configuration that tests are run in. See ash::Config enum for
  // details.
  static Config config() { return config_; }

  // Creates the ash::Shell and performs associated initialization.  Set
  // |start_session| to true if the user should log in before the test is run.
  // Set |provide_local_state| to true to inject local-state PrefService into
  // the Shell before the test is run.
  void SetUp(bool start_session, bool provide_local_state = true);

  // Destroys the ash::Shell and performs associated cleanup.
  void TearDown();

  // Returns a root Window. Usually this is the active root Window, but that
  // method can return NULL sometimes, and in those cases, we fall back on the
  // primary root Window.
  aura::Window* CurrentContext();

  void RunAllPendingInMessageLoop();

  void NotifyClientAboutAcceleratedWidgets();

  PrefService* GetLocalStatePrefService();

  TestShellDelegate* test_shell_delegate() { return test_shell_delegate_; }
  void set_test_shell_delegate(TestShellDelegate* test_shell_delegate) {
    test_shell_delegate_ = test_shell_delegate;
  }
  AshTestViewsDelegate* test_views_delegate() {
    return test_views_delegate_.get();
  }

  AshTestEnvironment* ash_test_environment() { return ash_test_environment_; }

  display::Display GetSecondaryDisplay();

  // Null in classic ash.
  WindowManagerService* window_manager_service() {
    return window_manager_service_.get();
  }
  aura::TestWindowTreeClientSetup* window_tree_client_setup() {
    return &window_tree_client_setup_;
  }

  TestSessionControllerClient* test_session_controller_client() {
    return session_controller_client_.get();
  }
  void set_test_session_controller_client(
      std::unique_ptr<TestSessionControllerClient> session_controller_client) {
    session_controller_client_ = std::move(session_controller_client);
  }

  AppListTestHelper* app_list_test_helper() {
    return app_list_test_helper_.get();
  }

  void reset_commandline() { command_line_.reset(); }

 private:
  // These TestSuites need to manipulate |config_|.
  friend class AshTestSuite;
  friend class mash::test::MashTestSuite;

  // Called when running in mash to create the WindowManager.
  void CreateMashWindowManager();

  // Called when running in ash to create Shell.
  void CreateShell();

  // Creates a new RootWindowController based on |display_spec|. The origin is
  // set to |next_x| and on exit |next_x| is set to the origin + the width.
  RootWindowController* CreateRootWindowController(
      const std::string& display_spec,
      int* next_x);

  static Config config_;

  std::unique_ptr<aura::test::EnvWindowTreeClientSetter>
      env_window_tree_client_setter_;
  AshTestEnvironment* ash_test_environment_;  // Not owned.
  TestShellDelegate* test_shell_delegate_ = nullptr;  // Owned by ash::Shell.
  std::unique_ptr<ui::ScopedAnimationDurationScaleMode> zero_duration_mode_;

  std::unique_ptr<::wm::WMState> wm_state_;
  std::unique_ptr<AshTestViewsDelegate> test_views_delegate_;

  // Check if DBus Thread Manager was initialized here.
  bool dbus_thread_manager_initialized_ = false;
  // Check if Bluez DBus Manager was initialized here.
  bool bluez_dbus_manager_initialized_ = false;
  // Check if PowerPolicyController was initialized here.
  bool power_policy_controller_initialized_ = false;

  aura::TestWindowTreeClientSetup window_tree_client_setup_;
  std::unique_ptr<WindowManagerService> window_manager_service_;
  std::unique_ptr<aura::WindowTreeClientPrivate> window_tree_client_private_;
  // Id for the next Display created by CreateRootWindowController().
  int64_t next_display_id_ = 1;

  std::unique_ptr<TestSessionControllerClient> session_controller_client_;

  std::unique_ptr<base::test::ScopedCommandLine> command_line_;

  std::unique_ptr<AppListTestHelper> app_list_test_helper_;

  DISALLOW_COPY_AND_ASSIGN(AshTestHelper);
};

}  // namespace ash

#endif  // ASH_TEST_ASH_TEST_HELPER_H_
