// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "media/mojo/interfaces/video_encode_accelerator.mojom.h"
#include "media/mojo/services/mojo_video_encode_accelerator_service.h"
#include "media/video/fake_video_encode_accelerator.h"
#include "media/video/video_encode_accelerator.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {
struct GpuPreferences;
}  // namespace gpu

using ::testing::_;

namespace media {

static const gfx::Size kInputVisibleSize(64, 48);

std::unique_ptr<VideoEncodeAccelerator> CreateAndInitializeFakeVEA(
    bool will_initialization_succeed,
    VideoPixelFormat input_format,
    const gfx::Size& input_visible_size,
    VideoCodecProfile output_profile,
    uint32_t initial_bitrate,
    VideoEncodeAccelerator::Client* client,
    const gpu::GpuPreferences& gpu_preferences) {
  // Use FakeVEA as scoped_ptr to guarantee proper destruction via Destroy().
  auto vea = std::make_unique<FakeVideoEncodeAccelerator>(
      base::ThreadTaskRunnerHandle::Get());
  vea->SetWillInitializationSucceed(will_initialization_succeed);
  const bool result = vea->Initialize(input_format, input_visible_size,
                                      output_profile, initial_bitrate, client);

  // Mimic the behaviour of GpuVideoEncodeAcceleratorFactory::CreateVEA().
  return result ? base::WrapUnique<VideoEncodeAccelerator>(vea.release())
                : nullptr;
}

class MockMojoVideoEncodeAcceleratorClient
    : public mojom::VideoEncodeAcceleratorClient {
 public:
  MockMojoVideoEncodeAcceleratorClient() = default;

  MOCK_METHOD3(RequireBitstreamBuffers,
               void(uint32_t, const gfx::Size&, uint32_t));
  MOCK_METHOD4(BitstreamBufferReady,
               void(int32_t, uint32_t, bool, base::TimeDelta));
  MOCK_METHOD1(NotifyError, void(VideoEncodeAccelerator::Error));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockMojoVideoEncodeAcceleratorClient);
};

// Test harness for a MojoVideoEncodeAcceleratorService; the tests manipulate it
// via its MojoVideoEncodeAcceleratorService interface while observing a
// "remote" mojo::VideoEncodeAcceleratorClient (that we keep inside a Mojo
// binding). The class under test uses a FakeVideoEncodeAccelerator as
// implementation.
class MojoVideoEncodeAcceleratorServiceTest : public ::testing::Test {
 public:
  MojoVideoEncodeAcceleratorServiceTest() = default;

  void TearDown() override {
    // The destruction of a mojo::StrongBinding closes the bound message pipe
    // but does not destroy the implementation object: needs to happen manually,
    // otherwise we leak it. This only applies if BindAndInitialize() has been
    // called.
    if (mojo_vea_binding_)
      mojo_vea_binding_->Close();
  }

  // Creates the class under test, configuring the underlying FakeVEA to succeed
  // upon initialization (by default) or not.
  void CreateMojoVideoEncodeAccelerator(
      bool will_fake_vea_initialization_succeed = true) {
    mojo_vea_service_ = std::make_unique<MojoVideoEncodeAcceleratorService>(
        base::Bind(&CreateAndInitializeFakeVEA,
                   will_fake_vea_initialization_succeed),
        gpu::GpuPreferences());
  }

  void BindAndInitialize() {
    // Create an Mojo VEA Client InterfacePtr and point it to bind to our Mock.
    mojom::VideoEncodeAcceleratorClientPtr mojo_vea_client;
    mojo_vea_binding_ = mojo::MakeStrongBinding(
        std::make_unique<MockMojoVideoEncodeAcceleratorClient>(),
        mojo::MakeRequest(&mojo_vea_client));

    EXPECT_CALL(*mock_mojo_vea_client(),
                RequireBitstreamBuffers(_, kInputVisibleSize, _));

    const uint32_t kInitialBitrate = 100000u;
    mojo_vea_service()->Initialize(
        PIXEL_FORMAT_I420, kInputVisibleSize, H264PROFILE_MIN, kInitialBitrate,
        std::move(mojo_vea_client),
        base::Bind([](bool success) { ASSERT_TRUE(success); }));
    base::RunLoop().RunUntilIdle();
  }

