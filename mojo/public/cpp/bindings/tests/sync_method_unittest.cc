// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/sequence_token.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/tests/bindings_test_base.h"
#include "mojo/public/interfaces/bindings/tests/test_sync_methods.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

template <typename... Args>
struct LambdaBinder {
  using CallbackType = base::Callback<void(Args...)>;

  template <typename Func>
  static void RunLambda(Func func, Args... args) {
    func(std::move(args)...);
  }

  template <typename Func>
  static CallbackType BindLambda(Func func) {
    return base::Bind(&LambdaBinder::RunLambda<Func>, func);
  }
};

class TestSyncCommonImpl {
 public:
  TestSyncCommonImpl() {}

  using PingHandler = base::Callback<void(const base::Callback<void()>&)>;
  using PingBinder = LambdaBinder<const base::Callback<void()>&>;
  template <typename Func>
  void set_ping_handler(Func handler) {
    ping_handler_ = PingBinder::BindLambda(handler);
  }

  using EchoHandler =
      base::Callback<void(int32_t, const base::Callback<void(int32_t)>&)>;
  using EchoBinder =
      LambdaBinder<int32_t, const base::Callback<void(int32_t)>&>;
  template <typename Func>
  void set_echo_handler(Func handler) {
    echo_handler_ = EchoBinder::BindLambda(handler);
  }

  using AsyncEchoHandler =
      base::Callback<void(int32_t, const base::Callback<void(int32_t)>&)>;
  using AsyncEchoBinder =
      LambdaBinder<int32_t, const base::Callback<void(int32_t)>&>;
  template <typename Func>
  void set_async_echo_handler(Func handler) {
    async_echo_handler_ = AsyncEchoBinder::BindLambda(handler);
  }

  using SendInterfaceHandler = base::Callback<void(TestSyncAssociatedPtrInfo)>;
  using SendInterfaceBinder = LambdaBinder<TestSyncAssociatedPtrInfo>;
  template <typename Func>
  void set_send_interface_handler(Func handler) {
    send_interface_handler_ = SendInterfaceBinder::BindLambda(handler);
  }

  using SendRequestHandler = base::Callback<void(TestSyncAssociatedRequest)>;
  using SendRequestBinder = LambdaBinder<TestSyncAssociatedRequest>;
  template <typename Func>
  void set_send_request_handler(Func handler) {
    send_request_handler_ = SendRequestBinder::BindLambda(handler);
  }

  void PingImpl(const base::Callback<void()>& callback) {
    if (ping_handler_.is_null()) {
      callback.Run();
      return;
    }
    ping_handler_.Run(callback);
  }
  void EchoImpl(int32_t value, const base::Callback<void(int32_t)>& callback) {
    if (echo_handler_.is_null()) {
      callback.Run(value);
      return;
    }
    echo_handler_.Run(value, callback);
  }
  void AsyncEchoImpl(int32_t value,
                     const base::Callback<void(int32_t)>& callback) {
    if (async_echo_handler_.is_null()) {
      callback.Run(value);
      return;
    }
    async_echo_handler_.Run(value, callback);
  }
  void SendInterfaceImpl(TestSyncAssociatedPtrInfo ptr) {
    send_interface_handler_.Run(std::move(ptr));
  }
  void SendRequestImpl(TestSyncAssociatedRequest request) {
    send_request_handler_.Run(std::move(request));
  }

 private:
  PingHandler ping_handler_;
  EchoHandler echo_handler_;
  AsyncEchoHandler async_echo_handler_;
  SendInterfaceHandler send_interface_handler_;
  SendRequestHandler send_request_handler_;

  DISALLOW_COPY_AND_ASSIGN(TestSyncCommonImpl);
};

class TestSyncImpl : public TestSync, public TestSyncCommonImpl {
 public:
  explicit TestSyncImpl(TestSyncRequest request)
      : binding_(this, std::move(request)) {}

  // TestSync implementation:
  void Ping(const PingCallback& callback) override { PingImpl(callback); }
  void Echo(int32_t value, const EchoCallback& callback) override {
    EchoImpl(value, callback);
  }
  void AsyncEcho(int32_t value, const AsyncEchoCallback& callback) override {
    AsyncEchoImpl(value, callback);
  }

  Binding<TestSync>* binding() { return &binding_; }

 private:
  Binding<TestSync> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestSyncImpl);
};

class TestSyncMasterImpl : public TestSyncMaster, public TestSyncCommonImpl {
 public:
  explicit TestSyncMasterImpl(TestSyncMasterRequest request)
      : binding_(this, std::move(request)) {}

  // TestSyncMaster implementation:
  void Ping(const PingCallback& callback) override { PingImpl(callback); }
  void Echo(int32_t value, const EchoCallback& callback) override {
    EchoImpl(value, callback);
  }
  void AsyncEcho(int32_t value, const AsyncEchoCallback& callback) override {
    AsyncEchoImpl(value, callback);
  }
  void SendInterface(TestSyncAssociatedPtrInfo ptr) override {
    SendInterfaceImpl(std::move(ptr));
  }
  void SendRequest(TestSyncAssociatedRequest request) override {
    SendRequestImpl(std::move(request));
  }

  Binding<TestSyncMaster>* binding() { return &binding_; }

 private:
  Binding<TestSyncMaster> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestSyncMasterImpl);
};

class TestSyncAssociatedImpl : public TestSync, public TestSyncCommonImpl {
 public:
  explicit TestSyncAssociatedImpl(TestSyncAssociatedRequest request)
      : binding_(this, std::move(request)) {}

  // TestSync implementation:
  void Ping(const PingCallback& callback) override { PingImpl(callback); }
  void Echo(int32_t value, const EchoCallback& callback) override {
    EchoImpl(value, callback);
  }
  void AsyncEcho(int32_t value, const AsyncEchoCallback& callback) override {
    AsyncEchoImpl(value, callback);
  }

  AssociatedBinding<TestSync>* binding() { return &binding_; }

 private:
  AssociatedBinding<TestSync> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestSyncAssociatedImpl);
};

