// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/scripted_animation_controller.h"

#include <memory>
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/events/event_listener.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/dom/frame_request_callback_collection.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

class ScriptedAnimationControllerTest : public testing::Test {
 protected:
  void SetUp() override;

  Document& GetDocument() const { return dummy_page_holder_->GetDocument(); }
  ScriptedAnimationController& Controller() { return *controller_; }

 private:
  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
  Persistent<ScriptedAnimationController> controller_;
};

void ScriptedAnimationControllerTest::SetUp() {
  dummy_page_holder_ = DummyPageHolder::Create(IntSize(800, 600));

  // Note: The document doesn't know about this ScriptedAnimationController
  // instance, and will create another if
  // Document::ensureScriptedAnimationController is called.
  controller_ =
      WrapPersistent(ScriptedAnimationController::Create(&GetDocument()));
}

namespace {

class TaskOrderObserver {
 public:
  base::RepeatingClosure CreateTask(int id) {
    return WTF::BindRepeating(&TaskOrderObserver::RunTask,
                              WTF::Unretained(this), id);
  }
  const Vector<int>& Order() const { return order_; }

 private:
  void RunTask(int id) { order_.push_back(id); }
  Vector<int> order_;
};

}  // anonymous namespace

TEST_F(ScriptedAnimationControllerTest, EnqueueOneTask) {
  TaskOrderObserver observer;

  Controller().EnqueueTask(observer.CreateTask(1));
  EXPECT_EQ(0u, observer.Order().size());

  Controller().ServiceScriptedAnimations(base::TimeTicks());
  EXPECT_EQ(1u, observer.Order().size());
  EXPECT_EQ(1, observer.Order()[0]);
}

TEST_F(ScriptedAnimationControllerTest, EnqueueTwoTasks) {
  TaskOrderObserver observer;

  Controller().EnqueueTask(observer.CreateTask(1));
  Controller().EnqueueTask(observer.CreateTask(2));
  EXPECT_EQ(0u, observer.Order().size());

  Controller().ServiceScriptedAnimations(base::TimeTicks());
  EXPECT_EQ(2u, observer.Order().size());
  EXPECT_EQ(1, observer.Order()[0]);
  EXPECT_EQ(2, observer.Order()[1]);
}

namespace {

void EnqueueTask(ScriptedAnimationController* controller,
                 TaskOrderObserver* observer,
                 int id) {
  controller->EnqueueTask(observer->CreateTask(id));
}

}  // anonymous namespace

// A task enqueued while running tasks should not be run immediately after, but
// the next time tasks are run.
TEST_F(ScriptedAnimationControllerTest, EnqueueWithinTask) {
  TaskOrderObserver observer;

  Controller().EnqueueTask(observer.CreateTask(1));
  Controller().EnqueueTask(WTF::Bind(&EnqueueTask,
                                     WrapPersistent(&Controller()),
                                     WTF::Unretained(&observer), 2));
  Controller().EnqueueTask(observer.CreateTask(3));
  EXPECT_EQ(0u, observer.Order().size());

  Controller().ServiceScriptedAnimations(base::TimeTicks());
  EXPECT_EQ(2u, observer.Order().size());
  EXPECT_EQ(1, observer.Order()[0]);
  EXPECT_EQ(3, observer.Order()[1]);

  Controller().ServiceScriptedAnimations(base::TimeTicks());
  EXPECT_EQ(3u, observer.Order().size());
  EXPECT_EQ(1, observer.Order()[0]);
  EXPECT_EQ(3, observer.Order()[1]);
  EXPECT_EQ(2, observer.Order()[2]);
}

namespace {

class RunTaskEventListener final : public EventListener {
 public:
  RunTaskEventListener(base::RepeatingClosure task)
      : EventListener(kCPPEventListenerType), task_(std::move(task)) {}
  void handleEvent(ExecutionContext*, Event*) override { task_.Run(); }
  bool operator==(const EventListener& other) const override {
    return this == &other;
  }

 private:
  base::RepeatingClosure task_;
};

}  // anonymous namespace

// Tasks should be run after events are dispatched, even if they were enqueued
// first.
TEST_F(ScriptedAnimationControllerTest, EnqueueTaskAndEvent) {
  TaskOrderObserver observer;

  Controller().EnqueueTask(observer.CreateTask(1));
  GetDocument().addEventListener(
      "test", new RunTaskEventListener(observer.CreateTask(2)));
  Event* event = Event::Create("test");
  event->SetTarget(&GetDocument());
  Controller().EnqueueEvent(event);
  EXPECT_EQ(0u, observer.Order().size());

  Controller().ServiceScriptedAnimations(base::TimeTicks());
  EXPECT_EQ(2u, observer.Order().size());
  EXPECT_EQ(2, observer.Order()[0]);
  EXPECT_EQ(1, observer.Order()[1]);
}

namespace {

class RunTaskCallback final
    : public FrameRequestCallbackCollection::FrameCallback {
 public:
  RunTaskCallback(base::RepeatingClosure task) : task_(std::move(task)) {}
  void Invoke(double) override { task_.Run(); }

 private:
  base::RepeatingClosure task_;
};

}  // anonymous namespace

// Animation frame callbacks should be run after tasks, even if they were
// enqueued first.
TEST_F(ScriptedAnimationControllerTest, RegisterCallbackAndEnqueueTask) {
  TaskOrderObserver observer;

  Event* event = Event::Create("test");
  event->SetTarget(&GetDocument());

  Controller().RegisterCallback(new RunTaskCallback(observer.CreateTask(1)));
  Controller().EnqueueTask(observer.CreateTask(2));
  EXPECT_EQ(0u, observer.Order().size());

  Controller().ServiceScriptedAnimations(base::TimeTicks());
  EXPECT_EQ(2u, observer.Order().size());
  EXPECT_EQ(2, observer.Order()[0]);
  EXPECT_EQ(1, observer.Order()[1]);
}

TEST_F(ScriptedAnimationControllerTest, TestHasCallback) {
  TaskOrderObserver observer;

  Controller().RegisterCallback(new RunTaskCallback(observer.CreateTask(1)));
  EXPECT_TRUE(Controller().HasCallback());

  Controller().CancelCallback(1);
  EXPECT_FALSE(Controller().HasCallback());

  Controller().RegisterCallback(new RunTaskCallback(observer.CreateTask(1)));
  Controller().RegisterCallback(new RunTaskCallback(observer.CreateTask(2)));
  EXPECT_TRUE(Controller().HasCallback());

  Controller().CancelCallback(1);
  EXPECT_TRUE(Controller().HasCallback());

  // Servicing the scripted animations should call the remaining callback and
  // clear it.
  Controller().ServiceScriptedAnimations(base::TimeTicks());
  EXPECT_FALSE(Controller().HasCallback());
}

}  // namespace blink
