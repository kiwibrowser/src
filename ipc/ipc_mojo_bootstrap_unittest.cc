// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ipc/ipc_mojo_bootstrap.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ipc/ipc.mojom.h"
#include "ipc/ipc_test_base.h"
#include "mojo/edk/test/multiprocess_test_helper.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace {

constexpr int32_t kTestServerPid = 42;
constexpr int32_t kTestClientPid = 4242;

class Connection {
 public:
  explicit Connection(std::unique_ptr<IPC::MojoBootstrap> bootstrap,
                      int32_t sender_id)
      : bootstrap_(std::move(bootstrap)) {
    bootstrap_->Connect(&sender_, &receiver_);
    sender_->SetPeerPid(sender_id);
  }

  void TakeReceiver(IPC::mojom::ChannelAssociatedRequest* receiver) {
    *receiver = std::move(receiver_);
  }

  IPC::mojom::ChannelAssociatedPtr& GetSender() { return sender_; }

 private:
  IPC::mojom::ChannelAssociatedPtr sender_;
  IPC::mojom::ChannelAssociatedRequest receiver_;
  std::unique_ptr<IPC::MojoBootstrap> bootstrap_;
};

class PeerPidReceiver : public IPC::mojom::Channel {
 public:
  enum class MessageExpectation {
    kNotExpected,
    kExpectedValid,
    kExpectedInvalid
  };

  PeerPidReceiver(
      IPC::mojom::ChannelAssociatedRequest request,
      const base::Closure& on_peer_pid_set,
      MessageExpectation message_expectation = MessageExpectation::kNotExpected)
      : binding_(this, std::move(request)),
        on_peer_pid_set_(on_peer_pid_set),
        message_expectation_(message_expectation) {
    binding_.set_connection_error_handler(disconnect_run_loop_.QuitClosure());
  }

  ~PeerPidReceiver() override {
    bool expected_message =
        message_expectation_ != MessageExpectation::kNotExpected;
    EXPECT_EQ(expected_message, received_message_);
  }

  // mojom::Channel:
  void SetPeerPid(int32_t pid) override {
    peer_pid_ = pid;
    on_peer_pid_set_.Run();
  }

  void Receive(base::span<const uint8_t> data,
               base::Optional<std::vector<mojo::native::SerializedHandlePtr>>
                   handles) override {
    ASSERT_NE(MessageExpectation::kNotExpected, message_expectation_);
    received_message_ = true;

    IPC::Message message(reinterpret_cast<const char*>(data.data()),
                         static_cast<uint32_t>(data.size()));
    bool expected_valid =
        message_expectation_ == MessageExpectation::kExpectedValid;
    EXPECT_EQ(expected_valid, message.IsValid());
  }

  void GetAssociatedInterface(
      const std::string& name,
      IPC::mojom::GenericInterfaceAssociatedRequest request) override {}

  int32_t peer_pid() const { return peer_pid_; }

  void RunUntilDisconnect() { disconnect_run_loop_.Run(); }

 private:
  mojo::AssociatedBinding<IPC::mojom::Channel> binding_;
  const base::Closure on_peer_pid_set_;
  MessageExpectation message_expectation_;
  int32_t peer_pid_ = -1;
  bool received_message_ = false;
  base::RunLoop disconnect_run_loop_;

  DISALLOW_COPY_AND_ASSIGN(PeerPidReceiver);
};

class IPCMojoBootstrapTest : public testing::Test {
 protected:
  mojo::edk::test::MultiprocessTestHelper helper_;
};

TEST_F(IPCMojoBootstrapTest, Connect) {
  base::MessageLoop message_loop;
  Connection connection(
      IPC::MojoBootstrap::Create(
          helper_.StartChild("IPCMojoBootstrapTestClient"),
          IPC::Channel::MODE_SERVER, base::ThreadTaskRunnerHandle::Get(),
          base::ThreadTaskRunnerHandle::Get()),
      kTestServerPid);

  IPC::mojom::ChannelAssociatedRequest receiver;
  connection.TakeReceiver(&receiver);

  base::RunLoop run_loop;
  PeerPidReceiver impl(std::move(receiver), run_loop.QuitClosure());
  run_loop.Run();

  EXPECT_EQ(kTestClientPid, impl.peer_pid());

  impl.RunUntilDisconnect();
  EXPECT_TRUE(helper_.WaitForChildTestShutdown());
}

// A long running process that connects to us.
MULTIPROCESS_TEST_MAIN_WITH_SETUP(
    IPCMojoBootstrapTestClientTestChildMain,
    ::mojo::edk::test::MultiprocessTestHelper::ChildSetup) {
  base::MessageLoop message_loop;
  Connection connection(
      IPC::MojoBootstrap::Create(
          std::move(mojo::edk::test::MultiprocessTestHelper::primordial_pipe),
          IPC::Channel::MODE_CLIENT, base::ThreadTaskRunnerHandle::Get(),
          base::ThreadTaskRunnerHandle::Get()),
      kTestClientPid);

  IPC::mojom::ChannelAssociatedRequest receiver;
  connection.TakeReceiver(&receiver);

  base::RunLoop run_loop;
  PeerPidReceiver impl(std::move(receiver), run_loop.QuitClosure());
  run_loop.Run();

  EXPECT_EQ(kTestServerPid, impl.peer_pid());

  return 0;
}

TEST_F(IPCMojoBootstrapTest, ReceiveEmptyMessage) {
  base::MessageLoop message_loop;
  Connection connection(
      IPC::MojoBootstrap::Create(
          helper_.StartChild("IPCMojoBootstrapTestEmptyMessage"),
          IPC::Channel::MODE_SERVER, base::ThreadTaskRunnerHandle::Get(),
          base::ThreadTaskRunnerHandle::Get()),
      kTestServerPid);

  IPC::mojom::ChannelAssociatedRequest receiver;
  connection.TakeReceiver(&receiver);

  base::RunLoop run_loop;
  PeerPidReceiver impl(std::move(receiver), run_loop.QuitClosure(),
                       PeerPidReceiver::MessageExpectation::kExpectedInvalid);
  run_loop.Run();

  // Wait for the Channel to be disconnected so we can reasonably assert that
  // the child's empty message must have been received before we pass the test.
  impl.RunUntilDisconnect();

  EXPECT_TRUE(helper_.WaitForChildTestShutdown());
}

// A long running process that connects to us.
MULTIPROCESS_TEST_MAIN_WITH_SETUP(
    IPCMojoBootstrapTestEmptyMessageTestChildMain,
    ::mojo::edk::test::MultiprocessTestHelper::ChildSetup) {
  base::MessageLoop message_loop;
  Connection connection(
      IPC::MojoBootstrap::Create(
          std::move(mojo::edk::test::MultiprocessTestHelper::primordial_pipe),
          IPC::Channel::MODE_CLIENT, base::ThreadTaskRunnerHandle::Get(),
          base::ThreadTaskRunnerHandle::Get()),
      kTestClientPid);

  IPC::mojom::ChannelAssociatedRequest receiver;
  connection.TakeReceiver(&receiver);
  auto& sender = connection.GetSender();

  uint8_t data = 0;
  sender->Receive(base::make_span(&data, 0), {});

  base::RunLoop run_loop;
  PeerPidReceiver impl(std::move(receiver), run_loop.QuitClosure());
  run_loop.Run();

  return 0;
}

}  // namespace
