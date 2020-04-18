// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/input_ipc.h"

#include "base/test/scoped_task_environment.h"
#include "media/mojo/interfaces/audio_data_pipe.mojom.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/audio/public/cpp/device_factory.h"
#include "services/audio/public/mojom/constants.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::StrictMock;

namespace audio {

namespace {

const std::string& kDeviceId = "1234";
const size_t kShMemSize = 456;
const double kNewVolume = 0.271828;

class MockStream : public media::mojom::AudioInputStream {
 public:
  MOCK_METHOD0(Record, void());
  MOCK_METHOD1(SetVolume, void(double));
};

class FakeStreamFactory : public audio::mojom::StreamFactory {
 public:
  FakeStreamFactory() : binding_(this), stream_(), stream_binding_(&stream_) {}
  ~FakeStreamFactory() override = default;
  void CreateInputStream(media::mojom::AudioInputStreamRequest stream_request,
                         media::mojom::AudioInputStreamClientPtr client,
                         media::mojom::AudioInputStreamObserverPtr observer,
                         media::mojom::AudioLogPtr log,
                         const std::string& device_id,
                         const media::AudioParameters& params,
                         uint32_t shared_memory_count,
                         bool enable_agc,
                         mojo::ScopedSharedBufferHandle key_press_count_buffer,
                         CreateInputStreamCallback created_callback) {
    if (should_fail_) {
      std::move(created_callback).Run(nullptr, initially_muted_, base::nullopt);
      return;
    }

    // Keep the client alive to avoid binding errors.
    client_ = std::move(client);

    if (stream_binding_.is_bound())
      stream_binding_.Unbind();

    stream_binding_.Bind(std::move(stream_request));

    base::SyncSocket socket1, socket2;
    base::SyncSocket::CreatePair(&socket1, &socket2);
    auto h = mojo::SharedBufferHandle::Create(kShMemSize);
    std::move(created_callback)
        .Run({base::in_place,
              h->Clone(mojo::SharedBufferHandle::AccessMode::READ_ONLY),
              mojo::WrapPlatformFile(socket1.Release())},
             initially_muted_, base::UnguessableToken::Create());
  }

  MOCK_METHOD2(AssociateInputAndOutputForAec,
               void(const base::UnguessableToken& input_stream_id,
                    const std::string& output_device_id));

  MOCK_METHOD7(CreateOutputStream,
               void(media::mojom::AudioOutputStreamRequest stream_request,
                    media::mojom::AudioOutputStreamObserverAssociatedPtrInfo
                        observer_info,
                    media::mojom::AudioLogPtr log,
                    const std::string& output_device_id,
                    const media::AudioParameters& params,
                    const base::UnguessableToken& group_id,
                    CreateOutputStreamCallback created_callback));

  MOCK_METHOD2(BindMuter,
               void(mojom::LocalMuterAssociatedRequest request,
                    const base::UnguessableToken& group_id));

  MOCK_METHOD7(CreateLoopbackStream,
               void(media::mojom::AudioInputStreamRequest stream_request,
                    media::mojom::AudioInputStreamClientPtr client,
                    media::mojom::AudioInputStreamObserverPtr observer,
                    const media::AudioParameters& params,
                    uint32_t shared_memory_count,
                    const base::UnguessableToken& group_id,
                    CreateLoopbackStreamCallback created_callback));

  void Bind(mojo::ScopedMessagePipeHandle handle) {
    binding_.Bind(audio::mojom::StreamFactoryRequest(std::move(handle)));
  }

  mojo::Binding<audio::mojom::StreamFactory> binding_;
  StrictMock<MockStream> stream_;
  media::mojom::AudioInputStreamClientPtr client_;
  mojo::Binding<media::mojom::AudioInputStream> stream_binding_;
  bool initially_muted_ = true;
  bool should_fail_ = false;
};

class MockDelegate : public media::AudioInputIPCDelegate {
 public:
  MockDelegate() {}
  ~MockDelegate() override {}

  void OnStreamCreated(base::SharedMemoryHandle mem_handle,
                       base::SyncSocket::Handle socket_handle,
                       bool initially_muted) override {
    base::SharedMemory sh_mem(
        mem_handle, /*read_only*/ true);  // Releases the shared memory handle.
    base::SyncSocket socket(socket_handle);  // Releases the socket descriptor.
    GotOnStreamCreated(initially_muted);
  }

  MOCK_METHOD1(GotOnStreamCreated, void(bool initially_muted));
  MOCK_METHOD0(OnError, void());
  MOCK_METHOD1(OnMuted, void(bool));
  MOCK_METHOD0(OnIPCClosed, void());
};

class InputIPCTest : public ::testing::Test {
 public:
  base::test::ScopedTaskEnvironment scoped_task_environment;
  std::unique_ptr<audio::InputIPC> ipc;
  const media::AudioParameters audioParameters =
      media::AudioParameters(media::AudioParameters::AUDIO_PCM_LINEAR,
                             media::CHANNEL_LAYOUT_STEREO,
                             16000,
                             1600);

 protected:
  InputIPCTest()
      : scoped_task_environment(
            base::test::ScopedTaskEnvironment::MainThreadType::DEFAULT,
            base::test::ScopedTaskEnvironment::ExecutionMode::QUEUED) {}
  std::unique_ptr<StrictMock<FakeStreamFactory>> factory_;

