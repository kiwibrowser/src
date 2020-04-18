// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/shared_memory.h"
#include "base/test/scoped_task_environment.h"
#include "content/child/child_process.h"
#include "content/renderer/media/video_capture_impl.h"
#include "media/capture/mojom/video_capture.mojom.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::SaveArg;
using ::testing::WithArgs;

namespace content {

const int kSessionId = 11;

void RunEmptyFormatsCallback(
    media::mojom::VideoCaptureHost::GetDeviceSupportedFormatsCallback&
        callback) {
  media::VideoCaptureFormats formats;
  std::move(callback).Run(formats);
}

ACTION(DoNothing) {}

// Mock implementation of the Mojo Host service.
class MockMojoVideoCaptureHost : public media::mojom::VideoCaptureHost {
 public:
  MockMojoVideoCaptureHost() : released_buffer_count_(0) {
    ON_CALL(*this, GetDeviceSupportedFormatsMock(_, _, _))
        .WillByDefault(WithArgs<2>(Invoke(RunEmptyFormatsCallback)));
    ON_CALL(*this, GetDeviceFormatsInUseMock(_, _, _))
        .WillByDefault(WithArgs<2>(Invoke(RunEmptyFormatsCallback)));
    ON_CALL(*this, ReleaseBuffer(_, _, _))
        .WillByDefault(InvokeWithoutArgs(
            this, &MockMojoVideoCaptureHost::increase_released_buffer_count));
  }

  // Start() can't be mocked directly due to move-only |observer|.
  void Start(int32_t device_id,
             int32_t session_id,
             const media::VideoCaptureParams& params,
             media::mojom::VideoCaptureObserverPtr observer) override {
    DoStart(device_id, session_id, params);
  }
  MOCK_METHOD3(DoStart,
               void(int32_t, int32_t, const media::VideoCaptureParams&));
  MOCK_METHOD1(Stop, void(int32_t));
  MOCK_METHOD1(Pause, void(int32_t));
  MOCK_METHOD3(Resume,
               void(int32_t, int32_t, const media::VideoCaptureParams&));
  MOCK_METHOD1(RequestRefreshFrame, void(int32_t));
  MOCK_METHOD3(ReleaseBuffer, void(int32_t, int32_t, double));
  MOCK_METHOD3(GetDeviceSupportedFormatsMock,
               void(int32_t, int32_t, GetDeviceSupportedFormatsCallback&));
  MOCK_METHOD3(GetDeviceFormatsInUseMock,
               void(int32_t, int32_t, GetDeviceFormatsInUseCallback&));

  void GetDeviceSupportedFormats(
      int32_t arg1,
      int32_t arg2,
      GetDeviceSupportedFormatsCallback arg3) override {
    GetDeviceSupportedFormatsMock(arg1, arg2, arg3);
  }

  void GetDeviceFormatsInUse(int32_t arg1,
                             int32_t arg2,
                             GetDeviceFormatsInUseCallback arg3) override {
    GetDeviceFormatsInUseMock(arg1, arg2, arg3);
  }

  int released_buffer_count() const { return released_buffer_count_; }
  void increase_released_buffer_count() { released_buffer_count_++; }

 private:
  int released_buffer_count_;

  DISALLOW_COPY_AND_ASSIGN(MockMojoVideoCaptureHost);
};

// This class encapsulates a VideoCaptureImpl under test and the necessary
// accessory classes, namely:
// - a MockMojoVideoCaptureHost, mimicking the RendererHost;
// - a few callbacks that are bound when calling operations of VideoCaptureImpl
//  and on which we set expectations.
class VideoCaptureImplTest : public ::testing::Test {
 public:
  VideoCaptureImplTest()
      : video_capture_impl_(new VideoCaptureImpl(kSessionId)) {
    params_small_.requested_format = media::VideoCaptureFormat(
        gfx::Size(176, 144), 30, media::PIXEL_FORMAT_I420);
    params_large_.requested_format = media::VideoCaptureFormat(
        gfx::Size(320, 240), 30, media::PIXEL_FORMAT_I420);

    video_capture_impl_->SetVideoCaptureHostForTesting(
        &mock_video_capture_host_);

    ON_CALL(mock_video_capture_host_, DoStart(_, _, _))
        .WillByDefault(InvokeWithoutArgs([this]() {
          video_capture_impl_->OnStateChanged(
              media::mojom::VideoCaptureState::STARTED);
        }));
  }

