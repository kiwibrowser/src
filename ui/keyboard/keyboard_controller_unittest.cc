// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/keyboard_controller.h"

#include <memory>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"
#include "ui/base/ime/dummy_text_input_client.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/input_method_factory.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/ui_base_switches.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer_type.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/compositor/test/context_factories_for_test.h"
#include "ui/compositor/test/layer_animator_test_controller.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/keyboard/container_full_width_behavior.h"
#include "ui/keyboard/keyboard_controller_observer.h"
#include "ui/keyboard/keyboard_test_util.h"
#include "ui/keyboard/keyboard_ui.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/wm/core/default_activation_client.h"

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif

namespace keyboard {
namespace {

const int kDefaultVirtualKeyboardHeight = 100;

// Verify if the |keyboard| window covers the |container| window completely.
void VerifyKeyboardWindowSize(aura::Window* container, aura::Window* keyboard) {
  ASSERT_EQ(gfx::Rect(0, 0, container->bounds().width(),
                      container->bounds().height()),
            keyboard->bounds());
}

// Steps a layer animation until it is completed. Animations must be enabled.
void RunAnimationForLayer(ui::Layer* layer) {
  // Animations must be enabled for stepping to work.
  ASSERT_NE(ui::ScopedAnimationDurationScaleMode::duration_scale_mode(),
            ui::ScopedAnimationDurationScaleMode::ZERO_DURATION);

  ui::LayerAnimatorTestController controller(layer->GetAnimator());
  // Multiple steps are required to complete complex animations.
  // TODO(vollick): This should not be necessary. crbug.com/154017
  while (controller.animator()->is_animating()) {
    controller.StartThreadedAnimationsIfNeeded();
    base::TimeTicks step_time = controller.animator()->last_step_time();
    controller.animator()->Step(step_time +
                                base::TimeDelta::FromMilliseconds(1000));
  }
}

class ScopedTouchKeyboardEnabler {
 public:
  ScopedTouchKeyboardEnabler() : enabled_(keyboard::GetTouchKeyboardEnabled()) {
    keyboard::SetTouchKeyboardEnabled(true);
  }

  ~ScopedTouchKeyboardEnabler() { keyboard::SetTouchKeyboardEnabled(enabled_); }

 private:
  const bool enabled_;
};

class ScopedAccessibilityKeyboardEnabler {
 public:
  ScopedAccessibilityKeyboardEnabler()
      : enabled_(keyboard::GetAccessibilityKeyboardEnabled()) {
    keyboard::SetAccessibilityKeyboardEnabled(true);
  }

  ~ScopedAccessibilityKeyboardEnabler() {
    keyboard::SetAccessibilityKeyboardEnabled(enabled_);
  }

 private:
  const bool enabled_;
};

// An event handler that focuses a window when it is clicked/touched on. This is
// used to match the focus manger behaviour in ash and views.
class TestFocusController : public ui::EventHandler {
 public:
  explicit TestFocusController(aura::Window* root)
      : root_(root) {
    root_->AddPreTargetHandler(this);
  }

  ~TestFocusController() override { root_->RemovePreTargetHandler(this); }

 private:
  // Overridden from ui::EventHandler:
  void OnEvent(ui::Event* event) override {
    aura::Window* target = static_cast<aura::Window*>(event->target());
    if (event->type() == ui::ET_MOUSE_PRESSED ||
        event->type() == ui::ET_TOUCH_PRESSED) {
      aura::client::GetFocusClient(target)->FocusWindow(target);
    }
  }

  aura::Window* root_;
  DISALLOW_COPY_AND_ASSIGN(TestFocusController);
};

// Keeps a count of all the events a window receives.
class EventObserver : public ui::EventHandler {
 public:
  EventObserver() {}
  ~EventObserver() override {}

  int GetEventCount(ui::EventType type) {
    return event_counts_[type];
  }

 private:
  // Overridden from ui::EventHandler:
  void OnEvent(ui::Event* event) override {
    ui::EventHandler::OnEvent(event);
    event_counts_[event->type()]++;
  }

  std::map<ui::EventType, int> event_counts_;
  DISALLOW_COPY_AND_ASSIGN(EventObserver);
};

class KeyboardContainerObserver : public aura::WindowObserver {
 public:
  explicit KeyboardContainerObserver(aura::Window* window,
                                     base::RunLoop* run_loop)
      : window_(window), run_loop_(run_loop) {
    window_->AddObserver(this);
  }
  ~KeyboardContainerObserver() override { window_->RemoveObserver(this); }

