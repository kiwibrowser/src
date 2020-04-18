// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: This file tests both binding.h (mojo::Binding) and strong_binding.h
// (mojo::StrongBinding).

#include "mojo/public/cpp/bindings/binding.h"

#include <stdint.h>
#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/bindings/tests/bindings_test_base.h"
#include "mojo/public/interfaces/bindings/tests/ping_service.mojom.h"
#include "mojo/public/interfaces/bindings/tests/sample_interfaces.mojom.h"
#include "mojo/public/interfaces/bindings/tests/sample_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

class ServiceImpl : public sample::Service {
 public:
  explicit ServiceImpl(bool* was_deleted = nullptr)
      : was_deleted_(was_deleted) {}
  ~ServiceImpl() override {
    if (was_deleted_)
      *was_deleted_ = true;
  }

 private:
  // sample::Service implementation
  void Frobinate(sample::FooPtr foo,
                 BazOptions options,
                 sample::PortPtr port,
                 const FrobinateCallback& callback) override {
    callback.Run(1);
  }
  void GetPort(InterfaceRequest<sample::Port> port) override {}

  bool* const was_deleted_;

  DISALLOW_COPY_AND_ASSIGN(ServiceImpl);
};

template <typename... Args>
void DoSetFlagAndRunClosure(bool* flag,
                            const base::Closure& closure,
                            Args... args) {
  *flag = true;
  if (!closure.is_null())
    closure.Run();
}

template <typename... Args>
base::Callback<void(Args...)> SetFlagAndRunClosure(
    bool* flag,
    const base::Closure& callback = base::Closure()) {
  return base::Bind(&DoSetFlagAndRunClosure<Args...>, flag, callback);
}

// BindingTest -----------------------------------------------------------------

using BindingTest = BindingsTestBase;

TEST_P(BindingTest, Close) {
  bool called = false;
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  base::RunLoop run_loop;
  ptr.set_connection_error_handler(
      SetFlagAndRunClosure(&called, run_loop.QuitClosure()));
  ServiceImpl impl;
  Binding<sample::Service> binding(&impl, std::move(request));

  binding.Close();
  EXPECT_FALSE(called);
  run_loop.Run();
  EXPECT_TRUE(called);
}

// Tests that destroying a mojo::Binding closes the bound message pipe handle.
TEST_P(BindingTest, DestroyClosesMessagePipe) {
  bool encountered_error = false;
  ServiceImpl impl;
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  base::RunLoop run_loop;
  ptr.set_connection_error_handler(
      SetFlagAndRunClosure(&encountered_error, run_loop.QuitClosure()));
  bool called = false;
  base::RunLoop run_loop2;
  {
    Binding<sample::Service> binding(&impl, std::move(request));
    ptr->Frobinate(nullptr, sample::Service::BazOptions::REGULAR, nullptr,
                   SetFlagAndRunClosure<int32_t>(&called,
                                                 run_loop2.QuitClosure()));
    run_loop2.Run();
    EXPECT_TRUE(called);
    EXPECT_FALSE(encountered_error);
  }
  // Now that the Binding is out of scope we should detect an error on the other
  // end of the pipe.
  run_loop.Run();
  EXPECT_TRUE(encountered_error);

  // And calls should fail.
  called = false;
  ptr->Frobinate(nullptr, sample::Service::BazOptions::REGULAR, nullptr,
                 SetFlagAndRunClosure<int32_t>(&called,
                                               run_loop2.QuitClosure()));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(called);
}

// Tests that the binding's connection error handler gets called when the other
// end is closed.
TEST_P(BindingTest, ConnectionError) {
  bool called = false;
  {
    ServiceImpl impl;
    sample::ServicePtr ptr;
    Binding<sample::Service> binding(&impl, MakeRequest(&ptr));
    base::RunLoop run_loop;
    binding.set_connection_error_handler(
        SetFlagAndRunClosure(&called, run_loop.QuitClosure()));
    ptr.reset();
    EXPECT_FALSE(called);
    run_loop.Run();
    EXPECT_TRUE(called);
    // We want to make sure that it isn't called again during destruction.
    called = false;
  }
  EXPECT_FALSE(called);
}

