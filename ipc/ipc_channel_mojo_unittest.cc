// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ipc/ipc_channel_mojo.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/containers/queue.h"
#include "base/files/file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/platform_shared_memory_region.h"
#include "base/memory/shared_memory.h"
#include "base/memory/shared_memory_mapping.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/pickle.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/test_io_thread.h"
#include "base/test/test_shared_memory_util.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_utils.h"
#include "ipc/ipc_mojo_handle_attachment.h"
#include "ipc/ipc_mojo_message_helper.h"
#include "ipc/ipc_mojo_param_traits.h"
#include "ipc/ipc_sync_channel.h"
#include "ipc/ipc_sync_message.h"
#include "ipc/ipc_test.mojom.h"
#include "ipc/ipc_test_base.h"
#include "ipc/ipc_test_channel_listener.h"
#include "mojo/public/cpp/system/wait.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
#include "base/file_descriptor_posix.h"
#include "ipc/ipc_platform_file_attachment_posix.h"
#endif

namespace {

void SendString(IPC::Sender* sender, const std::string& str) {
  IPC::Message* message = new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
  message->WriteString(str);
  ASSERT_TRUE(sender->Send(message));
}

void SendValue(IPC::Sender* sender, int32_t value) {
  IPC::Message* message = new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
  message->WriteInt(value);
  ASSERT_TRUE(sender->Send(message));
}

class ListenerThatExpectsOK : public IPC::Listener {
 public:
  ListenerThatExpectsOK(base::Closure quit_closure)
      : received_ok_(false), quit_closure_(quit_closure) {}

  ~ListenerThatExpectsOK() override = default;

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    std::string should_be_ok;
    EXPECT_TRUE(iter.ReadString(&should_be_ok));
    EXPECT_EQ(should_be_ok, "OK");
    received_ok_ = true;
    quit_closure_.Run();
    return true;
  }

  void OnChannelError() override {
    // The connection should be healthy while the listener is waiting
    // message.  An error can occur after that because the peer
    // process dies.
    CHECK(received_ok_);
  }

  static void SendOK(IPC::Sender* sender) { SendString(sender, "OK"); }

 private:
  bool received_ok_;
  base::Closure quit_closure_;
};

class TestListenerBase : public IPC::Listener {
 public:
  TestListenerBase(base::Closure quit_closure) : quit_closure_(quit_closure) {}

  ~TestListenerBase() override = default;

  void OnChannelError() override { quit_closure_.Run(); }

  void set_sender(IPC::Sender* sender) { sender_ = sender; }
  IPC::Sender* sender() const { return sender_; }
  base::Closure quit_closure() const { return quit_closure_; }

 private:
  IPC::Sender* sender_ = nullptr;
  base::Closure quit_closure_;
};

using IPCChannelMojoTest = IPCChannelMojoTestBase;

class TestChannelListenerWithExtraExpectations
    : public IPC::TestChannelListener {
 public:
  TestChannelListenerWithExtraExpectations() : is_connected_called_(false) {}

  void OnChannelConnected(int32_t peer_pid) override {
    IPC::TestChannelListener::OnChannelConnected(peer_pid);
    EXPECT_TRUE(base::kNullProcessId != peer_pid);
    is_connected_called_ = true;
  }

  bool is_connected_called() const { return is_connected_called_; }

 private:
  bool is_connected_called_;
};

TEST_F(IPCChannelMojoTest, ConnectedFromClient) {
  Init("IPCChannelMojoTestClient");

  // Set up IPC channel and start client.
  TestChannelListenerWithExtraExpectations listener;
  CreateChannel(&listener);
  listener.Init(sender());
  ASSERT_TRUE(ConnectChannel());

  IPC::TestChannelListener::SendOneMessage(sender(), "hello from parent");

  base::RunLoop().Run();

  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  EXPECT_TRUE(listener.is_connected_called());
  EXPECT_TRUE(listener.HasSentAll());

  DestroyChannel();
}

// A long running process that connects to us
DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestClient) {
  TestChannelListenerWithExtraExpectations listener;
  Connect(&listener);
  listener.Init(channel());

  IPC::TestChannelListener::SendOneMessage(channel(), "hello from child");
  base::RunLoop().Run();
  EXPECT_TRUE(listener.is_connected_called());
  EXPECT_TRUE(listener.HasSentAll());

  Close();
}

class ListenerExpectingErrors : public TestListenerBase {
 public:
  ListenerExpectingErrors(base::Closure quit_closure)
      : TestListenerBase(quit_closure), has_error_(false) {}

  bool OnMessageReceived(const IPC::Message& message) override { return true; }

  void OnChannelError() override {
    has_error_ = true;
    TestListenerBase::OnChannelError();
  }

  bool has_error() const { return has_error_; }

 private:
  bool has_error_;
};

class ListenerThatQuits : public IPC::Listener {
 public:
  ListenerThatQuits(base::Closure quit_closure) : quit_closure_(quit_closure) {}

  bool OnMessageReceived(const IPC::Message& message) override { return true; }

  void OnChannelConnected(int32_t peer_pid) override { quit_closure_.Run(); }

 private:
  base::Closure quit_closure_;
};

// A long running process that connects to us.
DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoErraticTestClient) {
  base::RunLoop run_loop;
  ListenerThatQuits listener(run_loop.QuitClosure());
  Connect(&listener);

  run_loop.Run();

  Close();
}

TEST_F(IPCChannelMojoTest, SendFailWithPendingMessages) {
  Init("IPCChannelMojoErraticTestClient");

  // Set up IPC channel and start client.
  base::RunLoop run_loop;
  ListenerExpectingErrors listener(run_loop.QuitClosure());
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  // This matches a value in mojo/edk/system/constants.h
  const int kMaxMessageNumBytes = 4 * 1024 * 1024;
  std::string overly_large_data(kMaxMessageNumBytes, '*');
  // This messages are queued as pending.
  for (size_t i = 0; i < 10; ++i) {
    IPC::TestChannelListener::SendOneMessage(sender(),
                                             overly_large_data.c_str());
  }

  run_loop.Run();

  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  EXPECT_TRUE(listener.has_error());

  DestroyChannel();
}

struct TestingMessagePipe {
  TestingMessagePipe() {
    EXPECT_EQ(MOJO_RESULT_OK, mojo::CreateMessagePipe(nullptr, &self, &peer));
  }

  mojo::ScopedMessagePipeHandle self;
  mojo::ScopedMessagePipeHandle peer;
};

