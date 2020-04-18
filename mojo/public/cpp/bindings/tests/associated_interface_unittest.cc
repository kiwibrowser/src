// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr_info.h"
#include "mojo/public/cpp/bindings/associated_interface_request.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/lib/multiplex_router.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/bindings/thread_safe_interface_ptr.h"
#include "mojo/public/interfaces/bindings/tests/ping_service.mojom.h"
#include "mojo/public/interfaces/bindings/tests/test_associated_interfaces.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

using mojo::internal::MultiplexRouter;

class IntegerSenderImpl : public IntegerSender {
 public:
  explicit IntegerSenderImpl(AssociatedInterfaceRequest<IntegerSender> request)
      : binding_(this, std::move(request)) {}

  ~IntegerSenderImpl() override {}

  void set_notify_send_method_called(
      const base::Callback<void(int32_t)>& callback) {
    notify_send_method_called_ = callback;
  }

  void Echo(int32_t value, const EchoCallback& callback) override {
    callback.Run(value);
  }
  void Send(int32_t value) override { notify_send_method_called_.Run(value); }

  AssociatedBinding<IntegerSender>* binding() { return &binding_; }

  void set_connection_error_handler(const base::Closure& handler) {
    binding_.set_connection_error_handler(handler);
  }

 private:
  AssociatedBinding<IntegerSender> binding_;
  base::Callback<void(int32_t)> notify_send_method_called_;
};

class IntegerSenderConnectionImpl : public IntegerSenderConnection {
 public:
  explicit IntegerSenderConnectionImpl(
      InterfaceRequest<IntegerSenderConnection> request)
      : binding_(this, std::move(request)) {}

  ~IntegerSenderConnectionImpl() override {}

  void GetSender(AssociatedInterfaceRequest<IntegerSender> sender) override {
    IntegerSenderImpl* sender_impl = new IntegerSenderImpl(std::move(sender));
    sender_impl->set_connection_error_handler(
        base::Bind(&DeleteSender, sender_impl));
  }

  void AsyncGetSender(const AsyncGetSenderCallback& callback) override {
    IntegerSenderAssociatedPtrInfo ptr_info;
    auto request = MakeRequest(&ptr_info);
    GetSender(std::move(request));
    callback.Run(std::move(ptr_info));
  }

  Binding<IntegerSenderConnection>* binding() { return &binding_; }

 private:
  static void DeleteSender(IntegerSenderImpl* sender) { delete sender; }

  Binding<IntegerSenderConnection> binding_;
};

class AssociatedInterfaceTest : public testing::Test {
 public:
  AssociatedInterfaceTest()
      : main_runner_(base::ThreadTaskRunnerHandle::Get()) {}
  ~AssociatedInterfaceTest() override { base::RunLoop().RunUntilIdle(); }

  void PumpMessages() { base::RunLoop().RunUntilIdle(); }

  template <typename T>
  AssociatedInterfacePtrInfo<T> EmulatePassingAssociatedPtrInfo(
      AssociatedInterfacePtrInfo<T> ptr_info,
      scoped_refptr<MultiplexRouter> source,
      scoped_refptr<MultiplexRouter> target) {
    ScopedInterfaceEndpointHandle handle = ptr_info.PassHandle();
    CHECK(handle.pending_association());
    auto id = source->AssociateInterface(std::move(handle));
    return AssociatedInterfacePtrInfo<T>(target->CreateLocalEndpointHandle(id),
                                         ptr_info.version());
  }

  void CreateRouterPair(scoped_refptr<MultiplexRouter>* router0,
                        scoped_refptr<MultiplexRouter>* router1) {
    MessagePipe pipe;
    *router0 = new MultiplexRouter(std::move(pipe.handle0),
                                   MultiplexRouter::MULTI_INTERFACE, true,
                                   main_runner_);
    *router1 = new MultiplexRouter(std::move(pipe.handle1),
                                   MultiplexRouter::MULTI_INTERFACE, false,
                                   main_runner_);
  }

  void CreateIntegerSenderWithExistingRouters(
      scoped_refptr<MultiplexRouter> router0,
      IntegerSenderAssociatedPtrInfo* ptr_info0,
      scoped_refptr<MultiplexRouter> router1,
      IntegerSenderAssociatedRequest* request1) {
    *request1 = MakeRequest(ptr_info0);
    *ptr_info0 = EmulatePassingAssociatedPtrInfo(std::move(*ptr_info0), router1,
                                                 router0);
  }

  void CreateIntegerSender(IntegerSenderAssociatedPtrInfo* ptr_info,
                           IntegerSenderAssociatedRequest* request) {
    scoped_refptr<MultiplexRouter> router0;
    scoped_refptr<MultiplexRouter> router1;
    CreateRouterPair(&router0, &router1);
    CreateIntegerSenderWithExistingRouters(router1, ptr_info, router0, request);
  }

  // Okay to call from any thread.
  void QuitRunLoop(base::RunLoop* run_loop) {
    if (main_runner_->RunsTasksInCurrentSequence()) {
      run_loop->Quit();
    } else {
      main_runner_->PostTask(
          FROM_HERE,
          base::Bind(&AssociatedInterfaceTest::QuitRunLoop,
                     base::Unretained(this), base::Unretained(run_loop)));
    }
  }

 private:
  base::test::ScopedTaskEnvironment task_environment;
  scoped_refptr<base::SequencedTaskRunner> main_runner_;
};

void DoSetFlagAndRunClosure(bool* flag, const base::Closure& closure) {
  *flag = true;
  closure.Run();
}

