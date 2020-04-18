/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef TEST_SCENARIO_QUALITY_INFO_H_
#define TEST_SCENARIO_QUALITY_INFO_H_

#include "api/units/timestamp.h"

namespace webrtc {
namespace test {
struct VideoFrameQualityInfo {
  Timestamp capture_time;
  Timestamp received_capture_time;
  Timestamp render_time;
  int width;
  int height;
  double psnr;
};
}  // namespace test
}  // namespace webrtc
#endif  // TEST_SCENARIO_QUALITY_INFO_H_
