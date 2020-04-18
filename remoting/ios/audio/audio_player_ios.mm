// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "remoting/base/auto_thread.h"
#include "remoting/client/audio/audio_player_buffer.h"
#include "remoting/ios/audio/audio_player_ios.h"
#include "remoting/ios/audio/audio_stream_consumer_proxy.h"

namespace remoting {

std::unique_ptr<AudioPlayerIos> AudioPlayerIos::CreateAudioPlayer(
    scoped_refptr<AutoThreadTaskRunner> audio_thread_runner) {
  AudioPlayerBuffer* buffer = new AudioPlayerBuffer();

  std::unique_ptr<AsyncAudioFrameSupplier> supplier =
      (std::unique_ptr<AsyncAudioFrameSupplier>)base::WrapUnique(buffer);

  std::unique_ptr<AudioStreamConsumer> consumer_proxy =
      AudioStreamConsumerProxy::Create(audio_thread_runner,
                                       buffer->AudioStreamConsumerAsWeakPtr());

  return std::make_unique<AudioPlayerIos>(
      std::move(consumer_proxy), std::move(supplier), audio_thread_runner);
}

// public

AudioPlayerIos::AudioPlayerIos(
    std::unique_ptr<AudioStreamConsumer> audio_stream_consumer,
    std::unique_ptr<AsyncAudioFrameSupplier> audio_frame_supplier,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner)
    : state_(UNALLOCATED),
      enqueued_frames_count_(0),
      audio_task_runner_(audio_task_runner),
      weak_factory_(this) {
  audio_stream_consumer_ = std::move(audio_stream_consumer);
  audio_frame_supplier_ = std::move(audio_frame_supplier);
}

AudioPlayerIos::~AudioPlayerIos() {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());
  DCHECK(!audio_stream_consumer_)
      << "Invalidate() must be called once on the UI thread.";
  StopOnAudioThread();
  // Disposing of an audio queue also disposes of its resources, including its
  // buffers.
  AudioQueueDispose(output_queue_, true /* Immediate */);
  for (uint32_t i = 0; i < kOutputBuffers; i++) {
    output_buffers_[i] = nullptr;
  }
}

void AudioPlayerIos::Invalidate() {
  audio_stream_consumer_.release();
}

base::WeakPtr<AudioStreamConsumer> AudioPlayerIos::GetAudioStreamConsumer() {
  return audio_stream_consumer_->AudioStreamConsumerAsWeakPtr();
}

void AudioPlayerIos::Start() {
  audio_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&AudioPlayerIos::StartOnAudioThread,
                                          weak_factory_.GetWeakPtr()));
}

void AudioPlayerIos::Stop() {
  audio_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&AudioPlayerIos::StopOnAudioThread,
                                          weak_factory_.GetWeakPtr()));
}

void AudioPlayerIos::Prime(AudioQueueBufferRef output_buffer,
                           bool was_dequeued) {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());
  if (state_ == STOPPING) {
    return;
  }
  if (was_dequeued) {
    --enqueued_frames_count_;
  }

  audio_frame_supplier_->AsyncGetAudioFrame(
      audio_frame_supplier_->bytes_per_frame(),
      reinterpret_cast<void*>(output_buffer->mAudioData),
      base::Bind(&AudioPlayerIos::AsyncGetAudioFrameCallback,
                 weak_factory_.GetWeakPtr(),
                 reinterpret_cast<void*>(output_buffer),
                 reinterpret_cast<void*>(output_buffer->mAudioData)));

  if (state_ == PLAYING && enqueued_frames_count_ == 0) {
    state_ = UNDERRUN;
    StopOnAudioThread();
    // We have a bunch of pending requests so we are not stopped, but really in
    // PRIMING. Avoid re-priming the queue.
    state_ = PRIMING;
    priming_frames_needed_count_ = kOutputBuffers;
  }
}

void AudioPlayerIos::Pump(AudioQueueBufferRef output_buffer) {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());
  OSStatus err = AudioQueueEnqueueBuffer(output_queue_, output_buffer, 0, nil);
  if (err) {
    LOG(FATAL) << "AudioQueueEnqueueBuffer: " << err;
  } else {
    ++enqueued_frames_count_;
  }

  if (state_ == PRIMING) {
    --priming_frames_needed_count_;
    if (priming_frames_needed_count_ == 0) {
      audio_task_runner_->PostTask(FROM_HERE,
                                   base::Bind(&AudioPlayerIos::StartPlayback,
                                              weak_factory_.GetWeakPtr()));
    }
  }
}

