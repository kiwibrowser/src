// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_manager_state.h"

#include <memory>

#include "base/command_line.h"
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/service_manager/public/mojom/connector.mojom.h"
#include "services/ui/common/accelerator_util.h"
#include "services/ui/common/switches.h"
#include "services/ui/ws/accelerator.h"
#include "services/ui/ws/cursor_location_manager.h"
#include "services/ui/ws/display.h"
#include "services/ui/ws/display_manager.h"
#include "services/ui/ws/event_location.h"
#include "services/ui/ws/platform_display.h"
#include "services/ui/ws/test_change_tracker.h"
#include "services/ui/ws/test_server_window_delegate.h"
#include "services/ui/ws/test_utils.h"
#include "services/ui/ws/window_manager_access_policy.h"
#include "services/ui/ws/window_manager_display_root.h"
#include "services/ui/ws/window_manager_state.h"
#include "services/ui/ws/window_server.h"
#include "services/ui/ws/window_tree.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/cursor/cursor.h"
#include "ui/events/event.h"

namespace ui {
namespace ws {
namespace test {

// Used in checking if an event was processed. See usage for examples.
void SetBoolToTrue(bool* value) {
  *value = true;
}

class WindowManagerStateTest : public testing::Test {
 public:
  WindowManagerStateTest();
  ~WindowManagerStateTest() override {}

  std::unique_ptr<Accelerator> CreateAccelerator();

  // Creates a child |server_window| with associataed |window_tree| and
  // |test_client|. The window is setup for processing input.
  void CreateSecondaryTree(TestWindowTreeClient** test_client,
                           WindowTree** window_tree,
                           ServerWindow** server_window);

  void DispatchInputEventToWindow(ServerWindow* target,
                                  const EventLocation& event_location,
                                  const ui::Event& event,
                                  Accelerator* accelerator);
  void OnEventAckTimeout(ClientSpecificId client_id);

  // This is the tree associated with the WindowManagerState. That is, this is
  // the WindowTree of the WindowManager.
  WindowTree* tree() {
    return window_event_targeting_helper_.window_server()->GetTreeWithId(
        kWindowManagerClientId);
  }
  // This is *not* the tree associated with the WindowManagerState, use tree()
  // if you need the window manager tree.
  WindowTree* window_tree() { return window_tree_; }
  TestWindowTreeClient* window_tree_client() { return window_tree_client_; }
  ServerWindow* window() { return window_; }
  TestWindowManager* window_manager() { return &window_manager_; }
  TestWindowTreeClient* wm_client() {
    return window_event_targeting_helper_.wm_client();
  }
  WindowManagerState* window_manager_state() { return window_manager_state_; }
  WindowServer* window_server() {
    return window_event_targeting_helper_.window_server();
  }
  ui::CursorType cursor_type() const {
    return window_event_targeting_helper_.cursor_type();
  }

  void EmbedAt(WindowTree* tree,
               const ClientWindowId& embed_window_id,
               uint32_t embed_flags,
               WindowTree** embed_tree,
               TestWindowTreeClient** embed_client_proxy) {
    mojom::WindowTreeClientPtr embed_client;
    auto client_request = mojo::MakeRequest(&embed_client);
    ASSERT_TRUE(
        tree->Embed(embed_window_id, std::move(embed_client), embed_flags));
    TestWindowTreeClient* client =
        window_event_targeting_helper_.last_window_tree_client();
    ASSERT_EQ(1u, client->tracker()->changes()->size());
    EXPECT_EQ(CHANGE_TYPE_EMBED, (*client->tracker()->changes())[0].type);
    client->tracker()->changes()->clear();
    *embed_client_proxy = client;
    *embed_tree = window_event_targeting_helper_.last_binding()->tree();
  }

  void DestroyWindowTree() {
    window_event_targeting_helper_.window_server()->DestroyTree(window_tree_);
    window_tree_ = nullptr;
  }

  // testing::Test:
  void SetUp() override;

 protected:
  // Handles WindowStateManager ack timeouts.
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;

 private:
  WindowEventTargetingHelper window_event_targeting_helper_;

  WindowManagerState* window_manager_state_;

  TestWindowManager window_manager_;
  ServerWindow* window_ = nullptr;
  WindowTree* window_tree_ = nullptr;
  TestWindowTreeClient* window_tree_client_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WindowManagerStateTest);
};

WindowManagerStateTest::WindowManagerStateTest()
    : task_runner_(new base::TestSimpleTaskRunner) {}

std::unique_ptr<Accelerator> WindowManagerStateTest::CreateAccelerator() {
  mojom::EventMatcherPtr matcher = ui::CreateKeyMatcher(
      ui::mojom::KeyboardCode::W, ui::mojom::kEventFlagControlDown);
  matcher->accelerator_phase = ui::mojom::AcceleratorPhase::POST_TARGET;
  uint32_t accelerator_id = 1;
  std::unique_ptr<Accelerator> accelerator(
      new Accelerator(accelerator_id, *matcher));
  return accelerator;
}

void WindowManagerStateTest::CreateSecondaryTree(
    TestWindowTreeClient** test_client,
    WindowTree** window_tree,
    ServerWindow** server_window) {
  window_event_targeting_helper_.CreateSecondaryTree(
      window_, gfx::Rect(20, 20, 20, 20), test_client, window_tree,
      server_window);
}

void WindowManagerStateTest::DispatchInputEventToWindow(
    ServerWindow* target,
    const EventLocation& event_location,
    const ui::Event& event,
    Accelerator* accelerator) {
  WindowManagerStateTestApi test_api(window_manager_state_);
  ClientSpecificId client_id = test_api.GetEventTargetClientId(target, false);
  test_api.DispatchInputEventToWindow(target, client_id, event_location, event,
                                      accelerator);
}

void WindowManagerStateTest::OnEventAckTimeout(
    ClientSpecificId client_id) {
  WindowManagerStateTestApi test_api(window_manager_state_);
  test_api.OnEventAckTimeout(client_id);
}

void WindowManagerStateTest::SetUp() {
  window_event_targeting_helper_.SetTaskRunner(task_runner_);
  window_manager_state_ = window_event_targeting_helper_.display()
                              ->window_manager_display_root()
                              ->window_manager_state();
  window_ = window_event_targeting_helper_.CreatePrimaryTree(
      gfx::Rect(0, 0, 100, 100), gfx::Rect(0, 0, 50, 50));
  window_tree_ = window_event_targeting_helper_.last_binding()->tree();
  window_tree_client_ =
      window_event_targeting_helper_.last_window_tree_client();
  ASSERT_TRUE(window_tree_->HasRoot(window_));

  WindowTreeTestApi(tree()).set_window_manager_internal(&window_manager_);
  wm_client()->tracker()->changes()->clear();
  window_tree_client_->tracker()->changes()->clear();
}

void SetCanFocusUp(ServerWindow* window) {
  while (window) {
    window->set_can_focus(true);
    window = window->parent();
  }
}

class WindowManagerStateTestAsync : public WindowManagerStateTest {
 public:
  WindowManagerStateTestAsync() {}
  ~WindowManagerStateTestAsync() override {}

