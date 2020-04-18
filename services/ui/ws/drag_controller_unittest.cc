// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/drag_controller.h"

#include <map>
#include <memory>
#include <utility>

#include "base/containers/queue.h"
#include "services/ui/public/interfaces/cursor/cursor.mojom.h"
#include "services/ui/ws/drag_source.h"
#include "services/ui/ws/drag_target_connection.h"
#include "services/ui/ws/ids.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/test_server_window_delegate.h"
#include "services/ui/ws/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/cursor/cursor.h"
#include "ui/events/base_event_utils.h"

namespace ui {
namespace ws {

enum class QueuedType { NONE, ENTER, OVER, LEAVE, DROP };

class DragControllerTest;

// All the classes to represent a window.
class DragTestWindow : public DragTargetConnection {
 public:
  struct DragEvent {
    QueuedType type;
    uint32_t key_state;
    gfx::Point cursor_offset;
    uint32_t effect_bitmask;
    base::Callback<void(uint32_t)> callback;
  };

  DragTestWindow(DragControllerTest* parent,
                 TestServerWindowDelegate* window_delegate,
                 const viz::FrameSinkId& id)
      : parent_(parent),
        window_delegate_(window_delegate),
        window_(window_delegate_, id) {
    window_.SetCanAcceptDrops(true);
  }
  ~DragTestWindow() override;

  TestServerWindowDelegate* delegate() { return window_delegate_; }
  ServerWindow* window() { return &window_; }

  QueuedType queue_response_type() {
    if (queued_callbacks_.empty())
      return QueuedType::NONE;
    return queued_callbacks_.front().type;
  }

  const DragEvent& queue_front() { return queued_callbacks_.front(); }

  size_t queue_size() { return queued_callbacks_.size(); }

  uint32_t times_received_drag_drop_start() {
    return times_received_drag_drop_start_;
  }

  void SetParent(DragTestWindow* p) { p->window_.Add(&window_); }

  void OptOutOfDrag() { window_.SetCanAcceptDrops(false); }

  // Calls the callback at the front of the queue.
  void Respond(bool respond_with_effect) {
    if (queued_callbacks_.size()) {
      if (!queued_callbacks_.front().callback.is_null()) {
        queued_callbacks_.front().callback.Run(
            respond_with_effect ? queued_callbacks_.front().effect_bitmask : 0);
      }

      queued_callbacks_.pop();
    }
  }

  // Overridden from DragTestConnection:
  void PerformOnDragDropStart(
      const base::flat_map<std::string, std::vector<uint8_t>>& mime_data)
      override {
    times_received_drag_drop_start_++;
    mime_data_ = mime_data;
  }

  void PerformOnDragEnter(
      const ServerWindow* window,
      uint32_t key_state,
      const gfx::Point& cursor_offset,
      uint32_t effect_bitmask,
      const base::Callback<void(uint32_t)>& callback) override {
    DCHECK_EQ(window, &window_);
    queued_callbacks_.push({QueuedType::ENTER, key_state, cursor_offset,
                            effect_bitmask, callback});
  }

  void PerformOnDragOver(
      const ServerWindow* window,
      uint32_t key_state,
      const gfx::Point& cursor_offset,
      uint32_t effect_bitmask,
      const base::Callback<void(uint32_t)>& callback) override {
    DCHECK_EQ(window, &window_);
    queued_callbacks_.push(
        {QueuedType::OVER, key_state, cursor_offset, effect_bitmask, callback});
  }

  void PerformOnDragLeave(const ServerWindow* window) override {
    DCHECK_EQ(window, &window_);
    queued_callbacks_.push({QueuedType::LEAVE, 0, gfx::Point(), 0,
                            base::Callback<void(uint32_t)>()});
  }

  void PerformOnCompleteDrop(
      const ServerWindow* window,
      uint32_t key_state,
      const gfx::Point& cursor_offset,
      uint32_t effect_bitmask,
      const base::Callback<void(uint32_t)>& callback) override {
    DCHECK_EQ(window, &window_);
    queued_callbacks_.push(
        {QueuedType::DROP, key_state, cursor_offset, effect_bitmask, callback});
  }

