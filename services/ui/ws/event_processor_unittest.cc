// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/event_processor.h"

#include <stddef.h>
#include <stdint.h>

#include "base/command_line.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "components/viz/common/features.h"
#include "components/viz/host/hit_test/hit_test_query.h"
#include "services/ui/common/accelerator_util.h"
#include "services/ui/common/switches.h"
#include "services/ui/public/interfaces/window_tree_constants.mojom.h"
#include "services/ui/ws/accelerator.h"
#include "services/ui/ws/event_dispatcher.h"
#include "services/ui/ws/event_location.h"
#include "services/ui/ws/event_processor_delegate.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/test_server_window_delegate.h"
#include "services/ui/ws/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"

namespace ui {
namespace ws {
namespace test {
namespace {

// Client ids used to indicate the client area and non-client area.
const ClientSpecificId kClientAreaId = 11;
const ClientSpecificId kNonclientAreaId = 111;

// Identifies a generated event.
struct DispatchedEventDetails {
  DispatchedEventDetails()
      : window(nullptr), client_id(kInvalidClientId), accelerator(nullptr) {}

  bool IsNonclientArea() const { return client_id == kNonclientAreaId; }
  bool IsClientArea() const { return client_id == kClientAreaId; }

  ServerWindow* window;
  ClientSpecificId client_id;
  std::unique_ptr<ui::Event> event;
  Accelerator* accelerator;
};

// Serves as both the EventProcessorDelegate and EventDispatcher
// implementation. Records interesting calls for various assertions.
class TestEventProcessorDelegate : public EventProcessorDelegate,
                                   public EventDispatcher {
 public:
  // Delegate interface used by this class to release capture on event
  // dispatcher.
  class Delegate {
   public:
    virtual void ReleaseCapture() = 0;
  };

  explicit TestEventProcessorDelegate(Delegate* delegate)
      : delegate_(delegate),
        focused_window_(nullptr),
        lost_capture_window_(nullptr),
        last_accelerator_(0) {}
  ~TestEventProcessorDelegate() override {}

  void EnableFallbackToRoot() { fallback_to_root_ = true; }

  void set_is_window_in_display_root(bool value) {
    is_window_in_display_root_ = value;
  }

  ui::Event* last_event_target_not_found() {
    return last_event_target_not_found_.get();
  }

  uint32_t GetAndClearLastAccelerator() {
    uint32_t return_value = last_accelerator_;
    last_accelerator_ = 0;
    last_accelerator_phase_ = AcceleratorPhase::kPost;
    return return_value;
  }

  AcceleratorPhase last_accelerator_phase() const {
    return last_accelerator_phase_;
  }

  void set_root(ServerWindow* root) { root_ = root; }

  ServerWindow* window_that_blocked_event() {
    return window_that_blocked_event_;
  }
  void ClearWindowThatBlockedEvent() { window_that_blocked_event_ = nullptr; }

  // Returns the last dispatched event, or null if there are no more.
  std::unique_ptr<DispatchedEventDetails>
  GetAndAdvanceDispatchedEventDetails() {
    if (dispatched_event_queue_.empty())
      return nullptr;

    std::unique_ptr<DispatchedEventDetails> details =
        std::move(dispatched_event_queue_.front());
    dispatched_event_queue_.pop();
    return details;
  }

  ServerWindow* GetAndClearLastFocusedWindow() {
    ServerWindow* result = focused_window_;
    focused_window_ = nullptr;
    return result;
  }

  bool has_queued_events() const { return !dispatched_event_queue_.empty(); }
  ServerWindow* lost_capture_window() { return lost_capture_window_; }

  base::Optional<bool> last_cursor_visibility() {
    return last_cursor_visibility_;
  }

  viz::HitTestQuery* hit_test_query() { return &hit_test_query_; }

  void AddWindow(ServerWindow* window) { windows_.push_back(window); }

  // EventProcessorDelegate:
  void SetFocusedWindowFromEventProcessor(ServerWindow* window) override {
    focused_window_ = window;
  }

 private:
  // EventDispatcher:
  void DispatchInputEventToWindow(ServerWindow* target,
                                  ClientSpecificId client_id,
                                  const EventLocation& event_location,
                                  const ui::Event& event,
                                  Accelerator* accelerator) override {
    std::unique_ptr<DispatchedEventDetails> details(new DispatchedEventDetails);
    details->window = target;
    details->client_id = client_id;
    details->event = ui::Event::Clone(event);
    details->accelerator = accelerator;
    dispatched_event_queue_.push(std::move(details));
  }
  void OnAccelerator(uint32_t accelerator,
                     int64_t display_id,
                     const ui::Event& event,
                     AcceleratorPhase phase) override {
    EXPECT_EQ(0u, last_accelerator_);
    last_accelerator_ = accelerator;
    last_accelerator_phase_ = phase;
  }

  // EventProcessorDelegate:
  ServerWindow* GetFocusedWindowForEventProcessor(int64_t display_id) override {
    return focused_window_;
  }
  void SetNativeCapture(ServerWindow* window) override {}
  void ReleaseNativeCapture() override {
    if (delegate_)
      delegate_->ReleaseCapture();
  }
  void UpdateNativeCursorFromEventProcessor() override {}
  void OnCaptureChanged(ServerWindow* new_capture_window,
                        ServerWindow* old_capture_window) override {
    lost_capture_window_ = old_capture_window;
  }
  void OnMouseCursorLocationChanged(const gfx::PointF& point,
                                    int64_t display_id) override {}
  void OnEventChangesCursorVisibility(const ui::Event& event,
                                      bool visible) override {
    last_cursor_visibility_ = visible;
  }
  void OnEventChangesCursorTouchVisibility(const ui::Event& event,
                                           bool visible) override {}
  ClientSpecificId GetEventTargetClientId(const ServerWindow* window,
                                          bool in_nonclient_area) override {
    return in_nonclient_area ? kNonclientAreaId : kClientAreaId;
  }
  ServerWindow* GetRootWindowForDisplay(int64_t display_id) override {
    return root_;
  }
  void OnEventTargetNotFound(const ui::Event& event,
                             int64_t display_id) override {
    last_event_target_not_found_ = ui::Event::Clone(event);
  }
  ServerWindow* GetFallbackTargetForEventBlockedByModal(
      ServerWindow* window) override {
    return fallback_to_root_ ? root_ : nullptr;
  }
  void OnEventOccurredOutsideOfModalWindow(
      ServerWindow* modal_transient) override {
    window_that_blocked_event_ = modal_transient;
  }
  viz::HitTestQuery* GetHitTestQueryForDisplay(int64_t display_id) override {
    return &hit_test_query_;
  }
  ServerWindow* GetWindowFromFrameSinkId(
      const viz::FrameSinkId& frame_sink_id) override {
    for (ServerWindow* window : windows_) {
      if (window->frame_sink_id() == frame_sink_id)
        return window;
    }
    return nullptr;
  }
  ServerWindow* GetRootWindowForEventDispatch(ServerWindow* window) override {
    return is_window_in_display_root_ ? window->GetRootForDrawn() : nullptr;
  }

  Delegate* delegate_;
  ServerWindow* focused_window_;
  ServerWindow* lost_capture_window_;
  uint32_t last_accelerator_;
  AcceleratorPhase last_accelerator_phase_ = AcceleratorPhase::kPost;
  base::queue<std::unique_ptr<DispatchedEventDetails>> dispatched_event_queue_;
  ServerWindow* root_ = nullptr;
  std::unique_ptr<ui::Event> last_event_target_not_found_;
  base::Optional<bool> last_cursor_visibility_;
  ServerWindow* window_that_blocked_event_ = nullptr;
  viz::HitTestQuery hit_test_query_;
  std::vector<ServerWindow*> windows_;

  // If true events blocked by a modal window are sent to |root_|.
  bool fallback_to_root_ = false;

  bool is_window_in_display_root_ = true;

  DISALLOW_COPY_AND_ASSIGN(TestEventProcessorDelegate);
};

// Used by RunMouseEventTests(). Can identify up to two generated events. The
// first ServerWindow and two points identify the first event, the second
// ServerWindow and points identify the second event. If only one event is
// generated set the second window to null.
struct MouseEventTest {
  std::unique_ptr<ui::MouseEvent> input_event;
  ServerWindow* expected_target_window1;
  gfx::Point expected_root_location1;
  gfx::Point expected_location1;
  ServerWindow* expected_target_window2;
  gfx::Point expected_root_location2;
  gfx::Point expected_location2;
};

// Verifies |details| matches the supplied ServerWindow and points.
void ExpectDispatchedEventDetailsMatches(const DispatchedEventDetails* details,
                                         ServerWindow* target,
                                         const gfx::Point& root_location,
                                         const gfx::Point& location) {
  if (!target) {
    ASSERT_FALSE(details);
    return;
  }
  ASSERT_TRUE(details);
  ASSERT_EQ(target, details->window);
  ASSERT_TRUE(details->event);
  ASSERT_TRUE(details->event->IsLocatedEvent());
  ASSERT_TRUE(details->IsClientArea());
  ASSERT_EQ(root_location, details->event->AsLocatedEvent()->root_location());
  ASSERT_EQ(location, details->event->AsLocatedEvent()->location());
}

ui::mojom::EventMatcherPtr BuildKeyMatcher(ui::mojom::KeyboardCode code) {
  ui::mojom::EventMatcherPtr matcher(ui::mojom::EventMatcher::New());
  matcher->key_matcher = ui::mojom::KeyEventMatcher::New();
  matcher->key_matcher->keyboard_code = code;
  return matcher;
}

EventLocation EventLocationFromEvent(const Event& event) {
  EventLocation event_location(0);
  if (event.IsLocatedEvent()) {
    event_location.raw_location = event_location.location =
        event.AsLocatedEvent()->root_location_f();
  }
  return event_location;
}

}  // namespace

// Test fixture for EventProcessor with friend access to verify the internal
// state. Setup creates a TestServerWindowDelegate, a visible root ServerWindow,
// a TestEventProcessor and the EventProcessor for testing.
class EventProcessorTest : public testing::TestWithParam<bool>,
                           public TestEventProcessorDelegate::Delegate {
 public:
  EventProcessorTest() {}
  ~EventProcessorTest() override {}

  ServerWindow* root_window() { return root_window_.get(); }
  TestEventProcessorDelegate* test_event_dispatcher_delegate() {
    return test_event_dispatcher_delegate_.get();
  }
  EventProcessor* event_dispatcher() { return event_dispatcher_.get(); }
  VizHostProxy* viz_host_proxy() {
    return ws_test_helper_.window_server()->GetVizHostProxy();
  }
  TestServerWindowDelegate* window_delegate() { return window_delegate_.get(); }

  void DispatchEvent(EventProcessor* dispatcher,
                     const ui::Event& event,
                     EventProcessor::AcceleratorMatchPhase match_phase);
  void RunMouseEventTests(EventProcessor* dispatcher,
                          TestEventProcessorDelegate* dispatcher_delegate,
                          MouseEventTest* tests,
                          size_t test_count);
  bool AreAnyPointersDown() const;
  // Deletes everything created during SetUp()
  void ClearSetup();
  std::unique_ptr<ServerWindow> CreateChildWindowWithParent(
      const viz::FrameSinkId& id,
      ServerWindow* parent);
  // Creates a window which is a child of |root_window_|.
  std::unique_ptr<ServerWindow> CreateChildWindow(const viz::FrameSinkId& id);
  bool IsMouseButtonDown() const;
  bool IsWindowPointerTarget(const ServerWindow* window) const;
  int NumberPointerTargetsForWindow(ServerWindow* window) const;
  const ServerWindow* GetActiveSystemModalWindow() const;

 protected:
  bool is_event_processing_async() const { return GetParam(); }

  // testing::TestWithParam<bool>:
  void SetUp() override;

  void RunTasks();

 private:
  // TestEventProcessorDelegate::Delegate:
  void ReleaseCapture() override {
    event_dispatcher_->SetCaptureWindow(nullptr, kInvalidClientId);
  }

  WindowServerTestHelper ws_test_helper_;

  std::unique_ptr<TestServerWindowDelegate> window_delegate_;
  std::unique_ptr<ServerWindow> root_window_;
  std::unique_ptr<TestEventProcessorDelegate> test_event_dispatcher_delegate_;
  std::unique_ptr<EventProcessor> event_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(EventProcessorTest);
};

void EventProcessorTest::DispatchEvent(
    EventProcessor* dispatcher,
    const ui::Event& event,
    EventProcessor::AcceleratorMatchPhase match_phase) {
  dispatcher->ProcessEvent(event, EventLocationFromEvent(event), match_phase);
  RunTasks();
}

void EventProcessorTest::RunMouseEventTests(
    EventProcessor* dispatcher,
    TestEventProcessorDelegate* dispatcher_delegate,
    MouseEventTest* tests,
    size_t test_count) {
  for (size_t i = 0; i < test_count; ++i) {
    const MouseEventTest& test = tests[i];
    ASSERT_FALSE(dispatcher_delegate->has_queued_events())
        << " unexpected queued events before running " << i;
    DispatchEvent(dispatcher, ui::PointerEvent(*test.input_event),
                  EventProcessor::AcceleratorMatchPhase::ANY);

    std::unique_ptr<DispatchedEventDetails> details =
        dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
    ASSERT_NO_FATAL_FAILURE(ExpectDispatchedEventDetailsMatches(
        details.get(), test.expected_target_window1,
        test.expected_root_location1, test.expected_location1))
        << " details don't match " << i;
    details = dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
    ASSERT_NO_FATAL_FAILURE(ExpectDispatchedEventDetailsMatches(
        details.get(), test.expected_target_window2,
        test.expected_root_location2, test.expected_location2))
        << " details2 don't match " << i;
    ASSERT_FALSE(dispatcher_delegate->has_queued_events())
        << " unexpected queued events after running " << i;
  }
}

bool EventProcessorTest::AreAnyPointersDown() const {
  return EventProcessorTestApi(event_dispatcher_.get()).AreAnyPointersDown();
}

void EventProcessorTest::ClearSetup() {
  // ServerWindowDelegate should outlive ServerWindow.
  root_window_.reset();
  window_delegate_.reset();
  test_event_dispatcher_delegate_.reset();
  event_dispatcher_.reset();
}

std::unique_ptr<ServerWindow> EventProcessorTest::CreateChildWindowWithParent(
    const viz::FrameSinkId& id,
    ServerWindow* parent) {
  std::unique_ptr<ServerWindow> child(
      new ServerWindow(window_delegate_.get(), id));
  parent->Add(child.get());
  child->SetVisible(true);
  return child;
}

std::unique_ptr<ServerWindow> EventProcessorTest::CreateChildWindow(
    const viz::FrameSinkId& id) {
  return CreateChildWindowWithParent(id, root_window_.get());
}

bool EventProcessorTest::IsMouseButtonDown() const {
  return EventProcessorTestApi(event_dispatcher_.get()).is_mouse_button_down();
}

bool EventProcessorTest::IsWindowPointerTarget(
    const ServerWindow* window) const {
  return EventProcessorTestApi(event_dispatcher_.get())
      .IsWindowPointerTarget(window);
}

int EventProcessorTest::NumberPointerTargetsForWindow(
    ServerWindow* window) const {
  return EventProcessorTestApi(event_dispatcher_.get())
      .NumberPointerTargetsForWindow(window);
}

const ServerWindow* EventProcessorTest::GetActiveSystemModalWindow() const {
  ModalWindowController* mwc =
      EventProcessorTestApi(event_dispatcher_.get()).modal_window_controller();
  return ModalWindowControllerTestApi(mwc).GetActiveSystemModalWindow();
}

void EventProcessorTest::RunTasks() {
  if (!is_event_processing_async())
    return;

  base::RunLoop runloop;
  runloop.RunUntilIdle();
}

void EventProcessorTest::SetUp() {
  if (is_event_processing_async()) {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kUseAsyncEventTargeting);
  }
  testing::TestWithParam<bool>::SetUp();

