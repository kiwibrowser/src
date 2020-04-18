// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/windows/d3d11_video_decoder.h"

#include <d3d11.h>
#include <d3d11_1.h>
#include <initguid.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/decoder_buffer.h"
#include "media/base/test_helpers.h"
#include "media/gpu/windows/d3d11_mocks.h"
#include "media/gpu/windows/d3d11_video_decoder_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace media {

class MockD3D11VideoDecoderImpl : public D3D11VideoDecoderImpl {
 public:
  MockD3D11VideoDecoderImpl()
      : D3D11VideoDecoderImpl(
            base::RepeatingCallback<gpu::CommandBufferStub*()>()) {}

  MOCK_METHOD6(
      Initialize,
      void(const VideoDecoderConfig& config,
           bool low_delay,
           CdmContext* cdm_context,
           const InitCB& init_cb,
           const OutputCB& output_cb,
           const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb));

  MOCK_METHOD2(Decode,
               void(scoped_refptr<DecoderBuffer> buffer,
                    const DecodeCB& decode_cb));
  MOCK_METHOD1(Reset, void(const base::RepeatingClosure& closure));
};

class D3D11VideoDecoderTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Set up some sane defaults.
    gpu_preferences_.enable_zero_copy_dxgi_video = true;
    gpu_preferences_.use_passthrough_cmd_decoder = false;
    gpu_workarounds_.disable_dxgi_zero_copy_video = false;
    supported_config_ = TestVideoConfig::NormalH264(H264PROFILE_MAIN);
  }

  void TearDown() override {
    decoder_.reset();
    // Run the gpu thread runner to tear down |impl_|.
    base::RunLoop().RunUntilIdle();
  }

  void CreateDecoder() {
    std::unique_ptr<MockD3D11VideoDecoderImpl> impl =
        std::make_unique<MockD3D11VideoDecoderImpl>();
    impl_ = impl.get();

    gpu_task_runner_ = base::ThreadTaskRunnerHandle::Get();

    decoder_ = base::WrapUnique<VideoDecoder>(new D3D11VideoDecoder(
        gpu_task_runner_, gpu_preferences_, gpu_workarounds_, std::move(impl)));
  }

  enum InitExpectation {
    kExpectFailure = false,
    kExpectSuccess = true,
  };

  void InitializeDecoder(const VideoDecoderConfig& config,
                         InitExpectation expectation) {
    const bool low_delay = false;
    CdmContext* cdm_context = nullptr;

    if (expectation == kExpectSuccess) {
      EXPECT_CALL(*this, MockInitCB(_)).Times(0);
      EXPECT_CALL(*impl_, Initialize(_, low_delay, cdm_context, _, _, _));
    } else {
      EXPECT_CALL(*this, MockInitCB(false));
    }

    decoder_->Initialize(config, low_delay, cdm_context,
                         base::BindRepeating(&D3D11VideoDecoderTest::MockInitCB,
                                             base::Unretained(this)),
                         VideoDecoder::OutputCB(), base::NullCallback());
    base::RunLoop().RunUntilIdle();
  }

  base::test::ScopedTaskEnvironment env_;

  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;

  std::unique_ptr<VideoDecoder> decoder_;
  gpu::GpuPreferences gpu_preferences_;
  gpu::GpuDriverBugWorkarounds gpu_workarounds_;
  MockD3D11VideoDecoderImpl* impl_ = nullptr;
  VideoDecoderConfig supported_config_;

  MOCK_METHOD1(MockInitCB, void(bool));
};

TEST_F(D3D11VideoDecoderTest, SupportsH264) {
  CreateDecoder();
  // Make sure that we're testing H264.
  ASSERT_EQ(supported_config_.profile(), H264PROFILE_MAIN);
  InitializeDecoder(supported_config_, kExpectSuccess);
}

TEST_F(D3D11VideoDecoderTest, DoesNotSupportVP8) {
  CreateDecoder();
  InitializeDecoder(TestVideoConfig::Normal(kCodecVP8), kExpectFailure);
}

TEST_F(D3D11VideoDecoderTest, DoesNotSupportVP9) {
  CreateDecoder();
  InitializeDecoder(TestVideoConfig::Normal(kCodecVP9), kExpectFailure);
}

TEST_F(D3D11VideoDecoderTest, RequiresZeroCopyPreference) {
  gpu_preferences_.enable_zero_copy_dxgi_video = false;
  CreateDecoder();
  InitializeDecoder(supported_config_, kExpectFailure);
}

TEST_F(D3D11VideoDecoderTest, FailsIfUsingPassthroughDecoder) {
  gpu_preferences_.use_passthrough_cmd_decoder = true;
  CreateDecoder();
  InitializeDecoder(supported_config_, kExpectFailure);
}

TEST_F(D3D11VideoDecoderTest, FailsIfZeroCopyWorkaround) {
  gpu_workarounds_.disable_dxgi_zero_copy_video = true;
  CreateDecoder();
  InitializeDecoder(supported_config_, kExpectFailure);
}

}  // namespace media
