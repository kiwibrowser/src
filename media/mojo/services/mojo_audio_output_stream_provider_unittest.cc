// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_audio_output_stream_provider.h"

#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "build/build_config.h"
#include "media/audio/audio_output_delegate.h"
#include "media/base/audio_parameters.h"
#include "mojo/edk/embedder/embedder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// Basic usage of MojoAudioOutputStreamProvider is tested in
// the RenderFrameAudioOutputStreamFactoryTest tests.
// These additional tests test some error conditions.

namespace media {

namespace {

using testing::DeleteArg;
using testing::Mock;
using testing::StrictMock;

using MockDeleter = base::MockCallback<
    base::OnceCallback<void(mojom::AudioOutputStreamProvider*)>>;

class FakeObserver : public mojom::AudioOutputStreamObserver {
 public:
  FakeObserver() = default;
  ~FakeObserver() override = default;

  void DidStartPlaying() override {}
  void DidStopPlaying() override {}
  void DidChangeAudibleState(bool is_audible) override {}
};

class FakeDelegate : public AudioOutputDelegate {
 public:
  explicit FakeDelegate(mojom::AudioOutputStreamObserverPtr observer)
      : observer_(std::move(observer)) {}
  ~FakeDelegate() override = default;

  int GetStreamId() override { return 0; }
  void OnPlayStream() override {}
  void OnPauseStream() override {}
  void OnSetVolume(double) override {}

 private:
  mojom::AudioOutputStreamObserverPtr observer_;
};

std::unique_ptr<AudioOutputDelegate> CreateFakeDelegate(
    const AudioParameters& params,
    mojom::AudioOutputStreamObserverPtr observer,
    AudioOutputDelegate::EventHandler*) {
  return std::make_unique<FakeDelegate>(std::move(observer));
}

}  // namespace

TEST(MojoAudioOutputStreamProviderTest, AcquireTwice_BadMessage) {
  base::MessageLoop loop;
  bool got_bad_message = false;
  mojo::edk::SetDefaultProcessErrorCallback(
      base::BindRepeating([](bool* got_bad_message,
                             const std::string& s) { *got_bad_message = true; },
                          &got_bad_message));

  mojom::AudioOutputStreamProviderPtr provider_ptr;
  StrictMock<MockDeleter> deleter;

  // Freed by deleter.
  auto* provider = new MojoAudioOutputStreamProvider(
      mojo::MakeRequest(&provider_ptr), base::BindOnce(&CreateFakeDelegate),
      deleter.Get(), std::make_unique<FakeObserver>());

  mojom::AudioOutputStreamProviderClientPtr client_1;
  mojo::MakeRequest(&client_1);
  provider_ptr->Acquire(media::AudioParameters::UnavailableDeviceParams(),
                        std::move(client_1));

  mojom::AudioOutputStreamProviderClientPtr client_2;
  mojo::MakeRequest(&client_2);
  provider_ptr->Acquire(media::AudioParameters::UnavailableDeviceParams(),
                        std::move(client_2));

  EXPECT_CALL(deleter, Run(provider)).WillOnce(DeleteArg<0>());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(got_bad_message);
  Mock::VerifyAndClear(&deleter);

  mojo::edk::SetDefaultProcessErrorCallback(mojo::edk::ProcessErrorCallback());
}

TEST(MojoAudioOutputStreamProviderTest,
     Bitstream_BadMessageOnNonAndoirdPlatforms) {
  base::MessageLoop loop;
  bool got_bad_message = false;
  mojo::edk::SetDefaultProcessErrorCallback(
      base::BindRepeating([](bool* got_bad_message,
                             const std::string& s) { *got_bad_message = true; },
                          &got_bad_message));

  mojom::AudioOutputStreamProviderPtr provider_ptr;
  StrictMock<MockDeleter> deleter;
  media::AudioParameters params =
      media::AudioParameters::UnavailableDeviceParams();
  params.set_format(AudioParameters::AUDIO_BITSTREAM_AC3);

  auto* provider = new MojoAudioOutputStreamProvider(
      mojo::MakeRequest(&provider_ptr), base::BindOnce(&CreateFakeDelegate),
      deleter.Get(), std::make_unique<FakeObserver>());

  mojom::AudioOutputStreamProviderClientPtr client;
  mojo::MakeRequest(&client);
  provider_ptr->Acquire(params, std::move(client));

#if defined(OS_ANDROID)
  base::RunLoop().RunUntilIdle();
  // Creating bitstream streams is allowed on Android.
  EXPECT_FALSE(got_bad_message);
  // |deleter| shouldn't have been called, so delete manually.
  Mock::VerifyAndClear(&deleter);
  delete provider;
#else
  EXPECT_CALL(deleter, Run(provider)).WillOnce(DeleteArg<0>());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(got_bad_message);
  Mock::VerifyAndClear(&deleter);
#endif
  mojo::edk::SetDefaultProcessErrorCallback(mojo::edk::ProcessErrorCallback());
}

}  // namespace media
