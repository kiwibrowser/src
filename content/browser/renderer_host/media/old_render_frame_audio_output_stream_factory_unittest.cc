// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/old_render_frame_audio_output_stream_factory.h"

#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/memory/unsafe_shared_memory_region.h"
#include "base/run_loop.h"
#include "base/sync_socket.h"
#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/browser/renderer_host/media/renderer_audio_output_stream_factory_context.h"
#include "content/common/media/renderer_audio_output_stream_factory.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "media/base/audio_parameters.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

using testing::Test;
using AudioOutputStreamFactory = mojom::RendererAudioOutputStreamFactory;
using AudioOutputStreamFactoryPtr =
    mojo::InterfacePtr<AudioOutputStreamFactory>;
using AudioOutputStreamFactoryRequest =
    mojo::InterfaceRequest<AudioOutputStreamFactory>;
using AudioOutputStream = media::mojom::AudioOutputStream;
using AudioOutputStreamPtr = mojo::InterfacePtr<AudioOutputStream>;
using AudioOutputStreamRequest = mojo::InterfaceRequest<AudioOutputStream>;
using AudioOutputStreamProviderClient =
    media::mojom::AudioOutputStreamProviderClient;
using AudioOutputStreamProviderClientPtr =
    mojo::InterfacePtr<AudioOutputStreamProviderClient>;
using AudioOutputStreamProviderClientRequest =
    mojo::InterfaceRequest<AudioOutputStreamProviderClient>;
using AudioOutputStreamProvider = media::mojom::AudioOutputStreamProvider;
using AudioOutputStreamProviderPtr =
    mojo::InterfacePtr<AudioOutputStreamProvider>;
using AudioOutputStreamProviderRequest =
    mojo::InterfaceRequest<AudioOutputStreamProvider>;

const int kStreamId = 0;
const int kNoSessionId = 0;
const int kRenderProcessId = 42;
const int kRenderFrameId = 24;
const int kSampleFrequency = 44100;
const int kSamplesPerBuffer = kSampleFrequency / 100;
const char kSalt[] = "salt";

media::AudioParameters GetTestAudioParameters() {
  return media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                                media::CHANNEL_LAYOUT_MONO, kSampleFrequency,
                                kSamplesPerBuffer);
}

class MockAudioOutputDelegate : public media::AudioOutputDelegate {
 public:
  // |on_destruction| can be used to observe the destruction of the delegate.
  explicit MockAudioOutputDelegate(
      base::OnceClosure on_destruction = base::OnceClosure())
      : on_destruction_(std::move(on_destruction)) {}

  ~MockAudioOutputDelegate() override {
    if (on_destruction_)
      std::move(on_destruction_).Run();
  }

  MOCK_METHOD0(GetStreamId, int());
  MOCK_METHOD0(OnPlayStream, void());
  MOCK_METHOD0(OnPauseStream, void());
  MOCK_METHOD1(OnSetVolume, void(double));

 private:
  base::OnceClosure on_destruction_;

  DISALLOW_COPY_AND_ASSIGN(MockAudioOutputDelegate);
};

class MockContext : public RendererAudioOutputStreamFactoryContext {
 public:
  explicit MockContext(bool auth_ok) : salt_(kSalt), auth_ok_(auth_ok) {}

  ~MockContext() override { EXPECT_EQ(nullptr, delegate_); }

  int GetRenderProcessId() const override { return kRenderProcessId; }