template <typename Interface>
struct ImplTraits;

template <>
struct ImplTraits<TestSync> {
  using Type = TestSyncImpl;
};

template <>
struct ImplTraits<TestSyncMaster> {
  using Type = TestSyncMasterImpl;
};

template <typename Interface>
using ImplTypeFor = typename ImplTraits<Interface>::Type;

// A wrapper for either an InterfacePtr or scoped_refptr<ThreadSafeInterfacePtr>
// that exposes the InterfacePtr interface.
template <typename Interface>
class PtrWrapper {
 public:
  explicit PtrWrapper(InterfacePtr<Interface> ptr) : ptr_(std::move(ptr)) {}

  explicit PtrWrapper(
      scoped_refptr<ThreadSafeInterfacePtr<Interface>> thread_safe_ptr)
      : thread_safe_ptr_(thread_safe_ptr) {}

  PtrWrapper(PtrWrapper&& other) = default;

  Interface* operator->() {
    return thread_safe_ptr_ ? thread_safe_ptr_->get() : ptr_.get();
  }

  void set_connection_error_handler(const base::Closure& error_handler) {
    DCHECK(!thread_safe_ptr_);
    ptr_.set_connection_error_handler(error_handler);
  }

  void reset() {
    ptr_ = nullptr;
    thread_safe_ptr_ = nullptr;
  }

 private:
  InterfacePtr<Interface> ptr_;
  scoped_refptr<ThreadSafeInterfacePtr<Interface>> thread_safe_ptr_;

  DISALLOW_COPY_AND_ASSIGN(PtrWrapper);
};

// The type parameter for SyncMethodCommonTests and
// SyncMethodOnSequenceCommonTests for varying the Interface and whether to use
// InterfacePtr or ThreadSafeInterfacePtr.
template <typename InterfaceT,
          bool use_thread_safe_ptr,
          BindingsTestSerializationMode serialization_mode>
struct TestParams {
  using Interface = InterfaceT;
  static const bool kIsThreadSafeInterfacePtrTest = use_thread_safe_ptr;

  static PtrWrapper<InterfaceT> Wrap(InterfacePtr<Interface> ptr) {
    if (kIsThreadSafeInterfacePtrTest) {
      return PtrWrapper<Interface>(
          ThreadSafeInterfacePtr<Interface>::Create(std::move(ptr)));
    } else {
      return PtrWrapper<Interface>(std::move(ptr));
    }
  }

  static const BindingsTestSerializationMode kSerializationMode =
      serialization_mode;
};

template <typename Interface>
class TestSyncServiceSequence {
 public:
  TestSyncServiceSequence()
      : task_runner_(base::CreateSequencedTaskRunnerWithTraits({})),
        ping_called_(false) {}

  void SetUp(InterfaceRequest<Interface> request) {
    CHECK(task_runner()->RunsTasksInCurrentSequence());
    impl_.reset(new ImplTypeFor<Interface>(std::move(request)));
    impl_->set_ping_handler(
        [this](const typename Interface::PingCallback& callback) {
          {
            base::AutoLock locker(lock_);
            ping_called_ = true;
          }
          callback.Run();
        });
  }

  void TearDown() {
    CHECK(task_runner()->RunsTasksInCurrentSequence());
    impl_.reset();
  }

  base::SequencedTaskRunner* task_runner() { return task_runner_.get(); }
  bool ping_called() const {
    base::AutoLock locker(lock_);
    return ping_called_;
  }

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  std::unique_ptr<ImplTypeFor<Interface>> impl_;

  mutable base::Lock lock_;
  bool ping_called_;

  DISALLOW_COPY_AND_ASSIGN(TestSyncServiceSequence);
};

class SyncMethodTest : public testing::Test {
 public:
  SyncMethodTest() {}
  ~SyncMethodTest() override { base::RunLoop().RunUntilIdle(); }

 protected:
  base::test::ScopedTaskEnvironment task_environment;
};

template <typename TypeParam>
class SyncMethodCommonTest : public SyncMethodTest {
 public:
  SyncMethodCommonTest() {}
  ~SyncMethodCommonTest() override {}

  void SetUp() override {
    BindingsTestBase::SetupSerializationBehavior(TypeParam::kSerializationMode);
  }
};

class SyncMethodAssociatedTest : public SyncMethodTest {
 public:
  SyncMethodAssociatedTest() {}
  ~SyncMethodAssociatedTest() override {}

 protected:
  void SetUp() override {
    master_impl_.reset(new TestSyncMasterImpl(MakeRequest(&master_ptr_)));

    asso_request_ = MakeRequest(&asso_ptr_info_);
    opposite_asso_request_ = MakeRequest(&opposite_asso_ptr_info_);

    master_impl_->set_send_interface_handler(
        [this](TestSyncAssociatedPtrInfo ptr) {
          opposite_asso_ptr_info_ = std::move(ptr);
        });
    base::RunLoop run_loop;
    master_impl_->set_send_request_handler(
        [this, &run_loop](TestSyncAssociatedRequest request) {
          asso_request_ = std::move(request);
          run_loop.Quit();
        });

    master_ptr_->SendInterface(std::move(opposite_asso_ptr_info_));
    master_ptr_->SendRequest(std::move(asso_request_));
    run_loop.Run();
  }

  void TearDown() override {
    asso_ptr_info_ = TestSyncAssociatedPtrInfo();
    asso_request_ = TestSyncAssociatedRequest();
    opposite_asso_ptr_info_ = TestSyncAssociatedPtrInfo();
    opposite_asso_request_ = TestSyncAssociatedRequest();

    master_ptr_ = nullptr;
    master_impl_.reset();
  }

  InterfacePtr<TestSyncMaster> master_ptr_;
  std::unique_ptr<TestSyncMasterImpl> master_impl_;

  // An associated interface whose binding lives at the |master_impl_| side.
  TestSyncAssociatedPtrInfo asso_ptr_info_;
  TestSyncAssociatedRequest asso_request_;

