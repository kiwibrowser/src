// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/ipc/service/vda_video_decoder.h"

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/time.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "media/base/decode_status.h"
#include "media/base/decoder_buffer.h"
#include "media/base/media_util.h"
#include "media/base/mock_media_log.h"
#include "media/base/video_codecs.h"
#include "media/base/video_frame.h"
#include "media/base/video_rotation.h"
#include "media/base/video_types.h"
#include "media/gpu/fake_command_buffer_helper.h"
#include "media/gpu/ipc/service/picture_buffer_manager.h"
#include "media/video/mock_video_decode_accelerator.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;

namespace media {

namespace {

constexpr uint8_t kData[] = "foo";
constexpr size_t kDataSize = arraysize(kData);

scoped_refptr<DecoderBuffer> CreateDecoderBuffer(base::TimeDelta timestamp) {
  scoped_refptr<DecoderBuffer> buffer =
      DecoderBuffer::CopyFrom(kData, kDataSize);
  buffer->set_timestamp(timestamp);
  return buffer;
}

// TODO(sandersd): Should be part of //media, as it is used by
// MojoVideoDecoderService (production code) as well.
class StaticSyncTokenClient : public VideoFrame::SyncTokenClient {
 public:
  explicit StaticSyncTokenClient(const gpu::SyncToken& sync_token)
      : sync_token_(sync_token) {}

  void GenerateSyncToken(gpu::SyncToken* sync_token) final {
    *sync_token = sync_token_;
  }

  void WaitSyncToken(const gpu::SyncToken& sync_token) final {}

 private:
  gpu::SyncToken sync_token_;

  DISALLOW_COPY_AND_ASSIGN(StaticSyncTokenClient);
};

VideoDecodeAccelerator::SupportedProfiles GetSupportedProfiles() {
  VideoDecodeAccelerator::SupportedProfiles profiles;
  {
    VideoDecodeAccelerator::SupportedProfile profile;
    profile.profile = VP9PROFILE_PROFILE0;
    profile.max_resolution = gfx::Size(1920, 1088);
    profile.min_resolution = gfx::Size(640, 480);
    profile.encrypted_only = false;
    profiles.push_back(std::move(profile));
  }
  return profiles;
}

VideoDecodeAccelerator::Capabilities GetCapabilities() {
  VideoDecodeAccelerator::Capabilities capabilities;
  capabilities.supported_profiles = GetSupportedProfiles();
  capabilities.flags = 0;
  return capabilities;
}

}  // namespace

class VdaVideoDecoderTest : public testing::Test {
 public:
  explicit VdaVideoDecoderTest() {
    // TODO(sandersd): Use a separate thread for the GPU task runner.
    scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        environment_.GetMainThreadTaskRunner();
    cbh_ = base::MakeRefCounted<FakeCommandBufferHelper>(task_runner);

    // |owned_vda_| exists to delete |vda_| when |this| is destructed. Ownership
    // is passed to |vdavd_| by CreateVda(), but |vda_| remains to be used for
    // configuring mock expectations.
    vda_ = new testing::StrictMock<MockVideoDecodeAccelerator>();
    owned_vda_.reset(vda_);

    // In either case, vda_->Destroy() should be called once.
    EXPECT_CALL(*vda_, Destroy());

    vdavd_.reset(new VdaVideoDecoder(
        task_runner, task_runner, &media_log_, gfx::ColorSpace(),
        base::BindOnce(&VdaVideoDecoderTest::CreatePictureBufferManager,
                       base::Unretained(this)),
        base::BindOnce(&VdaVideoDecoderTest::CreateCommandBufferHelper,
                       base::Unretained(this)),
        base::BindOnce(&VdaVideoDecoderTest::CreateAndInitializeVda,
                       base::Unretained(this)),
        GetCapabilities()));
    client_ = vdavd_.get();
  }

  ~VdaVideoDecoderTest() override {
    // Drop ownership of anything that may have an async destruction process,
    // then allow destruction to complete.
    cbh_->StubLost();
    cbh_ = nullptr;
    owned_vda_ = nullptr;
    pbm_ = nullptr;
    vdavd_ = nullptr;
    environment_.RunUntilIdle();
  }

