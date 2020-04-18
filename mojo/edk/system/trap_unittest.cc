// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <map>
#include <memory>
#include <set>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/time/time.h"
#include "mojo/edk/test/mojo_test_base.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/c/system/trap.h"
#include "mojo/public/c/system/types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace edk {
namespace {

// TODO(https://crbug.com/819046): These are temporary wrappers to reduce
// changes necessary during the API rename. Remove them.

MojoResult MojoWatch(MojoHandle trap_handle,
                     MojoHandle handle,
                     MojoHandleSignals signals,
                     MojoTriggerCondition condition,
                     uintptr_t context) {
  return MojoAddTrigger(trap_handle, handle, signals, condition, context,
                        nullptr);
}

MojoResult MojoCancelWatch(MojoHandle trap_handle, uintptr_t context) {
  return MojoRemoveTrigger(trap_handle, context, nullptr);
}

MojoResult MojoArmWatcher(MojoHandle trap_handle,
                          uint32_t* num_ready_contexts,
                          uintptr_t* ready_contexts,
                          MojoResult* ready_results,
                          MojoHandleSignalsState* ready_signals) {
  return MojoArmTrap(trap_handle, nullptr, num_ready_contexts, ready_contexts,
                     ready_results, ready_signals);
}

using WatcherTest = test::MojoTestBase;

class WatchHelper {
 public:
  using ContextCallback =
      base::Callback<void(MojoResult, MojoHandleSignalsState)>;

  WatchHelper() {}
  ~WatchHelper() {}

  MojoResult CreateWatcher(MojoHandle* handle) {
    return MojoCreateTrap(&Notify, nullptr, handle);
  }

  uintptr_t CreateContext(const ContextCallback& callback) {
    return CreateContextWithCancel(callback, base::Closure());
  }

  uintptr_t CreateContextWithCancel(const ContextCallback& callback,
                                    const base::Closure& cancel_callback) {
    auto context = std::make_unique<NotificationContext>(callback);
    NotificationContext* raw_context = context.get();
    raw_context->SetCancelCallback(base::Bind(
        [](std::unique_ptr<NotificationContext> context,
           const base::Closure& cancel_callback) {
          if (cancel_callback)
            cancel_callback.Run();
        },
        base::Passed(&context), cancel_callback));
    return reinterpret_cast<uintptr_t>(raw_context);
  }

 private:
  class NotificationContext {
   public:
    explicit NotificationContext(const ContextCallback& callback)
        : callback_(callback) {}

    ~NotificationContext() {}

    void SetCancelCallback(const base::Closure& cancel_callback) {
      cancel_callback_ = cancel_callback;
    }

    void Notify(MojoResult result, MojoHandleSignalsState state) {
      if (result == MOJO_RESULT_CANCELLED)
        cancel_callback_.Run();
      else
        callback_.Run(result, state);
    }

   private:
    const ContextCallback callback_;
    base::Closure cancel_callback_;

    DISALLOW_COPY_AND_ASSIGN(NotificationContext);
  };

  static void Notify(const MojoTrapEvent* event) {
    reinterpret_cast<NotificationContext*>(event->trigger_context)
        ->Notify(event->result, event->signals_state);
  }

  DISALLOW_COPY_AND_ASSIGN(WatchHelper);
};

class ThreadedRunner : public base::SimpleThread {
 public:
  explicit ThreadedRunner(const base::Closure& callback)
      : SimpleThread("ThreadedRunner"), callback_(callback) {}
  ~ThreadedRunner() override {}

  void Run() override { callback_.Run(); }

 private:
  const base::Closure callback_;

  DISALLOW_COPY_AND_ASSIGN(ThreadedRunner);
};

void ExpectNoNotification(const MojoTrapEvent* event) {
  NOTREACHED();
}

void ExpectOnlyCancel(const MojoTrapEvent* event) {
  EXPECT_EQ(event->result, MOJO_RESULT_CANCELLED);
}

TEST_F(WatcherTest, InvalidArguments) {
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoCreateTrap(&ExpectNoNotification, nullptr, nullptr));
  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateTrap(&ExpectNoNotification, nullptr, &w));

  // Try to watch unwatchable handles.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWatch(w, w, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, 0));
  MojoHandle buffer_handle = CreateBuffer(42);
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoWatch(w, buffer_handle, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, 0));

  // Try to cancel a watch on an invalid watcher handle.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoCancelWatch(buffer_handle, 0));

  // Try to arm an invalid handle.
  EXPECT_EQ(
      MOJO_RESULT_INVALID_ARGUMENT,
      MojoArmWatcher(MOJO_HANDLE_INVALID, nullptr, nullptr, nullptr, nullptr));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoArmWatcher(buffer_handle, nullptr, nullptr, nullptr, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(buffer_handle));

  // Try to arm with a non-null count but at least one null output buffer.
  uint32_t num_ready_contexts = 1;
  uintptr_t ready_context;
  MojoResult ready_result;
  MojoHandleSignalsState ready_state;
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoArmWatcher(w, &num_ready_contexts, nullptr, &ready_result,
                           &ready_state));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoArmWatcher(w, &num_ready_contexts, &ready_context, nullptr,
                           &ready_state));
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
            MojoArmWatcher(w, &num_ready_contexts, &ready_context,
                           &ready_result, nullptr));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
}

TEST_F(WatcherTest, WatchMessagePipeReadable) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  int num_expected_notifications = 1;
  const uintptr_t readable_a_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, int* expected_count, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_GT(*expected_count, 0);
        *expected_count -= 1;

        EXPECT_EQ(MOJO_RESULT_OK, result);
        event->Signal();
      },
      &event, &num_expected_notifications));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  const char kMessage1[] = "hey hey hey hey";
  const char kMessage2[] = "i said hey";
  const char kMessage3[] = "what's goin' on?";

  // Writing to |b| multiple times should notify exactly once.
  WriteMessage(b, kMessage1);
  WriteMessage(b, kMessage2);
  event.Wait();

  // This also shouldn't fire a notification; the watcher is still disarmed.
  WriteMessage(b, kMessage3);

  // Arming should fail with relevant information.
  constexpr size_t kMaxReadyContexts = 10;
  uint32_t num_ready_contexts = kMaxReadyContexts;
  uintptr_t ready_contexts[kMaxReadyContexts];
  MojoResult ready_results[kMaxReadyContexts];
  MojoHandleSignalsState ready_states[kMaxReadyContexts];
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(readable_a_context, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[0]);

  // Flush the three messages from above.
  EXPECT_EQ(kMessage1, ReadMessage(a));
  EXPECT_EQ(kMessage2, ReadMessage(a));
  EXPECT_EQ(kMessage3, ReadMessage(a));

  // Now we can rearm the watcher.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
}