  // An associated interface whose binding lives at the |master_ptr_| side.
  TestSyncAssociatedPtrInfo opposite_asso_ptr_info_;
  TestSyncAssociatedRequest opposite_asso_request_;
};

void SetFlagAndRunClosure(bool* flag, const base::Closure& closure) {
  *flag = true;
  closure.Run();
}

void ExpectValueAndRunClosure(int32_t expected_value,
                              const base::Closure& closure,
                              int32_t value) {
  EXPECT_EQ(expected_value, value);
  closure.Run();
}

template <typename Func>
void CallAsyncEchoCallback(Func func, int32_t value) {
  func(value);
}

template <typename Func>
TestSync::AsyncEchoCallback BindAsyncEchoCallback(Func func) {
  return base::Bind(&CallAsyncEchoCallback<Func>, func);
}

class SequencedTaskRunnerTestBase;

void RunTestOnSequencedTaskRunner(
    std::unique_ptr<SequencedTaskRunnerTestBase> test);

class SequencedTaskRunnerTestBase {
 public:
  virtual ~SequencedTaskRunnerTestBase() = default;

  void RunTest() {
    SetUp();
    Run();
  }

  virtual void Run() = 0;

  virtual void SetUp() {}
  virtual void TearDown() {}

 protected:
  void Done() {
    TearDown();
    task_runner_->PostTask(FROM_HERE, quit_closure_);
    delete this;
  }

  base::Closure DoneClosure() {
    return base::Bind(&SequencedTaskRunnerTestBase::Done,
                      base::Unretained(this));
  }

 private:
  friend void RunTestOnSequencedTaskRunner(
      std::unique_ptr<SequencedTaskRunnerTestBase> test);

  void Init(const base::Closure& quit_closure) {
    task_runner_ = base::SequencedTaskRunnerHandle::Get();
    quit_closure_ = quit_closure;
  }

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::Closure quit_closure_;
};

// A helper class to launch tests on a SequencedTaskRunner. This is necessary
// so gtest can instantiate copies for each |TypeParam|.
template <typename TypeParam>
class SequencedTaskRunnerTestLauncher : public testing::Test {
  base::test::ScopedTaskEnvironment task_environment;
};

// Similar to SyncMethodCommonTest, but the test body runs on a
// SequencedTaskRunner.
template <typename TypeParam>
class SyncMethodOnSequenceCommonTest : public SequencedTaskRunnerTestBase {
 public:
  void SetUp() override {
    BindingsTestBase::SetupSerializationBehavior(TypeParam::kSerializationMode);
    impl_ = std::make_unique<ImplTypeFor<typename TypeParam::Interface>>(
        MakeRequest(&ptr_));
  }

 protected:
  InterfacePtr<typename TypeParam::Interface> ptr_;
  std::unique_ptr<ImplTypeFor<typename TypeParam::Interface>> impl_;
};

void RunTestOnSequencedTaskRunner(
    std::unique_ptr<SequencedTaskRunnerTestBase> test) {
  base::RunLoop run_loop;
  test->Init(run_loop.QuitClosure());
  base::CreateSequencedTaskRunnerWithTraits({base::WithBaseSyncPrimitives()})
      ->PostTask(FROM_HERE, base::Bind(&SequencedTaskRunnerTestBase::RunTest,
                                       base::Unretained(test.release())));
  run_loop.Run();
}

// TestSync (without associated interfaces) and TestSyncMaster (with associated
// interfaces) exercise MultiplexRouter with different configurations.
// Each test is run once with an InterfacePtr and once with a
// ThreadSafeInterfacePtr to ensure that they behave the same with respect to
// sync calls. Finally, all such combinations are tested in different message
// serialization modes.
using InterfaceTypes = testing::Types<
    TestParams<TestSync,
               true,
               BindingsTestSerializationMode::kSerializeBeforeSend>,
    TestParams<TestSync,
               false,
               BindingsTestSerializationMode::kSerializeBeforeSend>,
    TestParams<TestSyncMaster,
               true,
               BindingsTestSerializationMode::kSerializeBeforeSend>,
    TestParams<TestSyncMaster,
               false,
               BindingsTestSerializationMode::kSerializeBeforeSend>,
    TestParams<TestSync,
               true,
               BindingsTestSerializationMode::kSerializeBeforeDispatch>,
    TestParams<TestSync,
               false,
               BindingsTestSerializationMode::kSerializeBeforeDispatch>,
    TestParams<TestSyncMaster,
               true,
               BindingsTestSerializationMode::kSerializeBeforeDispatch>,
    TestParams<TestSyncMaster,
               false,
               BindingsTestSerializationMode::kSerializeBeforeDispatch>,
    TestParams<TestSync, true, BindingsTestSerializationMode::kNeverSerialize>,
    TestParams<TestSync, false, BindingsTestSerializationMode::kNeverSerialize>,
    TestParams<TestSyncMaster,
               true,
               BindingsTestSerializationMode::kNeverSerialize>,
    TestParams<TestSyncMaster,
               false,
               BindingsTestSerializationMode::kNeverSerialize>>;

TYPED_TEST_CASE(SyncMethodCommonTest, InterfaceTypes);
TYPED_TEST_CASE(SequencedTaskRunnerTestLauncher, InterfaceTypes);

TYPED_TEST(SyncMethodCommonTest, CallSyncMethodAsynchronously) {
  using Interface = typename TypeParam::Interface;
  InterfacePtr<Interface> interface_ptr;
  ImplTypeFor<Interface> impl(MakeRequest(&interface_ptr));
  auto ptr = TypeParam::Wrap(std::move(interface_ptr));

  base::RunLoop run_loop;
  ptr->Echo(123, base::Bind(&ExpectValueAndRunClosure, 123,
                            run_loop.QuitClosure()));
  run_loop.Run();
}

#define SEQUENCED_TASK_RUNNER_TYPED_TEST_NAME(fixture_name, name) \
  fixture_name##name##_SequencedTaskRunnerTestSuffix