  // WindowManagerStateTest:
  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kUseAsyncEventTargeting);
    WindowManagerStateTest::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WindowManagerStateTestAsync);
};

EventLocation EventLocationFromEvent(const Event& event,
                                     const Display& display) {
  EventLocation event_location(display.GetId());
  if (event.IsLocatedEvent()) {
    event_location.raw_location = event_location.location =
        event.AsLocatedEvent()->root_location_f();
  }
  return event_location;
}

// Tests that when an event is dispatched with no accelerator, that post target
// accelerator is not triggered.
TEST_F(WindowManagerStateTest, NullAccelerator) {
  WindowManagerState* state = window_manager_state();
  EXPECT_TRUE(state);

  ServerWindow* target = window();
  const Display* display = window_tree()->GetDisplay(target);
  ASSERT_TRUE(display);
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  DispatchInputEventToWindow(target, EventLocationFromEvent(key, *display), key,
                             nullptr);
  WindowTree* target_tree = window_tree();
  TestChangeTracker* tracker = window_tree_client()->tracker();
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=7",
      ChangesToDescription1(*tracker->changes())[0]);

  WindowTreeTestApi(target_tree).AckOldestEvent();
  EXPECT_FALSE(window_manager()->on_accelerator_called());
}

// Tests that when a post target accelerator is provided on an event, that it is
// called on ack.
TEST_F(WindowManagerStateTest, PostTargetAccelerator) {
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  std::unique_ptr<Accelerator> accelerator = CreateAccelerator();

  ServerWindow* target = window();
  const Display* display = window_tree()->GetDisplay(target);
  ASSERT_TRUE(display);
  DispatchInputEventToWindow(target, EventLocationFromEvent(key, *display), key,
                             accelerator.get());
  TestChangeTracker* tracker = window_tree_client()->tracker();
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=7",
      ChangesToDescription1(*tracker->changes())[0]);

  WindowTreeTestApi(window_tree()).AckOldestEvent();
  EXPECT_TRUE(window_manager()->on_accelerator_called());
  EXPECT_EQ(accelerator->id(), window_manager()->on_accelerator_id());
}

// Tests that if a pre target accelerator consumes the event no other processing
// is done.
TEST_F(WindowManagerStateTest, PreTargetConsumed) {
  // Set up two trees with focus on a child in the second.
  const ClientWindowId child_window_id(window_tree()->id(), 11);
  window_tree()->NewWindow(child_window_id, ServerWindow::Properties());
  ServerWindow* child_window =
      window_tree()->GetWindowByClientId(child_window_id);
  window_tree()->AddWindow(FirstRootId(window_tree()), child_window_id);
  child_window->SetVisible(true);
  SetCanFocusUp(child_window);
  child_window->parent()->set_is_activation_parent(true);
  ASSERT_TRUE(window_tree()->SetFocus(child_window_id));

  // Register a pre-accelerator.
  uint32_t accelerator_id = 11;
  {
    mojom::EventMatcherPtr matcher = ui::CreateKeyMatcher(
        ui::mojom::KeyboardCode::W, ui::mojom::kEventFlagControlDown);

    ASSERT_TRUE(window_manager_state()->event_processor()->AddAccelerator(
        accelerator_id, std::move(matcher)));
  }
  TestChangeTracker* tracker = wm_client()->tracker();
  tracker->changes()->clear();
  TestChangeTracker* tracker2 = window_tree_client()->tracker();
  tracker2->changes()->clear();

  // Send and ensure only the pre accelerator is called.
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  bool was_event_processed = false;
  window_manager_state()->ProcessEvent(&key, 0);
  window_manager_state()->ScheduleCallbackWhenDoneProcessingEvents(
      base::BindOnce(&SetBoolToTrue, &was_event_processed));
  EXPECT_TRUE(window_manager()->on_accelerator_called());
  EXPECT_EQ(accelerator_id, window_manager()->on_accelerator_id());
  EXPECT_TRUE(tracker->changes()->empty());
  EXPECT_TRUE(tracker2->changes()->empty());
  EXPECT_FALSE(was_event_processed);

  // Ack the accelerator, saying we consumed it.
  WindowTreeTestApi(tree()).AckLastAccelerator(mojom::EventResult::HANDLED);
  EXPECT_TRUE(was_event_processed);
  // Nothing should change.
  EXPECT_TRUE(tracker->changes()->empty());
  EXPECT_TRUE(tracker2->changes()->empty());

  was_event_processed = false;
  window_manager()->ClearAcceleratorCalled();

  // Repeat, but respond with UNHANDLED.
  window_manager_state()->ProcessEvent(&key, 0);
  window_manager_state()->ScheduleCallbackWhenDoneProcessingEvents(
      base::BindOnce(&SetBoolToTrue, &was_event_processed));
  EXPECT_FALSE(was_event_processed);
  EXPECT_TRUE(window_manager()->on_accelerator_called());
  EXPECT_EQ(accelerator_id, window_manager()->on_accelerator_id());
  EXPECT_TRUE(tracker->changes()->empty());
  EXPECT_TRUE(tracker2->changes()->empty());
  WindowTreeTestApi(tree()).AckLastAccelerator(mojom::EventResult::UNHANDLED);
  // |was_event_processed| is false because the accelerator wasn't completely
  // handled yet.
  EXPECT_FALSE(was_event_processed);

  EXPECT_TRUE(tracker->changes()->empty());
  // The focused window should get the event.
  EXPECT_EQ("InputEvent window=0,11 event_action=7",
            SingleChangeToDescription(*tracker2->changes()));
  WindowTreeTestApi(window_tree()).AckLastEvent(mojom::EventResult::HANDLED);
  EXPECT_TRUE(was_event_processed);
}

