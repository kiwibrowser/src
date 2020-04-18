// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/wait.h"
#include "mojo/public/interfaces/bindings/tests/ping_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

class TestHelperTest : public testing::Test {
 public:
  TestHelperTest() = default;
  ~TestHelperTest() override = default;

 private:
  base::test::ScopedTaskEnvironment task_environment_;

  DISALLOW_COPY_AND_ASSIGN(TestHelperTest);
};

class PingImpl : public test::PingService {
 public:
  explicit PingImpl(test::PingServiceRequest request)
      : binding_(this, std::move(request)) {}
  ~PingImpl() override = default;

  bool pinged() const { return pinged_; }

  // test::PingService:
  void Ping(const PingCallback& callback) override {
    pinged_ = true;
    callback.Run();
  }

 private:
  bool pinged_ = false;
  Binding<test::PingService> binding_;

  DISALLOW_COPY_AND_ASSIGN(PingImpl);
};

class EchoImpl : public test::EchoService {
 public:
  explicit EchoImpl(test::EchoServiceRequest request)
      : binding_(this, std::move(request)) {}
  ~EchoImpl() override = default;

  // test::EchoService:
  void Echo(const std::string& message, const EchoCallback& callback) override {
    callback.Run(message);
  }

 private:
  Binding<test::EchoService> binding_;

  DISALLOW_COPY_AND_ASSIGN(EchoImpl);
};

class TrampolineImpl : public test::HandleTrampoline {
 public:
  explicit TrampolineImpl(test::HandleTrampolineRequest request)
      : binding_(this, std::move(request)) {}
  ~TrampolineImpl() override = default;

  // test::HandleTrampoline:
  void BounceOne(ScopedMessagePipeHandle one,
                 const BounceOneCallback& callback) override {
    callback.Run(std::move(one));
  }

  void BounceTwo(ScopedMessagePipeHandle one,
                 ScopedMessagePipeHandle two,
                 const BounceTwoCallback& callback) override {
    callback.Run(std::move(one), std::move(two));
  }

 private:
  Binding<test::HandleTrampoline> binding_;

  DISALLOW_COPY_AND_ASSIGN(TrampolineImpl);
};

TEST_F(TestHelperTest, AsyncWaiter) {
  test::PingServicePtr ping;
  PingImpl ping_impl(MakeRequest(&ping));

  test::PingServiceAsyncWaiter wait_for_ping(ping.get());
  EXPECT_FALSE(ping_impl.pinged());
  wait_for_ping.Ping();
  EXPECT_TRUE(ping_impl.pinged());

  test::EchoServicePtr echo;
  EchoImpl echo_impl(MakeRequest(&echo));

  test::EchoServiceAsyncWaiter wait_for_echo(echo.get());
  const std::string kTestString = "a machine that goes 'ping'";
  std::string response;
  wait_for_echo.Echo(kTestString, &response);
  EXPECT_EQ(kTestString, response);

  test::HandleTrampolinePtr trampoline;
  TrampolineImpl trampoline_impl(MakeRequest(&trampoline));

  test::HandleTrampolineAsyncWaiter wait_for_trampoline(trampoline.get());
  MessagePipe pipe;
  ScopedMessagePipeHandle handle0, handle1;
  WriteMessageRaw(pipe.handle0.get(), kTestString.data(), kTestString.size(),
                  nullptr, 0, MOJO_WRITE_MESSAGE_FLAG_NONE);
  wait_for_trampoline.BounceOne(std::move(pipe.handle0), &handle0);
  wait_for_trampoline.BounceTwo(std::move(handle0), std::move(pipe.handle1),
                                &handle0, &handle1);

  // Verify that our pipe handles are the same as the original pipe.
  Wait(handle1.get(), MOJO_HANDLE_SIGNAL_READABLE);
  std::vector<uint8_t> payload;
  ReadMessageRaw(handle1.get(), &payload, nullptr, MOJO_READ_MESSAGE_FLAG_NONE);
  std::string original_message(payload.begin(), payload.end());
  EXPECT_EQ(kTestString, original_message);
}

}  // namespace
}  // namespace mojo
