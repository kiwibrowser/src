// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_service_client.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <queue>

#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "services/ui/ws2/client_window.h"
#include "services/ui/ws2/client_window_test_helper.h"
#include "services/ui/ws2/event_test_utils.h"
#include "services/ui/ws2/window_service.h"
#include "services/ui/ws2/window_service_client_test_helper.h"
#include "services/ui/ws2/window_service_test_setup.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tracker.h"
#include "ui/events/test/event_generator.h"
#include "ui/wm/core/capture_controller.h"
#include "ui/wm/core/focus_controller.h"

namespace ui {
namespace ws2 {
namespace {

class TestLayoutManager : public aura::LayoutManager {
 public:
  TestLayoutManager() = default;
  ~TestLayoutManager() override = default;

  void set_next_bounds(const gfx::Rect& bounds) { next_bounds_ = bounds; }

  // aura::LayoutManager:
  void OnWindowResized() override {}
  void OnWindowAddedToLayout(aura::Window* child) override {}
  void OnWillRemoveWindowFromLayout(aura::Window* child) override {}
  void OnWindowRemovedFromLayout(aura::Window* child) override {}
  void OnChildWindowVisibilityChanged(aura::Window* child,
                                      bool visible) override {}
  void SetChildBounds(aura::Window* child,
                      const gfx::Rect& requested_bounds) override {
    if (next_bounds_) {
      SetChildBoundsDirect(child, *next_bounds_);
      next_bounds_.reset();
    } else {
      SetChildBoundsDirect(child, requested_bounds);
    }
  }

 private:
  base::Optional<gfx::Rect> next_bounds_;

