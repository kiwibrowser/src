// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/test/scoped_task_environment.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/test_data_util.h"
#include "media/capture/video/file_video_capture_device.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::InvokeWithoutArgs;

namespace media {

namespace {

class MockClient : public VideoCaptureDevice::Client {
 public:
  void OnIncomingCapturedData(const uint8_t* data,
                              int length,
                              const VideoCaptureFormat& frame_format,
                              int clockwise_rotation,
                              base::TimeTicks reference_time,
                              base::TimeDelta timestamp,
                              int frame_feedback_id = 0) override {}

  void OnIncomingCapturedGfxBuffer(gfx::GpuMemoryBuffer* buffer,
                                   const VideoCaptureFormat& frame_format,
                                   int clockwise_rotation,
                                   base::TimeTicks reference_time,
                                   base::TimeDelta timestamp,
                                   int frame_feedback_id = 0) override {}

  MOCK_METHOD3(ReserveOutputBuffer,
               Buffer(const gfx::Size&, VideoPixelFormat, int));

  void OnIncomingCapturedBuffer(Buffer buffer,
                                const VideoCaptureFormat& format,
                                base::TimeTicks reference_,
                                base::TimeDelta timestamp) override {}

  void OnIncomingCapturedBufferExt(
      Buffer buffer,
      const VideoCaptureFormat& format,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      gfx::Rect visible_rect,
      const VideoFrameMetadata& additional_metadata) override {}

  MOCK_METHOD3(ResurrectLastOutputBuffer,
               Buffer(const gfx::Size&, VideoPixelFormat, int));

  MOCK_METHOD2(OnError, void(const base::Location&, const std::string&));

  double GetBufferPoolUtilization() const override { return 0.0; }

  MOCK_METHOD0(OnStarted, void());
};

class MockImageCaptureClient {
 public:
  // GMock doesn't support move-only arguments, so we use this forward method.
  void DoOnGetPhotoState(mojom::PhotoStatePtr state) {
    state_ = std::move(state);
  }

  const mojom::PhotoState* state() { return state_.get(); }

  MOCK_METHOD1(OnCorrectSetPhotoOptions, void(bool));

  // GMock doesn't support move-only arguments, so we use this forward method.
  void DoOnPhotoTaken(mojom::BlobPtr blob) {
    EXPECT_TRUE(blob);
    OnCorrectPhotoTaken();
  }
  MOCK_METHOD0(OnCorrectPhotoTaken, void(void));

 private:
  mojom::PhotoStatePtr state_;
};

}  // namespace

class FileVideoCaptureDeviceTest : public ::testing::Test {
 protected:
  FileVideoCaptureDeviceTest() : client_(new MockClient()) {}

  void SetUp() override {
    EXPECT_CALL(*client_, OnError(_, _)).Times(0);
    EXPECT_CALL(*client_, OnStarted());
    device_ = std::make_unique<FileVideoCaptureDevice>(
        GetTestDataFilePath("bear.mjpeg"));
    device_->AllocateAndStart(VideoCaptureParams(), std::move(client_));
  }

  void TearDown() override { device_->StopAndDeAllocate(); }

  std::unique_ptr<MockClient> client_;
  MockImageCaptureClient image_capture_client_;
  std::unique_ptr<VideoCaptureDevice> device_;
  VideoCaptureFormat last_format_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(FileVideoCaptureDeviceTest, GetPhotoState) {
  VideoCaptureDevice::GetPhotoStateCallback scoped_get_callback =
      base::BindOnce(&MockImageCaptureClient::DoOnGetPhotoState,
                     base::Unretained(&image_capture_client_));

  device_->GetPhotoState(std::move(scoped_get_callback));

  const mojom::PhotoState* state = image_capture_client_.state();
  EXPECT_TRUE(state);
}

TEST_F(FileVideoCaptureDeviceTest, SetPhotoOptions) {
  mojom::PhotoSettingsPtr photo_settings = mojom::PhotoSettings::New();
  VideoCaptureDevice::SetPhotoOptionsCallback scoped_set_callback =
      base::BindOnce(&MockImageCaptureClient::OnCorrectSetPhotoOptions,
                     base::Unretained(&image_capture_client_));
  EXPECT_CALL(image_capture_client_, OnCorrectSetPhotoOptions(true)).Times(1);
  device_->SetPhotoOptions(std::move(photo_settings),
                           std::move(scoped_set_callback));
}

TEST_F(FileVideoCaptureDeviceTest, TakePhoto) {
  VideoCaptureDevice::TakePhotoCallback scoped_callback =
      base::BindOnce(&MockImageCaptureClient::DoOnPhotoTaken,
                     base::Unretained(&image_capture_client_));

  base::RunLoop run_loop;
  base::Closure quit_closure = BindToCurrentLoop(run_loop.QuitClosure());
  EXPECT_CALL(image_capture_client_, OnCorrectPhotoTaken())
      .Times(1)
      .WillOnce(InvokeWithoutArgs([quit_closure]() { quit_closure.Run(); }));
  device_->TakePhoto(std::move(scoped_callback));
  run_loop.Run();
}

}  // namespace media
