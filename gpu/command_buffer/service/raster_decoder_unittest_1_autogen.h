// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
// gpu/command_buffer/build_raster_cmd_buffer.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

// It is included by raster_cmd_decoder_unittest_1.cc
#ifndef GPU_COMMAND_BUFFER_SERVICE_RASTER_DECODER_UNITTEST_1_AUTOGEN_H_
#define GPU_COMMAND_BUFFER_SERVICE_RASTER_DECODER_UNITTEST_1_AUTOGEN_H_

TEST_P(RasterDecoderTest1, DeleteTexturesImmediateValidArgs) {
  EXPECT_CALL(*gl_, DeleteTextures(1, Pointee(kServiceTextureId))).Times(1);
  cmds::DeleteTexturesImmediate& cmd =
      *GetImmediateAs<cmds::DeleteTexturesImmediate>();
  SpecializedSetup<cmds::DeleteTexturesImmediate, 0>(true);
  cmd.Init(1, &client_texture_id_);
  EXPECT_EQ(error::kNoError,
            ExecuteImmediateCmd(cmd, sizeof(client_texture_id_)));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
  EXPECT_TRUE(GetTexture(client_texture_id_) == NULL);
}

TEST_P(RasterDecoderTest1, DeleteTexturesImmediateInvalidArgs) {
  cmds::DeleteTexturesImmediate& cmd =
      *GetImmediateAs<cmds::DeleteTexturesImmediate>();
  SpecializedSetup<cmds::DeleteTexturesImmediate, 0>(false);
  GLuint temp = kInvalidClientId;
  cmd.Init(1, &temp);
  EXPECT_EQ(error::kNoError, ExecuteImmediateCmd(cmd, sizeof(temp)));
}

TEST_P(RasterDecoderTest1, FinishValidArgs) {
  EXPECT_CALL(*gl_, Finish());
  SpecializedSetup<cmds::Finish, 0>(true);
  cmds::Finish cmd;
  cmd.Init();
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(RasterDecoderTest1, FlushValidArgs) {
  EXPECT_CALL(*gl_, Flush());
  SpecializedSetup<cmds::Flush, 0>(true);
  cmds::Flush cmd;
  cmd.Init();
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(RasterDecoderTest1, GetErrorValidArgs) {
  EXPECT_CALL(*gl_, GetError());
  SpecializedSetup<cmds::GetError, 0>(true);
  cmds::GetError cmd;
  cmd.Init(shared_memory_id_, shared_memory_offset_);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(RasterDecoderTest1, GetErrorInvalidArgsBadSharedMemoryId) {
  EXPECT_CALL(*gl_, GetError()).Times(0);
  SpecializedSetup<cmds::GetError, 0>(false);
  cmds::GetError cmd;
  cmd.Init(kInvalidSharedMemoryId, shared_memory_offset_);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
  cmd.Init(shared_memory_id_, kInvalidSharedMemoryOffset);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
}

TEST_P(RasterDecoderTest1, GetIntegervValidArgs) {
  EXPECT_CALL(*gl_, GetError()).WillRepeatedly(Return(GL_NO_ERROR));
  SpecializedSetup<cmds::GetIntegerv, 0>(true);
  typedef cmds::GetIntegerv::Result Result;
  Result* result = static_cast<Result*>(shared_memory_address_);
  EXPECT_CALL(*gl_, GetIntegerv(GL_ACTIVE_TEXTURE, result->GetData()));
  result->size = 0;
  cmds::GetIntegerv cmd;
  cmd.Init(GL_ACTIVE_TEXTURE, shared_memory_id_, shared_memory_offset_);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(decoder_->GetGLES2Util()->GLGetNumValuesReturned(GL_ACTIVE_TEXTURE),
            result->GetNumResults());
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(RasterDecoderTest1, GetIntegervInvalidArgs0_0) {
  EXPECT_CALL(*gl_, GetIntegerv(_, _)).Times(0);
  SpecializedSetup<cmds::GetIntegerv, 0>(false);
  cmds::GetIntegerv::Result* result =
      static_cast<cmds::GetIntegerv::Result*>(shared_memory_address_);
  result->size = 0;
  cmds::GetIntegerv cmd;
  cmd.Init(GL_FOG_HINT, shared_memory_id_, shared_memory_offset_);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(0u, result->size);
  EXPECT_EQ(GL_INVALID_ENUM, GetGLError());
}

TEST_P(RasterDecoderTest1, GetIntegervInvalidArgs1_0) {
  EXPECT_CALL(*gl_, GetIntegerv(_, _)).Times(0);
  SpecializedSetup<cmds::GetIntegerv, 0>(false);
  cmds::GetIntegerv::Result* result =
      static_cast<cmds::GetIntegerv::Result*>(shared_memory_address_);
  result->size = 0;
  cmds::GetIntegerv cmd;
  cmd.Init(GL_ACTIVE_TEXTURE, kInvalidSharedMemoryId, 0);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
  EXPECT_EQ(0u, result->size);
}

TEST_P(RasterDecoderTest1, GetIntegervInvalidArgs1_1) {
  EXPECT_CALL(*gl_, GetIntegerv(_, _)).Times(0);
  SpecializedSetup<cmds::GetIntegerv, 0>(false);
  cmds::GetIntegerv::Result* result =
      static_cast<cmds::GetIntegerv::Result*>(shared_memory_address_);
  result->size = 0;
  cmds::GetIntegerv cmd;
  cmd.Init(GL_ACTIVE_TEXTURE, shared_memory_id_, kInvalidSharedMemoryOffset);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
  EXPECT_EQ(0u, result->size);
}
#endif  // GPU_COMMAND_BUFFER_SERVICE_RASTER_DECODER_UNITTEST_1_AUTOGEN_H_