TEST_F(WindowManagerStateTest, AckWithProperties) {
  // Set up two trees with focus on a child in the second.
  const ClientWindowId child_window_id(window_tree()->id(), 11);
  window_tree()->NewWindow(child_window_id, ServerWindow::Properties());
  ServerWindow* child_window =
      window_tree()->GetWindowByClientId(child_window_id);
  window_tree()->AddWindow(FirstRootId(window_tree()), child_window_id);
  child_window->SetVisible(true);
  SetCanFocusUp(child_window);
  child_window->parent()->set_is_activation_parent(true);
  ASSERT_TRUE(window_tree()->SetFocus(child_window_id));

  // Register a pre-accelerator.
  uint32_t accelerator_id = 11;
  {
    mojom::EventMatcherPtr matcher = ui::CreateKeyMatcher(
        ui::mojom::KeyboardCode::W, ui::mojom::kEventFlagControlDown);

    ASSERT_TRUE(window_manager_state()->event_processor()->AddAccelerator(
        accelerator_id, std::move(matcher)));
  }
  TestChangeTracker* tracker = wm_client()->tracker();
  tracker->changes()->clear();
  TestChangeTracker* tracker2 = window_tree_client()->tracker();
  tracker2->changes()->clear();

  // Send and ensure only the pre accelerator is called.
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  window_manager_state()->ProcessEvent(&key, 0);
  EXPECT_TRUE(window_manager()->on_accelerator_called());
  EXPECT_EQ(accelerator_id, window_manager()->on_accelerator_id());
  EXPECT_TRUE(tracker->changes()->empty());
  EXPECT_TRUE(tracker2->changes()->empty());

  // Ack the accelerator, with unhandled.
  base::flat_map<std::string, std::vector<uint8_t>> event_properties;
  const std::string property_key = "x";
  const std::vector<uint8_t> property_value(2, 0xAB);
  event_properties[property_key] = property_value;
  EXPECT_TRUE(tracker->changes()->empty());
  EXPECT_TRUE(tracker2->changes()->empty());
  WindowTreeTestApi(tree()).AckLastAccelerator(mojom::EventResult::UNHANDLED,
                                               event_properties);

  // The focused window should get the event.
  EXPECT_EQ("InputEvent window=0,11 event_action=7",
            SingleChangeToDescription(*tracker2->changes()));
  ASSERT_EQ(1u, tracker2->changes()->size());
  EXPECT_EQ(1u, (*tracker2->changes())[0].key_event_properties.size());
  EXPECT_EQ(event_properties, (*tracker2->changes())[0].key_event_properties);

  WindowTreeTestApi(window_tree()).AckLastEvent(mojom::EventResult::HANDLED);
  tracker2->changes()->clear();

  // Send the event again, and ack with no properties. Ensure client gets no
  // properties.
  window_manager()->ClearAcceleratorCalled();
  window_manager_state()->ProcessEvent(&key, 0);
  EXPECT_TRUE(window_manager()->on_accelerator_called());
  EXPECT_EQ(accelerator_id, window_manager()->on_accelerator_id());
  EXPECT_TRUE(tracker->changes()->empty());
  EXPECT_TRUE(tracker2->changes()->empty());

  // Ack the accelerator with unhandled.
  WindowTreeTestApi(tree()).AckLastAccelerator(mojom::EventResult::UNHANDLED);

  // The focused window should get the event.
  EXPECT_EQ("InputEvent window=0,11 event_action=7",
            SingleChangeToDescription(*tracker2->changes()));
  ASSERT_EQ(1u, tracker2->changes()->size());
  EXPECT_TRUE((*tracker2->changes())[0].key_event_properties.empty());
}

// Tests that when a client handles an event that post target accelerators are
// not called.
TEST_F(WindowManagerStateTest, ClientHandlesEvent) {
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  std::unique_ptr<Accelerator> accelerator = CreateAccelerator();

  ServerWindow* target = window();
  const Display* display = window_tree()->GetDisplay(target);
  ASSERT_TRUE(display);
  DispatchInputEventToWindow(target, EventLocationFromEvent(key, *display), key,
                             accelerator.get());
  TestChangeTracker* tracker = window_tree_client()->tracker();
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=7",
      ChangesToDescription1(*tracker->changes())[0]);

  EXPECT_TRUE(WindowManagerStateTestApi(window_manager_state())
                  .AckInFlightEvent(mojom::EventResult::HANDLED));
  EXPECT_FALSE(window_manager()->on_accelerator_called());
}

