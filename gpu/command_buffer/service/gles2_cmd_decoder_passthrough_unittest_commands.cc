// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder_unittest.h"

namespace gpu {
namespace gles2 {

using namespace cmds;

template <typename T>
class GLES2DecoderPassthroughFixedCommandTest
    : public GLES2DecoderPassthroughTest {};
TYPED_TEST_CASE_P(GLES2DecoderPassthroughFixedCommandTest);

TYPED_TEST_P(GLES2DecoderPassthroughFixedCommandTest, InvalidCommand) {
  TypeParam cmd;
  cmd.SetHeader();
  EXPECT_EQ(error::kUnknownCommand, this->ExecuteCmd(cmd));
}
REGISTER_TYPED_TEST_CASE_P(GLES2DecoderPassthroughFixedCommandTest,
                           InvalidCommand);

template <typename T>
class GLES2DecoderPassthroughImmediateNoArgCommandTest
    : public GLES2DecoderPassthroughTest {};
TYPED_TEST_CASE_P(GLES2DecoderPassthroughImmediateNoArgCommandTest);

TYPED_TEST_P(GLES2DecoderPassthroughImmediateNoArgCommandTest, InvalidCommand) {
  TypeParam& cmd = *(this->template GetImmediateAs<TypeParam>());
  cmd.SetHeader();
  EXPECT_EQ(error::kUnknownCommand, this->ExecuteImmediateCmd(cmd, 64));
}
REGISTER_TYPED_TEST_CASE_P(GLES2DecoderPassthroughImmediateNoArgCommandTest,
                           InvalidCommand);

template <typename T>
class GLES2DecoderPassthroughImmediateSizeArgCommandTest
    : public GLES2DecoderPassthroughTest {};
TYPED_TEST_CASE_P(GLES2DecoderPassthroughImmediateSizeArgCommandTest);

TYPED_TEST_P(GLES2DecoderPassthroughImmediateSizeArgCommandTest,
             InvalidCommand) {
  TypeParam& cmd = *(this->template GetImmediateAs<TypeParam>());
  cmd.SetHeader(0);
  EXPECT_EQ(error::kUnknownCommand, this->ExecuteImmediateCmd(cmd, 0));
}
REGISTER_TYPED_TEST_CASE_P(GLES2DecoderPassthroughImmediateSizeArgCommandTest,
                           InvalidCommand);

using ES3FixedCommandTypes0 =
    ::testing::Types<BindBufferBase,
                     BindBufferRange,
                     BindSampler,
                     BindTransformFeedback,
                     ClearBufferfi,
                     ClientWaitSync,
                     CopyBufferSubData,
                     CompressedTexImage3D,
                     CompressedTexImage3DBucket,
                     CompressedTexSubImage3D,
                     CompressedTexSubImage3DBucket,
                     CopyTexSubImage3D,
                     DeleteSync,
                     FenceSync,
                     FlushMappedBufferRange,
                     FramebufferTextureLayer,
                     GetActiveUniformBlockiv,
                     GetActiveUniformBlockName,
                     GetActiveUniformsiv,
                     GetFragDataLocation,
                     GetBufferParameteri64v,
                     GetInteger64v,
                     GetInteger64i_v,
                     GetIntegeri_v,
                     GetInternalformativ,
                     GetSamplerParameterfv,
                     GetSamplerParameteriv,
                     GetSynciv,
                     GetUniformBlockIndex,
                     GetUniformBlocksCHROMIUM,
                     GetUniformsES3CHROMIUM,
                     GetTransformFeedbackVarying,
                     GetTransformFeedbackVaryingsCHROMIUM,
                     GetUniformuiv,
                     GetUniformIndices,
                     GetVertexAttribIiv,
                     GetVertexAttribIuiv,
                     IsSampler,
                     IsSync,
                     IsTransformFeedback,
                     MapBufferRange,
                     PauseTransformFeedback,
                     ReadBuffer,
                     ResumeTransformFeedback,
                     SamplerParameterf,
                     SamplerParameteri,
                     TexImage3D,
                     TexStorage3D,
                     TexSubImage3D>;

using ES3FixedCommandTypes1 = ::testing::Types<TransformFeedbackVaryingsBucket,
                                               Uniform1ui,
                                               Uniform2ui,
                                               Uniform3ui,
                                               Uniform4ui,
                                               UniformBlockBinding,
                                               UnmapBuffer,
                                               VertexAttribI4i,
                                               VertexAttribI4ui,
                                               VertexAttribIPointer,
                                               WaitSync,
                                               BeginTransformFeedback,
                                               EndTransformFeedback>;

using ES3ImmediateNoArgCommandTypes0 =
    ::testing::Types<ClearBufferivImmediate,
                     ClearBufferuivImmediate,
                     ClearBufferfvImmediate,
                     SamplerParameterfvImmediate,
                     SamplerParameterfvImmediate,
                     VertexAttribI4ivImmediate,
                     VertexAttribI4uivImmediate>;

using ES3ImmediateSizeArgCommandTypes0 =
    ::testing::Types<DeleteSamplersImmediate,
                     DeleteTransformFeedbacksImmediate,
                     GenTransformFeedbacksImmediate,
                     InvalidateFramebufferImmediate,
                     InvalidateSubFramebufferImmediate,
                     Uniform1uivImmediate,
                     Uniform2uivImmediate,
                     Uniform3uivImmediate,
                     Uniform4uivImmediate,
                     UniformMatrix2x3fvImmediate,
                     UniformMatrix2x4fvImmediate,
                     UniformMatrix3x2fvImmediate,
                     UniformMatrix3x4fvImmediate,
                     UniformMatrix4x2fvImmediate,
                     UniformMatrix4x3fvImmediate>;

INSTANTIATE_TYPED_TEST_CASE_P(0,
                              GLES2DecoderPassthroughFixedCommandTest,
                              ES3FixedCommandTypes0);
INSTANTIATE_TYPED_TEST_CASE_P(1,
                              GLES2DecoderPassthroughFixedCommandTest,
                              ES3FixedCommandTypes1);
INSTANTIATE_TYPED_TEST_CASE_P(0,
                              GLES2DecoderPassthroughImmediateNoArgCommandTest,
                              ES3ImmediateNoArgCommandTypes0);
INSTANTIATE_TYPED_TEST_CASE_P(
    0,
    GLES2DecoderPassthroughImmediateSizeArgCommandTest,
    ES3ImmediateSizeArgCommandTypes0);

}  // namespace gles2
}  // namespace gpu