class HandleSendingHelper {
 public:
  static std::string GetSendingFileContent() { return "Hello"; }

  static void WritePipe(IPC::Message* message, TestingMessagePipe* pipe) {
    std::string content = HandleSendingHelper::GetSendingFileContent();
    EXPECT_EQ(MOJO_RESULT_OK,
              mojo::WriteMessageRaw(pipe->self.get(), &content[0],
                                    static_cast<uint32_t>(content.size()),
                                    nullptr, 0, 0));
    EXPECT_TRUE(IPC::MojoMessageHelper::WriteMessagePipeTo(
        message, std::move(pipe->peer)));
  }

  static void WritePipeThenSend(IPC::Sender* sender, TestingMessagePipe* pipe) {
    IPC::Message* message =
        new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
    WritePipe(message, pipe);
    ASSERT_TRUE(sender->Send(message));
  }

  static void ReadReceivedPipe(const IPC::Message& message,
                               base::PickleIterator* iter) {
    mojo::ScopedMessagePipeHandle pipe;
    EXPECT_TRUE(
        IPC::MojoMessageHelper::ReadMessagePipeFrom(&message, iter, &pipe));
    std::vector<uint8_t> content;

    ASSERT_EQ(MOJO_RESULT_OK,
              mojo::Wait(pipe.get(), MOJO_HANDLE_SIGNAL_READABLE));
    EXPECT_EQ(MOJO_RESULT_OK,
              mojo::ReadMessageRaw(pipe.get(), &content, nullptr, 0));
    EXPECT_EQ(std::string(content.begin(), content.end()),
              GetSendingFileContent());
  }

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
  static base::FilePath GetSendingFilePath(const base::FilePath& dir_path) {
    return dir_path.Append("ListenerThatExpectsFile.txt");
  }

  static void WriteFile(IPC::Message* message, base::File& file) {
    std::string content = GetSendingFileContent();
    file.WriteAtCurrentPos(content.data(), content.size());
    file.Flush();
    message->WriteAttachment(new IPC::internal::PlatformFileAttachment(
        base::ScopedFD(file.TakePlatformFile())));
  }

  static void WriteFileThenSend(IPC::Sender* sender, base::File& file) {
    IPC::Message* message =
        new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
    WriteFile(message, file);
    ASSERT_TRUE(sender->Send(message));
  }

  static void WriteFileAndPipeThenSend(IPC::Sender* sender,
                                       base::File& file,
                                       TestingMessagePipe* pipe) {
    IPC::Message* message =
        new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
    WriteFile(message, file);
    WritePipe(message, pipe);
    ASSERT_TRUE(sender->Send(message));
  }

  static void ReadReceivedFile(const IPC::Message& message,
                               base::PickleIterator* iter) {
    scoped_refptr<base::Pickle::Attachment> attachment;
    EXPECT_TRUE(message.ReadAttachment(iter, &attachment));
    EXPECT_EQ(
        IPC::MessageAttachment::Type::PLATFORM_FILE,
        static_cast<IPC::MessageAttachment*>(attachment.get())->GetType());
    base::File file(
        static_cast<IPC::internal::PlatformFileAttachment*>(attachment.get())
            ->TakePlatformFile());
    std::string content(GetSendingFileContent().size(), ' ');
    file.Read(0, &content[0], content.size());
    EXPECT_EQ(content, GetSendingFileContent());
  }
#endif
};

class ListenerThatExpectsMessagePipe : public TestListenerBase {
 public:
  ListenerThatExpectsMessagePipe(base::Closure quit_closure)
      : TestListenerBase(quit_closure) {}

  ~ListenerThatExpectsMessagePipe() override = default;

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    HandleSendingHelper::ReadReceivedPipe(message, &iter);
    ListenerThatExpectsOK::SendOK(sender());
    return true;
  }
};

TEST_F(IPCChannelMojoTest, SendMessagePipe) {
  Init("IPCChannelMojoTestSendMessagePipeClient");

  base::RunLoop run_loop;
  ListenerThatExpectsOK listener(run_loop.QuitClosure());
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  TestingMessagePipe pipe;
  HandleSendingHelper::WritePipeThenSend(channel(), &pipe);

  run_loop.Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestSendMessagePipeClient) {
  base::RunLoop run_loop;
  ListenerThatExpectsMessagePipe listener(run_loop.QuitClosure());
  Connect(&listener);
  listener.set_sender(channel());

  run_loop.Run();

  Close();
}

void ReadOK(mojo::MessagePipeHandle pipe) {
  std::vector<uint8_t> should_be_ok;
  CHECK_EQ(MOJO_RESULT_OK, mojo::Wait(pipe, MOJO_HANDLE_SIGNAL_READABLE));
  CHECK_EQ(MOJO_RESULT_OK,
           mojo::ReadMessageRaw(pipe, &should_be_ok, nullptr, 0));
  EXPECT_EQ("OK", std::string(should_be_ok.begin(), should_be_ok.end()));
}

void WriteOK(mojo::MessagePipeHandle pipe) {
  std::string ok("OK");
  CHECK_EQ(MOJO_RESULT_OK,
           mojo::WriteMessageRaw(pipe, &ok[0], static_cast<uint32_t>(ok.size()),
                                 nullptr, 0, 0));
}

class ListenerThatExpectsMessagePipeUsingParamTrait : public TestListenerBase {
 public:
  explicit ListenerThatExpectsMessagePipeUsingParamTrait(
      base::Closure quit_closure,
      bool receiving_valid)
      : TestListenerBase(quit_closure), receiving_valid_(receiving_valid) {}

  ~ListenerThatExpectsMessagePipeUsingParamTrait() override = default;

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    mojo::MessagePipeHandle handle;
    EXPECT_TRUE(IPC::ParamTraits<mojo::MessagePipeHandle>::Read(&message, &iter,
                                                                &handle));
    EXPECT_EQ(handle.is_valid(), receiving_valid_);
    if (receiving_valid_) {
      ReadOK(handle);
      MojoClose(handle.value());
    }

    ListenerThatExpectsOK::SendOK(sender());
    return true;
  }

 private:
  bool receiving_valid_;
};

class ParamTraitMessagePipeClient : public IpcChannelMojoTestClient {
 public:
  void RunTest(bool receiving_valid_handle) {
    base::RunLoop run_loop;
    ListenerThatExpectsMessagePipeUsingParamTrait listener(
        run_loop.QuitClosure(), receiving_valid_handle);
    Connect(&listener);
    listener.set_sender(channel());

    run_loop.Run();

    Close();
  }
};