// Tests that calling Close doesn't result in the connection error handler being
// called.
TEST_P(BindingTest, CloseDoesntCallConnectionErrorHandler) {
  ServiceImpl impl;
  sample::ServicePtr ptr;
  Binding<sample::Service> binding(&impl, MakeRequest(&ptr));
  bool called = false;
  binding.set_connection_error_handler(SetFlagAndRunClosure(&called));
  binding.Close();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(called);

  // We can also close the other end, and the error handler still won't be
  // called.
  ptr.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(called);
}

class ServiceImplWithBinding : public ServiceImpl {
 public:
  ServiceImplWithBinding(bool* was_deleted,
                         const base::Closure& closure,
                         InterfaceRequest<sample::Service> request)
      : ServiceImpl(was_deleted),
        binding_(this, std::move(request)),
        closure_(closure) {
    binding_.set_connection_error_handler(
        base::Bind(&ServiceImplWithBinding::OnConnectionError,
                   base::Unretained(this)));
  }

 private:
  ~ServiceImplWithBinding() override{
    closure_.Run();
  }

  void OnConnectionError() { delete this; }

  Binding<sample::Service> binding_;
  base::Closure closure_;

  DISALLOW_COPY_AND_ASSIGN(ServiceImplWithBinding);
};

// Tests that the binding may be deleted in the connection error handler.
TEST_P(BindingTest, SelfDeleteOnConnectionError) {
  bool was_deleted = false;
  sample::ServicePtr ptr;
  // This should delete itself on connection error.
  base::RunLoop run_loop;
  new ServiceImplWithBinding(&was_deleted, run_loop.QuitClosure(),
                             MakeRequest(&ptr));
  ptr.reset();
  EXPECT_FALSE(was_deleted);
  run_loop.Run();
  EXPECT_TRUE(was_deleted);
}

// Tests that explicitly calling Unbind followed by rebinding works.
TEST_P(BindingTest, Unbind) {
  ServiceImpl impl;
  sample::ServicePtr ptr;
  Binding<sample::Service> binding(&impl, MakeRequest(&ptr));

  bool called = false;
  base::RunLoop run_loop;
  ptr->Frobinate(nullptr, sample::Service::BazOptions::REGULAR, nullptr,
                 SetFlagAndRunClosure<int32_t>(&called,
                                               run_loop.QuitClosure()));
  run_loop.Run();
  EXPECT_TRUE(called);

  called = false;
  auto request = binding.Unbind();
  EXPECT_FALSE(binding.is_bound());
  // All calls should fail when not bound...
  ptr->Frobinate(nullptr, sample::Service::BazOptions::REGULAR, nullptr,
                 SetFlagAndRunClosure<int32_t>(&called,
                                               run_loop.QuitClosure()));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(called);

  called = false;
  binding.Bind(std::move(request));
  EXPECT_TRUE(binding.is_bound());
  // ...and should succeed again when the rebound.
  base::RunLoop run_loop2;
  ptr->Frobinate(nullptr, sample::Service::BazOptions::REGULAR, nullptr,
                 SetFlagAndRunClosure<int32_t>(&called,
                                               run_loop2.QuitClosure()));
  run_loop2.Run();
  EXPECT_TRUE(called);
}

class IntegerAccessorImpl : public sample::IntegerAccessor {
 public:
  IntegerAccessorImpl() {}
  ~IntegerAccessorImpl() override {}

 private:
  // sample::IntegerAccessor implementation.
  void GetInteger(const GetIntegerCallback& callback) override {
    callback.Run(1, sample::Enum::VALUE);
  }
  void SetInteger(int64_t data, sample::Enum type) override {}

  DISALLOW_COPY_AND_ASSIGN(IntegerAccessorImpl);
};

TEST_P(BindingTest, PauseResume) {
  bool called = false;
  base::RunLoop run_loop;
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  ServiceImpl impl;
  Binding<sample::Service> binding(&impl, std::move(request));
  binding.PauseIncomingMethodCallProcessing();
  ptr->Frobinate(nullptr, sample::Service::BazOptions::REGULAR, nullptr,
                 SetFlagAndRunClosure<int32_t>(&called,
                                               run_loop.QuitClosure()));
  EXPECT_FALSE(called);
  base::RunLoop().RunUntilIdle();
  // Frobinate() should not be called as the binding is paused.
  EXPECT_FALSE(called);

  // Resume the binding, which should trigger processing.
  binding.ResumeIncomingMethodCallProcessing();
  run_loop.Run();
  EXPECT_TRUE(called);
}

