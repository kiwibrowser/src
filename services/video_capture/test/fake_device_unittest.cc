// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "media/base/video_frame.h"
#include "media/mojo/common/media_type_converters.h"
#include "services/video_capture/device_media_to_mojo_adapter.h"
#include "services/video_capture/public/mojom/constants.mojom.h"
#include "services/video_capture/public/mojom/device_factory.mojom.h"
#include "services/video_capture/test/fake_device_test.h"
#include "services/video_capture/test/mock_receiver.h"

using testing::_;
using testing::AtLeast;
using testing::Invoke;
using testing::InvokeWithoutArgs;

namespace video_capture {

// This alias ensures test output is easily attributed to this service's tests.
// TODO(rockot/chfremer): Consider just renaming the type.
using FakeVideoCaptureDeviceTest = FakeDeviceTest;

TEST_F(FakeVideoCaptureDeviceTest, FrameCallbacksArrive) {
  base::RunLoop wait_loop;
  // Constants must be static as a workaround
  // for a MSVC++ bug about lambda captures, see the discussion at
  // https://social.msdn.microsoft.com/Forums/SqlServer/4abf18bd-4ae4-4c72-ba3e-3b13e7909d5f
  static const int kNumFramesToWaitFor = 3;
  int num_frames_arrived = 0;
  mojom::ReceiverPtr receiver_proxy;
  MockReceiver receiver(mojo::MakeRequest(&receiver_proxy));
  EXPECT_CALL(receiver, DoOnNewBuffer(_, _)).Times(AtLeast(1));
  EXPECT_CALL(receiver, DoOnFrameReadyInBuffer(_, _, _, _))
      .WillRepeatedly(InvokeWithoutArgs([&wait_loop, &num_frames_arrived]() {
        num_frames_arrived += 1;
        if (num_frames_arrived >= kNumFramesToWaitFor) {
          wait_loop.Quit();
        }
      }));

  fake_device_proxy_->Start(requestable_settings_, std::move(receiver_proxy));
  wait_loop.Run();
}

// Tests that buffers get reused when receiving more frames than the maximum
// number of buffers in the pool.
TEST_F(FakeVideoCaptureDeviceTest, BuffersGetReused) {
  base::RunLoop wait_loop;
  const int kMaxBufferPoolBuffers =
      DeviceMediaToMojoAdapter::max_buffer_pool_buffer_count();
  // Constants must be static as a workaround
  // for a MSVC++ bug about lambda captures, see the discussion at
  // https://social.msdn.microsoft.com/Forums/SqlServer/4abf18bd-4ae4-4c72-ba3e-3b13e7909d5f
  static const int kNumFramesToWaitFor = kMaxBufferPoolBuffers + 3;
  int num_buffers_created = 0;
  int num_frames_arrived = 0;
  mojom::ReceiverPtr receiver_proxy;
  MockReceiver receiver(mojo::MakeRequest(&receiver_proxy));
  EXPECT_CALL(receiver, DoOnNewBuffer(_, _))
      .WillRepeatedly(InvokeWithoutArgs(
          [&num_buffers_created]() { num_buffers_created++; }));
  EXPECT_CALL(receiver, DoOnFrameReadyInBuffer(_, _, _, _))
      .WillRepeatedly(InvokeWithoutArgs([&wait_loop, &num_frames_arrived]() {
        if (++num_frames_arrived >= kNumFramesToWaitFor) {
          wait_loop.Quit();
        }
      }));

  fake_device_proxy_->Start(requestable_settings_, std::move(receiver_proxy));
  wait_loop.Run();

  ASSERT_LT(num_buffers_created, num_frames_arrived);
  ASSERT_LE(num_buffers_created, kMaxBufferPoolBuffers);
}

// Tests that the service requests to be closed when the last client disconnects
// while using a device.
TEST_F(FakeVideoCaptureDeviceTest,
       ServiceQuitsWhenClientDisconnectsWhileUsingDevice) {
  base::RunLoop wait_loop;
  EXPECT_CALL(*service_state_observer_, OnServiceStopped(_))
      .WillOnce(Invoke([&wait_loop](const service_manager::Identity& identity) {
        if (identity.name() == mojom::kServiceName)
          wait_loop.Quit();
      }));

  mojom::ReceiverPtr receiver_proxy;
  MockReceiver receiver(mojo::MakeRequest(&receiver_proxy));
  fake_device_proxy_->Start(requestable_settings_, std::move(receiver_proxy));

  fake_device_proxy_.reset();
  factory_.reset();
  factory_provider_.reset();

  wait_loop.Run();
}

}  // namespace video_capture
