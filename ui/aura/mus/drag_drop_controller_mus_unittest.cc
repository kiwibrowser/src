// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/drag_drop_controller_mus.h"

#include <memory>

#include "base/callback_forward.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/aura/client/drag_drop_client_observer.h"
#include "ui/aura/client/drag_drop_delegate.h"
#include "ui/aura/mus/drag_drop_controller_host.h"
#include "ui/aura/mus/window_mus.h"
#include "ui/aura/test/aura_mus_test_base.h"
#include "ui/aura/test/mus/test_window_tree.h"
#include "ui/base/dragdrop/drop_target_event.h"
#include "ui/events/event_utils.h"

namespace aura {
namespace {

class DragDropControllerMusTest : public test::AuraMusWmTestBase {
 public:
  DragDropControllerMusTest() = default;

  // test::AuraMusWmTestBase
  void SetUp() override {
    AuraMusWmTestBase::SetUp();
    controller_ = std::make_unique<DragDropControllerMus>(&controller_host_,
                                                          window_tree());
    window_ = std::unique_ptr<aura::Window>(
        CreateNormalWindow(0, root_window(), nullptr));
  }

  void TearDown() override {
    window_.reset();
    controller_.reset();
    AuraMusWmTestBase::TearDown();
  }

 protected:
  void PostDragMoveAndDrop() {
    // Posted task will be run when the inner loop runs in StartDragAndDrop.
    ASSERT_TRUE(base::ThreadTaskRunnerHandle::IsSet());
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&DragDropControllerMusTest::DragMoveAndDrop,
                                  base::Unretained(this)));
  }

  void StartDragAndDrop() {
    controller_->StartDragAndDrop(
        ui::OSExchangeData(), window_->GetRootWindow(), window_.get(),
        gfx::Point(5, 5), ui::DragDropTypes::DRAG_MOVE,
        ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE);
  }

  std::unique_ptr<DragDropControllerMus> controller_;
  std::unique_ptr<aura::Window> window_;

 private:
  void DragMoveAndDrop() {
    WindowMus* const window_mus = WindowMus::Get(window_.get());
    controller_->OnDragEnter(window_mus, 0, gfx::Point(5, 20), 0);
    controller_->OnDragOver(window_mus, 0, gfx::Point(5, 20), 0);
    controller_->OnCompleteDrop(window_mus, 0, gfx::Point(5, 20), 0);
    controller_->OnPerformDragDropCompleted(0);
  }

  class TestDragDropControllerHost : public DragDropControllerHost {
   public:
    TestDragDropControllerHost() : serial_(0u) {}

    // DragDropControllerHost
    uint32_t CreateChangeIdForDrag(WindowMus* window) override {
      return serial_++;
    }

   private:
    uint32_t serial_;

  } controller_host_;

  DISALLOW_COPY_AND_ASSIGN(DragDropControllerMusTest);
};

TEST_F(DragDropControllerMusTest, DragStartedAndEndedEvents) {
  enum class State { kNotInvoked, kDragStartInvoked, kDragEndInvoked };

  class TestObserver : public client::DragDropClientObserver {
   public:
    TestObserver() = default;
    State state() const { return state_; }

    // Overrides from client::DragDropClientObserver:
    void OnDragStarted() override {
      EXPECT_EQ(State::kNotInvoked, state_);
      state_ = State::kDragStartInvoked;
    }
    void OnDragEnded() override {
      EXPECT_EQ(State::kDragStartInvoked, state_);
      state_ = State::kDragEndInvoked;
    }

   private:
    State state_{State::kNotInvoked};

    DISALLOW_COPY_AND_ASSIGN(TestObserver);
  } observer;

  controller_->AddObserver(&observer);
  PostDragMoveAndDrop();
  StartDragAndDrop();
  EXPECT_EQ(State::kDragEndInvoked, observer.state());
  controller_->RemoveObserver(&observer);
}

TEST_F(DragDropControllerMusTest, EventTarget) {
  enum class State {
    kNotInvoked,
    kDragEnteredInvoked,
    kDragUpdateInvoked,
    kPerformDropInvoked
  };

  class TestDelegate : public client::DragDropDelegate {
   public:
    TestDelegate(aura::Window* window) : window_(window) {}
    State state() const { return state_; }

    // Overrides from client::DragDropClientObserver:
    void OnDragEntered(const ui::DropTargetEvent& event) override {
      EXPECT_EQ(State::kNotInvoked, state_);
      EXPECT_EQ(window_, event.target());
      state_ = State::kDragEnteredInvoked;
    }
    int OnDragUpdated(const ui::DropTargetEvent& event) override {
      EXPECT_TRUE(State::kDragEnteredInvoked == state_ ||
                  State::kDragUpdateInvoked == state_);
      EXPECT_EQ(window_, event.target());
      state_ = State::kDragUpdateInvoked;
      return ui::DragDropTypes::DRAG_MOVE;
    }
    void OnDragExited() override { ADD_FAILURE(); }
    int OnPerformDrop(const ui::DropTargetEvent& event) override {
      EXPECT_EQ(State::kDragUpdateInvoked, state_);
      EXPECT_EQ(window_, event.target());
      state_ = State::kPerformDropInvoked;
      return ui::DragDropTypes::DRAG_MOVE;
    }

   private:
    aura::Window* const window_;
    State state_{State::kNotInvoked};

    DISALLOW_COPY_AND_ASSIGN(TestDelegate);
  } delegate(window_.get());

  client::SetDragDropDelegate(window_.get(), &delegate);
  PostDragMoveAndDrop();
  StartDragAndDrop();
  EXPECT_EQ(State::kPerformDropInvoked, delegate.state());
  client::SetDragDropDelegate(window_.get(), nullptr);
}

}  // namespace
}  // namespace aura
