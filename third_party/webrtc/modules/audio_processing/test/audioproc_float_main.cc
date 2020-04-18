/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/test/audioproc_float.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "rtc_base/ptr_util.h"

int main(int argc, char* argv[]) {
  return webrtc::test::AudioprocFloat(
      rtc::MakeUnique<webrtc::AudioProcessingBuilder>(), argc, argv);
}