  void PerformOnDragDropDone() override { mime_data_.clear(); }

 private:
  DragControllerTest* parent_;
  TestServerWindowDelegate* window_delegate_;
  ServerWindow window_;
  base::flat_map<std::string, std::vector<uint8_t>> mime_data_;
  uint32_t times_received_drag_drop_start_ = 0;

  base::queue<DragEvent> queued_callbacks_;
};

class DragControllerTest : public testing::Test,
                           public DragCursorUpdater,
                           public DragSource {
 public:
  std::unique_ptr<DragTestWindow> BuildWindow() {
    viz::FrameSinkId id(1, ++window_id_);
    std::unique_ptr<DragTestWindow> p =
        std::make_unique<DragTestWindow>(this, window_delegate_.get(), id);
    connection_by_window_[p->window()] = p.get();
    return p;
  }

  void StartDragOperation(
      DragTestWindow* window,
      uint32_t drag_operations) {
    window->PerformOnDragDropStart(
        base::flat_map<std::string, std::vector<uint8_t>>());
    drag_operation_ = std::make_unique<DragController>(
        this, this, window->window(), window, MouseEvent::kMousePointerId,
        base::flat_map<std::string, std::vector<uint8_t>>(), drag_operations);

    // It would be nice if we could just let the observer method fire, but it
    // fires during the constructor when we haven't assigned the unique_ptr
    // yet.
    cursor_ = drag_operation_->current_cursor().cursor_type();
  }

  void DispatchDrag(DragTestWindow* window,
                    bool mouse_released,
                    uint32_t flags,
                    const gfx::Point& position) {
    ui::PointerEvent event(
        ui::MouseEvent(mouse_released ? ET_MOUSE_RELEASED : ET_MOUSE_PRESSED,
                       position, position, ui::EventTimeForNow(), flags, 0));
    drag_operation_->DispatchPointerEvent(event,
                                          window ? window->window() : nullptr);
  }

  void DispatchDragWithPointer(DragTestWindow* window,
                               int32_t drag_pointer,
                               bool mouse_released,
                               uint32_t flags,
                               const gfx::Point& position) {
    ui::PointerEvent event(
        ET_POINTER_DOWN, position, position, flags, 0,
        PointerDetails(ui::EventPointerType::POINTER_TYPE_MOUSE, drag_pointer),
        base::TimeTicks());
    drag_operation_->DispatchPointerEvent(event,
                                          window ? window->window() : nullptr);
  }

  void OnTestWindowDestroyed(DragTestWindow* test_window) {
    drag_operation_->OnWillDestroyDragTargetConnection(test_window);
    connection_by_window_.erase(test_window->window());
  }

  DragController* drag_operation() const { return drag_operation_.get(); }

  const base::Optional<uint32_t>& drag_completed_action() {
    return drag_completed_action_;
  }
  const base::Optional<bool>& drag_completed_value() {
    return drag_completed_value_;
  }

  ui::CursorType cursor_type() const { return cursor_; }

 private:
  // Overridden from testing::Test:
  void SetUp() override {
    testing::Test::SetUp();

    window_delegate_ = std::make_unique<TestServerWindowDelegate>(
        ws_test_helper_.window_server()->GetVizHostProxy());
    root_window_ = std::make_unique<ServerWindow>(window_delegate_.get(),
                                                  viz::FrameSinkId(1, 2));
    window_delegate_->set_root_window(root_window_.get());
    root_window_->SetVisible(true);
  }

  void TearDown() override {
    drag_operation_.reset();
    root_window_.reset();
    window_delegate_.reset();

    DCHECK(connection_by_window_.empty());

    testing::Test::TearDown();
  }

  // Overridden from DragCursorUpdater:
  void OnDragCursorUpdated() override {
    if (drag_operation_)
      cursor_ = drag_operation_->current_cursor().cursor_type();
  }

  // Overridden from DragControllerSource:
  void OnDragMoved(const gfx::Point& location) override {}

  void OnDragCompleted(bool success, uint32_t action_taken) override {
    drag_completed_action_ = action_taken;
    drag_completed_value_ = success;
  }

  DragTargetConnection* GetDragTargetForWindow(
      const ServerWindow* window) override {
    auto it = connection_by_window_.find(const_cast<ServerWindow*>(window));
    if (it == connection_by_window_.end())
      return nullptr;
    return it->second;
  }

  int window_id_ = 3;

  test::WindowServerTestHelper ws_test_helper_;

  ui::CursorType cursor_;

  std::map<ServerWindow*, DragTargetConnection*> connection_by_window_;

  std::unique_ptr<TestServerWindowDelegate> window_delegate_;
  std::unique_ptr<ServerWindow> root_window_;

  std::unique_ptr<DragController> drag_operation_;

  base::Optional<uint32_t> drag_completed_action_;
  base::Optional<bool> drag_completed_value_;
};

DragTestWindow::~DragTestWindow() {
  parent_->OnTestWindowDestroyed(this);
}

TEST_F(DragControllerTest, SimpleDragDrop) {
  std::unique_ptr<DragTestWindow> window = BuildWindow();
  StartDragOperation(window.get(), ui::mojom::kDropEffectMove);

  EXPECT_EQ(ui::CursorType::kNoDrop, cursor_type());

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window->queue_response_type());
  window->Respond(true);