 private:
  void OnWindowVisibilityChanged(aura::Window* window, bool visible) override {
    if (!visible)
      run_loop_->QuitWhenIdle();
  }

  aura::Window* window_;
  base::RunLoop* const run_loop_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardContainerObserver);
};

class TestKeyboardLayoutDelegate : public KeyboardLayoutDelegate {
 public:
  TestKeyboardLayoutDelegate() {}
  ~TestKeyboardLayoutDelegate() override {}

  // Overridden from keyboard::KeyboardLayoutDelegate
  void MoveKeyboardToDisplay(const display::Display& display) override {}
  void MoveKeyboardToTouchableDisplay() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestKeyboardLayoutDelegate);
};

class SetModeCallbackInvocationCounter {
 public:
  SetModeCallbackInvocationCounter() : weak_factory_invoke_(this) {}

  void Invoke(bool status) {
    if (status)
      invocation_count_success_++;
    else
      invocation_count_failure_++;
  }

  base::OnceCallback<void(bool)> GetInvocationCallback() {
    return base::BindOnce(&SetModeCallbackInvocationCounter::Invoke,
                          weak_factory_invoke_.GetWeakPtr());
  }

  int invocation_count_for_status(bool status) {
    return status ? invocation_count_success_ : invocation_count_failure_;
  }

 private:
  int invocation_count_success_ = 0;
  int invocation_count_failure_ = 0;
  base::WeakPtrFactory<SetModeCallbackInvocationCounter> weak_factory_invoke_;
};

}  // namespace

class KeyboardControllerTest : public aura::test::AuraTestBase,
                               public KeyboardControllerObserver {
 public:
  KeyboardControllerTest()
      : visible_bounds_number_of_calls_(0),
        occluding_bounds_number_of_calls_(0),
        is_available_number_of_calls_(0),
        is_available_(false),
        keyboard_closed_(false) {}
  ~KeyboardControllerTest() override {}

  void SetUp() override {
    ui::SetUpInputMethodFactoryForTesting();
    aura::test::AuraTestBase::SetUp();
    new wm::DefaultActivationClient(root_window());
    focus_controller_.reset(new TestFocusController(root_window()));
    layout_delegate_.reset(new TestKeyboardLayoutDelegate());
    controller_.reset(new KeyboardController(
        std::make_unique<TestKeyboardUI>(host()->GetInputMethod()),
        layout_delegate_.get()));
    controller()->AddObserver(this);
  }

  void TearDown() override {
    if (controller())
      controller()->RemoveObserver(this);
    controller_.reset();
    focus_controller_.reset();
    aura::test::AuraTestBase::TearDown();
  }

  KeyboardUI* ui() { return controller_->ui(); }
  KeyboardController* controller() { return controller_.get(); }

  void ShowKeyboard() {
    test_text_input_client_.reset(
        new ui::DummyTextInputClient(ui::TEXT_INPUT_TYPE_TEXT));
    SetFocus(test_text_input_client_.get());
  }

  void MockRotateScreen() {
    const gfx::Rect root_bounds = root_window()->bounds();
    root_window()->SetBounds(
        gfx::Rect(0, 0, root_bounds.height(), root_bounds.width()));
  }

 protected:
  // KeyboardControllerObserver overrides
  void OnKeyboardVisibleBoundsChanged(const gfx::Rect& new_bounds) override {
    visible_bounds_ = new_bounds;
    visible_bounds_number_of_calls_++;
  }
  void OnKeyboardWorkspaceOccludedBoundsChanged(
      const gfx::Rect& new_bounds) override {
    occluding_bounds_ = new_bounds;
    occluding_bounds_number_of_calls_++;
  }
  void OnKeyboardAvailabilityChanged(bool is_available) override {
    is_available_ = is_available;
    is_available_number_of_calls_++;
  }
  void OnKeyboardClosed() override { keyboard_closed_ = true; }

  int visible_bounds_number_of_calls() const {
    return visible_bounds_number_of_calls_;
  }
  int occluding_bounds_number_of_calls() const {
    return occluding_bounds_number_of_calls_;
  }
  int is_available_number_of_calls() const {
    return is_available_number_of_calls_;
  }

  const gfx::Rect& notified_visible_bounds() { return visible_bounds_; }
  const gfx::Rect& notified_occluding_bounds() { return occluding_bounds_; }
  bool notified_is_available() { return is_available_; }

  bool IsKeyboardClosed() { return keyboard_closed_; }

  void SetProgrammaticFocus(ui::TextInputClient* client) {
    controller_->OnTextInputStateChanged(client);
  }

  void AddTimeToTransientBlurCounter(double seconds) {
    controller_->time_of_last_blur_ -=
        base::TimeDelta::FromMilliseconds((int)(1000 * seconds));
  }

  void SetFocus(ui::TextInputClient* client) {
    ui::InputMethod* input_method = ui()->GetInputMethod();
    input_method->SetFocusedTextInputClient(client);
    if (client && client->GetTextInputType() != ui::TEXT_INPUT_TYPE_NONE &&
        client->GetTextInputMode() != ui::TEXT_INPUT_MODE_NONE) {
      input_method->ShowImeIfNeeded();
      if (controller_->ui()->GetContentsWindow()->bounds().height() == 0) {
        // Set initial bounds for test keyboard window.
        controller_->ui()->GetContentsWindow()->SetBounds(
            KeyboardBoundsFromRootBounds(root_window()->bounds(),
                                         kDefaultVirtualKeyboardHeight));
      }
    }
  }

  bool WillHideKeyboard() {
    return controller_->WillHideKeyboard();
  }

  bool ShouldEnableInsets(aura::Window* window) {
    aura::Window* contents_window = controller_->ui()->GetContentsWindow();
    return (contents_window->GetRootWindow() == window->GetRootWindow() &&
            keyboard::IsKeyboardOverscrollEnabled() &&
            contents_window->IsVisible() && controller_->keyboard_visible());
  }

  void ResetController() { controller_.reset(); }

  void RunLoop(base::RunLoop* run_loop) {
#if defined(USE_OZONE)
    // TODO(crbug/776357): Figure out why the initializer randomly doesn't run
    // for some tests. In the mean time, prevent flaky Ozone crash.
    ui::OzonePlatform::InitializeForGPU(ui::OzonePlatform::InitParams());
#endif
    run_loop->Run();
  }

  std::unique_ptr<TestFocusController> focus_controller_;

 private:
  int visible_bounds_number_of_calls_;
  gfx::Rect visible_bounds_;
  int occluding_bounds_number_of_calls_;
  gfx::Rect occluding_bounds_;
  int is_available_number_of_calls_;
  bool is_available_;

  std::unique_ptr<KeyboardLayoutDelegate> layout_delegate_;
  std::unique_ptr<KeyboardController> controller_;
  std::unique_ptr<ui::TextInputClient> test_text_input_client_;
  bool keyboard_closed_;
  DISALLOW_COPY_AND_ASSIGN(KeyboardControllerTest);
};