// Tests that when an accelerator is deleted before an ack, that it is not
// called.
TEST_F(WindowManagerStateTest, AcceleratorDeleted) {
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  std::unique_ptr<Accelerator> accelerator(CreateAccelerator());

  ServerWindow* target = window();
  const Display* display = window_tree()->GetDisplay(target);
  ASSERT_TRUE(display);
  DispatchInputEventToWindow(target, EventLocationFromEvent(key, *display), key,
                             accelerator.get());
  TestChangeTracker* tracker = window_tree_client()->tracker();
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=7",
      ChangesToDescription1(*tracker->changes())[0]);

  accelerator.reset();
  EXPECT_TRUE(WindowManagerStateTestApi(window_manager_state())
                  .AckInFlightEvent(mojom::EventResult::UNHANDLED));
  EXPECT_FALSE(window_manager()->on_accelerator_called());
}

// Tests that a events arriving before an ack don't notify the tree until the
// ack arrives, and that the correct accelerator is called.
TEST_F(WindowManagerStateTest, EnqueuedAccelerators) {
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  std::unique_ptr<Accelerator> accelerator(CreateAccelerator());

  ServerWindow* target = window();
  const Display* display = window_tree()->GetDisplay(target);
  ASSERT_TRUE(display);
  DispatchInputEventToWindow(target, EventLocationFromEvent(key, *display), key,
                             accelerator.get());
  TestChangeTracker* tracker = window_tree_client()->tracker();
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=7",
      ChangesToDescription1(*tracker->changes())[0]);

  tracker->changes()->clear();
  ui::KeyEvent key2(ui::ET_KEY_PRESSED, ui::VKEY_Y, ui::EF_CONTROL_DOWN);
  mojom::EventMatcherPtr matcher = ui::CreateKeyMatcher(
      ui::mojom::KeyboardCode::Y, ui::mojom::kEventFlagControlDown);
  matcher->accelerator_phase = ui::mojom::AcceleratorPhase::POST_TARGET;
  uint32_t accelerator_id = 2;
  std::unique_ptr<Accelerator> accelerator2(
      new Accelerator(accelerator_id, *matcher));
  DispatchInputEventToWindow(target, EventLocationFromEvent(key2, *display),
                             key2, accelerator2.get());
  EXPECT_TRUE(tracker->changes()->empty());

  WindowTreeTestApi(window_tree()).AckOldestEvent();
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=7",
      ChangesToDescription1(*tracker->changes())[0]);
  EXPECT_TRUE(window_manager()->on_accelerator_called());
  EXPECT_EQ(accelerator->id(), window_manager()->on_accelerator_id());
}

// Tests that the accelerator is not sent when the tree is dying.
TEST_F(WindowManagerStateTest, DeleteTree) {
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  std::unique_ptr<Accelerator> accelerator = CreateAccelerator();

  ServerWindow* target = window();
  const Display* display = window_tree()->GetDisplay(target);
  ASSERT_TRUE(display);
  DispatchInputEventToWindow(target, EventLocationFromEvent(key, *display), key,
                             accelerator.get());
  TestChangeTracker* tracker = window_tree_client()->tracker();
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=7",
      ChangesToDescription1(*tracker->changes())[0]);

  window_manager_state()->OnWillDestroyTree(tree());
  EXPECT_FALSE(window_manager()->on_accelerator_called());
}

// Tests that if a tree is destroyed before acking, that the accelerator is
// still sent if it is not the root tree.
TEST_F(WindowManagerStateTest, DeleteNonRootTree) {
  TestWindowTreeClient* embed_connection = nullptr;
  WindowTree* target_tree = nullptr;
  ServerWindow* target = nullptr;
  CreateSecondaryTree(&embed_connection, &target_tree, &target);
  TestWindowManager target_window_manager;
  WindowTreeTestApi(target_tree)
      .set_window_manager_internal(&target_window_manager);

  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  std::unique_ptr<Accelerator> accelerator = CreateAccelerator();
  const Display* display = target_tree->GetDisplay(target);
  ASSERT_TRUE(display);
  DispatchInputEventToWindow(target, EventLocationFromEvent(key, *display), key,
                             accelerator.get());
  TestChangeTracker* tracker = embed_connection->tracker();
  ASSERT_EQ(1u, tracker->changes()->size());
  // clients that created this window is receiving the event, so client_id part
  // would be reset to 0 before sending back to clients.
  EXPECT_EQ("InputEvent window=0," + std::to_string(kEmbedTreeWindowId) +
                " event_action=7",
            ChangesToDescription1(*tracker->changes())[0]);
  EXPECT_TRUE(wm_client()->tracker()->changes()->empty());

  window_manager_state()->OnWillDestroyTree(target_tree);
  EXPECT_FALSE(target_window_manager.on_accelerator_called());
  EXPECT_TRUE(window_manager()->on_accelerator_called());
}

// Tests that if a tree is destroyed before acking an event, that mus won't
// then try to send any queued events.
TEST_F(WindowManagerStateTest, DontSendQueuedEventsToADeadTree) {
  ServerWindow* target = window();
  TestChangeTracker* tracker = window_tree_client()->tracker();

  const Display* display = window_tree()->GetDisplay(target);
  ASSERT_TRUE(display);
  ui::MouseEvent press(ui::ET_MOUSE_PRESSED, gfx::Point(5, 5), gfx::Point(5, 5),
                       base::TimeTicks(), EF_LEFT_MOUSE_BUTTON,
                       EF_LEFT_MOUSE_BUTTON);
  DispatchInputEventToWindow(target, EventLocationFromEvent(press, *display),
                             press, nullptr);
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=1",
      ChangesToDescription1(*tracker->changes())[0]);
  tracker->changes()->clear();
  // The above is not setting TreeAwaitingInputAck.

  // Queue the key release event; it should not be immediately dispatched
  // because there's no ACK for the last one.
  ui::MouseEvent release(ui::ET_MOUSE_RELEASED, gfx::Point(5, 5),
                         gfx::Point(5, 5), base::TimeTicks(),
                         EF_LEFT_MOUSE_BUTTON, EF_LEFT_MOUSE_BUTTON);
  DispatchInputEventToWindow(target, EventLocationFromEvent(release, *display),
                             release, nullptr);
  EXPECT_EQ(0u, tracker->changes()->size());

  // Destroying a window tree with an event in queue shouldn't crash.
  DestroyWindowTree();
}