void DoExpectValueSetFlagAndRunClosure(int32_t expected_value,
                                       bool* flag,
                                       const base::Closure& closure,
                                       int32_t value) {
  EXPECT_EQ(expected_value, value);
  DoSetFlagAndRunClosure(flag, closure);
}

base::Closure SetFlagAndRunClosure(bool* flag, const base::Closure& closure) {
  return base::Bind(&DoSetFlagAndRunClosure, flag, closure);
}

base::Callback<void(int32_t)> ExpectValueSetFlagAndRunClosure(
    int32_t expected_value,
    bool* flag,
    const base::Closure& closure) {
  return base::Bind(
      &DoExpectValueSetFlagAndRunClosure, expected_value, flag, closure);
}

void Fail() {
  FAIL() << "Unexpected connection error";
}

TEST_F(AssociatedInterfaceTest, InterfacesAtBothEnds) {
  // Bind to the same pipe two associated interfaces, whose implementation lives
  // at different ends. Test that the two don't interfere with each other.

  scoped_refptr<MultiplexRouter> router0;
  scoped_refptr<MultiplexRouter> router1;
  CreateRouterPair(&router0, &router1);

  AssociatedInterfaceRequest<IntegerSender> request;
  IntegerSenderAssociatedPtrInfo ptr_info;
  CreateIntegerSenderWithExistingRouters(router1, &ptr_info, router0, &request);

  IntegerSenderImpl impl0(std::move(request));
  AssociatedInterfacePtr<IntegerSender> ptr0;
  ptr0.Bind(std::move(ptr_info));

  CreateIntegerSenderWithExistingRouters(router0, &ptr_info, router1, &request);

  IntegerSenderImpl impl1(std::move(request));
  AssociatedInterfacePtr<IntegerSender> ptr1;
  ptr1.Bind(std::move(ptr_info));

  base::RunLoop run_loop, run_loop2;
  bool ptr0_callback_run = false;
  ptr0->Echo(123, ExpectValueSetFlagAndRunClosure(123, &ptr0_callback_run,
                                                  run_loop.QuitClosure()));

  bool ptr1_callback_run = false;
  ptr1->Echo(456, ExpectValueSetFlagAndRunClosure(456, &ptr1_callback_run,
                                                  run_loop2.QuitClosure()));

  run_loop.Run();
  run_loop2.Run();
  EXPECT_TRUE(ptr0_callback_run);
  EXPECT_TRUE(ptr1_callback_run);

  bool ptr0_error_callback_run = false;
  base::RunLoop run_loop3;
  ptr0.set_connection_error_handler(
      SetFlagAndRunClosure(&ptr0_error_callback_run, run_loop3.QuitClosure()));

  impl0.binding()->Close();
  run_loop3.Run();
  EXPECT_TRUE(ptr0_error_callback_run);

  bool impl1_error_callback_run = false;
  base::RunLoop run_loop4;
  impl1.binding()->set_connection_error_handler(
      SetFlagAndRunClosure(&impl1_error_callback_run, run_loop4.QuitClosure()));

  ptr1.reset();
  run_loop4.Run();
  EXPECT_TRUE(impl1_error_callback_run);
}

class TestSender {
 public:
  TestSender()
      : task_runner_(base::CreateSequencedTaskRunnerWithTraits({})),
        next_sender_(nullptr),
        max_value_to_send_(-1) {}

  // The following three methods are called on the corresponding sender thread.
  void SetUp(IntegerSenderAssociatedPtrInfo ptr_info,
             TestSender* next_sender,
             int32_t max_value_to_send) {
    CHECK(task_runner()->RunsTasksInCurrentSequence());

    ptr_.Bind(std::move(ptr_info));
    next_sender_ = next_sender ? next_sender : this;
    max_value_to_send_ = max_value_to_send;
  }

  void Send(int32_t value) {
    CHECK(task_runner()->RunsTasksInCurrentSequence());

    if (value > max_value_to_send_)
      return;

    ptr_->Send(value);

    next_sender_->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&TestSender::Send, base::Unretained(next_sender_), ++value));
  }

  void TearDown() {
    CHECK(task_runner()->RunsTasksInCurrentSequence());

    ptr_.reset();
  }

  base::SequencedTaskRunner* task_runner() { return task_runner_.get(); }

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  TestSender* next_sender_;
  int32_t max_value_to_send_;

  AssociatedInterfacePtr<IntegerSender> ptr_;
};

class TestReceiver {
 public:
  TestReceiver()
      : task_runner_(base::CreateSequencedTaskRunnerWithTraits({})),
        expected_calls_(0) {}

  void SetUp(AssociatedInterfaceRequest<IntegerSender> request0,
             AssociatedInterfaceRequest<IntegerSender> request1,
             size_t expected_calls,
             const base::Closure& notify_finish) {
    CHECK(task_runner()->RunsTasksInCurrentSequence());

    impl0_.reset(new IntegerSenderImpl(std::move(request0)));
    impl0_->set_notify_send_method_called(
        base::Bind(&TestReceiver::SendMethodCalled, base::Unretained(this)));
    impl1_.reset(new IntegerSenderImpl(std::move(request1)));
    impl1_->set_notify_send_method_called(
        base::Bind(&TestReceiver::SendMethodCalled, base::Unretained(this)));

    expected_calls_ = expected_calls;
    notify_finish_ = notify_finish;
  }

  void TearDown() {
    CHECK(task_runner()->RunsTasksInCurrentSequence());

    impl0_.reset();
    impl1_.reset();
  }

