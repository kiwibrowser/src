// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/aura_test_helper.h"

#include "base/command_line.h"
#include "base/run_loop.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/env.h"
#include "ui/aura/input_state_lookup.h"
#include "ui/aura/local/layer_tree_frame_sink_local.h"
#include "ui/aura/mus/capture_synchronizer.h"
#include "ui/aura/mus/focus_synchronizer.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/test/env_test_helper.h"
#include "ui/aura/test/event_generator_delegate_aura.h"
#include "ui/aura/test/mus/test_window_manager_delegate.h"
#include "ui/aura/test/mus/test_window_tree.h"
#include "ui/aura/test/mus/test_window_tree_client_delegate.h"
#include "ui/aura/test/mus/test_window_tree_client_setup.h"
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/test/test_window_parenting_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_targeter.h"
#include "ui/base/ime/input_method_factory.h"
#include "ui/base/ime/input_method_initializer.h"
#include "ui/base/platform_window_defaults.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer_animator.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/display/screen.h"
#include "ui/wm/core/wm_state.h"

#if defined(USE_X11)
#include "ui/base/x/x11_util.h"  // nogncheck
#endif

namespace aura {
namespace test {
namespace {

AuraTestHelper* g_instance = nullptr;

}  // namespace

AuraTestHelper::AuraTestHelper()
    : setup_called_(false), teardown_called_(false) {
  // Disable animations during tests.
  zero_duration_mode_.reset(new ui::ScopedAnimationDurationScaleMode(
      ui::ScopedAnimationDurationScaleMode::ZERO_DURATION));
  ui::test::EnableTestConfigForPlatformWindows();
  InitializeAuraEventGeneratorDelegate();
}

AuraTestHelper::~AuraTestHelper() {
  CHECK(setup_called_)
      << "AuraTestHelper::SetUp() never called.";
  CHECK(teardown_called_)
      << "AuraTestHelper::TearDown() never called.";
}

// static
AuraTestHelper* AuraTestHelper::GetInstance() {
  return g_instance;
}

void AuraTestHelper::EnableMusWithTestWindowTree(
    WindowTreeClientDelegate* window_tree_delegate,
    WindowManagerDelegate* window_manager_delegate,
    WindowTreeClient::Config config) {
  DCHECK(!setup_called_);
  DCHECK_EQ(Mode::LOCAL, mode_);
  mode_ = (config == WindowTreeClient::Config::kMash)
              ? Mode::MUS_CREATE_WINDOW_TREE_CLIENT
              : Mode::MUS2_CREATE_WINDOW_TREE_CLIENT;
  window_tree_delegate_ = window_tree_delegate;
  window_manager_delegate_ = window_manager_delegate;
}

void AuraTestHelper::EnableMusWithWindowTreeClient(
    WindowTreeClient* window_tree_client) {
  DCHECK(!setup_called_);
  DCHECK_EQ(Mode::LOCAL, mode_);
  mode_ = Mode::MUS;
  window_tree_client_ = window_tree_client;
}

void AuraTestHelper::DeleteWindowTreeClient() {
  window_tree_client_setup_.reset();
  window_tree_client_ = nullptr;
}

void AuraTestHelper::SetUp(ui::ContextFactory* context_factory,
                           ui::ContextFactoryPrivate* context_factory_private) {
  // |mode_| defaults to LOCAL, but test suites may enable MUS. If this happens
  // enable mus.
  if (Env::GetInstanceDontCreate() &&
      Env::GetInstanceDontCreate()->mode() == Env::Mode::MUS &&
      mode_ == Mode::LOCAL) {
    test_window_tree_client_delegate_ =
        std::make_unique<TestWindowTreeClientDelegate>();
    test_window_manager_delegate_ =
        std::make_unique<TestWindowManagerDelegate>();
    EnableMusWithTestWindowTree(test_window_tree_client_delegate_.get(),
                                test_window_manager_delegate_.get());
  }

  setup_called_ = true;

  if (mode_ != Mode::MUS) {
    // Assume if an explicit WindowTreeClient was created then a WmState was
    // already created.
    wm_state_ = std::make_unique<wm::WMState>();
  }
  // Needs to be before creating WindowTreeClient.
  focus_client_ = std::make_unique<TestFocusClient>();
  capture_client_ = std::make_unique<client::DefaultCaptureClient>();
  const Env::Mode env_mode =
      (mode_ == Mode::LOCAL) ? Env::Mode::LOCAL : Env::Mode::MUS;

  if (mode_ == Mode::MUS_CREATE_WINDOW_TREE_CLIENT ||
      mode_ == Mode::MUS2_CREATE_WINDOW_TREE_CLIENT) {
    InitWindowTreeClient();
  }
  if (!Env::GetInstanceDontCreate())
    env_ = Env::CreateInstance(env_mode);
  else
    env_mode_to_restore_ = Env::GetInstance()->mode();
  EnvTestHelper env_helper;
  // Always reset the mode. This really only matters for if Env was created
  // above.
  env_helper.SetMode(env_mode);
  if (env_mode == Env::Mode::MUS) {
    env_window_tree_client_setter_ =
        std::make_unique<EnvWindowTreeClientSetter>(window_tree_client_);
  }
  // Tests assume they can set the mouse location on Env() and have it reflected
  // in tests.
  env_helper.SetAlwaysUseLastMouseLocation(true);
  context_factory_to_restore_ = Env::GetInstance()->context_factory();
  context_factory_private_to_restore_ =
      Env::GetInstance()->context_factory_private();
  Env::GetInstance()->set_context_factory(context_factory);
  Env::GetInstance()->set_context_factory_private(context_factory_private);
  // Unit tests generally don't want to query the system, rather use the state
  // from RootWindow.
  env_helper.SetInputStateLookup(nullptr);
  env_helper.ResetEventState();

  ui::InitializeInputMethodForTesting();

  if (mode_ != Mode::MUS) {
    display::Screen* screen = display::Screen::GetScreen();
    gfx::Size host_size(screen ? screen->GetPrimaryDisplay().GetSizeInPixel()
                               : gfx::Size(800, 600));
    // This must be reset before creating TestScreen, which sets up the display
    // scale factor for this test iteration.
    display::Display::ResetForceDeviceScaleFactorForTesting();
    test_screen_.reset(TestScreen::Create(host_size, window_tree_client_));
    if (!screen)
      display::Screen::SetScreenInstance(test_screen_.get());
    if (env_mode == Env::Mode::LOCAL || window_manager_delegate_) {
      host_.reset(test_screen_->CreateHostForPrimaryDisplay());
      host_->window()->SetEventTargeter(
          std::unique_ptr<ui::EventTargeter>(new WindowTargeter()));

      client::SetFocusClient(root_window(), focus_client_.get());
      client::SetCaptureClient(root_window(), capture_client());
      parenting_client_.reset(new TestWindowParentingClient(root_window()));

      root_window()->Show();
      // Ensure width != height so tests won't confuse them.
      host()->SetBoundsInPixels(gfx::Rect(host_size));
    }
  }

  if (mode_ == Mode::MUS_CREATE_WINDOW_TREE_CLIENT ||
      mode_ == Mode::MUS2_CREATE_WINDOW_TREE_CLIENT) {
    window_tree_client_->focus_synchronizer()->SetActiveFocusClient(
        focus_client_.get(), root_window());
    window_tree()->AckAllChanges();
  }

  g_instance = this;
}

void AuraTestHelper::TearDown() {
  g_instance = nullptr;
  teardown_called_ = true;
  parenting_client_.reset();
  env_window_tree_client_setter_.reset();
  if (mode_ != Mode::MUS && root_window()) {
    client::SetFocusClient(root_window(), nullptr);
    client::SetCaptureClient(root_window(), nullptr);
    host_.reset();

    if (display::Screen::GetScreen() == test_screen_.get())
      display::Screen::SetScreenInstance(nullptr);
    test_screen_.reset();

    window_tree_client_setup_.reset();
    focus_client_.reset();
    capture_client_.reset();
  } else {
    if (display::Screen::GetScreen() == test_screen_.get())
      display::Screen::SetScreenInstance(nullptr);
    test_screen_.reset();
    window_tree_client_setup_.reset();
  }
  ui::GestureRecognizer::Reset();
  ui::ShutdownInputMethodForTesting();

  if (env_) {
    env_.reset();
  } else {
    Env::GetInstance()->set_context_factory(context_factory_to_restore_);
    Env::GetInstance()->set_context_factory_private(
        context_factory_private_to_restore_);
    EnvTestHelper().SetMode(env_mode_to_restore_);
  }
  wm_state_.reset();
}

void AuraTestHelper::RunAllPendingInMessageLoop() {
  // TODO(jbates) crbug.com/134753 Find quitters of this RunLoop and have them
  //              use run_loop.QuitClosure().
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();
}

TestWindowTree* AuraTestHelper::window_tree() {
  return window_tree_client_setup_->window_tree();
}

WindowTreeClient* AuraTestHelper::window_tree_client() {
  return window_tree_client_;
}

client::CaptureClient* AuraTestHelper::capture_client() {
  return capture_client_.get();
}

void AuraTestHelper::InitWindowTreeClient() {
  window_tree_client_setup_ = std::make_unique<TestWindowTreeClientSetup>();
  window_tree_client_setup_->InitForWindowManager(window_tree_delegate_,
                                                  window_manager_delegate_);
  window_tree_client_ = window_tree_client_setup_->window_tree_client();
  window_tree_client_->capture_synchronizer()->AttachToCaptureClient(
      capture_client_.get());
}

}  // namespace test
}  // namespace aura