  window_delegate_ =
      std::make_unique<TestServerWindowDelegate>(viz_host_proxy());
  root_window_ = std::make_unique<ServerWindow>(window_delegate_.get(),
                                                viz::FrameSinkId(1, 2));
  root_window_->set_is_activation_parent(true);
  window_delegate_->set_root_window(root_window_.get());
  root_window_->SetVisible(true);

  test_event_dispatcher_delegate_ =
      std::make_unique<TestEventProcessorDelegate>(this);
  event_dispatcher_ =
      std::make_unique<EventProcessor>(test_event_dispatcher_delegate_.get(),
                                       test_event_dispatcher_delegate_.get());
  test_event_dispatcher_delegate_->set_root(root_window_.get());
}

TEST_P(EventProcessorTest, ProcessEvent) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  // Send event that is over child.
  const ui::PointerEvent ui_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(20, 25), gfx::Point(20, 25),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), ui_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  ASSERT_EQ(child.get(), details->window);

  ASSERT_TRUE(details->event);
  ASSERT_TRUE(details->event->IsPointerEvent());

  ui::PointerEvent* dispatched_event = details->event->AsPointerEvent();
  EXPECT_EQ(gfx::Point(20, 25), dispatched_event->root_location());
  EXPECT_EQ(gfx::Point(10, 15), dispatched_event->location());
}

TEST_P(EventProcessorTest, ProcessEventNoTarget) {
  // Send event without a target.
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  DispatchEvent(event_dispatcher(), key,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // Event wasn't dispatched to a target.
  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(details);

  // Delegate was informed that there wasn't a target.
  ui::Event* event_out =
      test_event_dispatcher_delegate()->last_event_target_not_found();
  ASSERT_TRUE(event_out);
  EXPECT_TRUE(event_out->IsKeyEvent());
  EXPECT_EQ(ui::VKEY_A, event_out->AsKeyEvent()->key_code());
}

TEST_P(EventProcessorTest, AcceleratorBasic) {
  ClearSetup();
  TestEventProcessorDelegate event_dispatcher_delegate(nullptr);
  EventProcessor dispatcher(&event_dispatcher_delegate,
                            &event_dispatcher_delegate);

  uint32_t accelerator_1 = 1;
  mojom::EventMatcherPtr matcher = ui::CreateKeyMatcher(
      ui::mojom::KeyboardCode::W, ui::mojom::kEventFlagControlDown);
  EXPECT_TRUE(dispatcher.AddAccelerator(accelerator_1, std::move(matcher)));

  uint32_t accelerator_2 = 2;
  matcher = ui::CreateKeyMatcher(ui::mojom::KeyboardCode::N,
                                 ui::mojom::kEventFlagNone);
  EXPECT_TRUE(dispatcher.AddAccelerator(accelerator_2, std::move(matcher)));

  // Attempting to add a new accelerator with the same id should fail.
  matcher = ui::CreateKeyMatcher(ui::mojom::KeyboardCode::T,
                                 ui::mojom::kEventFlagNone);
  EXPECT_FALSE(dispatcher.AddAccelerator(accelerator_2, std::move(matcher)));

  // Adding the accelerator with the same id should succeed once the existing
  // accelerator is removed.
  dispatcher.RemoveAccelerator(accelerator_2);
  matcher = ui::CreateKeyMatcher(ui::mojom::KeyboardCode::T,
                                 ui::mojom::kEventFlagNone);
  EXPECT_TRUE(dispatcher.AddAccelerator(accelerator_2, std::move(matcher)));

  // Attempting to add an accelerator with the same matcher should fail.
  uint32_t accelerator_3 = 3;
  matcher = ui::CreateKeyMatcher(ui::mojom::KeyboardCode::T,
                                 ui::mojom::kEventFlagNone);
  EXPECT_FALSE(dispatcher.AddAccelerator(accelerator_3, std::move(matcher)));

  matcher = ui::CreateKeyMatcher(ui::mojom::KeyboardCode::T,
                                 ui::mojom::kEventFlagControlDown);
  EXPECT_TRUE(dispatcher.AddAccelerator(accelerator_3, std::move(matcher)));
}

TEST_P(EventProcessorTest, EventMatching) {
  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EventProcessor* dispatcher = event_dispatcher();

  mojom::EventMatcherPtr matcher = ui::CreateKeyMatcher(
      ui::mojom::KeyboardCode::W, ui::mojom::kEventFlagControlDown);
  uint32_t accelerator_1 = 1;
  dispatcher->AddAccelerator(accelerator_1, std::move(matcher));

  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  DispatchEvent(dispatcher, key, EventProcessor::AcceleratorMatchPhase::ANY);
  EXPECT_EQ(accelerator_1,
            event_dispatcher_delegate->GetAndClearLastAccelerator());

  // EF_NUM_LOCK_ON should be ignored since CreateKeyMatcher defaults to
  // ignoring.
  key = ui::KeyEvent(ui::ET_KEY_PRESSED, ui::VKEY_W,
                     ui::EF_CONTROL_DOWN | ui::EF_NUM_LOCK_ON);
  DispatchEvent(dispatcher, key, EventProcessor::AcceleratorMatchPhase::ANY);
  EXPECT_EQ(accelerator_1,
            event_dispatcher_delegate->GetAndClearLastAccelerator());

  key = ui::KeyEvent(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_NONE);
  DispatchEvent(dispatcher, key, EventProcessor::AcceleratorMatchPhase::ANY);
  EXPECT_EQ(0u, event_dispatcher_delegate->GetAndClearLastAccelerator());

  uint32_t accelerator_2 = 2;
  matcher = ui::CreateKeyMatcher(ui::mojom::KeyboardCode::W,
                                 ui::mojom::kEventFlagNone);
  dispatcher->AddAccelerator(accelerator_2, std::move(matcher));
  DispatchEvent(dispatcher, key, EventProcessor::AcceleratorMatchPhase::ANY);
  EXPECT_EQ(accelerator_2,
            event_dispatcher_delegate->GetAndClearLastAccelerator());

  dispatcher->RemoveAccelerator(accelerator_2);
  DispatchEvent(dispatcher, key, EventProcessor::AcceleratorMatchPhase::ANY);
  EXPECT_EQ(0u, event_dispatcher_delegate->GetAndClearLastAccelerator());
}

// Tests that a post-target accelerator is not triggered by ProcessEvent.
TEST_P(EventProcessorTest, PostTargetAccelerator) {
  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EventProcessor* dispatcher = event_dispatcher();

  mojom::EventMatcherPtr matcher = ui::CreateKeyMatcher(
      ui::mojom::KeyboardCode::W, ui::mojom::kEventFlagControlDown);
  matcher->accelerator_phase = ui::mojom::AcceleratorPhase::POST_TARGET;
  uint32_t accelerator_1 = 1;
  dispatcher->AddAccelerator(accelerator_1, std::move(matcher));

  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  // The post-target accelerator should be fired if there is no focused window.
  DispatchEvent(dispatcher, key, EventProcessor::AcceleratorMatchPhase::ANY);
  EXPECT_EQ(accelerator_1,
            event_dispatcher_delegate->GetAndClearLastAccelerator());
  std::unique_ptr<DispatchedEventDetails> details =
      event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(details);

  // Set focused window for EventProcessor dispatches key events.
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  event_dispatcher_delegate->SetFocusedWindowFromEventProcessor(child.get());

  // With a focused window the event should be dispatched.
  DispatchEvent(dispatcher, key, EventProcessor::AcceleratorMatchPhase::ANY);
  EXPECT_EQ(0u, event_dispatcher_delegate->GetAndClearLastAccelerator());
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_TRUE(details);
  EXPECT_TRUE(details->accelerator);

  base::WeakPtr<Accelerator> accelerator_weak_ptr =
      details->accelerator->GetWeakPtr();
  dispatcher->RemoveAccelerator(accelerator_1);
  EXPECT_FALSE(accelerator_weak_ptr);

  // Post deletion there should be no accelerator
  DispatchEvent(dispatcher, key, EventProcessor::AcceleratorMatchPhase::ANY);
  EXPECT_EQ(0u, event_dispatcher_delegate->GetAndClearLastAccelerator());
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_TRUE(details);
  EXPECT_FALSE(details->accelerator);
}

TEST_P(EventProcessorTest, ProcessPost) {
  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EventProcessor* dispatcher = event_dispatcher();

  uint32_t pre_id = 1;
  {
    mojom::EventMatcherPtr matcher = ui::CreateKeyMatcher(
        ui::mojom::KeyboardCode::W, ui::mojom::kEventFlagControlDown);
    matcher->accelerator_phase = ui::mojom::AcceleratorPhase::PRE_TARGET;
    dispatcher->AddAccelerator(pre_id, std::move(matcher));
  }

  uint32_t post_id = 2;
  {
    mojom::EventMatcherPtr matcher = ui::CreateKeyMatcher(
        ui::mojom::KeyboardCode::W, ui::mojom::kEventFlagControlDown);
    matcher->accelerator_phase = ui::mojom::AcceleratorPhase::POST_TARGET;
    dispatcher->AddAccelerator(post_id, std::move(matcher));
  }

  // Set focused window for EventProcessor dispatches key events.
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  event_dispatcher_delegate->SetFocusedWindowFromEventProcessor(child.get());

  // Dispatch for ANY, which should trigger PRE and not call
  // DispatchInputEventToWindow().
  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_W, ui::EF_CONTROL_DOWN);
  DispatchEvent(dispatcher, key, EventProcessor::AcceleratorMatchPhase::ANY);
  EXPECT_EQ(EventDispatcher::AcceleratorPhase::kPre,
            event_dispatcher_delegate->last_accelerator_phase());
  EXPECT_EQ(pre_id, event_dispatcher_delegate->GetAndClearLastAccelerator());
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());

  // Dispatch for POST, which should trigger POST.
  DispatchEvent(dispatcher, key,
                EventProcessor::AcceleratorMatchPhase::POST_ONLY);
  std::unique_ptr<DispatchedEventDetails> details =
      event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  ASSERT_TRUE(details->accelerator);
  EXPECT_EQ(post_id, details->accelerator->id());
}