  base::SequencedTaskRunner* task_runner() { return task_runner_.get(); }
  const std::vector<int32_t>& values() const { return values_; }

 private:
  void SendMethodCalled(int32_t value) {
    values_.push_back(value);

    if (values_.size() >= expected_calls_)
      notify_finish_.Run();
  }

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  size_t expected_calls_;

  std::unique_ptr<IntegerSenderImpl> impl0_;
  std::unique_ptr<IntegerSenderImpl> impl1_;

  std::vector<int32_t> values_;

  base::Closure notify_finish_;
};

class NotificationCounter {
 public:
  NotificationCounter(size_t total_count, const base::Closure& notify_finish)
      : total_count_(total_count),
        current_count_(0),
        notify_finish_(notify_finish) {}

  ~NotificationCounter() {}

  // Okay to call from any thread.
  void OnGotNotification() {
    bool finshed = false;
    {
      base::AutoLock locker(lock_);
      CHECK_LT(current_count_, total_count_);
      current_count_++;
      finshed = current_count_ == total_count_;
    }

    if (finshed)
      notify_finish_.Run();
  }

 private:
  base::Lock lock_;
  const size_t total_count_;
  size_t current_count_;
  base::Closure notify_finish_;
};

TEST_F(AssociatedInterfaceTest, MultiThreadAccess) {
  // Set up four associated interfaces on a message pipe. Use the inteface
  // pointers on four threads in parallel; run the interface implementations on
  // two threads. Test that multi-threaded access works.

  const int32_t kMaxValue = 1000;
  MessagePipe pipe;
  scoped_refptr<MultiplexRouter> router0;
  scoped_refptr<MultiplexRouter> router1;
  CreateRouterPair(&router0, &router1);

  AssociatedInterfaceRequest<IntegerSender> requests[4];
  IntegerSenderAssociatedPtrInfo ptr_infos[4];
  for (size_t i = 0; i < 4; ++i) {
    CreateIntegerSenderWithExistingRouters(router1, &ptr_infos[i], router0,
                                           &requests[i]);
  }

  TestSender senders[4];
  for (size_t i = 0; i < 4; ++i) {
    senders[i].task_runner()->PostTask(
        FROM_HERE, base::Bind(&TestSender::SetUp, base::Unretained(&senders[i]),
                              base::Passed(&ptr_infos[i]), nullptr,
                              kMaxValue * (i + 1) / 4));
  }

  base::RunLoop run_loop;
  TestReceiver receivers[2];
  NotificationCounter counter(
      2, base::Bind(&AssociatedInterfaceTest::QuitRunLoop,
                    base::Unretained(this), base::Unretained(&run_loop)));
  for (size_t i = 0; i < 2; ++i) {
    receivers[i].task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&TestReceiver::SetUp, base::Unretained(&receivers[i]),
                   base::Passed(&requests[2 * i]),
                   base::Passed(&requests[2 * i + 1]),
                   static_cast<size_t>(kMaxValue / 2),
                   base::Bind(&NotificationCounter::OnGotNotification,
                              base::Unretained(&counter))));
  }

  for (size_t i = 0; i < 4; ++i) {
    senders[i].task_runner()->PostTask(
        FROM_HERE, base::Bind(&TestSender::Send, base::Unretained(&senders[i]),
                              kMaxValue * i / 4 + 1));
  }

  run_loop.Run();

  for (size_t i = 0; i < 4; ++i) {
    base::RunLoop run_loop;
    senders[i].task_runner()->PostTaskAndReply(
        FROM_HERE,
        base::Bind(&TestSender::TearDown, base::Unretained(&senders[i])),
        base::Bind(&AssociatedInterfaceTest::QuitRunLoop,
                   base::Unretained(this), base::Unretained(&run_loop)));
    run_loop.Run();
  }

  for (size_t i = 0; i < 2; ++i) {
    base::RunLoop run_loop;
    receivers[i].task_runner()->PostTaskAndReply(
        FROM_HERE,
        base::Bind(&TestReceiver::TearDown, base::Unretained(&receivers[i])),
        base::Bind(&AssociatedInterfaceTest::QuitRunLoop,
                   base::Unretained(this), base::Unretained(&run_loop)));
    run_loop.Run();
  }

  EXPECT_EQ(static_cast<size_t>(kMaxValue / 2), receivers[0].values().size());
  EXPECT_EQ(static_cast<size_t>(kMaxValue / 2), receivers[1].values().size());

  std::vector<int32_t> all_values;
  all_values.insert(all_values.end(), receivers[0].values().begin(),
                    receivers[0].values().end());
  all_values.insert(all_values.end(), receivers[1].values().begin(),
                    receivers[1].values().end());

  std::sort(all_values.begin(), all_values.end());
  for (size_t i = 0; i < all_values.size(); ++i)
    ASSERT_EQ(static_cast<int32_t>(i + 1), all_values[i]);
}