TEST_F(WatcherTest, CloseWatchedMessagePipeHandle) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  const uintptr_t readable_a_context = helper.CreateContextWithCancel(
      WatchHelper::ContextCallback(),
      base::Bind([](base::WaitableEvent* event) { event->Signal(); }, &event));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));

  // Test that closing a watched handle fires an appropriate notification, even
  // when the watcher is unarmed.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  event.Wait();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
}

TEST_F(WatcherTest, CloseWatchedMessagePipeHandlePeer) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  const uintptr_t readable_a_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, result);
        event->Signal();
      },
      &event));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));

  // Test that closing a watched handle's peer with an armed watcher fires an
  // appropriate notification.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
  event.Wait();

  // And now arming should fail with correct information about |a|'s state.
  constexpr size_t kMaxReadyContexts = 10;
  uint32_t num_ready_contexts = kMaxReadyContexts;
  uintptr_t ready_contexts[kMaxReadyContexts];
  MojoResult ready_results[kMaxReadyContexts];
  MojoHandleSignalsState ready_states[kMaxReadyContexts];
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(readable_a_context, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, ready_results[0]);
  EXPECT_TRUE(ready_states[0].satisfied_signals &
              MOJO_HANDLE_SIGNAL_PEER_CLOSED);
  EXPECT_FALSE(ready_states[0].satisfiable_signals &
               MOJO_HANDLE_SIGNAL_READABLE);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
}

TEST_F(WatcherTest, WatchDataPipeConsumerReadable) {
  constexpr size_t kTestPipeCapacity = 64;
  MojoHandle producer, consumer;
  CreateDataPipe(&producer, &consumer, kTestPipeCapacity);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  int num_expected_notifications = 1;
  const uintptr_t readable_consumer_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, int* expected_count, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_GT(*expected_count, 0);
        *expected_count -= 1;

        EXPECT_EQ(MOJO_RESULT_OK, result);
        event->Signal();
      },
      &event, &num_expected_notifications));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, consumer, MOJO_HANDLE_SIGNAL_READABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED,
                                      readable_consumer_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  const char kMessage1[] = "hey hey hey hey";
  const char kMessage2[] = "i said hey";
  const char kMessage3[] = "what's goin' on?";

  // Writing to |producer| multiple times should notify exactly once.
  WriteData(producer, kMessage1);
  WriteData(producer, kMessage2);
  event.Wait();

  // This also shouldn't fire a notification; the watcher is still disarmed.
  WriteData(producer, kMessage3);

  // Arming should fail with relevant information.
  constexpr size_t kMaxReadyContexts = 10;
  uint32_t num_ready_contexts = kMaxReadyContexts;
  uintptr_t ready_contexts[kMaxReadyContexts];
  MojoResult ready_results[kMaxReadyContexts];
  MojoHandleSignalsState ready_states[kMaxReadyContexts];
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(readable_consumer_context, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[0]);

  // Flush the three messages from above.
  EXPECT_EQ(kMessage1, ReadData(consumer, sizeof(kMessage1) - 1));
  EXPECT_EQ(kMessage2, ReadData(consumer, sizeof(kMessage2) - 1));
  EXPECT_EQ(kMessage3, ReadData(consumer, sizeof(kMessage3) - 1));

  // Now we can rearm the watcher.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(producer));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(consumer));
}

TEST_F(WatcherTest, WatchDataPipeConsumerNewDataReadable) {
  constexpr size_t kTestPipeCapacity = 64;
  MojoHandle producer, consumer;
  CreateDataPipe(&producer, &consumer, kTestPipeCapacity);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  int num_new_data_notifications = 0;
  const uintptr_t new_data_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, int* notification_count, MojoResult result,
         MojoHandleSignalsState state) {
        *notification_count += 1;

        EXPECT_EQ(MOJO_RESULT_OK, result);
        event->Signal();
      },
      &event, &num_new_data_notifications));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, consumer, MOJO_HANDLE_SIGNAL_NEW_DATA_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, new_data_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  const char kMessage1[] = "hey hey hey hey";
  const char kMessage2[] = "i said hey";
  const char kMessage3[] = "what's goin' on?";

  // Writing to |producer| multiple times should notify exactly once.
  WriteData(producer, kMessage1);
  WriteData(producer, kMessage2);
  event.Wait();

  // This also shouldn't fire a notification; the watcher is still disarmed.
  WriteData(producer, kMessage3);

  // Arming should fail with relevant information.
  constexpr size_t kMaxReadyContexts = 10;
  uint32_t num_ready_contexts = kMaxReadyContexts;
  uintptr_t ready_contexts[kMaxReadyContexts];
  MojoResult ready_results[kMaxReadyContexts];
  MojoHandleSignalsState ready_states[kMaxReadyContexts];
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(new_data_context, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[0]);

  // Attempt to read more data than is available. Should fail but clear the
  // NEW_DATA_READABLE signal.
  char large_buffer[512];
  uint32_t large_read_size = 512;
  MojoReadDataOptions options;
  options.struct_size = sizeof(options);
  options.flags = MOJO_READ_DATA_FLAG_ALL_OR_NONE;
  EXPECT_EQ(MOJO_RESULT_OUT_OF_RANGE,
            MojoReadData(consumer, &options, large_buffer, &large_read_size));

  // Attempt to arm again. Should succeed.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  // Write more data. Should notify.
  event.Reset();
  WriteData(producer, kMessage1);
  event.Wait();

  // Reading some data should clear NEW_DATA_READABLE again so we can rearm.
  EXPECT_EQ(kMessage1, ReadData(consumer, sizeof(kMessage1) - 1));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  EXPECT_EQ(2, num_new_data_notifications);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(producer));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(consumer));
}

