// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/common/raster_cmd_format.h"
#include "gpu/command_buffer/service/query_manager.h"
#include "gpu/command_buffer/service/raster_decoder_unittest_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_mock.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArrayArgument;

namespace gpu {
namespace raster {

using namespace cmds;

class RasterDecoderOOMTest : public RasterDecoderManualInitTest {
 protected:
  void Init(bool has_robustness) {
    InitState init;
    init.lose_context_when_out_of_memory = true;
    init.workarounds.simulate_out_of_memory_on_large_textures = true;
    if (has_robustness) {
      init.extensions.push_back("GL_ARB_robustness");
    }
    InitDecoder(init);
  }

  void OOM(GLenum reset_status,
           error::ContextLostReason expected_other_reason) {
    if (context_->WasAllocatedUsingRobustnessExtension()) {
      EXPECT_CALL(*gl_, GetGraphicsResetStatusARB())
          .WillOnce(Return(reset_status));
    }
    // Other contexts in the group should be lost also.
    EXPECT_CALL(*mock_decoder_, MarkContextLost(expected_other_reason))
        .Times(1)
        .RetiresOnSaturation();

    // Trigger OOM with simulate_out_of_memory_on_large_textures
    cmds::TexStorage2D cmd;
    cmd.Init(client_texture_id_, /*levels=*/1, /*width=*/50000,
             /*height=*/50000);
    EXPECT_EQ(error::kLostContext, ExecuteCmd(cmd));
  }
};

// Test that we lose context.
TEST_P(RasterDecoderOOMTest, ContextLostReasonOOM) {
  Init(/*has_robustness=*/false);
  const error::ContextLostReason expected_reason_for_other_contexts =
      error::kOutOfMemory;
  OOM(GL_NO_ERROR, expected_reason_for_other_contexts);
  EXPECT_EQ(GL_OUT_OF_MEMORY, GetGLError());
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_EQ(error::kOutOfMemory, GetContextLostReason());
}

TEST_P(RasterDecoderOOMTest, ContextLostReasonWhenStatusIsNoError) {
  Init(/*has_robustness=*/true);
  // If the reset status is NO_ERROR, we should be signaling kOutOfMemory.
  const error::ContextLostReason expected_reason_for_other_contexts =
      error::kOutOfMemory;
  OOM(GL_NO_ERROR, expected_reason_for_other_contexts);
  EXPECT_EQ(GL_OUT_OF_MEMORY, GetGLError());
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_EQ(error::kOutOfMemory, GetContextLostReason());
}

TEST_P(RasterDecoderOOMTest, ContextLostReasonWhenStatusIsGuilty) {
  Init(/*has_robustness=*/true);
  // If there was a reset, it should override kOutOfMemory.
  const error::ContextLostReason expected_reason_for_other_contexts =
      error::kUnknown;
  OOM(GL_GUILTY_CONTEXT_RESET_ARB, expected_reason_for_other_contexts);
  EXPECT_EQ(GL_OUT_OF_MEMORY, GetGLError());
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_EQ(error::kGuilty, GetContextLostReason());
}

TEST_P(RasterDecoderOOMTest, ContextLostReasonWhenStatusIsUnknown) {
  Init(/*has_robustness=*/true);
  // If there was a reset, it should override kOutOfMemory.
  const error::ContextLostReason expected_reason_for_other_contexts =
      error::kUnknown;
  OOM(GL_UNKNOWN_CONTEXT_RESET_ARB, expected_reason_for_other_contexts);
  EXPECT_EQ(GL_OUT_OF_MEMORY, GetGLError());
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_EQ(error::kUnknown, GetContextLostReason());
}

INSTANTIATE_TEST_CASE_P(Service, RasterDecoderOOMTest, ::testing::Bool());

class RasterDecoderLostContextTest : public RasterDecoderManualInitTest {
 protected:
  void Init(bool has_robustness) {
    InitState init;
    if (has_robustness) {
      init.extensions.push_back("GL_KHR_robustness");
    }
    InitDecoder(init);
  }

  void InitWithVirtualContextsAndRobustness() {
    InitState init;
    init.extensions.push_back("GL_KHR_robustness");
    init.workarounds.use_virtualized_gl_contexts = true;
    InitDecoder(init);
  }

  void DoGetErrorWithContextLost(GLenum reset_status) {
    DCHECK(context_->HasExtension("GL_KHR_robustness"));
    EXPECT_CALL(*gl_, GetError())
        .WillOnce(Return(GL_CONTEXT_LOST_KHR))
        .RetiresOnSaturation();
    EXPECT_CALL(*gl_, GetGraphicsResetStatusARB())
        .WillOnce(Return(reset_status));
    GetError cmd;
    cmd.Init(shared_memory_id_, shared_memory_offset_);
    EXPECT_EQ(error::kLostContext, ExecuteCmd(cmd));
    EXPECT_EQ(static_cast<GLuint>(GL_NO_ERROR), *GetSharedMemoryAs<GLenum*>());
  }