TEST_F(AssociatedInterfaceTest, FIFO) {
  // Set up four associated interfaces on a message pipe. Use the inteface
  // pointers on four threads; run the interface implementations on two threads.
  // Take turns to make calls using the four pointers. Test that FIFO-ness is
  // preserved.

  const int32_t kMaxValue = 100;
  MessagePipe pipe;
  scoped_refptr<MultiplexRouter> router0;
  scoped_refptr<MultiplexRouter> router1;
  CreateRouterPair(&router0, &router1);

  AssociatedInterfaceRequest<IntegerSender> requests[4];
  IntegerSenderAssociatedPtrInfo ptr_infos[4];
  for (size_t i = 0; i < 4; ++i) {
    CreateIntegerSenderWithExistingRouters(router1, &ptr_infos[i], router0,
                                           &requests[i]);
  }

  TestSender senders[4];
  for (size_t i = 0; i < 4; ++i) {
    senders[i].task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&TestSender::SetUp, base::Unretained(&senders[i]),
                   base::Passed(&ptr_infos[i]),
                   base::Unretained(&senders[(i + 1) % 4]), kMaxValue));
  }

  base::RunLoop run_loop;
  TestReceiver receivers[2];
  NotificationCounter counter(
      2, base::Bind(&AssociatedInterfaceTest::QuitRunLoop,
                    base::Unretained(this), base::Unretained(&run_loop)));
  for (size_t i = 0; i < 2; ++i) {
    receivers[i].task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&TestReceiver::SetUp, base::Unretained(&receivers[i]),
                   base::Passed(&requests[2 * i]),
                   base::Passed(&requests[2 * i + 1]),
                   static_cast<size_t>(kMaxValue / 2),
                   base::Bind(&NotificationCounter::OnGotNotification,
                              base::Unretained(&counter))));
  }

  senders[0].task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&TestSender::Send, base::Unretained(&senders[0]), 1));

  run_loop.Run();

  for (size_t i = 0; i < 4; ++i) {
    base::RunLoop run_loop;
    senders[i].task_runner()->PostTaskAndReply(
        FROM_HERE,
        base::Bind(&TestSender::TearDown, base::Unretained(&senders[i])),
        base::Bind(&AssociatedInterfaceTest::QuitRunLoop,
                   base::Unretained(this), base::Unretained(&run_loop)));
    run_loop.Run();
  }

  for (size_t i = 0; i < 2; ++i) {
    base::RunLoop run_loop;
    receivers[i].task_runner()->PostTaskAndReply(
        FROM_HERE,
        base::Bind(&TestReceiver::TearDown, base::Unretained(&receivers[i])),
        base::Bind(&AssociatedInterfaceTest::QuitRunLoop,
                   base::Unretained(this), base::Unretained(&run_loop)));
    run_loop.Run();
  }

  EXPECT_EQ(static_cast<size_t>(kMaxValue / 2), receivers[0].values().size());
  EXPECT_EQ(static_cast<size_t>(kMaxValue / 2), receivers[1].values().size());

  for (size_t i = 0; i < 2; ++i) {
    for (size_t j = 1; j < receivers[i].values().size(); ++j)
      EXPECT_LT(receivers[i].values()[j - 1], receivers[i].values()[j]);
  }
}

void CaptureInt32(int32_t* storage,
                  const base::Closure& closure,
                  int32_t value) {
  *storage = value;
  closure.Run();
}

void CaptureSenderPtrInfo(IntegerSenderAssociatedPtr* storage,
                          const base::Closure& closure,
                          IntegerSenderAssociatedPtrInfo info) {
  storage->Bind(std::move(info));
  closure.Run();
}

TEST_F(AssociatedInterfaceTest, PassAssociatedInterfaces) {
  IntegerSenderConnectionPtr connection_ptr;
  IntegerSenderConnectionImpl connection(MakeRequest(&connection_ptr));

  IntegerSenderAssociatedPtr sender0;
  connection_ptr->GetSender(MakeRequest(&sender0));

  int32_t echoed_value = 0;
  base::RunLoop run_loop;
  sender0->Echo(123, base::Bind(&CaptureInt32, &echoed_value,
                                run_loop.QuitClosure()));
  run_loop.Run();
  EXPECT_EQ(123, echoed_value);

  IntegerSenderAssociatedPtr sender1;
  base::RunLoop run_loop2;
  connection_ptr->AsyncGetSender(
      base::Bind(&CaptureSenderPtrInfo, &sender1, run_loop2.QuitClosure()));
  run_loop2.Run();
  EXPECT_TRUE(sender1);

  base::RunLoop run_loop3;
  sender1->Echo(456, base::Bind(&CaptureInt32, &echoed_value,
                                run_loop3.QuitClosure()));
  run_loop3.Run();
  EXPECT_EQ(456, echoed_value);
}

TEST_F(AssociatedInterfaceTest, BindingWaitAndPauseWhenNoAssociatedInterfaces) {
  IntegerSenderConnectionPtr connection_ptr;
  IntegerSenderConnectionImpl connection(MakeRequest(&connection_ptr));

  IntegerSenderAssociatedPtr sender0;
  connection_ptr->GetSender(MakeRequest(&sender0));

  EXPECT_FALSE(connection.binding()->HasAssociatedInterfaces());
  // There are no associated interfaces running on the pipe yet. It is okay to
  // pause.
  connection.binding()->PauseIncomingMethodCallProcessing();
  connection.binding()->ResumeIncomingMethodCallProcessing();

  // There are no associated interfaces running on the pipe yet. It is okay to
  // wait.
  EXPECT_TRUE(connection.binding()->WaitForIncomingMethodCall());

  // The previous wait has dispatched the GetSender request message, therefore
  // an associated interface has been set up on the pipe. It is not allowed to
  // wait or pause.
  EXPECT_TRUE(connection.binding()->HasAssociatedInterfaces());
}

class PingServiceImpl : public PingService {
 public:
  explicit PingServiceImpl(PingServiceAssociatedRequest request)
      : binding_(this, std::move(request)) {}
  ~PingServiceImpl() override {}

  AssociatedBinding<PingService>& binding() { return binding_; }

  void set_ping_handler(const base::Closure& handler) {
    ping_handler_ = handler;
  }