TEST_F(WatcherTest, WatchDataPipeProducerWritable) {
  constexpr size_t kTestPipeCapacity = 8;
  MojoHandle producer, consumer;
  CreateDataPipe(&producer, &consumer, kTestPipeCapacity);

  // Half the capacity of the data pipe.
  const char kTestData[] = "aaaa";
  static_assert((sizeof(kTestData) - 1) * 2 == kTestPipeCapacity,
                "Invalid test data for this test.");

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  int num_expected_notifications = 1;
  const uintptr_t writable_producer_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, int* expected_count, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_GT(*expected_count, 0);
        *expected_count -= 1;

        EXPECT_EQ(MOJO_RESULT_OK, result);
        event->Signal();
      },
      &event, &num_expected_notifications));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, producer, MOJO_HANDLE_SIGNAL_WRITABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED,
                                      writable_producer_context));

  // The producer is already writable, so arming should fail with relevant
  // information.
  constexpr size_t kMaxReadyContexts = 10;
  uint32_t num_ready_contexts = kMaxReadyContexts;
  uintptr_t ready_contexts[kMaxReadyContexts];
  MojoResult ready_results[kMaxReadyContexts];
  MojoHandleSignalsState ready_states[kMaxReadyContexts];
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(writable_producer_context, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[0]);
  EXPECT_TRUE(ready_states[0].satisfied_signals & MOJO_HANDLE_SIGNAL_WRITABLE);

  // Write some data, but don't fill the pipe yet. Arming should fail again.
  WriteData(producer, kTestData);
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(writable_producer_context, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[0]);
  EXPECT_TRUE(ready_states[0].satisfied_signals & MOJO_HANDLE_SIGNAL_WRITABLE);

  // Write more data, filling the pipe to capacity. Arming should succeed now.
  WriteData(producer, kTestData);
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  // Now read from the pipe, making the producer writable again. Should notify.
  EXPECT_EQ(kTestData, ReadData(consumer, sizeof(kTestData) - 1));
  event.Wait();

  // Arming should fail again.
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(writable_producer_context, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[0]);
  EXPECT_TRUE(ready_states[0].satisfied_signals & MOJO_HANDLE_SIGNAL_WRITABLE);

  // Fill the pipe once more and arm the watcher. Should succeed.
  WriteData(producer, kTestData);
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(producer));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(consumer));
};

TEST_F(WatcherTest, CloseWatchedDataPipeConsumerHandle) {
  constexpr size_t kTestPipeCapacity = 8;
  MojoHandle producer, consumer;
  CreateDataPipe(&producer, &consumer, kTestPipeCapacity);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  const uintptr_t readable_consumer_context = helper.CreateContextWithCancel(
      WatchHelper::ContextCallback(),
      base::Bind([](base::WaitableEvent* event) { event->Signal(); }, &event));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, consumer, MOJO_HANDLE_SIGNAL_READABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED,
                                      readable_consumer_context));

  // Closing the consumer should fire a cancellation notification.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(consumer));
  event.Wait();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(producer));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
}

TEST_F(WatcherTest, CloseWatcherDataPipeConsumerHandlePeer) {
  constexpr size_t kTestPipeCapacity = 8;
  MojoHandle producer, consumer;
  CreateDataPipe(&producer, &consumer, kTestPipeCapacity);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  const uintptr_t readable_consumer_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, result);
        event->Signal();
      },
      &event));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, consumer, MOJO_HANDLE_SIGNAL_READABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED,
                                      readable_consumer_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  // Closing the producer should fire a notification for an unsatisfiable watch.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(producer));
  event.Wait();

  // Now attempt to rearm and expect appropriate error feedback.
  constexpr size_t kMaxReadyContexts = 10;
  uint32_t num_ready_contexts = kMaxReadyContexts;
  uintptr_t ready_contexts[kMaxReadyContexts];
  MojoResult ready_results[kMaxReadyContexts];
  MojoHandleSignalsState ready_states[kMaxReadyContexts];
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(readable_consumer_context, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, ready_results[0]);
  EXPECT_FALSE(ready_states[0].satisfiable_signals &
               MOJO_HANDLE_SIGNAL_READABLE);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(consumer));
}

TEST_F(WatcherTest, CloseWatchedDataPipeProducerHandle) {
  constexpr size_t kTestPipeCapacity = 8;
  MojoHandle producer, consumer;
  CreateDataPipe(&producer, &consumer, kTestPipeCapacity);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  const uintptr_t writable_producer_context = helper.CreateContextWithCancel(
      WatchHelper::ContextCallback(),
      base::Bind([](base::WaitableEvent* event) { event->Signal(); }, &event));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, producer, MOJO_HANDLE_SIGNAL_WRITABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED,
                                      writable_producer_context));

  // Closing the consumer should fire a cancellation notification.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(producer));
  event.Wait();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(consumer));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
}

TEST_F(WatcherTest, CloseWatchedDataPipeProducerHandlePeer) {
  constexpr size_t kTestPipeCapacity = 8;
  MojoHandle producer, consumer;
  CreateDataPipe(&producer, &consumer, kTestPipeCapacity);

  const char kTestMessageFullCapacity[] = "xxxxxxxx";
  static_assert(sizeof(kTestMessageFullCapacity) - 1 == kTestPipeCapacity,
                "Invalid test message size for this test.");

  // Make the pipe unwritable initially.
  WriteData(producer, kTestMessageFullCapacity);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  const uintptr_t writable_producer_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, result);
        event->Signal();
      },
      &event));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, producer, MOJO_HANDLE_SIGNAL_WRITABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED,
                                      writable_producer_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  // Closing the consumer should fire a notification for an unsatisfiable watch,
  // as the full data pipe can never be read from again and is therefore
  // permanently full and unwritable.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(consumer));
  event.Wait();

  // Now attempt to rearm and expect appropriate error feedback.
  constexpr size_t kMaxReadyContexts = 10;
  uint32_t num_ready_contexts = kMaxReadyContexts;
  uintptr_t ready_contexts[kMaxReadyContexts];
  MojoResult ready_results[kMaxReadyContexts];
  MojoHandleSignalsState ready_states[kMaxReadyContexts];
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(writable_producer_context, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, ready_results[0]);
  EXPECT_FALSE(ready_states[0].satisfiable_signals &
               MOJO_HANDLE_SIGNAL_WRITABLE);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(producer));
}

TEST_F(WatcherTest, ArmWithNoWatches) {
  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateTrap(&ExpectNoNotification, nullptr, &w));
  EXPECT_EQ(MOJO_RESULT_NOT_FOUND,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
}

TEST_F(WatcherTest, WatchDuplicateContext) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateTrap(&ExpectOnlyCancel, nullptr, &w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED, 0));
  EXPECT_EQ(MOJO_RESULT_ALREADY_EXISTS,
            MojoWatch(w, b, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, 0));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
}

TEST_F(WatcherTest, CancelUnknownWatch) {
  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateTrap(&ExpectNoNotification, nullptr, &w));
  EXPECT_EQ(MOJO_RESULT_NOT_FOUND, MojoCancelWatch(w, 1234));
}

