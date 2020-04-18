// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/bind_helpers.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/sys_byteorder.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/speech/proto/google_streaming_api.pb.h"
#include "content/browser/speech/speech_recognition_engine.h"
#include "content/browser/speech/speech_recognizer_impl.h"
#include "content/public/browser/speech_recognition_event_listener.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_system_impl.h"
#include "media/audio/fake_audio_input_stream.h"
#include "media/audio/fake_audio_output_stream.h"
#include "media/audio/mock_audio_manager.h"
#include "media/audio/test_audio_thread.h"
#include "media/base/audio_bus.h"
#include "media/base/test_helpers.h"
#include "net/base/net_errors.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "testing/gtest/include/gtest/gtest.h"

using media::AudioInputStream;
using media::AudioOutputStream;
using media::AudioParameters;

namespace content {

namespace {

class MockCapturerSource : public media::AudioCapturerSource {
 public:
  MockCapturerSource() = default;
  MOCK_METHOD2(Initialize,
               void(const media::AudioParameters& params,
                    CaptureCallback* callback));
  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD1(SetAutomaticGainControl, void(bool enable));
  MOCK_METHOD1(SetVolume, void(double volume));
  MOCK_METHOD1(SetOutputDeviceForAec,
               void(const std::string& output_device_id));

 protected:
  ~MockCapturerSource() override = default;
};

}  // namespace

class SpeechRecognizerImplTest : public SpeechRecognitionEventListener,
                                 public testing::Test {
 public:
  SpeechRecognizerImplTest()
      : audio_capturer_source_(new testing::NiceMock<MockCapturerSource>()),
        recognition_started_(false),
        recognition_ended_(false),
        result_received_(false),
        audio_started_(false),
        audio_ended_(false),
        sound_started_(false),
        sound_ended_(false),
        error_(SPEECH_RECOGNITION_ERROR_NONE),
        volume_(-1.0f) {
    // SpeechRecognizer takes ownership of sr_engine.
    SpeechRecognitionEngine* sr_engine =
        new SpeechRecognitionEngine(nullptr /* URLRequestContextGetter */);
    SpeechRecognitionEngine::Config config;
    config.audio_num_bits_per_sample =
        SpeechRecognizerImpl::kNumBitsPerAudioSample;
    config.audio_sample_rate = SpeechRecognizerImpl::kAudioSampleRate;
    config.filter_profanities = false;
    sr_engine->SetConfig(config);

    const int kTestingSessionId = 1;

    audio_manager_.reset(new media::MockAudioManager(
        std::make_unique<media::TestAudioThread>(true)));
    audio_manager_->SetInputStreamParameters(
        media::AudioParameters::UnavailableDeviceParams());
    audio_system_ =
        std::make_unique<media::AudioSystemImpl>(audio_manager_.get());
    SpeechRecognizerImpl::SetAudioEnvironmentForTesting(
        audio_system_.get(), audio_capturer_source_.get());
    recognizer_ = new SpeechRecognizerImpl(
        this, audio_system_.get(), kTestingSessionId, false, false, sr_engine);

    int audio_packet_length_bytes =
        (SpeechRecognizerImpl::kAudioSampleRate *
         SpeechRecognitionEngine::kAudioPacketIntervalMs *
         ChannelLayoutToChannelCount(SpeechRecognizerImpl::kChannelLayout) *
         SpeechRecognizerImpl::kNumBitsPerAudioSample) / (8 * 1000);
    audio_packet_.resize(audio_packet_length_bytes);

    const int channels =
        ChannelLayoutToChannelCount(SpeechRecognizerImpl::kChannelLayout);
    bytes_per_sample_ = SpeechRecognizerImpl::kNumBitsPerAudioSample / 8;
    const int frames = audio_packet_length_bytes / channels / bytes_per_sample_;
    audio_bus_ = media::AudioBus::Create(channels, frames);
    audio_bus_->Zero();
  }

  ~SpeechRecognizerImplTest() override {
    SpeechRecognizerImpl::SetAudioEnvironmentForTesting(nullptr, nullptr);
    audio_manager_->Shutdown();
  }

  void CheckEventsConsistency() {
    // Note: "!x || y" == "x implies y".
    EXPECT_TRUE(!recognition_ended_ || recognition_started_);
    EXPECT_TRUE(!audio_ended_ || audio_started_);
    EXPECT_TRUE(!sound_ended_ || sound_started_);
    EXPECT_TRUE(!audio_started_ || recognition_started_);
    EXPECT_TRUE(!sound_started_ || audio_started_);
    EXPECT_TRUE(!audio_ended_ || (sound_ended_ || !sound_started_));
    EXPECT_TRUE(!recognition_ended_ || (audio_ended_ || !audio_started_));
  }

  void CheckFinalEventsConsistency() {
    // Note: "!(x ^ y)" == "(x && y) || (!x && !x)".
    EXPECT_FALSE(recognition_started_ ^ recognition_ended_);
    EXPECT_FALSE(audio_started_ ^ audio_ended_);
    EXPECT_FALSE(sound_started_ ^ sound_ended_);
  }

  // Overridden from SpeechRecognitionEventListener:
  void OnAudioStart(int session_id) override {
    audio_started_ = true;
    CheckEventsConsistency();
  }

  void OnAudioEnd(int session_id) override {
    audio_ended_ = true;
    CheckEventsConsistency();
  }

  void OnRecognitionResults(int session_id,
                            const SpeechRecognitionResults& results) override {
    result_received_ = true;
  }

  void OnRecognitionError(int session_id,
                          const SpeechRecognitionError& error) override {
    EXPECT_TRUE(recognition_started_);
    EXPECT_FALSE(recognition_ended_);
    error_ = error.code;
  }

  void OnAudioLevelsChange(int session_id,
                           float volume,
                           float noise_volume) override {
    volume_ = volume;
    noise_volume_ = noise_volume;
  }

  void OnRecognitionEnd(int session_id) override {
    recognition_ended_ = true;
    CheckEventsConsistency();
  }

  void OnRecognitionStart(int session_id) override {
    recognition_started_ = true;
    CheckEventsConsistency();
  }

  void OnEnvironmentEstimationComplete(int session_id) override {}

  void OnSoundStart(int session_id) override {
    sound_started_ = true;
    CheckEventsConsistency();
  }

  void OnSoundEnd(int session_id) override {
    sound_ended_ = true;
    CheckEventsConsistency();
  }

  void CopyPacketToAudioBus() {
    // Copy the created signal into an audio bus in a deinterleaved format.
    audio_bus_->FromInterleaved(
        &audio_packet_[0], audio_bus_->frames(), bytes_per_sample_);
  }

  void FillPacketWithTestWaveform() {
    // Fill the input with a simple pattern, a 125Hz sawtooth waveform.
    for (size_t i = 0; i < audio_packet_.size(); ++i)
      audio_packet_[i] = static_cast<uint8_t>(i);
    CopyPacketToAudioBus();
  }

  void FillPacketWithNoise() {
    int value = 0;
    int factor = 175;
    for (size_t i = 0; i < audio_packet_.size(); ++i) {
      value += factor;
      audio_packet_[i] = value % 100;
    }
    CopyPacketToAudioBus();
  }

  void Capture(media::AudioBus* data) {
    auto* capture_callback =
        static_cast<media::AudioCapturerSource::CaptureCallback*>(
            recognizer_.get());
    capture_callback->Capture(data, 0, 0.0, false);
  }

  void OnCaptureError() {
    auto* capture_callback =
        static_cast<media::AudioCapturerSource::CaptureCallback*>(
            recognizer_.get());
    capture_callback->OnCaptureError("");
  }

  void WaitForAudioThreadToPostDeviceInfo() {
    media::WaitableMessageLoopEvent event;
    audio_manager_->GetTaskRunner()->PostTaskAndReply(
        FROM_HERE, base::DoNothing(), event.GetClosure());
    // Runs the loop and waits for the audio thread to call event's closure,
    // which means AudioSystem reply containing device parameters is already
    // queued on the main thread.
    event.RunAndWait();
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  scoped_refptr<SpeechRecognizerImpl> recognizer_;
  std::unique_ptr<media::MockAudioManager> audio_manager_;
  std::unique_ptr<media::AudioSystem> audio_system_;
  scoped_refptr<MockCapturerSource> audio_capturer_source_;
  bool recognition_started_;
  bool recognition_ended_;
  bool result_received_;
  bool audio_started_;
  bool audio_ended_;
  bool sound_started_;
  bool sound_ended_;
  SpeechRecognitionErrorCode error_;
  net::TestURLFetcherFactory url_fetcher_factory_;
  std::vector<uint8_t> audio_packet_;
  std::unique_ptr<media::AudioBus> audio_bus_;
  int bytes_per_sample_;
  float volume_;
  float noise_volume_;
};

TEST_F(SpeechRecognizerImplTest, StartNoInputDevices) {
  // Check for callbacks when stopping record before any audio gets recorded.
  audio_manager_->SetHasInputDevices(false);
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();  // EVENT_START processing.
  EXPECT_TRUE(recognition_started_);
  EXPECT_FALSE(audio_started_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_AUDIO_CAPTURE, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, StopBeforeDeviceInfoReceived) {
  // Check for callbacks when stopping record before reply is received from
  // AudioSystem.
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  // Block audio thread.
  audio_manager_->GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&base::WaitableEvent::Wait, base::Unretained(&event)));

  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  recognizer_->StopAudioCapture();
  base::RunLoop().RunUntilIdle();

  // Release audio thread and receive a callback from it.
  event.Signal();
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(recognition_started_);
  EXPECT_FALSE(audio_started_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, CancelBeforeDeviceInfoReceived) {
  // Check for callbacks when stopping record before reply is received from
  // AudioSystem.
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);

  // Block audio thread.
  audio_manager_->GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&base::WaitableEvent::Wait, base::Unretained(&event)));

  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  recognizer_->AbortRecognition();
  base::RunLoop().RunUntilIdle();

  // Release audio thread and receive a callback from it.
  event.Signal();
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(recognition_started_);
  EXPECT_FALSE(audio_started_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, StopNoData) {
  // Check for callbacks when stopping record before any audio gets recorded.
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  recognizer_->StopAudioCapture();
  base::RunLoop().RunUntilIdle();  // EVENT_START and EVENT_STOP processing.
  EXPECT_TRUE(recognition_started_);
  EXPECT_FALSE(audio_started_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, CancelNoData) {
  // Check for callbacks when canceling recognition before any audio gets
  // recorded.
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  recognizer_->AbortRecognition();
  base::RunLoop().RunUntilIdle();  // EVENT_START and EVENT_ABORT processing.
  EXPECT_TRUE(recognition_started_);
  EXPECT_FALSE(audio_started_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_ABORTED, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, StopWithData) {
  // Start recording, give some data and then stop. This should wait for the
  // network callback to arrive before completion.
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();  // EVENT_START processing.

  // Try sending 5 chunks of mock audio data and verify that each of them
  // resulted immediately in a packet sent out via the network. This verifies
  // that we are streaming out encoded data as chunks without waiting for the
  // full recording to complete.
  const size_t kNumChunks = 5;
  for (size_t i = 0; i < kNumChunks; ++i) {
    Capture(audio_bus_.get());
    base::RunLoop().RunUntilIdle();
    net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
    ASSERT_TRUE(fetcher);
    EXPECT_EQ(i + 1, fetcher->upload_chunks().size());
  }

  recognizer_->StopAudioCapture();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(audio_started_);
  EXPECT_TRUE(audio_ended_);
  EXPECT_FALSE(recognition_ended_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);

  // Create a response string.
  proto::SpeechRecognitionEvent proto_event;
  proto_event.set_status(proto::SpeechRecognitionEvent::STATUS_SUCCESS);
  proto::SpeechRecognitionResult* proto_result = proto_event.add_result();
  proto_result->set_final(true);
  proto::SpeechRecognitionAlternative* proto_alternative =
      proto_result->add_alternative();
  proto_alternative->set_confidence(0.5f);
  proto_alternative->set_transcript("123");
  std::string msg_string;
  proto_event.SerializeToString(&msg_string);
  uint32_t prefix =
      base::HostToNet32(base::checked_cast<uint32_t>(msg_string.size()));
  msg_string.insert(0, reinterpret_cast<char*>(&prefix), sizeof(prefix));

  // Issue the network callback to complete the process.
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(
      SpeechRecognitionEngine::kDownstreamUrlFetcherIdForTesting);
  ASSERT_TRUE(fetcher);
  fetcher->set_url(fetcher->GetOriginalURL());
  fetcher->set_status(net::URLRequestStatus());
  fetcher->set_response_code(200);
  fetcher->SetResponseString(msg_string);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(recognition_ended_);
  EXPECT_TRUE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, CancelWithData) {
  // Start recording, give some data and then cancel.
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();  // EVENT_START processing.
  Capture(audio_bus_.get());
  base::RunLoop().RunUntilIdle();
  recognizer_->AbortRecognition();
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(url_fetcher_factory_.GetFetcherByID(0));
  EXPECT_TRUE(recognition_started_);
  EXPECT_TRUE(audio_started_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_ABORTED, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, ConnectionError) {
  // Start recording, give some data and then stop. Issue the network callback
  // with a connection error and verify that the recognizer bubbles the error up
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();  // EVENT_START processing.
  Capture(audio_bus_.get());
  base::RunLoop().RunUntilIdle();
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  recognizer_->StopAudioCapture();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(audio_started_);
  EXPECT_TRUE(audio_ended_);
  EXPECT_FALSE(recognition_ended_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);

  // Issue the network callback to complete the process.
  fetcher->set_url(fetcher->GetOriginalURL());
  fetcher->set_status(
      net::URLRequestStatus::FromError(net::ERR_CONNECTION_REFUSED));
  fetcher->set_response_code(0);
  fetcher->SetResponseString(std::string());
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(recognition_ended_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NETWORK, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, ServerError) {
  // Start recording, give some data and then stop. Issue the network callback
  // with a 500 error and verify that the recognizer bubbles the error up
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();  // EVENT_START processing.
  Capture(audio_bus_.get());
  base::RunLoop().RunUntilIdle();
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  recognizer_->StopAudioCapture();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(audio_started_);
  EXPECT_TRUE(audio_ended_);
  EXPECT_FALSE(recognition_ended_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);

  // Issue the network callback to complete the process.
  fetcher->set_url(fetcher->GetOriginalURL());
  fetcher->set_status(net::URLRequestStatus());
  fetcher->set_response_code(500);
  fetcher->SetResponseString("Internal Server Error");
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(recognition_ended_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NETWORK, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, OnCaptureError_PropagatesError) {
  // Check if things tear down properly if AudioInputController threw an error.
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();  // EVENT_START processing.

  OnCaptureError();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(recognition_started_);
  EXPECT_FALSE(audio_started_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_AUDIO_CAPTURE, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, NoSpeechCallbackIssued) {
  // Start recording and give a lot of packets with audio samples set to zero.
  // This should trigger the no-speech detector and issue a callback.
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();  // EVENT_START processing.

  int num_packets = (SpeechRecognizerImpl::kNoSpeechTimeoutMs) /
                     SpeechRecognitionEngine::kAudioPacketIntervalMs + 1;
  // The vector is already filled with zero value samples on create.
  for (int i = 0; i < num_packets; ++i) {
    Capture(audio_bus_.get());
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(recognition_started_);
  EXPECT_TRUE(audio_started_);
  EXPECT_FALSE(result_received_);
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NO_SPEECH, error_);
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, NoSpeechCallbackNotIssued) {
  // Start recording and give a lot of packets with audio samples set to zero
  // and then some more with reasonably loud audio samples. This should be
  // treated as normal speech input and the no-speech detector should not get
  // triggered.
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();  // EVENT_START processing.

  int num_packets = (SpeechRecognizerImpl::kNoSpeechTimeoutMs) /
                     SpeechRecognitionEngine::kAudioPacketIntervalMs;

  // The vector is already filled with zero value samples on create.
  for (int i = 0; i < num_packets / 2; ++i) {
    Capture(audio_bus_.get());
  }

  FillPacketWithTestWaveform();
  for (int i = 0; i < num_packets / 2; ++i) {
    Capture(audio_bus_.get());
  }

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  EXPECT_TRUE(audio_started_);
  EXPECT_FALSE(audio_ended_);
  EXPECT_FALSE(recognition_ended_);
  recognizer_->AbortRecognition();
  base::RunLoop().RunUntilIdle();
  CheckFinalEventsConsistency();
}

TEST_F(SpeechRecognizerImplTest, SetInputVolumeCallback) {
  // Start recording and give a lot of packets with audio samples set to zero
  // and then some more with reasonably loud audio samples. Check that we don't
  // get the callback during estimation phase, then get zero for the silence
  // samples and proper volume for the loud audio.
  recognizer_->StartRecognition(
      media::AudioDeviceDescription::kDefaultDeviceId);
  base::RunLoop().RunUntilIdle();  // EVENT_PREPARE processing.
  WaitForAudioThreadToPostDeviceInfo();
  base::RunLoop().RunUntilIdle();  // EVENT_START processing.

  // Feed some samples to begin with for the endpointer to do noise estimation.
  int num_packets = SpeechRecognizerImpl::kEndpointerEstimationTimeMs /
                    SpeechRecognitionEngine::kAudioPacketIntervalMs;
  FillPacketWithNoise();
  for (int i = 0; i < num_packets; ++i) {
    Capture(audio_bus_.get());
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(-1.0f, volume_);  // No audio volume set yet.

  // The vector is already filled with zero value samples on create.
  Capture(audio_bus_.get());
  base::RunLoop().RunUntilIdle();
  EXPECT_FLOAT_EQ(0.74939233f, volume_);

  FillPacketWithTestWaveform();
  Capture(audio_bus_.get());
  base::RunLoop().RunUntilIdle();
  EXPECT_NEAR(0.89926866f, volume_, 0.00001f);
  EXPECT_FLOAT_EQ(0.75071919f, noise_volume_);

  EXPECT_EQ(SPEECH_RECOGNITION_ERROR_NONE, error_);
  EXPECT_FALSE(audio_ended_);
  EXPECT_FALSE(recognition_ended_);
  recognizer_->AbortRecognition();
  base::RunLoop().RunUntilIdle();
  CheckFinalEventsConsistency();
}

}  // namespace content