TEST_F(KeyboardControllerTest, KeyboardSize) {
  aura::Window* container(controller()->GetContainerWindow());
  aura::Window* keyboard(ui()->GetContentsWindow());
  gfx::Rect screen_bounds = root_window()->bounds();
  root_window()->AddChild(container);
  container->AddChild(keyboard);
  const gfx::Rect& initial_bounds = container->bounds();
  // The container should be positioned at the bottom of screen and has 0
  // height.
  ASSERT_EQ(0, initial_bounds.height());
  ASSERT_EQ(screen_bounds.height(), initial_bounds.y());
  VerifyKeyboardWindowSize(container, keyboard);

  // Attempt to change window width or move window up from the bottom are
  // ignored. Changing window height is supported.
  gfx::Rect expected_bounds(0,
                            screen_bounds.height() - 50,
                            screen_bounds.width(),
                            50);

  // The x position of new bounds may not be 0 if shelf is on the left side of
  // screen. The virtual keyboard should always align with the left edge of
  // screen. See http://crbug.com/510595.
  gfx::Rect new_bounds(10, 0, 50, 50);
  keyboard->SetBounds(new_bounds);
  ASSERT_EQ(expected_bounds, container->bounds());
  VerifyKeyboardWindowSize(container, keyboard);

  MockRotateScreen();
  // The above call should resize keyboard to new width while keeping the old
  // height.
  ASSERT_EQ(gfx::Rect(0,
                      screen_bounds.width() - 50,
                      screen_bounds.height(),
                      50),
            container->bounds());
  VerifyKeyboardWindowSize(container, keyboard);
}