// Tests that when an ack times out that the accelerator is notified.
TEST_F(WindowManagerStateTest, AckTimeout) {
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  std::unique_ptr<Accelerator> accelerator = CreateAccelerator();
  const Display* display = window_tree()->GetDisplay(window());
  ASSERT_TRUE(display);
  DispatchInputEventToWindow(window(), EventLocationFromEvent(key, *display),
                             key, accelerator.get());
  TestChangeTracker* tracker = window_tree_client()->tracker();
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=7",
      ChangesToDescription1(*tracker->changes())[0]);

  OnEventAckTimeout(window()->owning_tree_id());
  EXPECT_TRUE(window_manager()->on_accelerator_called());
  EXPECT_EQ(accelerator->id(), window_manager()->on_accelerator_id());
}

TEST_F(WindowManagerStateTest, InterceptingEmbedderReceivesEvents) {
  WindowTree* embedder_tree = tree();
  const ClientWindowId embed_window_id(embedder_tree->id(), 12);
  embedder_tree->NewWindow(embed_window_id, ServerWindow::Properties());
  ServerWindow* embedder_window =
      embedder_tree->GetWindowByClientId(embed_window_id);
  ASSERT_TRUE(
      embedder_tree->AddWindow(FirstRootId(embedder_tree), embed_window_id));

  TestWindowTreeClient* embedder_client = wm_client();

  {
    // Do a normal embed.
    const uint32_t embed_flags = 0;
    WindowTree* embed_tree = nullptr;
    TestWindowTreeClient* embed_client_proxy = nullptr;
    EmbedAt(embedder_tree, embed_window_id, embed_flags, &embed_tree,
            &embed_client_proxy);
    ASSERT_TRUE(embed_client_proxy);

    // Send an event to the embed window. It should go to the embedded client.
    const Display* display = embed_tree->GetDisplay(embedder_window);
    ASSERT_TRUE(display);
    ui::MouseEvent mouse(ui::ET_MOUSE_MOVED, gfx::Point(), gfx::Point(),
                         base::TimeTicks(), 0, 0);
    DispatchInputEventToWindow(embedder_window,
                               EventLocationFromEvent(mouse, *display), mouse,
                               nullptr);
    ASSERT_EQ(1u, embed_client_proxy->tracker()->changes()->size());
    EXPECT_EQ(CHANGE_TYPE_INPUT_EVENT,
              (*embed_client_proxy->tracker()->changes())[0].type);
    WindowTreeTestApi(embed_tree).AckLastEvent(mojom::EventResult::UNHANDLED);
    embed_client_proxy->tracker()->changes()->clear();
  }

  {
    // Do an embed where the embedder wants to intercept events to the embedded
    // tree.
    const uint32_t embed_flags = mojom::kEmbedFlagEmbedderInterceptsEvents;
    WindowTree* embed_tree = nullptr;
    TestWindowTreeClient* embed_client_proxy = nullptr;
    const ClientWindowId embed_client_window_id =
        embedder_window->frame_sink_id();
    EmbedAt(embedder_tree, embed_window_id, embed_flags, &embed_tree,
            &embed_client_proxy);
    ASSERT_TRUE(embed_client_proxy);
    embedder_client->tracker()->changes()->clear();

    // Send an event to the embed window. But this time, it should reach the
    // embedder.
    const Display* display = embed_tree->GetDisplay(embedder_window);
    ASSERT_TRUE(display);
    ui::MouseEvent mouse(ui::ET_MOUSE_MOVED, gfx::Point(), gfx::Point(),
                         base::TimeTicks(), 0, 0);
    DispatchInputEventToWindow(embedder_window,
                               EventLocationFromEvent(mouse, *display), mouse,
                               nullptr);
    ASSERT_EQ(0u, embed_client_proxy->tracker()->changes()->size());
    ASSERT_EQ(1u, embedder_client->tracker()->changes()->size());
    EXPECT_EQ(CHANGE_TYPE_INPUT_EVENT,
              (*embedder_client->tracker()->changes())[0].type);
    WindowTreeTestApi(embedder_tree)
        .AckLastEvent(mojom::EventResult::UNHANDLED);
    embedder_client->tracker()->changes()->clear();

    // Embed another tree in the embedded tree.
    const ClientWindowId nested_embed_window_id(embed_tree->id(), 23);
    embed_tree->NewWindow(nested_embed_window_id, ServerWindow::Properties());
    ASSERT_TRUE(
        embed_tree->AddWindow(embed_client_window_id, nested_embed_window_id));

    WindowTree* nested_embed_tree = nullptr;
    TestWindowTreeClient* nested_embed_client_proxy = nullptr;
    // Intercept events (kEmbedFlagEmbedderInterceptsEvents) is inherited, so
    // even though this doesn't explicitly specify
    // kEmbedFlagEmbedderInterceptsEvents it gets
    // kEmbedFlagEmbedderInterceptsEvents from the parent embedding.
    EmbedAt(embed_tree, nested_embed_window_id, 0, &nested_embed_tree,
            &nested_embed_client_proxy);
    ASSERT_TRUE(nested_embed_client_proxy);
    embed_client_proxy->tracker()->changes()->clear();
    embedder_client->tracker()->changes()->clear();

    // Send an event to the nested embed window. The event should still reach
    // the outermost embedder.
    ServerWindow* nested_embed_window =
        embed_tree->GetWindowByClientId(nested_embed_window_id);
    ASSERT_TRUE(nested_embed_window->parent());
    mouse = ui::MouseEvent(ui::ET_MOUSE_MOVED, gfx::Point(), gfx::Point(),
                           base::TimeTicks(), 0, 0);
    DispatchInputEventToWindow(nested_embed_window,
                               EventLocationFromEvent(mouse, *display), mouse,
                               nullptr);
    ASSERT_EQ(0u, nested_embed_client_proxy->tracker()->changes()->size());
    ASSERT_EQ(0u, embed_client_proxy->tracker()->changes()->size());

    ASSERT_EQ(1u, embedder_client->tracker()->changes()->size());
    EXPECT_EQ(CHANGE_TYPE_INPUT_EVENT,
              (*embedder_client->tracker()->changes())[0].type);
    WindowTreeTestApi(embedder_tree)
        .AckLastEvent(mojom::EventResult::UNHANDLED);
  }
}

