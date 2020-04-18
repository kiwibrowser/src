// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_tree.h"

#include <stdint.h>

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "services/service_manager/public/mojom/connector.mojom.h"
#include "services/ui/common/task_runner_test_base.h"
#include "services/ui/common/types.h"
#include "services/ui/common/util.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws/default_access_policy.h"
#include "services/ui/ws/display_manager.h"
#include "services/ui/ws/ids.h"
#include "services/ui/ws/platform_display.h"
#include "services/ui/ws/platform_display_factory.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/test_change_tracker.h"
#include "services/ui/ws/test_server_window_delegate.h"
#include "services/ui/ws/test_utils.h"
#include "services/ui/ws/user_display_manager.h"
#include "services/ui/ws/window_manager_access_policy.h"
#include "services/ui/ws/window_manager_display_root.h"
#include "services/ui/ws/window_server.h"
#include "services/ui/ws/window_server_delegate.h"
#include "services/ui/ws/window_tree_binding.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/cursor/cursor.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {
namespace ws {
namespace test {
namespace {

const std::string kNextWindowClientIdString =
    std::to_string(kWindowManagerClientId + 1);

std::string ClientWindowIdToString(const ClientWindowId& id) {
  return base::StringPrintf("%d,%d", id.client_id(), id.sink_id());
}

ClientWindowId BuildClientWindowId(WindowTree* tree,
                                   ClientSpecificId window_id) {
  return ClientWindowId(tree->id(), window_id);
}

// -----------------------------------------------------------------------------

ui::PointerEvent CreatePointerDownEvent(int x, int y) {
  return ui::PointerEvent(ui::TouchEvent(
      ui::ET_TOUCH_PRESSED, gfx::Point(x, y), ui::EventTimeForNow(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
}

ui::PointerEvent CreatePointerUpEvent(int x, int y) {
  return ui::PointerEvent(ui::TouchEvent(
      ui::ET_TOUCH_RELEASED, gfx::Point(x, y), ui::EventTimeForNow(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
}

ui::PointerEvent CreatePointerWheelEvent(int x, int y) {
  return ui::PointerEvent(
      ui::MouseWheelEvent(gfx::Vector2d(), gfx::Point(x, y), gfx::Point(x, y),
                          ui::EventTimeForNow(), ui::EF_NONE, ui::EF_NONE));
}

ui::PointerEvent CreateMouseMoveEvent(int x, int y) {
  return ui::PointerEvent(
      ui::MouseEvent(ui::ET_MOUSE_MOVED, gfx::Point(x, y), gfx::Point(x, y),
                     ui::EventTimeForNow(), ui::EF_NONE, ui::EF_NONE));
}

ui::PointerEvent CreateMouseDownEvent(int x, int y) {
  return ui::PointerEvent(
      ui::MouseEvent(ui::ET_MOUSE_PRESSED, gfx::Point(x, y), gfx::Point(x, y),
                     ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                     ui::EF_LEFT_MOUSE_BUTTON));
}

ui::PointerEvent CreateMouseUpEvent(int x, int y) {
  return ui::PointerEvent(
      ui::MouseEvent(ui::ET_MOUSE_RELEASED, gfx::Point(x, y), gfx::Point(x, y),
                     ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                     ui::EF_LEFT_MOUSE_BUTTON));
}

ServerWindow* GetCaptureWindow(Display* display) {
  return display->window_manager_display_root()
      ->window_manager_state()
      ->capture_window();
}

class TestMoveLoopWindowManager : public TestWindowManager {
 public:
  TestMoveLoopWindowManager(WindowTree* tree) : tree_(tree) {}
  ~TestMoveLoopWindowManager() override {}

  void WmPerformMoveLoop(uint32_t change_id,
                         Id window_id,
                         mojom::MoveLoopSource source,
                         const gfx::Point& cursor_location) override {
    static_cast<mojom::WindowManagerClient*>(tree_)->WmResponse(
        change_id, true);
  }

 private:
  WindowTree* tree_;

  DISALLOW_COPY_AND_ASSIGN(TestMoveLoopWindowManager);
};

// This creates a WindowTree similar to how connecting via WindowTreeFactory
// creates a tree.
WindowTree* CreateTreeViaFactory(WindowServer* window_server,
                                 TestWindowTreeBinding** binding) {
  const bool is_for_embedding = false;
  WindowTree* tree = new WindowTree(window_server, is_for_embedding, nullptr,
                                    std::make_unique<DefaultAccessPolicy>());
  *binding = new TestWindowTreeBinding(tree);
  window_server->AddTree(base::WrapUnique(tree), base::WrapUnique(*binding),
                         nullptr);
  return tree;
}

}  // namespace

// -----------------------------------------------------------------------------

class WindowTreeTest : public testing::Test {
 public:
  WindowTreeTest() {}
  ~WindowTreeTest() override {}

  ui::CursorType cursor_type() {
    return window_event_targeting_helper_.cursor_type();
  }
  Display* display() { return window_event_targeting_helper_.display(); }
  TestWindowTreeClient* last_window_tree_client() {
    return window_event_targeting_helper_.last_window_tree_client();
  }
  TestWindowTreeClient* wm_client() {
    return window_event_targeting_helper_.wm_client();
  }
  WindowServer* window_server() {
    return window_event_targeting_helper_.window_server();
  }
  WindowTree* wm_tree() {
    return window_event_targeting_helper_.window_server()->GetTreeWithId(
        kWindowManagerClientId);
  }
  WindowTree* last_tree() {
    return window_event_targeting_helper_.last_binding()
               ? window_event_targeting_helper_.last_binding()->tree()
               : nullptr;
  }

  // Simulates an event coming from the system. The event is not acked
  // immediately, use AckPreviousEvent() to do that.
  void DispatchEventWithoutAck(const ui::Event& event) {
    std::unique_ptr<Event> tmp = ui::Event::Clone(event);
    display()->ProcessEvent(tmp.get());
  }

  void set_window_manager_internal(WindowTree* tree,
                                   mojom::WindowManager* wm_internal) {
    WindowTreeTestApi(tree).set_window_manager_internal(wm_internal);
  }

  void AckPreviousEvent() {
    WindowManagerStateTestApi test_api(
        display()->window_manager_display_root()->window_manager_state());
    while (test_api.tree_awaiting_input_ack()) {
      WindowTreeTestApi(test_api.tree_awaiting_input_ack())
          .AckOldestEvent(mojom::EventResult::HANDLED);
    }
  }

  void DispatchEventAndAckImmediately(const ui::Event& event) {
    DispatchEventWithoutAck(event);
    AckPreviousEvent();
  }

  // Creates a new window from wm_tree() and embeds a new client in it.
  void SetupEventTargeting(TestWindowTreeClient** out_client,
                           WindowTree** window_tree,
                           ServerWindow** window);

  // Creates a new tree as the specified user. This does what creation via
  // a WindowTreeFactory does.
  WindowTree* CreateNewTree(TestWindowTreeBinding** binding) {
    return CreateTreeViaFactory(window_server(), binding);
  }

  TestWindowServerDelegate* test_window_server_delegate() {
    return window_event_targeting_helper_.test_window_server_delegate();
  }

 protected:
  WindowEventTargetingHelper window_event_targeting_helper_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WindowTreeTest);
};

// Creates a new window in wm_tree(), adds it to the root, embeds a
// new client in the window and creates a child of said window. |window| is
// set to the child of |window_tree| that is created.
void WindowTreeTest::SetupEventTargeting(TestWindowTreeClient** out_client,
                                         WindowTree** window_tree,
                                         ServerWindow** window) {
  ServerWindow* embed_window = window_event_targeting_helper_.CreatePrimaryTree(
      gfx::Rect(0, 0, 100, 100), gfx::Rect(0, 0, 50, 50));
  window_event_targeting_helper_.CreateSecondaryTree(
      embed_window, gfx::Rect(20, 20, 20, 20), out_client, window_tree, window);
  FirstRoot(*window_tree)->set_is_activation_parent(true);
}

// Verifies focus does not change on pointer events.
TEST_F(WindowTreeTest, DontFocusOnPointer) {
  const ClientWindowId embed_window_id =
      BuildClientWindowId(wm_tree(), kEmbedTreeWindowId);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id, ServerWindow::Properties()));
  ServerWindow* embed_window = wm_tree()->GetWindowByClientId(embed_window_id);
  ASSERT_TRUE(embed_window);
  EXPECT_TRUE(wm_tree()->SetWindowVisibility(embed_window_id, true));
  ASSERT_TRUE(FirstRoot(wm_tree()));
  const ClientWindowId wm_root_id = FirstRootId(wm_tree());
  EXPECT_TRUE(wm_tree()->AddWindow(wm_root_id, embed_window_id));
  ServerWindow* wm_root = FirstRoot(wm_tree());
  ASSERT_TRUE(wm_root);
  wm_root->SetBounds(gfx::Rect(0, 0, 100, 100));
  // This tests expects |wm_root| to be a possible target.
  wm_root->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);
  display()->root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  mojom::WindowTreeClientPtr client;
  wm_client()->Bind(mojo::MakeRequest(&client));
  const uint32_t embed_flags = 0;
  wm_tree()->Embed(embed_window_id, std::move(client), embed_flags);
  WindowTree* tree1 = window_server()->GetTreeWithRoot(embed_window);
  ASSERT_TRUE(tree1 != nullptr);
  ASSERT_NE(tree1, wm_tree());

  embed_window->SetBounds(gfx::Rect(0, 0, 50, 50));

  const ClientWindowId child1_id(
      BuildClientWindowId(tree1, kEmbedTreeWindowId));
  EXPECT_TRUE(tree1->NewWindow(child1_id, ServerWindow::Properties()));
  EXPECT_TRUE(tree1->AddWindow(ClientWindowIdForWindow(tree1, embed_window),
                               child1_id));
  ServerWindow* child1 = tree1->GetWindowByClientId(child1_id);
  ASSERT_TRUE(child1);
  child1->SetVisible(true);
  child1->SetBounds(gfx::Rect(20, 20, 20, 20));

  embed_window->set_is_activation_parent(true);

  // Dispatch a pointer event to the child1, focus should be null.
  DispatchEventAndAckImmediately(CreatePointerDownEvent(21, 22));
  Display* display1 = tree1->GetDisplay(embed_window);
  EXPECT_EQ(nullptr, display1->GetFocusedWindow());
}

TEST_F(WindowTreeTest, BasicInputEventTarget) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(
      SetupEventTargeting(&embed_client, &tree, &window));

  // Send an event to |v1|. |embed_client| should get the event, not
  // |wm_client|, since |v1| lives inside an embedded window.
  DispatchEventAndAckImmediately(CreatePointerDownEvent(21, 22));
  ASSERT_EQ(0u, wm_client()->tracker()->changes()->size());
  ASSERT_EQ(1u, embed_client->tracker()->changes()->size());
  // embed_client created this window that is receiving the event, so client_id
  // part would be reset to 0 before sending back to clients.
  EXPECT_EQ("InputEvent window=0," + std::to_string(kEmbedTreeWindowId) +
                " event_action=" + std::to_string(ui::ET_POINTER_DOWN),
            ChangesToDescription1(*embed_client->tracker()->changes())[0]);
}

TEST_F(WindowTreeTest, EventDispatcherMouseCursorSourceWindowResetOnRemove) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  window->SetBounds(gfx::Rect(0, 0, 100, 100));

  DispatchEventAndAckImmediately(CreatePointerDownEvent(21, 22));

  WindowManagerState* wms =
      display()->window_manager_display_root()->window_manager_state();
  EXPECT_EQ(window, wms->event_processor()->mouse_cursor_source_window());