// Tests that tapping/clicking inside the keyboard does not give it focus.
TEST_F(KeyboardControllerTest, ClickDoesNotFocusKeyboard) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  const gfx::Rect& root_bounds = root_window()->bounds();
  aura::test::EventCountDelegate delegate;
  std::unique_ptr<aura::Window> window(new aura::Window(&delegate));
  window->Init(ui::LAYER_NOT_DRAWN);
  window->SetBounds(root_bounds);
  root_window()->AddChild(window.get());
  window->Show();
  window->Focus();

  aura::Window* keyboard_container(controller()->GetContainerWindow());

  root_window()->AddChild(keyboard_container);

  ShowKeyboard();

  EXPECT_TRUE(window->IsVisible());
  EXPECT_TRUE(keyboard_container->IsVisible());
  EXPECT_TRUE(window->HasFocus());
  EXPECT_FALSE(keyboard_container->HasFocus());

  // Click on the keyboard. Make sure the keyboard receives the event, but does
  // not get focus.
  EventObserver observer;
  keyboard_container->AddPreTargetHandler(&observer);

  ui::test::EventGenerator generator(root_window());
  generator.MoveMouseTo(keyboard_container->bounds().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_TRUE(window->HasFocus());
  EXPECT_FALSE(keyboard_container->HasFocus());
  EXPECT_EQ("0 0", delegate.GetMouseButtonCountsAndReset());
  EXPECT_EQ(1, observer.GetEventCount(ui::ET_MOUSE_PRESSED));
  EXPECT_EQ(1, observer.GetEventCount(ui::ET_MOUSE_RELEASED));

  // Click outside of the keyboard. It should reach the window behind.
  generator.MoveMouseTo(gfx::Point());
  generator.ClickLeftButton();
  EXPECT_EQ("1 1", delegate.GetMouseButtonCountsAndReset());
  keyboard_container->RemovePreTargetHandler(&observer);
}

// Tests that blur-then-focus that occur in less than the transient threshold
// cause the keyboard to re-show.
TEST_F(KeyboardControllerTest, TransientBlurShortDelay) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  ui::DummyTextInputClient input_client(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient no_input_client(ui::TEXT_INPUT_TYPE_NONE);
  base::RunLoop run_loop;
  aura::Window* keyboard_container(controller()->GetContainerWindow());
  std::unique_ptr<KeyboardContainerObserver> keyboard_container_observer(
      new KeyboardContainerObserver(keyboard_container, &run_loop));
  root_window()->AddChild(keyboard_container);

  // Keyboard is hidden
  EXPECT_FALSE(keyboard_container->IsVisible());

  // Set programmatic focus to the text field. Nothing happens
  SetProgrammaticFocus(&input_client);
  EXPECT_FALSE(keyboard_container->IsVisible());

  // Click it for real. Keyboard starts to appear.
  SetFocus(&input_client);
  EXPECT_TRUE(keyboard_container->IsVisible());

  // Focus a non text field
  SetFocus(&no_input_client);

  // It waits 100 ms and then hides. Wait for this routine to finish.
  EXPECT_TRUE(WillHideKeyboard());
  RunLoop(&run_loop);
  EXPECT_FALSE(keyboard_container->IsVisible());

  // Virtually wait half a second
  AddTimeToTransientBlurCounter(0.5);
  // Apply programmatic focus to the text field.
  SetProgrammaticFocus(&input_client);

  // TODO(blakeo): this is not technically wrong, but the DummyTextInputClient
  // should allow for overriding the text input flags, to simulate testing
  // a web-based text field.
  EXPECT_FALSE(keyboard_container->IsVisible());
  EXPECT_FALSE(WillHideKeyboard());
}

// Tests that blur-then-focus that occur past the transient threshold do not
// cause the keyboard to re-show.
TEST_F(KeyboardControllerTest, TransientBlurLongDelay) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  ui::DummyTextInputClient input_client(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient no_input_client(ui::TEXT_INPUT_TYPE_NONE);
  base::RunLoop run_loop;
  aura::Window* keyboard_container(controller()->GetContainerWindow());
  std::unique_ptr<KeyboardContainerObserver> keyboard_container_observer(
      new KeyboardContainerObserver(keyboard_container, &run_loop));
  root_window()->AddChild(keyboard_container);

  // Keyboard is hidden
  EXPECT_FALSE(keyboard_container->IsVisible());

  // Set programmatic focus to the text field. Nothing happens
  SetProgrammaticFocus(&input_client);
  EXPECT_FALSE(keyboard_container->IsVisible());

  // Click it for real. Keyboard starts to appear.
  SetFocus(&input_client);
  EXPECT_TRUE(keyboard_container->IsVisible());

  // Focus a non text field
  SetFocus(&no_input_client);

  // It waits 100 ms and then hides. Wait for this routine to finish.
  EXPECT_TRUE(WillHideKeyboard());
  RunLoop(&run_loop);
  EXPECT_FALSE(keyboard_container->IsVisible());

  // Wait 5 seconds and then set programmatic focus to a text field
  AddTimeToTransientBlurCounter(5.0);
  SetProgrammaticFocus(&input_client);
  EXPECT_FALSE(keyboard_container->IsVisible());
}