// Ensures accelerators are forgotten between events.
TEST_F(WindowManagerStateTest, PostAcceleratorForgotten) {
  // Send an event that matches the accelerator and have the target respond
  // that it handled the event so that the accelerator isn't called.
  ui::KeyEvent accelerator_key(ui::ET_KEY_PRESSED, ui::VKEY_W,
                               ui::EF_CONTROL_DOWN);
  std::unique_ptr<Accelerator> accelerator = CreateAccelerator();
  ServerWindow* target = window();
  const Display* display = window_tree()->GetDisplay(target);
  ASSERT_TRUE(display);
  DispatchInputEventToWindow(target,
                             EventLocationFromEvent(accelerator_key, *display),
                             accelerator_key, accelerator.get());
  TestChangeTracker* tracker = window_tree_client()->tracker();
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=7",
      ChangesToDescription1(*tracker->changes())[0]);
  tracker->changes()->clear();
  WindowTreeTestApi(window_tree()).AckLastEvent(mojom::EventResult::HANDLED);
  EXPECT_FALSE(window_manager()->on_accelerator_called());

  // Send another event that doesn't match the accelerator, the accelerator
  // shouldn't be called.
  ui::KeyEvent non_accelerator_key(ui::ET_KEY_PRESSED, ui::VKEY_T,
                                   ui::EF_CONTROL_DOWN);
  DispatchInputEventToWindow(
      target, EventLocationFromEvent(non_accelerator_key, *display),
      non_accelerator_key, nullptr);
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(
      "InputEvent window=" + kWindowManagerClientIdString + ",1 event_action=7",
      ChangesToDescription1(*tracker->changes())[0]);
  WindowTreeTestApi(window_tree()).AckLastEvent(mojom::EventResult::UNHANDLED);
  EXPECT_FALSE(window_manager()->on_accelerator_called());
}

// Verifies there is no crash if the WindowTree of a window manager is destroyed
// with no roots.
TEST(WindowManagerStateShutdownTest, DestroyTreeBeforeDisplay) {
  WindowServerTestHelper ws_test_helper;
  WindowServer* window_server = ws_test_helper.window_server();
  TestScreenManager screen_manager;
  screen_manager.Init(window_server->display_manager());
  screen_manager.AddDisplay();
  AddWindowManager(window_server);
  ASSERT_EQ(1u, window_server->display_manager()->displays().size());
  Display* display = *(window_server->display_manager()->displays().begin());
  WindowManagerDisplayRoot* window_manager_display_root =
      display->window_manager_display_root();
  ASSERT_TRUE(window_manager_display_root);
  WindowTree* tree =
      window_manager_display_root->window_manager_state()->window_tree();
  ASSERT_EQ(1u, tree->roots().size());
  ClientWindowId root_client_id;
  ASSERT_TRUE(tree->IsWindowKnown(*(tree->roots().begin()), &root_client_id));
  EXPECT_TRUE(tree->DeleteWindow(root_client_id));
  window_server->DestroyTree(tree);
}

TEST_F(WindowManagerStateTest, CursorResetOverNoTarget) {
  ASSERT_EQ(1u, window_server()->display_manager()->displays().size());
  const ClientWindowId child_window_id(window_tree()->id(), 11);
  window_tree()->NewWindow(child_window_id, ServerWindow::Properties());
  ServerWindow* child_window =
      window_tree()->GetWindowByClientId(child_window_id);
  window_tree()->AddWindow(FirstRootId(window_tree()), child_window_id);
  child_window->SetVisible(true);
  child_window->SetBounds(gfx::Rect(0, 0, 20, 20));
  child_window->parent()->SetCursor(ui::CursorData(ui::CursorType::kCopy));
  EXPECT_EQ(ui::CursorType::kCopy, cursor_type());
  // Move the mouse outside the bounds of the child, so that the mouse is not
  // over any valid windows. Cursor should change to POINTER.
  ui::PointerEvent move(
      ui::ET_POINTER_MOVED, gfx::Point(25, 25), gfx::Point(25, 25), 0, 0,
      ui::PointerDetails(EventPointerType::POINTER_TYPE_MOUSE, 0),
      base::TimeTicks());
  window_manager_state()->ProcessEvent(&move, 0);
  // The event isn't over a valid target, which should trigger resetting the
  // cursor to POINTER.
  EXPECT_EQ(ui::CursorType::kPointer, cursor_type());
}