 protected:
  void InitializeWithConfig(const VideoDecoderConfig& config) {
    vdavd_->Initialize(config, false, nullptr, init_cb_.Get(), output_cb_.Get(),
                       waiting_cb_.Get());
  }

  void Initialize() {
    InitializeWithConfig(VideoDecoderConfig(
        kCodecVP9, VP9PROFILE_PROFILE0, PIXEL_FORMAT_I420,
        COLOR_SPACE_HD_REC709, VIDEO_ROTATION_0, gfx::Size(1920, 1088),
        gfx::Rect(1920, 1080), gfx::Size(1920, 1080), EmptyExtraData(),
        Unencrypted()));

    EXPECT_CALL(*vda_, Initialize(_, vdavd_.get())).WillOnce(Return(true));
    EXPECT_CALL(init_cb_, Run(true));
    environment_.RunUntilIdle();
  }

  int32_t ProvidePictureBuffer() {
    std::vector<PictureBuffer> picture_buffers;
    client_->ProvidePictureBuffers(1, PIXEL_FORMAT_XRGB, 1,
                                   gfx::Size(1920, 1088), GL_TEXTURE_2D);
    EXPECT_CALL(*vda_, AssignPictureBuffers(_))
        .WillOnce(SaveArg<0>(&picture_buffers));
    environment_.RunUntilIdle();

    DCHECK_EQ(picture_buffers.size(), 1U);
    return picture_buffers[0].id();
  }

  int32_t Decode(base::TimeDelta timestamp) {
    int32_t bitstream_id = 0;
    vdavd_->Decode(CreateDecoderBuffer(timestamp), decode_cb_.Get());
    EXPECT_CALL(*vda_, Decode(_, _)).WillOnce(SaveArg<1>(&bitstream_id));
    environment_.RunUntilIdle();
    return bitstream_id;
  }

  void NotifyEndOfBitstreamBuffer(int32_t bitstream_id) {
    // Expectation must go before the call because NotifyEndOfBitstreamBuffer()
    // implements the same-thread optimization.
    EXPECT_CALL(decode_cb_, Run(DecodeStatus::OK));
    client_->NotifyEndOfBitstreamBuffer(bitstream_id);
    environment_.RunUntilIdle();
  }

  scoped_refptr<VideoFrame> PictureReady(int32_t bitstream_buffer_id,
                                         int32_t picture_buffer_id) {
    // Expectation must go before the call because PictureReady() implements the
    // same-thread optimization.
    scoped_refptr<VideoFrame> frame;
    EXPECT_CALL(output_cb_, Run(_)).WillOnce(SaveArg<0>(&frame));
    client_->PictureReady(Picture(picture_buffer_id, bitstream_buffer_id,
                                  gfx::Rect(1920, 1080),
                                  gfx::ColorSpace::CreateSRGB(), true));
    environment_.RunUntilIdle();
    return frame;
  }

  // TODO(sandersd): This exact code is also used in
  // PictureBufferManagerImplTest. Share the implementation.
  gpu::SyncToken GenerateSyncToken(scoped_refptr<VideoFrame> video_frame) {
    gpu::SyncToken sync_token(gpu::GPU_IO,
                              gpu::CommandBufferId::FromUnsafeValue(1),
                              next_release_count_++);
    StaticSyncTokenClient sync_token_client(sync_token);
    video_frame->UpdateReleaseSyncToken(&sync_token_client);
    return sync_token;
  }

  scoped_refptr<CommandBufferHelper> CreateCommandBufferHelper() {
    return cbh_;
  }

  scoped_refptr<PictureBufferManager> CreatePictureBufferManager(
      PictureBufferManager::ReusePictureBufferCB reuse_cb) {
    DCHECK(!pbm_);
    pbm_ = PictureBufferManager::Create(std::move(reuse_cb));
    return pbm_;
  }

  std::unique_ptr<VideoDecodeAccelerator> CreateAndInitializeVda(
      scoped_refptr<CommandBufferHelper> command_buffer_helper,
      VideoDecodeAccelerator::Client* client,
      MediaLog* media_log,
      const VideoDecodeAccelerator::Config& config) {
    DCHECK(owned_vda_);
    if (!owned_vda_->Initialize(config, client))
      return nullptr;
    return std::move(owned_vda_);
  }

  base::test::ScopedTaskEnvironment environment_;

