// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_PUBLIC_MEDIA_AUDIO_POST_PROCESSOR2_SHLIB_H_
#define CHROMECAST_PUBLIC_MEDIA_AUDIO_POST_PROCESSOR2_SHLIB_H_

#include <string>
#include <vector>

#include "chromecast_export.h"
#include "volume_control.h"

// Plugin interface for audio DSP modules.
// This is applicable only to audio CMA backends (Alsa, Fuscia).
//
// Please refer to
// chromecast/media/cma/backend/post_processors/governor_shlib.cc
// as an example for new code, but OEM's implementations should not have any
// Chromium dependencies.
//
// Please refer to
// chromecast/media/cma/backend/post_processors/post_processor_wrapper.h for an
// example of how to port an existing AudioPostProcessor to AudioPostProcessor2
//
// Notes on PostProcessors that have a different number of in/out channels:
//  * PostProcessor authors are free to define their channel order; Cast will
//    simply pass this data to subsequent PostProcessors and MixerOutputStream.
//  * Channel selection for stereo pairs will occur after the "mix" group, so
//    devices that support stereo pairs should only change the number of
//    in the "linearize" group of cast_audio.json.

// Creates a PostProcessor
// Called from StreamMixer when shared objects are listed in
// /etc/cast_audio.json
// AudioPostProcessors are created on startup and destroyed on shutdown.
// libcast_FOOBAR_1.0.so should export a CREATE function as follows:
// AUDIO_POST_PROCESSOR2_SHLIB_CREATE_FUNC(FOOBAR) { }
#define AUDIO_POST_PROCESSOR2_SHLIB_CREATE_FUNC(type)                   \
  extern "C" CHROMECAST_EXPORT chromecast::media::AudioPostProcessor2*  \
      AudioPostProcessor2Shlib##type##Create(const std::string& config, \
                                             int num_channels_in)

namespace chromecast {
namespace media {

// Interface for AudioPostProcessors used for applying DSP in StreamMixer.
class AudioPostProcessor2 {
 public:
  // The maximum amount of data that will ever be processed in one call.
  static constexpr int kMaxAudioWriteTimeMilliseconds = 20;

  virtual ~AudioPostProcessor2() = default;

  // Updates the sample rate of the processor.
  // Returns |false| if the processor cannot support |sample_rate|
  // Returning false will result in crashing cast_shell.
  virtual bool SetSampleRate(int sample_rate) = 0;

  // Returns the number of output channels. This value should never change after
  // construction.
  virtual int NumOutputChannels() = 0;

  // Processes audio frames from |data|.
  // This will never be called before SetSampleRate().
  // Output buffer should be made available via GetOutputBuffer().
  // ProcessFrames may overwrite |data|, in which case GetOutputBuffer() should
  // return |data|.
  // |data| will be 32-bit interleaved float with |channels_in| channels.
  // If |channels_out| is larger than |channels_in|, AudioPostProcessor2 must
  // own a buffer of length at least |channels_out| * frames;
  // |data| cannot be assumed to be larger than |channels_in| * frames.
  // |frames| is the number of audio frames in data and is
  // always non-zero and less than or equal to |kMaxAudioWriteTimeMilliseconds|.
  // AudioPostProcessor must always provide |frames| frames of data back
  // (may output 0’s).
  // |system_volume| is the Cast Volume applied to the stream
  // (normalized to 0-1). It is the same as the cast volume set via alsa.
  // |volume_dbfs| is the actual attenuation in dBFS (-inf to 0), equivalent to
  // VolumeMap::VolumeToDbFS(|volume|).
  // AudioPostProcessor should assume that volume has already been applied.
  // Returns the current rendering delay of the filter in frames.
  virtual int ProcessFrames(float* data,
                            int frames,
                            float system_volume,
                            float volume_dbfs) = 0;

  // Returns the data buffer in which the last output from ProcessFrames() was
  // stored.
  // This will never be called before ProcessFrames().
  // This data location should be valid until ProcessFrames() is called
  // again.
  // The data returned by GetOutputBuffer() should not be modified by this
  // instance until the next call to ProcessFrames().
  // If |channels_in| >= |channels_out|, this may return |data| from the
  // last call to ProcessFrames().
  // If |channels_in| < |channels_out|, this PostProcessor is responsible for
  // allocating an output buffer.
  // If this PostProcessor owns the outputbuffer, it must ensure that the memory
  // is valid until the next call to ProcessFrames() or destruction.
  virtual float* GetOutputBuffer() = 0;

  // Returns the number of frames of silence it will take for the processor to
  // come to rest after playing out audio.
  // In the case of an FIR filter, this is the length of the FIR kernel.
  // In the case of IIR filters, this should be calculated as the number of
  // frames for the output to decay to 10% (5 time constants).
  // When inputs are paused, at least |GetRingingTimeInFrames()| of
  // silence will be passed through the processor. This will only be checked
  // when SetSampleRate() is called.
  virtual int GetRingingTimeInFrames() = 0;

  // Sends a message to the PostProcessor. Implementations are responsible
  // for the format and parsing of messages.
  // Returns |true| if the message was accepted or |false| if the message could
  // not be applied (i.e. invalid parameter, format error, parameter out of
  // range, etc).
  // If the PostProcessor can/will not be updated at runtime, this can be
  // implemented as "return false;"
  virtual bool UpdateParameters(const std::string& message) = 0;

  // Sets content type to the PostProcessor so it could change processing
  // settings accordingly.
  virtual void SetContentType(AudioContentType content_type) {}

  // Called when device is playing as part of a stereo pair.
  // |channel| is the playout channel on this device (0 for left, 1 for right).
  // or -1 if the device is not part of a stereo pair.
  virtual void SetPlayoutChannel(int channel) {}
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_PUBLIC_MEDIA_AUDIO_POST_PROCESSOR2_SHLIB_H_