  window->parent()->Remove(window);
  // The remove should reset the mouse_cursor_source_window(). The important
  // thing is it changes to something other than |window|.
  EXPECT_NE(window, wms->event_processor()->mouse_cursor_source_window());
}

// Verifies SetChildModalParent() works correctly.
TEST_F(WindowTreeTest, SetChildModalParent) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  ClientWindowId w1_id;
  ServerWindow* w1 = NewWindowInTreeWithParent(tree, window, &w1_id);
  ASSERT_TRUE(w1);
  ClientWindowId w2_id;
  ServerWindow* w2 = NewWindowInTreeWithParent(tree, window, &w2_id);
  ASSERT_TRUE(w2);
  ASSERT_TRUE(tree->SetChildModalParent(w1_id, w2_id));
  EXPECT_EQ(w2, w1->GetChildModalParent());
  ASSERT_TRUE(tree->SetChildModalParent(w1_id, ClientWindowId()));
  EXPECT_EQ(nullptr, w1->GetChildModalParent());
}

// Tests that a client can watch for events outside its bounds.
TEST_F(WindowTreeTest, StartPointerWatcher) {
  // Create an embedded client.
  TestWindowTreeClient* client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&client, &tree, &window));

  // Create an event outside the bounds of the client.
  ui::PointerEvent pointer_down = CreatePointerDownEvent(5, 5);

  // Events are not watched before starting a watcher.
  DispatchEventAndAckImmediately(pointer_down);
  ASSERT_EQ(0u, client->tracker()->changes()->size());

  // Create a watcher for all events excluding move events.
  WindowTreeTestApi(tree).StartPointerWatcher(false);

  // Pointer-down events are sent to the client.
  DispatchEventAndAckImmediately(pointer_down);
  ASSERT_EQ(1u, client->tracker()->changes()->size());
  EXPECT_EQ("PointerWatcherEvent event_action=" +
                std::to_string(ui::ET_POINTER_DOWN) + " window=null",
            ChangesToDescription1(*client->tracker()->changes())[0]);
  client->tracker()->changes()->clear();

  // Create a pointer wheel event outside the bounds of the client.
  ui::PointerEvent pointer_wheel = CreatePointerWheelEvent(5, 5);

  // Pointer-wheel events are sent to the client.
  DispatchEventAndAckImmediately(pointer_wheel);
  ASSERT_EQ(1u, client->tracker()->changes()->size());
  EXPECT_EQ("PointerWatcherEvent event_action=" +
                std::to_string(ui::ET_POINTER_WHEEL_CHANGED) + " window=null",
            ChangesToDescription1(*client->tracker()->changes())[0]);
  client->tracker()->changes()->clear();

  // Stopping the watcher stops sending events to the client.
  WindowTreeTestApi(tree).StopPointerWatcher();
  DispatchEventAndAckImmediately(pointer_down);
  ASSERT_EQ(0u, client->tracker()->changes()->size());
  DispatchEventAndAckImmediately(pointer_wheel);
  ASSERT_EQ(0u, client->tracker()->changes()->size());

  // Create a watcher for all events including move events.
  WindowTreeTestApi(tree).StartPointerWatcher(true);

  // Pointer-wheel events are sent to the client.
  DispatchEventAndAckImmediately(pointer_wheel);
  ASSERT_EQ(1u, client->tracker()->changes()->size());
  EXPECT_EQ("PointerWatcherEvent event_action=" +
                std::to_string(ui::ET_POINTER_WHEEL_CHANGED) + " window=null",
            ChangesToDescription1(*client->tracker()->changes())[0]);
}

// Verifies PointerWatcher sees windows known to it.
TEST_F(WindowTreeTest, PointerWatcherGetsWindow) {
  // Create an embedded client.
  TestWindowTreeClient* client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&client, &tree, &window));

  WindowTreeTestApi(wm_tree()).StartPointerWatcher(false);

  // Create and dispatch an event that targets the embedded window.
  ui::PointerEvent pointer_down = CreatePointerDownEvent(25, 25);
  DispatchEventAndAckImmediately(pointer_down);

  ASSERT_EQ(1u, wm_client()->tracker()->changes()->size());
  EXPECT_EQ(
      "PointerWatcherEvent event_action=" +
          std::to_string(ui::ET_POINTER_DOWN) + " window=" +
          ClientWindowIdToString(ClientWindowIdForWindow(wm_tree(), window)),
      ChangesToDescription1(*wm_client()->tracker()->changes())[0]);
}

// Tests that a client using a pointer watcher does not receive events that
// don't match the |want_moves| setting.
TEST_F(WindowTreeTest, StartPointerWatcherNonMatching) {
  // Create an embedded client.
  TestWindowTreeClient* client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&client, &tree, &window));

  // Create a watcher for all events excluding move events.
  WindowTreeTestApi(tree).StartPointerWatcher(false);

  // Pointer-move events are not sent to the client, since they don't match.
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(5, 5));
  ASSERT_EQ(0u, client->tracker()->changes()->size());
}

// Tests that an event that both hits a client window and matches a pointer
// watcher is sent only once to the client.
TEST_F(WindowTreeTest, StartPointerWatcherSendsOnce) {
  // Create an embedded client.
  TestWindowTreeClient* client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&client, &tree, &window));

  // Create a watcher for all events excluding move events (which do not
  // cause focus changes).
  WindowTreeTestApi(tree).StartPointerWatcher(false);

  // Create an event inside the bounds of the client.
  ui::PointerEvent pointer_up = CreatePointerUpEvent(25, 25);

  // The event is dispatched once, with a flag set that it matched the pointer
  // watcher.
  DispatchEventAndAckImmediately(pointer_up);
  ASSERT_EQ(1u, client->tracker()->changes()->size());
  // clients that created this window is receiving the event, so client_id part
  // would be reset to 0 before sending back to clients.
  EXPECT_EQ("InputEvent window=0," + std::to_string(kEmbedTreeWindowId) +
                " event_action=" + std::to_string(ui::ET_POINTER_UP) +
                " matches_pointer_watcher",
            SingleChangeToDescription(*client->tracker()->changes()));
}

// Tests that a pointer watcher cannot watch keystrokes.
TEST_F(WindowTreeTest, StartPointerWatcherKeyEventsDisallowed) {
  TestWindowTreeBinding* other_binding;
  WindowTree* other_tree = CreateNewTree(&other_binding);
  other_binding->client()->tracker()->changes()->clear();

  WindowTreeTestApi(other_tree).StartPointerWatcher(false);
  ui::KeyEvent key_pressed(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  DispatchEventAndAckImmediately(key_pressed);
  EXPECT_EQ(0u, other_binding->client()->tracker()->changes()->size());
  EXPECT_EQ("InputEvent window=" + std::to_string(kWindowServerClientId) +
                ",3 event_action=" + std::to_string(ui::ET_KEY_PRESSED),
            SingleChangeToDescription(*wm_client()->tracker()->changes()));

  WindowTreeTestApi(wm_tree()).StartPointerWatcher(false);
  ui::KeyEvent key_released(ui::ET_KEY_RELEASED, ui::VKEY_A, ui::EF_NONE);
  DispatchEventAndAckImmediately(key_released);
  EXPECT_EQ(0u, other_binding->client()->tracker()->changes()->size());
}

TEST_F(WindowTreeTest, KeyEventSentToWindowManagerWhenNothingFocused) {
  ui::KeyEvent key_pressed(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  DispatchEventAndAckImmediately(key_pressed);
  EXPECT_EQ("InputEvent window=" + std::to_string(kWindowServerClientId) +
                ",3 event_action=" + std::to_string(ui::ET_KEY_PRESSED),
            SingleChangeToDescription(*wm_client()->tracker()->changes()));
}

TEST_F(WindowTreeTest, CursorChangesWhenMouseOverWindowAndWindowSetsCursor) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  // Like in BasicInputEventTarget, we send a pointer down event to be
  // dispatched. This is only to place the mouse cursor over that window though.
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(21, 22));

  // Set the cursor on the parent as that is where the cursor is picked up from.
  window->parent()->SetCursor(ui::CursorData(ui::CursorType::kIBeam));

  // Because the cursor is over the window when SetCursor was called, we should
  // have immediately changed the cursor.
  EXPECT_EQ(ui::CursorType::kIBeam, cursor_type());
}

TEST_F(WindowTreeTest, CursorChangesWhenEnteringWindowWithDifferentCursor) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  // Let's create a pointer event outside the window and then move the pointer
  // inside.
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(5, 5));
  // Set the cursor on the parent as that is where the cursor is picked up from.
  window->parent()->SetCursor(ui::CursorData(ui::CursorType::kIBeam));
  EXPECT_EQ(ui::CursorType::kPointer, cursor_type());

  DispatchEventAndAckImmediately(CreateMouseMoveEvent(21, 22));
  EXPECT_EQ(ui::CursorType::kIBeam, cursor_type());
}

TEST_F(WindowTreeTest, DragOutsideWindow) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  // Start with the cursor outside the window. Setting the cursor shouldn't
  // change the cursor.
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(5, 5));
  // Set the cursor on the parent as that is where the cursor is picked up from.
  window->parent()->SetCursor(ui::CursorData(ui::CursorType::kIBeam));
  EXPECT_EQ(ui::CursorType::kPointer, cursor_type());

  // Move the pointer to the inside of the window
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(21, 22));
  EXPECT_EQ(ui::CursorType::kIBeam, cursor_type());

  // Start the drag.
  DispatchEventAndAckImmediately(CreateMouseDownEvent(21, 22));
  EXPECT_EQ(ui::CursorType::kIBeam, cursor_type());

  // Move the cursor (mouse is still down) outside the window.
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(5, 5));
  EXPECT_EQ(ui::CursorType::kIBeam, cursor_type());

  // Release the cursor. We should now adapt the cursor of the window
  // underneath the pointer.
  DispatchEventAndAckImmediately(CreateMouseUpEvent(5, 5));
  EXPECT_EQ(ui::CursorType::kPointer, cursor_type());
}

TEST_F(WindowTreeTest, ChangingWindowBoundsChangesCursor) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  // Put the cursor just outside the bounds of the window.
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(41, 41));
  // Sets the cursor on the root as that is where the cursor is picked up from.
  window->parent()->SetCursor(ui::CursorData(ui::CursorType::kIBeam));
  EXPECT_EQ(ui::CursorType::kPointer, cursor_type());

  // Expand the bounds of the window so they now include where the cursor now
  // is.
  window->SetBounds(gfx::Rect(20, 20, 25, 25));
  EXPECT_EQ(ui::CursorType::kIBeam, cursor_type());

  // Contract the bounds again.
  window->SetBounds(gfx::Rect(20, 20, 20, 20));
  EXPECT_EQ(ui::CursorType::kPointer, cursor_type());
}

TEST_F(WindowTreeTest, WindowReorderingChangesCursor) {
  // Setup two trees parented to the root with the same bounds.
  ServerWindow* embed_window1 =
      window_event_targeting_helper_.CreatePrimaryTree(
          gfx::Rect(0, 0, 200, 200), gfx::Rect(0, 0, 50, 50));
  ServerWindow* embed_window2 =
      window_event_targeting_helper_.CreatePrimaryTree(
          gfx::Rect(0, 0, 200, 200), gfx::Rect(0, 0, 50, 50));

  ASSERT_EQ(embed_window1->parent(), embed_window2->parent());
  embed_window1->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);
  embed_window2->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);
  embed_window1->SetCursor(ui::CursorData(ui::CursorType::kIBeam));
  embed_window2->SetCursor(ui::CursorData(ui::CursorType::kCross));
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(5, 5));
  // Cursor should match that of top-most window, which is |embed_window2|.
  EXPECT_EQ(ui::CursorType::kCross, cursor_type());
  // Move |embed_window1| on top, cursor should now match it.
  embed_window1->parent()->StackChildAtTop(embed_window1);
  EXPECT_EQ(ui::CursorType::kIBeam, cursor_type());
}

// Assertions around moving cursor between trees with roots.
TEST_F(WindowTreeTest, CursorMultipleTrees) {
  // Setup two trees parented to the root with the same bounds.
  ServerWindow* embed_window1 =
      window_event_targeting_helper_.CreatePrimaryTree(
          gfx::Rect(0, 0, 200, 200), gfx::Rect(0, 0, 10, 10));
  ServerWindow* embed_window2 =
      window_event_targeting_helper_.CreatePrimaryTree(
          gfx::Rect(0, 0, 200, 200), gfx::Rect(20, 20, 20, 20));
  embed_window1->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);
  embed_window2->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);
  embed_window2->parent()->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);
  embed_window1->SetCursor(ui::CursorData(ui::CursorType::kIBeam));
  embed_window2->SetCursor(ui::CursorData(ui::CursorType::kCross));
  embed_window1->parent()->SetCursor(ui::CursorData(ui::CursorType::kCopy));

  // Create a child of |embed_window1|.
  ServerWindow* embed_window1_child = NewWindowInTreeWithParent(
      window_server()->GetTreeWithRoot(embed_window1), embed_window1);
  ASSERT_TRUE(embed_window1_child);
  embed_window1_child->SetBounds(gfx::Rect(0, 0, 10, 10));
  embed_window1_child->SetVisible(true);

  // Move mouse into |embed_window1|.
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(5, 5));
  EXPECT_EQ(ui::CursorType::kIBeam, cursor_type());

  // Move mouse into |embed_window2|.
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(25, 25));
  EXPECT_EQ(ui::CursorType::kCross, cursor_type());

  // Move mouse into area between, which should use cursor set on parent.
  DispatchEventAndAckImmediately(CreateMouseMoveEvent(15, 15));
  EXPECT_EQ(ui::CursorType::kCopy, cursor_type());
}