TEST_P(EventProcessorTest, Capture) {
  ServerWindow* root = root_window();
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  MouseEventTest tests[] = {
      // Send a mouse down event over child.
      {std::make_unique<ui::MouseEvent>(
           ui::ET_MOUSE_PRESSED, gfx::Point(20, 25), gfx::Point(20, 25),
           base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
           ui::EF_LEFT_MOUSE_BUTTON),
       child.get(), gfx::Point(20, 25), gfx::Point(10, 15), nullptr,
       gfx::Point(), gfx::Point()},

      // Capture should be activated. Let's send a mouse move outside the bounds
      // of the child.
      {std::make_unique<ui::MouseEvent>(ui::ET_MOUSE_MOVED, gfx::Point(50, 50),
                                        gfx::Point(50, 50), base::TimeTicks(),
                                        ui::EF_LEFT_MOUSE_BUTTON,
                                        ui::EF_LEFT_MOUSE_BUTTON),
       child.get(), gfx::Point(50, 50), gfx::Point(40, 40), nullptr,
       gfx::Point(), gfx::Point()},
      // Release the mouse and verify that the mouse up event goes to the child.
      {std::make_unique<ui::MouseEvent>(
           ui::ET_MOUSE_RELEASED, gfx::Point(50, 50), gfx::Point(50, 50),
           base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
           ui::EF_LEFT_MOUSE_BUTTON),
       child.get(), gfx::Point(50, 50), gfx::Point(40, 40), nullptr,
       gfx::Point(), gfx::Point()},

      // A mouse move at (50, 50) should now go to the root window. As the
      // move crosses between |child| and |root| |child| gets an exit, and
      // |root| the move.
      {std::make_unique<ui::MouseEvent>(ui::ET_MOUSE_MOVED, gfx::Point(50, 50),
                                        gfx::Point(50, 50), base::TimeTicks(),
                                        ui::EF_LEFT_MOUSE_BUTTON,
                                        ui::EF_LEFT_MOUSE_BUTTON),
       child.get(), gfx::Point(50, 50), gfx::Point(40, 40), root,
       gfx::Point(50, 50), gfx::Point(50, 50)},

  };
  RunMouseEventTests(event_dispatcher(), test_event_dispatcher_delegate(),
                     tests, arraysize(tests));
}

TEST_P(EventProcessorTest, CaptureMultipleMouseButtons) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  MouseEventTest tests[] = {
      // Send a mouse down event over child with a left mouse button
      {std::make_unique<ui::MouseEvent>(
           ui::ET_MOUSE_PRESSED, gfx::Point(20, 25), gfx::Point(20, 25),
           base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
           ui::EF_LEFT_MOUSE_BUTTON),
       child.get(), gfx::Point(20, 25), gfx::Point(10, 15), nullptr,
       gfx::Point(), gfx::Point()},

      // Capture should be activated. Let's send a mouse move outside the bounds
      // of the child and press the right mouse button too.
      {std::make_unique<ui::MouseEvent>(
           ui::ET_MOUSE_MOVED, gfx::Point(50, 50), gfx::Point(50, 50),
           base::TimeTicks(),
           ui::EF_LEFT_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON, 0),
       child.get(), gfx::Point(50, 50), gfx::Point(40, 40), nullptr,
       gfx::Point(), gfx::Point()},

      // Release the left mouse button and verify that the mouse up event goes
      // to the child.
      {std::make_unique<ui::MouseEvent>(
           ui::ET_MOUSE_RELEASED, gfx::Point(50, 50), gfx::Point(50, 50),
           base::TimeTicks(),
           ui::EF_LEFT_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON,
           ui::EF_RIGHT_MOUSE_BUTTON),
       child.get(), gfx::Point(50, 50), gfx::Point(40, 40), nullptr,
       gfx::Point(), gfx::Point()},

      // A mouse move at (50, 50) should still go to the child.
      {std::make_unique<ui::MouseEvent>(ui::ET_MOUSE_MOVED, gfx::Point(50, 50),
                                        gfx::Point(50, 50), base::TimeTicks(),
                                        ui::EF_LEFT_MOUSE_BUTTON, 0),
       child.get(), gfx::Point(50, 50), gfx::Point(40, 40), nullptr,
       gfx::Point(), gfx::Point()},

  };
  RunMouseEventTests(event_dispatcher(), test_event_dispatcher_delegate(),
                     tests, arraysize(tests));
}

