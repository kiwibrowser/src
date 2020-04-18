
// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/bindings/tests/bindings_test_base.h"
#include "mojo/public/cpp/bindings/thread_safe_interface_ptr.h"
#include "mojo/public/interfaces/bindings/tests/math_calculator.mojom.h"
#include "mojo/public/interfaces/bindings/tests/sample_interfaces.mojom.h"
#include "mojo/public/interfaces/bindings/tests/sample_service.mojom.h"
#include "mojo/public/interfaces/bindings/tests/scoping.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

typedef base::Callback<void(double)> CalcCallback;

class MathCalculatorImpl : public math::Calculator {
 public:
  explicit MathCalculatorImpl(InterfaceRequest<math::Calculator> request)
      : total_(0.0), binding_(this, std::move(request)) {}
  ~MathCalculatorImpl() override {}

  void Clear(const CalcCallback& callback) override {
    total_ = 0.0;
    callback.Run(total_);
  }

  void Add(double value, const CalcCallback& callback) override {
    total_ += value;
    callback.Run(total_);
  }

  void Multiply(double value, const CalcCallback& callback) override {
    total_ *= value;
    callback.Run(total_);
  }

  Binding<math::Calculator>* binding() { return &binding_; }

 private:
  double total_;
  Binding<math::Calculator> binding_;
};

class MathCalculatorUI {
 public:
  explicit MathCalculatorUI(math::CalculatorPtr calculator)
      : calculator_(std::move(calculator)),
        output_(0.0) {}

  bool encountered_error() const { return calculator_.encountered_error(); }
  void set_connection_error_handler(const base::Closure& closure) {
    calculator_.set_connection_error_handler(closure);
  }

  void Add(double value, const base::Closure& closure) {
    calculator_->Add(
        value,
        base::Bind(&MathCalculatorUI::Output, base::Unretained(this), closure));
  }

  void Multiply(double value, const base::Closure& closure) {
    calculator_->Multiply(
        value,
        base::Bind(&MathCalculatorUI::Output, base::Unretained(this), closure));
  }

  double GetOutput() const { return output_; }

  math::CalculatorPtr& GetInterfacePtr() { return calculator_; }

 private:
  void Output(const base::Closure& closure, double output) {
    output_ = output;
    if (!closure.is_null())
      closure.Run();
  }

  math::CalculatorPtr calculator_;
  double output_;
  base::Closure closure_;
};

class SelfDestructingMathCalculatorUI {
 public:
  explicit SelfDestructingMathCalculatorUI(math::CalculatorPtr calculator)
      : calculator_(std::move(calculator)), nesting_level_(0) {
    ++num_instances_;
  }

  void BeginTest(bool nested, const base::Closure& closure) {
    nesting_level_ = nested ? 2 : 1;
    calculator_->Add(
        1.0,
        base::Bind(&SelfDestructingMathCalculatorUI::Output,
                   base::Unretained(this), closure));
  }

  static int num_instances() { return num_instances_; }

  void Output(const base::Closure& closure, double value) {
    if (--nesting_level_ > 0) {
      // Add some more and wait for re-entrant call to Output!
      calculator_->Add(
          1.0,
          base::Bind(&SelfDestructingMathCalculatorUI::Output,
                     base::Unretained(this), closure));
    } else {
      closure.Run();
      delete this;
    }
  }

 private:
  ~SelfDestructingMathCalculatorUI() { --num_instances_; }

  math::CalculatorPtr calculator_;
  int nesting_level_;
  static int num_instances_;
};

// static
int SelfDestructingMathCalculatorUI::num_instances_ = 0;

class ReentrantServiceImpl : public sample::Service {
 public:
  ~ReentrantServiceImpl() override {}

  explicit ReentrantServiceImpl(InterfaceRequest<sample::Service> request)
      : call_depth_(0),
        max_call_depth_(0),
        binding_(this, std::move(request)) {}

  int max_call_depth() { return max_call_depth_; }