  // (Even though we're doing a move, the cursor name is COPY.)
  EXPECT_EQ(ui::CursorType::kCopy, cursor_type());

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(2, 2));
  EXPECT_EQ(QueuedType::OVER, window->queue_response_type());
  window->Respond(true);

  DispatchDrag(window.get(), true, 0, gfx::Point(2, 2));
  EXPECT_EQ(QueuedType::DROP, window->queue_response_type());
  window->Respond(true);

  EXPECT_EQ(ui::mojom::kDropEffectMove,
            drag_completed_action().value_or(ui::mojom::kDropEffectNone));
  EXPECT_TRUE(drag_completed_value().value_or(false));
}

TEST_F(DragControllerTest, FailsOnWindowSayingNo) {
  std::unique_ptr<DragTestWindow> window = BuildWindow();
  StartDragOperation(window.get(), ui::mojom::kDropEffectMove);

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window->queue_response_type());
  window->Respond(true);

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(2, 2));
  EXPECT_EQ(QueuedType::OVER, window->queue_response_type());
  window->Respond(true);

  DispatchDrag(window.get(), true, 0, gfx::Point(2, 2));
  EXPECT_EQ(QueuedType::DROP, window->queue_response_type());

  // Unlike SimpleDragDrop, respond with kDropEffectNone, which should make the
  // drag fail.
  window->Respond(false);

  EXPECT_EQ(ui::mojom::kDropEffectNone,
            drag_completed_action().value_or(ui::mojom::kDropEffectCopy));
  EXPECT_FALSE(drag_completed_value().value_or(true));
}