  // PingService:
  void Ping(const PingCallback& callback) override {
    if (!ping_handler_.is_null())
      ping_handler_.Run();
    callback.Run();
  }

 private:
  AssociatedBinding<PingService> binding_;
  base::Closure ping_handler_;
};

class PingProviderImpl : public AssociatedPingProvider {
 public:
  explicit PingProviderImpl(AssociatedPingProviderRequest request)
      : binding_(this, std::move(request)) {}
  ~PingProviderImpl() override {}

  // AssociatedPingProvider:
  void GetPing(PingServiceAssociatedRequest request) override {
    ping_services_.emplace_back(new PingServiceImpl(std::move(request)));

    if (expected_bindings_count_ > 0 &&
        ping_services_.size() == expected_bindings_count_ &&
        !quit_waiting_.is_null()) {
      expected_bindings_count_ = 0;
      base::ResetAndReturn(&quit_waiting_).Run();
    }
  }

  std::vector<std::unique_ptr<PingServiceImpl>>& ping_services() {
    return ping_services_;
  }

  void WaitForBindings(size_t count) {
    DCHECK(quit_waiting_.is_null());

    expected_bindings_count_ = count;
    base::RunLoop loop;
    quit_waiting_ = loop.QuitClosure();
    loop.Run();
  }

 private:
  Binding<AssociatedPingProvider> binding_;
  std::vector<std::unique_ptr<PingServiceImpl>> ping_services_;
  size_t expected_bindings_count_ = 0;
  base::Closure quit_waiting_;
};

class CallbackFilter : public MessageReceiver {
 public:
  explicit CallbackFilter(const base::Closure& callback)
      : callback_(callback) {}
  ~CallbackFilter() override {}

  static std::unique_ptr<CallbackFilter> Wrap(const base::Closure& callback) {
    return std::make_unique<CallbackFilter>(callback);
  }

  // MessageReceiver:
  bool Accept(Message* message) override {
    callback_.Run();
    return true;
  }

 private:
  const base::Closure callback_;
};

// Verifies that filters work as expected on associated bindings, i.e. that
// they're notified in order, before dispatch; and that each associated
// binding in a group operates with its own set of filters.
TEST_F(AssociatedInterfaceTest, BindingWithFilters) {
  AssociatedPingProviderPtr provider;
  PingProviderImpl provider_impl(MakeRequest(&provider));

  PingServiceAssociatedPtr ping_a, ping_b;
  provider->GetPing(MakeRequest(&ping_a));
  provider->GetPing(MakeRequest(&ping_b));
  provider_impl.WaitForBindings(2);

  ASSERT_EQ(2u, provider_impl.ping_services().size());
  PingServiceImpl& ping_a_impl = *provider_impl.ping_services()[0];
  PingServiceImpl& ping_b_impl = *provider_impl.ping_services()[1];

  int a_status, b_status;
  auto handler_helper = [] (int* a_status, int* b_status, int expected_a_status,
                            int new_a_status, int expected_b_status,
                            int new_b_status) {
    EXPECT_EQ(expected_a_status, *a_status);
    EXPECT_EQ(expected_b_status, *b_status);
    *a_status = new_a_status;
    *b_status = new_b_status;
  };
  auto create_handler = [&] (int expected_a_status, int new_a_status,
                             int expected_b_status, int new_b_status) {
    return base::Bind(handler_helper, &a_status, &b_status, expected_a_status,
                      new_a_status, expected_b_status, new_b_status);
  };

  ping_a_impl.binding().AddFilter(
      CallbackFilter::Wrap(create_handler(0, 1, 0, 0)));
  ping_a_impl.binding().AddFilter(
      CallbackFilter::Wrap(create_handler(1, 2, 0, 0)));
  ping_a_impl.set_ping_handler(create_handler(2, 3, 0, 0));

  ping_b_impl.binding().AddFilter(
      CallbackFilter::Wrap(create_handler(3, 3, 0, 1)));
  ping_b_impl.binding().AddFilter(
      CallbackFilter::Wrap(create_handler(3, 3, 1, 2)));
  ping_b_impl.set_ping_handler(create_handler(3, 3, 2, 3));

  for (int i = 0; i < 10; ++i) {
    a_status = 0;
    b_status = 0;

    {
      base::RunLoop loop;
      ping_a->Ping(loop.QuitClosure());
      loop.Run();
    }

    EXPECT_EQ(3, a_status);
    EXPECT_EQ(0, b_status);

    {
      base::RunLoop loop;
      ping_b->Ping(loop.QuitClosure());
      loop.Run();
    }

    EXPECT_EQ(3, a_status);
    EXPECT_EQ(3, b_status);
  }
}

TEST_F(AssociatedInterfaceTest, AssociatedPtrFlushForTesting) {
  AssociatedInterfaceRequest<IntegerSender> request;
  IntegerSenderAssociatedPtrInfo ptr_info;
  CreateIntegerSender(&ptr_info, &request);

  IntegerSenderImpl impl0(std::move(request));
  AssociatedInterfacePtr<IntegerSender> ptr0;
  ptr0.Bind(std::move(ptr_info));
  ptr0.set_connection_error_handler(base::Bind(&Fail));

  bool ptr0_callback_run = false;
  ptr0->Echo(123, ExpectValueSetFlagAndRunClosure(123, &ptr0_callback_run,
                                                  base::DoNothing()));
  ptr0.FlushForTesting();
  EXPECT_TRUE(ptr0_callback_run);
}

void SetBool(bool* value) {
  *value = true;
}