// private

// static, AudioQueue output queue callback.
void AudioPlayerIos::AudioEngineOutputBufferCallback(
    void* instance,
    AudioQueueRef outAQ,
    AudioQueueBufferRef samples) {
  AudioPlayerIos* player = (AudioPlayerIos*)instance;
  player->Prime(samples, true);
}

void AudioPlayerIos::StartOnAudioThread() {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());
  switch (state_) {
    case STOPPING:
      // Nothing to do.
      return;
    case UNALLOCATED:
      GenerateOutputQueue((void*)this);
      GenerateOutputBuffers(audio_frame_supplier_->bytes_per_frame());
      state_ = STOPPED;
      FALLTHROUGH;
    case STOPPED:
      PrimeQueue();
      return;
    case PRIMING:
      // Nothing to do.
      return;
    case PLAYING:
      // Nothing to do.
      return;
    case UNDERRUN:
      // Nothing to do.
      return;
    default:
      LOG(FATAL) << "Audio Player: Unknown State.";
  }
}

void AudioPlayerIos::StopOnAudioThread() {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());
  state_ = STOPPING;
  if (output_queue_) {
    AudioQueueStop(output_queue_, YES);
  }
  state_ = STOPPED;
}

// Should only be called while |state_| is STOPPED
void AudioPlayerIos::PrimeQueue() {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());
  if (state_ != STOPPED) {
    return;
  }
  state_ = PRIMING;
  priming_frames_needed_count_ = kOutputBuffers;
  for (uint32_t i = 0; i < kOutputBuffers; i++) {
    Prime(output_buffers_[i], false);
  }
}

void AudioPlayerIos::StartPlayback() {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());
  OSStatus err;
  err = AudioQueueStart(output_queue_, nil);
  if (err) {
    LOG(FATAL) << "AudioQueueStart: " << err;
    state_ = UNKNOWN;
    return;
  }
  state_ = PLAYING;
  return;
}

void AudioPlayerIos::AsyncGetAudioFrameCallback(const void* context,
                                                const void* samples) {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());
  AudioQueueBufferRef output_buffer = (AudioQueueBufferRef)context;
  output_buffer->mAudioDataByteSize = audio_frame_supplier_->bytes_per_frame();
  Pump(output_buffer);
}

bool AudioPlayerIos::GenerateOutputBuffers(uint32_t bytesPerFrame) {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());
  OSStatus err;
  for (uint32_t i = 0; i < kOutputBuffers; i++) {
    err = AudioQueueAllocateBuffer(output_queue_, bytesPerFrame,
                                   &output_buffers_[i]);
    if (err) {
      LOG(FATAL) << "AudioQueueAllocateBuffer[" << i << "] : " << err;
      return false;
    }
  }
  return true;
}

bool AudioPlayerIos::GenerateOutputQueue(void* context) {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());
  // Set up stream format fields
  AudioStreamBasicDescription streamFormat = GenerateStreamFormat();
  OSStatus err;
  err = AudioQueueNewOutput(&streamFormat, AudioEngineOutputBufferCallback,
                            context, CFRunLoopGetCurrent(),
                            kCFRunLoopCommonModes, 0, &output_queue_);
  if (err) {
    LOG(FATAL) << "AudioQueueNewOutput: " << err;
    return false;
  }
  return true;
}

AudioStreamBasicDescription AudioPlayerIos::GenerateStreamFormat() {
  // Set up stream format fields
  // TODO(nicholss): does this all needs to be generated dynamicly?
  AudioStreamBasicDescription streamFormat;
  streamFormat.mSampleRate = 48000;
  streamFormat.mFormatID = kAudioFormatLinearPCM;
  streamFormat.mFormatFlags =
      kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
  streamFormat.mBitsPerChannel = 16;
  streamFormat.mChannelsPerFrame = 2;
  streamFormat.mBytesPerPacket = 2 * streamFormat.mChannelsPerFrame;
  streamFormat.mBytesPerFrame = 2 * streamFormat.mChannelsPerFrame;
  streamFormat.mFramesPerPacket = 1;
  streamFormat.mReserved = 0;
  return streamFormat;
}

}  // namespace remoting