  DISALLOW_COPY_AND_ASSIGN(TestLayoutManager);
};

TEST(WindowServiceClientTest, NewWindow) {
  WindowServiceTestSetup setup;
  EXPECT_TRUE(setup.changes()->empty());
  aura::Window* window = setup.client_test_helper()->NewWindow(1);
  ASSERT_TRUE(window);
  EXPECT_EQ("ChangeCompleted id=1 sucess=true",
            SingleChangeToDescription(*setup.changes()));
}

TEST(WindowServiceClientTest, NewWindowWithProperties) {
  WindowServiceTestSetup setup;
  EXPECT_TRUE(setup.changes()->empty());
  aura::PropertyConverter::PrimitiveType value = true;
  std::vector<uint8_t> transport = mojo::ConvertTo<std::vector<uint8_t>>(value);
  aura::Window* window = setup.client_test_helper()->NewWindow(
      1, {{ui::mojom::WindowManager::kAlwaysOnTop_Property, transport}});
  ASSERT_TRUE(window);
  EXPECT_EQ("ChangeCompleted id=1 sucess=true",
            SingleChangeToDescription(*setup.changes()));
  EXPECT_TRUE(window->GetProperty(aura::client::kAlwaysOnTopKey));
}

TEST(WindowServiceClientTest, NewTopLevelWindow) {
  WindowServiceTestSetup setup;
  EXPECT_TRUE(setup.changes()->empty());
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  ASSERT_TRUE(top_level);
  EXPECT_EQ("TopLevelCreated id=1 window_id=0,1 drawn=false",
            SingleChangeToDescription(*setup.changes()));
}

TEST(WindowServiceClientTest, NewTopLevelWindowWithProperties) {
  WindowServiceTestSetup setup;
  EXPECT_TRUE(setup.changes()->empty());
  aura::PropertyConverter::PrimitiveType value = true;
  std::vector<uint8_t> transport = mojo::ConvertTo<std::vector<uint8_t>>(value);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(
      1, {{ui::mojom::WindowManager::kAlwaysOnTop_Property, transport}});
  ASSERT_TRUE(top_level);
  EXPECT_EQ("TopLevelCreated id=1 window_id=0,1 drawn=false",
            SingleChangeToDescription(*setup.changes()));
  EXPECT_TRUE(top_level->GetProperty(aura::client::kAlwaysOnTopKey));
}

TEST(WindowServiceClientTest, SetTopLevelWindowBounds) {
  WindowServiceTestSetup setup;
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  setup.changes()->clear();

  const gfx::Rect bounds_from_client = gfx::Rect(1, 2, 300, 400);
  setup.client_test_helper()->SetWindowBoundsWithAck(top_level,
                                                     bounds_from_client, 2);
  EXPECT_EQ(bounds_from_client, top_level->bounds());
  ASSERT_EQ(2u, setup.changes()->size());
  {
    const Change& change = (*setup.changes())[0];
    EXPECT_EQ(CHANGE_TYPE_NODE_BOUNDS_CHANGED, change.type);
    EXPECT_EQ(top_level->bounds(), change.bounds2);
    EXPECT_TRUE(change.local_surface_id);
    setup.changes()->erase(setup.changes()->begin());
  }
  // See comments in WindowServiceClient::SetBoundsImpl() for why this returns
  // false.
  EXPECT_EQ("ChangeCompleted id=2 sucess=false",
            SingleChangeToDescription(*setup.changes()));
  setup.changes()->clear();

  const gfx::Rect bounds_from_server = gfx::Rect(101, 102, 103, 104);
  top_level->SetBounds(bounds_from_server);
  ASSERT_EQ(1u, setup.changes()->size());
  EXPECT_EQ(CHANGE_TYPE_NODE_BOUNDS_CHANGED, (*setup.changes())[0].type);
  EXPECT_EQ(bounds_from_server, (*setup.changes())[0].bounds2);
  setup.changes()->clear();

  // Set a LayoutManager so that when the client requests a bounds change the
  // window is resized to a different bounds.
  // |layout_manager| is owned by top_level->parent();
  TestLayoutManager* layout_manager = new TestLayoutManager();
  const gfx::Rect restricted_bounds = gfx::Rect(401, 405, 406, 407);
  layout_manager->set_next_bounds(restricted_bounds);
  top_level->parent()->SetLayoutManager(layout_manager);
  setup.client_test_helper()->SetWindowBoundsWithAck(top_level,
                                                     bounds_from_client, 3);
  ASSERT_EQ(2u, setup.changes()->size());
  // The layout manager changes the bounds to a different value than the client
  // requested, so the client should get OnWindowBoundsChanged() with
  // |restricted_bounds|.
  EXPECT_EQ(CHANGE_TYPE_NODE_BOUNDS_CHANGED, (*setup.changes())[0].type);
  EXPECT_EQ(restricted_bounds, (*setup.changes())[0].bounds2);

  // And because the layout manager changed the bounds the result is false.
  EXPECT_EQ("ChangeCompleted id=3 sucess=false",
            ChangeToDescription((*setup.changes())[1]));
}

TEST(WindowServiceClientTest, SetTopLevelWindowBoundsFailsForSameSize) {
  WindowServiceTestSetup setup;
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  setup.changes()->clear();
  const gfx::Rect bounds = gfx::Rect(1, 2, 300, 400);
  top_level->SetBounds(bounds);
  setup.changes()->clear();
  // WindowServiceClientTestHelper::SetWindowBounds() uses a null
  // LocalSurfaceId, which differs from the current LocalSurfaceId (assigned by
  // ClientRoot). Because of this, the LocalSurfaceIds differ and the call
  // returns false.
  EXPECT_FALSE(setup.client_test_helper()->SetWindowBounds(top_level, bounds));
  EXPECT_TRUE(setup.changes()->empty());
}

// Tests the ability of the client to change properties on the server.
TEST(WindowServiceClientTest, SetTopLevelWindowProperty) {
  WindowServiceTestSetup setup;
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  setup.changes()->clear();

  EXPECT_FALSE(top_level->GetProperty(aura::client::kAlwaysOnTopKey));
  aura::PropertyConverter::PrimitiveType client_value = true;
  std::vector<uint8_t> client_transport_value =
      mojo::ConvertTo<std::vector<uint8_t>>(client_value);
  setup.client_test_helper()->SetWindowProperty(
      top_level, ui::mojom::WindowManager::kAlwaysOnTop_Property,
      client_transport_value, 2);
  EXPECT_EQ("ChangeCompleted id=2 sucess=true",
            SingleChangeToDescription(*setup.changes()));
  EXPECT_TRUE(top_level->GetProperty(aura::client::kAlwaysOnTopKey));
  setup.changes()->clear();

  top_level->SetProperty(aura::client::kAlwaysOnTopKey, false);
  EXPECT_EQ(
      "PropertyChanged window=0,1 key=prop:always_on_top "
      "value=0000000000000000",
      SingleChangeToDescription(*setup.changes()));
}

TEST(WindowServiceClientTest, WindowToWindowData) {
  WindowServiceTestSetup setup;
  aura::Window* window = setup.client_test_helper()->NewWindow(1);
  setup.changes()->clear();

  window->SetBounds(gfx::Rect(1, 2, 300, 400));
  window->SetProperty(aura::client::kAlwaysOnTopKey, true);
  window->Show();  // Called to make the window visible.
  mojom::WindowDataPtr data =
      setup.client_test_helper()->WindowToWindowData(window);
  EXPECT_EQ(gfx::Rect(1, 2, 300, 400), data->bounds);
  EXPECT_TRUE(data->visible);
  EXPECT_EQ(1u, data->properties.count(
                    ui::mojom::WindowManager::kAlwaysOnTop_Property));
  EXPECT_EQ(
      aura::PropertyConverter::PrimitiveType(true),
      mojo::ConvertTo<aura::PropertyConverter::PrimitiveType>(
          data->properties[ui::mojom::WindowManager::kAlwaysOnTop_Property]));
}

TEST(WindowServiceClientTest, MovePressDragRelease) {
  WindowServiceTestSetup setup;
  TestWindowTreeClient* window_tree_client = setup.window_tree_client();
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  ASSERT_TRUE(top_level);

  top_level->Show();
  top_level->SetBounds(gfx::Rect(10, 10, 100, 100));

  test::EventGenerator event_generator(setup.root());
  event_generator.MoveMouseTo(50, 50);
  EXPECT_EQ("POINTER_MOVED 40,40",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopInputEvent().event.get()));