TEST_P(EventProcessorTest, ClientAreaGoesToOwner) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  child->SetClientArea(gfx::Insets(5, 5, 5, 5), std::vector<gfx::Rect>());

  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EventProcessor* dispatcher = event_dispatcher();

  // Start move loop by sending mouse event over non-client area.
  const ui::PointerEvent press_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(12, 12), gfx::Point(12, 12),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(dispatcher, press_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // Events should target child and be in the non-client area.
  std::unique_ptr<DispatchedEventDetails> details =
      event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  ASSERT_TRUE(details);
  ASSERT_EQ(child.get(), details->window);
  EXPECT_TRUE(details->IsNonclientArea());

  // Move the mouse 5,6 pixels and target is the same.
  const ui::PointerEvent move_event(
      ui::MouseEvent(ui::ET_MOUSE_MOVED, gfx::Point(17, 18), gfx::Point(17, 18),
                     base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  DispatchEvent(dispatcher, move_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // Still same target.
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  ASSERT_EQ(child.get(), details->window);
  EXPECT_TRUE(details->IsNonclientArea());

  // Release the mouse.
  const ui::PointerEvent release_event(ui::MouseEvent(
      ui::ET_MOUSE_RELEASED, gfx::Point(17, 18), gfx::Point(17, 18),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(dispatcher, release_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // The event should not have been dispatched to the delegate.
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  ASSERT_EQ(child.get(), details->window);
  EXPECT_TRUE(details->IsNonclientArea());

  // Press in the client area and verify target/client area. The non-client area
  // should get an exit first.
  const ui::PointerEvent press_event2(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(21, 22), gfx::Point(21, 22),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(dispatcher, press_event2,
                EventProcessor::AcceleratorMatchPhase::ANY);
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_TRUE(event_dispatcher_delegate->has_queued_events());
  ASSERT_EQ(child.get(), details->window);
  EXPECT_TRUE(details->IsNonclientArea());
  EXPECT_EQ(ui::ET_POINTER_EXITED, details->event->type());

  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  ASSERT_EQ(child.get(), details->window);
  EXPECT_TRUE(details->IsClientArea());
  EXPECT_EQ(ui::ET_POINTER_DOWN, details->event->type());
}

TEST_P(EventProcessorTest, AdditionalClientArea) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  std::vector<gfx::Rect> additional_client_areas;
  additional_client_areas.push_back(gfx::Rect(18, 0, 2, 2));
  child->SetClientArea(gfx::Insets(5, 5, 5, 5), additional_client_areas);

  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  // Press in the additional client area, it should go to the child.
  const ui::PointerEvent press_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(28, 11), gfx::Point(28, 11),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), press_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // Events should target child and be in the client area.
  std::unique_ptr<DispatchedEventDetails> details =
      event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  ASSERT_EQ(child.get(), details->window);
  EXPECT_TRUE(details->IsClientArea());
}

TEST_P(EventProcessorTest, HitTestMask) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));
  child->SetHitTestMask(gfx::Rect(2, 2, 16, 16));

  // Move in the masked area.
  const ui::PointerEvent move1(
      ui::MouseEvent(ui::ET_MOUSE_MOVED, gfx::Point(11, 11), gfx::Point(11, 11),
                     base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  DispatchEvent(event_dispatcher(), move1,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // Event went through the child window and hit the root.
  std::unique_ptr<DispatchedEventDetails> details1 =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(root_window(), details1->window);
  EXPECT_TRUE(details1->IsClientArea());

  child->ClearHitTestMask();

  // Move right in the same part of the window.
  const ui::PointerEvent move2(
      ui::MouseEvent(ui::ET_MOUSE_MOVED, gfx::Point(11, 12), gfx::Point(11, 12),
                     base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  DispatchEvent(event_dispatcher(), move2,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // Mouse exits the root.
  std::unique_ptr<DispatchedEventDetails> details2 =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(ui::ET_POINTER_EXITED, details2->event->type());

  // Mouse hits the child.
  std::unique_ptr<DispatchedEventDetails> details3 =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(child.get(), details3->window);
  EXPECT_TRUE(details3->IsClientArea());
}

TEST_P(EventProcessorTest, DontFocusOnSecondDown) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EventProcessor* dispatcher = event_dispatcher();

  // Press on |child|. Press should not change focus.
  const ui::PointerEvent press_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(12, 12), gfx::Point(12, 12),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(dispatcher, press_event,
                EventProcessor::AcceleratorMatchPhase::ANY);
  std::unique_ptr<DispatchedEventDetails> details =
      event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  EXPECT_EQ(child.get(), details->window);
  EXPECT_EQ(nullptr, event_dispatcher_delegate->GetAndClearLastFocusedWindow());
}

TEST_P(EventProcessorTest, TwoPointersActive) {
  std::unique_ptr<ServerWindow> child1 =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> child2 =
      CreateChildWindow(viz::FrameSinkId(1, 4));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child1->SetBounds(gfx::Rect(10, 10, 20, 20));
  child2->SetBounds(gfx::Rect(50, 51, 11, 12));

  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EventProcessor* dispatcher = event_dispatcher();

  // Press on child1.
  const ui::PointerEvent touch_event1(ui::TouchEvent(
      ui::ET_TOUCH_PRESSED, gfx::Point(12, 13), base::TimeTicks(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
  DispatchEvent(dispatcher, touch_event1,
                EventProcessor::AcceleratorMatchPhase::ANY);
  std::unique_ptr<DispatchedEventDetails> details =
      event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(child1.get(), details->window);

  // Drag over child2, child1 should get the drag.
  const ui::PointerEvent drag_event1(ui::TouchEvent(
      ui::ET_TOUCH_MOVED, gfx::Point(53, 54), base::TimeTicks(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
  DispatchEvent(dispatcher, drag_event1,
                EventProcessor::AcceleratorMatchPhase::ANY);
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(child1.get(), details->window);

  // Press on child2 with a different touch id.
  const ui::PointerEvent touch_event2(ui::TouchEvent(
      ui::ET_TOUCH_PRESSED, gfx::Point(54, 55), base::TimeTicks(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 2)));
  DispatchEvent(dispatcher, touch_event2,
                EventProcessor::AcceleratorMatchPhase::ANY);
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(child2.get(), details->window);

  // Drag over child1 with id 2, child2 should continue to get the drag.
  const ui::PointerEvent drag_event2(ui::TouchEvent(
      ui::ET_TOUCH_MOVED, gfx::Point(13, 14), base::TimeTicks(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 2)));
  DispatchEvent(dispatcher, drag_event2,
                EventProcessor::AcceleratorMatchPhase::ANY);
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(child2.get(), details->window);

  // Drag again with id 1, child1 should continue to get it.
  DispatchEvent(dispatcher, drag_event1,
                EventProcessor::AcceleratorMatchPhase::ANY);
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(child1.get(), details->window);

  // Release touch id 1, and click on 2. 2 should get it.
  const ui::PointerEvent touch_release(ui::TouchEvent(
      ui::ET_TOUCH_RELEASED, gfx::Point(54, 55), base::TimeTicks(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
  DispatchEvent(dispatcher, touch_release,
                EventProcessor::AcceleratorMatchPhase::ANY);
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(child1.get(), details->window);
  const ui::PointerEvent touch_event3(ui::TouchEvent(
      ui::ET_TOUCH_PRESSED, gfx::Point(54, 55), base::TimeTicks(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 2)));
  DispatchEvent(dispatcher, touch_event3,
                EventProcessor::AcceleratorMatchPhase::ANY);
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(child2.get(), details->window);
}

TEST_P(EventProcessorTest, DestroyWindowWhileGettingEvents) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EventProcessor* dispatcher = event_dispatcher();

  // Press on child.
  const ui::PointerEvent touch_event1(ui::TouchEvent(
      ui::ET_TOUCH_PRESSED, gfx::Point(12, 13), base::TimeTicks(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
  DispatchEvent(dispatcher, touch_event1,
                EventProcessor::AcceleratorMatchPhase::ANY);
  std::unique_ptr<DispatchedEventDetails> details =
      event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  EXPECT_EQ(child.get(), details->window);

  // Delete child, and continue the drag. Event should not be dispatched.
  child.reset();

  const ui::PointerEvent drag_event1(ui::TouchEvent(
      ui::ET_TOUCH_MOVED, gfx::Point(53, 54), base::TimeTicks(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
  DispatchEvent(dispatcher, drag_event1,
                EventProcessor::AcceleratorMatchPhase::ANY);
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(nullptr, details.get());
}

TEST_P(EventProcessorTest, MouseInExtendedHitTestRegion) {
  ServerWindow* root = root_window();
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EventProcessor* dispatcher = event_dispatcher();

  // Send event that is not over child.
  const ui::PointerEvent ui_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(8, 9), gfx::Point(8, 9),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(dispatcher, ui_event,
                EventProcessor::AcceleratorMatchPhase::ANY);
  std::unique_ptr<DispatchedEventDetails> details =
      event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  ASSERT_EQ(root, details->window);

  // Release the mouse.
  const ui::PointerEvent release_event(ui::MouseEvent(
      ui::ET_MOUSE_RELEASED, gfx::Point(8, 9), gfx::Point(8, 9),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(dispatcher, release_event,
                EventProcessor::AcceleratorMatchPhase::ANY);
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  ASSERT_EQ(root, details->window);
  EXPECT_TRUE(details->IsClientArea());

  // Change the extended hit test region and send event in extended hit test
  // region. Should result in exit for root, followed by press for child.
  root->set_extended_hit_test_regions_for_children(gfx::Insets(-5, -5, -5, -5),
                                                   gfx::Insets(-5, -5, -5, -5));
  DispatchEvent(dispatcher, ui_event,
                EventProcessor::AcceleratorMatchPhase::ANY);
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_EQ(root, details->window);
  EXPECT_EQ(ui::ET_POINTER_EXITED, details->event->type());
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);

  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  EXPECT_TRUE(details->IsNonclientArea());
  ASSERT_EQ(child.get(), details->window);
  EXPECT_EQ(ui::ET_POINTER_DOWN, details->event->type());
  ASSERT_TRUE(details->event.get());
  ASSERT_TRUE(details->event->IsPointerEvent());
  EXPECT_EQ(gfx::Point(-2, -1), details->event->AsPointerEvent()->location());
}

TEST_P(EventProcessorTest, WheelWhileDown) {
  std::unique_ptr<ServerWindow> child1 =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> child2 =
      CreateChildWindow(viz::FrameSinkId(1, 4));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child1->SetBounds(gfx::Rect(10, 10, 20, 20));
  child2->SetBounds(gfx::Rect(50, 51, 11, 12));

  MouseEventTest tests[] = {
      // Send a mouse down event over child1.
      {std::make_unique<ui::MouseEvent>(
           ui::ET_MOUSE_PRESSED, gfx::Point(15, 15), gfx::Point(15, 15),
           base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
           ui::EF_LEFT_MOUSE_BUTTON),
       child1.get(), gfx::Point(15, 15), gfx::Point(5, 5), nullptr,
       gfx::Point(), gfx::Point()},
      // Send mouse wheel over child2, should go to child1 as it has capture.
      {std::make_unique<ui::MouseWheelEvent>(
           gfx::Vector2d(1, 0), gfx::Point(53, 54), gfx::Point(53, 54),
           base::TimeTicks(), ui::EF_NONE, ui::EF_NONE),
       child1.get(), gfx::Point(53, 54), gfx::Point(43, 44), nullptr,
       gfx::Point(), gfx::Point()},
  };
  RunMouseEventTests(event_dispatcher(), test_event_dispatcher_delegate(),
                     tests, arraysize(tests));
}

// Tests that when explicit capture has been set that all events go to the
// designated window, and that when capture is cleared, events find the
// appropriate target window.
TEST_P(EventProcessorTest, SetExplicitCapture) {
  ServerWindow* root = root_window();
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EventProcessor* dispatcher = event_dispatcher();

  {
    // Send all pointer events to the child.
    dispatcher->SetCaptureWindow(child.get(), kClientAreaId);

    // The mouse press should go to the child even though its outside its
    // bounds.
    const ui::PointerEvent left_press_event(ui::MouseEvent(
        ui::ET_MOUSE_PRESSED, gfx::Point(5, 5), gfx::Point(5, 5),
        base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
    DispatchEvent(dispatcher, left_press_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);

    // Events should target child.
    std::unique_ptr<DispatchedEventDetails> details =
        event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();

    ASSERT_TRUE(details);
    ASSERT_EQ(child.get(), details->window);
    EXPECT_TRUE(details->IsClientArea());
    EXPECT_TRUE(IsMouseButtonDown());

    // The mouse down state should update while capture is set.
    const ui::PointerEvent right_press_event(ui::MouseEvent(
        ui::ET_MOUSE_PRESSED, gfx::Point(5, 5), gfx::Point(5, 5),
        base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON,
        ui::EF_RIGHT_MOUSE_BUTTON));
    DispatchEvent(dispatcher, right_press_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);
    details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
    EXPECT_TRUE(IsMouseButtonDown());

    // One button released should not clear mouse down
    const ui::PointerEvent left_release_event(ui::MouseEvent(
        ui::ET_MOUSE_RELEASED, gfx::Point(5, 5), gfx::Point(5, 5),
        base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON,
        ui::EF_LEFT_MOUSE_BUTTON));
    DispatchEvent(dispatcher, left_release_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);
    details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
    EXPECT_TRUE(IsMouseButtonDown());

    // Touch Event while mouse is down should not affect state.
    const ui::PointerEvent touch_event(ui::TouchEvent(
        ui::ET_TOUCH_PRESSED, gfx::Point(15, 15), base::TimeTicks(),
        ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 2)));
    DispatchEvent(dispatcher, touch_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);
    details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
    EXPECT_TRUE(IsMouseButtonDown());

    // Move event should not affect down
    const ui::PointerEvent move_event(
        ui::MouseEvent(ui::ET_MOUSE_MOVED, gfx::Point(15, 5), gfx::Point(15, 5),
                       base::TimeTicks(), ui::EF_RIGHT_MOUSE_BUTTON,
                       ui::EF_RIGHT_MOUSE_BUTTON));
    DispatchEvent(dispatcher, move_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);
    details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
    EXPECT_TRUE(IsMouseButtonDown());

    // All mouse buttons up should clear mouse down.
    const ui::PointerEvent right_release_event(
        ui::MouseEvent(ui::ET_MOUSE_RELEASED, gfx::Point(5, 5),
                       gfx::Point(5, 5), base::TimeTicks(),
                       ui::EF_RIGHT_MOUSE_BUTTON, ui::EF_RIGHT_MOUSE_BUTTON));
    DispatchEvent(dispatcher, right_release_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);
    details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
    EXPECT_FALSE(IsMouseButtonDown());
  }

  {
    // Releasing capture and sending the same event will go to the root.
    dispatcher->SetCaptureWindow(nullptr, kClientAreaId);
    const ui::PointerEvent press_event(ui::MouseEvent(
        ui::ET_MOUSE_PRESSED, gfx::Point(5, 5), gfx::Point(5, 5),
        base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
    DispatchEvent(dispatcher, press_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);

    // Events should target the root.
    std::unique_ptr<DispatchedEventDetails> details =
        event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();

    ASSERT_TRUE(details);
    ASSERT_EQ(root, details->window);
  }
}

// This test verifies that explicit capture overrides and resets implicit
// capture.
TEST_P(EventProcessorTest, ExplicitCaptureOverridesImplicitCapture) {
  ServerWindow* root = root_window();
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EventProcessor* dispatcher = event_dispatcher();

  // Run some implicit capture tests.
  MouseEventTest tests[] = {
      // Send a mouse down event over child with a left mouse button
      {std::make_unique<ui::MouseEvent>(
           ui::ET_MOUSE_PRESSED, gfx::Point(20, 25), gfx::Point(20, 25),
           base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
           ui::EF_LEFT_MOUSE_BUTTON),
       child.get(), gfx::Point(20, 25), gfx::Point(10, 15)},
      // Capture should be activated. Let's send a mouse move outside the bounds
      // of the child and press the right mouse button too.
      {std::make_unique<ui::MouseEvent>(
           ui::ET_MOUSE_MOVED, gfx::Point(50, 50), gfx::Point(50, 50),
           base::TimeTicks(),
           ui::EF_LEFT_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON, 0),
       child.get(), gfx::Point(50, 50), gfx::Point(40, 40)},
      // Release the left mouse button and verify that the mouse up event goes
      // to the child.
      {std::make_unique<ui::MouseEvent>(
           ui::ET_MOUSE_RELEASED, gfx::Point(50, 50), gfx::Point(50, 50),
           base::TimeTicks(),
           ui::EF_LEFT_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON,
           ui::EF_RIGHT_MOUSE_BUTTON),
       child.get(), gfx::Point(50, 50), gfx::Point(40, 40)},
      // A mouse move at (50, 50) should still go to the child.
      {std::make_unique<ui::MouseEvent>(ui::ET_MOUSE_MOVED, gfx::Point(50, 50),
                                        gfx::Point(50, 50), base::TimeTicks(),
                                        ui::EF_LEFT_MOUSE_BUTTON, 0),
       child.get(), gfx::Point(50, 50), gfx::Point(40, 40)},

  };
  RunMouseEventTests(dispatcher, event_dispatcher_delegate, tests,
                     arraysize(tests));

  // Add a second pointer target to the child.
  {
    const ui::PointerEvent touch_event(ui::TouchEvent(
        ui::ET_TOUCH_PRESSED, gfx::Point(12, 13), base::TimeTicks(),
        ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
    DispatchEvent(dispatcher, touch_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);
  }

  std::unique_ptr<DispatchedEventDetails> details =
      event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  EXPECT_EQ(child.get(), details->window);

  // Verify that no window has explicit capture and hence we did indeed do
  // implicit capture.
  ASSERT_EQ(nullptr, dispatcher->capture_window());

  // Give the root window explicit capture and verify input events over the
  // child go to the root instead.
  dispatcher->SetCaptureWindow(root, kNonclientAreaId);

  // The implicit target should receive a cancel event for each pointer target.
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  EXPECT_TRUE(event_dispatcher_delegate->has_queued_events());
  EXPECT_EQ(child.get(), details->window);
  EXPECT_EQ(ui::ET_POINTER_CANCELLED, details->event->type());

  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  EXPECT_EQ(child.get(), details->window);
  EXPECT_EQ(ui::ET_POINTER_EXITED, details->event->type());

  const ui::PointerEvent press_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(15, 15), gfx::Point(15, 15),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(dispatcher, press_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // Events should target the root.
  details = event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  ASSERT_EQ(root, details->window);
  ASSERT_TRUE(details->IsNonclientArea());
}

// Tests that setting capture does delete active pointer targets for the capture
// window.
TEST_P(EventProcessorTest, CaptureUpdatesActivePointerTargets) {
  ServerWindow* root = root_window();
  root->SetBounds(gfx::Rect(0, 0, 100, 100));

  EventProcessor* dispatcher = event_dispatcher();
  {
    const ui::PointerEvent press_event(ui::MouseEvent(
        ui::ET_MOUSE_PRESSED, gfx::Point(5, 5), gfx::Point(5, 5),
        base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
    DispatchEvent(dispatcher, press_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);

    std::unique_ptr<DispatchedEventDetails> details =
        test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
    ASSERT_TRUE(details);
    ASSERT_EQ(root, details->window);
  }
  {
    const ui::PointerEvent touch_event(ui::TouchEvent(
        ui::ET_TOUCH_PRESSED, gfx::Point(12, 13), base::TimeTicks(),
        ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
    DispatchEvent(dispatcher, touch_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);
  }

  ASSERT_TRUE(AreAnyPointersDown());
  ASSERT_TRUE(IsWindowPointerTarget(root));
  EXPECT_EQ(2, NumberPointerTargetsForWindow(root));

  // Setting the capture should clear the implicit pointers for the specified
  // window.
  dispatcher->SetCaptureWindow(root, kNonclientAreaId);
  EXPECT_FALSE(AreAnyPointersDown());
  EXPECT_FALSE(IsWindowPointerTarget(root));
}

// Tests that when explicit capture is changed, that the previous window with
// capture is no longer being observed.
TEST_P(EventProcessorTest, UpdatingCaptureStopsObservingPreviousCapture) {
  std::unique_ptr<ServerWindow> child1 =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> child2 =
      CreateChildWindow(viz::FrameSinkId(1, 4));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child1->SetBounds(gfx::Rect(10, 10, 20, 20));
  child2->SetBounds(gfx::Rect(50, 51, 11, 12));

  EventProcessor* dispatcher = event_dispatcher();
  ASSERT_FALSE(AreAnyPointersDown());
  ASSERT_FALSE(IsWindowPointerTarget(child1.get()));
  ASSERT_FALSE(IsWindowPointerTarget(child2.get()));
  dispatcher->SetCaptureWindow(child1.get(), kClientAreaId);
  dispatcher->SetCaptureWindow(child2.get(), kClientAreaId);
  EXPECT_EQ(child1.get(),
            test_event_dispatcher_delegate()->lost_capture_window());

  EXPECT_FALSE(
      EventProcessorTestApi(dispatcher).IsObservingWindow(child1.get()));
}

// Tests that destroying a window with explicit capture clears the capture
// state.
TEST_P(EventProcessorTest, DestroyingCaptureWindowRemovesExplicitCapture) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  EventProcessor* dispatcher = event_dispatcher();
  dispatcher->SetCaptureWindow(child.get(), kClientAreaId);
  EXPECT_EQ(child.get(), dispatcher->capture_window());

  ServerWindow* lost_capture_window = child.get();
  child.reset();
  EXPECT_EQ(nullptr, dispatcher->capture_window());
  EXPECT_EQ(lost_capture_window,
            test_event_dispatcher_delegate()->lost_capture_window());
}

// Tests that when |client_id| is set for a window performing capture, that this
// preference is used regardless of whether an event targets the client region.
TEST_P(EventProcessorTest, CaptureInNonClientAreaOverridesActualPoint) {
  ServerWindow* root = root_window();
  root->SetBounds(gfx::Rect(0, 0, 100, 100));

  root->SetClientArea(gfx::Insets(5, 5, 5, 5), std::vector<gfx::Rect>());
  EventProcessor* dispatcher = event_dispatcher();
  dispatcher->SetCaptureWindow(root, kNonclientAreaId);

  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  // Press in the client area, it should be marked as non client.
  const ui::PointerEvent press_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(6, 6), gfx::Point(6, 6),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), press_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // Events should target child and be in the client area.
  std::unique_ptr<DispatchedEventDetails> details =
      event_dispatcher_delegate->GetAndAdvanceDispatchedEventDetails();
  EXPECT_FALSE(event_dispatcher_delegate->has_queued_events());
  ASSERT_EQ(root, details->window);
  EXPECT_TRUE(details->IsNonclientArea());
}

TEST_P(EventProcessorTest, ProcessPointerEvents) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  {
    const ui::PointerEvent pointer_event(ui::MouseEvent(
        ui::ET_MOUSE_PRESSED, gfx::Point(20, 25), gfx::Point(20, 25),
        base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
    DispatchEvent(event_dispatcher(), pointer_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);

    std::unique_ptr<DispatchedEventDetails> details =
        test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
    ASSERT_TRUE(details);
    ASSERT_EQ(child.get(), details->window);

    ASSERT_TRUE(details->event);
    ASSERT_TRUE(details->event->IsPointerEvent());

    ui::PointerEvent* dispatched_event = details->event->AsPointerEvent();
    EXPECT_EQ(gfx::Point(20, 25), dispatched_event->root_location());
    EXPECT_EQ(gfx::Point(10, 15), dispatched_event->location());
  }

  {
    const int touch_id = 3;
    const ui::PointerEvent pointer_event(ui::TouchEvent(
        ui::ET_TOUCH_RELEASED, gfx::Point(25, 20), base::TimeTicks(),
        ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH,
                           touch_id)));
    DispatchEvent(event_dispatcher(), pointer_event,
                  EventProcessor::AcceleratorMatchPhase::ANY);

    std::unique_ptr<DispatchedEventDetails> details =
        test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
    ASSERT_TRUE(details);
    ASSERT_EQ(child.get(), details->window);

    ASSERT_TRUE(details->event);
    ASSERT_TRUE(details->event->IsPointerEvent());

    ui::PointerEvent* dispatched_event = details->event->AsPointerEvent();
    EXPECT_EQ(gfx::Point(25, 20), dispatched_event->root_location());
    EXPECT_EQ(gfx::Point(15, 10), dispatched_event->location());
    EXPECT_EQ(touch_id, dispatched_event->pointer_details().id);
  }
}

TEST_P(EventProcessorTest, ResetClearsPointerDown) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  // Send event that is over child.
  const ui::PointerEvent ui_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(20, 25), gfx::Point(20, 25),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), ui_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  ASSERT_EQ(child.get(), details->window);

  EXPECT_TRUE(AreAnyPointersDown());

  event_dispatcher()->Reset();
  EXPECT_FALSE(test_event_dispatcher_delegate()->has_queued_events());
  EXPECT_FALSE(AreAnyPointersDown());
}

TEST_P(EventProcessorTest, ResetClearsCapture) {
  ServerWindow* root = root_window();
  root->SetBounds(gfx::Rect(0, 0, 100, 100));

  root->SetClientArea(gfx::Insets(5, 5, 5, 5), std::vector<gfx::Rect>());
  EventProcessor* dispatcher = event_dispatcher();
  dispatcher->SetCaptureWindow(root, kNonclientAreaId);

  event_dispatcher()->Reset();
  EXPECT_FALSE(test_event_dispatcher_delegate()->has_queued_events());
  EXPECT_EQ(nullptr, event_dispatcher()->capture_window());
}

// Tests that events on a parent of a modal window are blocked.
TEST_P(EventProcessorTest, ModalWindowEventOnModalParent) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 5));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));

  w1->AddTransientWindow(w2.get());
  w2->SetModalType(MODAL_TYPE_WINDOW);

  // Send event that is over |w1|.
  const ui::PointerEvent mouse_pressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(15, 15), gfx::Point(15, 15),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_pressed,
                EventProcessor::AcceleratorMatchPhase::ANY);
  // As |w2| is modal and the event is over |w1|, no events should be queued,
  // and the delegate should be informed of this.
  ASSERT_FALSE(test_event_dispatcher_delegate()->has_queued_events());
  EXPECT_EQ(w2.get(),
            test_event_dispatcher_delegate()->window_that_blocked_event());
  test_event_dispatcher_delegate()->ClearWindowThatBlockedEvent();

  const ui::PointerEvent mouse_released(ui::MouseEvent(
      ui::ET_MOUSE_RELEASED, gfx::Point(15, 15), gfx::Point(15, 15),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_released,
                EventProcessor::AcceleratorMatchPhase::ANY);
  ASSERT_FALSE(test_event_dispatcher_delegate()->has_queued_events());
  EXPECT_FALSE(test_event_dispatcher_delegate()->window_that_blocked_event());

  // Send event over |w2|.
  const ui::PointerEvent mouse_pressed2(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(55, 15), gfx::Point(55, 15),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_pressed2,
                EventProcessor::AcceleratorMatchPhase::ANY);
  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  EXPECT_EQ(w2.get(), details->window);

  ASSERT_TRUE(details->event);
  ASSERT_TRUE(details->event->IsPointerEvent());

  ui::PointerEvent* dispatched_event = details->event->AsPointerEvent();
  EXPECT_EQ(gfx::Point(55, 15), dispatched_event->root_location());
  EXPECT_EQ(gfx::Point(5, 5), dispatched_event->location());
  EXPECT_FALSE(test_event_dispatcher_delegate()->window_that_blocked_event());
}

// Tests that events on a modal child target the modal child itself.
TEST_P(EventProcessorTest, ModalWindowEventOnModalChild) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 5));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));

  w1->AddTransientWindow(w2.get());
  w2->SetModalType(MODAL_TYPE_WINDOW);

  // Send event that is over |w2|.
  const ui::PointerEvent mouse_pressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(55, 15), gfx::Point(55, 15),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_pressed,
                EventProcessor::AcceleratorMatchPhase::ANY);

  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  EXPECT_EQ(w2.get(), details->window);
  EXPECT_TRUE(details->IsClientArea());

  ASSERT_TRUE(details->event);
  ASSERT_TRUE(details->event->IsPointerEvent());

  ui::PointerEvent* dispatched_event = details->event->AsPointerEvent();
  EXPECT_EQ(gfx::Point(55, 15), dispatched_event->root_location());
  EXPECT_EQ(gfx::Point(5, 5), dispatched_event->location());
}

// Tests that events on an unrelated window are not affected by the modal
// window.
TEST_P(EventProcessorTest, ModalWindowEventOnUnrelatedWindow) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 5));
  std::unique_ptr<ServerWindow> w3 = CreateChildWindow(viz::FrameSinkId(1, 6));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));
  w3->SetBounds(gfx::Rect(70, 10, 10, 10));

  w1->AddTransientWindow(w2.get());
  w2->SetModalType(MODAL_TYPE_WINDOW);

  // Send event that is over |w3|.
  const ui::PointerEvent mouse_pressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(75, 15), gfx::Point(75, 15),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_pressed,
                EventProcessor::AcceleratorMatchPhase::ANY);

  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  EXPECT_EQ(w3.get(), details->window);
  EXPECT_TRUE(details->IsClientArea());

  ASSERT_TRUE(details->event);
  ASSERT_TRUE(details->event->IsPointerEvent());

  ui::PointerEvent* dispatched_event = details->event->AsPointerEvent();
  EXPECT_EQ(gfx::Point(75, 15), dispatched_event->root_location());
  EXPECT_EQ(gfx::Point(5, 5), dispatched_event->location());
}

// Tests that events on a descendant of a modal parent target the modal child.
TEST_P(EventProcessorTest, ModalWindowEventOnDescendantOfModalParent) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> w11 =
      CreateChildWindowWithParent(viz::FrameSinkId(1, 4), w1.get());
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 5));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w11->SetBounds(gfx::Rect(10, 10, 10, 10));
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));

  w1->AddTransientWindow(w2.get());
  w2->SetModalType(MODAL_TYPE_WINDOW);

  // Send event that is over |w11|.
  const ui::PointerEvent mouse_pressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(25, 25), gfx::Point(25, 25),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_pressed,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // The event is targeted at |w11|, but is blocked by the modal window |w2|.
  ASSERT_TRUE(test_event_dispatcher_delegate()->last_event_target_not_found());
  ASSERT_FALSE(test_event_dispatcher_delegate()->has_queued_events());
}

// Same as ModalWindowEventOnDescendantOfModalParent but with fallback to root
// enabled.
TEST_P(EventProcessorTest,
       ModalWindowEventOnDescendantOfModalParentWithFallback) {
  test_event_dispatcher_delegate()->EnableFallbackToRoot();

  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> w11 =
      CreateChildWindowWithParent(viz::FrameSinkId(1, 4), w1.get());
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 5));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w11->SetBounds(gfx::Rect(10, 10, 10, 10));
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));

  w1->AddTransientWindow(w2.get());
  w2->SetModalType(MODAL_TYPE_WINDOW);

  // Send event that is over |w11|.
  const ui::PointerEvent mouse_pressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(25, 25), gfx::Point(25, 25),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_pressed,
                EventProcessor::AcceleratorMatchPhase::ANY);

  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  EXPECT_EQ(root_window(), details->window);
  EXPECT_TRUE(details->IsNonclientArea());

  ASSERT_TRUE(details->event);
  ASSERT_TRUE(details->event->IsPointerEvent());

  ui::PointerEvent* dispatched_event = details->event->AsPointerEvent();
  EXPECT_EQ(gfx::Point(25, 25), dispatched_event->root_location());
  EXPECT_EQ(gfx::Point(25, 25), dispatched_event->location());
}

// Tests that events on a system modal window target the modal window itself.
TEST_P(EventProcessorTest, ModalWindowEventOnSystemModal) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w1->SetModalType(MODAL_TYPE_SYSTEM);

  // Send event that is over |w1|.
  const ui::PointerEvent mouse_pressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(15, 15), gfx::Point(15, 15),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_pressed,
                EventProcessor::AcceleratorMatchPhase::ANY);

  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  EXPECT_EQ(w1.get(), details->window);
  EXPECT_TRUE(details->IsClientArea());

  ASSERT_TRUE(details->event);
  ASSERT_TRUE(details->event->IsPointerEvent());

  ui::PointerEvent* dispatched_event = details->event->AsPointerEvent();
  EXPECT_EQ(gfx::Point(15, 15), dispatched_event->root_location());
  EXPECT_EQ(gfx::Point(5, 5), dispatched_event->location());
}