  void ClearCurrentDecoderError() {
    DCHECK(decoder_->WasContextLost());
    EXPECT_CALL(*gl_, GetError())
        .WillOnce(Return(GL_CONTEXT_LOST_KHR))
        .RetiresOnSaturation();
    GetError cmd;
    cmd.Init(shared_memory_id_, shared_memory_offset_);
    EXPECT_EQ(error::kLostContext, ExecuteCmd(cmd));
  }
};

TEST_P(RasterDecoderLostContextTest, LostFromMakeCurrent) {
  Init(/*has_robustness=*/false);
  EXPECT_CALL(*context_, MakeCurrent(surface_.get())).WillOnce(Return(false));
  // Expect the group to be lost.
  EXPECT_CALL(*mock_decoder_, MarkContextLost(error::kUnknown)).Times(1);
  decoder_->MakeCurrent();
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_EQ(error::kMakeCurrentFailed, GetContextLostReason());

  // We didn't process commands, so we need to clear the decoder error,
  // so that we can shut down cleanly.
  ClearCurrentDecoderError();
}

TEST_P(RasterDecoderLostContextTest, LostFromMakeCurrentWithRobustness) {
  Init(/*has_robustness=*/true);  // with robustness
  // If we can't make the context current, we cannot query the robustness
  // extension.
  EXPECT_CALL(*gl_, GetGraphicsResetStatusARB()).Times(0);
  EXPECT_CALL(*context_, MakeCurrent(surface_.get())).WillOnce(Return(false));
  // Expect the group to be lost.
  EXPECT_CALL(*mock_decoder_, MarkContextLost(error::kUnknown)).Times(1);
  decoder_->MakeCurrent();
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_FALSE(decoder_->WasContextLostByRobustnessExtension());
  EXPECT_EQ(error::kMakeCurrentFailed, GetContextLostReason());

  // We didn't process commands, so we need to clear the decoder error,
  // so that we can shut down cleanly.
  ClearCurrentDecoderError();
}

TEST_P(RasterDecoderLostContextTest, TextureDestroyAfterLostFromMakeCurrent) {
  Init(/*has_robustness=*/true);

  // Create the texture.
  DoTexStorage2D(client_texture_id_, /*levels=*/1, /*width=*/10, /*height=*/10);

  // The texture should never be deleted at the GL level.
  EXPECT_CALL(*gl_, DeleteTextures(1, Pointee(kServiceTextureId)))
      .Times(0)
      .RetiresOnSaturation();

  // Force context lost for MakeCurrent().
  EXPECT_CALL(*context_, MakeCurrent(surface_.get())).WillOnce(Return(false));
  // Expect the group to be lost.
  EXPECT_CALL(*mock_decoder_, MarkContextLost(error::kUnknown)).Times(1);

  decoder_->MakeCurrent();
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_EQ(error::kMakeCurrentFailed, GetContextLostReason());
  ClearCurrentDecoderError();
}

TEST_P(RasterDecoderLostContextTest, QueryDestroyAfterLostFromMakeCurrent) {
  Init(/*has_robustness=*/false);

  const GLsync kGlSync = reinterpret_cast<GLsync>(0xdeadbeef);
  GenHelper<GenQueriesEXTImmediate>(kNewClientId);

  BeginQueryEXT begin_cmd;
  begin_cmd.Init(GL_COMMANDS_COMPLETED_CHROMIUM, kNewClientId,
                 shared_memory_id_, kSharedMemoryOffset);
  EXPECT_EQ(error::kNoError, ExecuteCmd(begin_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  QueryManager* query_manager = decoder_->GetQueryManager();
  ASSERT_TRUE(query_manager != nullptr);
  QueryManager::Query* query = query_manager->GetQuery(kNewClientId);
  ASSERT_TRUE(query != nullptr);
  EXPECT_FALSE(query->IsPending());

  EXPECT_CALL(*gl_, Flush()).RetiresOnSaturation();
  EXPECT_CALL(*gl_, FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0))
      .WillOnce(Return(kGlSync))
      .RetiresOnSaturation();
#if DCHECK_IS_ON()
  EXPECT_CALL(*gl_, IsSync(kGlSync))
      .WillOnce(Return(GL_TRUE))
      .RetiresOnSaturation();
#endif

  EndQueryEXT end_cmd;
  end_cmd.Init(GL_COMMANDS_COMPLETED_CHROMIUM, 1);
  EXPECT_EQ(error::kNoError, ExecuteCmd(end_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

#if DCHECK_IS_ON()
  EXPECT_CALL(*gl_, IsSync(kGlSync)).Times(0).RetiresOnSaturation();
#endif
  EXPECT_CALL(*gl_, DeleteSync(kGlSync)).Times(0).RetiresOnSaturation();

  // Force context lost for MakeCurrent().
  EXPECT_CALL(*context_, MakeCurrent(surface_.get())).WillOnce(Return(false));
  // Expect the group to be lost.
  EXPECT_CALL(*mock_decoder_, MarkContextLost(error::kUnknown)).Times(1);

  decoder_->MakeCurrent();
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_EQ(error::kMakeCurrentFailed, GetContextLostReason());
  ClearCurrentDecoderError();
  ResetDecoder();
}

TEST_P(RasterDecoderLostContextTest, LostFromResetAfterMakeCurrent) {
  Init(/*has_robustness=*/true);
  InSequence seq;
  EXPECT_CALL(*context_, MakeCurrent(surface_.get())).WillOnce(Return(true));
  EXPECT_CALL(*gl_, GetGraphicsResetStatusARB())
      .WillOnce(Return(GL_GUILTY_CONTEXT_RESET_KHR));
  // Expect the group to be lost.
  EXPECT_CALL(*mock_decoder_, MarkContextLost(error::kUnknown)).Times(1);
  decoder_->MakeCurrent();
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_TRUE(decoder_->WasContextLostByRobustnessExtension());
  EXPECT_EQ(error::kGuilty, GetContextLostReason());

  // We didn't process commands, so we need to clear the decoder error,
  // so that we can shut down cleanly.
  ClearCurrentDecoderError();
}

TEST_P(RasterDecoderLostContextTest, LoseGuiltyFromGLError) {
  Init(/*has_robustness=*/true);
  // Always expect other contexts to be signaled as 'kUnknown' since we can't
  // query their status without making them current.
  EXPECT_CALL(*mock_decoder_, MarkContextLost(error::kUnknown)).Times(1);
  DoGetErrorWithContextLost(GL_GUILTY_CONTEXT_RESET_KHR);
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_TRUE(decoder_->WasContextLostByRobustnessExtension());
  EXPECT_EQ(error::kGuilty, GetContextLostReason());
}

TEST_P(RasterDecoderLostContextTest, LoseInnocentFromGLError) {
  Init(/*has_robustness=*/true);
  // Always expect other contexts to be signaled as 'kUnknown' since we can't
  // query their status without making them current.
  EXPECT_CALL(*mock_decoder_, MarkContextLost(error::kUnknown)).Times(1);
  DoGetErrorWithContextLost(GL_INNOCENT_CONTEXT_RESET_KHR);
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_TRUE(decoder_->WasContextLostByRobustnessExtension());
  EXPECT_EQ(error::kInnocent, GetContextLostReason());
}

TEST_P(RasterDecoderLostContextTest, LoseVirtualContextWithRobustness) {
  InitWithVirtualContextsAndRobustness();
  EXPECT_CALL(*mock_decoder_, MarkContextLost(error::kUnknown)).Times(1);
  // Signal guilty....
  DoGetErrorWithContextLost(GL_GUILTY_CONTEXT_RESET_KHR);
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_TRUE(decoder_->WasContextLostByRobustnessExtension());
  // ...but make sure we don't pretend, since for virtual contexts we don't
  // know if this was really the guilty client.
  EXPECT_EQ(error::kUnknown, GetContextLostReason());
}

TEST_P(RasterDecoderLostContextTest, LoseGroupFromRobustness) {
  // If one context in a group is lost through robustness,
  // the other ones should also get lost and query the reset status.
  Init(true);
  EXPECT_CALL(*mock_decoder_, MarkContextLost(error::kUnknown)).Times(1);
  // There should be no GL calls, since we might not have a current context.
  EXPECT_CALL(*gl_, GetGraphicsResetStatusARB()).Times(0);
  LoseContexts(error::kUnknown);
  EXPECT_TRUE(decoder_->WasContextLost());
  EXPECT_EQ(error::kUnknown, GetContextLostReason());

  // We didn't process commands, so we need to clear the decoder error,
  // so that we can shut down cleanly.
  ClearCurrentDecoderError();
}

INSTANTIATE_TEST_CASE_P(Service,
                        RasterDecoderLostContextTest,
                        ::testing::Bool());

}  // namespace raster
}  // namespace gpu
