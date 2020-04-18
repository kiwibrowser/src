// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <memory>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "remoting/client/audio/audio_player_buffer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const uint32_t kAudioSampleBytes = 4;  // = 2 channels * 2 bytes
const uint32_t kPaddingBytes = 16;

// TODO(garykac): Generate random audio data in the tests rather than having
// a single constant value.
const uint8_t kDefaultBufferData = 0x5A;
const uint8_t kDummyAudioData = 0x8B;

void ConsumeAsyncAudioFrameCallback(const void* samples) {
  // TODO(nicholss): we could register something in the context and the test
  // class to be able to track when and if the test comes back with something.

  // uint8_t* buffer = (uint8_t*)samples;

  // Verify we haven't written beyond the end of the buffer.
  // for (int i = 0; i < kPaddingBytes; i++) {
  //   ASSERT_EQ(kDefaultBufferData, *(buffer + audio_->bytes_per_frame() + i));
  // }
}

std::unique_ptr<remoting::AudioPacket> CreateAudioPacketWithSamplingRate(
    remoting::AudioPacket::SamplingRate rate,
    int samples) {
  std::unique_ptr<remoting::AudioPacket> packet(new remoting::AudioPacket());
  packet->set_encoding(remoting::AudioPacket::ENCODING_RAW);
  packet->set_sampling_rate(rate);
  packet->set_bytes_per_sample(remoting::AudioPacket::BYTES_PER_SAMPLE_2);
  packet->set_channels(remoting::AudioPacket::CHANNELS_STEREO);

  // The data must be a multiple of 4 bytes (channels x bytes_per_sample).
  std::string data;
  data.resize(samples * kAudioSampleBytes, kDummyAudioData);
  packet->add_data(data);

  return packet;
}

std::unique_ptr<remoting::AudioPacket> CreatePacket48kHz(int samples) {
  return CreateAudioPacketWithSamplingRate(
      remoting::AudioPacket::SAMPLING_RATE_48000, samples);
}

}  // namespace

namespace remoting {

class AudioPlayerBufferTest : public ::testing::Test {
 public:
  void AddAudioPacket(std::unique_ptr<AudioPacket> packet) {
    audio_->AddAudioPacket(std::move(packet));
  }

 protected:
  void SetUp() override {
    audio_.reset(new AudioPlayerBuffer());
    buffer_.reset(new char[audio_->bytes_per_frame() + kPaddingBytes]);
  }

  void TearDown() override {}

  void ConsumeAudioFrame() {
    uint8_t* buffer = reinterpret_cast<uint8_t*>(buffer_.get());
    memset(buffer, kDefaultBufferData,
           audio_->bytes_per_frame() + kPaddingBytes);

    audio_->GetAudioFrame(audio_->bytes_per_frame(),
                          reinterpret_cast<void*>(buffer));

    // Verify we haven't written beyond the end of the buffer.
    for (uint32_t i = 0; i < kPaddingBytes; i++) {
      ASSERT_EQ(kDefaultBufferData, *(buffer + audio_->bytes_per_frame() + i));
    }
  }

  void ConsumeAsyncAudioFrame() {
    uint8_t* buffer = reinterpret_cast<uint8_t*>(buffer_.get());
    memset(buffer, kDefaultBufferData,
           audio_->bytes_per_frame() + kPaddingBytes);

    audio_->AsyncGetAudioFrame(audio_->bytes_per_frame(),
                               reinterpret_cast<void*>(buffer),
                               base::Bind(&ConsumeAsyncAudioFrameCallback,
                                          reinterpret_cast<void*>(buffer)));
  }

  // Check that the first |num_bytes| bytes are filled with audio data and
  // the rest of the buffer is zero-filled.
  void CheckAudioFrameBytes(uint32_t num_bytes) {
    uint8_t* buffer = reinterpret_cast<uint8_t*>(buffer_.get());
    uint32_t i = 0;
    for (; i < num_bytes; i++) {
      ASSERT_EQ(kDummyAudioData, *(buffer + i));
    }
    // Rest of audio frame must be filled with '0's.
    for (; i < audio_->bytes_per_frame(); i++) {
      ASSERT_EQ(0, *(buffer + i));
    }
  }

  uint32_t GetNumQueuedSamples() {
    return audio_->queued_bytes_ / kAudioSampleBytes;
  }

  uint32_t GetNumQueuedPackets() {
    return static_cast<int>(audio_->queued_packets_.size());
  }

  uint32_t GetBytesConsumed() {
    return static_cast<int>(audio_->bytes_consumed_);
  }