 protected:
  // These four mocks are used to create callbacks for the different oeprations.
  MOCK_METHOD2(OnFrameReady,
               void(const scoped_refptr<media::VideoFrame>&, base::TimeTicks));
  MOCK_METHOD1(OnStateUpdate, void(VideoCaptureState));
  MOCK_METHOD1(OnDeviceFormatsInUse, void(const media::VideoCaptureFormats&));
  MOCK_METHOD1(OnDeviceSupportedFormats,
               void(const media::VideoCaptureFormats&));

  void StartCapture(int client_id, const media::VideoCaptureParams& params) {
    const auto state_update_callback = base::Bind(
        &VideoCaptureImplTest::OnStateUpdate, base::Unretained(this));
    const auto frame_ready_callback =
        base::Bind(&VideoCaptureImplTest::OnFrameReady, base::Unretained(this));

    video_capture_impl_->StartCapture(client_id, params, state_update_callback,
                                      frame_ready_callback);
  }

  void StopCapture(int client_id) {
    video_capture_impl_->StopCapture(client_id);
  }

  bool CreateAndMapSharedMemory(size_t size, base::SharedMemory* shm) {
    base::SharedMemoryCreateOptions options;
    options.size = size;
    options.share_read_only = true;
    if (!shm->Create(options))
      return false;
    return shm->Map(size);
  }

  void SimulateOnBufferCreated(int buffer_id, const base::SharedMemory& shm) {
    media::mojom::VideoBufferHandlePtr buffer_handle =
        media::mojom::VideoBufferHandle::New();
    buffer_handle->set_shared_buffer_handle(mojo::WrapSharedMemoryHandle(
        shm.GetReadOnlyHandle(), shm.mapped_size(),
        mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly));
    video_capture_impl_->OnNewBuffer(buffer_id, std::move(buffer_handle));
  }

  void SimulateBufferReceived(int buffer_id, const gfx::Size& size) {
    media::mojom::VideoFrameInfoPtr info = media::mojom::VideoFrameInfo::New();

    const base::TimeTicks now = base::TimeTicks::Now();
    media::VideoFrameMetadata frame_metadata;
    frame_metadata.SetTimeTicks(media::VideoFrameMetadata::REFERENCE_TIME, now);
    info->metadata = frame_metadata.GetInternalValues().Clone();

    info->timestamp = now - base::TimeTicks();
    info->pixel_format = media::PIXEL_FORMAT_I420;
    info->coded_size = size;
    info->visible_rect = gfx::Rect(size);

    video_capture_impl_->OnBufferReady(buffer_id, std::move(info));
  }

  void SimulateBufferDestroyed(int buffer_id) {
    video_capture_impl_->OnBufferDestroyed(buffer_id);
  }

  void GetDeviceSupportedFormats() {
    const base::Callback<void(const media::VideoCaptureFormats&)>
        callback = base::Bind(
            &VideoCaptureImplTest::OnDeviceSupportedFormats,
            base::Unretained(this));
    video_capture_impl_->GetDeviceSupportedFormats(callback);
  }

  void GetDeviceFormatsInUse() {
    const base::Callback<void(const media::VideoCaptureFormats&)>
        callback = base::Bind(
            &VideoCaptureImplTest::OnDeviceFormatsInUse,
            base::Unretained(this));
    video_capture_impl_->GetDeviceFormatsInUse(callback);
  }

  void OnStateChanged(media::mojom::VideoCaptureState state) {
    video_capture_impl_->OnStateChanged(state);
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  const ChildProcess child_process_;
  const std::unique_ptr<VideoCaptureImpl> video_capture_impl_;
  MockMojoVideoCaptureHost mock_video_capture_host_;
  media::VideoCaptureParams params_small_;
  media::VideoCaptureParams params_large_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoCaptureImplTest);
};

TEST_F(VideoCaptureImplTest, Simple) {
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STOPPED));
  EXPECT_CALL(mock_video_capture_host_, DoStart(_, kSessionId, params_small_));
  EXPECT_CALL(mock_video_capture_host_, Stop(_));

  StartCapture(0, params_small_);
  StopCapture(0);
}