TEST_F(WindowTreeTest, EventAck) {
  const ClientWindowId embed_window_id =
      BuildClientWindowId(wm_tree(), kEmbedTreeWindowId);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id, ServerWindow::Properties()));
  EXPECT_TRUE(wm_tree()->SetWindowVisibility(embed_window_id, true));
  ASSERT_TRUE(FirstRoot(wm_tree()));
  EXPECT_TRUE(wm_tree()->AddWindow(FirstRootId(wm_tree()), embed_window_id));
  ASSERT_EQ(1u, display()->root_window()->children().size());
  ServerWindow* wm_root = FirstRoot(wm_tree());
  ASSERT_TRUE(wm_root);
  wm_root->SetBounds(gfx::Rect(0, 0, 100, 100));
  // This tests expects |wm_root| to be a possible target.
  wm_root->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);

  wm_client()->tracker()->changes()->clear();
  DispatchEventWithoutAck(CreateMouseMoveEvent(21, 22));
  ASSERT_EQ(1u, wm_client()->tracker()->changes()->size());
  EXPECT_EQ("InputEvent window=" + std::to_string(kWindowServerClientId) +
                ",3 event_action=" + std::to_string(ui::ET_POINTER_MOVED),
            ChangesToDescription1(*wm_client()->tracker()->changes())[0]);
  wm_client()->tracker()->changes()->clear();

  // Send another event. This event shouldn't reach the client.
  DispatchEventWithoutAck(CreateMouseMoveEvent(21, 22));
  ASSERT_EQ(0u, wm_client()->tracker()->changes()->size());

  // Ack the first event. That should trigger the dispatch of the second event.
  AckPreviousEvent();
  ASSERT_EQ(1u, wm_client()->tracker()->changes()->size());
  EXPECT_EQ("InputEvent window=" + std::to_string(kWindowServerClientId) +
                ",3 event_action=" + std::to_string(ui::ET_POINTER_MOVED),
            ChangesToDescription1(*wm_client()->tracker()->changes())[0]);
}

TEST_F(WindowTreeTest, RemoveWindowFromParentWithQueuedEvent) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  window->SetBounds(gfx::Rect(0, 0, 100, 100));
  window->set_is_activation_parent(true);
  ClientWindowId w1_id;
  ServerWindow* w1 =
      NewWindowInTreeWithParent(tree, window, &w1_id, gfx::Rect(0, 0, 20, 20));
  ASSERT_TRUE(w1);
  ClientWindowId w2_id;
  ServerWindow* w2 =
      NewWindowInTreeWithParent(tree, window, &w2_id, gfx::Rect(25, 0, 20, 20));
  ASSERT_TRUE(w2);

  DispatchEventAndAckImmediately(CreateMouseMoveEvent(5, 5));

  // This moves between |w1| and |w2|, which results in two events (move and
  // enter).
  DispatchEventWithoutAck(CreateMouseMoveEvent(27, 5));

  // There should be an event in flight for the move.
  EXPECT_TRUE(WindowTreeTestApi(tree).HasEventInFlight());

  // Simulate the client taking too long.
  WindowManagerStateTestApi wm_state_test_api(
      wm_tree()->window_manager_state());
  wm_state_test_api.OnEventAckTimeout(tree->id());

  // There should be an event queued (for the enter).
  EXPECT_EQ(1u, WindowTreeTestApi(tree).event_queue_size());

  // Remove the window from the hierarchy, which should make it so the client
  // doesn't get the queued event.
  w2->parent()->Remove(w2);

  // Ack the in flight event, which should trigger processing the queued event.
  // Because |w2| was removed, the event should not be dispatched to the client
  // and WindowManagerState should no longer be waiting (because there are no
  // inflight events).
  WindowTreeTestApi(tree).AckOldestEvent();
  EXPECT_FALSE(WindowTreeTestApi(tree).HasEventInFlight());
  EXPECT_EQ(nullptr, wm_state_test_api.tree_awaiting_input_ack());
}

// Establish client, call Embed() in WM, make sure to get FrameSinkId.
TEST_F(WindowTreeTest, Embed) {
  const ClientWindowId embed_window_id =
      BuildClientWindowId(wm_tree(), kEmbedTreeWindowId);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id, ServerWindow::Properties()));
  ServerWindow* embed_window = wm_tree()->GetWindowByClientId(embed_window_id);
  ASSERT_TRUE(embed_window);
  const ClientWindowId wm_root_id = FirstRootId(wm_tree());
  EXPECT_TRUE(wm_tree()->AddWindow(wm_root_id, embed_window_id));
  ServerWindow* wm_root = FirstRoot(wm_tree());
  ASSERT_TRUE(wm_root);
  mojom::WindowTreeClientPtr client;
  wm_client()->Bind(mojo::MakeRequest(&client));
  const uint32_t embed_flags = 0;
  wm_tree()->Embed(embed_window_id, std::move(client), embed_flags);
  ASSERT_EQ(1u, wm_client()->tracker()->changes()->size())
      << SingleChangeToDescription(*wm_client()->tracker()->changes());
  // The window manager should be told about the FrameSinkId of the embedded
  // window. Clients that created this window is receiving the event, so
  // client_id part would be reset to 0 before sending back to clients.
  EXPECT_EQ(
      base::StringPrintf(
          "OnFrameSinkIdAllocated window=%s %s",
          ClientWindowIdToString(ClientWindowId(0, embed_window_id.sink_id()))
              .c_str(),
          embed_window->frame_sink_id().ToString().c_str()),
      SingleChangeToDescription(*wm_client()->tracker()->changes()));
}

TEST_F(WindowTreeTest, DisallowSetSystemModalForEmbedded) {
  const ClientWindowId embed_window_id =
      BuildClientWindowId(wm_tree(), kEmbedTreeWindowId);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id, ServerWindow::Properties()));
  ServerWindow* embed_window = wm_tree()->GetWindowByClientId(embed_window_id);
  ASSERT_TRUE(embed_window);
  const ClientWindowId wm_root_id = FirstRootId(wm_tree());
  EXPECT_TRUE(wm_tree()->AddWindow(wm_root_id, embed_window_id));
  ServerWindow* wm_root = FirstRoot(wm_tree());
  ASSERT_TRUE(wm_root);
  mojom::WindowTreeClientPtr client;
  wm_client()->Bind(mojo::MakeRequest(&client));
  const uint32_t embed_flags = 0;
  ASSERT_TRUE(
      wm_tree()->Embed(embed_window_id, std::move(client), embed_flags));
  ASSERT_TRUE(last_tree());
  EXPECT_FALSE(last_tree()->SetModalType(
      ClientWindowIdForWindow(last_tree(), embed_window), MODAL_TYPE_SYSTEM));
}

TEST_F(WindowTreeTest, ModalTypeSystemToModalTypeNone) {
  const ClientWindowId test_window_id = BuildClientWindowId(wm_tree(), 21);
  EXPECT_TRUE(wm_tree()->NewWindow(test_window_id, ServerWindow::Properties()));
  ServerWindow* test_window = wm_tree()->GetWindowByClientId(test_window_id);
  ASSERT_TRUE(test_window);
  test_window->SetVisible(true);
  const ClientWindowId wm_root_id = FirstRootId(wm_tree());
  EXPECT_TRUE(wm_tree()->AddWindow(wm_root_id, test_window_id));
  EXPECT_TRUE(wm_tree()->SetModalType(test_window_id, MODAL_TYPE_SYSTEM));
  WindowManagerState* wms =
      display()->window_manager_display_root()->window_manager_state();
  ModalWindowControllerTestApi modal_window_controller_test_api(
      EventProcessorTestApi(wms->event_processor()).modal_window_controller());
  EXPECT_EQ(test_window,
            modal_window_controller_test_api.GetActiveSystemModalWindow());
  EXPECT_TRUE(wm_tree()->SetModalType(test_window_id, MODAL_TYPE_NONE));
  EXPECT_EQ(nullptr,
            modal_window_controller_test_api.GetActiveSystemModalWindow());
}

TEST_F(WindowTreeTest, ModalTypeSystemUnparentedThenParented) {
  const ClientWindowId test_window_id = BuildClientWindowId(wm_tree(), 21);
  EXPECT_TRUE(wm_tree()->NewWindow(test_window_id, ServerWindow::Properties()));
  ServerWindow* test_window = wm_tree()->GetWindowByClientId(test_window_id);
  ASSERT_TRUE(test_window);
  test_window->SetVisible(true);
  const ClientWindowId wm_root_id = FirstRootId(wm_tree());
  EXPECT_TRUE(wm_tree()->SetModalType(test_window_id, MODAL_TYPE_SYSTEM));
  WindowManagerState* wms =
      display()->window_manager_display_root()->window_manager_state();
  ModalWindowControllerTestApi modal_window_controller_test_api(
      EventProcessorTestApi(wms->event_processor()).modal_window_controller());
  EXPECT_EQ(nullptr,
            modal_window_controller_test_api.GetActiveSystemModalWindow());
  EXPECT_TRUE(wm_tree()->AddWindow(wm_root_id, test_window_id));
  EXPECT_EQ(test_window,
            modal_window_controller_test_api.GetActiveSystemModalWindow());
  EXPECT_TRUE(wm_tree()->SetModalType(test_window_id, MODAL_TYPE_NONE));
  EXPECT_EQ(nullptr,
            modal_window_controller_test_api.GetActiveSystemModalWindow());
}

// Establish client, call NewTopLevelWindow(), make sure get id, and make
// sure client paused.
TEST_F(WindowTreeTest, NewTopLevelWindow) {
  TestWindowManager wm_internal;
  set_window_manager_internal(wm_tree(), &wm_internal);

  TestWindowTreeBinding* child_binding;
  WindowTree* child_tree = CreateNewTree(&child_binding);
  child_binding->client()->tracker()->changes()->clear();
  child_binding->client()->set_record_on_change_completed(true);

  // Create a new top level window.
  base::flat_map<std::string, std::vector<uint8_t>> properties;
  const uint32_t initial_change_id = 17;
  // Explicitly use an id that does not contain the client id.
  const ClientWindowId embed_window_id2_in_child(child_tree->id(), 27);
  static_cast<mojom::WindowTree*>(child_tree)
      ->NewTopLevelWindow(
          initial_change_id,
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          properties);

  // The binding should be paused until the wm acks the change.
  uint32_t wm_change_id = 0u;
  ASSERT_TRUE(wm_internal.did_call_create_top_level_window(&wm_change_id));
  EXPECT_TRUE(child_binding->is_paused());

  // Create the window for |embed_window_id2_in_child|.
  const ClientWindowId embed_window_id2 = BuildClientWindowId(wm_tree(), 2);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id2, ServerWindow::Properties()));
  EXPECT_TRUE(wm_tree()->SetWindowVisibility(embed_window_id2, true));
  EXPECT_TRUE(wm_tree()->AddWindow(FirstRootId(wm_tree()), embed_window_id2));

  // Ack the change, which should resume the binding.
  child_binding->client()->tracker()->changes()->clear();
  static_cast<mojom::WindowManagerClient*>(wm_tree())
      ->OnWmCreatedTopLevelWindow(
          wm_change_id,
          wm_tree()->ClientWindowIdToTransportId(embed_window_id2));

  ServerWindow* embed_window = wm_tree()->GetWindowByClientId(embed_window_id2);
  ASSERT_TRUE(embed_window);
  EXPECT_EQ(embed_window_id2_in_child, embed_window->frame_sink_id());
  EXPECT_FALSE(child_binding->is_paused());
  // TODO(fsamuel): Currently the FrameSinkId maps directly to the server's
  // window ID. This is likely bad from a security perspective and should be
  // fixed.
  EXPECT_EQ(base::StringPrintf(
                "TopLevelCreated id=17 window_id=%s drawn=true",
                ClientWindowIdToString(
                    ClientWindowId(0, embed_window_id2_in_child.sink_id()))
                    .c_str()),
            SingleChangeToDescription(
                *child_binding->client()->tracker()->changes()));
  child_binding->client()->tracker()->changes()->clear();

  // Change the visibility of the window from the owner and make sure the
  // client sees the right id.
  EXPECT_TRUE(embed_window->visible());
  ASSERT_TRUE(wm_tree()->SetWindowVisibility(
      ClientWindowIdForWindow(wm_tree(), embed_window), false));
  EXPECT_FALSE(embed_window->visible());
  EXPECT_EQ("VisibilityChanged window=" +
                ClientWindowIdToString(
                    ClientWindowId(0, embed_window_id2_in_child.sink_id())) +
                " visible=false",
            SingleChangeToDescription(
                *child_binding->client()->tracker()->changes()));

  // Set the visibility from the child using the client assigned id.
  ASSERT_TRUE(child_tree->SetWindowVisibility(embed_window_id2_in_child, true));
  EXPECT_TRUE(embed_window->visible());
}