#define SEQUENCED_TASK_RUNNER_TYPED_TEST(fixture_name, name)        \
  template <typename TypeParam>                                     \
  class SEQUENCED_TASK_RUNNER_TYPED_TEST_NAME(fixture_name, name)   \
      : public fixture_name<TypeParam> {                            \
    void Run() override;                                            \
  };                                                                \
  TYPED_TEST(SequencedTaskRunnerTestLauncher, name) {               \
    RunTestOnSequencedTaskRunner(                                   \
        std::make_unique<SEQUENCED_TASK_RUNNER_TYPED_TEST_NAME(     \
                             fixture_name, name) < TypeParam>> ()); \
  }                                                                 \
  template <typename TypeParam>                                     \
  void SEQUENCED_TASK_RUNNER_TYPED_TEST_NAME(fixture_name,          \
                                             name)<TypeParam>::Run()

#define SEQUENCED_TASK_RUNNER_TYPED_TEST_F(fixture_name, name)      \
  template <typename TypeParam>                                     \
  class SEQUENCED_TASK_RUNNER_TYPED_TEST_NAME(fixture_name, name);  \
  TYPED_TEST(SequencedTaskRunnerTestLauncher, name) {               \
    RunTestOnSequencedTaskRunner(                                   \
        std::make_unique<SEQUENCED_TASK_RUNNER_TYPED_TEST_NAME(     \
                             fixture_name, name) < TypeParam>> ()); \
  }                                                                 \
  template <typename TypeParam>                                     \
  class SEQUENCED_TASK_RUNNER_TYPED_TEST_NAME(fixture_name, name)   \
      : public fixture_name<TypeParam>

SEQUENCED_TASK_RUNNER_TYPED_TEST(SyncMethodOnSequenceCommonTest,
                                 CallSyncMethodAsynchronously) {
  this->ptr_->Echo(
      123, base::Bind(&ExpectValueAndRunClosure, 123, this->DoneClosure()));
}

TYPED_TEST(SyncMethodCommonTest, BasicSyncCalls) {
  using Interface = typename TypeParam::Interface;
  InterfacePtr<Interface> interface_ptr;
  InterfaceRequest<Interface> request = MakeRequest(&interface_ptr);
  auto ptr = TypeParam::Wrap(std::move(interface_ptr));

  TestSyncServiceSequence<Interface> service_sequence;
  service_sequence.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&TestSyncServiceSequence<Interface>::SetUp,
                 base::Unretained(&service_sequence), base::Passed(&request)));
  ASSERT_TRUE(ptr->Ping());
  ASSERT_TRUE(service_sequence.ping_called());

  int32_t output_value = -1;
  ASSERT_TRUE(ptr->Echo(42, &output_value));
  ASSERT_EQ(42, output_value);

  base::RunLoop run_loop;
  service_sequence.task_runner()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&TestSyncServiceSequence<Interface>::TearDown,
                 base::Unretained(&service_sequence)),
      run_loop.QuitClosure());
  run_loop.Run();
}

TYPED_TEST(SyncMethodCommonTest, ReenteredBySyncMethodBinding) {
  // Test that an interface pointer waiting for a sync call response can be
  // reentered by a binding serving sync methods on the same thread.

  using Interface = typename TypeParam::Interface;
  InterfacePtr<Interface> interface_ptr;
  // The binding lives on the same thread as the interface pointer.
  ImplTypeFor<Interface> impl(MakeRequest(&interface_ptr));
  auto ptr = TypeParam::Wrap(std::move(interface_ptr));
  int32_t output_value = -1;
  ASSERT_TRUE(ptr->Echo(42, &output_value));
  EXPECT_EQ(42, output_value);
}

SEQUENCED_TASK_RUNNER_TYPED_TEST(SyncMethodOnSequenceCommonTest,
                                 ReenteredBySyncMethodBinding) {
  // Test that an interface pointer waiting for a sync call response can be
  // reentered by a binding serving sync methods on the same thread.

  int32_t output_value = -1;
  ASSERT_TRUE(this->ptr_->Echo(42, &output_value));
  EXPECT_EQ(42, output_value);
  this->Done();
}

TYPED_TEST(SyncMethodCommonTest, InterfacePtrDestroyedDuringSyncCall) {
  // Test that it won't result in crash or hang if an interface pointer is
  // destroyed while it is waiting for a sync call response.

  using Interface = typename TypeParam::Interface;
  InterfacePtr<Interface> interface_ptr;
  ImplTypeFor<Interface> impl(MakeRequest(&interface_ptr));
  auto ptr = TypeParam::Wrap(std::move(interface_ptr));
  impl.set_ping_handler([&ptr](const TestSync::PingCallback& callback) {
    ptr.reset();
    callback.Run();
  });
  ASSERT_FALSE(ptr->Ping());
}

SEQUENCED_TASK_RUNNER_TYPED_TEST(SyncMethodOnSequenceCommonTest,
                                 InterfacePtrDestroyedDuringSyncCall) {
  // Test that it won't result in crash or hang if an interface pointer is
  // destroyed while it is waiting for a sync call response.

  auto* ptr = &this->ptr_;
  this->impl_->set_ping_handler([ptr](const TestSync::PingCallback& callback) {
    ptr->reset();
    callback.Run();
  });
  ASSERT_FALSE(this->ptr_->Ping());
  this->Done();
}

TYPED_TEST(SyncMethodCommonTest, BindingDestroyedDuringSyncCall) {
  // Test that it won't result in crash or hang if a binding is
  // closed (and therefore the message pipe handle is closed) while the
  // corresponding interface pointer is waiting for a sync call response.

  using Interface = typename TypeParam::Interface;
  InterfacePtr<Interface> interface_ptr;
  ImplTypeFor<Interface> impl(MakeRequest(&interface_ptr));
  auto ptr = TypeParam::Wrap(std::move(interface_ptr));
  impl.set_ping_handler([&impl](const TestSync::PingCallback& callback) {
    impl.binding()->Close();
    callback.Run();
  });
  ASSERT_FALSE(ptr->Ping());
}