  event_generator.PressLeftButton();
  EXPECT_EQ("POINTER_DOWN 40,40",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopInputEvent().event.get()));

  event_generator.MoveMouseTo(0, 0);
  EXPECT_EQ("POINTER_MOVED -10,-10",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopInputEvent().event.get()));

  event_generator.ReleaseLeftButton();
  EXPECT_EQ("POINTER_UP -10,-10",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopInputEvent().event.get()));
}

class EventRecordingWindowDelegate : public aura::test::TestWindowDelegate {
 public:
  EventRecordingWindowDelegate() = default;
  ~EventRecordingWindowDelegate() override = default;

  std::queue<std::unique_ptr<ui::Event>>& events() { return events_; }

  std::unique_ptr<Event> PopEvent() {
    if (events_.empty())
      return nullptr;
    auto event = std::move(events_.front());
    events_.pop();
    return event;
  }

  void ClearEvents() {
    std::queue<std::unique_ptr<ui::Event>> events;
    std::swap(events_, events);
  }

  // aura::test::TestWindowDelegate:
  void OnEvent(ui::Event* event) override {
    events_.push(ui::Event::Clone(*event));
  }

 private:
  std::queue<std::unique_ptr<ui::Event>> events_;

  DISALLOW_COPY_AND_ASSIGN(EventRecordingWindowDelegate);
};