// Verifies the connection error handler is not run while a binding is paused.
TEST_P(BindingTest, ErrorHandleNotRunWhilePaused) {
  bool called = false;
  base::RunLoop run_loop;
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  ServiceImpl impl;
  Binding<sample::Service> binding(&impl, std::move(request));
  binding.set_connection_error_handler(
      SetFlagAndRunClosure(&called, run_loop.QuitClosure()));
  binding.PauseIncomingMethodCallProcessing();

  ptr.reset();
  base::RunLoop().RunUntilIdle();
  // The connection error handle should not be called as the binding is paused.
  EXPECT_FALSE(called);

  // Resume the binding, which should trigger the error handler.
  binding.ResumeIncomingMethodCallProcessing();
  run_loop.Run();
  EXPECT_TRUE(called);
}

class PingServiceImpl : public test::PingService {
 public:
  PingServiceImpl() {}
  ~PingServiceImpl() override {}

  // test::PingService:
  void Ping(const PingCallback& callback) override {
    if (!ping_handler_.is_null())
      ping_handler_.Run();
    callback.Run();
  }

  void set_ping_handler(const base::Closure& handler) {
    ping_handler_ = handler;
  }

 private:
  base::Closure ping_handler_;

  DISALLOW_COPY_AND_ASSIGN(PingServiceImpl);
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

// Verifies that message filters are notified in the order they were added and
// are always notified before a message is dispatched.
TEST_P(BindingTest, MessageFilter) {
  test::PingServicePtr ptr;
  PingServiceImpl impl;
  mojo::Binding<test::PingService> binding(&impl, MakeRequest(&ptr));

  int status = 0;
  auto handler_helper = [] (int* status, int expected_status, int new_status) {
    EXPECT_EQ(expected_status, *status);
    *status = new_status;
  };
  auto create_handler = [&] (int expected_status, int new_status) {
    return base::Bind(handler_helper, &status, expected_status, new_status);
  };

  binding.AddFilter(CallbackFilter::Wrap(create_handler(0, 1)));
  binding.AddFilter(CallbackFilter::Wrap(create_handler(1, 2)));
  impl.set_ping_handler(create_handler(2, 3));

  for (int i = 0; i < 10; ++i) {
    status = 0;
    base::RunLoop loop;
    ptr->Ping(loop.QuitClosure());
    loop.Run();
    EXPECT_EQ(3, status);
  }
}

void Fail() {
  FAIL() << "Unexpected connection error";
}

TEST_P(BindingTest, FlushForTesting) {
  bool called = false;
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  ServiceImpl impl;
  Binding<sample::Service> binding(&impl, std::move(request));
  binding.set_connection_error_handler(base::Bind(&Fail));

  ptr->Frobinate(nullptr, sample::Service::BazOptions::REGULAR, nullptr,
                 SetFlagAndRunClosure<int32_t>(&called));
  EXPECT_FALSE(called);
  // Because the flush is sent from the binding, it only guarantees that the
  // request has been received, not the response. The second flush waits for the
  // response to be received.
  binding.FlushForTesting();
  binding.FlushForTesting();
  EXPECT_TRUE(called);
}

TEST_P(BindingTest, FlushForTestingWithClosedPeer) {
  bool called = false;
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  ServiceImpl impl;
  Binding<sample::Service> binding(&impl, std::move(request));
  binding.set_connection_error_handler(SetFlagAndRunClosure(&called));
  ptr.reset();

  EXPECT_FALSE(called);
  binding.FlushForTesting();
  EXPECT_TRUE(called);
  binding.FlushForTesting();
}

TEST_P(BindingTest, ConnectionErrorWithReason) {
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  ServiceImpl impl;
  Binding<sample::Service> binding(&impl, std::move(request));

  base::RunLoop run_loop;
  binding.set_connection_error_with_reason_handler(base::Bind(
      [](const base::Closure& quit_closure, uint32_t custom_reason,
         const std::string& description) {
        EXPECT_EQ(1234u, custom_reason);
        EXPECT_EQ("hello", description);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  ptr.ResetWithReason(1234u, "hello");

  run_loop.Run();
}

template <typename T>
struct WeakPtrImplRefTraits {
  using PointerType = base::WeakPtr<T>;

  static bool IsNull(const base::WeakPtr<T>& ptr) { return !ptr; }
  static T* GetRawPointer(base::WeakPtr<T>* ptr) { return ptr->get(); }
};

template <typename T>
using WeakBinding = Binding<T, WeakPtrImplRefTraits<T>>;

TEST_P(BindingTest, CustomImplPointerType) {
  PingServiceImpl impl;
  base::WeakPtrFactory<test::PingService> weak_factory(&impl);

  test::PingServicePtr proxy;
  WeakBinding<test::PingService> binding(weak_factory.GetWeakPtr(),
                                         MakeRequest(&proxy));

  {
    // Ensure the binding is functioning.
    base::RunLoop run_loop;
    proxy->Ping(run_loop.QuitClosure());
    run_loop.Run();
  }

  {
    // Attempt to dispatch another message after the WeakPtr is invalidated.
    base::Closure assert_not_reached = base::Bind([] { NOTREACHED(); });
    impl.set_ping_handler(assert_not_reached);
    proxy->Ping(assert_not_reached);

    // The binding will close its end of the pipe which will trigger a
    // connection error on |proxy|.
    base::RunLoop run_loop;
    proxy.set_connection_error_handler(run_loop.QuitClosure());
    weak_factory.InvalidateWeakPtrs();
    run_loop.Run();
  }
}

TEST_P(BindingTest, ReportBadMessage) {
  bool called = false;
  test::PingServicePtr ptr;
  auto request = MakeRequest(&ptr);
  base::RunLoop run_loop;
  ptr.set_connection_error_handler(
      SetFlagAndRunClosure(&called, run_loop.QuitClosure()));
  PingServiceImpl impl;
  Binding<test::PingService> binding(&impl, std::move(request));
  impl.set_ping_handler(base::Bind(
      [](Binding<test::PingService>* binding) {
        binding->ReportBadMessage("received bad message");
      },
      &binding));

  std::string received_error;
  edk::SetDefaultProcessErrorCallback(
      base::Bind([](std::string* out_error,
                    const std::string& error) { *out_error = error; },
                 &received_error));

  ptr->Ping(base::Bind([] {}));
  EXPECT_FALSE(called);
  run_loop.Run();
  EXPECT_TRUE(called);
  EXPECT_EQ("received bad message", received_error);

  edk::SetDefaultProcessErrorCallback(mojo::edk::ProcessErrorCallback());
}

TEST_P(BindingTest, GetBadMessageCallback) {
  test::PingServicePtr ptr;
  auto request = MakeRequest(&ptr);
  base::RunLoop run_loop;
  PingServiceImpl impl;
  ReportBadMessageCallback bad_message_callback;

  std::string received_error;
  edk::SetDefaultProcessErrorCallback(
      base::Bind([](std::string* out_error,
                    const std::string& error) { *out_error = error; },
                 &received_error));

  {
    Binding<test::PingService> binding(&impl, std::move(request));
    impl.set_ping_handler(base::Bind(
        [](Binding<test::PingService>* binding,
           ReportBadMessageCallback* out_callback) {
          *out_callback = binding->GetBadMessageCallback();
        },
        &binding, &bad_message_callback));
    ptr->Ping(run_loop.QuitClosure());
    run_loop.Run();
    EXPECT_TRUE(received_error.empty());
    EXPECT_TRUE(bad_message_callback);
  }

  std::move(bad_message_callback).Run("delayed bad message");
  EXPECT_EQ("delayed bad message", received_error);

  edk::SetDefaultProcessErrorCallback(mojo::edk::ProcessErrorCallback());
}

// StrongBindingTest -----------------------------------------------------------

using StrongBindingTest = BindingsTestBase;

// Tests that destroying a mojo::StrongBinding closes the bound message pipe
// handle but does *not* destroy the implementation object.
TEST_P(StrongBindingTest, DestroyClosesMessagePipe) {
  base::RunLoop run_loop;
  bool encountered_error = false;
  bool was_deleted = false;
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  ptr.set_connection_error_handler(
      SetFlagAndRunClosure(&encountered_error, run_loop.QuitClosure()));
  bool called = false;
  base::RunLoop run_loop2;

  auto binding = MakeStrongBinding(std::make_unique<ServiceImpl>(&was_deleted),
                                   std::move(request));
  ptr->Frobinate(
      nullptr, sample::Service::BazOptions::REGULAR, nullptr,
      SetFlagAndRunClosure<int32_t>(&called, run_loop2.QuitClosure()));
  run_loop2.Run();
  EXPECT_TRUE(called);
  EXPECT_FALSE(encountered_error);
  binding->Close();

  // Now that the StrongBinding is closed we should detect an error on the other
  // end of the pipe.
  run_loop.Run();
  EXPECT_TRUE(encountered_error);

  // Destroying the StrongBinding also destroys the impl.
  ASSERT_TRUE(was_deleted);
}

// Tests the typical case, where the implementation object owns the
// StrongBinding (and should be destroyed on connection error).
TEST_P(StrongBindingTest, ConnectionErrorDestroysImpl) {
  sample::ServicePtr ptr;
  bool was_deleted = false;
  // Will delete itself.
  base::RunLoop run_loop;
  new ServiceImplWithBinding(&was_deleted, run_loop.QuitClosure(),
                             MakeRequest(&ptr));

  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(was_deleted);

  ptr.reset();
  EXPECT_FALSE(was_deleted);
  run_loop.Run();
  EXPECT_TRUE(was_deleted);
}

TEST_P(StrongBindingTest, FlushForTesting) {
  bool called = false;
  bool was_deleted = false;
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  auto binding = MakeStrongBinding(std::make_unique<ServiceImpl>(&was_deleted),
                                   std::move(request));
  binding->set_connection_error_handler(base::Bind(&Fail));

  ptr->Frobinate(nullptr, sample::Service::BazOptions::REGULAR, nullptr,
                 SetFlagAndRunClosure<int32_t>(&called));
  EXPECT_FALSE(called);
  // Because the flush is sent from the binding, it only guarantees that the
  // request has been received, not the response. The second flush waits for the
  // response to be received.
  ASSERT_TRUE(binding);
  binding->FlushForTesting();
  ASSERT_TRUE(binding);
  binding->FlushForTesting();
  EXPECT_TRUE(called);
  EXPECT_FALSE(was_deleted);
  ptr.reset();
  ASSERT_TRUE(binding);
  binding->set_connection_error_handler(base::Closure());
  binding->FlushForTesting();
  EXPECT_TRUE(was_deleted);
}

TEST_P(StrongBindingTest, FlushForTestingWithClosedPeer) {
  bool called = false;
  bool was_deleted = false;
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  auto binding = MakeStrongBinding(std::make_unique<ServiceImpl>(&was_deleted),
                                   std::move(request));
  binding->set_connection_error_handler(SetFlagAndRunClosure(&called));
  ptr.reset();

  EXPECT_FALSE(called);
  EXPECT_FALSE(was_deleted);
  ASSERT_TRUE(binding);
  binding->FlushForTesting();
  EXPECT_TRUE(called);
  EXPECT_TRUE(was_deleted);
  ASSERT_FALSE(binding);
}

TEST_P(StrongBindingTest, ConnectionErrorWithReason) {
  sample::ServicePtr ptr;
  auto request = MakeRequest(&ptr);
  auto binding =
      MakeStrongBinding(std::make_unique<ServiceImpl>(), std::move(request));
  base::RunLoop run_loop;
  binding->set_connection_error_with_reason_handler(base::Bind(
      [](const base::Closure& quit_closure, uint32_t custom_reason,
         const std::string& description) {
        EXPECT_EQ(5678u, custom_reason);
        EXPECT_EQ("hello", description);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  ptr.ResetWithReason(5678u, "hello");

  run_loop.Run();
}

INSTANTIATE_MOJO_BINDINGS_TEST_CASE_P(BindingTest);
INSTANTIATE_MOJO_BINDINGS_TEST_CASE_P(StrongBindingTest);

}  // namespace
}  // mojo
