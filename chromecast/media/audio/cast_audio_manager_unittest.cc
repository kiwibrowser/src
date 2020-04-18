// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/audio/cast_audio_manager.h"

#include <memory>
#include <string>
#include <utility>

#include "base/test/test_message_loop.h"
#include "chromecast/media/cma/test/mock_cma_backend.h"
#include "chromecast/media/cma/test/mock_cma_backend_factory.h"
#include "media/audio/fake_audio_log_factory.h"
#include "media/audio/test_audio_thread.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

namespace chromecast {
namespace media {
namespace {

const ::media::AudioParameters kDefaultAudioParams(
    ::media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
    ::media::CHANNEL_LAYOUT_STEREO,
    ::media::AudioParameters::kAudioCDSampleRate,
    256);

class CastAudioManagerTest : public testing::Test {
 public:
  CastAudioManagerTest() : media_thread_("CastMediaThread") {
    CHECK(media_thread_.Start());

    auto backend = std::make_unique<MockCmaBackendFactory>();
    backend_factory_ = backend.get();
    audio_manager_ = std::make_unique<CastAudioManager>(
        std::make_unique<::media::TestAudioThread>(), &audio_log_factory_,
        std::move(backend), media_thread_.task_runner(), false);
  }

  ~CastAudioManagerTest() override { audio_manager_->Shutdown(); }

 protected:
  base::TestMessageLoop message_loop_;
  base::Thread media_thread_;
  ::media::FakeAudioLogFactory audio_log_factory_;
  std::unique_ptr<CastAudioManager> audio_manager_;

  // Owned by |audio_manager_|
  MockCmaBackendFactory* backend_factory_;
};

TEST_F(CastAudioManagerTest, MakeAudioOutputStreamProxy) {
  StrictMock<MockCmaBackend::AudioDecoder> audio_decoder;
  EXPECT_CALL(audio_decoder, SetDelegate(_)).Times(1);
  EXPECT_CALL(audio_decoder, SetConfig(_)).WillOnce(Return(true));

  auto backend = std::make_unique<StrictMock<MockCmaBackend>>();
  EXPECT_CALL(*backend, CreateAudioDecoder()).WillOnce(Return(&audio_decoder));
  EXPECT_CALL(*backend, Initialize()).WillOnce(Return(true));

  EXPECT_CALL(*backend_factory_, CreateBackend(_))
      .WillOnce(Invoke([&backend](const MediaPipelineDeviceParams&) {
        return std::move(backend);
      }));

  ::media::AudioOutputStream* stream =
      audio_manager_->MakeAudioOutputStreamProxy(kDefaultAudioParams,
                                                 std::string());
  EXPECT_TRUE(stream->Open());
  stream->Close();
}

}  // namespace
}  // namespace media
}  // namespace chromecast