TEST_F(WatcherTest, ArmWithWatchAlreadySatisfied) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateTrap(&ExpectOnlyCancel, nullptr, &w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, a, MOJO_HANDLE_SIGNAL_WRITABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED, 0));

  // |a| is always writable, so we can never arm this watcher.
  constexpr size_t kMaxReadyContexts = 10;
  uint32_t num_ready_contexts = kMaxReadyContexts;
  uintptr_t ready_contexts[kMaxReadyContexts];
  MojoResult ready_results[kMaxReadyContexts];
  MojoHandleSignalsState ready_states[kMaxReadyContexts];
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(0u, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[0]);
  EXPECT_TRUE(ready_states[0].satisfied_signals & MOJO_HANDLE_SIGNAL_WRITABLE);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
}

TEST_F(WatcherTest, ArmWithWatchAlreadyUnsatisfiable) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateTrap(&ExpectOnlyCancel, nullptr, &w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED, 0));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));

  // |b| is closed and never wrote any messages, so |a| won't be readable again.
  // MojoArmWatcher() should fail, incidcating as much.
  constexpr size_t kMaxReadyContexts = 10;
  uint32_t num_ready_contexts = kMaxReadyContexts;
  uintptr_t ready_contexts[kMaxReadyContexts];
  MojoResult ready_results[kMaxReadyContexts];
  MojoHandleSignalsState ready_states[kMaxReadyContexts];
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(0u, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, ready_results[0]);
  EXPECT_TRUE(ready_states[0].satisfied_signals &
              MOJO_HANDLE_SIGNAL_PEER_CLOSED);
  EXPECT_FALSE(ready_states[0].satisfiable_signals &
               MOJO_HANDLE_SIGNAL_READABLE);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
}

TEST_F(WatcherTest, MultipleWatches) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  base::WaitableEvent a_event(base::WaitableEvent::ResetPolicy::MANUAL,
                              base::WaitableEvent::InitialState::NOT_SIGNALED);
  base::WaitableEvent b_event(base::WaitableEvent::ResetPolicy::MANUAL,
                              base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  int num_a_notifications = 0;
  int num_b_notifications = 0;
  auto notify_callback =
      base::Bind([](base::WaitableEvent* event, int* notification_count,
                    MojoResult result, MojoHandleSignalsState state) {
        *notification_count += 1;
        EXPECT_EQ(MOJO_RESULT_OK, result);
        event->Signal();
      });
  uintptr_t readable_a_context = helper.CreateContext(
      base::Bind(notify_callback, &a_event, &num_a_notifications));
  uintptr_t readable_b_context = helper.CreateContext(
      base::Bind(notify_callback, &b_event, &num_b_notifications));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));

  // Add two independent watch contexts to watch for |a| or |b| readability.
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, b, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_b_context));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  const char kMessage1[] = "things are happening";
  const char kMessage2[] = "ok. ok. ok. ok.";
  const char kMessage3[] = "plz wake up";

  // Writing to |b| should signal |a|'s watch.
  WriteMessage(b, kMessage1);
  a_event.Wait();
  a_event.Reset();

  // Subsequent messages on |b| should not trigger another notification.
  WriteMessage(b, kMessage2);
  WriteMessage(b, kMessage3);

  // Messages on |a| also shouldn't trigger |b|'s notification, since the
  // watcher should be disarmed by now.
  WriteMessage(a, kMessage1);
  WriteMessage(a, kMessage2);
  WriteMessage(a, kMessage3);

  // Arming should fail. Since we only ask for at most one context's information
  // that's all we should get back. Which one we get is unspecified.
  constexpr size_t kMaxReadyContexts = 10;
  uint32_t num_ready_contexts = 1;
  uintptr_t ready_contexts[kMaxReadyContexts];
  MojoResult ready_results[kMaxReadyContexts];
  MojoHandleSignalsState ready_states[kMaxReadyContexts];
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_TRUE(ready_contexts[0] == readable_a_context ||
              ready_contexts[0] == readable_b_context);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[0]);
  EXPECT_TRUE(ready_states[0].satisfied_signals & MOJO_HANDLE_SIGNAL_WRITABLE);

  // Now try arming again, verifying that both contexts are returned.
  num_ready_contexts = kMaxReadyContexts;
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(2u, num_ready_contexts);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[0]);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[1]);
  EXPECT_TRUE(ready_states[0].satisfied_signals & MOJO_HANDLE_SIGNAL_WRITABLE);
  EXPECT_TRUE(ready_states[1].satisfied_signals & MOJO_HANDLE_SIGNAL_WRITABLE);
  EXPECT_TRUE((ready_contexts[0] == readable_a_context &&
               ready_contexts[1] == readable_b_context) ||
              (ready_contexts[0] == readable_b_context &&
               ready_contexts[1] == readable_a_context));

  // Flush out the test messages so we should be able to successfully rearm.
  EXPECT_EQ(kMessage1, ReadMessage(a));
  EXPECT_EQ(kMessage2, ReadMessage(a));
  EXPECT_EQ(kMessage3, ReadMessage(a));
  EXPECT_EQ(kMessage1, ReadMessage(b));
  EXPECT_EQ(kMessage2, ReadMessage(b));
  EXPECT_EQ(kMessage3, ReadMessage(b));

  // Add a watch which is always satisfied, so we can't arm. Arming should fail
  // with only this new watch's information.
  uintptr_t writable_c_context = helper.CreateContext(base::Bind(
      [](MojoResult result, MojoHandleSignalsState state) { NOTREACHED(); }));
  MojoHandle c, d;
  CreateMessagePipe(&c, &d);

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, c, MOJO_HANDLE_SIGNAL_WRITABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, writable_c_context));
  num_ready_contexts = kMaxReadyContexts;
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            MojoArmWatcher(w, &num_ready_contexts, ready_contexts,
                           ready_results, ready_states));
  EXPECT_EQ(1u, num_ready_contexts);
  EXPECT_EQ(writable_c_context, ready_contexts[0]);
  EXPECT_EQ(MOJO_RESULT_OK, ready_results[0]);
  EXPECT_TRUE(ready_states[0].satisfied_signals & MOJO_HANDLE_SIGNAL_WRITABLE);

  // Cancel the new watch and arming should succeed once again.
  EXPECT_EQ(MOJO_RESULT_OK, MojoCancelWatch(w, writable_c_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(c));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(d));
}

