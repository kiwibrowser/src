// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_CRAS_AUDIO_MANAGER_CRAS_H_
#define MEDIA_AUDIO_CRAS_AUDIO_MANAGER_CRAS_H_

#include <cras_types.h>

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromeos/audio/audio_device.h"
#include "media/audio/audio_manager_base.h"

namespace media {

class MEDIA_EXPORT AudioManagerCras : public AudioManagerBase {
 public:
  AudioManagerCras(std::unique_ptr<AudioThread> audio_thread,
                   AudioLogFactory* audio_log_factory);
  ~AudioManagerCras() override;

  // AudioManager implementation.
  bool HasAudioOutputDevices() override;
  bool HasAudioInputDevices() override;
  void GetAudioInputDeviceNames(AudioDeviceNames* device_names) override;
  void GetAudioOutputDeviceNames(AudioDeviceNames* device_names) override;
  AudioParameters GetInputStreamParameters(
      const std::string& device_id) override;
  std::string GetAssociatedOutputDeviceID(
      const std::string& input_device_id) override;
  std::string GetDefaultInputDeviceID() override;
  std::string GetDefaultOutputDeviceID() override;
  std::string GetGroupIDOutput(const std::string& output_device_id) override;
  std::string GetGroupIDInput(const std::string& input_device_id) override;
  const char* GetName() override;
  bool Shutdown() override;

  // AudioManagerBase implementation.
  AudioOutputStream* MakeLinearOutputStream(
      const AudioParameters& params,
      const LogCallback& log_callback) override;
  AudioOutputStream* MakeLowLatencyOutputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;
  AudioInputStream* MakeLinearInputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;
  AudioInputStream* MakeLowLatencyInputStream(
      const AudioParameters& params,
      const std::string& device_id,
      const LogCallback& log_callback) override;

  // Checks if |device_id| corresponds to the default device.
  // Set |is_input| to true for capture devices, false for output.
  bool IsDefault(const std::string& device_id, bool is_input);

 protected:
  AudioParameters GetPreferredOutputStreamParameters(
      const std::string& output_device_id,
      const AudioParameters& input_params) override;

 private:
  // Called by MakeLinearOutputStream and MakeLowLatencyOutputStream.
  AudioOutputStream* MakeOutputStream(const AudioParameters& params,
                                      const std::string& device_id);

  // Called by MakeLinearInputStream and MakeLowLatencyInputStream.
  AudioInputStream* MakeInputStream(const AudioParameters& params,
                                    const std::string& device_id);

  // Get default output buffer size for this board.
  int GetDefaultOutputBufferSizePerBoard();

  void GetAudioDeviceNamesImpl(bool is_input, AudioDeviceNames* device_names);

  std::string GetHardwareDeviceFromDeviceId(
      const chromeos::AudioDeviceList& devices,
      bool is_input,
      const std::string& device_id);

  void GetAudioDevices(chromeos::AudioDeviceList* devices);
  void GetAudioDevicesOnMainThread(chromeos::AudioDeviceList* devices,
                                   base::WaitableEvent* event);
  uint64_t GetPrimaryActiveInputNode();
  uint64_t GetPrimaryActiveOutputNode();
  void GetPrimaryActiveInputNodeOnMainThread(uint64_t* active_input_node_id,
                                             base::WaitableEvent* event);
  void GetPrimaryActiveOutputNodeOnMainThread(uint64_t* active_output_node_id,
                                              base::WaitableEvent* event);
  void GetDefaultOutputBufferSizeOnMainThread(int32_t* buffer_size,
                                              base::WaitableEvent* event);

  void WaitEventOrShutdown(base::WaitableEvent* event);

  // Signaled if AudioManagerCras is shutting down.
  base::WaitableEvent on_shutdown_;

  // Task runner of browser main thread. CrasAudioHandler should be only
  // accessed on this thread.
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // For posting tasks from audio thread to |main_task_runner_|.
  base::WeakPtr<AudioManagerCras> weak_this_;

  base::WeakPtrFactory<AudioManagerCras> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AudioManagerCras);
};

}  // namespace media

#endif  // MEDIA_AUDIO_CRAS_AUDIO_MANAGER_CRAS_H_