// Tests that only the capture window can release capture.
TEST_F(WindowTreeTest, ExplicitSetCapture) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));
  const ServerWindow* root_window = *tree->roots().begin();
  tree->AddWindow(FirstRootId(tree), ClientWindowIdForWindow(tree, window));
  window->SetBounds(gfx::Rect(0, 0, 100, 100));
  ASSERT_TRUE(tree->GetDisplay(window));

  // Set capture.
  mojom::WindowTree* mojom_window_tree = static_cast<mojom::WindowTree*>(tree);
  uint32_t change_id = 42;
  mojom_window_tree->SetCapture(
      change_id, tree->ClientWindowIdToTransportId(window->frame_sink_id()));
  Display* display = tree->GetDisplay(window);
  EXPECT_EQ(window, GetCaptureWindow(display));

  // Only the capture window should be able to release capture
  mojom_window_tree->ReleaseCapture(
      ++change_id,
      tree->ClientWindowIdToTransportId(root_window->frame_sink_id()));
  EXPECT_EQ(window, GetCaptureWindow(display));

  mojom_window_tree->ReleaseCapture(
      ++change_id, tree->ClientWindowIdToTransportId(window->frame_sink_id()));
  EXPECT_EQ(nullptr, GetCaptureWindow(display));
}

// Tests that while a client is interacting with input, that capture is not
// allowed for invisible windows.
TEST_F(WindowTreeTest, CaptureWindowMustBeVisible) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));
  tree->AddWindow(FirstRootId(tree), ClientWindowIdForWindow(tree, window));
  window->SetBounds(gfx::Rect(0, 0, 100, 100));
  ASSERT_TRUE(tree->GetDisplay(window));

  DispatchEventWithoutAck(CreatePointerDownEvent(10, 10));
  window->SetVisible(false);
  EXPECT_FALSE(tree->SetCapture(ClientWindowIdForWindow(tree, window)));
  EXPECT_NE(window, GetCaptureWindow(tree->GetDisplay(window)));
}

// Tests that showing a modal window releases the capture if the capture is on a
// descendant of the modal parent.
TEST_F(WindowTreeTest, ShowModalWindowWithDescendantCapture) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* w1 = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &w1));

  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  const ServerWindow* root_window = *tree->roots().begin();
  ClientWindowId root_window_id = ClientWindowIdForWindow(tree, root_window);
  ClientWindowId w1_id = ClientWindowIdForWindow(tree, w1);
  Display* display = tree->GetDisplay(w1);

  // Create |w11| as a child of |w1| and make it visible.
  ClientWindowId w11_id = BuildClientWindowId(tree, 11);
  ASSERT_TRUE(tree->NewWindow(w11_id, ServerWindow::Properties()));
  ServerWindow* w11 = tree->GetWindowByClientId(w11_id);
  w11->SetBounds(gfx::Rect(10, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(w1_id, w11_id));
  ASSERT_TRUE(tree->SetWindowVisibility(w11_id, true));

  // Create |w2| as a child of |root_window| and modal to |w1| and leave it
  // hidden.
  ClientWindowId w2_id = BuildClientWindowId(tree, 3);
  ASSERT_TRUE(tree->NewWindow(w2_id, ServerWindow::Properties()));
  ServerWindow* w2 = tree->GetWindowByClientId(w2_id);
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(root_window_id, w2_id));
  ASSERT_TRUE(tree->AddTransientWindow(w1_id, w2_id));
  ASSERT_TRUE(tree->SetModalType(w2_id, MODAL_TYPE_WINDOW));

  // Set capture to |w11|.
  DispatchEventWithoutAck(CreatePointerDownEvent(25, 25));
  ASSERT_TRUE(tree->SetCapture(w11_id));
  EXPECT_EQ(w11, GetCaptureWindow(display));
  AckPreviousEvent();

  // Make |w2| visible. This should release capture as capture is set to a
  // descendant of the modal parent.
  ASSERT_TRUE(tree->SetWindowVisibility(w2_id, true));
  EXPECT_EQ(nullptr, GetCaptureWindow(display));
}

// Tests that setting a visible window as modal releases the capture if the
// capture is on a descendant of the modal parent.
TEST_F(WindowTreeTest, VisibleWindowToModalWithDescendantCapture) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* w1 = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &w1));

  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  const ServerWindow* root_window = *tree->roots().begin();
  ClientWindowId root_window_id = ClientWindowIdForWindow(tree, root_window);
  ClientWindowId w1_id = ClientWindowIdForWindow(tree, w1);
  Display* display = tree->GetDisplay(w1);

  // Create |w11| as a child of |w1| and make it visible.
  ClientWindowId w11_id = BuildClientWindowId(tree, 11);
  ASSERT_TRUE(tree->NewWindow(w11_id, ServerWindow::Properties()));
  ServerWindow* w11 = tree->GetWindowByClientId(w11_id);
  w11->SetBounds(gfx::Rect(10, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(w1_id, w11_id));
  ASSERT_TRUE(tree->SetWindowVisibility(w11_id, true));

  // Create |w2| as a child of |root_window| and make it visible.
  ClientWindowId w2_id = BuildClientWindowId(tree, 3);
  ASSERT_TRUE(tree->NewWindow(w2_id, ServerWindow::Properties()));
  ServerWindow* w2 = tree->GetWindowByClientId(w2_id);
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(root_window_id, w2_id));
  ASSERT_TRUE(tree->SetWindowVisibility(w2_id, true));

  // Set capture to |w11|.
  DispatchEventWithoutAck(CreatePointerDownEvent(25, 25));
  ASSERT_TRUE(tree->SetCapture(w11_id));
  EXPECT_EQ(w11, GetCaptureWindow(display));
  AckPreviousEvent();

  // Set |w2| modal to |w1|. This should release the capture as the capture is
  // set to a descendant of the modal parent.
  ASSERT_TRUE(tree->AddTransientWindow(w1_id, w2_id));
  ASSERT_TRUE(tree->SetModalType(w2_id, MODAL_TYPE_WINDOW));
  EXPECT_EQ(nullptr, GetCaptureWindow(display));
}

// Tests that showing a modal window does not change capture if the capture is
// not on a descendant of the modal parent.
TEST_F(WindowTreeTest, ShowModalWindowWithNonDescendantCapture) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* w1 = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &w1));

  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  const ServerWindow* root_window = *tree->roots().begin();
  ClientWindowId root_window_id = ClientWindowIdForWindow(tree, root_window);
  ClientWindowId w1_id = ClientWindowIdForWindow(tree, w1);
  Display* display = tree->GetDisplay(w1);

  // Create |w2| as a child of |root_window| and modal to |w1| and leave it
  // hidden.
  ClientWindowId w2_id = BuildClientWindowId(tree, kEmbedTreeWindowId + 1);
  ASSERT_TRUE(tree->NewWindow(w2_id, ServerWindow::Properties()));
  ServerWindow* w2 = tree->GetWindowByClientId(w2_id);
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(root_window_id, w2_id));
  ASSERT_TRUE(tree->AddTransientWindow(w1_id, w2_id));
  ASSERT_TRUE(tree->SetModalType(w2_id, MODAL_TYPE_WINDOW));

  // Create |w3| as a child of |root_window| and make it visible.
  ClientWindowId w3_id = BuildClientWindowId(tree, kEmbedTreeWindowId + 2);
  ASSERT_TRUE(tree->NewWindow(w3_id, ServerWindow::Properties()));
  ServerWindow* w3 = tree->GetWindowByClientId(w3_id);
  w3->SetBounds(gfx::Rect(70, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(root_window_id, w3_id));
  ASSERT_TRUE(tree->SetWindowVisibility(w3_id, true));

  // Set capture to |w3|.
  DispatchEventWithoutAck(CreatePointerDownEvent(25, 25));
  ASSERT_TRUE(tree->SetCapture(w3_id));
  EXPECT_EQ(w3, GetCaptureWindow(display));
  AckPreviousEvent();

  // Make |w2| visible. This should not change the capture as the capture is not
  // set to a descendant of the modal parent.
  ASSERT_TRUE(tree->SetWindowVisibility(w2_id, true));
  EXPECT_EQ(w3, GetCaptureWindow(display));
}

// Tests that setting a visible window as modal does not change the capture if
// the capture is not set to a descendant of the modal parent.
TEST_F(WindowTreeTest, VisibleWindowToModalWithNonDescendantCapture) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* w1 = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &w1));

  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  const ServerWindow* root_window = *tree->roots().begin();
  ClientWindowId root_window_id = ClientWindowIdForWindow(tree, root_window);
  ClientWindowId w1_id = ClientWindowIdForWindow(tree, w1);
  Display* display = tree->GetDisplay(w1);

  // Create |w2| and |w3| as children of |root_window| and make them visible.
  ClientWindowId w2_id = BuildClientWindowId(tree, kEmbedTreeWindowId + 1);
  ASSERT_TRUE(tree->NewWindow(w2_id, ServerWindow::Properties()));
  ServerWindow* w2 = tree->GetWindowByClientId(w2_id);
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(root_window_id, w2_id));
  ASSERT_TRUE(tree->SetWindowVisibility(w2_id, true));

  ClientWindowId w3_id = BuildClientWindowId(tree, kEmbedTreeWindowId + 2);
  ASSERT_TRUE(tree->NewWindow(w3_id, ServerWindow::Properties()));
  ServerWindow* w3 = tree->GetWindowByClientId(w3_id);
  w3->SetBounds(gfx::Rect(70, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(root_window_id, w3_id));
  ASSERT_TRUE(tree->SetWindowVisibility(w3_id, true));

  // Set capture to |w3|.
  DispatchEventWithoutAck(CreatePointerDownEvent(25, 25));
  ASSERT_TRUE(tree->SetCapture(w3_id));
  EXPECT_EQ(w3, GetCaptureWindow(display));
  AckPreviousEvent();

  // Set |w2| modal to |w1|. This should not release the capture as the capture
  // is not set to a descendant of the modal parent.
  ASSERT_TRUE(tree->AddTransientWindow(w1_id, w2_id));
  ASSERT_TRUE(tree->SetModalType(w2_id, MODAL_TYPE_WINDOW));
  EXPECT_EQ(w3, GetCaptureWindow(display));
}

// Tests that showing a system modal window releases the capture.
TEST_F(WindowTreeTest, ShowSystemModalWindowWithCapture) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* w1 = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &w1));

  w1->SetBounds(gfx::Rect(10, 10, 10, 10));
  const ServerWindow* root_window = *tree->roots().begin();
  ClientWindowId root_window_id = ClientWindowIdForWindow(tree, root_window);
  ClientWindowId w1_id = ClientWindowIdForWindow(tree, w1);
  Display* display = tree->GetDisplay(w1);

  // Create a system modal window |w2| as a child of |root_window| and leave it
  // hidden.
  ClientWindowId w2_id = BuildClientWindowId(tree, kEmbedTreeWindowId + 1);
  ASSERT_TRUE(tree->NewWindow(w2_id, ServerWindow::Properties()));
  ServerWindow* w2 = tree->GetWindowByClientId(w2_id);
  w2->SetBounds(gfx::Rect(30, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(root_window_id, w2_id));
  ASSERT_TRUE(tree->SetModalType(w2_id, MODAL_TYPE_SYSTEM));

  // Set capture to |w1|.
  DispatchEventWithoutAck(CreatePointerDownEvent(15, 15));
  ASSERT_TRUE(tree->SetCapture(w1_id));
  EXPECT_EQ(w1, GetCaptureWindow(display));
  AckPreviousEvent();

  // Make |w2| visible. This should release capture as it is system modal
  // window.
  ASSERT_TRUE(tree->SetWindowVisibility(w2_id, true));
  EXPECT_EQ(nullptr, GetCaptureWindow(display));
}