TEST_F(WatcherTest, NotifyOtherFromNotificationCallback) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  static const char kTestMessageToA[] = "hello a";
  static const char kTestMessageToB[] = "hello b";

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  WatchHelper helper;
  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));

  uintptr_t readable_a_context = helper.CreateContext(base::Bind(
      [](MojoHandle w, MojoHandle a, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        EXPECT_EQ("hello a", ReadMessage(a));

        // Re-arm the watcher and signal |b|.
        EXPECT_EQ(MOJO_RESULT_OK,
                  MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));
        WriteMessage(a, kTestMessageToB);
      },
      w, a));

  uintptr_t readable_b_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, MojoHandle w, MojoHandle b,
         MojoResult result, MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        EXPECT_EQ(kTestMessageToB, ReadMessage(b));
        EXPECT_EQ(MOJO_RESULT_OK,
                  MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));
        event->Signal();
      },
      &event, w, b));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, b, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_b_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  // Send a message to |a|. The relevant watch context should be notified, and
  // should in turn send a message to |b|, waking up the other context. The
  // second context signals |event|.
  WriteMessage(b, kTestMessageToA);
  event.Wait();
}

TEST_F(WatcherTest, NotifySelfFromNotificationCallback) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  static const char kTestMessageToA[] = "hello a";

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  WatchHelper helper;
  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));

  int expected_notifications = 10;
  uintptr_t readable_a_context = helper.CreateContext(base::Bind(
      [](int* expected_count, MojoHandle w, MojoHandle a, MojoHandle b,
         base::WaitableEvent* event, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        EXPECT_EQ("hello a", ReadMessage(a));

        EXPECT_GT(*expected_count, 0);
        *expected_count -= 1;
        if (*expected_count == 0) {
          event->Signal();
          return;
        } else {
          // Re-arm the watcher and signal |a| again.
          EXPECT_EQ(MOJO_RESULT_OK,
                    MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));
          WriteMessage(b, kTestMessageToA);
        }
      },
      &expected_notifications, w, a, b, &event));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  // Send a message to |a|. When the watch above is notified, it will rearm and
  // send another message to |a|. This will happen until
  // |expected_notifications| reaches 0.
  WriteMessage(b, kTestMessageToA);
  event.Wait();
}

TEST_F(WatcherTest, ImplicitCancelOtherFromNotificationCallback) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  MojoHandle c, d;
  CreateMessagePipe(&c, &d);

  static const char kTestMessageToA[] = "hi a";
  static const char kTestMessageToC[] = "hi c";

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  WatchHelper helper;
  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));

  uintptr_t readable_a_context = helper.CreateContextWithCancel(
      base::Bind([](MojoResult result, MojoHandleSignalsState state) {
        NOTREACHED();
      }),
      base::Bind([](base::WaitableEvent* event) { event->Signal(); }, &event));

  uintptr_t readable_c_context = helper.CreateContext(base::Bind(
      [](MojoHandle w, MojoHandle a, MojoHandle b, MojoHandle c,
         MojoResult result, MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        EXPECT_EQ(kTestMessageToC, ReadMessage(c));

        // Now rearm the watcher.
        EXPECT_EQ(MOJO_RESULT_OK,
                  MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

        // Must result in exactly ONE notification on the above context, for
        // CANCELLED only. Because we cannot dispatch notifications until the
        // stack unwinds, and because we must never dispatch non-cancellation
        // notifications for a handle once it's been closed, we must be certain
        // that cancellation due to closure preemptively invalidates any
        // pending non-cancellation notifications queued on the current
        // RequestContext, such as the one resulting from the WriteMessage here.
        WriteMessage(b, kTestMessageToA);
        EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));

        // Rearming should be fine since |a|'s watch should already be
        // implicitly cancelled (even though the notification will not have
        // been invoked yet.)
        EXPECT_EQ(MOJO_RESULT_OK,
                  MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

        // Nothing interesting should happen as a result of this.
        EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
      },
      w, a, b, c));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, c, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_c_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  WriteMessage(d, kTestMessageToC);
  event.Wait();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(c));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(d));
}

TEST_F(WatcherTest, ExplicitCancelOtherFromNotificationCallback) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  MojoHandle c, d;
  CreateMessagePipe(&c, &d);

  static const char kTestMessageToA[] = "hi a";
  static const char kTestMessageToC[] = "hi c";

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  WatchHelper helper;
  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));

  uintptr_t readable_a_context = helper.CreateContext(base::Bind(
      [](MojoResult result, MojoHandleSignalsState state) { NOTREACHED(); }));

  uintptr_t readable_c_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, uintptr_t readable_a_context, MojoHandle w,
         MojoHandle a, MojoHandle b, MojoHandle c, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        EXPECT_EQ(kTestMessageToC, ReadMessage(c));

        // Now rearm the watcher.
        EXPECT_EQ(MOJO_RESULT_OK,
                  MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

        // Should result in no notifications on the above context, because the
        // watch will have been cancelled by the time the notification callback
        // can execute.
        WriteMessage(b, kTestMessageToA);
        WriteMessage(b, kTestMessageToA);
        EXPECT_EQ(MOJO_RESULT_OK, MojoCancelWatch(w, readable_a_context));

        // Rearming should be fine now.
        EXPECT_EQ(MOJO_RESULT_OK,
                  MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

        // Nothing interesting should happen as a result of these.
        EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
        EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));

        event->Signal();
      },
      &event, readable_a_context, w, a, b, c));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, c, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_c_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  WriteMessage(d, kTestMessageToC);
  event.Wait();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(c));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(d));
}