// Tests that events outside of system modal window target the modal window.
TEST_P(EventProcessorTest, ModalWindowEventOutsideSystemModal) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w1->SetModalType(MODAL_TYPE_SYSTEM);
  event_dispatcher()->AddSystemModalWindow(w1.get());

  // Send event that is outside of |w1|.
  const ui::PointerEvent mouse_pressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(45, 15), gfx::Point(45, 15),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_pressed,
                EventProcessor::AcceleratorMatchPhase::ANY);

  ASSERT_TRUE(test_event_dispatcher_delegate()->last_event_target_not_found());
  ASSERT_FALSE(test_event_dispatcher_delegate()->has_queued_events());
  EXPECT_EQ(w1.get(),
            test_event_dispatcher_delegate()->window_that_blocked_event());
}

// Variant of ModalWindowEventOutsideSystemModal with
// EnableFallbackToRoot().
TEST_P(EventProcessorTest, ModalWindowEventOutsideSystemModalWithFallback) {
  test_event_dispatcher_delegate()->EnableFallbackToRoot();

  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w1->SetModalType(MODAL_TYPE_SYSTEM);
  event_dispatcher()->AddSystemModalWindow(w1.get());

  // Send event that is outside of |w1|.
  const ui::PointerEvent mouse_pressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(45, 15), gfx::Point(45, 15),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_pressed,
                EventProcessor::AcceleratorMatchPhase::ANY);

  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  EXPECT_EQ(root_window(), details->window);
  EXPECT_TRUE(details->IsNonclientArea());

  ASSERT_TRUE(details->event);
  ASSERT_TRUE(details->event->IsPointerEvent());

  ui::PointerEvent* dispatched_event = details->event->AsPointerEvent();
  EXPECT_EQ(gfx::Point(45, 15), dispatched_event->root_location());
  EXPECT_EQ(gfx::Point(45, 15), dispatched_event->location());
}

