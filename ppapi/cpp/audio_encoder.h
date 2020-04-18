// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_AUDIO_ENCODER_H_
#define PPAPI_CPP_AUDIO_ENCODER_H_

#include <stdint.h>

#include "ppapi/c/pp_codecs.h"
#include "ppapi/c/ppb_audio_buffer.h"
#include "ppapi/cpp/audio_buffer.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/resource.h"

/// @file
/// This file defines the API to create and use a AudioEncoder resource.

namespace pp {

class InstanceHandle;

/// Audio encoder interface.
///
/// Typical usage:
/// - Call Create() to create a new audio encoder resource.
/// - Call GetSupportedFormats() to determine which codecs and profiles are
///   available.
/// - Call Initialize() to initialize the encoder for a supported profile.
/// - Call GetBuffer() to get a blank frame and fill it in, or get an audio
///   frame from another resource, e.g. <code>PPB_MediaStreamAudioTrack</code>.
/// - Call Encode() to push the audio buffer to the encoder. If an external
///   buffer is pushed, wait for completion to recycle the frame.
/// - Call GetBitstreamBuffer() continuously (waiting for each previous call to
///   complete) to pull encoded buffers from the encoder.
/// - Call RecycleBitstreamBuffer() after consuming the data in the bitstream
///   buffer.
/// - To destroy the encoder, the plugin should release all of its references to
///   it. Any pending callbacks will abort before the encoder is destroyed.
///
/// Available audio codecs vary by platform.
/// All: opus.
class AudioEncoder : public Resource {
 public:
  /// Default constructor for creating an is_null() <code>AudioEncoder</code>
  /// object.
  AudioEncoder();

  /// A constructor used to create a <code>AudioEncoder</code> and associate it
  /// with the provided <code>Instance</code>.
  /// @param[in] instance The instance with which this resource will be
  /// associated.
  explicit AudioEncoder(const InstanceHandle& instance);

  /// The copy constructor for <code>AudioEncoder</code>.
  /// @param[in] other A reference to a <code>AudioEncoder</code>.
  AudioEncoder(const AudioEncoder& other);

  /// Gets an array of supported audio encoder profiles.
  /// These can be used to choose a profile before calling Initialize().
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with the PP_AudioProfileDescription structs.
  ///
  /// @return If >= 0, the number of supported profiles returned, otherwise an
  /// error code from <code>pp_errors.h</code>.
  int32_t GetSupportedProfiles(const CompletionCallbackWithOutput<
      std::vector<PP_AudioProfileDescription> >& cc);

  /// Initializes a audio encoder resource. This should be called after
  /// GetSupportedProfiles() and before any functions below.
  ///
  /// @param[in] channels The number of audio channels to encode.
  /// @param[in] input_sampling_rate The sampling rate of the input audio
  /// buffer.
  /// @param[in] input_sample_size The sample size of the input audio buffer.
  /// @param[in] output_profile A <code>PP_AudioProfile</code> specifying the
  /// codec profile of the encoded output stream.
  /// @param[in] initial_bitrate The initial bitrate for the encoder.
  /// @param[in] acceleration A <code>PP_HardwareAcceleration</code> specifying
  /// whether to use a hardware accelerated or a software implementation.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>.
  /// Returns PP_ERROR_NOTSUPPORTED if audio encoding is not available, or the
  /// requested codec profile is not supported.
  /// Returns PP_ERROR_NOMEMORY if bitstream buffers can't be created.
  int32_t Initialize(uint32_t channels,
                     PP_AudioBuffer_SampleRate input_sample_rate,
                     PP_AudioBuffer_SampleSize input_sample_size,
                     PP_AudioProfile output_profile,
                     uint32_t initial_bitrate,
                     PP_HardwareAcceleration acceleration,
                     const CompletionCallback& cc);

  /// Gets the number of audio samples per channel that audio buffers
  /// must contain in order to be processed by the encoder. This will
  /// be the number of samples per channels contained in buffers
  /// returned by GetBuffer().
  ///
  /// @return An int32_t containing the number of samples required, or an error
  /// code from <code>pp_errors.h</code>.
  /// Returns PP_ERROR_FAILED if Initialize() has not successfully completed.
  int32_t GetNumberOfSamples();

  /// Gets a blank audio frame which can be filled with audio data and passed
  /// to the encoder.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with the blank <code>AudioBuffer</code> resource.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>.
  int32_t GetBuffer(const CompletionCallbackWithOutput<AudioBuffer>& cc);

  /// Encodes an audio buffer.
  ///
  /// @param[in] audio_buffer The <code>AudioBuffer</code> to be encoded.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion. Plugins that pass <code>AudioBuffer</code> resources owned
  /// by other resources should wait for completion before reusing them.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>.
  /// Returns PP_ERROR_FAILED if Initialize() has not successfully completed.
  int32_t Encode(const AudioBuffer& buffer, const CompletionCallback& cc);

  /// Gets the next encoded bitstream buffer from the encoder.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code> to be
  /// called upon completion with the next bitstream buffer. The plugin can call
  /// GetBitstreamBuffer from the callback in order to continuously "pull"
  /// bitstream buffers from the encoder.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>.
  /// Returns PP_ERROR_FAILED if Initialize() has not successfully completed.
  /// Returns PP_ERROR_INPROGRESS if a prior call to GetBitstreamBuffer() has
  /// not completed.
  int32_t GetBitstreamBuffer(
      const CompletionCallbackWithOutput<PP_AudioBitstreamBuffer>& cc);

  /// Recycles a bitstream buffer back to the encoder.
  ///
  /// @param[in] bitstream_buffer A<code>PP_AudioBitstreamBuffer</code> that
  /// is no longer needed by the plugin.
  void RecycleBitstreamBuffer(const PP_AudioBitstreamBuffer& bitstream_buffer);

  /// Requests a change to the encoding bitrate. This is only a request,
  /// fulfilled on a best-effort basis.
  ///
  /// @param[in] audio_encoder A <code>PP_Resource</code> identifying the audio
  /// encoder.
  void RequestBitrateChange(uint32_t bitrate);

  /// Closes the audio encoder, and cancels any pending encodes. Any pending
  /// callbacks will still run, reporting <code>PP_ERROR_ABORTED</code> . It is
  /// not valid to call any encoder functions after a call to this method.
  /// <strong>Note:</strong> Destroying the audio encoder closes it implicitly,
  /// so you are not required to call Close().
  void Close();
};

}  // namespace pp

#endif  // PPAPI_CPP_AUDIO_ENCODER_H_
