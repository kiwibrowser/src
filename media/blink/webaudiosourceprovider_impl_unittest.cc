// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "media/base/audio_parameters.h"
#include "media/base/fake_audio_render_callback.h"
#include "media/base/media_log.h"
#include "media/base/mock_audio_renderer_sink.h"
#include "media/blink/webaudiosourceprovider_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_audio_source_provider_client.h"

using ::testing::_;

namespace media {

namespace {
const float kTestVolume = 0.25;
const int kSampleRate = 48000;

class WebAudioSourceProviderImplUnderTest : public WebAudioSourceProviderImpl {
 public:
  WebAudioSourceProviderImplUnderTest(
      scoped_refptr<SwitchableAudioRendererSink> sink)
      : WebAudioSourceProviderImpl(std::move(sink), &media_log_),
        fallback_sink_(new MockAudioRendererSink()) {}

  MockAudioRendererSink* fallback_sink() { return fallback_sink_.get(); }

 protected:
  scoped_refptr<SwitchableAudioRendererSink> CreateFallbackSink() override {
    return fallback_sink_;
  }

 private:
  ~WebAudioSourceProviderImplUnderTest() override = default;
  MediaLog media_log_;
  scoped_refptr<MockAudioRendererSink> fallback_sink_;

  DISALLOW_COPY_AND_ASSIGN(WebAudioSourceProviderImplUnderTest);
};

enum class WaspSinkStatus { WASP_SINK_OK, WASP_SINK_ERROR, WASP_SINK_NULL };

scoped_refptr<MockAudioRendererSink> CreateWaspMockSink(WaspSinkStatus status) {
  if (status == WaspSinkStatus::WASP_SINK_NULL)
    return nullptr;
  return new MockAudioRendererSink(status == WaspSinkStatus::WASP_SINK_OK
                                       ? OUTPUT_DEVICE_STATUS_OK
                                       : OUTPUT_DEVICE_STATUS_ERROR_INTERNAL);
}

}  // namespace

class WebAudioSourceProviderImplTest
    : public testing::TestWithParam<WaspSinkStatus>,
      public blink::WebAudioSourceProviderClient {
 public:
  WebAudioSourceProviderImplTest()
      : params_(AudioParameters::AUDIO_PCM_LINEAR,
                CHANNEL_LAYOUT_STEREO,
                kSampleRate,
                64),
        fake_callback_(0.1, kSampleRate),
        mock_sink_(CreateWaspMockSink(GetParam())),
        wasp_impl_(new WebAudioSourceProviderImplUnderTest(mock_sink_)),
        expected_sink_(GetParam() == WaspSinkStatus::WASP_SINK_OK
                           ? mock_sink_.get()
                           : wasp_impl_->fallback_sink()) {}

  virtual ~WebAudioSourceProviderImplTest() = default;

  void CallAllSinkMethodsAndVerify(bool verify) {
    testing::InSequence s;

    EXPECT_CALL(*expected_sink_, Start()).Times(verify);
    wasp_impl_->Start();

    EXPECT_CALL(*expected_sink_, Play()).Times(verify);
    wasp_impl_->Play();

    EXPECT_CALL(*expected_sink_, Pause()).Times(verify);
    wasp_impl_->Pause();

    EXPECT_CALL(*expected_sink_, SetVolume(kTestVolume)).Times(verify);
    wasp_impl_->SetVolume(kTestVolume);

    EXPECT_CALL(*expected_sink_, Stop()).Times(verify);
    wasp_impl_->Stop();

    testing::Mock::VerifyAndClear(mock_sink_.get());
  }

  void SetClient(blink::WebAudioSourceProviderClient* client) {
    testing::InSequence s;

    if (client) {
      EXPECT_CALL(*expected_sink_, Stop());
      EXPECT_CALL(*this, SetFormat(params_.channels(), params_.sample_rate()));
    }
    wasp_impl_->SetClient(client);
    base::RunLoop().RunUntilIdle();

    testing::Mock::VerifyAndClear(mock_sink_.get());
    testing::Mock::VerifyAndClear(wasp_impl_->fallback_sink());
    testing::Mock::VerifyAndClear(this);
  }

  bool CompareBusses(const AudioBus* bus1, const AudioBus* bus2) {
    EXPECT_EQ(bus1->channels(), bus2->channels());
    EXPECT_EQ(bus1->frames(), bus2->frames());
    for (int ch = 0; ch < bus1->channels(); ++ch) {
      if (memcmp(bus1->channel(ch), bus2->channel(ch),
                 sizeof(*bus1->channel(ch)) * bus1->frames()) != 0) {
        return false;
      }
    }
    return true;
  }

  // blink::WebAudioSourceProviderClient implementation.
  MOCK_METHOD2(SetFormat, void(size_t numberOfChannels, float sampleRate));

  // CopyAudioCB. Added forwarder method due to GMock troubles with scoped_ptr.
  MOCK_METHOD3(DoCopyAudioCB,
               void(AudioBus*, uint32_t frames_delayed, int sample_rate));
  void OnAudioBus(std::unique_ptr<AudioBus> bus,
                  uint32_t frames_delayed,
                  int sample_rate) {
    DoCopyAudioCB(bus.get(), frames_delayed, sample_rate);
  }

  int Render(AudioBus* audio_bus) {
    return wasp_impl_->RenderForTesting(audio_bus);
  }

  void ExpectUnhealthySinkToStop() {
    if (GetParam() == WaspSinkStatus::WASP_SINK_ERROR)
      EXPECT_CALL(*mock_sink_.get(), Stop());
  }

 protected:
  AudioParameters params_;
  FakeAudioRenderCallback fake_callback_;
  scoped_refptr<MockAudioRendererSink> mock_sink_;
  scoped_refptr<WebAudioSourceProviderImplUnderTest> wasp_impl_;
  MockAudioRendererSink* expected_sink_;

  DISALLOW_COPY_AND_ASSIGN(WebAudioSourceProviderImplTest);
};

TEST_P(WebAudioSourceProviderImplTest, SetClientBeforeInitialize) {
  // setClient() with a NULL client should do nothing if no client is set.
  wasp_impl_->SetClient(NULL);

  // If |mock_sink_| is not null, it should be stopped during setClient(this).
  // If it is unhealthy, it should also be stopped during fallback in
  // Initialize().
  if (mock_sink_)
    EXPECT_CALL(*mock_sink_.get(), Stop())
        .Times(2 + static_cast<int>(GetParam()));

  wasp_impl_->SetClient(this);
  base::RunLoop().RunUntilIdle();

  if (mock_sink_)
    EXPECT_CALL(*mock_sink_.get(), SetVolume(1)).Times(1);
  wasp_impl_->SetClient(NULL);
  base::RunLoop().RunUntilIdle();

  wasp_impl_->SetClient(this);
  base::RunLoop().RunUntilIdle();

  // When Initialize() is called after setClient(), the params should propagate
  // to the client via setFormat() during the call.
  EXPECT_CALL(*this, SetFormat(params_.channels(), params_.sample_rate()));
  wasp_impl_->Initialize(params_, &fake_callback_);
  base::RunLoop().RunUntilIdle();

  // setClient() with the same client should do nothing.
  wasp_impl_->SetClient(this);
  base::RunLoop().RunUntilIdle();
}

// Verify AudioRendererSink functionality w/ and w/o a client.
TEST_P(WebAudioSourceProviderImplTest, SinkMethods) {
  ExpectUnhealthySinkToStop();
  wasp_impl_->Initialize(params_, &fake_callback_);

  // Without a client all WASP calls should fall through to the underlying sink.
  CallAllSinkMethodsAndVerify(true);

  // With a client no calls should reach the Stop()'d sink.  Also, setClient()
  // should propagate the params provided during Initialize() at call time.
  SetClient(this);
  CallAllSinkMethodsAndVerify(false);

  // Removing the client should cause WASP to revert to the underlying sink.
  EXPECT_CALL(*expected_sink_, SetVolume(kTestVolume));
  SetClient(NULL);
  CallAllSinkMethodsAndVerify(true);
}

// Verify underlying sink state is restored after client removal.
TEST_P(WebAudioSourceProviderImplTest, SinkStateRestored) {
  ExpectUnhealthySinkToStop();
  wasp_impl_->Initialize(params_, &fake_callback_);

  // Verify state set before the client is set propagates back afterward.
  EXPECT_CALL(*expected_sink_, Start());
  wasp_impl_->Start();
  SetClient(this);

  EXPECT_CALL(*expected_sink_, SetVolume(1.0));
  EXPECT_CALL(*expected_sink_, Start());
  SetClient(NULL);

  // Verify state set while the client was attached propagates back afterward.
  SetClient(this);
  wasp_impl_->Play();
  wasp_impl_->SetVolume(kTestVolume);

  EXPECT_CALL(*expected_sink_, SetVolume(kTestVolume));
  EXPECT_CALL(*expected_sink_, Start());
  EXPECT_CALL(*expected_sink_, Play());
  SetClient(NULL);
}

// Test the AudioRendererSink state machine and its effects on provideInput().
TEST_P(WebAudioSourceProviderImplTest, ProvideInput) {
  ExpectUnhealthySinkToStop();
  std::unique_ptr<AudioBus> bus1 = AudioBus::Create(params_);
  std::unique_ptr<AudioBus> bus2 = AudioBus::Create(params_);

  // Point the WebVector into memory owned by |bus1|.
  blink::WebVector<float*> audio_data(static_cast<size_t>(bus1->channels()));
  for (size_t i = 0; i < audio_data.size(); ++i)
    audio_data[i] = bus1->channel(static_cast<int>(i));

  // Verify provideInput() works before Initialize() and returns silence.
  bus1->channel(0)[0] = 1;
  bus2->Zero();
  wasp_impl_->ProvideInput(audio_data, params_.frames_per_buffer());
  ASSERT_TRUE(CompareBusses(bus1.get(), bus2.get()));

  wasp_impl_->Initialize(params_, &fake_callback_);
  SetClient(this);

  // Verify provideInput() is muted prior to Start() and no calls to the render
  // callback have occurred.
  bus1->channel(0)[0] = 1;
  bus2->Zero();
  wasp_impl_->ProvideInput(audio_data, params_.frames_per_buffer());
  ASSERT_TRUE(CompareBusses(bus1.get(), bus2.get()));
  ASSERT_EQ(fake_callback_.last_delay(), base::TimeDelta::Max());

  wasp_impl_->Start();

  // Ditto for Play().
  bus1->channel(0)[0] = 1;
  wasp_impl_->ProvideInput(audio_data, params_.frames_per_buffer());
  ASSERT_TRUE(CompareBusses(bus1.get(), bus2.get()));
  ASSERT_EQ(fake_callback_.last_delay(), base::TimeDelta::Max());

  wasp_impl_->Play();

  // Now we should get real audio data.
  wasp_impl_->ProvideInput(audio_data, params_.frames_per_buffer());
  ASSERT_FALSE(CompareBusses(bus1.get(), bus2.get()));

  // Ensure volume adjustment is working.
  fake_callback_.reset();
  fake_callback_.Render(base::TimeDelta(), base::TimeTicks::Now(), 0,
                        bus2.get());
  bus2->Scale(kTestVolume);

  fake_callback_.reset();
  wasp_impl_->SetVolume(kTestVolume);
  wasp_impl_->ProvideInput(audio_data, params_.frames_per_buffer());
  ASSERT_TRUE(CompareBusses(bus1.get(), bus2.get()));

  // Pause should return to silence.
  wasp_impl_->Pause();
  bus1->channel(0)[0] = 1;
  bus2->Zero();
  wasp_impl_->ProvideInput(audio_data, params_.frames_per_buffer());
  ASSERT_TRUE(CompareBusses(bus1.get(), bus2.get()));

  // Ensure if a renderer properly fill silence for partial Render() calls by
  // configuring the fake callback to return half the data.  After these calls
  // bus1 is full of junk data, and bus2 is partially filled.
  wasp_impl_->SetVolume(1);
  fake_callback_.Render(base::TimeDelta(), base::TimeTicks::Now(), 0,
                        bus1.get());
  fake_callback_.reset();
  fake_callback_.Render(base::TimeDelta(), base::TimeTicks::Now(), 0,
                        bus2.get());
  bus2->ZeroFramesPartial(bus2->frames() / 2,
                          bus2->frames() - bus2->frames() / 2);
  fake_callback_.reset();
  fake_callback_.set_half_fill(true);
  wasp_impl_->Play();

  // Play should return real audio data again, but the last half should be zero.
  wasp_impl_->ProvideInput(audio_data, params_.frames_per_buffer());
  ASSERT_TRUE(CompareBusses(bus1.get(), bus2.get()));

  // Stop() should return silence.
  wasp_impl_->Stop();
  bus1->channel(0)[0] = 1;
  bus2->Zero();
  wasp_impl_->ProvideInput(audio_data, params_.frames_per_buffer());
  ASSERT_TRUE(CompareBusses(bus1.get(), bus2.get()));
}

// Verify CopyAudioCB is called if registered.
TEST_P(WebAudioSourceProviderImplTest, CopyAudioCB) {
  ExpectUnhealthySinkToStop();

  testing::InSequence s;
  wasp_impl_->Initialize(params_, &fake_callback_);
  wasp_impl_->SetCopyAudioCallback(base::Bind(
      &WebAudioSourceProviderImplTest::OnAudioBus, base::Unretained(this)));

  const std::unique_ptr<AudioBus> bus1 = AudioBus::Create(params_);
  EXPECT_CALL(*this, DoCopyAudioCB(_, 0, params_.sample_rate())).Times(1);
  Render(bus1.get());

  wasp_impl_->ClearCopyAudioCallback();
  EXPECT_CALL(*this, DoCopyAudioCB(_, _, _)).Times(0);
  Render(bus1.get());

  testing::Mock::VerifyAndClear(mock_sink_.get());
}

INSTANTIATE_TEST_CASE_P(
    /* prefix intentionally left blank due to only one parameterization */,
    WebAudioSourceProviderImplTest,
    testing::Values(WaspSinkStatus::WASP_SINK_OK,
                    WaspSinkStatus::WASP_SINK_ERROR,
                    WaspSinkStatus::WASP_SINK_NULL));

}  // namespace media