// Tests events on a sub-window of system modal window target the window itself.
TEST_P(EventProcessorTest, ModalWindowEventSubWindowSystemModal) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  w1->SetModalType(MODAL_TYPE_SYSTEM);
  event_dispatcher()->AddSystemModalWindow(w1.get());

  std::unique_ptr<ServerWindow> w2 =
      CreateChildWindowWithParent(viz::FrameSinkId(1, 4), w1.get());
  std::unique_ptr<ServerWindow> w3 = CreateChildWindow(viz::FrameSinkId(1, 5));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w2->SetBounds(gfx::Rect(10, 10, 10, 10));
  w3->SetBounds(gfx::Rect(50, 10, 10, 10));

  struct {
    gfx::Point location;
    ServerWindow* expected_target;
  } kTouchData[] = {
      // Touch on |w1| should go to |w1|.
      {gfx::Point(11, 11), w1.get()},
      // Touch on |w2| should go to |w2|.
      {gfx::Point(25, 25), w2.get()},
      // Touch on |w3| should go to |w1|.
      {gfx::Point(11, 31), w1.get()},
  };

  for (size_t i = 0; i < arraysize(kTouchData); i++) {
    // Send touch press and check that the expected target receives it.
    DispatchEvent(
        event_dispatcher(),
        ui::PointerEvent(ui::TouchEvent(
            ui::ET_TOUCH_PRESSED, kTouchData[i].location, base::TimeTicks(),
            ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0))),
        EventProcessor::AcceleratorMatchPhase::ANY);
    std::unique_ptr<DispatchedEventDetails> details =
        test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
    ASSERT_TRUE(details) << " details is nullptr " << i;
    EXPECT_EQ(kTouchData[i].expected_target, details->window) << i;

    // Release touch.
    DispatchEvent(
        event_dispatcher(),
        ui::PointerEvent(ui::TouchEvent(
            ui::ET_TOUCH_RELEASED, kTouchData[i].location, base::TimeTicks(),
            ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0))),
        EventProcessor::AcceleratorMatchPhase::ANY);
    test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  }
}