  uint32_t GetNumQueuedRequests() {
    return static_cast<int>(audio_->queued_requests_.size());
  }

  std::unique_ptr<AudioPlayerBuffer> audio_;
  std::unique_ptr<char[]> buffer_;
};

TEST_F(AudioPlayerBufferTest, Init) {
  ASSERT_EQ((uint32_t)0, GetNumQueuedPackets());

  audio_->AddAudioPacket(CreatePacket48kHz(10));
  ASSERT_EQ((uint32_t)1, GetNumQueuedPackets());
}

TEST_F(AudioPlayerBufferTest, MultipleSamples) {
  audio_->AddAudioPacket(CreatePacket48kHz(10));
  ASSERT_EQ((uint32_t)10, GetNumQueuedSamples());
  ASSERT_EQ((uint32_t)1, GetNumQueuedPackets());

  audio_->AddAudioPacket(CreatePacket48kHz(20));
  ASSERT_EQ((uint32_t)30, GetNumQueuedSamples());
  ASSERT_EQ((uint32_t)2, GetNumQueuedPackets());
}

TEST_F(AudioPlayerBufferTest, ExceedLatency) {
  // Push about 4 seconds worth of samples.
  for (uint32_t i = 0; i < 100; ++i) {
    audio_->AddAudioPacket(CreatePacket48kHz(2000));
  }

  // Verify that we don't have more than 0.5s.
  EXPECT_LT(GetNumQueuedSamples(), (uint32_t)24000);
}

// Incoming packets: 2 frames worth
// Consume: 1 frame
TEST_F(AudioPlayerBufferTest, ConsumePartialPacket) {
  uint32_t total_samples = 0;
  uint32_t bytes_consumed = 0;

  // Process 2x samples.
  total_samples += audio_->samples_per_frame() * 2;
  audio_->AddAudioPacket(CreatePacket48kHz(audio_->samples_per_frame() * 2));

  ASSERT_EQ(total_samples, GetNumQueuedSamples());
  ASSERT_EQ((uint32_t)1, GetNumQueuedPackets());
  ASSERT_EQ(bytes_consumed, GetBytesConsumed());

  // Consume one frame of samples.
  ConsumeAudioFrame();
  total_samples -= audio_->samples_per_frame();
  bytes_consumed += audio_->bytes_per_frame();
  ASSERT_EQ(total_samples, GetNumQueuedSamples());
  ASSERT_EQ((uint32_t)1, GetNumQueuedPackets());
  ASSERT_EQ(bytes_consumed, GetBytesConsumed());
  CheckAudioFrameBytes(audio_->bytes_per_frame());

  // Remaining samples.
  ASSERT_EQ(audio_->samples_per_frame(), total_samples);
  ASSERT_EQ(audio_->samples_per_frame() * kAudioSampleBytes, bytes_consumed);

  // Consume remaining packet.
  ConsumeAudioFrame();
  total_samples -= audio_->samples_per_frame();
  bytes_consumed += audio_->bytes_per_frame();
  ASSERT_EQ((uint32_t)0, GetNumQueuedSamples());
}

TEST_F(AudioPlayerBufferTest, SingleAsyncRequest) {
  // Create a pending Audio Request.
  ConsumeAsyncAudioFrame();
  ASSERT_EQ((uint32_t)1, GetNumQueuedRequests());
}

TEST_F(AudioPlayerBufferTest, TwoAsyncRequest) {
  // Create a pending Audio Request.
  ConsumeAsyncAudioFrame();
  ConsumeAsyncAudioFrame();
  ASSERT_EQ((uint32_t)2, GetNumQueuedRequests());

  // Add just enough samples to fulfill one request.
  audio_->AddAudioPacket(CreatePacket48kHz(audio_->samples_per_frame()));

  ASSERT_EQ((uint32_t)1, GetNumQueuedRequests());
}

TEST_F(AudioPlayerBufferTest, TwoPartFrameAsyncRequest) {
  // Create a pending Audio Request.
  audio_->AddAudioPacket(CreatePacket48kHz(audio_->samples_per_frame() / 2));

  ConsumeAsyncAudioFrame();
  ASSERT_EQ((uint32_t)1, GetNumQueuedRequests());

  // Add just enough samples to fulfill one request.
  audio_->AddAudioPacket(CreatePacket48kHz(audio_->samples_per_frame() / 2));

  ASSERT_EQ((uint32_t)0, GetNumQueuedSamples());
  ASSERT_EQ((uint32_t)0, GetNumQueuedRequests());
}

}  // namespace remoting