  void Frobinate(sample::FooPtr foo,
                 sample::Service::BazOptions baz,
                 sample::PortPtr port,
                 const sample::Service::FrobinateCallback& callback) override {
    max_call_depth_ = std::max(++call_depth_, max_call_depth_);
    if (call_depth_ == 1) {
      EXPECT_TRUE(binding_.WaitForIncomingMethodCall());
    }
    call_depth_--;
    callback.Run(5);
  }

  void GetPort(mojo::InterfaceRequest<sample::Port> port) override {}

 private:
  int call_depth_;
  int max_call_depth_;
  Binding<sample::Service> binding_;
};

class IntegerAccessorImpl : public sample::IntegerAccessor {
 public:
  IntegerAccessorImpl() : integer_(0) {}
  ~IntegerAccessorImpl() override {}

  int64_t integer() const { return integer_; }

  void set_closure(const base::Closure& closure) { closure_ = closure; }

 private:
  // sample::IntegerAccessor implementation.
  void GetInteger(const GetIntegerCallback& callback) override {
    callback.Run(integer_, sample::Enum::VALUE);
  }
  void SetInteger(int64_t data, sample::Enum type) override {
    integer_ = data;
    if (!closure_.is_null()) {
      closure_.Run();
      closure_.Reset();
    }
  }

  int64_t integer_;
  base::Closure closure_;
};

class InterfacePtrTest : public BindingsTestBase {
 public:
  InterfacePtrTest() {}
  ~InterfacePtrTest() override { base::RunLoop().RunUntilIdle(); }

  void PumpMessages() { base::RunLoop().RunUntilIdle(); }
};

void SetFlagAndRunClosure(bool* flag, const base::Closure& closure) {
  *flag = true;
  closure.Run();
}

void IgnoreValueAndRunClosure(const base::Closure& closure, int32_t value) {
  closure.Run();
}

void ExpectValueAndRunClosure(uint32_t expected_value,
                              const base::Closure& closure,
                              uint32_t value) {
  EXPECT_EQ(expected_value, value);
  closure.Run();
}

TEST_P(InterfacePtrTest, IsBound) {
  math::CalculatorPtr calc;
  EXPECT_FALSE(calc.is_bound());
  MathCalculatorImpl calc_impl(MakeRequest(&calc));
  EXPECT_TRUE(calc.is_bound());
}

class EndToEndInterfacePtrTest : public InterfacePtrTest {
 public:
  void RunTest(const scoped_refptr<base::SequencedTaskRunner> runner) {
    base::RunLoop run_loop;
    done_closure_ = run_loop.QuitClosure();
    done_runner_ = base::ThreadTaskRunnerHandle::Get();
    runner->PostTask(FROM_HERE,
                     base::Bind(&EndToEndInterfacePtrTest::RunTestImpl,
                                base::Unretained(this)));
    run_loop.Run();
  }

 private:
  void RunTestImpl() {
    math::CalculatorPtr calc;
    calc_impl_ = std::make_unique<MathCalculatorImpl>(MakeRequest(&calc));
    calculator_ui_ = std::make_unique<MathCalculatorUI>(std::move(calc));
    calculator_ui_->Add(2.0, base::Bind(&EndToEndInterfacePtrTest::AddDone,
                                        base::Unretained(this)));
    calculator_ui_->Multiply(5.0,
                             base::Bind(&EndToEndInterfacePtrTest::MultiplyDone,
                                        base::Unretained(this)));
    EXPECT_EQ(0.0, calculator_ui_->GetOutput());
  }

  void AddDone() { EXPECT_EQ(2.0, calculator_ui_->GetOutput()); }

  void MultiplyDone() {
    EXPECT_EQ(10.0, calculator_ui_->GetOutput());
    calculator_ui_.reset();
    calc_impl_.reset();
    done_runner_->PostTask(FROM_HERE, base::ResetAndReturn(&done_closure_));
  }

  base::Closure done_closure_;
  scoped_refptr<base::SingleThreadTaskRunner> done_runner_;
  std::unique_ptr<MathCalculatorUI> calculator_ui_;
  std::unique_ptr<MathCalculatorImpl> calc_impl_;
};