TEST_F(KeyboardControllerTest, VisibilityChangeWithTextInputTypeChange) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  ui::DummyTextInputClient input_client_0(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient input_client_1(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient input_client_2(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient no_input_client_0(ui::TEXT_INPUT_TYPE_NONE);
  ui::DummyTextInputClient no_input_client_1(ui::TEXT_INPUT_TYPE_NONE);

  base::RunLoop run_loop;
  aura::Window* keyboard_container(controller()->GetContainerWindow());
  std::unique_ptr<KeyboardContainerObserver> keyboard_container_observer(
      new KeyboardContainerObserver(keyboard_container, &run_loop));
  root_window()->AddChild(keyboard_container);

  SetFocus(&input_client_0);

  EXPECT_TRUE(keyboard_container->IsVisible());

  SetFocus(&no_input_client_0);
  // Keyboard should not immediately hide itself. It is delayed to avoid layout
  // flicker when the focus of input field quickly change.
  EXPECT_TRUE(keyboard_container->IsVisible());
  EXPECT_TRUE(WillHideKeyboard());
  // Wait for hide keyboard to finish.

  RunLoop(&run_loop);
  EXPECT_FALSE(keyboard_container->IsVisible());

  SetFocus(&input_client_1);
  EXPECT_TRUE(keyboard_container->IsVisible());

  // Schedule to hide keyboard.
  SetFocus(&no_input_client_1);
  EXPECT_TRUE(WillHideKeyboard());
  // Cancel keyboard hide.
  SetFocus(&input_client_2);

  EXPECT_FALSE(WillHideKeyboard());
  EXPECT_TRUE(keyboard_container->IsVisible());
}

// Test to prevent spurious overscroll boxes when changing tabs during keyboard
// hide. Refer to crbug.com/401670 for more context.
TEST_F(KeyboardControllerTest, CheckOverscrollInsetDuringVisibilityChange) {
  ui::DummyTextInputClient input_client(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient no_input_client(ui::TEXT_INPUT_TYPE_NONE);

  aura::Window* keyboard_container(controller()->GetContainerWindow());
  root_window()->AddChild(keyboard_container);

  // Enable touch keyboard / overscroll mode to test insets.
  ScopedTouchKeyboardEnabler scoped_keyboard_enabler;
  EXPECT_TRUE(keyboard::IsKeyboardOverscrollEnabled());

  SetFocus(&input_client);
  SetFocus(&no_input_client);
  // Insets should not be enabled for new windows while keyboard is in the
  // process of hiding when overscroll is enabled.
  EXPECT_FALSE(ShouldEnableInsets(ui()->GetContentsWindow()));
  // Cancel keyboard hide.
  SetFocus(&input_client);
  // Insets should be enabled for new windows as hide was cancelled.
  EXPECT_TRUE(ShouldEnableInsets(ui()->GetContentsWindow()));
}

TEST_F(KeyboardControllerTest, AlwaysVisibleWhenLocked) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  ui::DummyTextInputClient input_client_0(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient input_client_1(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient no_input_client_0(ui::TEXT_INPUT_TYPE_NONE);
  ui::DummyTextInputClient no_input_client_1(ui::TEXT_INPUT_TYPE_NONE);

  base::RunLoop run_loop;
  aura::Window* keyboard_container(controller()->GetContainerWindow());
  std::unique_ptr<KeyboardContainerObserver> keyboard_container_observer(
      new KeyboardContainerObserver(keyboard_container, &run_loop));
  root_window()->AddChild(keyboard_container);

  SetFocus(&input_client_0);

  EXPECT_TRUE(keyboard_container->IsVisible());

  // Lock keyboard.
  controller()->set_keyboard_locked(true);

  SetFocus(&no_input_client_0);
  // Keyboard should not try to hide itself as it is locked.
  EXPECT_TRUE(keyboard_container->IsVisible());
  EXPECT_FALSE(WillHideKeyboard());

  // MaybeHideKeyboard is no-op when the keyboard is locked.
  controller()->MaybeHideKeyboard();
  EXPECT_TRUE(keyboard_container->IsVisible());
  EXPECT_FALSE(WillHideKeyboard());

  SetFocus(&input_client_1);
  EXPECT_TRUE(keyboard_container->IsVisible());

  // Unlock keyboard.
  controller()->set_keyboard_locked(false);

  // Keyboard should hide when focus on no input client.
  SetFocus(&no_input_client_1);
  EXPECT_TRUE(WillHideKeyboard());

  // Wait for hide keyboard to finish.
  RunLoop(&run_loop);
  EXPECT_FALSE(keyboard_container->IsVisible());
}

// Tests that deactivates keyboard will get closed event.
TEST_F(KeyboardControllerTest, CloseKeyboard) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  aura::Window* keyboard_container(controller()->GetContainerWindow());
  root_window()->AddChild(keyboard_container);

  ShowKeyboard();
  EXPECT_TRUE(keyboard_container->IsVisible());
  EXPECT_FALSE(IsKeyboardClosed());

  root_window()->RemoveChild(keyboard_container);
  ResetController();
  EXPECT_TRUE(IsKeyboardClosed());
}

class KeyboardControllerAnimationTest : public KeyboardControllerTest {
 public:
  KeyboardControllerAnimationTest() {}
  ~KeyboardControllerAnimationTest() override {}

  void SetUp() override {
    // We cannot short-circuit animations for this test.
    ui::ScopedAnimationDurationScaleMode test_duration_mode(
        ui::ScopedAnimationDurationScaleMode::NON_ZERO_DURATION);

    KeyboardControllerTest::SetUp();

    const gfx::Rect& root_bounds = root_window()->bounds();
    keyboard_container()->SetBounds(root_bounds);
    root_window()->AddChild(keyboard_container());
  }

  void TearDown() override {
    KeyboardControllerTest::TearDown();
  }

 protected:
  aura::Window* keyboard_container() {
    return controller()->GetContainerWindow();
  }

  aura::Window* contents_window() { return ui()->GetContentsWindow(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardControllerAnimationTest);
};

TEST_F(KeyboardControllerAnimationTest, ContainerAnimation) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  ui::Layer* layer = keyboard_container()->layer();
  ShowKeyboard();

  // Keyboard container and window should immediately become visible before
  // animation starts.
  EXPECT_TRUE(keyboard_container()->IsVisible());
  EXPECT_TRUE(contents_window()->IsVisible());
  float show_start_opacity = layer->opacity();
  gfx::Transform transform;
  transform.Translate(0, keyboard::kFullWidthKeyboardAnimationDistance);
  EXPECT_EQ(transform, layer->transform());
  // Actual final bounds should be notified after animation finishes to avoid
  // flash of background being seen.
  EXPECT_EQ(gfx::Rect(), notified_visible_bounds());
  EXPECT_EQ(gfx::Rect(), notified_occluding_bounds());
  EXPECT_FALSE(notified_is_available());

  RunAnimationForLayer(layer);
  EXPECT_TRUE(keyboard_container()->IsVisible());
  EXPECT_TRUE(contents_window()->IsVisible());
  float show_end_opacity = layer->opacity();
  EXPECT_LT(show_start_opacity, show_end_opacity);
  EXPECT_EQ(gfx::Transform(), layer->transform());
  // KeyboardController should notify the bounds of container window to its
  // observers after show animation finished.
  EXPECT_EQ(keyboard_container()->bounds(), notified_visible_bounds());
  EXPECT_EQ(keyboard_container()->bounds(), notified_occluding_bounds());
  EXPECT_TRUE(notified_is_available());

  // Directly hide keyboard without delay.
  float hide_start_opacity = layer->opacity();
  controller()->HideKeyboard(KeyboardController::HIDE_REASON_AUTOMATIC);
  EXPECT_FALSE(keyboard_container()->IsVisible());
  EXPECT_FALSE(keyboard_container()->layer()->visible());
  EXPECT_FALSE(contents_window()->IsVisible());
  layer = keyboard_container()->layer();
  // KeyboardController should notify the bounds of keyboard window to its
  // observers before hide animation starts.
  EXPECT_EQ(gfx::Rect(), notified_visible_bounds());
  EXPECT_EQ(gfx::Rect(), notified_occluding_bounds());
  EXPECT_FALSE(notified_is_available());

  RunAnimationForLayer(layer);
  EXPECT_FALSE(keyboard_container()->IsVisible());
  EXPECT_FALSE(keyboard_container()->layer()->visible());
  EXPECT_FALSE(contents_window()->IsVisible());
  float hide_end_opacity = layer->opacity();
  EXPECT_GT(hide_start_opacity, hide_end_opacity);
  EXPECT_EQ(gfx::Rect(), notified_visible_bounds());
  EXPECT_EQ(gfx::Rect(), notified_occluding_bounds());
  EXPECT_FALSE(notified_is_available());

  SetModeCallbackInvocationCounter invocation_counter;
  controller()->SetContainerType(ContainerType::FLOATING, base::nullopt,
                                 invocation_counter.GetInvocationCallback());
  EXPECT_EQ(1, invocation_counter.invocation_count_for_status(true));
  EXPECT_EQ(0, invocation_counter.invocation_count_for_status(false));
  ShowKeyboard();
  RunAnimationForLayer(layer);
  EXPECT_EQ(1, invocation_counter.invocation_count_for_status(true));
  EXPECT_EQ(0, invocation_counter.invocation_count_for_status(false));
  // Visible bounds and occluding bounds are now different.
  EXPECT_EQ(keyboard_container()->bounds(), notified_visible_bounds());
  EXPECT_EQ(gfx::Rect(), notified_occluding_bounds());
  EXPECT_TRUE(notified_is_available());

  // callback should do nothing when container mode is set to the current active
  // container type. An unnecessary call gets registered synchronously as a
  // failure status to the callback.
  controller()->SetContainerType(ContainerType::FLOATING, base::nullopt,
                                 invocation_counter.GetInvocationCallback());
  EXPECT_EQ(1, invocation_counter.invocation_count_for_status(true));
  EXPECT_EQ(1, invocation_counter.invocation_count_for_status(false));
}

TEST_F(KeyboardControllerAnimationTest, ChangeContainerModeWithBounds) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  SetModeCallbackInvocationCounter invocation_counter;

  ui::Layer* layer = keyboard_container()->layer();
  ShowKeyboard();
  RunAnimationForLayer(layer);
  EXPECT_EQ(ContainerType::FULL_WIDTH, controller()->GetActiveContainerType());
  EXPECT_TRUE(keyboard_container()->IsVisible());
  EXPECT_TRUE(contents_window()->IsVisible());

  // Changing the mode to another mode invokes hiding + showing.
  const gfx::Rect target_bounds(0, 0, 1200, 600);
  controller()->SetContainerType(ContainerType::FLOATING,
                                 base::make_optional(target_bounds),
                                 invocation_counter.GetInvocationCallback());
  // The container window shouldn't be resized until it's hidden even if the
  // target bounds is passed to |SetContainerType|.
  EXPECT_EQ(gfx::Rect(), notified_visible_bounds());
  EXPECT_EQ(0, invocation_counter.invocation_count_for_status(true));
  EXPECT_EQ(0, invocation_counter.invocation_count_for_status(false));
  RunAnimationForLayer(layer);
  // Hiding animation finished. The container window should be resized to the
  // target bounds.
  EXPECT_EQ(keyboard_container()->bounds().size(), target_bounds.size());
  // Then showing animation automatically start.
  layer = keyboard_container()->layer();
  RunAnimationForLayer(layer);
  EXPECT_EQ(1, invocation_counter.invocation_count_for_status(true));
  EXPECT_EQ(0, invocation_counter.invocation_count_for_status(false));
}

// Show keyboard during keyboard hide animation should abort the hide animation
// and the keyboard should animate in.
// Test for crbug.com/333284.
TEST_F(KeyboardControllerAnimationTest, ContainerShowWhileHide) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  ui::Layer* layer = keyboard_container()->layer();
  ShowKeyboard();
  RunAnimationForLayer(layer);

  controller()->HideKeyboard(KeyboardController::HIDE_REASON_AUTOMATIC);
  // Before hide animation finishes, show keyboard again.
  ShowKeyboard();
  layer = keyboard_container()->layer();
  RunAnimationForLayer(layer);
  EXPECT_TRUE(keyboard_container()->IsVisible());
  EXPECT_TRUE(contents_window()->IsVisible());
  EXPECT_EQ(1.0, layer->opacity());
}

TEST_F(KeyboardControllerAnimationTest, SetBoundsOnOldKeyboardUiReference) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;

  // Ensure keyboard ui is populated
  ui::Layer* layer = keyboard_container()->layer();
  ShowKeyboard();
  RunAnimationForLayer(layer);

  ASSERT_TRUE(controller()->ui());

  aura::Window* container_window = controller()->GetContainerWindow();

  // Simulate removal of keyboard controller from root window as done by
  // RootWindowController::DeactivateKeyboard()
  container_window->parent()->RemoveChild(container_window);

  // lingering handle to the contents window is adjusted.
  // container_window's LayoutManager should abort silently and not crash.
  controller()->ui()->GetContentsWindow()->SetBounds(gfx::Rect());
}