// Tests that setting a visible window as modal to system releases the capture.
TEST_F(WindowTreeTest, VisibleWindowToSystemModalWithCapture) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* w1 = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &w1));

  w1->SetBounds(gfx::Rect(10, 10, 10, 10));
  const ServerWindow* root_window = *tree->roots().begin();
  ClientWindowId root_window_id = ClientWindowIdForWindow(tree, root_window);
  ClientWindowId w1_id = ClientWindowIdForWindow(tree, w1);
  Display* display = tree->GetDisplay(w1);

  // Create |w2| as a child of |root_window| and make it visible.
  ClientWindowId w2_id = BuildClientWindowId(tree, kEmbedTreeWindowId + 1);
  ASSERT_TRUE(tree->NewWindow(w2_id, ServerWindow::Properties()));
  ServerWindow* w2 = tree->GetWindowByClientId(w2_id);
  w2->SetBounds(gfx::Rect(30, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(root_window_id, w2_id));
  ASSERT_TRUE(tree->SetWindowVisibility(w2_id, true));

  // Set capture to |w1|.
  DispatchEventWithoutAck(CreatePointerDownEvent(15, 15));
  ASSERT_TRUE(tree->SetCapture(w1_id));
  EXPECT_EQ(w1, GetCaptureWindow(display));
  AckPreviousEvent();

  // Make |w2| modal to system. This should release capture.
  ASSERT_TRUE(tree->SetModalType(w2_id, MODAL_TYPE_SYSTEM));
  EXPECT_EQ(nullptr, GetCaptureWindow(display));
}

// Tests that moving the capture window to a modal parent releases the capture
// as capture cannot be blocked by a modal window.
TEST_F(WindowTreeTest, MoveCaptureWindowToModalParent) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* w1 = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &w1));

  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  ServerWindow* root_window = FirstRoot(tree);
  ClientWindowId root_window_id = ClientWindowIdForWindow(tree, root_window);
  ClientWindowId w1_id = ClientWindowIdForWindow(tree, w1);
  Display* display = tree->GetDisplay(w1);

  // Create |w2| and |w3| as children of |root_window| and make them visible.
  ClientWindowId w2_id = BuildClientWindowId(tree, kEmbedTreeWindowId + 1);
  ASSERT_TRUE(tree->NewWindow(w2_id, ServerWindow::Properties()));
  ServerWindow* w2 = tree->GetWindowByClientId(w2_id);
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(root_window_id, w2_id));
  ASSERT_TRUE(tree->SetWindowVisibility(w2_id, true));

  ClientWindowId w3_id = BuildClientWindowId(tree, kEmbedTreeWindowId + 2);
  ASSERT_TRUE(tree->NewWindow(w3_id, ServerWindow::Properties()));
  ServerWindow* w3 = tree->GetWindowByClientId(w3_id);
  w3->SetBounds(gfx::Rect(70, 10, 10, 10));
  ASSERT_TRUE(tree->AddWindow(root_window_id, w3_id));
  ASSERT_TRUE(tree->SetWindowVisibility(w3_id, true));

  // Set |w2| modal to |w1|.
  ASSERT_TRUE(tree->AddTransientWindow(w1_id, w2_id));
  ASSERT_TRUE(tree->SetModalType(w2_id, MODAL_TYPE_WINDOW));

  // Set capture to |w3|.
  DispatchEventWithoutAck(CreatePointerDownEvent(25, 25));
  ASSERT_TRUE(tree->SetCapture(w3_id));
  EXPECT_EQ(w3, GetCaptureWindow(display));
  AckPreviousEvent();

  // Make |w3| child of |w1|. This should release capture as |w3| is now blocked
  // by a modal window.
  ASSERT_TRUE(tree->AddWindow(w1_id, w3_id));
  EXPECT_EQ(nullptr, GetCaptureWindow(display));
}

// Tests that opacity can be set on a known window.
TEST_F(WindowTreeTest, SetOpacity) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  const float new_opacity = 0.5f;
  EXPECT_NE(new_opacity, window->opacity());
  ASSERT_TRUE(tree->SetWindowOpacity(ClientWindowIdForWindow(tree, window),
                                     new_opacity));
  EXPECT_EQ(new_opacity, window->opacity());

  // Re-applying the same opacity will succeed.
  EXPECT_TRUE(tree->SetWindowOpacity(ClientWindowIdForWindow(tree, window),
                                     new_opacity));
}

// Tests that opacity requests for unknown windows are rejected.
TEST_F(WindowTreeTest, SetOpacityFailsOnUnknownWindow) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  const ClientWindowId root_id = FirstRootId(tree);

  EXPECT_FALSE(tree->SetWindowOpacity(
      ClientWindowId(root_id.client_id(), root_id.sink_id() + 10), .5f));
}

TEST_F(WindowTreeTest, SetCaptureTargetsRightConnection) {
  ServerWindow* window = window_event_targeting_helper_.CreatePrimaryTree(
      gfx::Rect(0, 0, 100, 100), gfx::Rect(0, 0, 50, 50));
  WindowTree* owning_tree =
      window_server()->GetTreeWithId(window->owning_tree_id());
  WindowTree* embed_tree = window_server()->GetTreeWithRoot(window);
  ASSERT_NE(owning_tree, embed_tree);
  ASSERT_TRUE(
      owning_tree->SetCapture(ClientWindowIdForWindow(owning_tree, window)));
  DispatchEventWithoutAck(CreateMouseMoveEvent(21, 22));
  WindowManagerStateTestApi wm_state_test_api(
      display()->window_manager_display_root()->window_manager_state());
  EXPECT_EQ(owning_tree, wm_state_test_api.tree_awaiting_input_ack());
  AckPreviousEvent();

  // Set capture from the embedded client and make sure it gets the event.
  ASSERT_TRUE(
      embed_tree->SetCapture(ClientWindowIdForWindow(embed_tree, window)));
  DispatchEventWithoutAck(CreateMouseMoveEvent(22, 23));
  EXPECT_EQ(embed_tree, wm_state_test_api.tree_awaiting_input_ack());
}

TEST_F(WindowTreeTest, ValidMoveLoopWithWM) {
  TestWindowManager wm_internal;
  set_window_manager_internal(wm_tree(), &wm_internal);

  TestWindowTreeBinding* child_binding;
  WindowTree* child_tree = CreateNewTree(&child_binding);
  child_binding->client()->tracker()->changes()->clear();
  child_binding->client()->set_record_on_change_completed(true);

  // Create a new top level window.
  base::flat_map<std::string, std::vector<uint8_t>> properties;
  const uint32_t initial_change_id = 17;
  // Explicitly use an id that does not contain the client id.
  const ClientWindowId embed_window_id2_in_child(child_tree->id(), 27);
  static_cast<mojom::WindowTree*>(child_tree)
      ->NewTopLevelWindow(
          initial_change_id,
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          properties);

  // The binding should be paused until the wm acks the change.
  uint32_t wm_change_id = 0u;
  ASSERT_TRUE(wm_internal.did_call_create_top_level_window(&wm_change_id));
  EXPECT_TRUE(child_binding->is_paused());

  // Create the window for |embed_window_id2_in_child|.
  const ClientWindowId embed_window_id2 = BuildClientWindowId(wm_tree(), 2);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id2, ServerWindow::Properties()));
  EXPECT_TRUE(wm_tree()->SetWindowVisibility(embed_window_id2, true));
  EXPECT_TRUE(wm_tree()->AddWindow(FirstRootId(wm_tree()), embed_window_id2));

  // Ack the change, which should resume the binding.
  child_binding->client()->tracker()->changes()->clear();
  static_cast<mojom::WindowManagerClient*>(wm_tree())
      ->OnWmCreatedTopLevelWindow(
          wm_change_id,
          wm_tree()->ClientWindowIdToTransportId(embed_window_id2));
  EXPECT_FALSE(child_binding->is_paused());

  // The child_tree is the one that has to make this call; the
  const uint32_t change_id = 7;
  static_cast<mojom::WindowTree*>(child_tree)
      ->PerformWindowMove(
          change_id,
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          mojom::MoveLoopSource::MOUSE, gfx::Point(0, 0));

  EXPECT_TRUE(wm_internal.on_perform_move_loop_called());
}

TEST_F(WindowTreeTest, MoveLoopAckOKByWM) {
  TestMoveLoopWindowManager wm_internal(wm_tree());
  set_window_manager_internal(wm_tree(), &wm_internal);

  TestWindowTreeBinding* child_binding;
  WindowTree* child_tree = CreateNewTree(&child_binding);
  child_binding->client()->tracker()->changes()->clear();
  child_binding->client()->set_record_on_change_completed(true);

  // Create a new top level window.
  base::flat_map<std::string, std::vector<uint8_t>> properties;
  const uint32_t initial_change_id = 17;
  // Explicitly use an id that does not contain the client id.
  const ClientWindowId embed_window_id2_in_child(child_tree->id(), 27);
  static_cast<mojom::WindowTree*>(child_tree)
      ->NewTopLevelWindow(
          initial_change_id,
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          properties);

  // The binding should be paused until the wm acks the change.
  uint32_t wm_change_id = 0u;
  ASSERT_TRUE(wm_internal.did_call_create_top_level_window(&wm_change_id));
  EXPECT_TRUE(child_binding->is_paused());

  // Create the window for |embed_window_id2_in_child|.
  const ClientWindowId embed_window_id2 = BuildClientWindowId(wm_tree(), 2);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id2, ServerWindow::Properties()));
  EXPECT_TRUE(wm_tree()->SetWindowVisibility(embed_window_id2, true));
  EXPECT_TRUE(wm_tree()->AddWindow(FirstRootId(wm_tree()), embed_window_id2));

  // Ack the change, which should resume the binding.
  child_binding->client()->tracker()->changes()->clear();
  static_cast<mojom::WindowManagerClient*>(wm_tree())
      ->OnWmCreatedTopLevelWindow(
          wm_change_id,
          wm_tree()->ClientWindowIdToTransportId(embed_window_id2));
  EXPECT_FALSE(child_binding->is_paused());

  // The child_tree is the one that has to make this call; the
  const uint32_t change_id = 7;
  child_binding->client()->tracker()->changes()->clear();
  static_cast<mojom::WindowTree*>(child_tree)
      ->PerformWindowMove(
          change_id,
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          mojom::MoveLoopSource::MOUSE, gfx::Point(0, 0));

  // There should be three changes, the first two relating to capture changing,
  // the last for the completion.
  std::vector<Change>* child_changes =
      child_binding->client()->tracker()->changes();
  ASSERT_EQ(3u, child_changes->size());
  EXPECT_EQ(CHANGE_TYPE_CAPTURE_CHANGED, (*child_changes)[0].type);
  EXPECT_EQ(CHANGE_TYPE_CAPTURE_CHANGED, (*child_changes)[1].type);
  child_changes->erase(child_changes->begin(), child_changes->begin() + 2);
  EXPECT_EQ("ChangeCompleted id=7 sucess=true",
            SingleChangeToDescription(*child_changes));
}

TEST_F(WindowTreeTest, WindowManagerCantMoveLoop) {
  TestWindowManager wm_internal;
  set_window_manager_internal(wm_tree(), &wm_internal);

  TestWindowTreeBinding* child_binding;
  WindowTree* child_tree = CreateNewTree(&child_binding);
  child_binding->client()->tracker()->changes()->clear();
  child_binding->client()->set_record_on_change_completed(true);

  // Create a new top level window.
  base::flat_map<std::string, std::vector<uint8_t>> properties;
  const uint32_t initial_change_id = 17;
  // Explicitly use an id that does not contain the client id.
  const ClientWindowId embed_window_id2_in_child(child_tree->id(), 27);
  static_cast<mojom::WindowTree*>(child_tree)
      ->NewTopLevelWindow(
          initial_change_id,
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          properties);

  // The binding should be paused until the wm acks the change.
  uint32_t wm_change_id = 0u;
  ASSERT_TRUE(wm_internal.did_call_create_top_level_window(&wm_change_id));
  EXPECT_TRUE(child_binding->is_paused());

  // Create the window for |embed_window_id2_in_child|.
  const ClientWindowId embed_window_id2 = BuildClientWindowId(wm_tree(), 2);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id2, ServerWindow::Properties()));
  EXPECT_TRUE(wm_tree()->SetWindowVisibility(embed_window_id2, true));
  EXPECT_TRUE(wm_tree()->AddWindow(FirstRootId(wm_tree()), embed_window_id2));

  // Ack the change, which should resume the binding.
  child_binding->client()->tracker()->changes()->clear();
  static_cast<mojom::WindowManagerClient*>(wm_tree())
      ->OnWmCreatedTopLevelWindow(
          wm_change_id,
          wm_tree()->ClientWindowIdToTransportId(embed_window_id2));
  EXPECT_FALSE(child_binding->is_paused());

  // Making this call from the wm_tree() must be invalid.
  const uint32_t change_id = 7;
  static_cast<mojom::WindowTree*>(wm_tree())->PerformWindowMove(
      change_id, wm_tree()->ClientWindowIdToTransportId(embed_window_id2),
      mojom::MoveLoopSource::MOUSE, gfx::Point(0, 0));

  EXPECT_FALSE(wm_internal.on_perform_move_loop_called());
}

