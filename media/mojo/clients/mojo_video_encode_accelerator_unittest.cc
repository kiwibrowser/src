// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "gpu/config/gpu_info.h"
#include "media/mojo/clients/mojo_video_encode_accelerator.h"
#include "media/mojo/interfaces/video_encode_accelerator.mojom.h"
#include "media/video/video_encode_accelerator.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::InSequence;

namespace media {

static const gfx::Size kInputVisibleSize(64, 48);

// Mock implementation of the Mojo "service" side of the VEA dialogue. Upon an
// Initialize() call, checks |initialization_success_| and responds to |client|
// with a RequireBitstreamBuffers() if so configured; upon Encode(), responds
// with a BitstreamBufferReady() with the bitstream buffer id previously
// configured by a call to UseOutputBitstreamBuffer(). This mock class only
// allows for one bitstream buffer in flight.
class MockMojoVideoEncodeAccelerator : public mojom::VideoEncodeAccelerator {
 public:
  MockMojoVideoEncodeAccelerator() = default;

  // mojom::VideoEncodeAccelerator impl.
  void Initialize(media::VideoPixelFormat input_format,
                  const gfx::Size& input_visible_size,
                  media::VideoCodecProfile output_profile,
                  uint32_t initial_bitrate,
                  mojom::VideoEncodeAcceleratorClientPtr client,
                  InitializeCallback success_callback) override {
    if (initialization_success_) {
      ASSERT_TRUE(client);
      client_ = std::move(client);
      const size_t allocation_size =
          VideoFrame::AllocationSize(input_format, input_visible_size);

      client_->RequireBitstreamBuffers(1, input_visible_size, allocation_size);

      DoInitialize(input_format, input_visible_size, output_profile,
                   initial_bitrate, &client);
    }
    std::move(success_callback).Run(initialization_success_);
  }
  MOCK_METHOD5(DoInitialize,
               void(media::VideoPixelFormat,
                    const gfx::Size&,
                    media::VideoCodecProfile,
                    uint32_t,
                    mojom::VideoEncodeAcceleratorClientPtr*));

  void Encode(const scoped_refptr<VideoFrame>& frame,
              bool keyframe,
              EncodeCallback callback) override {
    EXPECT_NE(-1, configured_bitstream_buffer_id_);
    EXPECT_TRUE(client_);
    client_->BitstreamBufferReady(configured_bitstream_buffer_id_, 0, keyframe,
                                  frame->timestamp());
    configured_bitstream_buffer_id_ = -1;

    DoEncode(frame, keyframe);
    std::move(callback).Run();
  }
  MOCK_METHOD2(DoEncode, void(const scoped_refptr<VideoFrame>&, bool));

  void UseOutputBitstreamBuffer(
      int32_t bitstream_buffer_id,
      mojo::ScopedSharedBufferHandle buffer) override {
    EXPECT_EQ(-1, configured_bitstream_buffer_id_);
    configured_bitstream_buffer_id_ = bitstream_buffer_id;

    DoUseOutputBitstreamBuffer(bitstream_buffer_id, &buffer);
  }
  MOCK_METHOD2(DoUseOutputBitstreamBuffer,
               void(int32_t, mojo::ScopedSharedBufferHandle*));

  MOCK_METHOD2(RequestEncodingParametersChange, void(uint32_t, uint32_t));

  void set_initialization_success(bool success) {
    initialization_success_ = success;
  }

 private:
  mojom::VideoEncodeAcceleratorClientPtr client_;
  int32_t configured_bitstream_buffer_id_ = -1;
  bool initialization_success_ = true;

  DISALLOW_COPY_AND_ASSIGN(MockMojoVideoEncodeAccelerator);
};

// Mock implementation of the client of MojoVideoEncodeAccelerator.
class MockVideoEncodeAcceleratorClient : public VideoEncodeAccelerator::Client {
 public:
  MockVideoEncodeAcceleratorClient() = default;

  MOCK_METHOD3(RequireBitstreamBuffers,
               void(unsigned int, const gfx::Size&, size_t));
  MOCK_METHOD4(BitstreamBufferReady,
               void(int32_t, size_t, bool, base::TimeDelta));
  MOCK_METHOD1(NotifyError, void(VideoEncodeAccelerator::Error));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockVideoEncodeAcceleratorClient);
};

// Test wrapper for a MojoVideoEncodeAccelerator, which translates between a
// pipe to a remote mojom::MojoVideoEncodeAccelerator, and a local
// media::VideoEncodeAccelerator::Client.
class MojoVideoEncodeAcceleratorTest : public ::testing::Test {
 public:
  MojoVideoEncodeAcceleratorTest() = default;

  void SetUp() override {
    mojom::VideoEncodeAcceleratorPtr mojo_vea;
    mojo_vea_binding_ = mojo::MakeStrongBinding(
        std::make_unique<MockMojoVideoEncodeAccelerator>(),
        mojo::MakeRequest(&mojo_vea));

    mojo_vea_.reset(new MojoVideoEncodeAccelerator(
        std::move(mojo_vea), gpu::VideoEncodeAcceleratorSupportedProfiles()));
  }

  void TearDown() override {
    // The destruction of a mojo::StrongBinding closes the bound message pipe
    // but does not destroy the implementation object(s): this needs to happen
    // manually by Close()ing it.
    mojo_vea_binding_->Close();
  }

  MockMojoVideoEncodeAccelerator* mock_mojo_vea() {
    return static_cast<media::MockMojoVideoEncodeAccelerator*>(
        mojo_vea_binding_->impl());
  }
  VideoEncodeAccelerator* mojo_vea() { return mojo_vea_.get(); }