  void SetUp() override {
    service_manager::mojom::ConnectorRequest request;
    std::unique_ptr<service_manager::Connector> connector =
        service_manager::Connector::Create(&request);

    factory_ = std::make_unique<StrictMock<FakeStreamFactory>>();
    {
      service_manager::Connector::TestApi test_api(connector.get());

      test_api.OverrideBinderForTesting(
          service_manager::Identity(audio::mojom::kServiceName),
          audio::mojom::StreamFactory::Name_,
          base::BindRepeating(&FakeStreamFactory::Bind,
                              base::Unretained(factory_.get())));
    }
    ipc = std::make_unique<InputIPC>(std::move(connector), kDeviceId, nullptr);
  }
};

}  // namespace

TEST_F(InputIPCTest, CreateStreamPropagates) {
  StrictMock<MockDelegate> delegate;
  EXPECT_CALL(delegate, GotOnStreamCreated(_));
  ipc->CreateStream(&delegate, audioParameters, false, 0);
  scoped_task_environment.RunUntilIdle();
}

TEST_F(InputIPCTest, CreateStreamPropagatesInitiallyMuted) {
  StrictMock<MockDelegate> delegate;

  factory_->initially_muted_ = true;
  EXPECT_CALL(delegate, GotOnStreamCreated(true));
  ipc->CreateStream(&delegate, audioParameters, false, 0);
  scoped_task_environment.RunUntilIdle();
  ipc->CloseStream();
  scoped_task_environment.RunUntilIdle();

  factory_->initially_muted_ = false;
  EXPECT_CALL(delegate, GotOnStreamCreated(false));
  ipc->CreateStream(&delegate, audioParameters, false, 0);
  scoped_task_environment.RunUntilIdle();
  ipc->CloseStream();
  scoped_task_environment.RunUntilIdle();
}

TEST_F(InputIPCTest, MutedStateChangesPropagates) {
  StrictMock<MockDelegate> delegate;

  EXPECT_CALL(delegate, GotOnStreamCreated(_));
  ipc->CreateStream(&delegate, audioParameters, false, 0);
  scoped_task_environment.RunUntilIdle();

  EXPECT_CALL(delegate, OnMuted(true));
  factory_->client_->OnMutedStateChanged(true);
  scoped_task_environment.RunUntilIdle();

  EXPECT_CALL(delegate, OnMuted(false));
  factory_->client_->OnMutedStateChanged(false);
  scoped_task_environment.RunUntilIdle();
}

TEST_F(InputIPCTest, Record_Records) {
  StrictMock<MockDelegate> delegate;
  EXPECT_CALL(delegate, GotOnStreamCreated(_));
  ipc->CreateStream(&delegate, audioParameters, false, 0);
  scoped_task_environment.RunUntilIdle();

  EXPECT_CALL(factory_->stream_, Record());
  ipc->RecordStream();
  scoped_task_environment.RunUntilIdle();
}

TEST_F(InputIPCTest, IsReusable) {
  for (int i = 0; i < 5; i++) {
    StrictMock<MockDelegate> delegate;
    EXPECT_CALL(delegate, GotOnStreamCreated(_));
    ipc->CreateStream(&delegate, audioParameters, false, 0);
    scoped_task_environment.RunUntilIdle();

    ipc->CloseStream();
    scoped_task_environment.RunUntilIdle();

    testing::Mock::VerifyAndClearExpectations(&delegate);
  }
}

TEST_F(InputIPCTest, SetVolume_SetsVolume) {
  StrictMock<MockDelegate> delegate;
  EXPECT_CALL(delegate, GotOnStreamCreated(_));
  ipc->CreateStream(&delegate, audioParameters, false, 0);
  scoped_task_environment.RunUntilIdle();

  EXPECT_CALL(factory_->stream_, SetVolume(kNewVolume));
  ipc->SetVolume(kNewVolume);
  scoped_task_environment.RunUntilIdle();
}

TEST_F(InputIPCTest, SetOutputDeviceForAec_AssociatesInputAndOutputForAec) {
  const std::string kOutputDeviceId = "2345";
  StrictMock<MockDelegate> delegate;
  EXPECT_CALL(delegate, GotOnStreamCreated(_));
  ipc->CreateStream(&delegate, audioParameters, false, 0);
  scoped_task_environment.RunUntilIdle();

  EXPECT_CALL(*factory_, AssociateInputAndOutputForAec(_, kOutputDeviceId));
  ipc->SetOutputDeviceForAec(kOutputDeviceId);
  scoped_task_environment.RunUntilIdle();
}

TEST_F(InputIPCTest, FailedStreamCreationNullCallback) {
  StrictMock<MockDelegate> delegate;
  EXPECT_CALL(delegate, OnError()).Times(2);
  factory_->should_fail_ = true;
  ipc->CreateStream(&delegate, audioParameters, false, 0);
  scoped_task_environment.RunUntilIdle();
}

TEST_F(InputIPCTest, FailedStreamCreationDestuctedFactory) {
  StrictMock<MockDelegate> delegate;
  EXPECT_CALL(delegate, OnError());
  factory_ = nullptr;
  ipc->CreateStream(&delegate, audioParameters, false, 0);
  scoped_task_environment.RunUntilIdle();
}

}  // namespace audio