// Tests that setting capture to a descendant of a modal parent fails.
TEST_P(EventProcessorTest, ModalWindowSetCaptureDescendantOfModalParent) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> w11 =
      CreateChildWindowWithParent(viz::FrameSinkId(1, 4), w1.get());
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 5));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w11->SetBounds(gfx::Rect(10, 10, 10, 10));
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));

  w1->AddTransientWindow(w2.get());
  w2->SetModalType(MODAL_TYPE_WINDOW);

  EXPECT_FALSE(event_dispatcher()->SetCaptureWindow(w11.get(), kClientAreaId));
  EXPECT_EQ(nullptr, event_dispatcher()->capture_window());
}

// Tests that setting capture to a window unrelated to a modal parent works.
TEST_P(EventProcessorTest, ModalWindowSetCaptureUnrelatedWindow) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 4));
  std::unique_ptr<ServerWindow> w3 = CreateChildWindow(viz::FrameSinkId(1, 5));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));
  w3->SetBounds(gfx::Rect(70, 10, 10, 10));

  w1->AddTransientWindow(w2.get());
  w2->SetModalType(MODAL_TYPE_WINDOW);

  EXPECT_TRUE(event_dispatcher()->SetCaptureWindow(w3.get(), kClientAreaId));
  EXPECT_EQ(w3.get(), event_dispatcher()->capture_window());
}

// Tests that setting capture fails when there is a system modal window.
TEST_P(EventProcessorTest, ModalWindowSystemSetCapture) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 4));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));

  w2->SetModalType(MODAL_TYPE_SYSTEM);
  event_dispatcher()->AddSystemModalWindow(w2.get());

  EXPECT_FALSE(event_dispatcher()->SetCaptureWindow(w1.get(), kClientAreaId));
  EXPECT_EQ(nullptr, event_dispatcher()->capture_window());
}

// Tests having multiple system modal windows.
TEST_P(EventProcessorTest, ModalWindowMultipleSystemModals) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 4));
  std::unique_ptr<ServerWindow> w3 = CreateChildWindow(viz::FrameSinkId(1, 5));

  w2->SetVisible(false);

  // In the beginning, there should be no active system modal window.
  EXPECT_EQ(nullptr, GetActiveSystemModalWindow());

  // Add a visible system modal window. It should become the active one.
  w1->SetModalType(MODAL_TYPE_SYSTEM);
  event_dispatcher()->AddSystemModalWindow(w1.get());
  EXPECT_EQ(w1.get(), GetActiveSystemModalWindow());

  // Add an invisible system modal window. It should not change the active one.
  w2->SetModalType(MODAL_TYPE_SYSTEM);
  event_dispatcher()->AddSystemModalWindow(w2.get());
  EXPECT_EQ(w1.get(), GetActiveSystemModalWindow());

  // Add another visible system modal window. It should become the active one.
  w3->SetModalType(MODAL_TYPE_SYSTEM);
  event_dispatcher()->AddSystemModalWindow(w3.get());
  EXPECT_EQ(w3.get(), GetActiveSystemModalWindow());

  // Make an existing system modal window visible. It should become the active
  // one.
  w2->SetVisible(true);
  EXPECT_EQ(w2.get(), GetActiveSystemModalWindow());

  // Remove the active system modal window. Next one should become active.
  w2.reset();
  EXPECT_EQ(w3.get(), GetActiveSystemModalWindow());

  // Remove an inactive system modal window. It should not change the active
  // one.
  w1.reset();
  EXPECT_EQ(w3.get(), GetActiveSystemModalWindow());

  // Remove the last remaining system modal window. There should be no active
  // one anymore.
  w3.reset();
  EXPECT_EQ(nullptr, GetActiveSystemModalWindow());
}

TEST_P(EventProcessorTest, CaptureNotResetOnParentChange) {
  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  w1->set_event_targeting_policy(mojom::EventTargetingPolicy::DESCENDANTS_ONLY);
  std::unique_ptr<ServerWindow> w11 =
      CreateChildWindowWithParent(viz::FrameSinkId(1, 4), w1.get());
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 5));
  w2->set_event_targeting_policy(mojom::EventTargetingPolicy::DESCENDANTS_ONLY);

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  w1->SetBounds(gfx::Rect(0, 0, 100, 100));
  w11->SetBounds(gfx::Rect(10, 10, 10, 10));
  w2->SetBounds(gfx::Rect(0, 0, 100, 100));

  // Send event that is over |w11|.
  const ui::PointerEvent mouse_pressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(15, 15), gfx::Point(15, 15),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), mouse_pressed,
                EventProcessor::AcceleratorMatchPhase::ANY);
  event_dispatcher()->SetCaptureWindow(w11.get(), kClientAreaId);

  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  EXPECT_EQ(w11.get(), details->window);
  EXPECT_TRUE(details->IsClientArea());

  // Move |w11| to |w2| and verify the mouse is still down, and |w11| has
  // capture.
  w2->Add(w11.get());
  EXPECT_TRUE(IsMouseButtonDown());
  EXPECT_EQ(w11.get(),
            EventProcessorTestApi(event_dispatcher()).capture_window());
}

TEST_P(EventProcessorTest, ChangeCaptureFromClientToNonclient) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  event_dispatcher()->SetCaptureWindow(child.get(), kNonclientAreaId);
  EXPECT_EQ(kNonclientAreaId, event_dispatcher()->capture_window_client_id());
  EXPECT_EQ(nullptr, test_event_dispatcher_delegate()->lost_capture_window());
  event_dispatcher()->SetCaptureWindow(child.get(), kClientAreaId);
  // Changing capture from client to non-client should notify the delegate.
  // The delegate can decide if it really wants to forward the event or not.
  EXPECT_EQ(child.get(),
            test_event_dispatcher_delegate()->lost_capture_window());
  EXPECT_EQ(child.get(), event_dispatcher()->capture_window());
  EXPECT_EQ(kClientAreaId, event_dispatcher()->capture_window_client_id());
}

TEST_P(EventProcessorTest, MoveMouseFromNoTargetToValidTarget) {
  ServerWindow* root = root_window();
  root->set_event_targeting_policy(
      mojom::EventTargetingPolicy::DESCENDANTS_ONLY);
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  MouseEventTest tests[] = {
      // Send a mouse down over the root, but not the child. No event should
      // be generated.
      {std::make_unique<ui::MouseEvent>(ui::ET_MOUSE_MOVED, gfx::Point(5, 5),
                                        gfx::Point(5, 5), base::TimeTicks(), 0,
                                        0),
       nullptr, gfx::Point(), gfx::Point(), nullptr, gfx::Point(),
       gfx::Point()},

      // Move into child.
      {std::make_unique<ui::MouseEvent>(ui::ET_MOUSE_MOVED, gfx::Point(12, 12),
                                        gfx::Point(12, 12), base::TimeTicks(),
                                        0, 0),
       child.get(), gfx::Point(12, 12), gfx::Point(2, 2), nullptr, gfx::Point(),
       gfx::Point()}};
  RunMouseEventTests(event_dispatcher(), test_event_dispatcher_delegate(),
                     tests, arraysize(tests));
}

TEST_P(EventProcessorTest, NoTargetToTargetWithMouseDown) {
  ServerWindow* root = root_window();
  root->set_event_targeting_policy(
      mojom::EventTargetingPolicy::DESCENDANTS_ONLY);
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  MouseEventTest tests[] = {
      // Mouse over the root, but not the child. No event should be generated.
      {std::make_unique<ui::MouseEvent>(ui::ET_MOUSE_MOVED, gfx::Point(5, 5),
                                        gfx::Point(5, 5), base::TimeTicks(), 0,
                                        0),
       nullptr, gfx::Point(), gfx::Point(), nullptr, gfx::Point(),
       gfx::Point()},

      // Press in same location, still no target.
      {std::make_unique<ui::MouseEvent>(ui::ET_MOUSE_PRESSED, gfx::Point(5, 5),
                                        gfx::Point(5, 5), base::TimeTicks(),
                                        ui::EF_LEFT_MOUSE_BUTTON,
                                        ui::EF_LEFT_MOUSE_BUTTON),
       nullptr, gfx::Point(), gfx::Point(), nullptr, gfx::Point(),
       gfx::Point()},

      // Move into child, still no target.
      {std::make_unique<ui::MouseEvent>(ui::ET_MOUSE_MOVED, gfx::Point(12, 12),
                                        gfx::Point(12, 12), base::TimeTicks(),
                                        ui::EF_LEFT_MOUSE_BUTTON, 0),
       nullptr, gfx::Point(), gfx::Point(), nullptr, gfx::Point(),
       gfx::Point()}};
  RunMouseEventTests(event_dispatcher(), test_event_dispatcher_delegate(),
                     tests, arraysize(tests));
}