template <typename T>
void SetBoolWithUnusedParameter(bool* value, T unused) {
  *value = true;
}

TEST_F(AssociatedInterfaceTest, AssociatedPtrFlushForTestingWithClosedPeer) {
  AssociatedInterfaceRequest<IntegerSender> request;
  IntegerSenderAssociatedPtrInfo ptr_info;
  CreateIntegerSender(&ptr_info, &request);

  AssociatedInterfacePtr<IntegerSender> ptr0;
  ptr0.Bind(std::move(ptr_info));
  bool called = false;
  ptr0.set_connection_error_handler(base::Bind(&SetBool, &called));
  request = nullptr;

  ptr0.FlushForTesting();
  EXPECT_TRUE(called);
  ptr0.FlushForTesting();
}

TEST_F(AssociatedInterfaceTest, AssociatedBindingFlushForTesting) {
  AssociatedInterfaceRequest<IntegerSender> request;
  IntegerSenderAssociatedPtrInfo ptr_info;
  CreateIntegerSender(&ptr_info, &request);

  IntegerSenderImpl impl0(std::move(request));
  impl0.set_connection_error_handler(base::Bind(&Fail));
  AssociatedInterfacePtr<IntegerSender> ptr0;
  ptr0.Bind(std::move(ptr_info));

  bool ptr0_callback_run = false;
  ptr0->Echo(123, ExpectValueSetFlagAndRunClosure(123, &ptr0_callback_run,
                                                  base::DoNothing()));
  // Because the flush is sent from the binding, it only guarantees that the
  // request has been received, not the response. The second flush waits for the
  // response to be received.
  impl0.binding()->FlushForTesting();
  impl0.binding()->FlushForTesting();
  EXPECT_TRUE(ptr0_callback_run);
}

TEST_F(AssociatedInterfaceTest,
       AssociatedBindingFlushForTestingWithClosedPeer) {
  scoped_refptr<MultiplexRouter> router0;
  scoped_refptr<MultiplexRouter> router1;
  CreateRouterPair(&router0, &router1);

  AssociatedInterfaceRequest<IntegerSender> request;
  {
    IntegerSenderAssociatedPtrInfo ptr_info;
    CreateIntegerSenderWithExistingRouters(router1, &ptr_info, router0,
                                           &request);
  }

  IntegerSenderImpl impl(std::move(request));
  bool called = false;
  impl.set_connection_error_handler(base::Bind(&SetBool, &called));
  impl.binding()->FlushForTesting();
  EXPECT_TRUE(called);
  impl.binding()->FlushForTesting();
}

TEST_F(AssociatedInterfaceTest, BindingFlushForTesting) {
  IntegerSenderConnectionPtr ptr;
  IntegerSenderConnectionImpl impl(MakeRequest(&ptr));
  bool called = false;
  ptr->AsyncGetSender(base::Bind(
      &SetBoolWithUnusedParameter<IntegerSenderAssociatedPtrInfo>, &called));
  EXPECT_FALSE(called);
  impl.binding()->set_connection_error_handler(base::Bind(&Fail));
  // Because the flush is sent from the binding, it only guarantees that the
  // request has been received, not the response. The second flush waits for the
  // response to be received.
  impl.binding()->FlushForTesting();
  impl.binding()->FlushForTesting();
  EXPECT_TRUE(called);
}

TEST_F(AssociatedInterfaceTest, BindingFlushForTestingWithClosedPeer) {
  IntegerSenderConnectionPtr ptr;
  IntegerSenderConnectionImpl impl(MakeRequest(&ptr));
  bool called = false;
  impl.binding()->set_connection_error_handler(base::Bind(&SetBool, &called));
  ptr.reset();
  EXPECT_FALSE(called);
  impl.binding()->FlushForTesting();
  EXPECT_TRUE(called);
  impl.binding()->FlushForTesting();
}

TEST_F(AssociatedInterfaceTest, StrongBindingFlushForTesting) {
  IntegerSenderConnectionPtr ptr;
  auto binding =
      MakeStrongBinding(std::make_unique<IntegerSenderConnectionImpl>(
                            IntegerSenderConnectionRequest{}),
                        MakeRequest(&ptr));
  bool called = false;
  IntegerSenderAssociatedPtr sender_ptr;
  ptr->GetSender(MakeRequest(&sender_ptr));
  sender_ptr->Echo(1, base::Bind(&SetBoolWithUnusedParameter<int>, &called));
  EXPECT_FALSE(called);
  // Because the flush is sent from the binding, it only guarantees that the
  // request has been received, not the response. The second flush waits for the
  // response to be received.
  ASSERT_TRUE(binding);
  binding->FlushForTesting();
  ASSERT_TRUE(binding);
  binding->FlushForTesting();
  EXPECT_TRUE(called);
}

TEST_F(AssociatedInterfaceTest, StrongBindingFlushForTestingWithClosedPeer) {
  IntegerSenderConnectionPtr ptr;
  bool called = false;
  auto binding =
      MakeStrongBinding(std::make_unique<IntegerSenderConnectionImpl>(
                            IntegerSenderConnectionRequest{}),
                        MakeRequest(&ptr));
  binding->set_connection_error_handler(base::Bind(&SetBool, &called));
  ptr.reset();
  EXPECT_FALSE(called);
  ASSERT_TRUE(binding);
  binding->FlushForTesting();
  EXPECT_TRUE(called);
  ASSERT_FALSE(binding);
}