TEST(WindowServiceClientTest, MoveFromClientToNonClient) {
  EventRecordingWindowDelegate window_delegate;
  WindowServiceTestSetup setup;
  TestWindowTreeClient* window_tree_client = setup.window_tree_client();
  setup.delegate()->set_delegate_for_next_top_level(&window_delegate);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  ASSERT_TRUE(top_level);

  top_level->Show();
  top_level->SetBounds(gfx::Rect(10, 10, 100, 100));
  setup.client_test_helper()->SetClientArea(top_level,
                                            gfx::Insets(10, 0, 0, 0));

  window_delegate.ClearEvents();

  test::EventGenerator event_generator(setup.root());
  event_generator.MoveMouseTo(50, 50);
  EXPECT_EQ("POINTER_MOVED 40,40",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopInputEvent().event.get()));

  // The delegate should see the same events (but as mouse events).
  EXPECT_EQ("MOUSE_ENTERED 40,40", LocatedEventToEventTypeAndLocation(
                                       window_delegate.PopEvent().get()));
  EXPECT_EQ("MOUSE_MOVED 40,40", LocatedEventToEventTypeAndLocation(
                                     window_delegate.PopEvent().get()));

  // Move the mouse over the non-client area.
  // The event is still sent to the client, and the delegate.
  event_generator.MoveMouseTo(15, 16);
  EXPECT_EQ("POINTER_MOVED 5,6",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopInputEvent().event.get()));

  // Delegate should also get the events.
  EXPECT_EQ("MOUSE_MOVED 5,6", LocatedEventToEventTypeAndLocation(
                                   window_delegate.PopEvent().get()));

  // Only the delegate should get the press in this case.
  event_generator.PressLeftButton();
  ASSERT_FALSE(window_tree_client->PopInputEvent().event.get());

  EXPECT_EQ("MOUSE_PRESSED 5,6", LocatedEventToEventTypeAndLocation(
                                     window_delegate.PopEvent().get()));

  // Move mouse into client area, only the delegate should get the move (drag).
  event_generator.MoveMouseTo(35, 51);
  ASSERT_FALSE(window_tree_client->PopInputEvent().event.get());

  EXPECT_EQ("MOUSE_DRAGGED 25,41", LocatedEventToEventTypeAndLocation(
                                       window_delegate.PopEvent().get()));

  // Release over client area, again only delegate should get it.
  event_generator.ReleaseLeftButton();
  ASSERT_FALSE(window_tree_client->PopInputEvent().event.get());

  EXPECT_EQ("MOUSE_RELEASED",
            EventToEventType(window_delegate.PopEvent().get()));

  event_generator.MoveMouseTo(26, 50);
  EXPECT_EQ("POINTER_MOVED 16,40",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopInputEvent().event.get()));

  // Delegate should also get the events.
  EXPECT_EQ("MOUSE_MOVED 16,40", LocatedEventToEventTypeAndLocation(
                                     window_delegate.PopEvent().get()));

  // Press in client area. Only the client should get the event.
  event_generator.PressLeftButton();
  EXPECT_EQ("POINTER_DOWN 16,40",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopInputEvent().event.get()));

  ASSERT_FALSE(window_delegate.PopEvent().get());
}

TEST(WindowServiceClientTest, PointerWatcher) {
  WindowServiceTestSetup setup;
  TestWindowTreeClient* window_tree_client = setup.window_tree_client();
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(1);
  ASSERT_TRUE(top_level);
  setup.client_test_helper()->SetEventTargetingPolicy(
      top_level, mojom::EventTargetingPolicy::NONE);
  EXPECT_EQ(mojom::EventTargetingPolicy::NONE,
            top_level->event_targeting_policy());
  // Start the pointer watcher only for pointer down/up.
  setup.client_test_helper()->window_tree()->StartPointerWatcher(false);

  top_level->Show();
  top_level->SetBounds(gfx::Rect(10, 10, 100, 100));

  test::EventGenerator event_generator(setup.root());
  event_generator.MoveMouseTo(50, 50);
  ASSERT_TRUE(window_tree_client->observed_pointer_events().empty());

  event_generator.MoveMouseTo(5, 6);
  ASSERT_TRUE(window_tree_client->observed_pointer_events().empty());

  event_generator.PressLeftButton();
  EXPECT_EQ("POINTER_DOWN 5,6",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopObservedPointerEvent().event.get()));

  event_generator.ReleaseLeftButton();
  EXPECT_EQ("POINTER_UP 5,6",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopObservedPointerEvent().event.get()));

  // Enable observing move events.
  setup.client_test_helper()->window_tree()->StartPointerWatcher(true);
  event_generator.MoveMouseTo(8, 9);
  EXPECT_EQ("POINTER_MOVED 8,9",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopObservedPointerEvent().event.get()));

  const int kTouchId = 11;
  event_generator.MoveTouchId(gfx::Point(2, 3), kTouchId);
  EXPECT_EQ("POINTER_MOVED 2,3",
            LocatedEventToEventTypeAndLocation(
                window_tree_client->PopObservedPointerEvent().event.get()));
}

