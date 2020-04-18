// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/audio_output_stream_broker.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/sync_socket.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "base/unguessable_token.h"
#include "media/base/audio_parameters.h"
#include "media/mojo/interfaces/audio_output_stream.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/audio/public/cpp/fake_stream_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Test;
using ::testing::Mock;
using ::testing::StrictMock;
using ::testing::InSequence;

namespace content {

namespace {

const int kRenderProcessId = 123;
const int kRenderFrameId = 234;
const int kStreamId = 345;
const size_t kShMemSize = 456;
const char kDeviceId[] = "testdeviceid";

media::AudioParameters TestParams() {
  return media::AudioParameters::UnavailableDeviceParams();
}

using MockDeleterCallback = StrictMock<
    base::MockCallback<base::OnceCallback<void(AudioStreamBroker*)>>>;

class MockAudioOutputStreamProviderClient
    : public media::mojom::AudioOutputStreamProviderClient {
 public:
  MockAudioOutputStreamProviderClient() : binding_(this) {}
  ~MockAudioOutputStreamProviderClient() override {}

  void Created(media::mojom::AudioOutputStreamPtr,
               media::mojom::AudioDataPipePtr) override {
    OnCreated();
  }

  MOCK_METHOD0(OnCreated, void());

  MOCK_METHOD2(ConnectionError, void(uint32_t, const std::string&));

  media::mojom::AudioOutputStreamProviderClientPtr MakePtr() {
    media::mojom::AudioOutputStreamProviderClientPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    binding_.set_connection_error_with_reason_handler(
        base::BindOnce(&MockAudioOutputStreamProviderClient::ConnectionError,
                       base::Unretained(this)));
    return ptr;
  }

  void CloseBinding() { binding_.Close(); }

 private:
  mojo::Binding<media::mojom::AudioOutputStreamProviderClient> binding_;
  DISALLOW_COPY_AND_ASSIGN(MockAudioOutputStreamProviderClient);
};

class MockStreamFactory : public audio::FakeStreamFactory {
 public:
  MockStreamFactory() {}
  ~MockStreamFactory() final {}

  // State of an expected stream creation. |output_device_id|, |params|,
  // and |groups_id| are set ahead of time and verified during request.
  // The other fields are filled in when the request is received.
  struct StreamRequestData {
    StreamRequestData(const std::string& output_device_id,
                      const media::AudioParameters& params,
                      const base::UnguessableToken& group_id)
        : output_device_id(output_device_id),
          params(params),
          group_id(group_id) {}

    bool requested = false;
    media::mojom::AudioOutputStreamRequest stream_request;
    media::mojom::AudioOutputStreamObserverAssociatedPtrInfo observer_info;
    media::mojom::AudioLogPtr log;
    const std::string output_device_id;
    const media::AudioParameters params;
    const base::UnguessableToken group_id;
    CreateOutputStreamCallback created_callback;
  };

  void ExpectStreamCreation(StreamRequestData* ex) {
    stream_request_data_ = ex;
  }

 private:
  void CreateOutputStream(
      media::mojom::AudioOutputStreamRequest stream_request,
      media::mojom::AudioOutputStreamObserverAssociatedPtrInfo observer_info,
      media::mojom::AudioLogPtr log,
      const std::string& output_device_id,
      const media::AudioParameters& params,
      const base::UnguessableToken& group_id,
      CreateOutputStreamCallback created_callback) final {
    // No way to cleanly exit the test here in case of failure, so use CHECK.
    CHECK(stream_request_data_);
    EXPECT_EQ(stream_request_data_->output_device_id, output_device_id);
    EXPECT_TRUE(stream_request_data_->params.Equals(params));
    EXPECT_EQ(stream_request_data_->group_id, group_id);
    stream_request_data_->requested = true;
    stream_request_data_->stream_request = std::move(stream_request);
    stream_request_data_->observer_info = std::move(observer_info);
    stream_request_data_->log = std::move(log);
    stream_request_data_->created_callback = std::move(created_callback);
  }

  StreamRequestData* stream_request_data_;
  DISALLOW_COPY_AND_ASSIGN(MockStreamFactory);
};

// This struct collects test state we need without doing anything fancy.
struct TestEnvironment {
  TestEnvironment()
      : group(base::UnguessableToken::Create()),
        broker(std::make_unique<AudioOutputStreamBroker>(
            kRenderProcessId,
            kRenderFrameId,
            kStreamId,
            kDeviceId,
            TestParams(),
            group,
            deleter.Get(),
            provider_client.MakePtr())) {}

  void RunUntilIdle() { env.RunUntilIdle(); }