TEST(WindowManagerStateEventTest, AdjustEventLocation) {
  WindowServerTestHelper ws_test_helper;
  WindowServer* window_server = ws_test_helper.window_server();
  TestScreenManager screen_manager;
  screen_manager.Init(window_server->display_manager());
  AddWindowManager(window_server);
  const int64_t first_display_id = screen_manager.AddDisplay();
  const int64_t second_display_id = screen_manager.AddDisplay();
  Display* first_display =
      window_server->display_manager()->GetDisplayById(first_display_id);
  // As there are no child windows make sure the root is a valid target.
  first_display->window_manager_display_root()
      ->GetClientVisibleRoot()
      ->set_event_targeting_policy(
          mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);
  Display* second_display =
      window_server->display_manager()->GetDisplayById(second_display_id);
  ASSERT_TRUE(second_display);
  display::Display second_display_display = second_display->GetDisplay();
  second_display_display.set_bounds(gfx::Rect(100, 0, 100, 100));
  display::ViewportMetrics second_metrics;
  // The DIP display layout is horizontal and the pixel layout vertical.
  second_metrics.device_scale_factor = 1.0f;
  second_metrics.bounds_in_pixels = gfx::Rect(0, 200, 100, 100);
  second_metrics.ui_scale_factor = 1.0f;
  screen_manager.ModifyDisplay(second_display_display, second_metrics);
  const gfx::Point move_location(5, 210);
  ui::PointerEvent move(
      ui::ET_POINTER_MOVED, move_location, move_location, 0, 0,
      ui::PointerDetails(EventPointerType::POINTER_TYPE_MOUSE, 0),
      base::TimeTicks());
  WindowManagerDisplayRoot* window_manager_display_root =
      second_display->window_manager_display_root();
  TestChangeTracker* tracker =
      ws_test_helper.window_server_delegate()->last_client()->tracker();
  tracker->changes()->clear();
  window_manager_display_root->window_manager_state()->ProcessEvent(
      &move, first_display_id);
  ASSERT_EQ(1u, tracker->changes()->size());
  // |location2| is the location supplied in terms of the pixel display layout.
  EXPECT_EQ(gfx::PointF(move_location), (*tracker->changes())[0].location2);
  // |location1| is the location in DIP display layout.
  EXPECT_EQ(gfx::Point(105, 10), (*tracker->changes())[0].location1);
}

TEST_F(WindowManagerStateTest, CursorLocationManagerUpdatedOnMouseMove) {
  WindowManagerStateTestApi test_api(window_manager_state());
  ASSERT_EQ(1u, test_api.window_manager_display_roots().size());
  WindowManagerDisplayRoot* window_manager_display_root =
      test_api.window_manager_display_roots().begin()->get();
  ASSERT_TRUE(window_manager_display_root->GetClientVisibleRoot());
  // Install a transform on the root, which impacts the location reported to
  // clients.
  gfx::Transform transform;
  transform.Translate(6, 7);
  window_manager_display_root->GetClientVisibleRoot()->SetTransform(transform);
  ui::PointerEvent move(
      ui::ET_POINTER_MOVED, gfx::Point(25, 25), gfx::Point(25, 25), 0, 0,
      ui::PointerDetails(EventPointerType::POINTER_TYPE_MOUSE, 0),
      base::TimeTicks());
  // Tests add display with kInvalidDisplayId.
  window_manager_state()->ProcessEvent(&move, display::kInvalidDisplayId);
  CursorLocationManager* cursor_location_manager =
      window_server()->display_manager()->cursor_location_manager();
  // The location reported to clients is offset by the root transform.
  EXPECT_EQ(
      gfx::Point(19, 18),
      Atomic32ToPoint(CursorLocationManagerTestApi(cursor_location_manager)
                          .current_cursor_location()));
}

TEST_F(WindowManagerStateTest, SetCapture) {
  ASSERT_EQ(1u, window_server()->display_manager()->displays().size());
  Display* display = *(window_server()->display_manager()->displays().begin());
  TestPlatformDisplay* platform_display =
      static_cast<TestPlatformDisplay*>(display->platform_display());
  EXPECT_TRUE(window_tree()->SetCapture(FirstRootId(window_tree())));
  EXPECT_EQ(FirstRoot(window_tree()), window_manager_state()->capture_window());
  EXPECT_TRUE(platform_display->has_capture());
  EXPECT_TRUE(window_tree()->ReleaseCapture(FirstRootId(window_tree())));
  EXPECT_FALSE(platform_display->has_capture());

  // In unified mode capture should not propagate to the PlatformDisplay. This
  // is for compatibility with classic ash. See http://crbug.com/773348.
  display->SetDisplay(display::Display(display::kUnifiedDisplayId));
  EXPECT_TRUE(window_tree()->SetCapture(FirstRootId(window_tree())));
  EXPECT_EQ(FirstRoot(window_tree()), window_manager_state()->capture_window());
  EXPECT_FALSE(platform_display->has_capture());
  EXPECT_TRUE(window_tree()->ReleaseCapture(FirstRootId(window_tree())));
  EXPECT_FALSE(platform_display->has_capture());
}

TEST_F(WindowManagerStateTestAsync, CursorResetOverNoTargetAsync) {
  ASSERT_EQ(1u, window_server()->display_manager()->displays().size());
  const ClientWindowId child_window_id(window_tree()->id(), 11);
  window_tree()->NewWindow(child_window_id, ServerWindow::Properties());
  ServerWindow* child_window =
      window_tree()->GetWindowByClientId(child_window_id);
  window_tree()->AddWindow(FirstRootId(window_tree()), child_window_id);
  // Setup steps already do hit-test for mouse cursor update so this should go
  // to the queue in EventTargeter.
  EventTargeterTestApi event_targeter_test_api(
      EventProcessorTestApi(window_manager_state()->event_processor())
          .event_targeter());
  EXPECT_TRUE(event_targeter_test_api.HasPendingQueries());
  // But no events have been generated, so IsProcessingEvent() should be false.
  EXPECT_FALSE(window_manager_state()->event_processor()->IsProcessingEvent());
  child_window->SetVisible(true);
  child_window->SetBounds(gfx::Rect(0, 0, 20, 20));
  child_window->parent()->SetCursor(ui::CursorData(ui::CursorType::kCopy));
  // Move the mouse outside the bounds of the child, so that the mouse is not
  // over any valid windows. Cursor should change to POINTER.
  ui::PointerEvent move(
      ui::ET_POINTER_MOVED, gfx::Point(25, 25), gfx::Point(25, 25), 0, 0,
      ui::PointerDetails(EventPointerType::POINTER_TYPE_MOUSE, 0),
      base::TimeTicks());
  WindowManagerStateTestApi test_api(window_manager_state());
  EXPECT_TRUE(test_api.is_event_tasks_empty());
  window_manager_state()->ProcessEvent(&move, 0);
  EXPECT_FALSE(test_api.tree_awaiting_input_ack());
  EXPECT_TRUE(window_manager_state()->event_processor()->IsProcessingEvent());
  EXPECT_TRUE(test_api.is_event_tasks_empty());
  task_runner_->RunUntilIdle();
  EXPECT_TRUE(test_api.is_event_tasks_empty());
  // The event isn't over a valid target, which should trigger resetting the
  // cursor to POINTER.
  EXPECT_EQ(ui::CursorType::kPointer, cursor_type());
}