TEST_F(VideoCaptureImplTest, TwoClientsInSequence) {
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STOPPED)).Times(2);
  EXPECT_CALL(mock_video_capture_host_, DoStart(_, kSessionId, params_small_));
  EXPECT_CALL(mock_video_capture_host_, Stop(_));

  StartCapture(0, params_small_);
  StopCapture(0);
  StartCapture(1, params_small_);
  StopCapture(1);
}

TEST_F(VideoCaptureImplTest, LargeAndSmall) {
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STOPPED)).Times(2);
  EXPECT_CALL(mock_video_capture_host_, DoStart(_, kSessionId, params_large_));
  EXPECT_CALL(mock_video_capture_host_, Stop(_));

  StartCapture(0, params_large_);
  StopCapture(0);
  StartCapture(1, params_small_);
  StopCapture(1);
}

TEST_F(VideoCaptureImplTest, SmallAndLarge) {
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STOPPED)).Times(2);
  EXPECT_CALL(mock_video_capture_host_, DoStart(_, kSessionId, params_small_));
  EXPECT_CALL(mock_video_capture_host_, Stop(_));

  StartCapture(0, params_small_);
  StopCapture(0);
  StartCapture(1, params_large_);
  StopCapture(1);
}

// Checks that a request to GetDeviceSupportedFormats() ends up eventually in
// the provided callback.
TEST_F(VideoCaptureImplTest, GetDeviceFormats) {
  EXPECT_CALL(*this, OnDeviceSupportedFormats(_));
  EXPECT_CALL(mock_video_capture_host_,
              GetDeviceSupportedFormatsMock(_, kSessionId, _));

  GetDeviceSupportedFormats();
}

// Checks that two requests to GetDeviceSupportedFormats() end up eventually
// calling the provided callbacks.
TEST_F(VideoCaptureImplTest, TwoClientsGetDeviceFormats) {
  EXPECT_CALL(*this, OnDeviceSupportedFormats(_)).Times(2);
  EXPECT_CALL(mock_video_capture_host_,
              GetDeviceSupportedFormatsMock(_, kSessionId, _))
      .Times(2);

  GetDeviceSupportedFormats();
  GetDeviceSupportedFormats();
}

// Checks that a request to GetDeviceFormatsInUse() ends up eventually in the
// provided callback.
TEST_F(VideoCaptureImplTest, GetDeviceFormatsInUse) {
  EXPECT_CALL(*this, OnDeviceFormatsInUse(_));
  EXPECT_CALL(mock_video_capture_host_,
              GetDeviceFormatsInUseMock(_, kSessionId, _));

  GetDeviceFormatsInUse();
}

TEST_F(VideoCaptureImplTest, BufferReceived) {
  const int kBufferId = 11;

  base::SharedMemory shm;
  const size_t frame_size = media::VideoFrame::AllocationSize(
      media::PIXEL_FORMAT_I420, params_small_.requested_format.frame_size);
  ASSERT_TRUE(CreateAndMapSharedMemory(frame_size, &shm));

  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STOPPED));
  EXPECT_CALL(*this, OnFrameReady(_, _));
  EXPECT_CALL(mock_video_capture_host_, DoStart(_, kSessionId, params_small_));
  EXPECT_CALL(mock_video_capture_host_, Stop(_));
  EXPECT_CALL(mock_video_capture_host_, ReleaseBuffer(_, kBufferId, _))
      .Times(0);

  StartCapture(0, params_small_);
  SimulateOnBufferCreated(kBufferId, shm);
  SimulateBufferReceived(kBufferId, params_small_.requested_format.frame_size);
  StopCapture(0);
  SimulateBufferDestroyed(kBufferId);

  EXPECT_EQ(mock_video_capture_host_.released_buffer_count(), 0);
}