SEQUENCED_TASK_RUNNER_TYPED_TEST(SyncMethodOnSequenceCommonTest,
                                 BindingDestroyedDuringSyncCall) {
  // Test that it won't result in crash or hang if a binding is
  // closed (and therefore the message pipe handle is closed) while the
  // corresponding interface pointer is waiting for a sync call response.

  auto& impl = *this->impl_;
  this->impl_->set_ping_handler(
      [&impl](const TestSync::PingCallback& callback) {
        impl.binding()->Close();
        callback.Run();
      });
  ASSERT_FALSE(this->ptr_->Ping());
  this->Done();
}

TYPED_TEST(SyncMethodCommonTest, NestedSyncCallsWithInOrderResponses) {
  // Test that we can call a sync method on an interface ptr, while there is
  // already a sync call ongoing. The responses arrive in order.

  using Interface = typename TypeParam::Interface;
  InterfacePtr<Interface> interface_ptr;
  ImplTypeFor<Interface> impl(MakeRequest(&interface_ptr));
  auto ptr = TypeParam::Wrap(std::move(interface_ptr));

  // The same variable is used to store the output of the two sync calls, in
  // order to test that responses are handled in the correct order.
  int32_t result_value = -1;

  bool first_call = true;
  impl.set_echo_handler([&first_call, &ptr, &result_value](
      int32_t value, const TestSync::EchoCallback& callback) {
    if (first_call) {
      first_call = false;
      ASSERT_TRUE(ptr->Echo(456, &result_value));
      EXPECT_EQ(456, result_value);
    }
    callback.Run(value);
  });

  ASSERT_TRUE(ptr->Echo(123, &result_value));
  EXPECT_EQ(123, result_value);
}

SEQUENCED_TASK_RUNNER_TYPED_TEST(SyncMethodOnSequenceCommonTest,
                                 NestedSyncCallsWithInOrderResponses) {
  // Test that we can call a sync method on an interface ptr, while there is
  // already a sync call ongoing. The responses arrive in order.

  // The same variable is used to store the output of the two sync calls, in
  // order to test that responses are handled in the correct order.
  int32_t result_value = -1;

  bool first_call = true;
  auto& ptr = this->ptr_;
  auto& impl = *this->impl_;
  impl.set_echo_handler(
      [&first_call, &ptr, &result_value](
          int32_t value, const TestSync::EchoCallback& callback) {
        if (first_call) {
          first_call = false;
          ASSERT_TRUE(ptr->Echo(456, &result_value));
          EXPECT_EQ(456, result_value);
        }
        callback.Run(value);
      });

  ASSERT_TRUE(ptr->Echo(123, &result_value));
  EXPECT_EQ(123, result_value);
  this->Done();
}

TYPED_TEST(SyncMethodCommonTest, NestedSyncCallsWithOutOfOrderResponses) {
  // Test that we can call a sync method on an interface ptr, while there is
  // already a sync call ongoing. The responses arrive out of order.

  using Interface = typename TypeParam::Interface;
  InterfacePtr<Interface> interface_ptr;
  ImplTypeFor<Interface> impl(MakeRequest(&interface_ptr));
  auto ptr = TypeParam::Wrap(std::move(interface_ptr));

  // The same variable is used to store the output of the two sync calls, in
  // order to test that responses are handled in the correct order.
  int32_t result_value = -1;

  bool first_call = true;
  impl.set_echo_handler([&first_call, &ptr, &result_value](
      int32_t value, const TestSync::EchoCallback& callback) {
    callback.Run(value);
    if (first_call) {
      first_call = false;
      ASSERT_TRUE(ptr->Echo(456, &result_value));
      EXPECT_EQ(456, result_value);
    }
  });

  ASSERT_TRUE(ptr->Echo(123, &result_value));
  EXPECT_EQ(123, result_value);
}

SEQUENCED_TASK_RUNNER_TYPED_TEST(SyncMethodOnSequenceCommonTest,
                                 NestedSyncCallsWithOutOfOrderResponses) {
  // Test that we can call a sync method on an interface ptr, while there is
  // already a sync call ongoing. The responses arrive out of order.

  // The same variable is used to store the output of the two sync calls, in
  // order to test that responses are handled in the correct order.
  int32_t result_value = -1;

  bool first_call = true;
  auto& ptr = this->ptr_;
  auto& impl = *this->impl_;
  impl.set_echo_handler(
      [&first_call, &ptr, &result_value](
          int32_t value, const TestSync::EchoCallback& callback) {
        callback.Run(value);
        if (first_call) {
          first_call = false;
          ASSERT_TRUE(ptr->Echo(456, &result_value));
          EXPECT_EQ(456, result_value);
        }
      });

  ASSERT_TRUE(ptr->Echo(123, &result_value));
  EXPECT_EQ(123, result_value);
  this->Done();
}

TYPED_TEST(SyncMethodCommonTest, AsyncResponseQueuedDuringSyncCall) {
  // Test that while an interface pointer is waiting for the response to a sync
  // call, async responses are queued until the sync call completes.

  using Interface = typename TypeParam::Interface;
  InterfacePtr<Interface> interface_ptr;
  ImplTypeFor<Interface> impl(MakeRequest(&interface_ptr));
  auto ptr = TypeParam::Wrap(std::move(interface_ptr));

  int32_t async_echo_request_value = -1;
  TestSync::AsyncEchoCallback async_echo_request_callback;
  base::RunLoop run_loop1;
  impl.set_async_echo_handler(
      [&async_echo_request_value, &async_echo_request_callback, &run_loop1](
          int32_t value, const TestSync::AsyncEchoCallback& callback) {
        async_echo_request_value = value;
        async_echo_request_callback = callback;
        run_loop1.Quit();
      });

  bool async_echo_response_dispatched = false;
  base::RunLoop run_loop2;
  ptr->AsyncEcho(
      123,
      BindAsyncEchoCallback(
         [&async_echo_response_dispatched, &run_loop2](int32_t result) {
           async_echo_response_dispatched = true;
           EXPECT_EQ(123, result);
           run_loop2.Quit();
         }));
  // Run until the AsyncEcho request reaches the service side.
  run_loop1.Run();

  impl.set_echo_handler(
      [&async_echo_request_value, &async_echo_request_callback](
          int32_t value, const TestSync::EchoCallback& callback) {
        // Send back the async response first.
        EXPECT_FALSE(async_echo_request_callback.is_null());
        async_echo_request_callback.Run(async_echo_request_value);

        callback.Run(value);
      });

  int32_t result_value = -1;
  ASSERT_TRUE(ptr->Echo(456, &result_value));
  EXPECT_EQ(456, result_value);

  // Although the AsyncEcho response arrives before the Echo response, it should
  // be queued and not yet dispatched.
  EXPECT_FALSE(async_echo_response_dispatched);

  // Run until the AsyncEcho response is dispatched.
  run_loop2.Run();

  EXPECT_TRUE(async_echo_response_dispatched);
}