TEST(WindowServiceClientTest, Capture) {
  WindowServiceTestSetup setup;
  aura::Window* window = setup.client_test_helper()->NewWindow(1);

  // Setting capture on |window| should fail as it's not visible.
  EXPECT_FALSE(setup.client_test_helper()->SetCapture(window));

  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(2);
  ASSERT_TRUE(top_level);
  EXPECT_FALSE(setup.client_test_helper()->SetCapture(top_level));
  top_level->Show();
  EXPECT_TRUE(setup.client_test_helper()->SetCapture(top_level));

  EXPECT_FALSE(setup.client_test_helper()->ReleaseCapture(window));
  EXPECT_TRUE(setup.client_test_helper()->ReleaseCapture(top_level));

  top_level->AddChild(window);
  window->Show();
  EXPECT_TRUE(setup.client_test_helper()->SetCapture(window));
  EXPECT_TRUE(setup.client_test_helper()->ReleaseCapture(window));
}

TEST(WindowServiceClientTest, CaptureNotification) {
  WindowServiceTestSetup setup;
  aura::Window* window = setup.client_test_helper()->NewWindow(1);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(2);
  top_level->AddChild(window);
  ASSERT_TRUE(top_level);
  top_level->Show();
  window->Show();
  setup.changes()->clear();
  EXPECT_TRUE(setup.client_test_helper()->SetCapture(window));
  EXPECT_TRUE(setup.changes()->empty());

  wm::CaptureController::Get()->ReleaseCapture(window);
  EXPECT_EQ("OnCaptureChanged new_window=null old_window=0,1",
            SingleChangeToDescription(*(setup.changes())));
}

TEST(WindowServiceClientTest, CaptureNotificationForEmbedRoot) {
  WindowServiceTestSetup setup;
  aura::Window* window = setup.client_test_helper()->NewWindow(1);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(2);
  top_level->AddChild(window);
  ASSERT_TRUE(top_level);
  top_level->Show();
  window->Show();
  setup.changes()->clear();
  EXPECT_TRUE(setup.client_test_helper()->SetCapture(window));
  EXPECT_TRUE(setup.changes()->empty());

  // Set capture on the embed-root from the embedded client. The embedder
  // should be notified.
  std::unique_ptr<EmbeddingHelper> embedding_helper =
      setup.CreateEmbedding(window);
  ASSERT_TRUE(embedding_helper);
  setup.changes()->clear();
  embedding_helper->changes()->clear();
  EXPECT_TRUE(embedding_helper->client_test_helper->SetCapture(window));
  EXPECT_EQ("OnCaptureChanged new_window=null old_window=0,1",
            SingleChangeToDescription(*(setup.changes())));
  setup.changes()->clear();
  EXPECT_TRUE(embedding_helper->changes()->empty());

  // Set capture from the embedder. This triggers the embedded client to lose
  // capture.
  EXPECT_TRUE(setup.client_test_helper()->SetCapture(window));
  EXPECT_TRUE(setup.changes()->empty());
  // NOTE: the '2' is because the embedded client sees the high order bits of
  // the root.
  EXPECT_EQ("OnCaptureChanged new_window=null old_window=2,1",
            SingleChangeToDescription(*(embedding_helper->changes())));
  embedding_helper->changes()->clear();

  // And release capture locally.
  wm::CaptureController::Get()->ReleaseCapture(window);
  EXPECT_EQ("OnCaptureChanged new_window=null old_window=0,1",
            SingleChangeToDescription(*(setup.changes())));
  EXPECT_TRUE(embedding_helper->changes()->empty());
}