TEST_F(WatcherTest, NestedCancellation) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  MojoHandle c, d;
  CreateMessagePipe(&c, &d);

  static const char kTestMessageToA[] = "hey a";
  static const char kTestMessageToC[] = "hey c";
  static const char kTestMessageToD[] = "hey d";

  // This is a tricky test. It establishes a watch on |b| using one watcher and
  // watches on |c| and |d| using another watcher.
  //
  // A message is written to |d| to wake up |c|'s watch, and the notification
  // handler for that event does the following:
  //   1. Writes to |a| to eventually wake up |b|'s watcher.
  //   2. Rearms |c|'s watcher.
  //   3. Writes to |d| to eventually wake up |c|'s watcher again.
  //
  // Meanwhile, |b|'s watch notification handler cancels |c|'s watch altogether
  // before writing to |c| to wake up |d|.
  //
  // The net result should be that |c|'s context only gets notified once (from
  // the first write to |d| above) and everyone else gets notified as expected.

  MojoHandle b_watcher;
  MojoHandle cd_watcher;
  WatchHelper helper;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&b_watcher));
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&cd_watcher));

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  uintptr_t readable_d_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, MojoHandle d, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        EXPECT_EQ(kTestMessageToD, ReadMessage(d));
        event->Signal();
      },
      &event, d));

  static int num_expected_c_notifications = 1;
  uintptr_t readable_c_context = helper.CreateContext(base::Bind(
      [](MojoHandle cd_watcher, MojoHandle a, MojoHandle c, MojoHandle d,
         MojoResult result, MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        EXPECT_GT(num_expected_c_notifications--, 0);

        // Trigger an eventual |readable_b_context| notification.
        WriteMessage(a, kTestMessageToA);

        EXPECT_EQ(kTestMessageToC, ReadMessage(c));
        EXPECT_EQ(MOJO_RESULT_OK, MojoArmWatcher(cd_watcher, nullptr, nullptr,
                                                 nullptr, nullptr));

        // Trigger another eventual |readable_c_context| notification.
        WriteMessage(d, kTestMessageToC);
      },
      cd_watcher, a, c, d));

  uintptr_t readable_b_context = helper.CreateContext(base::Bind(
      [](MojoHandle cd_watcher, uintptr_t readable_c_context, MojoHandle c,
         MojoResult result, MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK,
                  MojoCancelWatch(cd_watcher, readable_c_context));

        EXPECT_EQ(MOJO_RESULT_OK, MojoArmWatcher(cd_watcher, nullptr, nullptr,
                                                 nullptr, nullptr));

        WriteMessage(c, kTestMessageToD);
      },
      cd_watcher, readable_c_context, c));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(b_watcher, b, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_b_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(cd_watcher, c, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_c_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(cd_watcher, d, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_d_context));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(b_watcher, nullptr, nullptr, nullptr, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(cd_watcher, nullptr, nullptr, nullptr, nullptr));

  WriteMessage(d, kTestMessageToC);
  event.Wait();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(cd_watcher));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b_watcher));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(c));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(d));
}

TEST_F(WatcherTest, CancelSelfInNotificationCallback) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  static const char kTestMessageToA[] = "hey a";

  MojoHandle w;
  WatchHelper helper;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  static uintptr_t readable_a_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, MojoHandle w, MojoHandle a,
         MojoResult result, MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);

        // There should be no problem cancelling this watch from its own
        // notification invocation.
        EXPECT_EQ(MOJO_RESULT_OK, MojoCancelWatch(w, readable_a_context));
        EXPECT_EQ(kTestMessageToA, ReadMessage(a));

        // Arming should fail because there are no longer any registered
        // watches on the watcher.
        EXPECT_EQ(MOJO_RESULT_NOT_FOUND,
                  MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

        // And closing |a| should be fine (and should not invoke this
        // notification with MOJO_RESULT_CANCELLED) for the same reason.
        EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));

        event->Signal();
      },
      &event, w, a));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  WriteMessage(b, kTestMessageToA);
  event.Wait();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
}

TEST_F(WatcherTest, CloseWatcherInNotificationCallback) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  static const char kTestMessageToA1[] = "hey a";
  static const char kTestMessageToA2[] = "hey a again";

  MojoHandle w;
  WatchHelper helper;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  uintptr_t readable_a_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, MojoHandle w, MojoHandle a, MojoHandle b,
         MojoResult result, MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        EXPECT_EQ(kTestMessageToA1, ReadMessage(a));
        EXPECT_EQ(MOJO_RESULT_OK,
                  MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

        // There should be no problem closing this watcher from its own
        // notification callback.
        EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));

        // And these should not trigger more notifications, because |w| has been
        // closed already.
        WriteMessage(b, kTestMessageToA2);
        EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
        EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));

        event->Signal();
      },
      &event, w, a, b));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  WriteMessage(b, kTestMessageToA1);
  event.Wait();
}

TEST_F(WatcherTest, CloseWatcherAfterImplicitCancel) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  static const char kTestMessageToA[] = "hey a";

  MojoHandle w;
  WatchHelper helper;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  uintptr_t readable_a_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, MojoHandle w, MojoHandle a,
         MojoResult result, MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        EXPECT_EQ(kTestMessageToA, ReadMessage(a));
        EXPECT_EQ(MOJO_RESULT_OK,
                  MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

        // This will cue up a notification for |MOJO_RESULT_CANCELLED|...
        EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));

        // ...but it should never fire because we close the watcher here.
        EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));

        event->Signal();
      },
      &event, w, a));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  WriteMessage(b, kTestMessageToA);
  event.Wait();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
}

TEST_F(WatcherTest, OtherThreadCancelDuringNotification) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  static const char kTestMessageToA[] = "hey a";

  MojoHandle w;
  WatchHelper helper;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));

  base::WaitableEvent wait_for_notification(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);

  base::WaitableEvent wait_for_cancellation(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);

  static bool callback_done = false;
  uintptr_t readable_a_context = helper.CreateContextWithCancel(
      base::Bind(
          [](base::WaitableEvent* wait_for_notification, MojoHandle w,
             MojoHandle a, MojoResult result, MojoHandleSignalsState state) {
            EXPECT_EQ(MOJO_RESULT_OK, result);
            EXPECT_EQ(kTestMessageToA, ReadMessage(a));

            wait_for_notification->Signal();

            // Give the other thread sufficient time to race with the completion
            // of this callback. There should be no race, since the cancellation
            // notification must be mutually exclusive to this notification.
            base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(1));

            callback_done = true;
          },
          &wait_for_notification, w, a),
      base::Bind(
          [](base::WaitableEvent* wait_for_cancellation) {
            EXPECT_TRUE(callback_done);
            wait_for_cancellation->Signal();
          },
          &wait_for_cancellation));

  ThreadedRunner runner(base::Bind(
      [](base::WaitableEvent* wait_for_notification,
         base::WaitableEvent* wait_for_cancellation, MojoHandle w,
         uintptr_t readable_a_context) {
        wait_for_notification->Wait();

        // Cancel the watch while the notification is still running.
        EXPECT_EQ(MOJO_RESULT_OK, MojoCancelWatch(w, readable_a_context));

        wait_for_cancellation->Wait();

        EXPECT_TRUE(callback_done);
      },
      &wait_for_notification, &wait_for_cancellation, w, readable_a_context));
  runner.Start();

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  WriteMessage(b, kTestMessageToA);
  runner.Join();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
}

