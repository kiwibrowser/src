// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_AUDIO_AUDIO_PLAYER_IOS_H_
#define REMOTING_IOS_AUDIO_AUDIO_PLAYER_IOS_H_

#import <AudioToolbox/AudioToolbox.h>

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "remoting/base/auto_thread.h"
#include "remoting/client/audio/async_audio_frame_supplier.h"
#include "remoting/client/audio/audio_stream_consumer.h"

namespace remoting {

/*
  For iOS, the audio subsystem uses a multi-buffer queue to produce smooth
  audio playback. To allow this to work with remoting, we need to add a buffer
  that is capable of enqueuing exactly the requested amount of data
  asynchronously then calling us back and we will push it into the iOS audio
  queue.
*/
class AudioPlayerIos {
 public:
  static std::unique_ptr<AudioPlayerIos> CreateAudioPlayer(
      scoped_refptr<AutoThreadTaskRunner> audio_thread_runner);

  AudioPlayerIos(std::unique_ptr<AudioStreamConsumer> audio_stream_consumer,
                 std::unique_ptr<AsyncAudioFrameSupplier> audio_frame_supplier,
                 scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner);

  // Call on the audio thread.
  ~AudioPlayerIos();

  // Must be called once on the UI thread before destroying the object on
  // audio thread.
  // TODO(yuweih): This is a dirty fix for thread safety issue. The cleaner fix
  // could be turn AudioPlayerIos into shell-core model then invalidate UI
  // resources in the shell.
  void Invalidate();

  base::WeakPtr<AudioStreamConsumer> GetAudioStreamConsumer();
  void Start();
  void Stop();

 private:
  static const uint32_t kOutputBuffers = 8;

  enum AudioPlayerState {
    UNALLOCATED,
    STOPPING,
    STOPPED,
    PRIMING,
    PLAYING,
    UNDERRUN,
    UNKNOWN
  };

  static void AudioEngineOutputBufferCallback(void* instance,
                                              AudioQueueRef outAQ,
                                              AudioQueueBufferRef samples);

  // Audio Thread versions of public functions.
  void StartOnAudioThread();
  void StopOnAudioThread();

  // Audio Thread private functions.
  void Prime(AudioQueueBufferRef output_buffer, bool was_dequeued);
  void Pump(AudioQueueBufferRef output_buffer);
  void PrimeQueue();
  void StartPlayback();
  void AsyncGetAudioFrameCallback(const void* context, const void* samples);
  bool GenerateOutputBuffers(uint32_t bytesPerFrame);
  bool GenerateOutputQueue(void* context);
  AudioStreamBasicDescription GenerateStreamFormat();

  AudioPlayerState state_;
  uint32_t priming_frames_needed_count_;
  AudioQueueRef output_queue_;
  AudioQueueBufferRef output_buffers_[kOutputBuffers];
  std::unique_ptr<AudioStreamConsumer> audio_stream_consumer_;
  std::unique_ptr<AsyncAudioFrameSupplier> audio_frame_supplier_;
  uint32_t enqueued_frames_count_;
  // Task runner on which the |audio_frame_supplier_| is running.
  scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner_;

  base::WeakPtrFactory<AudioPlayerIos> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AudioPlayerIos);
};

}  // namespace remoting

#endif  // REMOTING_IOS_AUDIO_AUDIO_PLAYER_IOS_H_