  // This method calls Initialize() with semantically correct parameters and
  // verifies that the appropriate message goes through the mojo pipe and is
  // responded by a RequireBitstreamBuffers() on |mock_vea_client|.
  void Initialize(MockVideoEncodeAcceleratorClient* mock_vea_client) {
    const VideoCodecProfile kOutputProfile = VIDEO_CODEC_PROFILE_UNKNOWN;
    const uint32_t kInitialBitrate = 100000u;

    EXPECT_CALL(*mock_mojo_vea(),
                DoInitialize(PIXEL_FORMAT_I420, kInputVisibleSize,
                             kOutputProfile, kInitialBitrate, _));
    EXPECT_CALL(
        *mock_vea_client,
        RequireBitstreamBuffers(
            _, kInputVisibleSize,
            VideoFrame::AllocationSize(PIXEL_FORMAT_I420, kInputVisibleSize)));

    EXPECT_TRUE(mojo_vea()->Initialize(PIXEL_FORMAT_I420, kInputVisibleSize,
                                       kOutputProfile, kInitialBitrate,
                                       mock_vea_client));
    base::RunLoop().RunUntilIdle();
  }

 private:
  const base::MessageLoop message_loop_;

  // This member holds on to the mock implementation of the "service" side.
  mojo::StrongBindingPtr<mojom::VideoEncodeAccelerator> mojo_vea_binding_;

  // The class under test, as a generic media::VideoEncodeAccelerator.
  std::unique_ptr<VideoEncodeAccelerator> mojo_vea_;

  DISALLOW_COPY_AND_ASSIGN(MojoVideoEncodeAcceleratorTest);
};

TEST_F(MojoVideoEncodeAcceleratorTest, CreateAndDestroy) {}

// This test verifies the Initialize() communication prologue in isolation.
TEST_F(MojoVideoEncodeAcceleratorTest, InitializeAndRequireBistreamBuffers) {
  std::unique_ptr<MockVideoEncodeAcceleratorClient> mock_vea_client =
      std::make_unique<MockVideoEncodeAcceleratorClient>();
  Initialize(mock_vea_client.get());
}

// This test verifies the Initialize() communication prologue followed by a
// sharing of a single bitstream buffer and the Encode() of one frame.
TEST_F(MojoVideoEncodeAcceleratorTest, EncodeOneFrame) {
  std::unique_ptr<MockVideoEncodeAcceleratorClient> mock_vea_client =
      std::make_unique<MockVideoEncodeAcceleratorClient>();
  Initialize(mock_vea_client.get());

  const int32_t kBitstreamBufferId = 17;
  {
    const int32_t kShMemSize = 10;
    base::SharedMemory shmem;
    shmem.CreateAnonymous(kShMemSize);
    EXPECT_CALL(*mock_mojo_vea(),
                DoUseOutputBitstreamBuffer(kBitstreamBufferId, _));
    mojo_vea()->UseOutputBitstreamBuffer(
        BitstreamBuffer(kBitstreamBufferId, shmem.handle(), kShMemSize,
                        0 /* offset */, base::TimeDelta()));
    base::RunLoop().RunUntilIdle();
  }

  {
    const scoped_refptr<VideoFrame> video_frame = VideoFrame::CreateFrame(
        PIXEL_FORMAT_I420, kInputVisibleSize, gfx::Rect(kInputVisibleSize),
        kInputVisibleSize, base::TimeDelta());
    base::SharedMemory shmem;
    shmem.CreateAnonymous(
        VideoFrame::AllocationSize(PIXEL_FORMAT_I420, kInputVisibleSize) * 2);
    video_frame->AddSharedMemoryHandle(shmem.handle());
    const bool is_keyframe = true;

    // The remote end of the mojo Pipe doesn't receive |video_frame| itself.
    EXPECT_CALL(*mock_mojo_vea(), DoEncode(_, is_keyframe));
    EXPECT_CALL(*mock_vea_client,
                BitstreamBufferReady(kBitstreamBufferId, _, is_keyframe,
                                     video_frame->timestamp()));

    mojo_vea()->Encode(video_frame, is_keyframe);
    base::RunLoop().RunUntilIdle();
  }
}

// Tests that a RequestEncodingParametersChange() ripples through correctly.
TEST_F(MojoVideoEncodeAcceleratorTest, EncodingParametersChange) {
  const uint32_t kNewBitrate = 123123u;
  const uint32_t kNewFramerate = 321321u;

  // In a real world scenario, we should go through an Initialize() prologue,
  // but we can skip that in unit testing.

  EXPECT_CALL(*mock_mojo_vea(),
              RequestEncodingParametersChange(kNewBitrate, kNewFramerate));
  mojo_vea()->RequestEncodingParametersChange(kNewBitrate, kNewFramerate);
  base::RunLoop().RunUntilIdle();
}

// This test verifies the Initialize() communication prologue fails when the
// FakeVEA is configured to do so.
TEST_F(MojoVideoEncodeAcceleratorTest, InitializeFailure) {
  std::unique_ptr<MockVideoEncodeAcceleratorClient> mock_vea_client =
      std::make_unique<MockVideoEncodeAcceleratorClient>();

  const uint32_t kInitialBitrate = 100000u;

  mock_mojo_vea()->set_initialization_success(false);

  EXPECT_FALSE(mojo_vea()->Initialize(PIXEL_FORMAT_I420, kInputVisibleSize,
                                      VIDEO_CODEC_PROFILE_UNKNOWN,
                                      kInitialBitrate, mock_vea_client.get()));
  base::RunLoop().RunUntilIdle();
}

}  // namespace media
