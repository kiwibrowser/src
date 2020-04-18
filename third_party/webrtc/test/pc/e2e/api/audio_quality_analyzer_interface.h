/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_PC_E2E_API_AUDIO_QUALITY_ANALYZER_INTERFACE_H_
#define TEST_PC_E2E_API_AUDIO_QUALITY_ANALYZER_INTERFACE_H_

namespace webrtc {

class AudioQualityAnalyzerInterface {
 public:
  virtual ~AudioQualityAnalyzerInterface() = default;
};

}  // namespace webrtc

#endif  // TEST_PC_E2E_API_AUDIO_QUALITY_ANALYZER_INTERFACE_H_