SEQUENCED_TASK_RUNNER_TYPED_TEST_F(SyncMethodOnSequenceCommonTest,
                                   AsyncResponseQueuedDuringSyncCall) {
  // Test that while an interface pointer is waiting for the response to a sync
  // call, async responses are queued until the sync call completes.

  void Run() override {
    this->impl_->set_async_echo_handler(
        [this](int32_t value, const TestSync::AsyncEchoCallback& callback) {
          async_echo_request_value_ = value;
          async_echo_request_callback_ = callback;
          OnAsyncEchoReceived();
        });

    this->ptr_->AsyncEcho(123, BindAsyncEchoCallback([this](int32_t result) {
                            async_echo_response_dispatched_ = true;
                            EXPECT_EQ(123, result);
                            EXPECT_TRUE(async_echo_response_dispatched_);
                            this->Done();
                          }));
  }

  // Called when the AsyncEcho request reaches the service side.
  void OnAsyncEchoReceived() {
    this->impl_->set_echo_handler(
        [this](int32_t value, const TestSync::EchoCallback& callback) {
          // Send back the async response first.
          EXPECT_FALSE(async_echo_request_callback_.is_null());
          async_echo_request_callback_.Run(async_echo_request_value_);

          callback.Run(value);
        });

    int32_t result_value = -1;
    ASSERT_TRUE(this->ptr_->Echo(456, &result_value));
    EXPECT_EQ(456, result_value);

    // Although the AsyncEcho response arrives before the Echo response, it
    // should be queued and not yet dispatched.
    EXPECT_FALSE(async_echo_response_dispatched_);
  }

  int32_t async_echo_request_value_ = -1;
  TestSync::AsyncEchoCallback async_echo_request_callback_;
  bool async_echo_response_dispatched_ = false;
};

TYPED_TEST(SyncMethodCommonTest, AsyncRequestQueuedDuringSyncCall) {
  // Test that while an interface pointer is waiting for the response to a sync
  // call, async requests for a binding running on the same thread are queued
  // until the sync call completes.

  using Interface = typename TypeParam::Interface;
  InterfacePtr<Interface> interface_ptr;
  ImplTypeFor<Interface> impl(MakeRequest(&interface_ptr));
  auto ptr = TypeParam::Wrap(std::move(interface_ptr));

  bool async_echo_request_dispatched = false;
  impl.set_async_echo_handler([&async_echo_request_dispatched](
      int32_t value, const TestSync::AsyncEchoCallback& callback) {
    async_echo_request_dispatched = true;
    callback.Run(value);
  });

  bool async_echo_response_dispatched = false;
  base::RunLoop run_loop;
  ptr->AsyncEcho(
      123,
      BindAsyncEchoCallback(
         [&async_echo_response_dispatched, &run_loop](int32_t result) {
           async_echo_response_dispatched = true;
           EXPECT_EQ(123, result);
           run_loop.Quit();
         }));

  impl.set_echo_handler([&async_echo_request_dispatched](
      int32_t value, const TestSync::EchoCallback& callback) {
    // Although the AsyncEcho request is sent before the Echo request, it
    // shouldn't be dispatched yet at this point, because there is an ongoing
    // sync call on the same thread.
    EXPECT_FALSE(async_echo_request_dispatched);
    callback.Run(value);
  });

  int32_t result_value = -1;
  ASSERT_TRUE(ptr->Echo(456, &result_value));
  EXPECT_EQ(456, result_value);

  // Although the AsyncEcho request is sent before the Echo request, it
  // shouldn't be dispatched yet.
  EXPECT_FALSE(async_echo_request_dispatched);

  // Run until the AsyncEcho response is dispatched.
  run_loop.Run();

  EXPECT_TRUE(async_echo_response_dispatched);
}

SEQUENCED_TASK_RUNNER_TYPED_TEST_F(SyncMethodOnSequenceCommonTest,
                                   AsyncRequestQueuedDuringSyncCall) {
  // Test that while an interface pointer is waiting for the response to a sync
  // call, async requests for a binding running on the same thread are queued
  // until the sync call completes.
  void Run() override {
    this->impl_->set_async_echo_handler(
        [this](int32_t value, const TestSync::AsyncEchoCallback& callback) {
          async_echo_request_dispatched_ = true;
          callback.Run(value);
        });

    this->ptr_->AsyncEcho(123, BindAsyncEchoCallback([this](int32_t result) {
                            EXPECT_EQ(123, result);
                            this->Done();
                          }));

    this->impl_->set_echo_handler(
        [this](int32_t value, const TestSync::EchoCallback& callback) {
          // Although the AsyncEcho request is sent before the Echo request, it
          // shouldn't be dispatched yet at this point, because there is an
          // ongoing
          // sync call on the same thread.
          EXPECT_FALSE(async_echo_request_dispatched_);
          callback.Run(value);
        });

    int32_t result_value = -1;
    ASSERT_TRUE(this->ptr_->Echo(456, &result_value));
    EXPECT_EQ(456, result_value);

    // Although the AsyncEcho request is sent before the Echo request, it
    // shouldn't be dispatched yet.
    EXPECT_FALSE(async_echo_request_dispatched_);
  }
  bool async_echo_request_dispatched_ = false;
};