  void RequestDeviceAuthorization(
      int render_frame_id,
      int session_id,
      const std::string& device_id,
      AuthorizationCompletedCallback cb) const override {
    EXPECT_EQ(render_frame_id, kRenderFrameId);
    EXPECT_EQ(session_id, 0);
    if (auth_ok_) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::BindOnce(std::move(cb),
                         media::OutputDeviceStatus::OUTPUT_DEVICE_STATUS_OK,
                         GetTestAudioParameters(), "default", std::string()));
      return;
    }
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(cb),
                       media::OutputDeviceStatus::
                           OUTPUT_DEVICE_STATUS_ERROR_NOT_AUTHORIZED,
                       media::AudioParameters::UnavailableDeviceParams(),
                       std::string(), std::string()));
  }

  // The event handler for the delegate will be stored at
  // |*event_handler_location| when the delegate is created.
  void PrepareDelegateForCreation(
      std::unique_ptr<media::AudioOutputDelegate> delegate,
      media::AudioOutputDelegate::EventHandler** event_handler_location) {
    EXPECT_EQ(nullptr, delegate_);
    EXPECT_EQ(nullptr, delegate_event_handler_location_);
    delegate_ = std::move(delegate);
    delegate_event_handler_location_ = event_handler_location;
  }

  std::unique_ptr<media::AudioOutputDelegate> CreateDelegate(
      const std::string& unique_device_id,
      int render_frame_id,
      int stream_id,
      const media::AudioParameters& params,
      media::mojom::AudioOutputStreamObserverPtr stream_observer,
      media::AudioOutputDelegate::EventHandler* handler) override {
    EXPECT_NE(nullptr, delegate_);
    EXPECT_NE(nullptr, delegate_event_handler_location_);
    *delegate_event_handler_location_ = handler;
    delegate_event_handler_location_ = nullptr;
    return std::move(delegate_);
  }

  AudioOutputStreamFactoryPtr CreateFactory() {
    DCHECK(!factory_);
    AudioOutputStreamFactoryPtr ret;
    factory_ = std::make_unique<OldRenderFrameAudioOutputStreamFactory>(
        kRenderFrameId, this);
    factory_binding_ = std::make_unique<
        mojo::Binding<mojom::RendererAudioOutputStreamFactory>>(
        factory_.get(), mojo::MakeRequest(&ret));
    return ret;
  }

 private:
  const std::string salt_;
  const bool auth_ok_;
  std::unique_ptr<OldRenderFrameAudioOutputStreamFactory> factory_;
  std::unique_ptr<mojo::Binding<mojom::RendererAudioOutputStreamFactory>>
      factory_binding_;
  std::unique_ptr<media::AudioOutputDelegate> delegate_;
  media::AudioOutputDelegate::EventHandler** delegate_event_handler_location_ =
      nullptr;

  DISALLOW_COPY_AND_ASSIGN(MockContext);
};

class MockClient : public AudioOutputStreamProviderClient {
 public:
  MockClient() : provider_client_binding_(this) {}
  ~MockClient() override {}

  AudioOutputStreamProviderClientPtr MakeProviderClientPtr() {
    AudioOutputStreamProviderClientPtr p;
    provider_client_binding_.Bind(mojo::MakeRequest(&p));
    return p;
  }

  void Created(AudioOutputStreamPtr stream,
               media::mojom::AudioDataPipePtr data_pipe) override {
    was_called_ = true;
    stream_ = std::move(stream);
  }

  bool was_called() { return was_called_; }

  MOCK_METHOD0(OnError, void());

 private:
  mojo::Binding<AudioOutputStreamProviderClient> provider_client_binding_;
  AudioOutputStreamPtr stream_;
  bool was_called_ = false;

  DISALLOW_COPY_AND_ASSIGN(MockClient);
};

void AuthCallback(media::OutputDeviceStatus* status_out,
                  media::AudioParameters* params_out,
                  std::string* id_out,
                  media::OutputDeviceStatus status,
                  const media::AudioParameters& params,
                  const std::string& id) {
  *status_out = status;
  *params_out = params;
  *id_out = id;
}

}  // namespace

// This test authorizes and creates a stream, and checks that
// 1. the ProviderClient callback is called with appropriate parameters.
// 2. the AudioOutputDelegate is created.
// 3. when the delegate calls OnStreamCreated, this is propagated to the client.
TEST(OldRenderFrameAudioOutputStreamFactoryTest, CreateStream) {
  content::TestBrowserThreadBundle thread_bundle;
  AudioOutputStreamProviderPtr provider;
  MockClient client;
  media::AudioOutputDelegate::EventHandler* event_handler = nullptr;
  auto factory_context = std::make_unique<MockContext>(true);
  factory_context->PrepareDelegateForCreation(
      std::make_unique<MockAudioOutputDelegate>(), &event_handler);
  AudioOutputStreamFactoryPtr factory_ptr = factory_context->CreateFactory();

  media::OutputDeviceStatus status;
  media::AudioParameters params;
  std::string id;
  factory_ptr->RequestDeviceAuthorization(
      mojo::MakeRequest(&provider), kNoSessionId, "default",
      base::BindOnce(&AuthCallback, base::Unretained(&status),
                     base::Unretained(&params), base::Unretained(&id)));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(status, media::OUTPUT_DEVICE_STATUS_OK);
  EXPECT_EQ(params.AsHumanReadableString(),
            GetTestAudioParameters().AsHumanReadableString());
  EXPECT_TRUE(id.empty());

  provider->Acquire(params, client.MakeProviderClientPtr());
  base::RunLoop().RunUntilIdle();
  ASSERT_NE(event_handler, nullptr);

  base::UnsafeSharedMemoryRegion shared_memory_region =
      base::UnsafeSharedMemoryRegion::Create(100);
  ASSERT_TRUE(shared_memory_region.IsValid());

  auto local = std::make_unique<base::CancelableSyncSocket>();
  auto remote = std::make_unique<base::CancelableSyncSocket>();
  ASSERT_TRUE(
      base::CancelableSyncSocket::CreatePair(local.get(), remote.get()));
  event_handler->OnStreamCreated(kStreamId, std::move(shared_memory_region),
                                 std::move(remote));

  base::RunLoop().RunUntilIdle();
  // Make sure we got the callback from creating stream.
  EXPECT_TRUE(client.was_called());
}