TEST_F(WindowTreeTest, RevertWindowBoundsOnMoveLoopFailure) {
  TestWindowManager wm_internal;
  set_window_manager_internal(wm_tree(), &wm_internal);

  TestWindowTreeBinding* child_binding;
  WindowTree* child_tree = CreateNewTree(&child_binding);
  child_binding->client()->tracker()->changes()->clear();
  child_binding->client()->set_record_on_change_completed(true);

  // Create a new top level window.
  base::flat_map<std::string, std::vector<uint8_t>> properties;
  const uint32_t initial_change_id = 17;
  // Explicitly use an id that does not contain the client id.
  const ClientWindowId embed_window_id2_in_child(child_tree->id(), 27);
  static_cast<mojom::WindowTree*>(child_tree)
      ->NewTopLevelWindow(
          initial_change_id,
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          properties);

  // The binding should be paused until the wm acks the change.
  uint32_t wm_change_id = 0u;
  ASSERT_TRUE(wm_internal.did_call_create_top_level_window(&wm_change_id));
  EXPECT_TRUE(child_binding->is_paused());

  // Create the window for |embed_window_id2_in_child|.
  const ClientWindowId embed_window_id2 = BuildClientWindowId(wm_tree(), 2);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id2, ServerWindow::Properties()));
  EXPECT_TRUE(wm_tree()->SetWindowVisibility(embed_window_id2, true));
  EXPECT_TRUE(wm_tree()->AddWindow(FirstRootId(wm_tree()), embed_window_id2));

  // Ack the change, which should resume the binding.
  child_binding->client()->tracker()->changes()->clear();
  static_cast<mojom::WindowManagerClient*>(wm_tree())
      ->OnWmCreatedTopLevelWindow(
          wm_change_id,
          wm_tree()->ClientWindowIdToTransportId(embed_window_id2));
  EXPECT_FALSE(child_binding->is_paused());

  // The child_tree is the one that has to make this call; the
  const uint32_t change_id = 7;
  static_cast<mojom::WindowTree*>(child_tree)
      ->PerformWindowMove(
          change_id,
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          mojom::MoveLoopSource::MOUSE, gfx::Point(0, 0));

  ServerWindow* server_window =
      wm_tree()->GetWindowByClientId(embed_window_id2);
  gfx::Rect old_bounds = server_window->bounds();
  server_window->SetBounds(gfx::Rect(10, 10, 20, 20));

  // Cancel the move loop.
  const uint32_t kFirstWMChange = 1;
  static_cast<mojom::WindowManagerClient*>(wm_tree())->WmResponse(
      kFirstWMChange, false);

  // Canceling the move loop should have reverted the bounds.
  EXPECT_EQ(old_bounds, server_window->bounds());
}

TEST_F(WindowTreeTest, InvalidMoveLoopStillAcksAttempt) {
  // We send a PerformWindowMove for an invalid window. We expect to receive a
  // non-success OnMoveLoopCompleted() event.
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  embed_client->set_record_on_change_completed(true);

  const uint32_t kChangeId = 8;
  const Id kInvalidWindowId = 1234567890;
  static_cast<mojom::WindowTree*>(tree)->PerformWindowMove(
      kChangeId, kInvalidWindowId, mojom::MoveLoopSource::MOUSE,
      gfx::Point(0, 0));

  EXPECT_EQ("ChangeCompleted id=8 sucess=false",
            SingleChangeToDescription(*embed_client->tracker()->changes()));
}

TEST_F(WindowTreeTest, SetCanAcceptEvents) {
  TestWindowTreeClient* embed_client = nullptr;
  WindowTree* tree = nullptr;
  ServerWindow* window = nullptr;
  EXPECT_NO_FATAL_FAILURE(SetupEventTargeting(&embed_client, &tree, &window));

  EXPECT_EQ(mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS,
            window->event_targeting_policy());
  WindowTreeTestApi(tree).SetEventTargetingPolicy(
      tree->ClientWindowIdToTransportId(ClientWindowIdForWindow(tree, window)),
      mojom::EventTargetingPolicy::NONE);
  EXPECT_EQ(mojom::EventTargetingPolicy::NONE,
            window->event_targeting_policy());
}

// Verifies wm observers capture changes in client.
TEST_F(WindowTreeTest, CaptureNotifiesWm) {
  ServerWindow* window = window_event_targeting_helper_.CreatePrimaryTree(
      gfx::Rect(0, 0, 100, 100), gfx::Rect(0, 0, 50, 50));
  TestWindowTreeClient* embed_client = last_window_tree_client();
  WindowTree* owning_tree =
      window_server()->GetTreeWithId(window->owning_tree_id());
  WindowTree* embed_tree = window_server()->GetTreeWithRoot(window);
  ASSERT_NE(owning_tree, embed_tree);

  const ClientWindowId embed_child_window_id =
      BuildClientWindowId(embed_tree, kEmbedTreeWindowId + 1);
  ASSERT_TRUE(
      embed_tree->NewWindow(embed_child_window_id, ServerWindow::Properties()));
  EXPECT_TRUE(embed_tree->SetWindowVisibility(embed_child_window_id, true));
  EXPECT_TRUE(
      embed_tree->AddWindow(FirstRootId(embed_tree), embed_child_window_id));
  wm_client()->tracker()->changes()->clear();
  embed_client->tracker()->changes()->clear();
  EXPECT_TRUE(embed_tree->SetCapture(embed_child_window_id));
  ASSERT_TRUE(!wm_client()->tracker()->changes()->empty());
  EXPECT_EQ("OnCaptureChanged new_window=" + kNextWindowClientIdString + "," +
                std::to_string(embed_child_window_id.sink_id()) +
                " old_window=null",
            ChangesToDescription1(*wm_client()->tracker()->changes())[0]);
  EXPECT_TRUE(embed_client->tracker()->changes()->empty());

  // Set capture to embed window, and ensure notified as well.
  wm_client()->tracker()->changes()->clear();
  EXPECT_TRUE(embed_tree->SetCapture(FirstRootId(embed_tree)));
  ASSERT_TRUE(!wm_client()->tracker()->changes()->empty());
  // clients that created this window is receiving the event, so client_id part
  // would be reset to 0 before sending back to clients.
  EXPECT_EQ("OnCaptureChanged new_window=0," +
                std::to_string(kEmbedTreeWindowId) +
                " old_window=" + kNextWindowClientIdString + "," +
                std::to_string(embed_child_window_id.sink_id()),
            ChangesToDescription1(*wm_client()->tracker()->changes())[0]);
  EXPECT_TRUE(embed_client->tracker()->changes()->empty());
  wm_client()->tracker()->changes()->clear();

  // Set capture from server and ensure embedded tree notified.
  EXPECT_TRUE(owning_tree->ReleaseCapture(
      ClientWindowIdForWindow(owning_tree, FirstRoot(embed_tree))));
  EXPECT_TRUE(wm_client()->tracker()->changes()->empty());
  ASSERT_TRUE(!embed_client->tracker()->changes()->empty());
  EXPECT_EQ("OnCaptureChanged new_window=null old_window=" +
                kWindowManagerClientIdString + ",1",
            ChangesToDescription1(*embed_client->tracker()->changes())[0]);
}

TEST_F(WindowTreeTest, SetModalTypeForwardedToWindowManager) {
  TestWindowManager wm_internal;
  set_window_manager_internal(wm_tree(), &wm_internal);

  TestWindowTreeBinding* child_binding = nullptr;
  WindowTree* child_tree = CreateNewTree(&child_binding);

  // Create a new top level window.
  base::flat_map<std::string, std::vector<uint8_t>> properties;
  const uint32_t initial_change_id = 17;
  // Explicitly use an id that does not contain the client id.
  const ClientWindowId embed_window_id2_in_child(child_tree->id(), 27);
  static_cast<mojom::WindowTree*>(child_tree)
      ->NewTopLevelWindow(
          initial_change_id,
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          properties);

  // Create the window for |embed_window_id2_in_child|.
  const ClientWindowId embed_window_id2 = BuildClientWindowId(wm_tree(), 2);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id2, ServerWindow::Properties()));
  EXPECT_TRUE(wm_tree()->SetWindowVisibility(embed_window_id2, true));
  EXPECT_TRUE(wm_tree()->AddWindow(FirstRootId(wm_tree()), embed_window_id2));

  // Ack the change, which should resume the binding.
  static_cast<mojom::WindowManagerClient*>(wm_tree())
      ->OnWmCreatedTopLevelWindow(
          0u, wm_tree()->ClientWindowIdToTransportId(embed_window_id2));

  // Change modal type to MODAL_TYPE_SYSTEM and check that it is forwarded to
  // the window manager.
  child_tree->SetModalType(embed_window_id2_in_child, MODAL_TYPE_SYSTEM);
  EXPECT_TRUE(wm_internal.on_set_modal_type_called());
}

TEST_F(WindowTreeTest, TestWindowManagerSettingCursorLocation) {
  const ClientWindowId embed_window_id =
      BuildClientWindowId(wm_tree(), kEmbedTreeWindowId);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id, ServerWindow::Properties()));
  ServerWindow* embed_window = wm_tree()->GetWindowByClientId(embed_window_id);
  ASSERT_TRUE(embed_window);
  EXPECT_TRUE(wm_tree()->SetWindowVisibility(embed_window_id, true));
  ASSERT_TRUE(FirstRoot(wm_tree()));
  const ClientWindowId wm_root_id = FirstRootId(wm_tree());
  EXPECT_TRUE(wm_tree()->AddWindow(wm_root_id, embed_window_id));
  ServerWindow* wm_root = FirstRoot(wm_tree());
  ASSERT_TRUE(wm_root);
  wm_root->SetBounds(gfx::Rect(0, 0, 100, 100));
  // This tests expects |wm_root| to be a possible target.
  wm_root->set_event_targeting_policy(
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS);
  display()->root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  mojom::WindowTreeClientPtr client;
  wm_client()->Bind(mojo::MakeRequest(&client));
  const uint32_t embed_flags = 0;
  wm_tree()->Embed(embed_window_id, std::move(client), embed_flags);
  WindowTree* tree1 = window_server()->GetTreeWithRoot(embed_window);
  ASSERT_TRUE(tree1 != nullptr);
  ASSERT_NE(tree1, wm_tree());

  embed_window->SetBounds(gfx::Rect(20, 20, 20, 20));
  embed_window->SetCursor(ui::CursorData(ui::CursorType::kIBeam));

  // Because the cursor is still at the origin, changing the cursor shouldn't
  // have switched to ibeam.
  EXPECT_EQ(ui::CursorType::kPointer, cursor_type());

  // Have the window manager move the cursor within the embed window.
  static_cast<mojom::WindowManagerClient*>(wm_tree())
      ->WmMoveCursorToDisplayLocation(gfx::Point(21, 21), -1);
}

TEST_F(WindowTreeTest, TestWindowManagerConfineCursor) {
  const gfx::Rect bounds(10, 10, 100, 100);
  const int64_t display_id = display()->GetId();
  static_cast<mojom::WindowManagerClient*>(wm_tree())->WmConfineCursorToBounds(
      bounds, display_id);

  PlatformDisplay* platform_display = display()->platform_display();
  EXPECT_EQ(bounds, static_cast<TestPlatformDisplay*>(platform_display)
                        ->confine_cursor_bounds());
}

using WindowTreeShutdownTest = testing::Test;

// Makes sure WindowTreeClient doesn't get any messages during shutdown.
TEST_F(WindowTreeShutdownTest, DontSendMessagesDuringShutdown) {
  std::unique_ptr<TestWindowTreeClient> client;
  {
    // Create a tree with one window.
    WindowServerTestHelper ws_test_helper;
    WindowServer* window_server = ws_test_helper.window_server();
    TestScreenManager screen_manager;
    screen_manager.Init(window_server->display_manager());
    screen_manager.AddDisplay();

    AddWindowManager(window_server);
    TestWindowTreeBinding* test_binding =
        ws_test_helper.window_server_delegate()->last_binding();
    ASSERT_TRUE(test_binding);
    WindowTree* tree = test_binding->tree();
    const ClientWindowId window_id = BuildClientWindowId(tree, 2);
    ASSERT_TRUE(tree->NewWindow(window_id, ServerWindow::Properties()));

    // Release the client so that it survices shutdown.
    client = test_binding->ReleaseClient();
    client->tracker()->changes()->clear();
  }

  // Client should not have got any messages after shutdown.
  EXPECT_TRUE(client->tracker()->changes()->empty());
}