TEST_F(DragControllerTest, OnlyDeliverMimeDataOnce) {
  std::unique_ptr<DragTestWindow> window1 = BuildWindow();
  std::unique_ptr<DragTestWindow> window2 = BuildWindow();

  // The client lib is responsible for sending the data to the window that's
  // the drag source to minimize IPC.
  EXPECT_EQ(0u, window1->times_received_drag_drop_start());
  StartDragOperation(window1.get(), ui::mojom::kDropEffectMove);
  EXPECT_EQ(1u, window1->times_received_drag_drop_start());
  DispatchDrag(window1.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  EXPECT_EQ(1u, window1->times_received_drag_drop_start());
  window1->Respond(true);

  // Window2 doesn't receive the drag data until mouse is over it.
  EXPECT_EQ(0u, window2->times_received_drag_drop_start());
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(2, 2));
  EXPECT_EQ(1u, window2->times_received_drag_drop_start());

  // Moving back to the source window doesn't send an additional start message.
  DispatchDrag(window1.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  EXPECT_EQ(1u, window1->times_received_drag_drop_start());

  // Moving back to window2 doesn't send an additional start message.
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  EXPECT_EQ(1u, window2->times_received_drag_drop_start());
}

TEST_F(DragControllerTest, DeliverMessageToParent) {
  std::unique_ptr<DragTestWindow> window1 = BuildWindow();
  std::unique_ptr<DragTestWindow> window2 = BuildWindow();
  std::unique_ptr<DragTestWindow> window3 = BuildWindow();

  window3->SetParent(window2.get());
  window3->OptOutOfDrag();

  StartDragOperation(window1.get(), ui::mojom::kDropEffectMove);

  // Dispatching a drag to window3 (which has can accept drags off) redirects
  // to window2, which is its parent.
  DispatchDrag(window3.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  EXPECT_EQ(1u, window2->times_received_drag_drop_start());
}

TEST_F(DragControllerTest, FailWhenDropOverNoWindow) {
  std::unique_ptr<DragTestWindow> window = BuildWindow();
  StartDragOperation(window.get(), ui::mojom::kDropEffectMove);

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window->queue_response_type());
  window->Respond(true);

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(2, 2));
  EXPECT_EQ(QueuedType::OVER, window->queue_response_type());
  window->Respond(true);

  DispatchDrag(nullptr, true, 0, gfx::Point(2, 2));
  // Moving outside of |window| should result in |window| getting a leave.
  EXPECT_EQ(QueuedType::LEAVE, window->queue_response_type());
  window->Respond(true);

  EXPECT_FALSE(drag_completed_value().value_or(true));
}

TEST_F(DragControllerTest, EnterLeaveWhenMovingBetweenTwoWindows) {
  std::unique_ptr<DragTestWindow> window1 = BuildWindow();
  std::unique_ptr<DragTestWindow> window2 = BuildWindow();
  StartDragOperation(window1.get(), ui::mojom::kDropEffectMove);

  DispatchDrag(window1.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window1->queue_response_type());
  window1->Respond(true);

  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(2, 2));
  EXPECT_EQ(QueuedType::ENTER, window2->queue_response_type());
  EXPECT_EQ(QueuedType::LEAVE, window1->queue_response_type());
  window1->Respond(true);
  window2->Respond(true);
}

TEST_F(DragControllerTest, DeadWindowDoesntBlock) {
  std::unique_ptr<DragTestWindow> window1 = BuildWindow();
  std::unique_ptr<DragTestWindow> window2 = BuildWindow();
  StartDragOperation(window1.get(), ui::mojom::kDropEffectMove);

  test::DragControllerTestApi api(drag_operation());

  // Simulate a dead window by giving it a few messages, but don't respond to
  // them.
  DispatchDrag(window1.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  DispatchDrag(window1.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(2, 2));
  EXPECT_EQ(1u, window1->queue_size());
  EXPECT_EQ(2u, api.GetSizeOfQueueForWindow(window1->window()));

  // Moving to window2 should dispatch the enter event to it immediately.
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(3, 3));
  EXPECT_EQ(QueuedType::ENTER, window2->queue_response_type());
  EXPECT_EQ(1u, window1->queue_size());
}

TEST_F(DragControllerTest, EnterToOverQueued) {
  std::unique_ptr<DragTestWindow> window = BuildWindow();
  StartDragOperation(window.get(), ui::mojom::kDropEffectMove);

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(1, 1));
  ASSERT_EQ(1u, window->queue_size());
  EXPECT_EQ(QueuedType::ENTER, window->queue_response_type());
  // Don't respond.

  // We don't receive another message since we haven't acknowledged the first.
  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(1, 2));
  ASSERT_EQ(1u, window->queue_size());

  // Responding causes us to receive our next event.
  window->Respond(true);
  ASSERT_EQ(1u, window->queue_size());
  EXPECT_EQ(QueuedType::OVER, window->queue_response_type());
}