TEST(WindowServiceClientTest, CaptureNotificationForTopLevel) {
  WindowServiceTestSetup setup;
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(11);
  ASSERT_TRUE(top_level);
  top_level->Show();
  setup.changes()->clear();
  EXPECT_TRUE(setup.client_test_helper()->SetCapture(top_level));
  EXPECT_TRUE(setup.changes()->empty());

  // Release capture locally.
  wm::CaptureController* capture_controller = wm::CaptureController::Get();
  capture_controller->ReleaseCapture(top_level);
  EXPECT_EQ("OnCaptureChanged new_window=null old_window=0,11",
            SingleChangeToDescription(*(setup.changes())));
  setup.changes()->clear();

  // Set capture locally.
  capture_controller->SetCapture(top_level);
  EXPECT_TRUE(setup.changes()->empty());

  // Set capture from client.
  EXPECT_TRUE(setup.client_test_helper()->SetCapture(top_level));
  EXPECT_TRUE(setup.changes()->empty());

  // Release locally.
  capture_controller->ReleaseCapture(top_level);
  EXPECT_EQ("OnCaptureChanged new_window=null old_window=0,11",
            SingleChangeToDescription(*(setup.changes())));
}

TEST(WindowServiceClientTest, EventsGoToCaptureWindow) {
  WindowServiceTestSetup setup;
  aura::Window* window = setup.client_test_helper()->NewWindow(1);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(2);
  top_level->AddChild(window);
  ASSERT_TRUE(top_level);
  top_level->Show();
  window->Show();
  top_level->SetBounds(gfx::Rect(0, 0, 100, 100));
  window->SetBounds(gfx::Rect(10, 10, 90, 90));
  // Left press on the top-level, leaving mouse down.
  test::EventGenerator event_generator(setup.root());
  event_generator.MoveMouseTo(5, 5);
  event_generator.PressLeftButton();
  setup.window_tree_client()->ClearInputEvents();

  // Set capture on |window|.
  EXPECT_TRUE(setup.client_test_helper()->SetCapture(window));
  EXPECT_TRUE(setup.window_tree_client()->input_events().empty());

  // Move mouse, should go to |window|.
  event_generator.MoveMouseTo(6, 6);
  auto drag_event = setup.window_tree_client()->PopInputEvent();
  EXPECT_EQ(setup.client_test_helper()->TransportIdForWindow(window),
            drag_event.window_id);
  EXPECT_EQ("POINTER_MOVED -4,-4",
            LocatedEventToEventTypeAndLocation(drag_event.event.get()));
}

TEST(WindowServiceClientTest, PointerDownResetOnCaptureChange) {
  WindowServiceTestSetup setup;
  aura::Window* window = setup.client_test_helper()->NewWindow(1);
  ASSERT_TRUE(window);
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(2);
  ASSERT_TRUE(top_level);
  top_level->AddChild(window);
  setup.client_test_helper()->SetClientArea(top_level,
                                            gfx::Insets(10, 0, 0, 0));
  top_level->Show();
  window->Show();
  top_level->SetBounds(gfx::Rect(0, 0, 100, 100));
  window->SetBounds(gfx::Rect(10, 10, 90, 90));
  // Left press on the top-level, leaving mouse down.
  test::EventGenerator event_generator(setup.root());
  event_generator.MoveMouseTo(5, 5);
  event_generator.PressLeftButton();
  ClientWindow* top_level_client_window = ClientWindow::GetMayBeNull(top_level);
  ASSERT_TRUE(top_level_client_window);
  ClientWindowTestHelper top_level_client_window_helper(
      top_level_client_window);
  EXPECT_TRUE(top_level_client_window_helper.IsInPointerPressed());

  // Set capture on |window|, top_level should no longer be in pointer-down
  // (because capture changed).
  EXPECT_TRUE(setup.client_test_helper()->SetCapture(window));
  EXPECT_FALSE(top_level_client_window_helper.IsInPointerPressed());
}