TEST_P(EndToEndInterfacePtrTest, EndToEnd) {
  RunTest(base::ThreadTaskRunnerHandle::Get());
}

TEST_P(EndToEndInterfacePtrTest, EndToEndOnSequence) {
  RunTest(base::CreateSequencedTaskRunnerWithTraits({}));
}

TEST_P(InterfacePtrTest, Movable) {
  math::CalculatorPtr a;
  math::CalculatorPtr b;
  MathCalculatorImpl calc_impl(MakeRequest(&b));

  EXPECT_TRUE(!a);
  EXPECT_FALSE(!b);

  a = std::move(b);

  EXPECT_FALSE(!a);
  EXPECT_TRUE(!b);
}

TEST_P(InterfacePtrTest, Resettable) {
  math::CalculatorPtr a;

  EXPECT_TRUE(!a);

  MessagePipe pipe;

  // Save this so we can test it later.
  Handle handle = pipe.handle0.get();

  a = MakeProxy(
      InterfacePtrInfo<math::Calculator>(std::move(pipe.handle0), 0u));

  EXPECT_FALSE(!a);

  a.reset();

  EXPECT_TRUE(!a);
  EXPECT_FALSE(a.internal_state()->is_bound());

  // Test that handle was closed.
  EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, CloseRaw(handle));
}

TEST_P(InterfacePtrTest, BindInvalidHandle) {
  math::CalculatorPtr ptr;
  EXPECT_FALSE(ptr.get());
  EXPECT_FALSE(ptr);

  ptr.Bind(InterfacePtrInfo<math::Calculator>());
  EXPECT_FALSE(ptr.get());
  EXPECT_FALSE(ptr);
}

TEST_P(InterfacePtrTest, EncounteredError) {
  math::CalculatorPtr proxy;
  MathCalculatorImpl calc_impl(MakeRequest(&proxy));

  MathCalculatorUI calculator_ui(std::move(proxy));

  base::RunLoop run_loop;
  calculator_ui.Add(2.0, run_loop.QuitClosure());
  run_loop.Run();
  EXPECT_EQ(2.0, calculator_ui.GetOutput());
  EXPECT_FALSE(calculator_ui.encountered_error());

  calculator_ui.Multiply(5.0, base::Closure());
  EXPECT_FALSE(calculator_ui.encountered_error());

  // Close the server.
  calc_impl.binding()->Close();

  // The state change isn't picked up locally yet.
  base::RunLoop run_loop2;
  calculator_ui.set_connection_error_handler(run_loop2.QuitClosure());
  EXPECT_FALSE(calculator_ui.encountered_error());

  run_loop2.Run();

  // OK, now we see the error.
  EXPECT_TRUE(calculator_ui.encountered_error());
}

TEST_P(InterfacePtrTest, EncounteredErrorCallback) {
  math::CalculatorPtr proxy;
  MathCalculatorImpl calc_impl(MakeRequest(&proxy));

  bool encountered_error = false;
  base::RunLoop run_loop;
  proxy.set_connection_error_handler(
      base::Bind(&SetFlagAndRunClosure, &encountered_error,
                 run_loop.QuitClosure()));

  MathCalculatorUI calculator_ui(std::move(proxy));

  base::RunLoop run_loop2;
  calculator_ui.Add(2.0, run_loop2.QuitClosure());
  run_loop2.Run();
  EXPECT_EQ(2.0, calculator_ui.GetOutput());
  EXPECT_FALSE(calculator_ui.encountered_error());

  calculator_ui.Multiply(5.0, base::Closure());
  EXPECT_FALSE(calculator_ui.encountered_error());

  // Close the server.
  calc_impl.binding()->Close();

  // The state change isn't picked up locally yet.
  EXPECT_FALSE(calculator_ui.encountered_error());

  run_loop.Run();

  // OK, now we see the error.
  EXPECT_TRUE(calculator_ui.encountered_error());

  // We should have also been able to observe the error through the error
  // handler.
  EXPECT_TRUE(encountered_error);
}