// Used to test the window manager configured to manually create displays roots.
class WindowTreeManualDisplayTest : public TaskRunnerTestBase {
 public:
  WindowTreeManualDisplayTest() {}
  ~WindowTreeManualDisplayTest() override {}

  WindowServer* window_server() { return ws_test_helper_.window_server(); }
  DisplayManager* display_manager() {
    return window_server()->display_manager();
  }
  TestWindowServerDelegate* window_server_delegate() {
    return ws_test_helper_.window_server_delegate();
  }
  TestScreenManager& screen_manager() { return screen_manager_; }

 protected:
  // testing::Test:
  void SetUp() override {
    TaskRunnerTestBase::SetUp();
    screen_manager_.Init(window_server()->display_manager());
  }

 private:
  WindowServerTestHelper ws_test_helper_;
  TestScreenManager screen_manager_;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeManualDisplayTest);
};

TEST_F(WindowTreeManualDisplayTest, ClientCreatesDisplayRoot) {
  const bool automatically_create_display_roots = false;
  AddWindowManager(window_server(), automatically_create_display_roots);

  WindowManagerState* window_manager_state =
      window_server()->GetWindowManagerState();
  ASSERT_TRUE(window_manager_state);
  WindowTree* window_manager_tree = window_manager_state->window_tree();
  EXPECT_TRUE(window_manager_tree->roots().empty());
  TestWindowManager* test_window_manager =
      window_server_delegate()->last_binding()->window_manager();
  EXPECT_EQ(1, test_window_manager->connect_count());
  EXPECT_EQ(0, test_window_manager->display_added_count());

  // Create a window for the windowmanager and set it as the root.
  ClientWindowId display_root_id = BuildClientWindowId(window_manager_tree, 10);
  ASSERT_TRUE(window_manager_tree->NewWindow(display_root_id,
                                             ServerWindow::Properties()));
  ServerWindow* display_root =
      window_manager_tree->GetWindowByClientId(display_root_id);
  ASSERT_TRUE(display_root);
  display::Display display1 = MakeDisplay(0, 0, 1024, 768, 1.0f);
  display1.set_id(101);

  display::ViewportMetrics metrics;
  metrics.bounds_in_pixels = display1.bounds();
  metrics.device_scale_factor = 1.5;
  metrics.ui_scale_factor = 2.5;
  const bool is_primary_display = true;
  ASSERT_TRUE(WindowTreeTestApi(window_manager_tree)
                  .ProcessSetDisplayRoot(display1, metrics, is_primary_display,
                                         display_root_id));
  EXPECT_TRUE(display_root->parent());
  EXPECT_TRUE(window_server_delegate()
                  ->last_binding()
                  ->client()
                  ->tracker()
                  ->changes()
                  ->empty());
  EXPECT_EQ(1u, window_manager_tree->roots().size());

  // Delete the root, which should delete the WindowManagerDisplayRoot.
  EXPECT_TRUE(window_manager_tree->DeleteWindow(display_root_id));
  EXPECT_TRUE(window_manager_tree->roots().empty());
  EXPECT_TRUE(WindowManagerStateTestApi(window_manager_state)
                  .window_manager_display_roots()
                  .empty());
}

TEST_F(WindowTreeManualDisplayTest, MoveDisplayRootToNewDisplay) {
  const bool automatically_create_display_roots = false;
  AddWindowManager(window_server(), automatically_create_display_roots);

  WindowManagerState* window_manager_state =
      window_server()->GetWindowManagerState();
  ASSERT_TRUE(window_manager_state);
  WindowTree* window_manager_tree = window_manager_state->window_tree();
  EXPECT_TRUE(window_manager_tree->roots().empty());
  TestWindowManager* test_window_manager =
      window_server_delegate()->last_binding()->window_manager();
  EXPECT_EQ(1, test_window_manager->connect_count());
  EXPECT_EQ(0, test_window_manager->display_added_count());

  // Create a window for the windowmanager and set it as the root.
  ClientWindowId display_root_id = BuildClientWindowId(window_manager_tree, 10);
  ASSERT_TRUE(window_manager_tree->NewWindow(display_root_id,
                                             ServerWindow::Properties()));
  ServerWindow* display_root =
      window_manager_tree->GetWindowByClientId(display_root_id);
  ASSERT_TRUE(display_root);
  display::Display display1 = MakeDisplay(0, 0, 1024, 768, 1.0f);
  constexpr int64_t display1_id = 101;
  display1.set_id(display1_id);

  display::ViewportMetrics metrics;
  metrics.bounds_in_pixels = display1.bounds();
  metrics.device_scale_factor = 1.5;
  metrics.ui_scale_factor = 2.5;
  const bool is_primary_display = true;
  ASSERT_TRUE(WindowTreeTestApi(window_manager_tree)
                  .ProcessSetDisplayRoot(display1, metrics, is_primary_display,
                                         display_root_id));
  ASSERT_TRUE(display_root->parent());
  const viz::FrameSinkId display1_parent_id =
      display_root->parent()->frame_sink_id();
  EXPECT_TRUE(window_server_delegate()
                  ->last_binding()
                  ->client()
                  ->tracker()
                  ->changes()
                  ->empty());
  EXPECT_EQ(1u, window_manager_tree->roots().size());

  // Call ProcessSetDisplayRoot() again, with a different display.
  display::Display display2 = MakeDisplay(0, 0, 1024, 768, 1.0f);
  constexpr int64_t display2_id = 102;
  display2.set_id(display2_id);
  ASSERT_TRUE(WindowTreeTestApi(window_manager_tree)
                  .ProcessSetDisplayRoot(display2, metrics, is_primary_display,
                                         display_root_id));
  ASSERT_TRUE(display_root->parent());
  EXPECT_NE(display1_parent_id, display_root->parent()->frame_sink_id());
  EXPECT_TRUE(window_server_delegate()
                  ->last_binding()
                  ->client()
                  ->tracker()
                  ->changes()
                  ->empty());
  EXPECT_EQ(1u, window_manager_tree->roots().size());
  // The WindowManagerDisplayRoot for |display1| should have been deleted.
  EXPECT_EQ(1u, WindowManagerStateTestApi(window_manager_state)
                    .window_manager_display_roots()
                    .size());
  EXPECT_FALSE(window_server()->display_manager()->GetDisplayById(display1_id));
  EXPECT_TRUE(window_server()->display_manager()->GetDisplayById(display2_id));

  // Delete the root, which should delete the WindowManagerDisplayRoot.
  EXPECT_TRUE(window_manager_tree->DeleteWindow(display_root_id));
  EXPECT_TRUE(window_manager_tree->roots().empty());
  EXPECT_TRUE(WindowManagerStateTestApi(window_manager_state)
                  .window_manager_display_roots()
                  .empty());
}

TEST_F(WindowTreeManualDisplayTest,
       DisplayManagerObserverNotifiedWithManualRoots) {
  const bool automatically_create_display_roots = false;
  AddWindowManager(window_server(), automatically_create_display_roots);

  TestScreenProviderObserver screen_provider_observer;
  DisplayManager* display_manager = window_server()->display_manager();
  UserDisplayManager* user_display_manager =
      display_manager->GetUserDisplayManager();
  ASSERT_TRUE(user_display_manager);
  user_display_manager->AddObserver(screen_provider_observer.GetPtr());

  // Observer should not have been notified yet.
  //
  // NOTE: the RunUntilIdle() calls are necessary anytime the calls are checked
  // as the observer is called via mojo, which is async.
  RunUntilIdle();
  EXPECT_TRUE(screen_provider_observer.GetAndClearObserverCalls().empty());

  // Set frame decorations, again observer should not be notified.
  WindowManagerState* window_manager_state =
      window_server()->GetWindowManagerState();
  ASSERT_TRUE(window_manager_state);
  WindowTree* window_manager_tree = window_manager_state->window_tree();
  window_manager_state->SetFrameDecorationValues(
      mojom::FrameDecorationValues::New());
  RunUntilIdle();
  EXPECT_TRUE(screen_provider_observer.GetAndClearObserverCalls().empty());

  // Create a window for the windowmanager and set it as the root.
  ClientWindowId display_root_id = BuildClientWindowId(window_manager_tree, 10);
  ASSERT_TRUE(window_manager_tree->NewWindow(display_root_id,
                                             ServerWindow::Properties()));
  ServerWindow* display_root =
      window_manager_tree->GetWindowByClientId(display_root_id);
  ASSERT_TRUE(display_root);
  RunUntilIdle();
  EXPECT_TRUE(screen_provider_observer.GetAndClearObserverCalls().empty());

  // Add a new display.
  // The value for the scale factor doesn't matter, just choosing something
  // other than 1 to ensure values other than 1 correctly take.
  const float kDisplay1ScaleFactor = 2.25;
  display::Display display1 =
      MakeDisplay(0, 0, 1024, 768, kDisplay1ScaleFactor);
  const int64_t display_id1 = 101;
  display1.set_id(display_id1);
  display::ViewportMetrics metrics1;
  metrics1.bounds_in_pixels = display1.bounds();
  metrics1.device_scale_factor = kDisplay1ScaleFactor;
  metrics1.ui_scale_factor = 2.5;
  const bool is_primary_display = true;
  ASSERT_TRUE(WindowTreeTestApi(window_manager_tree)
                  .ProcessSetDisplayRoot(display1, metrics1, is_primary_display,
                                         display_root_id));
  RunUntilIdle();
  EXPECT_TRUE(screen_provider_observer.GetAndClearObserverCalls().empty());

  // Configure the displays, updating the bounds of the first display.
  std::vector<display::Display> displays;
  displays.push_back(display1);
  std::vector<display::ViewportMetrics> viewport_metrics;
  viewport_metrics.push_back(metrics1);
  const gfx::Rect updated_bounds(1, 2, 3, 4);
  viewport_metrics[0].bounds_in_pixels = updated_bounds;
  std::vector<display::Display> mirrors;
  ASSERT_TRUE(display_manager->SetDisplayConfiguration(
      displays, viewport_metrics, display_id1, display::kInvalidDisplayId,
      mirrors));
  RunUntilIdle();
  EXPECT_EQ("OnDisplaysChanged " + std::to_string(display_id1) + " " +
                std::to_string(display::kInvalidDisplayId),
            screen_provider_observer.GetAndClearObserverCalls());
  PlatformDisplay* platform_display1 =
      display_manager->GetDisplayById(display_id1)->platform_display();
  ASSERT_TRUE(platform_display1);
  EXPECT_EQ(
      kDisplay1ScaleFactor * ui::mojom::kCursorMultiplierForExternalDisplays,
      static_cast<TestPlatformDisplay*>(platform_display1)->cursor_scale());
  EXPECT_EQ(updated_bounds, static_cast<TestPlatformDisplay*>(platform_display1)
                                ->metrics()
                                .bounds_in_pixels);

  // Create a window for the windowmanager and set it as the root.
  ClientWindowId display_root_id2 =
      BuildClientWindowId(window_manager_tree, 11);
  ASSERT_TRUE(window_manager_tree->NewWindow(display_root_id2,
                                             ServerWindow::Properties()));
  ServerWindow* display_root2 =
      window_manager_tree->GetWindowByClientId(display_root_id);
  ASSERT_TRUE(display_root2);
  RunUntilIdle();
  EXPECT_TRUE(screen_provider_observer.GetAndClearObserverCalls().empty());

  // Add another display.
  const float kDisplay2ScaleFactor = 1.75;
  display::Display display2 =
      MakeDisplay(0, 0, 1024, 768, kDisplay2ScaleFactor);
  const int64_t display_id2 = 102;
  display2.set_id(display_id2);
  display::ViewportMetrics metrics2;
  metrics2.bounds_in_pixels = display2.bounds();
  metrics2.device_scale_factor = kDisplay2ScaleFactor;
  metrics2.ui_scale_factor = 2.5;
  ASSERT_TRUE(
      WindowTreeTestApi(window_manager_tree)
          .ProcessSetDisplayRoot(display2, metrics2, false, display_root_id2));
  RunUntilIdle();
  EXPECT_TRUE(screen_provider_observer.GetAndClearObserverCalls().empty());

  // Make |display2| the default, and resize both displays.
  display1.set_bounds(gfx::Rect(0, 0, 1024, 1280));
  metrics1.bounds_in_pixels = display1.bounds();
  displays.clear();
  displays.push_back(display1);

  display2.set_bounds(gfx::Rect(0, 0, 500, 600));
  metrics2.bounds_in_pixels = display2.bounds();
  displays.push_back(display2);

  viewport_metrics.clear();
  viewport_metrics.push_back(metrics1);
  viewport_metrics.push_back(metrics2);
  ASSERT_TRUE(display_manager->SetDisplayConfiguration(
      displays, viewport_metrics, display_id2, display_id2, mirrors));
  RunUntilIdle();
  EXPECT_EQ("OnDisplaysChanged " + std::to_string(display_id1) + " " +
                std::to_string(display_id2) + " " + std::to_string(display_id2),
            screen_provider_observer.GetAndClearObserverCalls());
  EXPECT_EQ(
      kDisplay1ScaleFactor * ui::mojom::kCursorMultiplierForExternalDisplays,
      static_cast<TestPlatformDisplay*>(platform_display1)->cursor_scale());
  PlatformDisplay* platform_display2 =
      display_manager->GetDisplayById(display_id2)->platform_display();
  ASSERT_TRUE(platform_display2);
  EXPECT_EQ(
      kDisplay2ScaleFactor,
      static_cast<TestPlatformDisplay*>(platform_display2)->cursor_scale());

  // Delete the second display, no notification should be sent.
  EXPECT_TRUE(window_manager_tree->DeleteWindow(display_root_id2));
  RunUntilIdle();
  EXPECT_TRUE(screen_provider_observer.GetAndClearObserverCalls().empty());
  EXPECT_FALSE(display_manager->GetDisplayById(display_id2));

  // Set the config back to only the first.
  displays.clear();
  displays.push_back(display1);

  viewport_metrics.clear();
  viewport_metrics.push_back(metrics1);
  ASSERT_TRUE(display_manager->SetDisplayConfiguration(
      displays, viewport_metrics, display_id1, display_id1, mirrors));
  RunUntilIdle();
  EXPECT_EQ("OnDisplaysChanged " + std::to_string(display_id1) + " " +
                std::to_string(display_id1),
            screen_provider_observer.GetAndClearObserverCalls());

  // The display list should not have display2.
  display::DisplayList& display_list =
      display::ScreenManager::GetInstance()->GetScreen()->display_list();
  EXPECT_TRUE(display_list.FindDisplayById(display_id2) ==
              display_list.displays().end());
  ASSERT_TRUE(display_list.GetPrimaryDisplayIterator() !=
              display_list.displays().end());
  EXPECT_EQ(display_id1, display_list.GetPrimaryDisplayIterator()->id());
}

