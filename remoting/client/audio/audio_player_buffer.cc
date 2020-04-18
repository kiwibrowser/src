// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/audio/audio_player_buffer.h"

#include <algorithm>
#include <string>

#include "base/logging.h"
#include "base/stl_util.h"

namespace {

// If queue grows bigger than this we start dropping packets.
const int kMaxQueueLatencyMs = 150;
const int kFrameSizeMs = 10;
const int kMaxQueueLatencyFrames = kMaxQueueLatencyMs / kFrameSizeMs;

};  // namespace

namespace remoting {

struct AudioPlayerBuffer::AudioFrameRequest {
  const size_t bytes_needed_;
  const void* samples_;
  base::Closure callback_;
  size_t bytes_extracted_;

  AudioFrameRequest(const size_t bytes_needed,
                    const void* samples,
                    const base::Closure& callback);
  ~AudioFrameRequest() = default;
};

AudioPlayerBuffer::AudioFrameRequest::AudioFrameRequest(
    const size_t bytes_needed,
    const void* samples,
    const base::Closure& callback)
    : bytes_needed_(bytes_needed),
      samples_(samples),
      callback_(callback),
      bytes_extracted_(0) {}

AudioPlayerBuffer::AudioPlayerBuffer()
    : queued_bytes_(0), bytes_consumed_(0), weak_factory_(this) {
  DETACH_FROM_THREAD(thread_checker_);
}

AudioPlayerBuffer::~AudioPlayerBuffer() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  ResetQueue();
}

void AudioPlayerBuffer::AddAudioPacket(std::unique_ptr<AudioPacket> packet) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  CHECK_EQ(1, packet->data_size());
  DCHECK_EQ(AudioPacket::ENCODING_RAW, packet->encoding());
  DCHECK_NE(AudioPacket::SAMPLING_RATE_INVALID, packet->sampling_rate());
  DCHECK_EQ(buffered_sampling_rate(), packet->sampling_rate());
  DCHECK_EQ(buffered_byes_per_sample(),
            static_cast<int>(packet->bytes_per_sample()));
  DCHECK_EQ(buffered_channels(), static_cast<int>(packet->channels()));
  DCHECK_EQ(packet->data(0).size() %
                (buffered_channels() * buffered_byes_per_sample()),
            0u);

  // Push the new data to the back of the queue.
  queued_bytes_ += packet->data(0).size();
  queued_packets_.push_back(std::move(packet));

  // TODO(nicholss): MIGHT WANT TO JUST CALCULATE THIS ONCE.
  int max_buffer_size_ = kMaxQueueLatencyFrames * bytes_per_frame();

  // Trim off the front of the buffer if we have enqueued too many packets.
  while (queued_bytes_ > max_buffer_size_) {
    queued_bytes_ -= queued_packets_.front()->data(0).size() - bytes_consumed_;
    DCHECK_GE(queued_bytes_, 0);
    queued_packets_.pop_front();
    bytes_consumed_ = 0;
  }

  // Attempt to process a FrameRequest now we have changed queued packets.
  ProcessFrameRequestQueue();
}

void AudioPlayerBuffer::Stop() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  ResetQueue();
}

void AudioPlayerBuffer::AsyncGetAudioFrame(uint32_t buffer_size,
                                           void* buffer,
                                           const base::Closure& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Create an AudioFrameRequest and enqueue it into the buffer.
  CHECK_EQ(buffer_size % bytes_per_frame(), 0u);
  std::unique_ptr<AudioFrameRequest> audioFrameRequest(
      new AudioFrameRequest(buffer_size, buffer, callback));
  queued_requests_.push_back(std::move(audioFrameRequest));
  ProcessFrameRequestQueue();
  return;
}

void AudioPlayerBuffer::ResetQueue() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  queued_packets_.clear();
  queued_bytes_ = 0;
  bytes_consumed_ = 0;
  queued_requests_.clear();
}

