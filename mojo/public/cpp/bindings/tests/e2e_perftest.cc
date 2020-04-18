// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/perf_time_logger.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/test/mojo_test_base.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/interfaces/bindings/tests/ping_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

class EchoServiceImpl : public test::EchoService {
 public:
  explicit EchoServiceImpl(const base::Closure& quit_closure);
  ~EchoServiceImpl() override;

  // |EchoService| methods:
  void Echo(const std::string& test_data,
            const EchoCallback& callback) override;

 private:
  const base::Closure quit_closure_;
};

EchoServiceImpl::EchoServiceImpl(const base::Closure& quit_closure)
    : quit_closure_(quit_closure) {}

EchoServiceImpl::~EchoServiceImpl() {
  quit_closure_.Run();
}

void EchoServiceImpl::Echo(const std::string& test_data,
                           const EchoCallback& callback) {
  callback.Run(test_data);
}

class PingPongTest {
 public:
  explicit PingPongTest(test::EchoServicePtr service);

  void RunTest(int iterations, int batch_size, int message_size);

 private:
  void DoPing();
  void OnPingDone(const std::string& reply);

  test::EchoServicePtr service_;
  const base::Callback<void(const std::string&)> ping_done_callback_;

  int iterations_;
  int batch_size_;
  std::string message_;

  int current_iterations_;
  int calls_outstanding_;

  base::Closure quit_closure_;
};

PingPongTest::PingPongTest(test::EchoServicePtr service)
    : service_(std::move(service)),
      ping_done_callback_(
          base::Bind(&PingPongTest::OnPingDone, base::Unretained(this))) {}

void PingPongTest::RunTest(int iterations, int batch_size, int message_size) {
  iterations_ = iterations;
  batch_size_ = batch_size;
  message_ = std::string(message_size, 'a');
  current_iterations_ = 0;
  calls_outstanding_ = 0;

  base::MessageLoopCurrent::Get()->SetNestableTasksAllowed(true);
  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&PingPongTest::DoPing, base::Unretained(this)));
  run_loop.Run();
}

void PingPongTest::DoPing() {
  DCHECK_EQ(0, calls_outstanding_);
  current_iterations_++;
  if (current_iterations_ > iterations_) {
    quit_closure_.Run();
    return;
  }

  calls_outstanding_ = batch_size_;
  for (int i = 0; i < batch_size_; i++) {
    service_->Echo(message_, ping_done_callback_);
  }
}

void PingPongTest::OnPingDone(const std::string& reply) {
  DCHECK_GT(calls_outstanding_, 0);
  calls_outstanding_--;

  if (!calls_outstanding_)
    DoPing();
}

class MojoE2EPerftest : public edk::test::MojoTestBase {
 public:
  void RunTestOnTaskRunner(base::TaskRunner* runner,
                           MojoHandle client_mp,
                           const std::string& test_name) {
    if (runner == base::ThreadTaskRunnerHandle::Get().get()) {
      RunTests(client_mp, test_name);
    } else {
      base::RunLoop run_loop;
      runner->PostTaskAndReply(
          FROM_HERE,
          base::Bind(&MojoE2EPerftest::RunTests, base::Unretained(this),
                     client_mp, test_name),
          run_loop.QuitClosure());
      run_loop.Run();
    }
  }

 protected:
  base::MessageLoop message_loop_;

 private:
  void RunTests(MojoHandle client_mp, const std::string& test_name) {
    const int kMessages = 10000;
    const int kBatchSizes[] = {1, 10, 100};
    const int kMessageSizes[] = {8, 64, 512, 4096, 65536};

    test::EchoServicePtr service;
    service.Bind(InterfacePtrInfo<test::EchoService>(
        ScopedMessagePipeHandle(MessagePipeHandle(client_mp)),
        service.version()));
    PingPongTest test(std::move(service));

    for (int batch_size : kBatchSizes) {
      for (int message_size : kMessageSizes) {
        int num_messages = kMessages;
        if (message_size == 65536)
          num_messages /= 10;
        std::string sub_test_name = base::StringPrintf(
            "%s/%dx%d/%dbytes", test_name.c_str(), num_messages / batch_size,
            batch_size, message_size);
        base::PerfTimeLogger timer(sub_test_name.c_str());
        test.RunTest(num_messages / batch_size, batch_size, message_size);
      }
    }
  }
};

void CreateAndRunService(InterfaceRequest<test::EchoService> request,
                         const base::Closure& cb) {
  MakeStrongBinding(std::make_unique<EchoServiceImpl>(cb), std::move(request));
}

DEFINE_TEST_CLIENT_TEST_WITH_PIPE(PingService, MojoE2EPerftest, mp) {
  MojoHandle service_mp;
  EXPECT_EQ("hello", ReadMessageWithHandles(mp, &service_mp, 1));

  auto request = InterfaceRequest<test::EchoService>(
      ScopedMessagePipeHandle(MessagePipeHandle(service_mp)));
  base::RunLoop run_loop;
  edk::GetIOTaskRunner()->PostTask(
      FROM_HERE,
      base::Bind(&CreateAndRunService, base::Passed(&request),
                 base::Bind(base::IgnoreResult(&base::TaskRunner::PostTask),
                            message_loop_.task_runner(), FROM_HERE,
                            run_loop.QuitClosure())));
  run_loop.Run();
}

TEST_F(MojoE2EPerftest, MultiProcessEchoMainThread) {
  RunTestClient("PingService", [&](MojoHandle mp) {
    MojoHandle client_mp, service_mp;
    CreateMessagePipe(&client_mp, &service_mp);
    WriteMessageWithHandles(mp, "hello", &service_mp, 1);
    RunTestOnTaskRunner(message_loop_.task_runner().get(), client_mp,
                        "MultiProcessEchoMainThread");
  });
}

TEST_F(MojoE2EPerftest, MultiProcessEchoIoThread) {
  RunTestClient("PingService", [&](MojoHandle mp) {
    MojoHandle client_mp, service_mp;
    CreateMessagePipe(&client_mp, &service_mp);
    WriteMessageWithHandles(mp, "hello", &service_mp, 1);
    RunTestOnTaskRunner(edk::GetIOTaskRunner().get(), client_mp,
                        "MultiProcessEchoIoThread");
  });
}

}  // namespace
}  // namespace mojo
