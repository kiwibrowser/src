// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "media/capture/video/mac/video_capture_device_factory_mac.h"
#include "media/capture/video/mac/video_capture_device_mac.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

TEST(VideoCaptureDeviceFactoryMacTest, ListDevicesAVFoundation) {
  VideoCaptureDeviceFactoryMac video_capture_device_factory;

  VideoCaptureDeviceDescriptors descriptors;
  video_capture_device_factory.GetDeviceDescriptors(&descriptors);
  if (descriptors.empty()) {
    DVLOG(1) << "No camera available. Exiting test.";
    return;
  }
  for (const auto& descriptor : descriptors)
    EXPECT_EQ(VideoCaptureApi::MACOSX_AVFOUNDATION, descriptor.capture_api);
}

};  // namespace media
