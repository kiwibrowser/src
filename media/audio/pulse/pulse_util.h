// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_PULSE_PULSE_UTIL_H_
#define MEDIA_AUDIO_PULSE_PULSE_UTIL_H_

#include <pulse/pulseaudio.h>

#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "media/audio/audio_device_name.h"
#include "media/base/audio_parameters.h"
#include "media/base/channel_layout.h"

namespace media {

class AudioParameters;

namespace pulse {

// A helper class that acquires pa_threaded_mainloop_lock() while in scope.
class AutoPulseLock {
 public:
  explicit AutoPulseLock(pa_threaded_mainloop* pa_mainloop)
      : pa_mainloop_(pa_mainloop) {
    pa_threaded_mainloop_lock(pa_mainloop_);
  }

  ~AutoPulseLock() {
    pa_threaded_mainloop_unlock(pa_mainloop_);
  }

 private:
  pa_threaded_mainloop* pa_mainloop_;
  DISALLOW_COPY_AND_ASSIGN(AutoPulseLock);
};

bool MEDIA_EXPORT InitPulse(pa_threaded_mainloop** mainloop,
                            pa_context** context);
void DestroyPulse(pa_threaded_mainloop* mainloop, pa_context* context);

// Triggers pa_threaded_mainloop_signal() to avoid deadlocks.
void StreamSuccessCallback(pa_stream* s, int error, void* mainloop);
void ContextStateCallback(pa_context* context, void* mainloop);

pa_channel_map ChannelLayoutToPAChannelMap(ChannelLayout channel_layout);

void WaitForOperationCompletion(pa_threaded_mainloop* mainloop,
                                pa_operation* operation);

base::TimeDelta GetHardwareLatency(pa_stream* stream);

constexpr SampleFormat kInputSampleFormat = kSampleFormatS16;

// Create a recording stream for the threaded mainloop, return true if success,
// otherwise false. |mainloop| and |context| have to be from a valid Pulse
// threaded mainloop and the handle of the created stream will be returned by
// |stream|.
bool CreateInputStream(pa_threaded_mainloop* mainloop,
                       pa_context* context,
                       pa_stream** stream,
                       const AudioParameters& params,
                       const std::string& device_id,
                       pa_stream_notify_cb_t stream_callback,
                       void* user_data);

// Create a playback stream for the threaded mainloop, return true if success,
// otherwise false. This function will create a new Pulse threaded mainloop,
// and the handles of the mainloop, context and stream will be returned by
// |mainloop|, |context| and |stream|.
bool CreateOutputStream(pa_threaded_mainloop** mainloop,
                        pa_context** context,
                        pa_stream** stream,
                        const AudioParameters& params,
                        const std::string& device_id,
                        const std::string& app_name,
                        pa_stream_notify_cb_t stream_callback,
                        pa_stream_request_cb_t write_callback,
                        void* user_data);

}  // namespace pulse

}  // namespace media

#endif  // MEDIA_AUDIO_PULSE_PULSE_UTIL_H_
