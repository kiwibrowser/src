// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/test/fake_device_descriptor_test.h"

#include "base/run_loop.h"

using testing::_;
using testing::Invoke;

namespace video_capture {

FakeDeviceDescriptorTest::FakeDeviceDescriptorTest()
    : video_capture::DeviceFactoryProviderTest() {}

FakeDeviceDescriptorTest::~FakeDeviceDescriptorTest() = default;

void FakeDeviceDescriptorTest::SetUp() {
  video_capture::DeviceFactoryProviderTest::SetUp();

  base::RunLoop wait_loop;
  EXPECT_CALL(device_info_receiver_, Run(_))
      .WillOnce(
          Invoke([this, &wait_loop](
                     const std::vector<media::VideoCaptureDeviceInfo>& infos) {
            fake_device_info_ = infos[0];
            wait_loop.Quit();
          }));
  factory_->GetDeviceInfos(device_info_receiver_.Get());
  wait_loop.Run();
}

}  // namespace video_capture