TEST_F(DragControllerTest, CoalesceMouseOverEvents) {
  std::unique_ptr<DragTestWindow> window = BuildWindow();
  StartDragOperation(window.get(), ui::mojom::kDropEffectMove);

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window->queue_response_type());

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(1, 2));
  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(2, 2));
  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(2, 3));

  // Responding to the first delivers us the last mouse over event's position.
  window->Respond(true);
  ASSERT_EQ(1u, window->queue_size());
  EXPECT_EQ(QueuedType::OVER, window->queue_response_type());
  EXPECT_EQ(gfx::Point(2, 3), window->queue_front().cursor_offset);

  // There are no queued events because they were coalesced.
  window->Respond(true);
  EXPECT_EQ(0u, window->queue_size());
}

TEST_F(DragControllerTest, RemovePendingMouseOversOnLeave) {
  std::unique_ptr<DragTestWindow> window1 = BuildWindow();
  std::unique_ptr<DragTestWindow> window2 = BuildWindow();
  StartDragOperation(window1.get(), ui::mojom::kDropEffectMove);

  // Enter
  DispatchDrag(window1.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window1->queue_response_type());

  // Over
  DispatchDrag(window1.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));

  // Leave
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));

  // The window finally responds to the enter message; we should not receive
  // any over messages since we didn't respond to the enter message in time.
  window1->Respond(true);
  ASSERT_EQ(1u, window1->queue_size());
  EXPECT_EQ(QueuedType::LEAVE, window1->queue_response_type());
}

TEST_F(DragControllerTest, TargetWindowClosedWhileDrag) {
  std::unique_ptr<DragTestWindow> window1 = BuildWindow();
  std::unique_ptr<DragTestWindow> window2 = BuildWindow();
  StartDragOperation(window1.get(), ui::mojom::kDropEffectMove);

  test::DragControllerTestApi api(drag_operation());

  // Send some events to |window|.
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window2->queue_response_type());
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));

  ServerWindow* server_window = window2->window();

  // Ensure that DragController is waiting for a response from |window|.
  EXPECT_EQ(2u, api.GetSizeOfQueueForWindow(server_window));
  EXPECT_EQ(server_window, api.GetCurrentTarget());

  // Force the destruction of |window.window|.
  window2.reset();

  // DragController doesn't know anything about the server window now.
  EXPECT_EQ(0u, api.GetSizeOfQueueForWindow(server_window));
  EXPECT_EQ(nullptr, api.GetCurrentTarget());

  // But a target window closing out from under us doesn't fail the drag.
  EXPECT_FALSE(drag_completed_value().has_value());
}

TEST_F(DragControllerTest, TargetWindowClosedResetsCursor) {
  std::unique_ptr<DragTestWindow> window1 = BuildWindow();
  std::unique_ptr<DragTestWindow> window2 = BuildWindow();
  StartDragOperation(window1.get(), ui::mojom::kDropEffectMove);
  EXPECT_EQ(ui::CursorType::kNoDrop, cursor_type());

  // Send some events to |window|.
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window2->queue_response_type());
  window2->Respond(true);
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  window2->Respond(true);
  EXPECT_EQ(ui::CursorType::kCopy, cursor_type());

  // Force the destruction of |window.window|.
  window2.reset();

  // The cursor no loner indicates that it can drop on |window2|.
  EXPECT_EQ(ui::CursorType::kNoDrop, cursor_type());
}

TEST_F(DragControllerTest, SourceWindowClosedWhileDrag) {
  std::unique_ptr<DragTestWindow> window1 = BuildWindow();
  std::unique_ptr<DragTestWindow> window2 = BuildWindow();
  StartDragOperation(window1.get(), ui::mojom::kDropEffectMove);

  test::DragControllerTestApi api(drag_operation());

  // Send some events to |windoww|.
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window2->queue_response_type());
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));

  ServerWindow* server_window = window2->window();

  // Ensure that DragController is waiting for a response from |window2|.
  EXPECT_EQ(2u, api.GetSizeOfQueueForWindow(server_window));
  EXPECT_EQ(server_window, api.GetCurrentTarget());

  // Force the destruction of the source window.
  window1.reset();

  // The source window going away fails the drag.
  EXPECT_FALSE(drag_completed_value().value_or(true));
}