TEST_P(InterfacePtrTest, DestroyInterfacePtrOnMethodResponse) {
  math::CalculatorPtr proxy;
  MathCalculatorImpl calc_impl(MakeRequest(&proxy));

  EXPECT_EQ(0, SelfDestructingMathCalculatorUI::num_instances());

  SelfDestructingMathCalculatorUI* impl =
      new SelfDestructingMathCalculatorUI(std::move(proxy));
  base::RunLoop run_loop;
  impl->BeginTest(false, run_loop.QuitClosure());
  run_loop.Run();

  EXPECT_EQ(0, SelfDestructingMathCalculatorUI::num_instances());
}

TEST_P(InterfacePtrTest, NestedDestroyInterfacePtrOnMethodResponse) {
  math::CalculatorPtr proxy;
  MathCalculatorImpl calc_impl(MakeRequest(&proxy));

  EXPECT_EQ(0, SelfDestructingMathCalculatorUI::num_instances());

  SelfDestructingMathCalculatorUI* impl =
      new SelfDestructingMathCalculatorUI(std::move(proxy));
  base::RunLoop run_loop;
  impl->BeginTest(true, run_loop.QuitClosure());
  run_loop.Run();

  EXPECT_EQ(0, SelfDestructingMathCalculatorUI::num_instances());
}

TEST_P(InterfacePtrTest, ReentrantWaitForIncomingMethodCall) {
  sample::ServicePtr proxy;
  ReentrantServiceImpl impl(MakeRequest(&proxy));

  base::RunLoop run_loop, run_loop2;
  proxy->Frobinate(nullptr, sample::Service::BazOptions::REGULAR, nullptr,
                   base::Bind(&IgnoreValueAndRunClosure,
                              run_loop.QuitClosure()));
  proxy->Frobinate(nullptr, sample::Service::BazOptions::REGULAR, nullptr,
                   base::Bind(&IgnoreValueAndRunClosure,
                              run_loop2.QuitClosure()));

  run_loop.Run();
  run_loop2.Run();

  EXPECT_EQ(2, impl.max_call_depth());
}

TEST_P(InterfacePtrTest, QueryVersion) {
  IntegerAccessorImpl impl;
  sample::IntegerAccessorPtr ptr;
  Binding<sample::IntegerAccessor> binding(&impl, MakeRequest(&ptr));

  EXPECT_EQ(0u, ptr.version());

  base::RunLoop run_loop;
  ptr.QueryVersion(base::Bind(&ExpectValueAndRunClosure, 3u,
                              run_loop.QuitClosure()));
  run_loop.Run();

  EXPECT_EQ(3u, ptr.version());
}

TEST_P(InterfacePtrTest, RequireVersion) {
  IntegerAccessorImpl impl;
  sample::IntegerAccessorPtr ptr;
  Binding<sample::IntegerAccessor> binding(&impl, MakeRequest(&ptr));

  EXPECT_EQ(0u, ptr.version());

  ptr.RequireVersion(1u);
  EXPECT_EQ(1u, ptr.version());
  base::RunLoop run_loop;
  impl.set_closure(run_loop.QuitClosure());
  ptr->SetInteger(123, sample::Enum::VALUE);
  run_loop.Run();
  EXPECT_FALSE(ptr.encountered_error());
  EXPECT_EQ(123, impl.integer());

  ptr.RequireVersion(3u);
  EXPECT_EQ(3u, ptr.version());
  base::RunLoop run_loop2;
  impl.set_closure(run_loop2.QuitClosure());
  ptr->SetInteger(456, sample::Enum::VALUE);
  run_loop2.Run();
  EXPECT_FALSE(ptr.encountered_error());
  EXPECT_EQ(456, impl.integer());

  // Require a version that is not supported by the impl side.
  ptr.RequireVersion(4u);
  // This value is set to the input of RequireVersion() synchronously.
  EXPECT_EQ(4u, ptr.version());
  base::RunLoop run_loop3;
  ptr.set_connection_error_handler(run_loop3.QuitClosure());
  ptr->SetInteger(789, sample::Enum::VALUE);
  run_loop3.Run();
  EXPECT_TRUE(ptr.encountered_error());
  // The call to SetInteger() after RequireVersion(4u) is ignored.
  EXPECT_EQ(456, impl.integer());
}