  MojoVideoEncodeAcceleratorService* mojo_vea_service() {
    return mojo_vea_service_.get();
  }

  MockMojoVideoEncodeAcceleratorClient* mock_mojo_vea_client() const {
    return static_cast<media::MockMojoVideoEncodeAcceleratorClient*>(
        mojo_vea_binding_->impl());
  }

  FakeVideoEncodeAccelerator* fake_vea() const {
    return static_cast<FakeVideoEncodeAccelerator*>(
        mojo_vea_service_->encoder_.get());
  }

 private:
  const base::MessageLoop message_loop_;

  mojo::StrongBindingPtr<mojom::VideoEncodeAcceleratorClient> mojo_vea_binding_;

  // The class under test.
  std::unique_ptr<MojoVideoEncodeAcceleratorService> mojo_vea_service_;

  DISALLOW_COPY_AND_ASSIGN(MojoVideoEncodeAcceleratorServiceTest);
};

// This test verifies the BindAndInitialize() communication prologue in
// isolation.
TEST_F(MojoVideoEncodeAcceleratorServiceTest,
       InitializeAndRequireBistreamBuffers) {
  CreateMojoVideoEncodeAccelerator();
  BindAndInitialize();
}

// This test verifies the BindAndInitialize() communication prologue followed by
// a sharing of a single bitstream buffer and the Encode() of one frame.
TEST_F(MojoVideoEncodeAcceleratorServiceTest, EncodeOneFrame) {
  CreateMojoVideoEncodeAccelerator();
  BindAndInitialize();

  const int32_t kBitstreamBufferId = 17;
  {
    const uint64_t kShMemSize = fake_vea()->minimum_output_buffer_size();
    auto handle = mojo::SharedBufferHandle::Create(kShMemSize);

    mojo_vea_service()->UseOutputBitstreamBuffer(kBitstreamBufferId,
                                                 std::move(handle));
    base::RunLoop().RunUntilIdle();
  }

  {
    const auto video_frame = VideoFrame::CreateBlackFrame(kInputVisibleSize);
    EXPECT_CALL(*mock_mojo_vea_client(),
                BitstreamBufferReady(kBitstreamBufferId, _, _, _));

    mojo_vea_service()->Encode(video_frame, true /* is_keyframe */,
                               base::DoNothing());
    base::RunLoop().RunUntilIdle();
  }
}

// Tests that a RequestEncodingParametersChange() ripples through correctly.
TEST_F(MojoVideoEncodeAcceleratorServiceTest, EncodingParametersChange) {
  CreateMojoVideoEncodeAccelerator();
  BindAndInitialize();

  const uint32_t kNewBitrate = 123123u;
  const uint32_t kNewFramerate = 321321u;
  mojo_vea_service()->RequestEncodingParametersChange(kNewBitrate,
                                                      kNewFramerate);
  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(fake_vea());
  EXPECT_EQ(kNewBitrate, fake_vea()->stored_bitrates().front());
}

// This test verifies that MojoVEA::Initialize() fails with an invalid |client|.
TEST_F(MojoVideoEncodeAcceleratorServiceTest,
       InitializeWithInvalidClientFails) {
  CreateMojoVideoEncodeAccelerator();

  mojom::VideoEncodeAcceleratorClientPtr invalid_mojo_vea_client = nullptr;

  const uint32_t kInitialBitrate = 100000u;
  mojo_vea_service()->Initialize(
      PIXEL_FORMAT_I420, kInputVisibleSize, H264PROFILE_MIN, kInitialBitrate,
      std::move(invalid_mojo_vea_client),
      base::Bind([](bool success) { ASSERT_FALSE(success); }));
  base::RunLoop().RunUntilIdle();
}

