// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/notifications/notification_event_dispatcher_impl.h"

#include <stdint.h>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/notifications/notification_service.mojom.h"

namespace content {

namespace {

const char kPrimaryUniqueId[] = "this_should_be_a_unique_id";
const char kSomeOtherUniqueId[] = "and_this_one_is_different_and_also_unique";

class TestNotificationListener
    : public blink::mojom::NonPersistentNotificationListener {
 public:
  TestNotificationListener() : binding_(this) {}
  ~TestNotificationListener() override = default;

  // Closes the bindings associated with this listener.
  void Close() { binding_.Close(); }

  // Returns an InterfacePtr to this listener.
  blink::mojom::NonPersistentNotificationListenerPtr GetPtr() {
    blink::mojom::NonPersistentNotificationListenerPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  // Returns the number of OnShow events received by this listener.
  int on_show_count() const { return on_show_count_; }

  // Returns the number of OnClick events received by this listener.
  int on_click_count() const { return on_click_count_; }

  // Returns the number of OnClose events received by this listener.
  int on_close_count() const { return on_close_count_; }

  // blink::mojom::NonPersistentNotificationListener implementation.
  void OnShow() override { on_show_count_++; }
  void OnClick() override { on_click_count_++; }
  void OnClose(OnCloseCallback completed_closure) override {
    on_close_count_++;
    std::move(completed_closure).Run();
  }

 private:
  int on_show_count_ = 0;
  int on_click_count_ = 0;
  int on_close_count_ = 0;
  mojo::Binding<blink::mojom::NonPersistentNotificationListener> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestNotificationListener);
};

}  // anonymous namespace

class NotificationEventDispatcherImplTest : public ::testing::Test {
 public:
  NotificationEventDispatcherImplTest()
      : task_runner_(new base::TestSimpleTaskRunner),
        handle_(task_runner_),
        dispatcher_(new NotificationEventDispatcherImpl()) {}

  ~NotificationEventDispatcherImplTest() override { delete dispatcher_; }

  // Waits until the task runner managing the Mojo connection has finished.
  void WaitForMojoTasksToComplete() { task_runner_->RunUntilIdle(); }

 protected:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle handle_;

  // Using a raw pointer because NotificationEventDispatcherImpl is a singleton
  // with private constructor and destructor, so unique_ptr is not an option.
  NotificationEventDispatcherImpl* dispatcher_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationEventDispatcherImplTest);
};

TEST_F(NotificationEventDispatcherImplTest,
       DispatchNonPersistentShowEvent_NotifiesCorrectRegisteredListener) {
  auto listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(
      kPrimaryUniqueId, listener->GetPtr().PassInterface());
  auto other_listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(
      kSomeOtherUniqueId, other_listener->GetPtr().PassInterface());

  dispatcher_->DispatchNonPersistentShowEvent(kPrimaryUniqueId);

  WaitForMojoTasksToComplete();

  EXPECT_EQ(listener->on_show_count(), 1);
  EXPECT_EQ(other_listener->on_show_count(), 0);

  dispatcher_->DispatchNonPersistentShowEvent(kSomeOtherUniqueId);

  WaitForMojoTasksToComplete();

  EXPECT_EQ(listener->on_show_count(), 1);
  EXPECT_EQ(other_listener->on_show_count(), 1);
}

TEST_F(NotificationEventDispatcherImplTest,
       RegisterReplacementNonPersistentListener_FirstListenerGetsOnClose) {
  auto original_listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(
      kPrimaryUniqueId, original_listener->GetPtr().PassInterface());

  dispatcher_->DispatchNonPersistentShowEvent(kPrimaryUniqueId);

  ASSERT_EQ(original_listener->on_close_count(), 0);

  auto replacement_listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(
      kPrimaryUniqueId, replacement_listener->GetPtr().PassInterface());

  WaitForMojoTasksToComplete();

  EXPECT_EQ(original_listener->on_close_count(), 1);
  EXPECT_EQ(replacement_listener->on_close_count(), 0);
}

TEST_F(NotificationEventDispatcherImplTest,
       DispatchNonPersistentClickEvent_NotifiesCorrectRegisteredListener) {
  auto listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(
      kPrimaryUniqueId, listener->GetPtr().PassInterface());
  auto other_listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(
      kSomeOtherUniqueId, other_listener->GetPtr().PassInterface());

  dispatcher_->DispatchNonPersistentClickEvent(kPrimaryUniqueId);

  WaitForMojoTasksToComplete();

  EXPECT_EQ(listener->on_click_count(), 1);
  EXPECT_EQ(other_listener->on_click_count(), 0);

  dispatcher_->DispatchNonPersistentClickEvent(kSomeOtherUniqueId);

  WaitForMojoTasksToComplete();

  EXPECT_EQ(listener->on_click_count(), 1);
  EXPECT_EQ(other_listener->on_click_count(), 1);
}

TEST_F(NotificationEventDispatcherImplTest,
       DispatchNonPersistentCloseEvent_NotifiesCorrectRegisteredListener) {
  auto listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(
      kPrimaryUniqueId, listener->GetPtr().PassInterface());
  auto other_listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(
      kSomeOtherUniqueId, other_listener->GetPtr().PassInterface());

  dispatcher_->DispatchNonPersistentCloseEvent(kPrimaryUniqueId,
                                               base::DoNothing());

  WaitForMojoTasksToComplete();

  EXPECT_EQ(listener->on_close_count(), 1);
  EXPECT_EQ(other_listener->on_close_count(), 0);

  dispatcher_->DispatchNonPersistentCloseEvent(kSomeOtherUniqueId,
                                               base::DoNothing());

  WaitForMojoTasksToComplete();

  EXPECT_EQ(listener->on_close_count(), 1);
  EXPECT_EQ(other_listener->on_close_count(), 1);
}

TEST_F(NotificationEventDispatcherImplTest,
       DispatchMultipleNonPersistentEvents_StopsNotifyingAfterClose) {
  auto listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(
      kPrimaryUniqueId, listener->GetPtr().PassInterface());

  dispatcher_->DispatchNonPersistentShowEvent(kPrimaryUniqueId);
  dispatcher_->DispatchNonPersistentClickEvent(kPrimaryUniqueId);
  dispatcher_->DispatchNonPersistentCloseEvent(kPrimaryUniqueId,
                                               base::DoNothing());

  WaitForMojoTasksToComplete();

  EXPECT_EQ(listener->on_show_count(), 1);
  EXPECT_EQ(listener->on_click_count(), 1);
  EXPECT_EQ(listener->on_close_count(), 1);

  // Should not be counted as the notification was already closed.
  dispatcher_->DispatchNonPersistentClickEvent(kPrimaryUniqueId);

  WaitForMojoTasksToComplete();

  EXPECT_EQ(listener->on_click_count(), 1);
}
}  // namespace content
