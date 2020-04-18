// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/old_render_frame_audio_input_stream_factory.h"

#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/memory/read_only_shared_memory_region.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "base/sync_socket.h"
#include "content/browser/renderer_host/media/audio_input_device_manager.h"
#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/common/media/renderer_audio_input_stream_factory.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "media/audio/audio_system_impl.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "media/base/audio_parameters.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

using testing::Test;

const size_t kShmemSize = 1234;
const int kSessionId = 234;
const bool kAGC = false;
const uint32_t kSharedMemoryCount = 345;
const int kSampleFrequency = 44100;
const int kSamplesPerBuffer = kSampleFrequency / 100;
const bool kInitiallyMuted = false;
const int kRenderProcessID = -1;
const int kRenderFrameID = MSG_ROUTING_NONE;

media::AudioParameters GetTestAudioParameters() {
  return media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                                media::CHANNEL_LAYOUT_MONO, kSampleFrequency,
                                kSamplesPerBuffer);
}

class FakeAudioInputDelegate : public media::AudioInputDelegate {
 public:
  FakeAudioInputDelegate() {}

  ~FakeAudioInputDelegate() override {}

  int GetStreamId() override { return 0; };
  void OnRecordStream() override{};
  void OnSetVolume(double volume) override{};
  void OnSetOutputDeviceForAec(const std::string& output_device_id) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeAudioInputDelegate);
};

class FakeAudioInputStreamClient : public media::mojom::AudioInputStreamClient {
 public:
  void OnMutedStateChanged(bool is_muted) override {}
  void OnError() override {}
};

class MockRendererAudioInputStreamFactoryClient
    : public mojom::RendererAudioInputStreamFactoryClient {
 public:
  MOCK_METHOD0(Created, void());

  void StreamCreated(
      media::mojom::AudioInputStreamPtr input_stream,
      media::mojom::AudioInputStreamClientRequest client_request,
      media::mojom::AudioDataPipePtr data_pipe,
      bool initially_muted,
      const base::Optional<base::UnguessableToken>& stream_id) override {
    EXPECT_TRUE(stream_id.has_value());
    Created();
  }
};

// Creates a fake delegate and saves the provided event handler in
// |event_handler_out|.
std::unique_ptr<media::AudioInputDelegate> CreateFakeDelegate(
    media::AudioInputDelegate::EventHandler** event_handler_out,
    AudioInputDeviceManager* audio_input_device_manager,
    media::mojom::AudioLogPtr audio_log,
    AudioInputDeviceManager::KeyboardMicRegistration keyboard_mic_registration,
    uint32_t shared_memory_count,
    int stream_id,
    int session_id,
    bool automatic_gain_control,
    const media::AudioParameters& parameters,
    media::AudioInputDelegate::EventHandler* event_handler) {
  *event_handler_out = event_handler;
  return std::make_unique<FakeAudioInputDelegate>();
}

}  // namespace

class OldOldRenderFrameAudioInputStreamFactoryTest : public testing::Test {
 public:
  OldOldRenderFrameAudioInputStreamFactoryTest()
      : thread_bundle_(base::in_place),
        audio_manager_(std::make_unique<media::TestAudioThread>()),
        audio_system_(&audio_manager_),
        media_stream_manager_(&audio_system_, audio_manager_.GetTaskRunner()),
        client_binding_(&client_, mojo::MakeRequest(&client_ptr_)),
        factory_handle_(RenderFrameAudioInputStreamFactoryHandle::CreateFactory(
            base::BindRepeating(&CreateFakeDelegate, &event_handler_),
            &media_stream_manager_,
            kRenderProcessID,
            kRenderFrameID,
            mojo::MakeRequest(&factory_ptr_))) {}

  ~OldOldRenderFrameAudioInputStreamFactoryTest() override {
    audio_manager_.Shutdown();

    // UniqueAudioInputStreamFactoryPtr uses DeleteOnIOThread and must run
    // before |thread_bundle_| tear down.
    factory_handle_.reset();

    // Shutdown BrowserThread::IO before tearing down members.
    thread_bundle_.reset();
  }

  // |thread_bundle_| needs to be up before the members below (as they use
  // BrowserThreads for their initialization) but then needs to be torn down
  // before them as some verify they're town down in a single-threaded
  // environment (while
  // !BrowserThread::IsThreadInitiaslized(BrowserThread::IO)).
  base::Optional<TestBrowserThreadBundle> thread_bundle_;

  // These members need to be torn down after |thread_bundle_|.
  media::MockAudioManager audio_manager_;
  media::AudioSystemImpl audio_system_;
  MediaStreamManager media_stream_manager_;

  mojom::RendererAudioInputStreamFactoryPtr factory_ptr_;
  media::mojom::AudioInputStreamPtr stream_ptr_;
  MockRendererAudioInputStreamFactoryClient client_;
  mojom::RendererAudioInputStreamFactoryClientPtr client_ptr_;
  media::AudioInputDelegate::EventHandler* event_handler_ = nullptr;
  mojo::Binding<mojom::RendererAudioInputStreamFactoryClient> client_binding_;
  UniqueAudioInputStreamFactoryPtr factory_handle_;
};

TEST_F(OldOldRenderFrameAudioInputStreamFactoryTest, CreateStream) {
  factory_ptr_->CreateStream(std::move(client_ptr_), kSessionId,
                             GetTestAudioParameters(), kAGC,
                             kSharedMemoryCount);

  // Wait for delegate to be created and |event_handler| set.
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(event_handler_);
  auto shared_memory = base::ReadOnlySharedMemoryRegion::Create(kShmemSize);
  auto local = std::make_unique<base::CancelableSyncSocket>();
  auto remote = std::make_unique<base::CancelableSyncSocket>();
  ASSERT_TRUE(
      base::CancelableSyncSocket::CreatePair(local.get(), remote.get()));
  event_handler_->OnStreamCreated(/*stream_id, irrelevant*/ 0,
                                  std::move(shared_memory.region),
                                  std::move(remote), kInitiallyMuted);

  EXPECT_CALL(client_, Created());
  base::RunLoop().RunUntilIdle();
}

}  // namespace content
