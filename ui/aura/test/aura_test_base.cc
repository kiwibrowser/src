// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/aura_test_base.h"

#include "base/command_line.h"
#include "ui/aura/client/window_parenting_client.h"
#include "ui/aura/mus/property_utils.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/test/aura_test_context_factory.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"
#include "ui/base/ime/input_method_initializer.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/test/material_design_controller_test_api.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches.h"
#include "ui/base/ui_base_switches_util.h"
#include "ui/compositor/test/context_factories_for_test.h"
#include "ui/events/event_dispatcher.h"
#include "ui/events/event_sink.h"
#include "ui/events/gesture_detection/gesture_configuration.h"

namespace aura {
namespace test {

AuraTestBase::AuraTestBase()
    : scoped_task_environment_(
          base::test::ScopedTaskEnvironment::MainThreadType::UI),
      window_manager_delegate_(this),
      window_tree_client_delegate_(this) {}

AuraTestBase::~AuraTestBase() {
  CHECK(setup_called_)
      << "You have overridden SetUp but never called super class's SetUp";
  CHECK(teardown_called_)
      << "You have overridden TearDown but never called super class's TearDown";
}

void AuraTestBase::SetUp() {
  setup_called_ = true;
  testing::Test::SetUp();
  // ContentTestSuiteBase might have already initialized
  // MaterialDesignController in unit_tests suite.
  ui::test::MaterialDesignControllerTestAPI::Uninitialize();
  ui::MaterialDesignController::Initialize();
  ui::InitializeInputMethodForTesting();
  ui::GestureConfiguration* gesture_config =
      ui::GestureConfiguration::GetInstance();
  // Changing the parameters for gesture recognition shouldn't cause
  // tests to fail, so we use a separate set of parameters for unit
  // testing.
  gesture_config->set_default_radius(0);
  gesture_config->set_fling_max_cancel_to_down_time_in_ms(400);
  gesture_config->set_fling_max_tap_gap_time_in_ms(200);
  gesture_config->set_gesture_begin_end_types_enabled(true);
  gesture_config->set_long_press_time_in_ms(1000);
  gesture_config->set_max_distance_between_taps_for_double_tap(20);
  gesture_config->set_max_distance_for_two_finger_tap_in_pixels(300);
  gesture_config->set_max_fling_velocity(15000);
  gesture_config->set_max_gesture_bounds_length(0);
  gesture_config->set_max_separation_for_gesture_touches_in_pixels(150);
  gesture_config->set_max_swipe_deviation_angle(20);
  gesture_config->set_max_time_between_double_click_in_ms(700);
  gesture_config->set_max_touch_down_duration_for_click_in_ms(800);
  gesture_config->set_max_touch_move_in_pixels_for_click(5);
  gesture_config->set_min_distance_for_pinch_scroll_in_pixels(20);
  gesture_config->set_min_fling_velocity(30.0f);
  gesture_config->set_min_pinch_update_span_delta(0);
  gesture_config->set_min_scaling_span_in_pixels(125);
  gesture_config->set_min_swipe_velocity(10);
  gesture_config->set_scroll_debounce_interval_in_ms(0);
  gesture_config->set_semi_long_press_time_in_ms(400);
  gesture_config->set_show_press_delay_in_ms(5);
  gesture_config->set_swipe_enabled(true);
  gesture_config->set_tab_scrub_activation_delay_in_ms(200);
  gesture_config->set_two_finger_tap_enabled(true);
  gesture_config->set_velocity_tracker_strategy(
      ui::VelocityTracker::Strategy::LSQ2_RESTRICTED);

  // The ContextFactory must exist before any Compositors are created.
  ui::ContextFactory* context_factory = nullptr;
  ui::ContextFactoryPrivate* context_factory_private = nullptr;
  if (backend_type_ != BackendType::CLASSIC) {
    mus_context_factory_ = std::make_unique<AuraTestContextFactory>();
    context_factory = mus_context_factory_.get();
  } else {
    const bool enable_pixel_output = false;
    ui::InitializeContextFactoryForTests(enable_pixel_output, &context_factory,
                                         &context_factory_private);
  }

  helper_ = std::make_unique<AuraTestHelper>();
  if (backend_type_ != BackendType::CLASSIC) {
    helper_->EnableMusWithTestWindowTree(
        window_tree_client_delegate_, window_manager_delegate_,
        backend_type_ == BackendType::MUS2 ? WindowTreeClient::Config::kMus2
                                           : WindowTreeClient::Config::kMash);
  }
  helper_->SetUp(context_factory, context_factory_private);
}

void AuraTestBase::TearDown() {
  teardown_called_ = true;

  // Flush the message loop because we have pending release tasks
  // and these tasks if un-executed would upset Valgrind.
  RunAllPendingInMessageLoop();

  // AuraTestHelper may own a WindowTreeHost, don't delete it here else
  // AuraTestHelper will have use after frees.
  for (size_t i = window_tree_hosts_.size(); i > 0; --i) {
    if (window_tree_hosts_[i - 1].get() == helper_->host()) {
      window_tree_hosts_[i - 1].release();
      window_tree_hosts_.erase(window_tree_hosts_.begin() + i - 1);
      break;
    }
  }

  helper_->TearDown();
  window_tree_hosts_.clear();
  ui::TerminateContextFactoryForTests();
  ui::ShutdownInputMethodForTesting();
  testing::Test::TearDown();
}

Window* AuraTestBase::CreateNormalWindow(int id, Window* parent,
                                         WindowDelegate* delegate) {
  Window* window = new Window(
      delegate ? delegate :
      test::TestWindowDelegate::CreateSelfDestroyingDelegate());
  window->set_id(id);
  window->Init(ui::LAYER_TEXTURED);
  parent->AddChild(window);
  window->SetBounds(gfx::Rect(0, 0, 100, 100));
  window->Show();
  return window;
}

void AuraTestBase::EnableMusWithTestWindowTree() {
  DCHECK(!setup_called_);
  backend_type_ = BackendType::MUS;
}

void AuraTestBase::DeleteWindowTreeClient() {
  DCHECK_NE(backend_type_, BackendType::CLASSIC);
  helper_->DeleteWindowTreeClient();
}

void AuraTestBase::ConfigureBackend(BackendType type) {
  DCHECK(!setup_called_);
  backend_type_ = type;
}

void AuraTestBase::RunAllPendingInMessageLoop() {
  helper_->RunAllPendingInMessageLoop();
}

void AuraTestBase::ParentWindow(Window* window) {
  client::ParentWindowWithContext(window, root_window(), gfx::Rect());
}

bool AuraTestBase::DispatchEventUsingWindowDispatcher(ui::Event* event) {
  ui::EventDispatchDetails details = event_sink()->OnEventFromSource(event);
  CHECK(!details.dispatcher_destroyed);
  return event->handled();
}

ui::mojom::WindowTreeClient* AuraTestBase::window_tree_client() {
  return helper_->window_tree_client();
}

void AuraTestBase::OnEmbed(
    std::unique_ptr<WindowTreeHostMus> window_tree_host) {}

void AuraTestBase::OnUnembed(Window* root) {}

void AuraTestBase::OnEmbedRootDestroyed(WindowTreeHostMus* window_tree_host) {}

void AuraTestBase::OnLostConnection(WindowTreeClient* client) {}

void AuraTestBase::OnPointerEventObserved(const ui::PointerEvent& event,
                                          int64_t display_id,
                                          Window* target) {
  observed_pointer_events_.push_back(std::unique_ptr<ui::PointerEvent>(
      static_cast<ui::PointerEvent*>(ui::Event::Clone(event).release())));
}

void AuraTestBase::SetWindowManagerClient(WindowManagerClient* client) {}

void AuraTestBase::OnWmConnected() {}

void AuraTestBase::OnWmSetBounds(Window* window, const gfx::Rect& bounds) {}

bool AuraTestBase::OnWmSetProperty(
    Window* window,
    const std::string& name,
    std::unique_ptr<std::vector<uint8_t>>* new_data) {
  return true;
}

void AuraTestBase::OnWmSetModalType(Window* window, ui::ModalType type) {}

void AuraTestBase::OnWmSetCanFocus(Window* window, bool can_focus) {}

Window* AuraTestBase::OnWmCreateTopLevelWindow(
    ui::mojom::WindowType window_type,
    std::map<std::string, std::vector<uint8_t>>* properties) {
  Window* window = new Window(nullptr);
  SetWindowType(window, window_type);
  window->Init(ui::LAYER_NOT_DRAWN);
  return window;
}

void AuraTestBase::OnWmClientJankinessChanged(
    const std::set<Window*>& client_windows,
    bool janky) {}

void AuraTestBase::OnWmWillCreateDisplay(const display::Display& display) {}

void AuraTestBase::OnWmNewDisplay(
    std::unique_ptr<WindowTreeHostMus> window_tree_host,
    const display::Display& display) {
  // Take ownership of the WindowTreeHost.
  window_tree_hosts_.push_back(std::move(window_tree_host));
}

void AuraTestBase::OnWmDisplayRemoved(WindowTreeHostMus* window_tree_host) {
  for (auto iter = window_tree_hosts_.begin(); iter != window_tree_hosts_.end();
       ++iter) {
    if (iter->get() == window_tree_host) {
      window_tree_hosts_.erase(iter);
      return;
    }
  }
  NOTREACHED();
}

void AuraTestBase::OnWmDisplayModified(const display::Display& display) {}

ui::mojom::EventResult AuraTestBase::OnAccelerator(
    uint32_t id,
    const ui::Event& event,
    base::flat_map<std::string, std::vector<uint8_t>>* properties) {
  return ui::mojom::EventResult::HANDLED;
}

void AuraTestBase::OnCursorTouchVisibleChanged(bool enabled) {}

void AuraTestBase::OnWmPerformMoveLoop(
    Window* window,
    ui::mojom::MoveLoopSource source,
    const gfx::Point& cursor_location,
    const base::Callback<void(bool)>& on_done) {}

void AuraTestBase::OnWmCancelMoveLoop(Window* window) {}

void AuraTestBase::OnWmSetClientArea(
    Window* window,
    const gfx::Insets& insets,
    const std::vector<gfx::Rect>& additional_client_areas) {}

bool AuraTestBase::IsWindowActive(aura::Window* window) { return false; }

void AuraTestBase::OnWmDeactivateWindow(Window* window) {}

PropertyConverter* AuraTestBase::GetPropertyConverter() {
  return &property_converter_;
}

AuraTestBaseWithType::AuraTestBaseWithType() {}

AuraTestBaseWithType::~AuraTestBaseWithType() {
  DCHECK(setup_called_);
}

void AuraTestBaseWithType::SetUp() {
  DCHECK(!setup_called_);
  setup_called_ = true;
  ConfigureBackend(GetParam());
  AuraTestBase::SetUp();
}

AuraTestBaseMus::AuraTestBaseMus() {}

AuraTestBaseMus::~AuraTestBaseMus() {}

void AuraTestBaseMus::SetUp() {
  ConfigureBackend(test::BackendType::MUS);
  AuraTestBase::SetUp();
}

}  // namespace test
}  // namespace aura