TEST_F(WindowManagerStateTest, DeleteTreeWithPendingEventAck) {
  ASSERT_EQ(1u, window_server()->display_manager()->displays().size());
  Display* display = *(window_server()->display_manager()->displays().begin());

  TestWindowTreeClient* embed_connection = nullptr;
  WindowTree* target_tree = nullptr;
  ServerWindow* target = nullptr;
  CreateSecondaryTree(&embed_connection, &target_tree, &target);
  target->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);

  bool was_event_processed = false;
  ui::PointerEvent move(
      ui::ET_POINTER_MOVED, gfx::Point(25, 25), gfx::Point(25, 25), 0, 0,
      ui::PointerDetails(EventPointerType::POINTER_TYPE_MOUSE, 0),
      base::TimeTicks());
  window_manager_state()->ProcessEvent(&move, display->GetId());
  window_manager_state()->ScheduleCallbackWhenDoneProcessingEvents(
      base::BindOnce(&SetBoolToTrue, &was_event_processed));
  EXPECT_FALSE(was_event_processed);
  EXPECT_TRUE(WindowTreeTestApi(target_tree).HasEventInFlight());
  window_server()->DestroyTree(target_tree);
  // Destroying the tree triggers the event to be considered processed.
  EXPECT_TRUE(was_event_processed);
}

TEST_F(WindowManagerStateTest, EventProcessedCallbackNotRunForGeneratedEvents) {
  ASSERT_EQ(1u, window_server()->display_manager()->displays().size());
  Display* display = *(window_server()->display_manager()->displays().begin());

  // Create two children of the root.
  ServerWindow* wm_root = FirstRoot(window_tree());
  wm_root->SetBounds(gfx::Rect(0, 0, 100, 100));
  wm_root->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);
  ServerWindow* child_window1 = NewWindowInTreeWithParent(
      window_tree(), wm_root, nullptr, gfx::Rect(0, 0, 20, 20));
  child_window1->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);
  ServerWindow* child_window2 = NewWindowInTreeWithParent(
      window_tree(), wm_root, nullptr, gfx::Rect(50, 0, 20, 20));
  child_window2->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);

  TestChangeTracker* tracker = window_tree_client()->tracker();
  tracker->changes()->clear();

  bool was_event_processed = false;
  ui::PointerEvent move1(
      ui::ET_POINTER_MOVED, gfx::Point(15, 15), gfx::Point(15, 15), 0, 0,
      ui::PointerDetails(EventPointerType::POINTER_TYPE_MOUSE, 0),
      base::TimeTicks());
  window_manager_state()->ProcessEvent(&move1, display->GetId());
  window_manager_state()->ScheduleCallbackWhenDoneProcessingEvents(
      base::BindOnce(&SetBoolToTrue, &was_event_processed));
  EXPECT_FALSE(was_event_processed);
  EXPECT_TRUE(WindowTreeTestApi(window_tree()).HasEventInFlight());
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(CHANGE_TYPE_INPUT_EVENT, (*tracker->changes())[0].type);
  tracker->changes()->clear();
  WindowTreeTestApi(window_tree()).AckLastEvent(mojom::EventResult::HANDLED);
  EXPECT_TRUE(was_event_processed);
  was_event_processed = false;

  ui::PointerEvent move2(
      ui::ET_POINTER_MOVED, gfx::Point(65, 15), gfx::Point(65, 15), 0, 0,
      ui::PointerDetails(EventPointerType::POINTER_TYPE_MOUSE, 0),
      base::TimeTicks());
  window_manager_state()->ProcessEvent(&move2, display->GetId());
  window_manager_state()->ScheduleCallbackWhenDoneProcessingEvents(
      base::BindOnce(&SetBoolToTrue, &was_event_processed));
  EXPECT_FALSE(was_event_processed);
  EXPECT_TRUE(WindowTreeTestApi(window_tree()).HasEventInFlight());
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(CHANGE_TYPE_INPUT_EVENT, (*tracker->changes())[0].type);
  EXPECT_EQ(ET_POINTER_EXITED, (*tracker->changes())[0].event_action);
  tracker->changes()->clear();

  WindowTreeTestApi(window_tree()).AckLastEvent(mojom::EventResult::HANDLED);
  // |was_event_processed| is false, because the exit was synthesized and will
  // be followed by a move.
  EXPECT_FALSE(was_event_processed);
  ASSERT_EQ(1u, tracker->changes()->size());
  EXPECT_EQ(CHANGE_TYPE_INPUT_EVENT, (*tracker->changes())[0].type);
  EXPECT_EQ(ET_POINTER_MOVED, (*tracker->changes())[0].event_action);
  tracker->changes()->clear();
  WindowTreeTestApi(window_tree()).AckLastEvent(mojom::EventResult::HANDLED);
  EXPECT_TRUE(was_event_processed);
  EXPECT_TRUE(tracker->changes()->empty());
}

}  // namespace test
}  // namespace ws
}  // namespace ui