TEST(WindowServiceClientTest, PointerDownResetOnHide) {
  WindowServiceTestSetup setup;
  aura::Window* top_level = setup.client_test_helper()->NewTopLevelWindow(2);
  ASSERT_TRUE(top_level);
  setup.client_test_helper()->SetClientArea(top_level,
                                            gfx::Insets(10, 0, 0, 0));
  top_level->Show();
  top_level->SetBounds(gfx::Rect(0, 0, 100, 100));
  // Left press on the top-level, leaving mouse down.
  test::EventGenerator event_generator(setup.root());
  event_generator.MoveMouseTo(5, 5);
  event_generator.PressLeftButton();
  ClientWindow* top_level_client_window = ClientWindow::GetMayBeNull(top_level);
  ASSERT_TRUE(top_level_client_window);
  ClientWindowTestHelper top_level_client_window_helper(
      top_level_client_window);
  EXPECT_TRUE(top_level_client_window_helper.IsInPointerPressed());

  // Hiding should implicitly cancel capture.
  top_level->Hide();
  EXPECT_FALSE(top_level_client_window_helper.IsInPointerPressed());
}

TEST(WindowServiceClientTest, DeleteWindow) {
  WindowServiceTestSetup setup;
  aura::Window* window = setup.client_test_helper()->NewWindow(1);
  ASSERT_TRUE(window);
  aura::WindowTracker tracker;
  tracker.Add(window);
  setup.changes()->clear();
  setup.client_test_helper()->DeleteWindow(window);
  EXPECT_TRUE(tracker.windows().empty());
  EXPECT_EQ("ChangeCompleted id=1 sucess=true",
            SingleChangeToDescription(*setup.changes()));
}

TEST(WindowServiceClientTest, ExternalDeleteWindow) {
  WindowServiceTestSetup setup;
  aura::Window* window = setup.client_test_helper()->NewWindow(1);
  ASSERT_TRUE(window);
  setup.changes()->clear();
  delete window;
  EXPECT_EQ("WindowDeleted window=0,1",
            SingleChangeToDescription(*setup.changes()));
}

TEST(WindowServiceClientTest, Embed) {
  WindowServiceTestSetup setup;
  aura::Window* window = setup.client_test_helper()->NewWindow(2);
  aura::Window* embed_window = setup.client_test_helper()->NewWindow(3);
  ASSERT_TRUE(window);
  ASSERT_TRUE(embed_window);
  window->AddChild(embed_window);
  embed_window->SetBounds(gfx::Rect(1, 2, 3, 4));

  std::unique_ptr<EmbeddingHelper> embedding_helper =
      setup.CreateEmbedding(embed_window);
  ASSERT_TRUE(embedding_helper);
  ASSERT_EQ("OnEmbed", SingleChangeToDescription(*embedding_helper->changes()));
  const Change& test_change = (*embedding_helper->changes())[0];
  ASSERT_EQ(1u, test_change.windows.size());
  EXPECT_EQ(embed_window->bounds(), test_change.windows[0].bounds);
  EXPECT_EQ(kInvalidTransportId, test_change.windows[0].parent_id);
  EXPECT_EQ(embed_window->TargetVisibility(), test_change.windows[0].visible);
  EXPECT_NE(kInvalidTransportId, test_change.windows[0].window_id);
}

}  // namespace
}  // namespace ws2
}  // namespace ui