TEST_F(IPCChannelMojoTest, ParamTraitValidMessagePipe) {
  Init("ParamTraitValidMessagePipeClient");

  base::RunLoop run_loop;
  ListenerThatExpectsOK listener(run_loop.QuitClosure());
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  TestingMessagePipe pipe;

  std::unique_ptr<IPC::Message> message(new IPC::Message());
  IPC::ParamTraits<mojo::MessagePipeHandle>::Write(message.get(),
                                                   pipe.peer.release());
  WriteOK(pipe.self.get());

  channel()->Send(message.release());
  run_loop.Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT_WITH_CUSTOM_FIXTURE(
    ParamTraitValidMessagePipeClient,
    ParamTraitMessagePipeClient) {
  RunTest(true);
}

TEST_F(IPCChannelMojoTest, ParamTraitInvalidMessagePipe) {
  Init("ParamTraitInvalidMessagePipeClient");

  base::RunLoop run_loop;
  ListenerThatExpectsOK listener(run_loop.QuitClosure());
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  mojo::MessagePipeHandle invalid_handle;
  std::unique_ptr<IPC::Message> message(new IPC::Message());
  IPC::ParamTraits<mojo::MessagePipeHandle>::Write(message.get(),
                                                   invalid_handle);

  channel()->Send(message.release());
  run_loop.Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT_WITH_CUSTOM_FIXTURE(
    ParamTraitInvalidMessagePipeClient,
    ParamTraitMessagePipeClient) {
  RunTest(false);
}

TEST_F(IPCChannelMojoTest, SendFailAfterClose) {
  Init("IPCChannelMojoTestSendOkClient");

  base::RunLoop run_loop;
  ListenerThatExpectsOK listener(run_loop.QuitClosure());
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  run_loop.Run();
  channel()->Close();
  ASSERT_FALSE(channel()->Send(new IPC::Message()));

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

class ListenerSendingOneOk : public TestListenerBase {
 public:
  ListenerSendingOneOk(base::Closure quit_closure)
      : TestListenerBase(quit_closure) {}

  bool OnMessageReceived(const IPC::Message& message) override { return true; }

  void OnChannelConnected(int32_t peer_pid) override {
    ListenerThatExpectsOK::SendOK(sender());
    quit_closure().Run();
  }
};

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestSendOkClient) {
  base::RunLoop run_loop;
  ListenerSendingOneOk listener(run_loop.QuitClosure());
  Connect(&listener);
  listener.set_sender(channel());

  run_loop.Run();

  Close();
}

class ListenerWithSimpleAssociatedInterface
    : public IPC::Listener,
      public IPC::mojom::SimpleTestDriver {
 public:
  static const int kNumMessages;

  ListenerWithSimpleAssociatedInterface() : binding_(this) {}

  ~ListenerWithSimpleAssociatedInterface() override = default;

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    int32_t should_be_expected;
    EXPECT_TRUE(iter.ReadInt(&should_be_expected));
    EXPECT_EQ(should_be_expected, next_expected_value_);
    num_messages_received_++;
    return true;
  }

  void OnChannelError() override { CHECK(received_quit_); }

  void RegisterInterfaceFactory(IPC::Channel* channel) {
    channel->GetAssociatedInterfaceSupport()->AddAssociatedInterface(
        base::Bind(&ListenerWithSimpleAssociatedInterface::BindRequest,
                   base::Unretained(this)));
  }

 private:
  // IPC::mojom::SimpleTestDriver:
  void ExpectValue(int32_t value) override {
    next_expected_value_ = value;
  }

  void GetExpectedValue(GetExpectedValueCallback callback) override {
    NOTREACHED();
  }

  void RequestValue(RequestValueCallback callback) override { NOTREACHED(); }

  void RequestQuit(RequestQuitCallback callback) override {
    EXPECT_EQ(kNumMessages, num_messages_received_);
    received_quit_ = true;
    std::move(callback).Run();
    base::RunLoop::QuitCurrentWhenIdleDeprecated();
  }

  void BindRequest(IPC::mojom::SimpleTestDriverAssociatedRequest request) {
    DCHECK(!binding_.is_bound());
    binding_.Bind(std::move(request));
  }

  int32_t next_expected_value_ = 0;
  int num_messages_received_ = 0;
  bool received_quit_ = false;

  mojo::AssociatedBinding<IPC::mojom::SimpleTestDriver> binding_;
};

const int ListenerWithSimpleAssociatedInterface::kNumMessages = 1000;

class ListenerSendingAssociatedMessages : public IPC::Listener {
 public:
  ListenerSendingAssociatedMessages() = default;

  bool OnMessageReceived(const IPC::Message& message) override { return true; }

  void OnChannelConnected(int32_t peer_pid) override {
    DCHECK(channel_);
    channel_->GetAssociatedInterfaceSupport()->GetRemoteAssociatedInterface(
        &driver_);

    // Send a bunch of interleaved messages, alternating between the associated
    // interface and a legacy IPC::Message.
    for (int i = 0; i < ListenerWithSimpleAssociatedInterface::kNumMessages;
         ++i) {
      driver_->ExpectValue(i);
      SendValue(channel_, i);
    }
    driver_->RequestQuit(base::Bind(&OnQuitAck));
  }

  void set_channel(IPC::Channel* channel) { channel_ = channel; }

 private:
  static void OnQuitAck() { base::RunLoop::QuitCurrentWhenIdleDeprecated(); }

  IPC::Channel* channel_ = nullptr;
  IPC::mojom::SimpleTestDriverAssociatedPtr driver_;
};

TEST_F(IPCChannelMojoTest, SimpleAssociatedInterface) {
  Init("SimpleAssociatedInterfaceClient");

  ListenerWithSimpleAssociatedInterface listener;
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  listener.RegisterInterfaceFactory(channel());

  base::RunLoop().Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(SimpleAssociatedInterfaceClient) {
  ListenerSendingAssociatedMessages listener;
  Connect(&listener);
  listener.set_channel(channel());

  base::RunLoop().Run();

  Close();
}

class ChannelProxyRunner {
 public:
  ChannelProxyRunner(mojo::ScopedMessagePipeHandle handle,
                     bool for_server)
      : for_server_(for_server),
        handle_(std::move(handle)),
        io_thread_("ChannelProxyRunner IO thread"),
        never_signaled_(base::WaitableEvent::ResetPolicy::MANUAL,
                        base::WaitableEvent::InitialState::NOT_SIGNALED) {
  }

  void CreateProxy(IPC::Listener* listener) {
    io_thread_.StartWithOptions(
        base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
    proxy_ = IPC::SyncChannel::Create(listener, io_thread_.task_runner(),
                                      base::ThreadTaskRunnerHandle::Get(),
                                      &never_signaled_);
  }

  void RunProxy() {
    std::unique_ptr<IPC::ChannelFactory> factory;
    if (for_server_) {
      factory = IPC::ChannelMojo::CreateServerFactory(
          std::move(handle_), io_thread_.task_runner(),
          base::ThreadTaskRunnerHandle::Get());
    } else {
      factory = IPC::ChannelMojo::CreateClientFactory(
          std::move(handle_), io_thread_.task_runner(),
          base::ThreadTaskRunnerHandle::Get());
    }
    proxy_->Init(std::move(factory), true);
  }

  IPC::ChannelProxy* proxy() { return proxy_.get(); }

 private:
  const bool for_server_;

  mojo::ScopedMessagePipeHandle handle_;
  base::Thread io_thread_;
  base::WaitableEvent never_signaled_;
  std::unique_ptr<IPC::ChannelProxy> proxy_;

  DISALLOW_COPY_AND_ASSIGN(ChannelProxyRunner);
};

class IPCChannelProxyMojoTest : public IPCChannelMojoTestBase {
 public:
  void Init(const std::string& client_name) {
    IPCChannelMojoTestBase::Init(client_name);
    runner_.reset(new ChannelProxyRunner(TakeHandle(), true));
  }
  void CreateProxy(IPC::Listener* listener) { runner_->CreateProxy(listener); }
  void RunProxy() {
    runner_->RunProxy();
  }
  void DestroyProxy() {
    runner_.reset();
    base::RunLoop().RunUntilIdle();
  }

  IPC::ChannelProxy* proxy() { return runner_->proxy(); }

 private:
  std::unique_ptr<ChannelProxyRunner> runner_;
};

class ListenerWithSimpleProxyAssociatedInterface
    : public IPC::Listener,
      public IPC::mojom::SimpleTestDriver {
 public:
  static const int kNumMessages;

  ListenerWithSimpleProxyAssociatedInterface() : binding_(this) {}

  ~ListenerWithSimpleProxyAssociatedInterface() override = default;

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    int32_t should_be_expected;
    EXPECT_TRUE(iter.ReadInt(&should_be_expected));
    EXPECT_EQ(should_be_expected, next_expected_value_);
    num_messages_received_++;
    return true;
  }

  void OnChannelError() override { CHECK(received_quit_); }

  void OnAssociatedInterfaceRequest(
      const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle handle) override {
    DCHECK_EQ(interface_name, IPC::mojom::SimpleTestDriver::Name_);
    binding_.Bind(
        IPC::mojom::SimpleTestDriverAssociatedRequest(std::move(handle)));
  }

  bool received_all_messages() const {
    return num_messages_received_ == kNumMessages && received_quit_;
  }

 private:
  // IPC::mojom::SimpleTestDriver:
  void ExpectValue(int32_t value) override {
    next_expected_value_ = value;
  }

  void GetExpectedValue(GetExpectedValueCallback callback) override {
    std::move(callback).Run(next_expected_value_);
  }

  void RequestValue(RequestValueCallback callback) override { NOTREACHED(); }

  void RequestQuit(RequestQuitCallback callback) override {
    received_quit_ = true;
    std::move(callback).Run();
    binding_.Close();
    base::RunLoop::QuitCurrentWhenIdleDeprecated();
  }

  void BindRequest(IPC::mojom::SimpleTestDriverAssociatedRequest request) {
    DCHECK(!binding_.is_bound());
    binding_.Bind(std::move(request));
  }

  int32_t next_expected_value_ = 0;
  int num_messages_received_ = 0;
  bool received_quit_ = false;

  mojo::AssociatedBinding<IPC::mojom::SimpleTestDriver> binding_;
};

const int ListenerWithSimpleProxyAssociatedInterface::kNumMessages = 1000;

TEST_F(IPCChannelProxyMojoTest, ProxyThreadAssociatedInterface) {
  Init("ProxyThreadAssociatedInterfaceClient");

  ListenerWithSimpleProxyAssociatedInterface listener;
  CreateProxy(&listener);
  RunProxy();

  base::RunLoop().Run();

  EXPECT_TRUE(WaitForClientShutdown());
  EXPECT_TRUE(listener.received_all_messages());

  DestroyProxy();
}

class ChannelProxyClient {
 public:
  void Init(mojo::ScopedMessagePipeHandle handle) {
    runner_.reset(new ChannelProxyRunner(std::move(handle), false));
  }

  void CreateProxy(IPC::Listener* listener) { runner_->CreateProxy(listener); }

  void RunProxy() { runner_->RunProxy(); }

  void DestroyProxy() {
    runner_.reset();
    base::RunLoop().RunUntilIdle();
  }

  void RequestQuitAndWaitForAck(IPC::mojom::SimpleTestDriver* driver) {
    base::RunLoop loop;
    driver->RequestQuit(loop.QuitClosure());
    loop.Run();
  }

  IPC::ChannelProxy* proxy() { return runner_->proxy(); }

 private:
  base::MessageLoop message_loop_;
  std::unique_ptr<ChannelProxyRunner> runner_;
};

class DummyListener : public IPC::Listener {
 public:
  // IPC::Listener
  bool OnMessageReceived(const IPC::Message& message) override { return true; }
};

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT_WITH_CUSTOM_FIXTURE(
    ProxyThreadAssociatedInterfaceClient,
    ChannelProxyClient) {
  DummyListener listener;
  CreateProxy(&listener);
  RunProxy();

  // Send a bunch of interleaved messages, alternating between the associated
  // interface and a legacy IPC::Message.
  IPC::mojom::SimpleTestDriverAssociatedPtr driver;
  proxy()->GetRemoteAssociatedInterface(&driver);
  for (int i = 0; i < ListenerWithSimpleProxyAssociatedInterface::kNumMessages;
       ++i) {
    driver->ExpectValue(i);
    SendValue(proxy(), i);
  }
  driver->RequestQuit(base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  base::RunLoop().Run();

  DestroyProxy();
}

class ListenerWithIndirectProxyAssociatedInterface
    : public IPC::Listener,
      public IPC::mojom::IndirectTestDriver,
      public IPC::mojom::PingReceiver {
 public:
  ListenerWithIndirectProxyAssociatedInterface()
      : driver_binding_(this), ping_receiver_binding_(this) {}
  ~ListenerWithIndirectProxyAssociatedInterface() override = default;

  // IPC::Listener:
  bool OnMessageReceived(const IPC::Message& message) override { return true; }

  void OnAssociatedInterfaceRequest(
      const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle handle) override {
    DCHECK(!driver_binding_.is_bound());
    DCHECK_EQ(interface_name, IPC::mojom::IndirectTestDriver::Name_);
    driver_binding_.Bind(
        IPC::mojom::IndirectTestDriverAssociatedRequest(std::move(handle)));
  }

  void set_ping_handler(const base::Closure& handler) {
    ping_handler_ = handler;
  }

 private:
  // IPC::mojom::IndirectTestDriver:
  void GetPingReceiver(
      IPC::mojom::PingReceiverAssociatedRequest request) override {
    ping_receiver_binding_.Bind(std::move(request));
  }

  // IPC::mojom::PingReceiver:
  void Ping(PingCallback callback) override {
    std::move(callback).Run();
    ping_handler_.Run();
  }

  mojo::AssociatedBinding<IPC::mojom::IndirectTestDriver> driver_binding_;
  mojo::AssociatedBinding<IPC::mojom::PingReceiver> ping_receiver_binding_;

  base::Closure ping_handler_;
};

TEST_F(IPCChannelProxyMojoTest, ProxyThreadAssociatedInterfaceIndirect) {
  // Tests that we can pipeline interface requests and subsequent messages
  // targeting proxy thread bindings, and the channel will still dispatch
  // messages appropriately.

  Init("ProxyThreadAssociatedInterfaceIndirectClient");

  ListenerWithIndirectProxyAssociatedInterface listener;
  CreateProxy(&listener);
  RunProxy();

  base::RunLoop loop;
  listener.set_ping_handler(loop.QuitClosure());
  loop.Run();

  EXPECT_TRUE(WaitForClientShutdown());

  DestroyProxy();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT_WITH_CUSTOM_FIXTURE(
    ProxyThreadAssociatedInterfaceIndirectClient,
    ChannelProxyClient) {
  DummyListener listener;
  CreateProxy(&listener);
  RunProxy();

  // Use an interface requested via another interface. On the remote end both
  // interfaces are bound on the proxy thread. This ensures that the Ping
  // message we send will still be dispatched properly even though the remote
  // endpoint may not have been bound yet by the time the message is initially
  // processed on the IO thread.
  IPC::mojom::IndirectTestDriverAssociatedPtr driver;
  IPC::mojom::PingReceiverAssociatedPtr ping_receiver;
  proxy()->GetRemoteAssociatedInterface(&driver);
  driver->GetPingReceiver(mojo::MakeRequest(&ping_receiver));

  base::RunLoop loop;
  ping_receiver->Ping(loop.QuitClosure());
  loop.Run();

  DestroyProxy();
}

class ListenerWithSyncAssociatedInterface
    : public IPC::Listener,
      public IPC::mojom::SimpleTestDriver {
 public:
  ListenerWithSyncAssociatedInterface() : binding_(this) {}
  ~ListenerWithSyncAssociatedInterface() override = default;

  void set_sync_sender(IPC::Sender* sync_sender) { sync_sender_ = sync_sender; }

  void RunUntilQuitRequested() {
    base::RunLoop loop;
    quit_closure_ = loop.QuitClosure();
    loop.Run();
  }

  void CloseBinding() { binding_.Close(); }

  void set_response_value(int32_t response) {
    response_value_ = response;
  }

 private:
  // IPC::mojom::SimpleTestDriver:
  void ExpectValue(int32_t value) override {
    next_expected_value_ = value;
  }

  void GetExpectedValue(GetExpectedValueCallback callback) override {
    std::move(callback).Run(next_expected_value_);
  }

  void RequestValue(RequestValueCallback callback) override {
    std::move(callback).Run(response_value_);
  }

  void RequestQuit(RequestQuitCallback callback) override {
    quit_closure_.Run();
    std::move(callback).Run();
  }

  // IPC::Listener:
  bool OnMessageReceived(const IPC::Message& message) override {
    EXPECT_EQ(0u, message.type());
    EXPECT_TRUE(message.is_sync());
    EXPECT_TRUE(message.should_unblock());
    std::unique_ptr<IPC::Message> reply(
        IPC::SyncMessage::GenerateReply(&message));
    reply->WriteInt(response_value_);
    DCHECK(sync_sender_);
    EXPECT_TRUE(sync_sender_->Send(reply.release()));
    return true;
  }

  void OnAssociatedInterfaceRequest(
      const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle handle) override {
    DCHECK(!binding_.is_bound());
    DCHECK_EQ(interface_name, IPC::mojom::SimpleTestDriver::Name_);
    binding_.Bind(
        IPC::mojom::SimpleTestDriverAssociatedRequest(std::move(handle)));
  }

  void BindRequest(IPC::mojom::SimpleTestDriverAssociatedRequest request) {
    DCHECK(!binding_.is_bound());
    binding_.Bind(std::move(request));
  }

  IPC::Sender* sync_sender_ = nullptr;
  int32_t next_expected_value_ = 0;
  int32_t response_value_ = 0;
  base::Closure quit_closure_;

  mojo::AssociatedBinding<IPC::mojom::SimpleTestDriver> binding_;
};

class SyncReplyReader : public IPC::MessageReplyDeserializer {
 public:
  explicit SyncReplyReader(int32_t* storage) : storage_(storage) {}
  ~SyncReplyReader() override = default;

 private:
  // IPC::MessageReplyDeserializer:
  bool SerializeOutputParameters(const IPC::Message& message,
                                 base::PickleIterator iter) override {
    if (!iter.ReadInt(storage_))
      return false;
    return true;
  }

  int32_t* storage_;

  DISALLOW_COPY_AND_ASSIGN(SyncReplyReader);
};

TEST_F(IPCChannelProxyMojoTest, SyncAssociatedInterface) {
  Init("SyncAssociatedInterface");

  ListenerWithSyncAssociatedInterface listener;
  CreateProxy(&listener);
  listener.set_sync_sender(proxy());
  RunProxy();

  // Run the client's simple sanity check to completion.
  listener.RunUntilQuitRequested();

  // Verify that we can send a sync IPC and service an incoming sync request
  // while waiting on it
  listener.set_response_value(42);
  IPC::mojom::SimpleTestClientAssociatedPtr client;
  proxy()->GetRemoteAssociatedInterface(&client);
  int32_t received_value;
  EXPECT_TRUE(client->RequestValue(&received_value));
  EXPECT_EQ(42, received_value);

  // Do it again. This time the client will send a classical sync IPC to us
  // while we wait.
  received_value = 0;
  EXPECT_TRUE(client->RequestValue(&received_value));
  EXPECT_EQ(42, received_value);

  // Now make a classical sync IPC request to the client. It will send a
  // sync associated interface message to us while we wait.
  received_value = 0;
  std::unique_ptr<IPC::SyncMessage> request(
      new IPC::SyncMessage(0, 0, IPC::Message::PRIORITY_NORMAL,
                           new SyncReplyReader(&received_value)));
  EXPECT_TRUE(proxy()->Send(request.release()));
  EXPECT_EQ(42, received_value);

  listener.CloseBinding();
  EXPECT_TRUE(WaitForClientShutdown());

  DestroyProxy();
}

class SimpleTestClientImpl : public IPC::mojom::SimpleTestClient,
                             public IPC::Listener {
 public:
  SimpleTestClientImpl() : binding_(this) {}

  void set_driver(IPC::mojom::SimpleTestDriver* driver) { driver_ = driver; }
  void set_sync_sender(IPC::Sender* sync_sender) { sync_sender_ = sync_sender; }

  void WaitForValueRequest() {
    run_loop_.reset(new base::RunLoop);
    run_loop_->Run();
  }

  void UseSyncSenderForRequest(bool use_sync_sender) {
    use_sync_sender_ = use_sync_sender;
  }

 private:
  // IPC::mojom::SimpleTestClient:
  void RequestValue(RequestValueCallback callback) override {
    int32_t response = 0;
    if (use_sync_sender_) {
      std::unique_ptr<IPC::SyncMessage> reply(new IPC::SyncMessage(
          0, 0, IPC::Message::PRIORITY_NORMAL, new SyncReplyReader(&response)));
      EXPECT_TRUE(sync_sender_->Send(reply.release()));
    } else {
      DCHECK(driver_);
      EXPECT_TRUE(driver_->RequestValue(&response));
    }

    std::move(callback).Run(response);

    DCHECK(run_loop_);
    run_loop_->Quit();
  }

  // IPC::Listener:
  bool OnMessageReceived(const IPC::Message& message) override {
    int32_t response;
    DCHECK(driver_);
    EXPECT_TRUE(driver_->RequestValue(&response));
    std::unique_ptr<IPC::Message> reply(
        IPC::SyncMessage::GenerateReply(&message));
    reply->WriteInt(response);
    EXPECT_TRUE(sync_sender_->Send(reply.release()));

    DCHECK(run_loop_);
    run_loop_->Quit();
    return true;
  }

  void OnAssociatedInterfaceRequest(
      const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle handle) override {
    DCHECK(!binding_.is_bound());
    DCHECK_EQ(interface_name, IPC::mojom::SimpleTestClient::Name_);

    binding_.Bind(
        IPC::mojom::SimpleTestClientAssociatedRequest(std::move(handle)));
  }

  bool use_sync_sender_ = false;
  mojo::AssociatedBinding<IPC::mojom::SimpleTestClient> binding_;
  IPC::Sender* sync_sender_ = nullptr;
  IPC::mojom::SimpleTestDriver* driver_ = nullptr;
  std::unique_ptr<base::RunLoop> run_loop_;

  DISALLOW_COPY_AND_ASSIGN(SimpleTestClientImpl);
};

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT_WITH_CUSTOM_FIXTURE(SyncAssociatedInterface,
                                                        ChannelProxyClient) {
  SimpleTestClientImpl client_impl;
  CreateProxy(&client_impl);
  client_impl.set_sync_sender(proxy());
  RunProxy();

  IPC::mojom::SimpleTestDriverAssociatedPtr driver;
  proxy()->GetRemoteAssociatedInterface(&driver);
  client_impl.set_driver(driver.get());

  // Simple sync message sanity check.
  driver->ExpectValue(42);
  int32_t expected_value = 0;
  EXPECT_TRUE(driver->GetExpectedValue(&expected_value));
  EXPECT_EQ(42, expected_value);
  RequestQuitAndWaitForAck(driver.get());

  // Wait for the test driver to perform a sync call test with our own sync
  // associated interface message nested inside.
  client_impl.UseSyncSenderForRequest(false);
  client_impl.WaitForValueRequest();

  // Wait for the test driver to perform a sync call test with our own classical
  // sync IPC nested inside.
  client_impl.UseSyncSenderForRequest(true);
  client_impl.WaitForValueRequest();

  // Wait for the test driver to perform a classical sync IPC request, with our
  // own sync associated interface message nested inside.
  client_impl.UseSyncSenderForRequest(false);
  client_impl.WaitForValueRequest();

  DestroyProxy();
}

TEST_F(IPCChannelProxyMojoTest, Pause) {
  // Ensures that pausing a channel elicits the expected behavior when sending
  // messages, unpausing, sending more messages, and then manually flushing.
  // Specifically a sequence like:
  //
  //   Connect()
  //   Send(A)
  //   Pause()
  //   Send(B)
  //   Send(C)
  //   Unpause(false)
  //   Send(D)
  //   Send(E)
  //   Flush()
  //
  // must result in the other end receiving messages A, D, E, B, D; in that
  // order.
  //
  // This behavior is required by some consumers of IPC::Channel, and it is not
  // sufficient to leave this up to the consumer to implement since associated
  // interface requests and messages also need to be queued according to the
  // same policy.
  Init("CreatePausedClient");

  DummyListener listener;
  CreateProxy(&listener);
  RunProxy();

  // This message must be sent immediately since the channel is unpaused.
  SendValue(proxy(), 1);

  proxy()->Pause();

  // These messages must be queued internally since the channel is paused.
  SendValue(proxy(), 2);
  SendValue(proxy(), 3);

  proxy()->Unpause(false /* flush */);

  // These messages must be sent immediately since the channel is unpaused.
  SendValue(proxy(), 4);
  SendValue(proxy(), 5);

  // Now we flush the previously queued messages.
  proxy()->Flush();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyProxy();
}

class ExpectValueSequenceListener : public IPC::Listener {
 public:
  explicit ExpectValueSequenceListener(base::queue<int32_t>* expected_values)
      : expected_values_(expected_values) {}
  ~ExpectValueSequenceListener() override = default;

  // IPC::Listener:
  bool OnMessageReceived(const IPC::Message& message) override {
    DCHECK(!expected_values_->empty());
    base::PickleIterator iter(message);
    int32_t should_be_expected;
    EXPECT_TRUE(iter.ReadInt(&should_be_expected));
    EXPECT_EQ(expected_values_->front(), should_be_expected);
    expected_values_->pop();
    if (expected_values_->empty())
      base::RunLoop::QuitCurrentWhenIdleDeprecated();
    return true;
  }

 private:
  base::queue<int32_t>* expected_values_;

  DISALLOW_COPY_AND_ASSIGN(ExpectValueSequenceListener);
};

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT_WITH_CUSTOM_FIXTURE(CreatePausedClient,
                                                        ChannelProxyClient) {
  base::queue<int32_t> expected_values;
  ExpectValueSequenceListener listener(&expected_values);
  CreateProxy(&listener);
  expected_values.push(1);
  expected_values.push(4);
  expected_values.push(5);
  expected_values.push(2);
  expected_values.push(3);
  RunProxy();
  base::RunLoop().Run();
  EXPECT_TRUE(expected_values.empty());
  DestroyProxy();
}

TEST_F(IPCChannelProxyMojoTest, AssociatedRequestClose) {
  Init("DropAssociatedRequest");

  DummyListener listener;
  CreateProxy(&listener);
  RunProxy();

  IPC::mojom::AssociatedInterfaceVendorAssociatedPtr vendor;
  proxy()->GetRemoteAssociatedInterface(&vendor);
  IPC::mojom::SimpleTestDriverAssociatedPtr tester;
  vendor->GetTestInterface(mojo::MakeRequest(&tester));
  base::RunLoop run_loop;
  tester.set_connection_error_handler(run_loop.QuitClosure());
  run_loop.Run();

  proxy()->GetRemoteAssociatedInterface(&tester);
  EXPECT_TRUE(WaitForClientShutdown());
  DestroyProxy();
}

class AssociatedInterfaceDroppingListener : public IPC::Listener {
 public:
  AssociatedInterfaceDroppingListener(const base::Closure& callback)
      : callback_(callback) {}
  bool OnMessageReceived(const IPC::Message& message) override { return false; }

  void OnAssociatedInterfaceRequest(
      const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle handle) override {
    if (interface_name == IPC::mojom::SimpleTestDriver::Name_)
      base::ResetAndReturn(&callback_).Run();
  }

 private:
  base::Closure callback_;
};

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT_WITH_CUSTOM_FIXTURE(DropAssociatedRequest,
                                                        ChannelProxyClient) {
  base::RunLoop run_loop;
  AssociatedInterfaceDroppingListener listener(run_loop.QuitClosure());
  CreateProxy(&listener);
  RunProxy();
  run_loop.Run();
  DestroyProxy();
}

#if !defined(OS_MACOSX)
// TODO(wez): On Mac we need to set up a MachPortBroker before we can transfer
// Mach ports (which underpin Sharedmemory on Mac) across IPC.

class ListenerThatExpectsSharedMemory : public TestListenerBase {
 public:
  ListenerThatExpectsSharedMemory(base::Closure quit_closure)
      : TestListenerBase(quit_closure) {}

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);

    base::SharedMemoryHandle shared_memory;
    EXPECT_TRUE(IPC::ReadParam(&message, &iter, &shared_memory));
    EXPECT_TRUE(shared_memory.IsValid());
    shared_memory.Close();

    ListenerThatExpectsOK::SendOK(sender());
    return true;
  }
};

TEST_F(IPCChannelMojoTest, SendSharedMemory) {
  Init("IPCChannelMojoTestSendSharedMemoryClient");

  // Create some shared-memory to share.
  base::SharedMemoryCreateOptions options;
  options.size = 1004;

  base::SharedMemory shmem;
  ASSERT_TRUE(shmem.Create(options));

  // Create a success listener, and launch the child process.
  base::RunLoop run_loop;
  ListenerThatExpectsOK listener(run_loop.QuitClosure());
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  // Send the child process an IPC with |shmem| attached, to verify
  // that is is correctly wrapped, transferred and unwrapped.
  IPC::Message* message = new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
  IPC::WriteParam(message, shmem.handle());
  ASSERT_TRUE(channel()->Send(message));

  run_loop.Run();

  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestSendSharedMemoryClient) {
  base::RunLoop run_loop;
  ListenerThatExpectsSharedMemory listener(run_loop.QuitClosure());
  Connect(&listener);
  listener.set_sender(channel());

  run_loop.Run();

  Close();
}

template <class SharedMemoryRegionType>
class IPCChannelMojoSharedMemoryRegionTypedTest : public IPCChannelMojoTest {};

struct WritableRegionTraits {
  using RegionType = base::WritableSharedMemoryRegion;
  static const char kClientName[];
};
const char WritableRegionTraits::kClientName[] =
    "IPCChannelMojoTestSendWritableSharedMemoryRegionClient";
struct UnsafeRegionTraits {
  using RegionType = base::UnsafeSharedMemoryRegion;
  static const char kClientName[];
};
const char UnsafeRegionTraits::kClientName[] =
    "IPCChannelMojoTestSendUnsafeSharedMemoryRegionClient";
struct ReadOnlyRegionTraits {
  using RegionType = base::ReadOnlySharedMemoryRegion;
  static const char kClientName[];
};
const char ReadOnlyRegionTraits::kClientName[] =
    "IPCChannelMojoTestSendReadOnlySharedMemoryRegionClient";

typedef ::testing::
    Types<WritableRegionTraits, UnsafeRegionTraits, ReadOnlyRegionTraits>
        AllSharedMemoryRegionTraits;
TYPED_TEST_CASE(IPCChannelMojoSharedMemoryRegionTypedTest,
                AllSharedMemoryRegionTraits);

template <class SharedMemoryRegionType>
class ListenerThatExpectsSharedMemoryRegion : public TestListenerBase {
 public:
  ListenerThatExpectsSharedMemoryRegion(base::Closure quit_closure)
      : TestListenerBase(std::move(quit_closure)) {}

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);

    SharedMemoryRegionType region;
    EXPECT_TRUE(IPC::ReadParam(&message, &iter, &region));
    EXPECT_TRUE(region.IsValid());

    // Verify the shared memory region has expected content.
    typename SharedMemoryRegionType::MappingType mapping = region.Map();
    std::string content = HandleSendingHelper::GetSendingFileContent();
    EXPECT_EQ(0, memcmp(mapping.memory(), content.data(), content.size()));

    ListenerThatExpectsOK::SendOK(sender());
    return true;
  }
};

