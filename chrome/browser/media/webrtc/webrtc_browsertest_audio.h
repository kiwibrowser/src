// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_BROWSERTEST_AUDIO_H_
#define CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_BROWSERTEST_AUDIO_H_

#include <vector>

namespace base {
class FilePath;
}

namespace media {
class AudioParameters;
}

namespace test {

// Computes the average power of the audio signal seen over the entire file.
//
// The |file_parameters| pointer is filled with a copy of the audio file's
// parameters as deduced by wav_audio_handler.h (i.e. what's in the header).
//
// Returns a audio dBFS (decibels relative to full-scale) value.
// See media/audio/audio_power_monitor.h for more details.
float ComputeAudioEnergyForWavFile(const base::FilePath& wav_file_path,
                                   media::AudioParameters* file_parameters);

}  // namespace test

#endif  // CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_BROWSERTEST_AUDIO_H_