TYPED_TEST(SyncMethodCommonTest,
           QueuedMessagesProcessedBeforeErrorNotification) {
  // Test that while an interface pointer is waiting for the response to a sync
  // call, async responses are queued. If the message pipe is disconnected
  // before the queued messages are processed, the connection error
  // notification is delayed until all the queued messages are processed.

  // ThreadSafeInterfacePtr doesn't guarantee that messages are delivered before
  // error notifications, so skip it for this test.
  if (TypeParam::kIsThreadSafeInterfacePtrTest)
    return;

  using Interface = typename TypeParam::Interface;
  InterfacePtr<Interface> ptr;
  ImplTypeFor<Interface> impl(MakeRequest(&ptr));

  int32_t async_echo_request_value = -1;
  TestSync::AsyncEchoCallback async_echo_request_callback;
  base::RunLoop run_loop1;
  impl.set_async_echo_handler(
      [&async_echo_request_value, &async_echo_request_callback, &run_loop1](
          int32_t value, const TestSync::AsyncEchoCallback& callback) {
        async_echo_request_value = value;
        async_echo_request_callback = callback;
        run_loop1.Quit();
      });

  bool async_echo_response_dispatched = false;
  bool connection_error_dispatched = false;
  base::RunLoop run_loop2;
  ptr->AsyncEcho(123, BindAsyncEchoCallback([&async_echo_response_dispatched,
                                             &connection_error_dispatched, &ptr,
                                             &run_loop2](int32_t result) {
                   async_echo_response_dispatched = true;
                   // At this point, error notification should not be dispatched
                   // yet.
                   EXPECT_FALSE(connection_error_dispatched);
                   EXPECT_FALSE(ptr.encountered_error());
                   EXPECT_EQ(123, result);
                   run_loop2.Quit();
                 }));
  // Run until the AsyncEcho request reaches the service side.
  run_loop1.Run();

  impl.set_echo_handler(
      [&impl, &async_echo_request_value, &async_echo_request_callback](
          int32_t value, const TestSync::EchoCallback& callback) {
        // Send back the async response first.
        EXPECT_FALSE(async_echo_request_callback.is_null());
        async_echo_request_callback.Run(async_echo_request_value);

        impl.binding()->Close();
      });

  base::RunLoop run_loop3;
  ptr.set_connection_error_handler(base::Bind(&SetFlagAndRunClosure,
                                              &connection_error_dispatched,
                                              run_loop3.QuitClosure()));

  int32_t result_value = -1;
  ASSERT_FALSE(ptr->Echo(456, &result_value));
  EXPECT_EQ(-1, result_value);
  ASSERT_FALSE(connection_error_dispatched);
  EXPECT_FALSE(ptr.encountered_error());

  // Although the AsyncEcho response arrives before the Echo response, it should
  // be queued and not yet dispatched.
  EXPECT_FALSE(async_echo_response_dispatched);

  // Run until the AsyncEcho response is dispatched.
  run_loop2.Run();

  EXPECT_TRUE(async_echo_response_dispatched);

  // Run until the error notification is dispatched.
  run_loop3.Run();

  ASSERT_TRUE(connection_error_dispatched);
  EXPECT_TRUE(ptr.encountered_error());
}

SEQUENCED_TASK_RUNNER_TYPED_TEST_F(
    SyncMethodOnSequenceCommonTest,
    QueuedMessagesProcessedBeforeErrorNotification) {
  // Test that while an interface pointer is waiting for the response to a sync
  // call, async responses are queued. If the message pipe is disconnected
  // before the queued messages are processed, the connection error
  // notification is delayed until all the queued messages are processed.

  void Run() override {
    this->impl_->set_async_echo_handler(
        [this](int32_t value, const TestSync::AsyncEchoCallback& callback) {
          OnAsyncEchoReachedService(value, callback);
        });

    this->ptr_->AsyncEcho(123, BindAsyncEchoCallback([this](int32_t result) {
                            async_echo_response_dispatched_ = true;
                            // At this point, error notification should not be
                            // dispatched
                            // yet.
                            EXPECT_FALSE(connection_error_dispatched_);
                            EXPECT_FALSE(this->ptr_.encountered_error());
                            EXPECT_EQ(123, result);
                            EXPECT_TRUE(async_echo_response_dispatched_);
                          }));
  }

  void OnAsyncEchoReachedService(int32_t value,
                                 const TestSync::AsyncEchoCallback& callback) {
    async_echo_request_value_ = value;
    async_echo_request_callback_ = callback;
    this->impl_->set_echo_handler(
        [this](int32_t value, const TestSync::EchoCallback& callback) {
          // Send back the async response first.
          EXPECT_FALSE(async_echo_request_callback_.is_null());
          async_echo_request_callback_.Run(async_echo_request_value_);

          this->impl_->binding()->Close();
        });

    this->ptr_.set_connection_error_handler(
        base::Bind(&SetFlagAndRunClosure, &connection_error_dispatched_,
                   LambdaBinder<>::BindLambda(
                       [this]() { OnErrorNotificationDispatched(); })));

    int32_t result_value = -1;
    ASSERT_FALSE(this->ptr_->Echo(456, &result_value));
    EXPECT_EQ(-1, result_value);
    ASSERT_FALSE(connection_error_dispatched_);
    EXPECT_FALSE(this->ptr_.encountered_error());

    // Although the AsyncEcho response arrives before the Echo response, it
    // should
    // be queued and not yet dispatched.
    EXPECT_FALSE(async_echo_response_dispatched_);
  }

  void OnErrorNotificationDispatched() {
    ASSERT_TRUE(connection_error_dispatched_);
    EXPECT_TRUE(this->ptr_.encountered_error());
    this->Done();
  }

  int32_t async_echo_request_value_ = -1;
  TestSync::AsyncEchoCallback async_echo_request_callback_;
  bool async_echo_response_dispatched_ = false;
  bool connection_error_dispatched_ = false;
};