class StrongMathCalculatorImpl : public math::Calculator {
 public:
  StrongMathCalculatorImpl(bool* destroyed) : destroyed_(destroyed) {}
  ~StrongMathCalculatorImpl() override { *destroyed_ = true; }

  // math::Calculator implementation.
  void Clear(const CalcCallback& callback) override { callback.Run(total_); }

  void Add(double value, const CalcCallback& callback) override {
    total_ += value;
    callback.Run(total_);
  }

  void Multiply(double value, const CalcCallback& callback) override {
    total_ *= value;
    callback.Run(total_);
  }

 private:
  double total_ = 0.0;
  bool* destroyed_;
};

TEST(StrongConnectorTest, Math) {
  base::MessageLoop loop;

  bool error_received = false;
  bool destroyed = false;
  math::CalculatorPtr calc;
  base::RunLoop run_loop;

  auto binding =
      MakeStrongBinding(std::make_unique<StrongMathCalculatorImpl>(&destroyed),
                        MakeRequest(&calc));
  binding->set_connection_error_handler(base::Bind(
      &SetFlagAndRunClosure, &error_received, run_loop.QuitClosure()));

  {
    // Suppose this is instantiated in a process that has the other end of the
    // message pipe.
    MathCalculatorUI calculator_ui(std::move(calc));

    base::RunLoop run_loop, run_loop2;
    calculator_ui.Add(2.0, run_loop.QuitClosure());
    calculator_ui.Multiply(5.0, run_loop2.QuitClosure());
    run_loop.Run();
    run_loop2.Run();

    EXPECT_EQ(10.0, calculator_ui.GetOutput());
    EXPECT_FALSE(error_received);
    EXPECT_FALSE(destroyed);
  }
  // Destroying calculator_ui should close the pipe and generate an error on the
  // other
  // end which will destroy the instance since it is strongly bound.

  run_loop.Run();
  EXPECT_TRUE(error_received);
  EXPECT_TRUE(destroyed);
}

class WeakMathCalculatorImpl : public math::Calculator {
 public:
  WeakMathCalculatorImpl(math::CalculatorRequest request,
                         bool* error_received,
                         bool* destroyed,
                         const base::Closure& closure)
      : error_received_(error_received),
        destroyed_(destroyed),
        closure_(closure),
        binding_(this, std::move(request)) {
    binding_.set_connection_error_handler(
        base::Bind(&SetFlagAndRunClosure, error_received_, closure_));
  }
  ~WeakMathCalculatorImpl() override { *destroyed_ = true; }

  void Clear(const CalcCallback& callback) override { callback.Run(total_); }

  void Add(double value, const CalcCallback& callback) override {
    total_ += value;
    callback.Run(total_);
  }

  void Multiply(double value, const CalcCallback& callback) override {
    total_ *= value;
    callback.Run(total_);
  }

 private:
  double total_ = 0.0;
  bool* error_received_;
  bool* destroyed_;
  base::Closure closure_;

  Binding<math::Calculator> binding_;
};