TEST_F(DragControllerTest, DontQueueEventsAfterDrop) {
  // The DragController needs to stick around to coordinate the drop, but
  // it should ignore further mouse events during this time.
  std::unique_ptr<DragTestWindow> window = BuildWindow();
  StartDragOperation(window.get(), ui::mojom::kDropEffectMove);

  test::DragControllerTestApi api(drag_operation());

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window->queue_response_type());
  window->Respond(true);

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(2, 2));
  EXPECT_EQ(QueuedType::OVER, window->queue_response_type());
  window->Respond(true);

  DispatchDrag(window.get(), true, 0, gfx::Point(2, 2));
  EXPECT_EQ(QueuedType::DROP, window->queue_response_type());
  EXPECT_EQ(1u, api.GetSizeOfQueueForWindow(window->window()));

  // Further located events don't result in additional drag messages.
  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(2, 2));
  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(2, 2));
  EXPECT_EQ(1u, api.GetSizeOfQueueForWindow(window->window()));
}

TEST_F(DragControllerTest, CancelDrag) {
  // The DragController needs to stick around to coordinate the drop, but
  // it should ignore further mouse events during this time.
  std::unique_ptr<DragTestWindow> window = BuildWindow();
  StartDragOperation(window.get(), ui::mojom::kDropEffectMove);

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window->queue_response_type());
  window->Respond(true);

  drag_operation()->Cancel();

  EXPECT_FALSE(drag_completed_value().value_or(true));
}

TEST_F(DragControllerTest, IgnoreEventsFromOtherPointers) {
  std::unique_ptr<DragTestWindow> window = BuildWindow();
  // This starts the operation with MouseEvent::kMousePointerId.
  StartDragOperation(window.get(), ui::mojom::kDropEffectMove);

  // Ignore events from pointer 5.
  DispatchDragWithPointer(window.get(), 5, false, ui::EF_LEFT_MOUSE_BUTTON,
                          gfx::Point(1, 1));
  ASSERT_EQ(0u, window->queue_size());
}

TEST_F(DragControllerTest, RejectingWindowHasProperCursor) {
  std::unique_ptr<DragTestWindow> window = BuildWindow();
  StartDragOperation(window.get(), ui::mojom::kDropEffectMove);

  EXPECT_EQ(ui::CursorType::kNoDrop, cursor_type());

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window->queue_response_type());
  window->Respond(true);

  EXPECT_EQ(ui::CursorType::kCopy, cursor_type());

  DispatchDrag(window.get(), false, ui::EF_LEFT_MOUSE_BUTTON, gfx::Point(2, 2));
  EXPECT_EQ(QueuedType::OVER, window->queue_response_type());

  // At this point, we respond with no available drag actions at this pixel.
  window->Respond(false);
  EXPECT_EQ(ui::CursorType::kNoDrop, cursor_type());
}

TEST_F(DragControllerTest, ResopnseFromOtherWindowDoesntChangeCursor) {
  std::unique_ptr<DragTestWindow> window1 = BuildWindow();
  std::unique_ptr<DragTestWindow> window2 = BuildWindow();
  StartDragOperation(window1.get(), ui::mojom::kDropEffectMove);

  // Send some events to |window2|.
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));
  EXPECT_EQ(QueuedType::ENTER, window2->queue_response_type());
  DispatchDrag(window2.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(1, 1));

  EXPECT_EQ(ui::CursorType::kNoDrop, cursor_type());

  // Now enter |window1|, and respond.
  DispatchDrag(window1.get(), false, ui::EF_LEFT_MOUSE_BUTTON,
               gfx::Point(5, 5));
  EXPECT_EQ(QueuedType::ENTER, window1->queue_response_type());
  window1->Respond(true);

  EXPECT_EQ(ui::CursorType::kCopy, cursor_type());

  // Window 2 responding negatively to its queued messages shouldn't change the
  // cursor.
  window2->Respond(false);

  EXPECT_EQ(ui::CursorType::kCopy, cursor_type());
}

}  // namespace ws
}  // namespace ui