TEST_P(EventProcessorTest, DontSendExitToSameClientWhenCaptureChanges) {
  ServerWindow* root = root_window();
  root->set_event_targeting_policy(
      mojom::EventTargetingPolicy::DESCENDANTS_ONLY);
  std::unique_ptr<ServerWindow> c1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> c2 = CreateChildWindow(viz::FrameSinkId(1, 4));

  root->SetBounds(gfx::Rect(0, 0, 100, 100));
  c1->SetBounds(gfx::Rect(10, 10, 20, 20));
  c2->SetBounds(gfx::Rect(15, 15, 20, 20));

  MouseEventTest tests[] = {
      // Mouse over |c2|.
      {std::make_unique<ui::MouseEvent>(ui::ET_MOUSE_MOVED, gfx::Point(16, 16),
                                        gfx::Point(16, 16), base::TimeTicks(),
                                        0, 0),
       c2.get(), gfx::Point(16, 16), gfx::Point(1, 1), nullptr, gfx::Point(),
       gfx::Point()},

      // Press in same location.
      {std::make_unique<ui::MouseEvent>(
           ui::ET_MOUSE_PRESSED, gfx::Point(16, 16), gfx::Point(16, 16),
           base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON,
           ui::EF_LEFT_MOUSE_BUTTON),
       c2.get(), gfx::Point(16, 16), gfx::Point(1, 1), nullptr, gfx::Point(),
       gfx::Point()}};
  RunMouseEventTests(event_dispatcher(), test_event_dispatcher_delegate(),
                     tests, arraysize(tests));

  // Set capture on |c1|. No events should be sent as |c1| is in the same
  // client.
  event_dispatcher()->SetCaptureWindow(c1.get(), kClientAreaId);
  EXPECT_FALSE(test_event_dispatcher_delegate()->has_queued_events());
}

TEST_P(EventProcessorTest, MousePointerClearedOnDestroy) {
  root_window()->set_event_targeting_policy(
      mojom::EventTargetingPolicy::DESCENDANTS_ONLY);
  std::unique_ptr<ServerWindow> c1 = CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  c1->SetBounds(gfx::Rect(10, 10, 20, 20));

  const ui::PointerEvent move_event(
      ui::MouseEvent(ui::ET_MOUSE_MOVED, gfx::Point(15, 15), gfx::Point(15, 15),
                     base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  DispatchEvent(event_dispatcher(), move_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  EXPECT_EQ(c1.get(), event_dispatcher()->mouse_cursor_source_window());
  c1.reset();
  EXPECT_EQ(nullptr, event_dispatcher()->mouse_cursor_source_window());
}

TEST_P(EventProcessorTest, LocationHonorsTransform) {
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  gfx::Transform transform;
  transform.Scale(SkIntToMScalar(2), SkIntToMScalar(2));
  child->SetTransform(transform);

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  // Send event that is over child.
  const ui::PointerEvent ui_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(20, 25), gfx::Point(20, 25),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), ui_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  std::unique_ptr<DispatchedEventDetails> details =
      test_event_dispatcher_delegate()->GetAndAdvanceDispatchedEventDetails();
  ASSERT_TRUE(details);
  ASSERT_EQ(child.get(), details->window);

  ASSERT_TRUE(details->event);
  ASSERT_TRUE(details->event->IsPointerEvent());

  ui::PointerEvent* dispatched_event = details->event->AsPointerEvent();
  EXPECT_EQ(gfx::Point(20, 25), dispatched_event->root_location());
  EXPECT_EQ(gfx::Point(5, 7), dispatched_event->location());
}

TEST_P(EventProcessorTest, MouseMovementsShowCursor) {
  EXPECT_EQ(base::Optional<bool>(),
            test_event_dispatcher_delegate()->last_cursor_visibility());

  const ui::PointerEvent move1(
      ui::MouseEvent(ui::ET_MOUSE_MOVED, gfx::Point(11, 11), gfx::Point(11, 11),
                     base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  DispatchEvent(event_dispatcher(), move1,
                EventProcessor::AcceleratorMatchPhase::ANY);

  EXPECT_EQ(base::Optional<bool>(true),
            test_event_dispatcher_delegate()->last_cursor_visibility());
}

TEST_P(EventProcessorTest, KeyDoesntHideCursorWithNoList) {
  // In the case of mus, we don't send a list to the window server so ensure we
  // don't hide the cursor in this mode.
  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  EXPECT_EQ(base::Optional<bool>(),
            event_dispatcher_delegate->last_cursor_visibility());

  // Set focused window for EventProcessor dispatches key events.
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  event_dispatcher_delegate->SetFocusedWindowFromEventProcessor(child.get());

  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  DispatchEvent(event_dispatcher(), key,
                EventProcessor::AcceleratorMatchPhase::ANY);

  EXPECT_EQ(base::Optional<bool>(),
            event_dispatcher_delegate->last_cursor_visibility());
}

TEST_P(EventProcessorTest, KeyDoesntHideCursorOnMatch) {
  // In the case of mash, we send a list of keys which don't hide the cursor.
  std::vector<ui::mojom::EventMatcherPtr> matchers;
  matchers.push_back(BuildKeyMatcher(ui::mojom::KeyboardCode::A));
  event_dispatcher()->SetKeyEventsThatDontHideCursor(std::move(matchers));

  // Set focused window for EventProcessor dispatches key events.
  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  event_dispatcher_delegate->SetFocusedWindowFromEventProcessor(child.get());

  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  DispatchEvent(event_dispatcher(), key,
                EventProcessor::AcceleratorMatchPhase::ANY);

  EXPECT_EQ(base::Optional<bool>(),
            event_dispatcher_delegate->last_cursor_visibility());
}

TEST_P(EventProcessorTest, KeyHidesCursorOnNoMatch) {
  // In the case of mash, we send a list of keys which don't hide the cursor.
  std::vector<ui::mojom::EventMatcherPtr> matchers;
  matchers.push_back(BuildKeyMatcher(ui::mojom::KeyboardCode::B));
  event_dispatcher()->SetKeyEventsThatDontHideCursor(std::move(matchers));

  // Set focused window for EventProcessor dispatches key events.
  TestEventProcessorDelegate* event_dispatcher_delegate =
      test_event_dispatcher_delegate();
  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  event_dispatcher_delegate->SetFocusedWindowFromEventProcessor(child.get());

  ui::KeyEvent key(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  DispatchEvent(event_dispatcher(), key,
                EventProcessor::AcceleratorMatchPhase::ANY);

  EXPECT_EQ(base::Optional<bool>(false),
            event_dispatcher_delegate->last_cursor_visibility());
}

TEST_P(EventProcessorTest, ChildModal) {
  std::unique_ptr<ServerWindow> modal_parent =
      CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> child_modal_window =
      CreateChildWindow(viz::FrameSinkId(1, 4));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  modal_parent->SetBounds(gfx::Rect(10, 10, 30, 30));
  child_modal_window->SetBounds(gfx::Rect(50, 10, 10, 10));
  modal_parent->AddTransientWindow(child_modal_window.get());
  child_modal_window->SetModalType(MODAL_TYPE_CHILD);
  child_modal_window->SetChildModalParent(modal_parent.get());

  const ui::PointerEvent press_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(20, 25), gfx::Point(20, 25),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), press_event,
                EventProcessor::AcceleratorMatchPhase::ANY);

  // As the event falls over |modal_parent|, but there is a CHILD_MODAL window,
  // the event is blocked and the delegate notified.
  EXPECT_FALSE(test_event_dispatcher_delegate()->has_queued_events());
  EXPECT_EQ(child_modal_window.get(),
            test_event_dispatcher_delegate()->window_that_blocked_event());
}

TEST_P(EventProcessorTest, DontCancelWhenMovedToSeparateDisplay) {
  TestServerWindowDelegate window_delegate2(viz_host_proxy());
  ServerWindow root2(&window_delegate2, viz::FrameSinkId(1, 100));
  root2.set_is_activation_parent(true);
  window_delegate2.set_root_window(&root2);
  root2.SetVisible(true);

  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  event_dispatcher()->SetCaptureWindow(w1.get(), kClientAreaId);
  ASSERT_EQ(w1.get(), event_dispatcher()->capture_window());
  test_event_dispatcher_delegate()->set_root(&root2);
  window_delegate()->AddRootWindow(&root2);
  root2.Add(w1.get());
  EXPECT_EQ(w1.get(), event_dispatcher()->capture_window());
}

// Tests that setting capture to a window unrelated to a modal parent works.
TEST_P(EventProcessorTest, MouseCursorSourceWindowChangesWithSystemModal) {
  BlockingContainers blocking_containers;
  blocking_containers.system_modal_container = root_window();
  event_dispatcher()->modal_window_controller()->SetBlockingContainers(
      {blocking_containers});

  std::unique_ptr<ServerWindow> w1 = CreateChildWindow(viz::FrameSinkId(1, 3));
  std::unique_ptr<ServerWindow> w2 = CreateChildWindow(viz::FrameSinkId(1, 4));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  root_window()->set_is_activation_parent(true);
  w1->SetBounds(gfx::Rect(10, 10, 30, 30));
  w2->SetBounds(gfx::Rect(50, 10, 10, 10));
  w2->SetModalType(MODAL_TYPE_SYSTEM);
  event_dispatcher()->modal_window_controller()->AddSystemModalWindow(w2.get());

  // Move the mouse over |w1|.
  const ui::PointerEvent move(
      ui::MouseEvent(ui::ET_MOUSE_MOVED, gfx::Point(11, 11), gfx::Point(11, 11),
                     base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  DispatchEvent(event_dispatcher(), move,
                EventProcessor::AcceleratorMatchPhase::ANY);

  EXPECT_EQ(nullptr, event_dispatcher()->mouse_cursor_source_window());

  event_dispatcher()->UpdateCursorProviderByLastKnownLocation();
  RunTasks();
  // EventProcessor fallsback to the root incase of invalid window.
  EXPECT_EQ(root_window(), event_dispatcher()->mouse_cursor_source_window());
}

TEST_P(EventProcessorTest, DontQueryWhileMouseIsDown) {
  // This test is easier to write with async event processing.
  if (!is_event_processing_async())
    return;

  std::unique_ptr<ServerWindow> child =
      CreateChildWindow(viz::FrameSinkId(1, 3));

  root_window()->SetBounds(gfx::Rect(0, 0, 100, 100));
  child->SetBounds(gfx::Rect(10, 10, 20, 20));

  const ui::PointerEvent press_event(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, gfx::Point(20, 25), gfx::Point(20, 25),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  DispatchEvent(event_dispatcher(), press_event,
                EventProcessor::AcceleratorMatchPhase::ANY);
  ASSERT_FALSE(event_dispatcher()->IsProcessingEvent());

  const ui::PointerEvent move_event(ui::MouseEvent(
      ui::ET_MOUSE_MOVED, gfx::Point(20, 25), gfx::Point(20, 25),
      base::TimeTicks(), ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  event_dispatcher()->ProcessEvent(move_event,
                                   EventLocationFromEvent(move_event),
                                   EventProcessor::AcceleratorMatchPhase::ANY);
  EXPECT_FALSE(event_dispatcher()->IsProcessingEvent());
}

INSTANTIATE_TEST_CASE_P(/* no prefix */, EventProcessorTest, testing::Bool());

}  // namespace test
}  // namespace ws
}  // namespace ui
