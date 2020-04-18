// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/media/media_devices.h"
#include "media/audio/audio_device_description.h"
#include "media/capture/video/video_capture_device_descriptor.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

TEST(MediaDevicesTest, MediaDeviceInfoFromAudioDescription) {
  const std::string kFakeDeviceID = "fake_device_id";
  const std::string kFakeLabel = "fake_label";
  const std::string kFakeGroupID = "fake_group_id";

  media::AudioDeviceDescription description(kFakeLabel, kFakeDeviceID,
                                            kFakeGroupID);
  MediaDeviceInfo device_info(description);
  EXPECT_EQ(kFakeDeviceID, device_info.device_id);
  EXPECT_EQ(kFakeLabel, device_info.label);
  EXPECT_EQ(kFakeGroupID, device_info.group_id);
}

TEST(MediaDevicesTest, MediaDeviceInfoFromVideoDescriptor) {
  media::VideoCaptureDeviceDescriptor descriptor(
      "display_name", "device_id", "model_id", media::VideoCaptureApi::UNKNOWN);

  // TODO(guidou): Add test for group ID when supported. See crbug.com/627793.
  MediaDeviceInfo device_info(descriptor);
  EXPECT_EQ(descriptor.device_id, device_info.device_id);
  EXPECT_EQ(descriptor.GetNameAndModel(), device_info.label);
}

}  // namespace content