TEST_F(WatcherTest, WatchesCancelEachOtherFromNotifications) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  static const char kTestMessageToA[] = "hey a";
  static const char kTestMessageToB[] = "hey b";

  base::WaitableEvent wait_for_a_to_notify(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  base::WaitableEvent wait_for_b_to_notify(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  base::WaitableEvent wait_for_a_to_cancel(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  base::WaitableEvent wait_for_b_to_cancel(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);

  MojoHandle a_watcher;
  MojoHandle b_watcher;
  WatchHelper helper;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&a_watcher));
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&b_watcher));

  // We set up two watchers, one on |a| and one on |b|. They cancel each other
  // from within their respective watch notifications. This should be safe,
  // i.e., it should not deadlock, in spite of the fact that we also guarantee
  // mutually exclusive notification execution (including cancellations) on any
  // given watch.
  bool a_cancelled = false;
  bool b_cancelled = false;
  static uintptr_t readable_b_context;
  uintptr_t readable_a_context = helper.CreateContextWithCancel(
      base::Bind(
          [](base::WaitableEvent* wait_for_a_to_notify,
             base::WaitableEvent* wait_for_b_to_notify, MojoHandle b_watcher,
             MojoHandle a, MojoResult result, MojoHandleSignalsState state) {
            EXPECT_EQ(MOJO_RESULT_OK, result);
            EXPECT_EQ(kTestMessageToA, ReadMessage(a));
            wait_for_a_to_notify->Signal();
            wait_for_b_to_notify->Wait();
            EXPECT_EQ(MOJO_RESULT_OK,
                      MojoCancelWatch(b_watcher, readable_b_context));
            EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b_watcher));
          },
          &wait_for_a_to_notify, &wait_for_b_to_notify, b_watcher, a),
      base::Bind(
          [](base::WaitableEvent* wait_for_a_to_cancel,
             base::WaitableEvent* wait_for_b_to_cancel, bool* a_cancelled) {
            *a_cancelled = true;
            wait_for_a_to_cancel->Signal();
            wait_for_b_to_cancel->Wait();
          },
          &wait_for_a_to_cancel, &wait_for_b_to_cancel, &a_cancelled));

  readable_b_context = helper.CreateContextWithCancel(
      base::Bind(
          [](base::WaitableEvent* wait_for_a_to_notify,
             base::WaitableEvent* wait_for_b_to_notify,
             uintptr_t readable_a_context, MojoHandle a_watcher, MojoHandle b,
             MojoResult result, MojoHandleSignalsState state) {
            EXPECT_EQ(MOJO_RESULT_OK, result);
            EXPECT_EQ(kTestMessageToB, ReadMessage(b));
            wait_for_b_to_notify->Signal();
            wait_for_a_to_notify->Wait();
            EXPECT_EQ(MOJO_RESULT_OK,
                      MojoCancelWatch(a_watcher, readable_a_context));
            EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a_watcher));
          },
          &wait_for_a_to_notify, &wait_for_b_to_notify, readable_a_context,
          a_watcher, b),
      base::Bind(
          [](base::WaitableEvent* wait_for_a_to_cancel,
             base::WaitableEvent* wait_for_b_to_cancel, bool* b_cancelled) {
            *b_cancelled = true;
            wait_for_b_to_cancel->Signal();
            wait_for_a_to_cancel->Wait();
          },
          &wait_for_a_to_cancel, &wait_for_b_to_cancel, &b_cancelled));

  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(a_watcher, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(a_watcher, nullptr, nullptr, nullptr, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(b_watcher, b, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_b_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(b_watcher, nullptr, nullptr, nullptr, nullptr));

  ThreadedRunner runner(
      base::Bind([](MojoHandle b) { WriteMessage(b, kTestMessageToA); }, b));
  runner.Start();

  WriteMessage(a, kTestMessageToB);

  wait_for_a_to_cancel.Wait();
  wait_for_b_to_cancel.Wait();
  runner.Join();

  EXPECT_TRUE(a_cancelled);
  EXPECT_TRUE(b_cancelled);

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
}

TEST_F(WatcherTest, AlwaysCancel) {
  // Basic sanity check to ensure that all possible ways to cancel a watch
  // result in a final MOJO_RESULT_CANCELLED notification.

  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  MojoHandle w;
  WatchHelper helper;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  const base::Closure signal_event =
      base::Bind(&base::WaitableEvent::Signal, base::Unretained(&event));

  // Cancel via |MojoCancelWatch()|.
  uintptr_t context = helper.CreateContextWithCancel(
      WatchHelper::ContextCallback(), signal_event);
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED, context));
  EXPECT_EQ(MOJO_RESULT_OK, MojoCancelWatch(w, context));
  event.Wait();
  event.Reset();

  // Cancel by closing the watched handle.
  context = helper.CreateContextWithCancel(WatchHelper::ContextCallback(),
                                           signal_event);
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED, context));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
  event.Wait();
  event.Reset();

  // Cancel by closing the watcher handle.
  context = helper.CreateContextWithCancel(WatchHelper::ContextCallback(),
                                           signal_event);
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, b, MOJO_HANDLE_SIGNAL_READABLE,
                                      MOJO_WATCH_CONDITION_SATISFIED, context));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  event.Wait();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
}

TEST_F(WatcherTest, ArmFailureCirculation) {
  // Sanity check to ensure that all ready handles will eventually be returned
  // over a finite number of calls to MojoArmWatcher().

  constexpr size_t kNumTestPipes = 100;
  constexpr size_t kNumTestHandles = kNumTestPipes * 2;
  MojoHandle handles[kNumTestHandles];

  // Create a bunch of pipes and make sure they're all readable.
  for (size_t i = 0; i < kNumTestPipes; ++i) {
    CreateMessagePipe(&handles[i], &handles[i + kNumTestPipes]);
    WriteMessage(handles[i], "hey");
    WriteMessage(handles[i + kNumTestPipes], "hay");
    WaitForSignals(handles[i], MOJO_HANDLE_SIGNAL_READABLE);
    WaitForSignals(handles[i + kNumTestPipes], MOJO_HANDLE_SIGNAL_READABLE);
  }

  // Create a watcher and watch all of them.
  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateTrap(&ExpectOnlyCancel, nullptr, &w));
  for (size_t i = 0; i < kNumTestHandles; ++i) {
    EXPECT_EQ(MOJO_RESULT_OK,
              MojoWatch(w, handles[i], MOJO_HANDLE_SIGNAL_READABLE,
                        MOJO_WATCH_CONDITION_SATISFIED, i));
  }

  // Keep trying to arm |w| until every watch gets an entry in |ready_contexts|.
  // If MojoArmWatcher() is well-behaved, this should terminate eventually.
  std::set<uintptr_t> ready_contexts;
  while (ready_contexts.size() < kNumTestHandles) {
    uint32_t num_ready_contexts = 1;
    uintptr_t ready_context;
    MojoResult ready_result;
    MojoHandleSignalsState ready_state;
    EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
              MojoArmWatcher(w, &num_ready_contexts, &ready_context,
                             &ready_result, &ready_state));
    EXPECT_EQ(1u, num_ready_contexts);
    EXPECT_EQ(MOJO_RESULT_OK, ready_result);
    ready_contexts.insert(ready_context);
  }

  for (size_t i = 0; i < kNumTestHandles; ++i)
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(handles[i]));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
}