TEST(OldRenderFrameAudioOutputStreamFactoryTest, NotAuthorized_Denied) {
  content::TestBrowserThreadBundle thread_bundle;
  AudioOutputStreamProviderPtr output_provider;
  auto factory_context = std::make_unique<MockContext>(false);
  AudioOutputStreamFactoryPtr factory_ptr = factory_context->CreateFactory();

  media::OutputDeviceStatus status;
  media::AudioParameters params;
  std::string id;
  factory_ptr->RequestDeviceAuthorization(
      mojo::MakeRequest(&output_provider), kNoSessionId, "default",
      base::BindOnce(&AuthCallback, base::Unretained(&status),
                     base::Unretained(&params), base::Unretained(&id)));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(status, media::OUTPUT_DEVICE_STATUS_ERROR_NOT_AUTHORIZED);
  EXPECT_TRUE(id.empty());
}

TEST(OldRenderFrameAudioOutputStreamFactoryTest,
     ConnectionError_DeletesStream) {
  content::TestBrowserThreadBundle thread_bundle;
  AudioOutputStreamProviderPtr provider;
  MockClient client;
  bool delegate_is_destructed = false;
  media::AudioOutputDelegate::EventHandler* event_handler = nullptr;
  auto factory_context = std::make_unique<MockContext>(true);
  factory_context->PrepareDelegateForCreation(
      std::make_unique<MockAudioOutputDelegate>(
          base::BindOnce([](bool* destructed) { *destructed = true; },
                         &delegate_is_destructed)),
      &event_handler);
  AudioOutputStreamFactoryPtr factory_ptr = factory_context->CreateFactory();

  factory_ptr->RequestDeviceAuthorization(
      mojo::MakeRequest(&provider), kNoSessionId, "default",
      base::BindOnce([](media::OutputDeviceStatus status,
                        const media::AudioParameters& params,
                        const std::string& id) {}));
  base::RunLoop().RunUntilIdle();

  provider->Acquire(GetTestAudioParameters(), client.MakeProviderClientPtr());
  base::RunLoop().RunUntilIdle();
  ASSERT_NE(event_handler, nullptr);
  EXPECT_FALSE(delegate_is_destructed);
  provider.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(delegate_is_destructed);
}

TEST(OldRenderFrameAudioOutputStreamFactoryTest, DelegateError_DeletesStream) {
  content::TestBrowserThreadBundle thread_bundle;
  AudioOutputStreamProviderPtr provider;
  MockClient client;
  bool delegate_is_destructed = false;
  media::AudioOutputDelegate::EventHandler* event_handler = nullptr;
  auto factory_context = std::make_unique<MockContext>(true);
  factory_context->PrepareDelegateForCreation(
      std::make_unique<MockAudioOutputDelegate>(
          base::BindOnce([](bool* destructed) { *destructed = true; },
                         &delegate_is_destructed)),
      &event_handler);
  AudioOutputStreamFactoryPtr factory_ptr = factory_context->CreateFactory();

  factory_ptr->RequestDeviceAuthorization(
      mojo::MakeRequest(&provider), kNoSessionId, "default",
      base::BindOnce([](media::OutputDeviceStatus status,
                        const media::AudioParameters& params,
                        const std::string& id) {}));
  base::RunLoop().RunUntilIdle();

  provider->Acquire(GetTestAudioParameters(), client.MakeProviderClientPtr());
  base::RunLoop().RunUntilIdle();
  ASSERT_NE(event_handler, nullptr);
  EXPECT_FALSE(delegate_is_destructed);
  event_handler->OnStreamError(kStreamId);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(delegate_is_destructed);
}

}  // namespace content