TEST(WeakConnectorTest, Math) {
  base::MessageLoop loop;

  bool error_received = false;
  bool destroyed = false;
  MessagePipe pipe;
  base::RunLoop run_loop;
  WeakMathCalculatorImpl impl(math::CalculatorRequest(std::move(pipe.handle0)),
                              &error_received, &destroyed,
                              run_loop.QuitClosure());

  math::CalculatorPtr calc;
  calc.Bind(InterfacePtrInfo<math::Calculator>(std::move(pipe.handle1), 0u));

  {
    // Suppose this is instantiated in a process that has the other end of the
    // message pipe.
    MathCalculatorUI calculator_ui(std::move(calc));

    base::RunLoop run_loop, run_loop2;
    calculator_ui.Add(2.0, run_loop.QuitClosure());
    calculator_ui.Multiply(5.0, run_loop2.QuitClosure());
    run_loop.Run();
    run_loop2.Run();

    EXPECT_EQ(10.0, calculator_ui.GetOutput());
    EXPECT_FALSE(error_received);
    EXPECT_FALSE(destroyed);
    // Destroying calculator_ui should close the pipe and generate an error on
    // the other
    // end which will destroy the instance since it is strongly bound.
  }

  run_loop.Run();
  EXPECT_TRUE(error_received);
  EXPECT_FALSE(destroyed);
}

class CImpl : public C {
 public:
  CImpl(bool* d_called, const base::Closure& closure)
      : d_called_(d_called), closure_(closure) {}
  ~CImpl() override {}

 private:
  void D() override {
    *d_called_ = true;
    closure_.Run();
  }

  bool* d_called_;
  base::Closure closure_;
};

class BImpl : public B {
 public:
  BImpl(bool* d_called, const base::Closure& closure)
      : d_called_(d_called), closure_(closure) {}
  ~BImpl() override {}

 private:
  void GetC(InterfaceRequest<C> c) override {
    MakeStrongBinding(std::make_unique<CImpl>(d_called_, closure_),
                      std::move(c));
  }

  bool* d_called_;
  base::Closure closure_;
};

class AImpl : public A {
 public:
  AImpl(InterfaceRequest<A> request, const base::Closure& closure)
      : d_called_(false), binding_(this, std::move(request)),
        closure_(closure) {}
  ~AImpl() override {}

  bool d_called() const { return d_called_; }

 private:
  void GetB(InterfaceRequest<B> b) override {
    MakeStrongBinding(std::make_unique<BImpl>(&d_called_, closure_),
                      std::move(b));
  }

  bool d_called_;
  Binding<A> binding_;
  base::Closure closure_;
};

TEST_P(InterfacePtrTest, Scoping) {
  APtr a;
  base::RunLoop run_loop;
  AImpl a_impl(MakeRequest(&a), run_loop.QuitClosure());

  EXPECT_FALSE(a_impl.d_called());

  {
    BPtr b;
    a->GetB(MakeRequest(&b));
    CPtr c;
    b->GetC(MakeRequest(&c));
    c->D();
  }

  // While B & C have fallen out of scope, the pipes will remain until they are
  // flushed.
  EXPECT_FALSE(a_impl.d_called());
  run_loop.Run();
  EXPECT_TRUE(a_impl.d_called());
}

class PingTestImpl : public sample::PingTest {
 public:
  explicit PingTestImpl(InterfaceRequest<sample::PingTest> request)
      : binding_(this, std::move(request)) {}
  ~PingTestImpl() override {}

 private:
  // sample::PingTest:
  void Ping(const PingCallback& callback) override { callback.Run(); }

  Binding<sample::PingTest> binding_;
};

// Tests that FuseProxy does what it's supposed to do.
TEST_P(InterfacePtrTest, Fusion) {
  sample::PingTestPtrInfo proxy_info;
  PingTestImpl impl(MakeRequest(&proxy_info));

  // Create another PingTest pipe and fuse it to the one hanging off |impl|.
  sample::PingTestPtr ptr;
  EXPECT_TRUE(FuseInterface(mojo::MakeRequest(&ptr), std::move(proxy_info)));

  // Ping!
  bool called = false;
  base::RunLoop loop;
  ptr->Ping(base::Bind(&SetFlagAndRunClosure, &called, loop.QuitClosure()));
  loop.Run();
  EXPECT_TRUE(called);
}

void Fail() {
  FAIL() << "Unexpected connection error";
}