TYPED_TEST(IPCChannelMojoSharedMemoryRegionTypedTest, Send) {
  this->Init(TypeParam::kClientName);

  const size_t size = 1004;
  typename TypeParam::RegionType region;
  base::WritableSharedMemoryMapping mapping;
  std::tie(region, mapping) =
      base::CreateMappedRegion<typename TypeParam::RegionType>(size);

  std::string content = HandleSendingHelper::GetSendingFileContent();
  memcpy(mapping.memory(), content.data(), content.size());

  // Create a success listener, and launch the child process.
  base::RunLoop run_loop;
  ListenerThatExpectsOK listener(run_loop.QuitClosure());
  this->CreateChannel(&listener);
  ASSERT_TRUE(this->ConnectChannel());

  // Send the child process an IPC with |shmem| attached, to verify
  // that is is correctly wrapped, transferred and unwrapped.
  IPC::Message* message = new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
  IPC::WriteParam(message, region);
  ASSERT_TRUE(this->channel()->Send(message));

  run_loop.Run();

  this->channel()->Close();

  EXPECT_TRUE(this->WaitForClientShutdown());
  EXPECT_FALSE(region.IsValid());
  this->DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(
    IPCChannelMojoTestSendWritableSharedMemoryRegionClient) {
  base::RunLoop run_loop;
  ListenerThatExpectsSharedMemoryRegion<base::WritableSharedMemoryRegion>
      listener(run_loop.QuitClosure());
  Connect(&listener);
  listener.set_sender(channel());

  run_loop.Run();

  Close();
}
DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(
    IPCChannelMojoTestSendUnsafeSharedMemoryRegionClient) {
  base::RunLoop run_loop;
  ListenerThatExpectsSharedMemoryRegion<base::UnsafeSharedMemoryRegion>
      listener(run_loop.QuitClosure());
  Connect(&listener);
  listener.set_sender(channel());

  run_loop.Run();

  Close();
}
DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(
    IPCChannelMojoTestSendReadOnlySharedMemoryRegionClient) {
  base::RunLoop run_loop;
  ListenerThatExpectsSharedMemoryRegion<base::ReadOnlySharedMemoryRegion>
      listener(run_loop.QuitClosure());
  Connect(&listener);
  listener.set_sender(channel());

  run_loop.Run();

  Close();
}
#endif  // !defined(OS_MACOSX)

#if defined(OS_POSIX) || defined(OS_FUCHSIA)

class ListenerThatExpectsFile : public TestListenerBase {
 public:
  ListenerThatExpectsFile(base::Closure quit_closure)
      : TestListenerBase(quit_closure) {}

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    HandleSendingHelper::ReadReceivedFile(message, &iter);
    ListenerThatExpectsOK::SendOK(sender());
    return true;
  }
};