TEST_F(AssociatedInterfaceTest, PtrFlushForTesting) {
  IntegerSenderConnectionPtr ptr;
  IntegerSenderConnectionImpl impl(MakeRequest(&ptr));
  bool called = false;
  ptr.set_connection_error_handler(base::Bind(&Fail));
  ptr->AsyncGetSender(base::Bind(
      &SetBoolWithUnusedParameter<IntegerSenderAssociatedPtrInfo>, &called));
  EXPECT_FALSE(called);
  ptr.FlushForTesting();
  EXPECT_TRUE(called);
}

TEST_F(AssociatedInterfaceTest, PtrFlushForTestingWithClosedPeer) {
  IntegerSenderConnectionPtr ptr;
  MakeRequest(&ptr);
  bool called = false;
  ptr.set_connection_error_handler(base::Bind(&SetBool, &called));
  EXPECT_FALSE(called);
  ptr.FlushForTesting();
  EXPECT_TRUE(called);
  ptr.FlushForTesting();
}

TEST_F(AssociatedInterfaceTest, AssociatedBindingConnectionErrorWithReason) {
  AssociatedInterfaceRequest<IntegerSender> request;
  IntegerSenderAssociatedPtrInfo ptr_info;
  CreateIntegerSender(&ptr_info, &request);

  IntegerSenderImpl impl(std::move(request));
  AssociatedInterfacePtr<IntegerSender> ptr;
  ptr.Bind(std::move(ptr_info));

  base::RunLoop run_loop;
  impl.binding()->set_connection_error_with_reason_handler(base::Bind(
      [](const base::Closure& quit_closure, uint32_t custom_reason,
         const std::string& description) {
        EXPECT_EQ(123u, custom_reason);
        EXPECT_EQ("farewell", description);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  ptr.ResetWithReason(123u, "farewell");

  run_loop.Run();
}

TEST_F(AssociatedInterfaceTest,
       PendingAssociatedBindingConnectionErrorWithReason) {
  // Test that AssociatedBinding is notified with connection error when the
  // interface hasn't associated with a message pipe and the peer is closed.

  IntegerSenderAssociatedPtr ptr;
  IntegerSenderImpl impl(MakeRequest(&ptr));

  base::RunLoop run_loop;
  impl.binding()->set_connection_error_with_reason_handler(base::Bind(
      [](const base::Closure& quit_closure, uint32_t custom_reason,
         const std::string& description) {
        EXPECT_EQ(123u, custom_reason);
        EXPECT_EQ("farewell", description);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  ptr.ResetWithReason(123u, "farewell");

  run_loop.Run();
}

TEST_F(AssociatedInterfaceTest, AssociatedPtrConnectionErrorWithReason) {
  AssociatedInterfaceRequest<IntegerSender> request;
  IntegerSenderAssociatedPtrInfo ptr_info;
  CreateIntegerSender(&ptr_info, &request);

  IntegerSenderImpl impl(std::move(request));
  AssociatedInterfacePtr<IntegerSender> ptr;
  ptr.Bind(std::move(ptr_info));

  base::RunLoop run_loop;
  ptr.set_connection_error_with_reason_handler(base::Bind(
      [](const base::Closure& quit_closure, uint32_t custom_reason,
         const std::string& description) {
        EXPECT_EQ(456u, custom_reason);
        EXPECT_EQ("farewell", description);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  impl.binding()->CloseWithReason(456u, "farewell");

  run_loop.Run();
}

TEST_F(AssociatedInterfaceTest, PendingAssociatedPtrConnectionErrorWithReason) {
  // Test that AssociatedInterfacePtr is notified with connection error when the
  // interface hasn't associated with a message pipe and the peer is closed.

  IntegerSenderAssociatedPtr ptr;
  auto request = MakeRequest(&ptr);

  base::RunLoop run_loop;
  ptr.set_connection_error_with_reason_handler(base::Bind(
      [](const base::Closure& quit_closure, uint32_t custom_reason,
         const std::string& description) {
        EXPECT_EQ(456u, custom_reason);
        EXPECT_EQ("farewell", description);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  request.ResetWithReason(456u, "farewell");

  run_loop.Run();
}

TEST_F(AssociatedInterfaceTest, AssociatedRequestResetWithReason) {
  AssociatedInterfaceRequest<IntegerSender> request;
  IntegerSenderAssociatedPtrInfo ptr_info;
  CreateIntegerSender(&ptr_info, &request);

  AssociatedInterfacePtr<IntegerSender> ptr;
  ptr.Bind(std::move(ptr_info));

  base::RunLoop run_loop;
  ptr.set_connection_error_with_reason_handler(base::Bind(
      [](const base::Closure& quit_closure, uint32_t custom_reason,
         const std::string& description) {
        EXPECT_EQ(789u, custom_reason);
        EXPECT_EQ("long time no see", description);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  request.ResetWithReason(789u, "long time no see");

  run_loop.Run();
}

TEST_F(AssociatedInterfaceTest, ThreadSafeAssociatedInterfacePtr) {
  IntegerSenderConnectionPtr connection_ptr;
  IntegerSenderConnectionImpl connection(MakeRequest(&connection_ptr));

  IntegerSenderAssociatedPtr sender;
  connection_ptr->GetSender(MakeRequest(&sender));

  scoped_refptr<ThreadSafeIntegerSenderAssociatedPtr> thread_safe_sender =
      ThreadSafeIntegerSenderAssociatedPtr::Create(std::move(sender));

  {
    // Test the thread safe pointer can be used from the interface ptr thread.
    int32_t echoed_value = 0;
    base::RunLoop run_loop;
    (*thread_safe_sender)
        ->Echo(123, base::Bind(&CaptureInt32, &echoed_value,
                               run_loop.QuitClosure()));
    run_loop.Run();
    EXPECT_EQ(123, echoed_value);
  }

  // Test the thread safe pointer can be used from another thread.
  base::RunLoop run_loop;

  auto run_method = base::Bind(
      [](const scoped_refptr<base::TaskRunner>& main_task_runner,
         const base::Closure& quit_closure,
         const scoped_refptr<ThreadSafeIntegerSenderAssociatedPtr>&
             thread_safe_sender) {
        auto done_callback = base::Bind(
            [](const scoped_refptr<base::TaskRunner>& main_task_runner,
               const base::Closure& quit_closure,
               scoped_refptr<base::SequencedTaskRunner> sender_sequence_runner,
               int32_t result) {
              EXPECT_EQ(123, result);
              // Validate the callback is invoked on the calling sequence.
              EXPECT_TRUE(sender_sequence_runner->RunsTasksInCurrentSequence());
              // Notify the run_loop to quit.
              main_task_runner->PostTask(FROM_HERE, quit_closure);
            });
        scoped_refptr<base::SequencedTaskRunner> current_sequence_runner =
            base::SequencedTaskRunnerHandle::Get();
        (*thread_safe_sender)
            ->Echo(123, base::Bind(done_callback, main_task_runner,
                                   quit_closure, current_sequence_runner));
      },
      base::SequencedTaskRunnerHandle::Get(), run_loop.QuitClosure(),
      thread_safe_sender);
  base::CreateSequencedTaskRunnerWithTraits({})->PostTask(FROM_HERE,
                                                          run_method);

  // Block until the method callback is called on the background thread.
  run_loop.Run();
}

struct ForwarderTestContext {
  IntegerSenderConnectionPtr connection_ptr;
  std::unique_ptr<IntegerSenderConnectionImpl> interface_impl;
  IntegerSenderAssociatedRequest sender_request;
};

TEST_F(AssociatedInterfaceTest,
       ThreadSafeAssociatedInterfacePtrWithTaskRunner) {
  const scoped_refptr<base::SequencedTaskRunner> other_thread_task_runner =
      base::CreateSequencedTaskRunnerWithTraits({});

  ForwarderTestContext* context = new ForwarderTestContext();
  IntegerSenderAssociatedPtrInfo sender_info;
  base::WaitableEvent sender_info_bound_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  auto setup = [](base::WaitableEvent* sender_info_bound_event,
                  IntegerSenderAssociatedPtrInfo* sender_info,
                  ForwarderTestContext* context) {
    context->interface_impl = std::make_unique<IntegerSenderConnectionImpl>(
        MakeRequest(&context->connection_ptr));

    auto sender_request = MakeRequest(sender_info);
    context->connection_ptr->GetSender(std::move(sender_request));

    // Unblock the main thread as soon as |sender_info| is set.
    sender_info_bound_event->Signal();
  };
  other_thread_task_runner->PostTask(
      FROM_HERE,
      base::Bind(setup, &sender_info_bound_event, &sender_info, context));
  sender_info_bound_event.Wait();

  // Create a ThreadSafeAssociatedPtr that binds on the background thread and is
  // associated with |connection_ptr| there.
  scoped_refptr<ThreadSafeIntegerSenderAssociatedPtr> thread_safe_ptr =
      ThreadSafeIntegerSenderAssociatedPtr::Create(std::move(sender_info),
                                                   other_thread_task_runner);

  // Issue a call on the thread-safe ptr immediately. Note that this may happen
  // before the interface is bound on the background thread, and that must be
  // OK.
  {
    auto echo_callback =
        base::Bind([](const base::Closure& quit_closure, int32_t result) {
          EXPECT_EQ(123, result);
          quit_closure.Run();
        });
    base::RunLoop run_loop;
    (*thread_safe_ptr)
        ->Echo(123, base::Bind(echo_callback, run_loop.QuitClosure()));

    // Block until the method callback is called.
    run_loop.Run();
  }

  other_thread_task_runner->DeleteSoon(FROM_HERE, context);

  // Reset the pointer now so the InterfacePtr associated resources can be
  // deleted before the background thread's message loop is invalidated.
  thread_safe_ptr = nullptr;
}

class DiscardingAssociatedPingProviderProvider
    : public AssociatedPingProviderProvider {
 public:
  void GetPingProvider(
      AssociatedPingProviderAssociatedRequest request) override {}
};

TEST_F(AssociatedInterfaceTest, CloseWithoutBindingAssociatedRequest) {
  DiscardingAssociatedPingProviderProvider ping_provider_provider;
  mojo::Binding<AssociatedPingProviderProvider> binding(
      &ping_provider_provider);
  AssociatedPingProviderProviderPtr provider_provider;
  binding.Bind(mojo::MakeRequest(&provider_provider));
  AssociatedPingProviderAssociatedPtr provider;
  provider_provider->GetPingProvider(mojo::MakeRequest(&provider));
  PingServiceAssociatedPtr ping;
  provider->GetPing(mojo::MakeRequest(&ping));
  base::RunLoop run_loop;
  ping.set_connection_error_handler(run_loop.QuitClosure());
  run_loop.Run();
}

TEST_F(AssociatedInterfaceTest, AssociateWithDisconnectedPipe) {
  IntegerSenderAssociatedPtr sender;
  AssociateWithDisconnectedPipe(MakeRequest(&sender).PassHandle());
  sender->Send(42);
}

}  // namespace
}  // namespace test
}  // namespace mojo