TEST_F(KeyboardControllerTest, DisplayChangeShouldNotifyBoundsChange) {
  ScopedTouchKeyboardEnabler scoped_keyboard_enabler;
  ui::DummyTextInputClient input_client(ui::TEXT_INPUT_TYPE_TEXT);

  aura::Window* container(controller()->GetContainerWindow());
  root_window()->AddChild(container);

  SetFocus(&input_client);
  gfx::Rect new_bounds(0, 0, 1280, 800);
  ASSERT_NE(new_bounds, root_window()->bounds());
  EXPECT_EQ(1, visible_bounds_number_of_calls());
  EXPECT_EQ(1, occluding_bounds_number_of_calls());
  EXPECT_EQ(1, is_available_number_of_calls());
  root_window()->SetBounds(new_bounds);
  EXPECT_EQ(2, visible_bounds_number_of_calls());
  EXPECT_EQ(2, occluding_bounds_number_of_calls());
  EXPECT_EQ(1, is_available_number_of_calls());
  MockRotateScreen();
  EXPECT_EQ(3, visible_bounds_number_of_calls());
  EXPECT_EQ(3, occluding_bounds_number_of_calls());
  EXPECT_EQ(1, is_available_number_of_calls());
}

TEST_F(KeyboardControllerTest, TextInputMode) {
  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  ui::DummyTextInputClient input_client(ui::TEXT_INPUT_TYPE_TEXT,
                                        ui::TEXT_INPUT_MODE_TEXT);
  ui::DummyTextInputClient no_input_client(ui::TEXT_INPUT_TYPE_TEXT,
                                           ui::TEXT_INPUT_MODE_NONE);

  base::RunLoop run_loop;
  aura::Window* keyboard_container(controller()->GetContainerWindow());
  std::unique_ptr<KeyboardContainerObserver> keyboard_container_observer(
      new KeyboardContainerObserver(keyboard_container, &run_loop));
  root_window()->AddChild(keyboard_container);

  SetFocus(&input_client);

  EXPECT_TRUE(keyboard_container->IsVisible());

  SetFocus(&no_input_client);
  // Keyboard should not immediately hide itself. It is delayed to avoid layout
  // flicker when the focus of input field quickly change.
  EXPECT_TRUE(keyboard_container->IsVisible());
  EXPECT_TRUE(WillHideKeyboard());
  // Wait for hide keyboard to finish.

  RunLoop(&run_loop);
  EXPECT_FALSE(keyboard_container->IsVisible());

  SetFocus(&input_client);
  EXPECT_TRUE(keyboard_container->IsVisible());
}