TEST_F(IPCChannelMojoTest, SendPlatformFile) {
  Init("IPCChannelMojoTestSendPlatformFileClient");

  base::RunLoop run_loop;
  ListenerThatExpectsOK listener(run_loop.QuitClosure());
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::File file(HandleSendingHelper::GetSendingFilePath(temp_dir.GetPath()),
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE |
                      base::File::FLAG_READ);
  HandleSendingHelper::WriteFileThenSend(channel(), file);
  run_loop.Run();

  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestSendPlatformFileClient) {
  base::RunLoop run_loop;
  ListenerThatExpectsFile listener(run_loop.QuitClosure());
  Connect(&listener);
  listener.set_sender(channel());

  run_loop.Run();

  Close();
}

class ListenerThatExpectsFileAndMessagePipe : public TestListenerBase {
 public:
  ListenerThatExpectsFileAndMessagePipe(base::Closure quit_closure)
      : TestListenerBase(quit_closure) {}

  ~ListenerThatExpectsFileAndMessagePipe() override = default;

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    HandleSendingHelper::ReadReceivedFile(message, &iter);
    HandleSendingHelper::ReadReceivedPipe(message, &iter);
    ListenerThatExpectsOK::SendOK(sender());
    return true;
  }
};

TEST_F(IPCChannelMojoTest, SendPlatformFileAndMessagePipe) {
  Init("IPCChannelMojoTestSendPlatformFileAndMessagePipeClient");

  base::RunLoop run_loop;
  ListenerThatExpectsOK listener(run_loop.QuitClosure());
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::File file(HandleSendingHelper::GetSendingFilePath(temp_dir.GetPath()),
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE |
                      base::File::FLAG_READ);
  TestingMessagePipe pipe;
  HandleSendingHelper::WriteFileAndPipeThenSend(channel(), file, &pipe);

  run_loop.Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(
    IPCChannelMojoTestSendPlatformFileAndMessagePipeClient) {
  base::RunLoop run_loop;
  ListenerThatExpectsFileAndMessagePipe listener(run_loop.QuitClosure());
  Connect(&listener);
  listener.set_sender(channel());

  run_loop.Run();

  Close();
}

#endif  // defined(OS_POSIX) || defined(OS_FUCHSIA)

#if defined(OS_LINUX)

const base::ProcessId kMagicChildId = 54321;

class ListenerThatVerifiesPeerPid : public TestListenerBase {
 public:
  ListenerThatVerifiesPeerPid(base::Closure quit_closure)
      : TestListenerBase(quit_closure) {}

  void OnChannelConnected(int32_t peer_pid) override {
    EXPECT_EQ(peer_pid, kMagicChildId);
    quit_closure().Run();
  }

  bool OnMessageReceived(const IPC::Message& message) override {
    NOTREACHED();
    return true;
  }
};

TEST_F(IPCChannelMojoTest, VerifyGlobalPid) {
  Init("IPCChannelMojoTestVerifyGlobalPidClient");

  base::RunLoop run_loop;
  ListenerThatVerifiesPeerPid listener(run_loop.QuitClosure());
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  run_loop.Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestVerifyGlobalPidClient) {
  IPC::Channel::SetGlobalPid(kMagicChildId);

  base::RunLoop run_loop;
  ListenerThatQuits listener(run_loop.QuitClosure());
  Connect(&listener);

  run_loop.Run();

  Close();
}

#endif  // OS_LINUX

}  // namespace
