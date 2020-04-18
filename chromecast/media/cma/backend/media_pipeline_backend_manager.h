// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_MEDIA_PIPELINE_BACKEND_MANAGER_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_MEDIA_PIPELINE_BACKEND_MANAGER_H_

#include <map>
#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list_threadsafe.h"
#include "base/single_thread_task_runner.h"
#include "base/timer/timer.h"
#include "chromecast/public/media/decoder_config.h"
#include "chromecast/public/media/media_pipeline_device_params.h"

namespace chromecast {
namespace media {

class AudioDecoderWrapper;
enum class AudioContentType;
class CastDecoderBuffer;
class CmaBackend;

// This class tracks all created media backends, tracking whether or not volume
// feedback sounds should be enabled based on the currently active backends.
// Volume feedback sounds are only enabled when there are no active audio
// streams (apart from sound-effects streams).
class MediaPipelineBackendManager {
 public:
  class AllowVolumeFeedbackObserver {
   public:
    virtual void AllowVolumeFeedbackSounds(bool allow) = 0;

   protected:
    virtual ~AllowVolumeFeedbackObserver() = default;
  };

  // Delegate which can process Audio buffers sent to us.
  class BufferDelegate {
   public:
    // Returns |true| if the delegate is accepting buffers.
    virtual bool IsActive() = 0;

    // Called when calls to |OnPushBuffer| will start.
    virtual void OnStreamStarted() = 0;

    // Called when calls to |OnPushBuffer| will stop.
    virtual void OnStreamStopped() = 0;

    // If |IsActive| returns true, the media stream's audio buffers will be sent
    // to the delegate and the media stream's volume will be set to 0.
    //
    // The client may only assume the buffer is in scope during the callback.
    // If the client needs to use the buffer out of scope of the callback (e.g.
    // posted onto a different thread), it must make a copy.
    virtual void OnPushBuffer(const CastDecoderBuffer* buffer) = 0;

    // Called when the media stream's audio config changes. After this call, all
    // subsequent buffers will have the new config. This method will be called
    // regardless if |IsActive| returs true or false.
    virtual void OnSetConfig(const AudioConfig& config) = 0;

    // Called when volume changes. |volume| is from [0.0, 1.0].
    virtual void OnSetVolume(float volume) = 0;

   protected:
    virtual ~BufferDelegate() = default;
  };

  enum DecoderType {
    AUDIO_DECODER,
    VIDEO_DECODER,
    SFX_DECODER,
    NUM_DECODER_TYPES
  };

  explicit MediaPipelineBackendManager(
      scoped_refptr<base::SingleThreadTaskRunner> media_task_runner);
  ~MediaPipelineBackendManager();

  // Creates a media pipeline backend. Must be called on the same thread as
  // |media_task_runner_|.
  std::unique_ptr<CmaBackend> CreateMediaPipelineBackend(
      const MediaPipelineDeviceParams& params);

  base::SingleThreadTaskRunner* task_runner() const {
    return media_task_runner_.get();
  }

  // Adds/removes an observer for when volume feedback sounds are allowed.
  // An observer must be removed on the same thread that added it.
  void AddAllowVolumeFeedbackObserver(AllowVolumeFeedbackObserver* observer);
  void RemoveAllowVolumeFeedbackObserver(AllowVolumeFeedbackObserver* observer);

  // Logically pauses/resumes a backend instance, without actually pausing or
  // resuming it. This is used by multiroom output to avoid playback stutter on
  // resume. |backend| must have been created via a call to this instance's
  // CreateMediaPipelineBackend().
  void LogicalPause(CmaBackend* backend);
  void LogicalResume(CmaBackend* backend);

  // Add/remove a playing audio stream that is not accounted for by a
  // CmaBackend instance (for example, direct audio output using
  // CastMediaShlib::AddDirectAudioSource()). |sfx| indicates whether or not
  // the stream is a sound effects stream (has no effect on volume feedback).
  void AddExtraPlayingStream(bool sfx);
  void RemoveExtraPlayingStream(bool sfx);

  // Sets a global multiplier for output volume for streams of the given |type|.
  // The multiplier may be any value >= 0; if the resulting volume for an
  // individual stream would be > 1.0, that stream's volume is clamped to 1.0.
  // The default multiplier is 1.0. May be called on any thread.
  void SetGlobalVolumeMultiplier(AudioContentType type, float multiplier);

  // |buffer_delegate| will get notified for all buffers on the media stream.
  // |buffer_delegate| must outlive |this|.
  // Can only be set once.
  void SetBufferDelegate(BufferDelegate* buffer_delegate);

  BufferDelegate* buffer_delegate() const { return buffer_delegate_; }

 private:
  friend class MediaPipelineBackendWrapper;
  friend class AudioDecoderWrapper;

  void AddAudioDecoder(AudioDecoderWrapper* decoder);
  void RemoveAudioDecoder(AudioDecoderWrapper* decoder);

  // Backend wrapper instances must use these APIs when allocating and releasing
  // decoder objects, so we can enforce global limit on #concurrent decoders.
  bool IncrementDecoderCount(DecoderType type);
  void DecrementDecoderCount(DecoderType type);

  // Update the count of playing non-effects audio streams.
  void UpdatePlayingAudioCount(bool sfx, int change);

  void EnterPowerSaveMode();

  const scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;

  // Total count of decoders created
  int decoder_count_[NUM_DECODER_TYPES];

  // Total number of playing audio streams.
  int playing_audio_streams_count_;

  // Total number of playing non-effects streams.
  int playing_noneffects_audio_streams_count_;

  scoped_refptr<base::ObserverListThreadSafe<AllowVolumeFeedbackObserver>>
      allow_volume_feedback_observers_;

  base::flat_set<AudioDecoderWrapper*> audio_decoders_;
  base::flat_map<AudioContentType, float> global_volume_multipliers_;

  BufferDelegate* buffer_delegate_;

  base::OneShotTimer power_save_timer_;

  base::WeakPtrFactory<MediaPipelineBackendManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaPipelineBackendManager);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_MEDIA_PIPELINE_BACKEND_MANAGER_H_