TEST_F(WindowTreeManualDisplayTest, SwapDisplayRoots) {
  const bool automatically_create_display_roots = false;
  AddWindowManager(window_server(), automatically_create_display_roots);

  WindowManagerState* window_manager_state =
      window_server()->GetWindowManagerState();
  ASSERT_TRUE(window_manager_state);
  WindowTree* window_manager_tree = window_manager_state->window_tree();
  window_manager_state->SetFrameDecorationValues(
      mojom::FrameDecorationValues::New());

  // Add two windows for the two displays.
  ClientWindowId display_root_id1 =
      BuildClientWindowId(window_manager_tree, 10);
  ASSERT_TRUE(window_manager_tree->NewWindow(display_root_id1,
                                             ServerWindow::Properties()));
  ServerWindow* display_root1 =
      window_manager_tree->GetWindowByClientId(display_root_id1);
  ASSERT_TRUE(display_root1);

  ClientWindowId display_root_id2 =
      BuildClientWindowId(window_manager_tree, 20);
  ASSERT_TRUE(window_manager_tree->NewWindow(display_root_id2,
                                             ServerWindow::Properties()));
  ServerWindow* display_root2 =
      window_manager_tree->GetWindowByClientId(display_root_id2);
  ASSERT_TRUE(display_root2);
  EXPECT_NE(display_root1, display_root2);

  // Add two displays.
  const int64_t display_id1 = 101;
  display::Display display1 = MakeDisplay(0, 0, 1024, 768, 1.0f);
  display1.set_id(display_id1);
  display::ViewportMetrics metrics;
  metrics.bounds_in_pixels = display1.bounds();
  metrics.device_scale_factor = 1.5;
  metrics.ui_scale_factor = 2.5;
  const bool is_primary_display = true;
  ASSERT_TRUE(WindowTreeTestApi(window_manager_tree)
                  .ProcessSetDisplayRoot(display1, metrics, is_primary_display,
                                         display_root_id1));

  display::Display display2 = MakeDisplay(0, 0, 1024, 768, 1.0f);
  const int64_t display_id2 = 102;
  display2.set_id(display_id2);
  ASSERT_TRUE(WindowTreeTestApi(window_manager_tree)
                  .ProcessSetDisplayRoot(display2, metrics, is_primary_display,
                                         display_root_id2));

  ServerWindow* display_root1_parent = display_root1->parent();
  ServerWindow* display_root2_parent = display_root2->parent();
  ASSERT_TRUE(WindowTreeTestApi(window_manager_tree)
                  .ProcessSwapDisplayRoots(display_id1, display_id2));
  EXPECT_EQ(display_root1_parent, display_root2->parent());
  EXPECT_EQ(display_root2_parent, display_root1->parent());
}

TEST_F(WindowTreeTest, EmbedFlagEmbedderControlsVisibility) {
  const ClientWindowId embed_window_id =
      BuildClientWindowId(wm_tree(), kEmbedTreeWindowId);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id, ServerWindow::Properties()));
  ServerWindow* embed_window = wm_tree()->GetWindowByClientId(embed_window_id);
  ASSERT_TRUE(embed_window);
  EXPECT_TRUE(wm_tree()->AddWindow(FirstRootId(wm_tree()), embed_window_id));
  mojom::WindowTreeClientPtr client;
  wm_client()->Bind(mojo::MakeRequest(&client));
  const uint32_t embed_flags = mojom::kEmbedFlagEmbedderControlsVisibility;
  wm_tree()->Embed(embed_window_id, std::move(client), embed_flags);
  WindowTree* tree1 = window_server()->GetTreeWithRoot(embed_window);
  ASSERT_TRUE(tree1);
  // |tree1| should not be able to control the visibility of its root because
  // |kEmbedFlagEmbedderControlsVisibility| was specified.
  EXPECT_FALSE(tree1->SetWindowVisibility(
      ClientWindowIdForWindow(tree1, embed_window), false));
  const ClientWindowId child_window_id = BuildClientWindowId(tree1, 101);
  // But |tree1| can control the visibility of any windows it creates.
  EXPECT_TRUE(tree1->NewWindow(child_window_id, ServerWindow::Properties()));
  EXPECT_TRUE(tree1->SetWindowVisibility(child_window_id, true));
}

TEST_F(WindowTreeTest, PerformWmAction) {
  TestWindowManager wm_internal;
  set_window_manager_internal(wm_tree(), &wm_internal);

  TestWindowTreeBinding* child_binding = nullptr;
  WindowTree* child_tree = CreateNewTree(&child_binding);

  // Create a new top level window.
  base::flat_map<std::string, std::vector<uint8_t>> properties;
  const uint32_t initial_change_id = 17;
  // Explicitly use an id that does not contain the client id.
  const ClientWindowId embed_window_id2_in_child(child_tree->id(), 27);
  static_cast<mojom::WindowTree*>(child_tree)
      ->NewTopLevelWindow(
          initial_change_id,
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          properties);

  // Create the window for |embed_window_id2_in_child|.
  const ClientWindowId embed_window_id2 = BuildClientWindowId(wm_tree(), 2);
  EXPECT_TRUE(
      wm_tree()->NewWindow(embed_window_id2, ServerWindow::Properties()));
  EXPECT_TRUE(wm_tree()->SetWindowVisibility(embed_window_id2, true));
  EXPECT_TRUE(wm_tree()->AddWindow(FirstRootId(wm_tree()), embed_window_id2));

  // Ack the change, which should resume the binding.
  static_cast<mojom::WindowManagerClient*>(wm_tree())
      ->OnWmCreatedTopLevelWindow(
          0u, wm_tree()->ClientWindowIdToTransportId(embed_window_id2));

  static_cast<mojom::WindowTree*>(child_tree)
      ->PerformWmAction(
          child_tree->ClientWindowIdToTransportId(embed_window_id2_in_child),
          "test-action");
  EXPECT_EQ("test-action", wm_internal.last_wm_action());
}

TEST_F(WindowTreeTest, EmbedderInterceptsEventsSeesWindowsInEmbeddedClients) {
  // Make the root visible and give it bounds.
  ServerWindow* wm_root = FirstRoot(wm_tree());
  ASSERT_TRUE(wm_root);
  const gfx::Rect bounds(0, 0, 20, 20);
  wm_root->SetBounds(bounds);
  wm_root->SetVisible(true);

  // Create window for embedded (|w1|).
  ClientWindowId w1_id;
  ServerWindow* w1 =
      NewWindowInTreeWithParent(wm_tree(), wm_root, &w1_id, bounds);
  ASSERT_TRUE(w1);

  // Embed a new client in |w1|.
  TestWindowTreeBinding* embed_binding1 =
      test_window_server_delegate()->Embed(wm_tree(), w1);
  // Set the |is_for_embedding| to false, otherwise
  // kEmbedFlagEmbedderInterceptsEvents is ignored
  WindowTreeTestApi(embed_binding1->tree()).set_is_for_embedding(false);
  ASSERT_TRUE(embed_binding1);

  // Create |w2| (in the embedded tree).
  ClientWindowId w2_id;
  ServerWindow* w2 =
      NewWindowInTreeWithParent(embed_binding1->tree(), w1, &w2_id, bounds);
  ASSERT_TRUE(w2);

  // Embed a new client in |w2|.
  TestWindowTreeBinding* embed_binding2 = test_window_server_delegate()->Embed(
      embed_binding1->tree(), w2, mojom::kEmbedFlagEmbedderInterceptsEvents);
  ASSERT_TRUE(embed_binding2);

  // Create |w3| as a child of |w2|.
  ClientWindowId w3_id;
  ServerWindow* w3 =
      NewWindowInTreeWithParent(embed_binding2->tree(), w2, &w3_id, bounds);
  ASSERT_TRUE(w3);

  // Embed a new client in |w3|.
  TestWindowTreeBinding* embed_binding3 =
      test_window_server_delegate()->Embed(embed_binding2->tree(), w3);
  ASSERT_TRUE(embed_binding3);

  // Create |w4| as a child of |w3|.
  ClientWindowId w4_id;
  ServerWindow* w4 =
      NewWindowInTreeWithParent(embed_binding3->tree(), w3, &w4_id, bounds);
  ASSERT_TRUE(w4);

  // |w4| and |w3| should be known to embed_binding1->tree() because of
  // kEmbedFlagEmbedderInterceptsEvents. |w3| should not be known to
  // embed_binding2->tree(), because it has an invalid user id.
  EXPECT_TRUE(embed_binding1->tree()->IsWindowKnown(w3, nullptr));
  ClientWindowId w4_in_tree1_id;
  EXPECT_TRUE(embed_binding1->tree()->IsWindowKnown(w4, &w4_in_tree1_id));
  EXPECT_FALSE(embed_binding2->tree()->IsWindowKnown(w4, nullptr));

  // Verify an event targetting |w4| goes to embed_binding1->tree().
  embed_binding1->client()->tracker()->changes()->clear();
  AckPreviousEvent();
  DispatchEventWithoutAck(CreatePointerDownEvent(5, 5));
  WindowManagerStateTestApi wm_state_test_api(
      wm_tree()->window_manager_state());
  EXPECT_EQ(embed_binding1->tree(),
            wm_state_test_api.tree_awaiting_input_ack());
  // Event targets |w4|, but goes to embed_binding1->tree() (because of
  // kEmbedFlagEmbedderInterceptsEvents).
  EXPECT_EQ(1u, embed_binding1->client()->tracker()->changes()->size());
  EXPECT_EQ("InputEvent window=" + ClientWindowIdToString(w4_in_tree1_id) +
                " event_action=" + std::to_string(ui::ET_POINTER_DOWN),
            SingleChangeToDescription(
                *embed_binding1->client()->tracker()->changes()));
}

}  // namespace test
}  // namespace ws
}  // namespace ui