uint32_t AudioPlayerBuffer::GetAudioFrame(uint32_t buffer_size, void* buffer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  const size_t bytes_needed = bytes_per_frame();
  // Make sure we don't overrun the buffer.
  CHECK_EQ(buffer_size, bytes_needed);

  char* next_frame = static_cast<char*>(buffer);
  size_t bytes_extracted = 0;

  while (bytes_extracted < bytes_needed) {
    // Check if we've run out of samples for this packet.
    if (queued_packets_.empty()) {
      memset(next_frame, 0, bytes_needed - bytes_extracted);
      return 0;
    }

    // Pop off the packet if we've already consumed all its bytes.
    if (queued_packets_.front()->data(0).size() == bytes_consumed_) {
      queued_packets_.pop_front();
      bytes_consumed_ = 0;
      continue;
    }

    const std::string& packet_data = queued_packets_.front()->data(0);
    size_t bytes_to_copy = std::min(packet_data.size() - bytes_consumed_,
                                    bytes_needed - bytes_extracted);
    memcpy(next_frame, packet_data.data() + bytes_consumed_, bytes_to_copy);

    next_frame += bytes_to_copy;
    bytes_consumed_ += bytes_to_copy;
    bytes_extracted += bytes_to_copy;
    queued_bytes_ -= bytes_to_copy;
    DCHECK_GE(queued_bytes_, 0);
  }
  return bytes_extracted;
}

// This is called when the Frame Request Queue or the Sample Queue is changed.
void AudioPlayerBuffer::ProcessFrameRequestQueue() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Get the active request if there is one.
  while (!queued_requests_.empty() && !queued_packets_.empty()) {
    AudioFrameRequest* activeRequest = queued_requests_.front().get();

    // Copy any available data into the active request up to as much requested
    while (activeRequest->bytes_extracted_ < activeRequest->bytes_needed_ &&
           !queued_packets_.empty()) {
      char* next_frame = (char*)(activeRequest->samples_);

      const std::string& packet_data = queued_packets_.front()->data(0);
      size_t bytes_to_copy = std::min(
          packet_data.size() - bytes_consumed_,
          activeRequest->bytes_needed_ - activeRequest->bytes_extracted_);

      memcpy(next_frame, packet_data.data() + bytes_consumed_, bytes_to_copy);

      bytes_consumed_ += bytes_to_copy;
      activeRequest->bytes_extracted_ += bytes_to_copy;
      queued_bytes_ -= bytes_to_copy;
      DCHECK_GE(queued_bytes_, 0);
      // Pop off the packet if we've already consumed all its bytes.
      if (queued_packets_.front()->data(0).size() == bytes_consumed_) {
        queued_packets_.pop_front();
        bytes_consumed_ = 0;
      }
    }

    // If this request is fulfilled, call the callback and pop it off the queue.
    if (activeRequest->bytes_extracted_ == activeRequest->bytes_needed_) {
      activeRequest->callback_.Run();
      queued_requests_.pop_front();
      activeRequest = nullptr;
    }
  }
}

uint32_t AudioPlayerBuffer::samples_per_frame() const {
  return (buffered_sampling_rate() * kFrameSizeMs) /
         base::Time::kMillisecondsPerSecond;
}

AudioPacket::SamplingRate AudioPlayerBuffer::buffered_sampling_rate() const {
  return AudioPacket::SAMPLING_RATE_48000;
}

AudioPacket::Channels AudioPlayerBuffer::buffered_channels() const {
  return AudioPacket::CHANNELS_STEREO;
}

AudioPacket::BytesPerSample AudioPlayerBuffer::buffered_byes_per_sample()
    const {
  return AudioPacket::BYTES_PER_SAMPLE_2;
}

uint32_t AudioPlayerBuffer::bytes_per_frame() const {
  return buffered_channels() * buffered_byes_per_sample() * samples_per_frame();
}

base::WeakPtr<AudioStreamConsumer>
AudioPlayerBuffer::AudioStreamConsumerAsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace remoting
