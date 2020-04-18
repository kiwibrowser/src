// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"

#include "services/video_capture/test/mock_device_test.h"

using testing::_;
using testing::Invoke;

namespace video_capture {

// This alias ensures test output is easily attributed to this service's tests.
// TODO(rockot/chfremer): Consider just renaming the type.
using MockVideoCaptureDeviceTest = MockDeviceTest;

// Tests that the service stops the capture device when the client closes the
// connection to the device proxy.
TEST_F(MockVideoCaptureDeviceTest, DeviceIsStoppedWhenDiscardingDeviceProxy) {
  {
    base::RunLoop wait_loop;

    EXPECT_CALL(mock_device_, DoStopAndDeAllocate())
        .WillOnce(Invoke([&wait_loop]() { wait_loop.Quit(); }));

    device_proxy_->Start(requested_settings_, std::move(mock_receiver_proxy_));
    device_proxy_.reset();

    wait_loop.Run();
  }

  // The internals of ReceiverOnTaskRunner perform a DeleteSoon().
  {
    base::RunLoop wait_loop;
    wait_loop.RunUntilIdle();
  }
}

// Tests that the service stops the capture device when the client closes the
// connection to the client proxy it provided to the service.
TEST_F(MockVideoCaptureDeviceTest, DeviceIsStoppedWhenDiscardingDeviceClient) {
  {
    base::RunLoop wait_loop;

    EXPECT_CALL(mock_device_, DoStopAndDeAllocate())
        .WillOnce(Invoke([&wait_loop]() { wait_loop.Quit(); }));

    device_proxy_->Start(requested_settings_, std::move(mock_receiver_proxy_));
    mock_receiver_.reset();

    wait_loop.Run();
  }

  // The internals of ReceiverOnTaskRunner perform a DeleteSoon().
  {
    base::RunLoop wait_loop;
    wait_loop.RunUntilIdle();
  }
}

// Tests that a utilization reported to a video_capture.mojom.Device via
// OnReceiverReportingUtilization() arrives at the corresponding
// media::VideoCaptureDevice.
TEST_F(MockVideoCaptureDeviceTest, ReceiverUtilizationIsForwardedToDevice) {
  base::RunLoop run_loop;
  const media::VideoCaptureFormat stub_frame_format(gfx::Size(320, 200), 25.0f,
                                                    media::PIXEL_FORMAT_I420);
  const int arbitrary_rotation = 0;
  const int arbitrary_frame_feedback_id = 654;
  const double arbitrary_utilization = 0.12345;

  EXPECT_CALL(*mock_receiver_, DoOnFrameReadyInBuffer(_, _, _, _))
      .WillOnce(Invoke([this, &arbitrary_utilization](
                           int32_t buffer_id, int32_t frame_feedback_id,
                           mojom::ScopedAccessPermissionPtr*,
                           media::mojom::VideoFrameInfoPtr*) {
        device_proxy_->OnReceiverReportingUtilization(frame_feedback_id,
                                                      arbitrary_utilization);
      }));

  EXPECT_CALL(mock_device_, OnUtilizationReport(arbitrary_frame_feedback_id,
                                                arbitrary_utilization))
      .Times(1);

  device_proxy_->Start(requested_settings_, std::move(mock_receiver_proxy_));
  run_loop.RunUntilIdle();

  // Simulate device sending a frame, which should trigger |mock_receiver|
  // DoOnFrameReadyInBuffer() getting called.
  base::RunLoop run_loop_2;
  mock_device_.SendStubFrame(stub_frame_format, arbitrary_rotation,
                             arbitrary_frame_feedback_id);
  run_loop_2.RunUntilIdle();

  base::RunLoop run_loop_3;
  mock_receiver_.reset();
  run_loop_3.RunUntilIdle();
}

}  // namespace video_capture
