/*
 *  Copyright 2019 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "api/task_queue/task_queue_test.h"

#include "absl/memory/memory.h"
#include "absl/strings/string_view.h"
#include "rtc_base/event.h"
#include "rtc_base/task_queue.h"
#include "rtc_base/time_utils.h"

namespace webrtc {
namespace {

std::unique_ptr<TaskQueueBase, TaskQueueDeleter> CreateTaskQueue(
    TaskQueueFactory* factory,
    absl::string_view task_queue_name,
    TaskQueueFactory::Priority priority = TaskQueueFactory::Priority::NORMAL) {
  return factory->CreateTaskQueue(task_queue_name, priority);
}

TEST_P(TaskQueueTest, Construct) {
  auto queue = CreateTaskQueue(GetParam(), "Construct");
  EXPECT_FALSE(queue->IsCurrent());
}

TEST_P(TaskQueueTest, PostAndCheckCurrent) {
  rtc::Event event;
  auto queue = CreateTaskQueue(GetParam(), "PostAndCheckCurrent");

  // We're not running a task, so there shouldn't be a current queue.
  EXPECT_FALSE(queue->IsCurrent());
  EXPECT_FALSE(TaskQueueBase::Current());

  queue->PostTask(rtc::NewClosure([&event, &queue] {
    EXPECT_TRUE(queue->IsCurrent());
    event.Set();
  }));
  EXPECT_TRUE(event.Wait(1000));
}

TEST_P(TaskQueueTest, PostCustomTask) {
  rtc::Event ran;
  auto queue = CreateTaskQueue(GetParam(), "PostCustomImplementation");

  class CustomTask : public QueuedTask {
   public:
    explicit CustomTask(rtc::Event* ran) : ran_(ran) {}

   private:
    bool Run() override {
      ran_->Set();
      return false;  // Do not allow the task to be deleted by the queue.
    }

    rtc::Event* const ran_;
  } my_task(&ran);

  queue->PostTask(absl::WrapUnique(&my_task));
  EXPECT_TRUE(ran.Wait(1000));
}

TEST_P(TaskQueueTest, PostDelayedZero) {
  rtc::Event event;
  auto queue = CreateTaskQueue(GetParam(), "PostDelayedZero");

  queue->PostDelayedTask(rtc::NewClosure([&event] { event.Set(); }), 0);
  EXPECT_TRUE(event.Wait(1000));
}

TEST_P(TaskQueueTest, PostFromQueue) {
  rtc::Event event;
  auto queue = CreateTaskQueue(GetParam(), "PostFromQueue");

  queue->PostTask(rtc::NewClosure([&event, &queue] {
    queue->PostTask(rtc::NewClosure([&event] { event.Set(); }));
  }));
  EXPECT_TRUE(event.Wait(1000));
}

TEST_P(TaskQueueTest, PostDelayed) {
  rtc::Event event;
  auto queue = CreateTaskQueue(GetParam(), "PostDelayed",
                               TaskQueueFactory::Priority::HIGH);

  int64_t start = rtc::TimeMillis();
  queue->PostDelayedTask(rtc::NewClosure([&event, &queue] {
                           EXPECT_TRUE(queue->IsCurrent());
                           event.Set();
                         }),
                         100);
  EXPECT_TRUE(event.Wait(1000));
  int64_t end = rtc::TimeMillis();
  // These tests are a little relaxed due to how "powerful" our test bots can
  // be.  Most recently we've seen windows bots fire the callback after 94-99ms,
  // which is why we have a little bit of leeway backwards as well.
  EXPECT_GE(end - start, 90u);
  EXPECT_NEAR(end - start, 190u, 100u);  // Accept 90-290.
}

TEST_P(TaskQueueTest, PostMultipleDelayed) {
  auto queue = CreateTaskQueue(GetParam(), "PostMultipleDelayed");

  std::vector<rtc::Event> events(100);
  for (int i = 0; i < 100; ++i) {
    rtc::Event* event = &events[i];
    queue->PostDelayedTask(rtc::NewClosure([event, &queue] {
                             EXPECT_TRUE(queue->IsCurrent());
                             event->Set();
                           }),
                           i);
  }

  for (rtc::Event& e : events)
    EXPECT_TRUE(e.Wait(1000));
}

TEST_P(TaskQueueTest, PostDelayedAfterDestruct) {
  rtc::Event run;
  rtc::Event deleted;
  auto queue = CreateTaskQueue(GetParam(), "PostDelayedAfterDestruct");
  queue->PostDelayedTask(
      rtc::NewClosure([&run] { run.Set(); }, [&deleted] { deleted.Set(); }),
      100);
  // Destroy the queue.
  queue = nullptr;
  // Task might outlive the TaskQueue, but still should be deleted.
  EXPECT_TRUE(deleted.Wait(200));
  EXPECT_FALSE(run.Wait(0));  // and should not run.
}

TEST_P(TaskQueueTest, PostAndReuse) {
  rtc::Event event;
  auto post_queue = CreateTaskQueue(GetParam(), "PostQueue");
  auto reply_queue = CreateTaskQueue(GetParam(), "ReplyQueue");

  int call_count = 0;

  class ReusedTask : public QueuedTask {
   public:
    ReusedTask(int* counter, TaskQueueBase* reply_queue, rtc::Event* event)
        : counter_(*counter), reply_queue_(reply_queue), event_(*event) {
      EXPECT_EQ(counter_, 0);
    }

   private:
    bool Run() override {
      if (++counter_ == 1) {
        reply_queue_->PostTask(absl::WrapUnique(this));
        // At this point, the object is owned by reply_queue_ and it's
        // theoratically possible that the object has been deleted (e.g. if
        // posting wasn't possible).  So, don't touch any member variables here.

        // Indicate to the current queue that ownership has been transferred.
        return false;
      } else {
        EXPECT_EQ(counter_, 2);
        EXPECT_TRUE(reply_queue_->IsCurrent());
        event_.Set();
        return true;  // Indicate that the object should be deleted.
      }
    }

    int& counter_;
    TaskQueueBase* const reply_queue_;
    rtc::Event& event_;
  };

  auto task =
      absl::make_unique<ReusedTask>(&call_count, reply_queue.get(), &event);
  post_queue->PostTask(std::move(task));
  EXPECT_TRUE(event.Wait(1000));
}

// Tests posting more messages than a queue can queue up.
// In situations like that, tasks will get dropped.
TEST_P(TaskQueueTest, PostALot) {
  // To destruct the event after the queue has gone out of scope.
  rtc::Event event;

  int tasks_executed = 0;
  int tasks_cleaned_up = 0;
  static const int kTaskCount = 0xffff;

  {
    auto queue = CreateTaskQueue(GetParam(), "PostALot");

    // On linux, the limit of pending bytes in the pipe buffer is 0xffff.
    // So here we post a total of 0xffff+1 messages, which triggers a failure
    // case inside of the libevent queue implementation.

    queue->PostTask(
        rtc::NewClosure([&event] { event.Wait(rtc::Event::kForever); }));
    for (int i = 0; i < kTaskCount; ++i)
      queue->PostTask(
          rtc::NewClosure([&tasks_executed] { ++tasks_executed; },
                          [&tasks_cleaned_up] { ++tasks_cleaned_up; }));
    event.Set();  // Unblock the first task.
  }

  EXPECT_GE(tasks_cleaned_up, tasks_executed);
  EXPECT_EQ(tasks_cleaned_up, kTaskCount);
}

// Test posting two tasks that have shared state not protected by a
// lock. The TaskQueue should guarantee memory read-write order and
// FIFO task execution order, so the second task should always see the
// changes that were made by the first task.
//
// If the TaskQueue doesn't properly synchronize the execution of
// tasks, there will be a data race, which is undefined behavior. The
// EXPECT calls may randomly catch this, but to make the most of this
// unit test, run it under TSan or some other tool that is able to
// directly detect data races.
TEST_P(TaskQueueTest, PostTwoWithSharedUnprotectedState) {
  struct SharedState {
    // First task will set this value to 1 and second will assert it.
    int state = 0;
  } state;

  auto queue = CreateTaskQueue(GetParam(), "PostTwoWithSharedUnprotectedState");
  rtc::Event done;
  queue->PostTask(rtc::NewClosure([&state, &queue, &done] {
    // Post tasks from queue to guarantee, that 1st task won't be
    // executed before the second one will be posted.
    queue->PostTask(rtc::NewClosure([&state] { state.state = 1; }));
    queue->PostTask(rtc::NewClosure([&state, &done] {
      EXPECT_EQ(state.state, 1);
      done.Set();
    }));
    // Check, that state changing tasks didn't start yet.
    EXPECT_EQ(state.state, 0);
  }));
  EXPECT_TRUE(done.Wait(1000));
}

}  // namespace
}  // namespace webrtc