TEST_F(VideoCaptureImplTest, BufferReceivedAfterStop) {
  const int kBufferId = 12;

  base::SharedMemory shm;
  const size_t frame_size = media::VideoFrame::AllocationSize(
      media::PIXEL_FORMAT_I420, params_large_.requested_format.frame_size);
  ASSERT_TRUE(CreateAndMapSharedMemory(frame_size, &shm));

  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STOPPED));
  EXPECT_CALL(*this, OnFrameReady(_, _)).Times(0);
  EXPECT_CALL(mock_video_capture_host_, DoStart(_, kSessionId, params_large_));
  EXPECT_CALL(mock_video_capture_host_, Stop(_));
  EXPECT_CALL(mock_video_capture_host_, ReleaseBuffer(_, kBufferId, _));

  StartCapture(0, params_large_);
  SimulateOnBufferCreated(kBufferId, shm);
  StopCapture(0);
  // A buffer received after StopCapture() triggers an instant ReleaseBuffer().
  SimulateBufferReceived(kBufferId, params_large_.requested_format.frame_size);
  SimulateBufferDestroyed(kBufferId);

  EXPECT_EQ(mock_video_capture_host_.released_buffer_count(), 1);
}

TEST_F(VideoCaptureImplTest, AlreadyStarted) {
  media::VideoCaptureParams params = {};
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STOPPED)).Times(2);
  EXPECT_CALL(mock_video_capture_host_, DoStart(_, kSessionId, params_small_))
      .WillOnce(DoAll(InvokeWithoutArgs([this]() {
                        video_capture_impl_->OnStateChanged(
                            media::mojom::VideoCaptureState::STARTED);
                      }),
                      SaveArg<2>(&params)));
  EXPECT_CALL(mock_video_capture_host_, Stop(_));

  StartCapture(0, params_small_);
  StartCapture(1, params_large_);
  StopCapture(0);
  StopCapture(1);
  DCHECK(params.requested_format == params_small_.requested_format);
}

TEST_F(VideoCaptureImplTest, EndedBeforeStop) {
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STOPPED));
  EXPECT_CALL(mock_video_capture_host_, DoStart(_, kSessionId, params_small_));

  StartCapture(0, params_small_);

  OnStateChanged(media::mojom::VideoCaptureState::ENDED);

  StopCapture(0);
}

TEST_F(VideoCaptureImplTest, ErrorBeforeStop) {
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_ERROR));
  EXPECT_CALL(mock_video_capture_host_, DoStart(_, kSessionId, params_small_));

  StartCapture(0, params_small_);

  OnStateChanged(media::mojom::VideoCaptureState::FAILED);

  StopCapture(0);
}

TEST_F(VideoCaptureImplTest, BufferReceivedBeforeOnStarted) {
  const int kBufferId = 16;

  base::SharedMemory shm;
  const size_t frame_size = media::VideoFrame::AllocationSize(
      media::PIXEL_FORMAT_I420, params_small_.requested_format.frame_size);
  ASSERT_TRUE(CreateAndMapSharedMemory(frame_size, &shm));

  InSequence s;
  EXPECT_CALL(mock_video_capture_host_, DoStart(_, kSessionId, params_small_))
      .WillOnce(DoNothing());
  EXPECT_CALL(mock_video_capture_host_, ReleaseBuffer(_, kBufferId, _));
  StartCapture(0, params_small_);
  SimulateOnBufferCreated(kBufferId, shm);
  SimulateBufferReceived(kBufferId, params_small_.requested_format.frame_size);

  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(mock_video_capture_host_, RequestRefreshFrame(_));
  video_capture_impl_->OnStateChanged(media::mojom::VideoCaptureState::STARTED);

  // Additional STARTED will cause RequestRefreshFrame a second time.
  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STARTED));
  EXPECT_CALL(mock_video_capture_host_, RequestRefreshFrame(_));
  video_capture_impl_->OnStateChanged(media::mojom::VideoCaptureState::STARTED);

  EXPECT_CALL(*this, OnStateUpdate(VIDEO_CAPTURE_STATE_STOPPED));
  EXPECT_CALL(mock_video_capture_host_, Stop(_));
  StopCapture(0);
}

}  // namespace content