// Checks that floating keyboard does not cause focused window to move upwards.
// Refer to crbug.com/838731.
TEST_F(KeyboardControllerAnimationTest, FloatingKeyboardEnsureCaretInWorkArea) {
  // Mock TextInputClient to intercept calls to EnsureCaretNotInRect.
  struct MockTextInputClient : public ui::DummyTextInputClient {
    MockTextInputClient() : DummyTextInputClient(ui::TEXT_INPUT_TYPE_TEXT) {}
    MOCK_METHOD1(EnsureCaretNotInRect, void(const gfx::Rect&));
  };

  // Floating keyboard should call EnsureCaretNotInRect with the empty rect.
  MockTextInputClient mock_input_client;
  EXPECT_CALL(mock_input_client, EnsureCaretNotInRect(gfx::Rect())).Times(1);

  ScopedAccessibilityKeyboardEnabler scoped_keyboard_enabler;
  controller()->SetContainerType(keyboard::ContainerType::FLOATING,
                                 base::nullopt, base::DoNothing());
  ASSERT_EQ(keyboard::ContainerType::FLOATING,
            controller()->GetActiveContainerType());

  // Ensure keyboard ui is populated
  ui::Layer* layer = keyboard_container()->layer();
  SetFocus(&mock_input_client);
  RunAnimationForLayer(layer);

  EXPECT_TRUE(keyboard_container()->IsVisible());
}

}  // namespace keyboard