// This test verifies that when FakeVEA is configured to fail upon start,
// MojoVEA::Initialize() causes a NotifyError().
TEST_F(MojoVideoEncodeAcceleratorServiceTest, InitializeFailure) {
  CreateMojoVideoEncodeAccelerator(
      false /* will_fake_vea_initialization_succeed */);

  mojom::VideoEncodeAcceleratorClientPtr mojo_vea_client;
  auto mojo_vea_binding = mojo::MakeStrongBinding(
      std::make_unique<MockMojoVideoEncodeAcceleratorClient>(),
      mojo::MakeRequest(&mojo_vea_client));

  const uint32_t kInitialBitrate = 100000u;
  mojo_vea_service()->Initialize(
      PIXEL_FORMAT_I420, kInputVisibleSize, H264PROFILE_MIN, kInitialBitrate,
      std::move(mojo_vea_client),
      base::Bind([](bool success) { ASSERT_FALSE(success); }));
  base::RunLoop().RunUntilIdle();

  mojo_vea_binding->Close();
}

// This test verifies that UseOutputBitstreamBuffer() with a wrong ShMem size
// causes NotifyError().
TEST_F(MojoVideoEncodeAcceleratorServiceTest,
       UseOutputBitstreamBufferWithWrongSizeFails) {
  CreateMojoVideoEncodeAccelerator();
  BindAndInitialize();

  const int32_t kBitstreamBufferId = 17;
  const uint64_t wrong_size = fake_vea()->minimum_output_buffer_size() / 2;
  auto handle = mojo::SharedBufferHandle::Create(wrong_size);

  EXPECT_CALL(*mock_mojo_vea_client(),
              NotifyError(VideoEncodeAccelerator::kInvalidArgumentError));

  mojo_vea_service()->UseOutputBitstreamBuffer(kBitstreamBufferId,
                                               std::move(handle));
  base::RunLoop().RunUntilIdle();
}

// This test verifies that Encode() with wrong coded size causes NotifyError().
TEST_F(MojoVideoEncodeAcceleratorServiceTest, EncodeWithWrongSizeFails) {
  CreateMojoVideoEncodeAccelerator();
  BindAndInitialize();

  // We should send a UseOutputBitstreamBuffer() first but in unit tests we can
  // skip that prologue.

  const gfx::Size wrong_size(kInputVisibleSize.width() / 2,
                             kInputVisibleSize.height() / 2);
  const auto video_frame = VideoFrame::CreateBlackFrame(wrong_size);

  EXPECT_CALL(*mock_mojo_vea_client(),
              NotifyError(VideoEncodeAccelerator::kInvalidArgumentError));

  mojo_vea_service()->Encode(video_frame, true /* is_keyframe */,
                             base::DoNothing());
  base::RunLoop().RunUntilIdle();
}

// This test verifies that an any mojom::VEA method call (e.g. Encode(),
// UseOutputBitstreamBuffer() etc) before MojoVEA::Initialize() is ignored (we
// can't expect NotifyError()s since there's no mojo client registered).
TEST_F(MojoVideoEncodeAcceleratorServiceTest, CallsBeforeInitializeAreIgnored) {
  CreateMojoVideoEncodeAccelerator();
  {
    const auto video_frame = VideoFrame::CreateBlackFrame(kInputVisibleSize);
    mojo_vea_service()->Encode(video_frame, true /* is_keyframe */,
                               base::DoNothing());
    base::RunLoop().RunUntilIdle();
  }
  {
    const int32_t kBitstreamBufferId = 17;
    const uint64_t kShMemSize = 10;
    auto handle = mojo::SharedBufferHandle::Create(kShMemSize);
    mojo_vea_service()->UseOutputBitstreamBuffer(kBitstreamBufferId,
                                                 std::move(handle));
    base::RunLoop().RunUntilIdle();
  }
  {
    const uint32_t kNewBitrate = 123123u;
    const uint32_t kNewFramerate = 321321u;
    mojo_vea_service()->RequestEncodingParametersChange(kNewBitrate,
                                                        kNewFramerate);
    base::RunLoop().RunUntilIdle();
  }
}

}  // namespace media