TYPED_TEST(SyncMethodCommonTest, InvalidMessageDuringSyncCall) {
  // Test that while an interface pointer is waiting for the response to a sync
  // call, an invalid incoming message will disconnect the message pipe, cause
  // the sync call to return false, and run the connection error handler
  // asynchronously.

  using Interface = typename TypeParam::Interface;
  MessagePipe pipe;

  InterfacePtr<Interface> interface_ptr;
  interface_ptr.Bind(InterfacePtrInfo<Interface>(std::move(pipe.handle0), 0u));
  auto ptr = TypeParam::Wrap(std::move(interface_ptr));

  MessagePipeHandle raw_binding_handle = pipe.handle1.get();
  ImplTypeFor<Interface> impl(
      InterfaceRequest<Interface>(std::move(pipe.handle1)));

  impl.set_echo_handler([&raw_binding_handle](
      int32_t value, const TestSync::EchoCallback& callback) {
    // Write a 1-byte message, which is considered invalid.
    char invalid_message = 0;
    MojoResult result =
        WriteMessageRaw(raw_binding_handle, &invalid_message, 1u, nullptr, 0u,
                        MOJO_WRITE_MESSAGE_FLAG_NONE);
    ASSERT_EQ(MOJO_RESULT_OK, result);
    callback.Run(value);
  });

  bool connection_error_dispatched = false;
  base::RunLoop run_loop;
  // ThreadSafeInterfacePtr doesn't support setting connection error handlers.
  if (!TypeParam::kIsThreadSafeInterfacePtrTest) {
    ptr.set_connection_error_handler(base::Bind(&SetFlagAndRunClosure,
                                                &connection_error_dispatched,
                                                run_loop.QuitClosure()));
  }

  int32_t result_value = -1;
  ASSERT_FALSE(ptr->Echo(456, &result_value));
  EXPECT_EQ(-1, result_value);
  ASSERT_FALSE(connection_error_dispatched);

  if (!TypeParam::kIsThreadSafeInterfacePtrTest) {
    run_loop.Run();
    ASSERT_TRUE(connection_error_dispatched);
  }
}

SEQUENCED_TASK_RUNNER_TYPED_TEST_F(SyncMethodOnSequenceCommonTest,
                                   InvalidMessageDuringSyncCall) {
  // Test that while an interface pointer is waiting for the response to a sync
  // call, an invalid incoming message will disconnect the message pipe, cause
  // the sync call to return false, and run the connection error handler
  // asynchronously.

  void Run() override {
    MessagePipe pipe;

    using InterfaceType = typename TypeParam::Interface;
    this->ptr_.Bind(
        InterfacePtrInfo<InterfaceType>(std::move(pipe.handle0), 0u));

    MessagePipeHandle raw_binding_handle = pipe.handle1.get();
    this->impl_ = std::make_unique<ImplTypeFor<InterfaceType>>(
        InterfaceRequest<InterfaceType>(std::move(pipe.handle1)));

    this->impl_->set_echo_handler(
        [raw_binding_handle](int32_t value,
                             const TestSync::EchoCallback& callback) {
          // Write a 1-byte message, which is considered invalid.
          char invalid_message = 0;
          MojoResult result =
              WriteMessageRaw(raw_binding_handle, &invalid_message, 1u, nullptr,
                              0u, MOJO_WRITE_MESSAGE_FLAG_NONE);
          ASSERT_EQ(MOJO_RESULT_OK, result);
          callback.Run(value);
        });

    this->ptr_.set_connection_error_handler(
        LambdaBinder<>::BindLambda([this]() {
          connection_error_dispatched_ = true;
          this->Done();
        }));

    int32_t result_value = -1;
    ASSERT_FALSE(this->ptr_->Echo(456, &result_value));
    EXPECT_EQ(-1, result_value);
    ASSERT_FALSE(connection_error_dispatched_);
  }
  bool connection_error_dispatched_ = false;
};

TEST_F(SyncMethodAssociatedTest, ReenteredBySyncMethodAssoBindingOfSameRouter) {
  // Test that an interface pointer waiting for a sync call response can be
  // reentered by an associated binding serving sync methods on the same thread.
  // The associated binding belongs to the same MultiplexRouter as the waiting
  // interface pointer.

  TestSyncAssociatedImpl opposite_asso_impl(std::move(opposite_asso_request_));
  TestSyncAssociatedPtr opposite_asso_ptr;
  opposite_asso_ptr.Bind(std::move(opposite_asso_ptr_info_));

  master_impl_->set_echo_handler([&opposite_asso_ptr](
      int32_t value, const TestSyncMaster::EchoCallback& callback) {
    int32_t result_value = -1;

    ASSERT_TRUE(opposite_asso_ptr->Echo(123, &result_value));
    EXPECT_EQ(123, result_value);
    callback.Run(value);
  });

  int32_t result_value = -1;
  ASSERT_TRUE(master_ptr_->Echo(456, &result_value));
  EXPECT_EQ(456, result_value);
}

TEST_F(SyncMethodAssociatedTest,
       ReenteredBySyncMethodAssoBindingOfDifferentRouter) {
  // Test that an interface pointer waiting for a sync call response can be
  // reentered by an associated binding serving sync methods on the same thread.
  // The associated binding belongs to a different MultiplexRouter as the
  // waiting interface pointer.

  TestSyncAssociatedImpl asso_impl(std::move(asso_request_));
  TestSyncAssociatedPtr asso_ptr;
  asso_ptr.Bind(std::move(asso_ptr_info_));

  master_impl_->set_echo_handler(
      [&asso_ptr](int32_t value, const TestSyncMaster::EchoCallback& callback) {
        int32_t result_value = -1;

        ASSERT_TRUE(asso_ptr->Echo(123, &result_value));
        EXPECT_EQ(123, result_value);
        callback.Run(value);
      });

  int32_t result_value = -1;
  ASSERT_TRUE(master_ptr_->Echo(456, &result_value));
  EXPECT_EQ(456, result_value);
}

// TODO(yzshen): Add more tests related to associated interfaces.

}  // namespace
}  // namespace test
}  // namespace mojo
