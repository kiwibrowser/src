// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_OPENVR_TEST_FAKE_OPENVR_LOG_H_
#define DEVICE_VR_OPENVR_TEST_FAKE_OPENVR_LOG_H_

namespace vr {

// We log instances of this structure when a frame is submitted.
// Tests can walk the log to validate things ran as expected.
struct VRSubmittedFrameEvent {
  struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
  } color;
};

inline char* GetVrPixelLogEnvVarName() {
  static char ret[] = "VR_MOCK_LOG_PATH";
  return ret;
}

}  // namespace vr

#endif  // DEVICE_VR_OPENVR_TEST_FAKE_OPENVR_LOG_H_