TEST_P(InterfacePtrTest, FlushForTesting) {
  math::CalculatorPtr calc;
  MathCalculatorImpl calc_impl(MakeRequest(&calc));
  calc.set_connection_error_handler(base::Bind(&Fail));

  MathCalculatorUI calculator_ui(std::move(calc));

  calculator_ui.Add(2.0, base::DoNothing());
  calculator_ui.GetInterfacePtr().FlushForTesting();
  EXPECT_EQ(2.0, calculator_ui.GetOutput());

  calculator_ui.Multiply(5.0, base::DoNothing());
  calculator_ui.GetInterfacePtr().FlushForTesting();

  EXPECT_EQ(10.0, calculator_ui.GetOutput());
}

void SetBool(bool* value) {
  *value = true;
}

TEST_P(InterfacePtrTest, FlushForTestingWithClosedPeer) {
  math::CalculatorPtr calc;
  MakeRequest(&calc);
  bool called = false;
  calc.set_connection_error_handler(base::Bind(&SetBool, &called));
  calc.FlushForTesting();
  EXPECT_TRUE(called);
  calc.FlushForTesting();
}

TEST_P(InterfacePtrTest, ConnectionErrorWithReason) {
  math::CalculatorPtr calc;
  MathCalculatorImpl calc_impl(MakeRequest(&calc));

  base::RunLoop run_loop;
  calc.set_connection_error_with_reason_handler(base::Bind(
      [](const base::Closure& quit_closure, uint32_t custom_reason,
         const std::string& description) {
        EXPECT_EQ(42u, custom_reason);
        EXPECT_EQ("hey", description);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  calc_impl.binding()->CloseWithReason(42u, "hey");

  run_loop.Run();
}

TEST_P(InterfacePtrTest, InterfaceRequestResetWithReason) {
  math::CalculatorPtr calc;
  auto request = MakeRequest(&calc);

  base::RunLoop run_loop;
  calc.set_connection_error_with_reason_handler(base::Bind(
      [](const base::Closure& quit_closure, uint32_t custom_reason,
         const std::string& description) {
        EXPECT_EQ(88u, custom_reason);
        EXPECT_EQ("greetings", description);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  request.ResetWithReason(88u, "greetings");

  run_loop.Run();
}

TEST_P(InterfacePtrTest, CallbackIsPassedInterfacePtr) {
  sample::PingTestPtr ptr;
  auto request = mojo::MakeRequest(&ptr);

  base::RunLoop run_loop;

  // Make a call with the proxy's lifetime bound to the response callback.
  sample::PingTest* raw_proxy = ptr.get();
  ptr.set_connection_error_handler(run_loop.QuitClosure());
  raw_proxy->Ping(base::Bind(base::DoNothing::Repeatedly<sample::PingTestPtr>(),
                             base::Passed(&ptr)));

  // Trigger an error on |ptr|. This will ultimately lead to the proxy's
  // response callbacks being destroyed, which will in turn lead to the proxy
  // being destroyed. This should not crash.
  request.PassMessagePipe();
  run_loop.Run();
}

TEST_P(InterfacePtrTest, ConnectionErrorHandlerOwnsInterfacePtr) {
  sample::PingTestPtr* ptr = new sample::PingTestPtr;
  auto request = mojo::MakeRequest(ptr);

  base::RunLoop run_loop;

  // Make a call with |ptr|'s lifetime bound to the connection error handler
  // callback.
  ptr->set_connection_error_handler(base::Bind(
      [](const base::Closure& quit, sample::PingTestPtr* ptr) {
        ptr->reset();
        quit.Run();
      },
      run_loop.QuitClosure(), base::Owned(ptr)));

  // Trigger an error on |ptr|. In the error handler |ptr| is reset. This
  // shouldn't immediately destroy the callback (and |ptr| that it owns), before
  // the callback is completed.
  request.PassMessagePipe();
  run_loop.Run();
}

TEST_P(InterfacePtrTest, ThreadSafeInterfacePointer) {
  math::CalculatorPtr ptr;
  MathCalculatorImpl calc_impl(MakeRequest(&ptr));
  scoped_refptr<math::ThreadSafeCalculatorPtr> thread_safe_ptr =
      math::ThreadSafeCalculatorPtr::Create(std::move(ptr));

  base::RunLoop run_loop;

  auto run_method = base::Bind(
      [](const scoped_refptr<base::TaskRunner>& main_task_runner,
         const base::Closure& quit_closure,
         const scoped_refptr<math::ThreadSafeCalculatorPtr>& thread_safe_ptr) {
        auto calc_callback = base::Bind(
            [](const scoped_refptr<base::TaskRunner>& main_task_runner,
               const base::Closure& quit_closure,
               scoped_refptr<base::SequencedTaskRunner> sender_sequence_runner,
               double result) {
              EXPECT_EQ(123, result);
              // Validate the callback is invoked on the calling sequence.
              EXPECT_TRUE(sender_sequence_runner->RunsTasksInCurrentSequence());
              // Notify the run_loop to quit.
              main_task_runner->PostTask(FROM_HERE, quit_closure);
            });
        scoped_refptr<base::SequencedTaskRunner> current_sequence_runner =
            base::SequencedTaskRunnerHandle::Get();
        (*thread_safe_ptr)
            ->Add(123, base::Bind(calc_callback, main_task_runner, quit_closure,
                                  current_sequence_runner));
      },
      base::SequencedTaskRunnerHandle::Get(), run_loop.QuitClosure(),
      thread_safe_ptr);
  base::CreateSequencedTaskRunnerWithTraits({})->PostTask(FROM_HERE,
                                                          run_method);

  // Block until the method callback is called on the background thread.
  run_loop.Run();
}

TEST_P(InterfacePtrTest, ThreadSafeInterfacePointerWithTaskRunner) {
  const scoped_refptr<base::SequencedTaskRunner> other_thread_task_runner =
      base::CreateSequencedTaskRunnerWithTraits({});

  math::CalculatorPtr ptr;
  auto request = mojo::MakeRequest(&ptr);

  // Create a ThreadSafeInterfacePtr that we'll bind from a different thread.
  scoped_refptr<math::ThreadSafeCalculatorPtr> thread_safe_ptr =
      math::ThreadSafeCalculatorPtr::Create(ptr.PassInterface(),
                                            other_thread_task_runner);
  ASSERT_TRUE(thread_safe_ptr);

  MathCalculatorImpl* math_calc_impl = nullptr;
  {
    base::RunLoop run_loop;
    auto run_method = base::Bind(
        [](const scoped_refptr<base::TaskRunner>& main_task_runner,
           const base::Closure& quit_closure,
           const scoped_refptr<math::ThreadSafeCalculatorPtr>& thread_safe_ptr,
           math::CalculatorRequest request,
           MathCalculatorImpl** math_calc_impl) {
          math::CalculatorPtr ptr;
          // In real life, the implementation would have a legitimate owner.
          *math_calc_impl = new MathCalculatorImpl(std::move(request));
          main_task_runner->PostTask(FROM_HERE, quit_closure);
        },
        base::SequencedTaskRunnerHandle::Get(), run_loop.QuitClosure(),
        thread_safe_ptr, base::Passed(&request), &math_calc_impl);
    other_thread_task_runner->PostTask(FROM_HERE, run_method);
    run_loop.Run();
  }

  {
    // The interface ptr is bound, we can call methods on it.
    auto calc_callback =
        base::Bind([](const base::Closure& quit_closure, double result) {
          EXPECT_EQ(123, result);
          quit_closure.Run();
        });
    base::RunLoop run_loop;
    (*thread_safe_ptr)
        ->Add(123, base::Bind(calc_callback, run_loop.QuitClosure()));
    // Block until the method callback is called.
    run_loop.Run();
  }

  other_thread_task_runner->DeleteSoon(FROM_HERE, math_calc_impl);

  // Reset the pointer now so the InterfacePtr associated resources can be
  // deleted before the background thread's message loop is invalidated.
  thread_safe_ptr = nullptr;
}

INSTANTIATE_MOJO_BINDINGS_TEST_CASE_P(InterfacePtrTest);

}  // namespace
}  // namespace test
}  // namespace mojo