  base::test::ScopedTaskEnvironment env;
  base::UnguessableToken group;
  MockDeleterCallback deleter;
  StrictMock<MockAudioOutputStreamProviderClient> provider_client;
  std::unique_ptr<AudioOutputStreamBroker> broker;
  MockStreamFactory stream_factory;
  audio::mojom::StreamFactoryPtr factory_ptr = stream_factory.MakePtr();
};

}  // namespace

TEST(AudioOutputStreamBrokerTest, StoresProcessAndFrameId) {
  base::test::ScopedTaskEnvironment env;
  MockDeleterCallback deleter;
  StrictMock<MockAudioOutputStreamProviderClient> provider_client;

  AudioOutputStreamBroker broker(kRenderProcessId, kRenderFrameId, kStreamId,
                                 kDeviceId, TestParams(),
                                 base::UnguessableToken::Create(),
                                 deleter.Get(), provider_client.MakePtr());

  EXPECT_EQ(kRenderProcessId, broker.render_process_id());
  EXPECT_EQ(kRenderFrameId, broker.render_frame_id());
}

TEST(AudioOutputStreamBrokerTest, ClientDisconnect_CallsDeleter) {
  TestEnvironment env;

  EXPECT_CALL(env.deleter, Run(env.broker.release()))
      .WillOnce(testing::DeleteArg<0>());
  env.provider_client.CloseBinding();
  env.RunUntilIdle();
}

TEST(AudioOutputStreamBrokerTest, StreamCreationSuccess_Propagates) {
  TestEnvironment env;
  MockStreamFactory::StreamRequestData stream_request_data(
      kDeviceId, TestParams(), env.group);
  env.stream_factory.ExpectStreamCreation(&stream_request_data);

  env.broker->CreateStream(env.factory_ptr.get());
  env.RunUntilIdle();

  EXPECT_TRUE(stream_request_data.requested);

  // Set up test IPC primitives.
  base::SyncSocket socket1, socket2;
  base::SyncSocket::CreatePair(&socket1, &socket2);
  std::move(stream_request_data.created_callback)
      .Run({base::in_place, mojo::SharedBufferHandle::Create(kShMemSize),
            mojo::WrapPlatformFile(socket1.Release())});

  EXPECT_CALL(env.provider_client, OnCreated());

  env.RunUntilIdle();

  Mock::VerifyAndClear(&env.provider_client);

  env.broker.reset();
}

TEST(AudioOutputStreamBrokerTest,
     StreamCreationFailure_PropagatesErrorAndCallsDeleter) {
  TestEnvironment env;
  MockStreamFactory::StreamRequestData stream_request_data(
      kDeviceId, TestParams(), env.group);
  env.stream_factory.ExpectStreamCreation(&stream_request_data);

  env.broker->CreateStream(env.factory_ptr.get());
  env.RunUntilIdle();

  EXPECT_TRUE(stream_request_data.requested);
  EXPECT_CALL(env.provider_client,
              ConnectionError(static_cast<uint32_t>(
                                  media::mojom::AudioOutputStreamObserver::
                                      DisconnectReason::kPlatformError),
                              std::string()));
  EXPECT_CALL(env.deleter, Run(env.broker.release()))
      .WillOnce(testing::DeleteArg<0>());

  std::move(stream_request_data.created_callback).Run(nullptr);

  env.RunUntilIdle();
}

TEST(AudioOutputStreamBrokerTest,
     ObserverDisconnect_PropagatesErrorAndCallsDeleter) {
  TestEnvironment env;
  MockStreamFactory::StreamRequestData stream_request_data(
      kDeviceId, TestParams(), env.group);
  env.stream_factory.ExpectStreamCreation(&stream_request_data);

  env.broker->CreateStream(env.factory_ptr.get());
  env.RunUntilIdle();

  EXPECT_TRUE(stream_request_data.requested);
  EXPECT_CALL(env.provider_client,
              ConnectionError(static_cast<uint32_t>(
                                  media::mojom::AudioOutputStreamObserver::
                                      DisconnectReason::kPlatformError),
                              std::string()));
  EXPECT_CALL(env.deleter, Run(env.broker.release()))
      .WillOnce(testing::DeleteArg<0>());

  // This results in a connection error.
  stream_request_data.observer_info.PassHandle();

  env.RunUntilIdle();
  env.stream_factory.CloseBinding();
  env.RunUntilIdle();
}

TEST(AudioOutputStreamBrokerTest,
     FactoryDisconnectDuringConstruction_PropagatesErrorAndCallsDeleter) {
  TestEnvironment env;

  env.broker->CreateStream(env.factory_ptr.get());
  env.stream_factory.CloseBinding();

  EXPECT_CALL(env.deleter, Run(env.broker.release()))
      .WillOnce(testing::DeleteArg<0>());
  EXPECT_CALL(env.provider_client,
              ConnectionError(static_cast<uint32_t>(
                                  media::mojom::AudioOutputStreamObserver::
                                      DisconnectReason::kPlatformError),
                              std::string()));

  env.RunUntilIdle();
}

}  // namespace content