  testing::NiceMock<MockMediaLog> media_log_;
  testing::StrictMock<base::MockCallback<VideoDecoder::InitCB>> init_cb_;
  testing::StrictMock<base::MockCallback<VideoDecoder::OutputCB>> output_cb_;
  testing::StrictMock<
      base::MockCallback<VideoDecoder::WaitingForDecryptionKeyCB>>
      waiting_cb_;
  testing::StrictMock<base::MockCallback<VideoDecoder::DecodeCB>> decode_cb_;
  testing::StrictMock<base::MockCallback<base::RepeatingClosure>> reset_cb_;

  scoped_refptr<FakeCommandBufferHelper> cbh_;
  testing::StrictMock<MockVideoDecodeAccelerator>* vda_;
  std::unique_ptr<VideoDecodeAccelerator> owned_vda_;
  scoped_refptr<PictureBufferManager> pbm_;
  std::unique_ptr<VdaVideoDecoder, std::default_delete<VideoDecoder>> vdavd_;

  VideoDecodeAccelerator::Client* client_;
  uint64_t next_release_count_ = 1;

  DISALLOW_COPY_AND_ASSIGN(VdaVideoDecoderTest);
};

TEST_F(VdaVideoDecoderTest, CreateAndDestroy) {}

TEST_F(VdaVideoDecoderTest, Initialize) {
  Initialize();
}

TEST_F(VdaVideoDecoderTest, Initialize_UnsupportedSize) {
  InitializeWithConfig(VideoDecoderConfig(
      kCodecVP9, VP9PROFILE_PROFILE0, PIXEL_FORMAT_I420, COLOR_SPACE_SD_REC601,
      VIDEO_ROTATION_0, gfx::Size(320, 240), gfx::Rect(320, 240),
      gfx::Size(320, 240), EmptyExtraData(), Unencrypted()));
  EXPECT_CALL(init_cb_, Run(false));
  environment_.RunUntilIdle();
}

TEST_F(VdaVideoDecoderTest, Initialize_UnsupportedCodec) {
  InitializeWithConfig(VideoDecoderConfig(
      kCodecH264, H264PROFILE_BASELINE, PIXEL_FORMAT_I420,
      COLOR_SPACE_HD_REC709, VIDEO_ROTATION_0, gfx::Size(1920, 1088),
      gfx::Rect(1920, 1080), gfx::Size(1920, 1080), EmptyExtraData(),
      Unencrypted()));
  EXPECT_CALL(init_cb_, Run(false));
  environment_.RunUntilIdle();
}

TEST_F(VdaVideoDecoderTest, Initialize_RejectedByVda) {
  InitializeWithConfig(VideoDecoderConfig(
      kCodecVP9, VP9PROFILE_PROFILE0, PIXEL_FORMAT_I420, COLOR_SPACE_HD_REC709,
      VIDEO_ROTATION_0, gfx::Size(1920, 1088), gfx::Rect(1920, 1080),
      gfx::Size(1920, 1080), EmptyExtraData(), Unencrypted()));

  EXPECT_CALL(*vda_, Initialize(_, vdavd_.get())).WillOnce(Return(false));
  EXPECT_CALL(init_cb_, Run(false));
  environment_.RunUntilIdle();
}

TEST_F(VdaVideoDecoderTest, ProvideAndDismissPictureBuffer) {
  Initialize();
  int32_t id = ProvidePictureBuffer();
  client_->DismissPictureBuffer(id);
  environment_.RunUntilIdle();
}

TEST_F(VdaVideoDecoderTest, Decode) {
  Initialize();
  int32_t bitstream_id = Decode(base::TimeDelta());
  NotifyEndOfBitstreamBuffer(bitstream_id);
}

TEST_F(VdaVideoDecoderTest, Decode_Reset) {
  Initialize();
  Decode(base::TimeDelta());

  vdavd_->Reset(reset_cb_.Get());
  EXPECT_CALL(*vda_, Reset());
  environment_.RunUntilIdle();

  client_->NotifyResetDone();
  EXPECT_CALL(decode_cb_, Run(DecodeStatus::ABORTED));
  EXPECT_CALL(reset_cb_, Run());
  environment_.RunUntilIdle();
}

TEST_F(VdaVideoDecoderTest, Decode_NotifyError) {
  Initialize();
  Decode(base::TimeDelta());

  client_->NotifyError(VideoDecodeAccelerator::PLATFORM_FAILURE);
  EXPECT_CALL(decode_cb_, Run(DecodeStatus::DECODE_ERROR));
  environment_.RunUntilIdle();
}

TEST_F(VdaVideoDecoderTest, Decode_OutputAndReuse) {
  Initialize();
  int32_t bitstream_id = Decode(base::TimeDelta());
  NotifyEndOfBitstreamBuffer(bitstream_id);
  int32_t picture_buffer_id = ProvidePictureBuffer();
  scoped_refptr<VideoFrame> frame =
      PictureReady(bitstream_id, picture_buffer_id);

  // Dropping the frame triggers reuse, which will wait on the SyncPoint.
  gpu::SyncToken sync_token = GenerateSyncToken(frame);
  frame = nullptr;
  environment_.RunUntilIdle();

  // But the VDA won't be notified until the SyncPoint wait completes.
  EXPECT_CALL(*vda_, ReusePictureBuffer(picture_buffer_id));
  cbh_->ReleaseSyncToken(sync_token);
  environment_.RunUntilIdle();
}

TEST_F(VdaVideoDecoderTest, Decode_OutputAndDismiss) {
  Initialize();
  int32_t bitstream_id = Decode(base::TimeDelta());
  NotifyEndOfBitstreamBuffer(bitstream_id);
  int32_t picture_buffer_id = ProvidePictureBuffer();
  scoped_refptr<VideoFrame> frame =
      PictureReady(bitstream_id, picture_buffer_id);

  client_->DismissPictureBuffer(picture_buffer_id);
  environment_.RunUntilIdle();

  // Dropping the frame still requires a SyncPoint to wait on.
  gpu::SyncToken sync_token = GenerateSyncToken(frame);
  frame = nullptr;
  environment_.RunUntilIdle();

  // But the VDA should not be notified when it completes.
  cbh_->ReleaseSyncToken(sync_token);
  environment_.RunUntilIdle();
}

TEST_F(VdaVideoDecoderTest, Decode_Output_MaintainsAspect) {
  // Initialize with a config that has a 2:1 pixel aspect ratio.
  InitializeWithConfig(VideoDecoderConfig(
      kCodecVP9, VP9PROFILE_PROFILE0, PIXEL_FORMAT_I420, COLOR_SPACE_HD_REC709,
      VIDEO_ROTATION_0, gfx::Size(640, 480), gfx::Rect(640, 480),
      gfx::Size(1280, 480), EmptyExtraData(), Unencrypted()));
  EXPECT_CALL(*vda_, Initialize(_, vdavd_.get())).WillOnce(Return(true));
  EXPECT_CALL(init_cb_, Run(true));
  environment_.RunUntilIdle();

  // Assign a picture buffer that has size 1920x1088.
  int32_t picture_buffer_id = ProvidePictureBuffer();

  // Produce a frame that has visible size 320x240.
  int32_t bitstream_id = Decode(base::TimeDelta());
  NotifyEndOfBitstreamBuffer(bitstream_id);

  scoped_refptr<VideoFrame> frame;
  EXPECT_CALL(output_cb_, Run(_)).WillOnce(SaveArg<0>(&frame));
  client_->PictureReady(Picture(picture_buffer_id, bitstream_id,
                                gfx::Rect(320, 240),
                                gfx::ColorSpace::CreateSRGB(), true));
  environment_.RunUntilIdle();

  // The frame should have |natural_size| 640x240 (pixel aspect ratio
  // preserved).
  ASSERT_TRUE(frame);
  EXPECT_EQ(frame->natural_size(), gfx::Size(640, 240));
  EXPECT_EQ(frame->coded_size(), gfx::Size(1920, 1088));
  EXPECT_EQ(frame->visible_rect(), gfx::Rect(320, 240));
}

TEST_F(VdaVideoDecoderTest, Flush) {
  Initialize();
  vdavd_->Decode(DecoderBuffer::CreateEOSBuffer(), decode_cb_.Get());
  EXPECT_CALL(*vda_, Flush());
  environment_.RunUntilIdle();

  client_->NotifyFlushDone();
  EXPECT_CALL(decode_cb_, Run(DecodeStatus::OK));
  environment_.RunUntilIdle();
}

}  // namespace media