TEST_F(WatcherTest, WatchNotSatisfied) {
  MojoHandle a, b;
  CreateMessagePipe(&a, &b);

  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  WatchHelper helper;
  const uintptr_t readable_a_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        event->Signal();
      },
      &event));

  MojoHandle w;
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_WATCH_CONDITION_SATISFIED, readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  const char kMessage[] = "this is not a message";

  WriteMessage(b, kMessage);
  event.Wait();

  // Now we know |a| is readable. Cancel the watch and watch for the
  // not-readable state.
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  const uintptr_t not_readable_a_context = helper.CreateContext(base::Bind(
      [](base::WaitableEvent* event, MojoResult result,
         MojoHandleSignalsState state) {
        EXPECT_EQ(MOJO_RESULT_OK, result);
        event->Signal();
      },
      &event));
  EXPECT_EQ(MOJO_RESULT_OK, helper.CreateWatcher(&w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWatch(w, a, MOJO_HANDLE_SIGNAL_READABLE,
                                      MOJO_WATCH_CONDITION_NOT_SATISFIED,
                                      not_readable_a_context));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoArmWatcher(w, nullptr, nullptr, nullptr, nullptr));

  // This should not block, because the event should be signaled by
  // |not_readable_a_context| when we read the only available message off of
  // |a|.
  event.Reset();
  EXPECT_EQ(kMessage, ReadMessage(a));
  event.Wait();

  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(w));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
}

base::Closure g_do_random_thing_callback;

void ReadAllMessages(const MojoTrapEvent* event) {
  if (event->result == MOJO_RESULT_OK) {
    MojoHandle handle = static_cast<MojoHandle>(event->trigger_context);
    MojoMessageHandle message;
    while (MojoReadMessage(handle, nullptr, &message) == MOJO_RESULT_OK)
      MojoDestroyMessage(message);
  }

  constexpr size_t kNumRandomThingsToDoOnNotify = 5;
  for (size_t i = 0; i < kNumRandomThingsToDoOnNotify; ++i)
    g_do_random_thing_callback.Run();
}

MojoHandle RandomHandle(MojoHandle* handles, size_t size) {
  return handles[base::RandInt(0, static_cast<int>(size) - 1)];
}

void DoRandomThing(MojoHandle* watchers,
                   size_t num_watchers,
                   MojoHandle* watched_handles,
                   size_t num_watched_handles) {
  switch (base::RandInt(0, 10)) {
    case 0:
      MojoClose(RandomHandle(watchers, num_watchers));
      break;
    case 1:
      MojoClose(RandomHandle(watched_handles, num_watched_handles));
      break;
    case 2:
    case 3:
    case 4: {
      MojoMessageHandle message;
      ASSERT_EQ(MOJO_RESULT_OK, MojoCreateMessage(nullptr, &message));
      ASSERT_EQ(MOJO_RESULT_OK,
                MojoSetMessageContext(message, 1, nullptr, nullptr, nullptr));
      MojoWriteMessage(RandomHandle(watched_handles, num_watched_handles),
                       message, nullptr);
      break;
    }
    case 5:
    case 6: {
      MojoHandle w = RandomHandle(watchers, num_watchers);
      MojoHandle h = RandomHandle(watched_handles, num_watched_handles);
      MojoWatch(w, h, MOJO_HANDLE_SIGNAL_READABLE,
                MOJO_WATCH_CONDITION_SATISFIED, static_cast<uintptr_t>(h));
      break;
    }
    case 7:
    case 8: {
      uint32_t num_ready_contexts = 1;
      uintptr_t ready_context;
      MojoResult ready_result;
      MojoHandleSignalsState ready_state;
      if (MojoArmWatcher(RandomHandle(watchers, num_watchers),
                         &num_ready_contexts, &ready_context, &ready_result,
                         &ready_state) == MOJO_RESULT_FAILED_PRECONDITION &&
          ready_result == MOJO_RESULT_OK) {
        MojoTrapEvent event;
        event.struct_size = sizeof(event);
        event.trigger_context = ready_context;
        event.result = ready_result;
        event.signals_state = ready_state;
        event.flags = MOJO_TRAP_EVENT_FLAG_NONE;
        ReadAllMessages(&event);
      }
      break;
    }
    case 9:
    case 10: {
      MojoHandle w = RandomHandle(watchers, num_watchers);
      MojoHandle h = RandomHandle(watched_handles, num_watched_handles);
      MojoCancelWatch(w, static_cast<uintptr_t>(h));
      break;
    }
    default:
      NOTREACHED();
      break;
  }
}

TEST_F(WatcherTest, ConcurrencyStressTest) {
  // Regression test for https://crbug.com/740044. Exercises racy usage of the
  // watcher API to weed out potential crashes.

  constexpr size_t kNumWatchers = 50;
  constexpr size_t kNumWatchedHandles = 50;
  static_assert(kNumWatchedHandles % 2 == 0, "Invalid number of test handles.");

  constexpr size_t kNumThreads = 10;
  static constexpr size_t kNumOperationsPerThread = 400;

  MojoHandle watchers[kNumWatchers];
  MojoHandle watched_handles[kNumWatchedHandles];
  g_do_random_thing_callback =
      base::Bind(&DoRandomThing, watchers, kNumWatchers, watched_handles,
                 kNumWatchedHandles);

  for (size_t i = 0; i < kNumWatchers; ++i)
    MojoCreateTrap(&ReadAllMessages, nullptr, &watchers[i]);
  for (size_t i = 0; i < kNumWatchedHandles; i += 2)
    CreateMessagePipe(&watched_handles[i], &watched_handles[i + 1]);

  std::unique_ptr<ThreadedRunner> threads[kNumThreads];
  auto runner_callback = base::Bind([]() {
    for (size_t i = 0; i < kNumOperationsPerThread; ++i)
      g_do_random_thing_callback.Run();
  });
  for (size_t i = 0; i < kNumThreads; ++i) {
    threads[i] = std::make_unique<ThreadedRunner>(runner_callback);
    threads[i]->Start();
  }
  for (size_t i = 0; i < kNumThreads; ++i)
    threads[i]->Join();
  for (size_t i = 0; i < kNumWatchers; ++i)
    MojoClose(watchers[i]);
  for (size_t i = 0; i < kNumWatchedHandles; ++i)
    MojoClose(watched_handles[i]);
}

}  // namespace
}  // namespace edk
}  // namespace mojo
