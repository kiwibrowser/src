// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Low-latency audio capturing class utilizing audio input stream provided
// by a server process by use of an IPC interface.
//
// Relationship of classes:
//
//  AudioInputController                 AudioInputDevice
//           ^                                  ^
//           |                                  |
//           v                  IPC             v
// MojoAudioInputStream    <----------->  AudioInputIPC
//           ^                            (MojoAudioInputIPC)
//           |
//           v
// AudioInputDeviceManager
//
// Transportation of audio samples from the browser to the render process
// is done by using shared memory in combination with a SyncSocket.
// The AudioInputDevice user registers an AudioInputDevice::CaptureCallback by
// calling Initialize().  The callback will be called with recorded audio from
// the underlying audio layers.
// The session ID is used by the RenderFrameAudioInputStreamFactory to start
// the device referenced by this ID.
//
// State sequences:
//
// Start -> CreateStream ->
//       <- OnStreamCreated <-
//       -> RecordStream ->
//
// AudioInputDevice::Capture => low latency audio transport on audio thread =>
//
// Stop ->  CloseStream -> Close
//
// This class depends on the audio transport thread. That thread is responsible
// for calling the CaptureCallback and feeding it audio samples from the server
// side audio layer using a socket and shared memory.
//
// Implementation notes:
// - The user must call Stop() before deleting the class instance.

#ifndef MEDIA_AUDIO_AUDIO_INPUT_DEVICE_H_
#define MEDIA_AUDIO_AUDIO_INPUT_DEVICE_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/shared_memory.h"
#include "base/optional.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "media/audio/alive_checker.h"
#include "media/audio/audio_device_thread.h"
#include "media/audio/audio_input_ipc.h"
#include "media/base/audio_capturer_source.h"
#include "media/base/audio_parameters.h"
#include "media/base/media_export.h"

namespace media {

class MEDIA_EXPORT AudioInputDevice : public AudioCapturerSource,
                                      public AudioInputIPCDelegate {
 public:
  // NOTE: Clients must call Initialize() before using.
  AudioInputDevice(std::unique_ptr<AudioInputIPC> ipc);

  // AudioCapturerSource implementation.
  void Initialize(const AudioParameters& params,
                  CaptureCallback* callback) override;
  void Start() override;
  void Stop() override;
  void SetVolume(double volume) override;
  void SetAutomaticGainControl(bool enabled) override;
  void SetOutputDeviceForAec(const std::string& output_device_id) override;

 private:
  friend class base::RefCountedThreadSafe<AudioInputDevice>;

  // Our audio thread callback class.  See source file for details.
  class AudioThreadCallback;

  // Note: The ordering of members in this enum is critical to correct behavior!
  enum State {
    IPC_CLOSED,       // No more IPCs can take place.
    IDLE,             // Not started.
    CREATING_STREAM,  // Waiting for OnStreamCreated() to be called back.
    RECORDING,        // Receiving audio data.
  };

  ~AudioInputDevice() override;

  // AudioInputIPCDelegate implementation.
  void OnStreamCreated(base::SharedMemoryHandle handle,
                       base::SyncSocket::Handle socket_handle,
                       bool initially_muted) override;
  void OnError() override;
  void OnMuted(bool is_muted) override;
  void OnIPCClosed() override;

  // This is called by |alive_checker_| if it detects that the input stream is
  // dead.
  void DetectedDeadInputStream();

  AudioParameters audio_parameters_;

  CaptureCallback* callback_;

  // A pointer to the IPC layer that takes care of sending requests over to
  // the stream implementation.  Only valid when state_ != IPC_CLOSED.
  std::unique_ptr<AudioInputIPC> ipc_;

  // Current state. See comments for State enum above.
  State state_;

  // For UMA stats.
  bool had_callback_error_ = false;

  // Stores the Automatic Gain Control state. Default is false.
  bool agc_is_enabled_;

  // Checks regularly that the input stream is alive and notifies us if it
  // isn't by calling DetectedDeadInputStream(). Must outlive |audio_callback_|.
  std::unique_ptr<AliveChecker> alive_checker_;

  std::unique_ptr<AudioInputDevice::AudioThreadCallback> audio_callback_;
  std::unique_ptr<AudioDeviceThread> audio_thread_;

  SEQUENCE_CHECKER(sequence_checker_);

  // Cache the output device used for AEC in case it's called before the stream
  // is created.
  base::Optional<std::string> output_device_id_for_aec_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(AudioInputDevice);
};

}  // namespace media

#endif  // MEDIA_AUDIO_AUDIO_INPUT_DEVICE_H_
