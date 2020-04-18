// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gles2_cmd_decoder.h"

#include <stddef.h>
#include <stdint.h>

#include "base/command_line.h"
#include "gpu/command_buffer/common/gles2_cmd_format.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder_unittest.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_mock.h"

using ::gl::MockGLInterface;
using ::testing::_;
using ::testing::Return;

namespace gpu {
namespace gles2 {

// Class to use to test that functions which need feature flags or
// extensions always return INVALID_OPERATION if the feature flags is not
// enabled or extension is not present.
class GLES2DecoderTestDisabledExtensions : public GLES2DecoderTest {
 public:
  GLES2DecoderTestDisabledExtensions() = default;
};
INSTANTIATE_TEST_CASE_P(Service,
                        GLES2DecoderTestDisabledExtensions,
                        ::testing::Bool());

TEST_P(GLES2DecoderTestDisabledExtensions, CHROMIUMPathRenderingDisabled) {
  const GLuint kClientPathId = 0;
  {
    cmds::MatrixLoadfCHROMIUMImmediate& cmd =
        *GetImmediateAs<cmds::MatrixLoadfCHROMIUMImmediate>();
    GLfloat temp[16] = {
        0,
    };
    cmd.Init(GL_PATH_MODELVIEW_CHROMIUM, temp);
    EXPECT_EQ(error::kUnknownCommand, ExecuteImmediateCmd(cmd, sizeof(temp)));
  }
  {
    cmds::MatrixLoadIdentityCHROMIUM cmd;
    cmd.Init(GL_PATH_PROJECTION_CHROMIUM);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::GenPathsCHROMIUM cmd;
    cmd.Init(0, 0);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::DeletePathsCHROMIUM cmd;
    cmd.Init(0, 0);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::IsPathCHROMIUM cmd;
    cmd.Init(kClientPathId, shared_memory_id_, shared_memory_offset_);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::PathCommandsCHROMIUM cmd;
    cmd.Init(kClientPathId, 0, 0, 0, 0, GL_FLOAT, 0, 0);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::PathParameterfCHROMIUM cmd;
    cmd.Init(kClientPathId, GL_PATH_STROKE_WIDTH_CHROMIUM, 1.0f);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::PathParameteriCHROMIUM cmd;
    cmd.Init(kClientPathId, GL_PATH_STROKE_WIDTH_CHROMIUM, 1);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::PathStencilFuncCHROMIUM cmd;
    cmd.Init(GL_NEVER, 2, 3);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::StencilFillPathCHROMIUM cmd;
    cmd.Init(kClientPathId, GL_COUNT_UP_CHROMIUM, 1);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::StencilStrokePathCHROMIUM cmd;
    cmd.Init(kClientPathId, 1, 2);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::CoverFillPathCHROMIUM cmd;
    cmd.Init(kClientPathId, GL_BOUNDING_BOX_CHROMIUM);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::CoverStrokePathCHROMIUM cmd;
    cmd.Init(kClientPathId, GL_BOUNDING_BOX_CHROMIUM);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::StencilThenCoverFillPathCHROMIUM cmd;
    cmd.Init(kClientPathId, GL_COUNT_UP_CHROMIUM, 1, GL_BOUNDING_BOX_CHROMIUM);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::StencilThenCoverStrokePathCHROMIUM cmd;
    cmd.Init(kClientPathId, 1, 2, GL_BOUNDING_BOX_CHROMIUM);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::StencilFillPathInstancedCHROMIUM cmd;
    GLuint* paths = GetSharedMemoryAs<GLuint*>();
    paths[0] = kClientPathId;
    cmd.Init(1, GL_UNSIGNED_INT, shared_memory_id_, shared_memory_offset_, 0,
             GL_COUNT_UP_CHROMIUM, 1, GL_NONE, 0, 0);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::StencilStrokePathInstancedCHROMIUM cmd;
    GLuint* paths = GetSharedMemoryAs<GLuint*>();
    paths[0] = kClientPathId;
    cmd.Init(1, GL_UNSIGNED_INT, shared_memory_id_, shared_memory_offset_, 0,
             0x80, 0x80, GL_NONE, 0, 0);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::CoverFillPathInstancedCHROMIUM cmd;
    GLuint* paths = GetSharedMemoryAs<GLuint*>();
    paths[0] = kClientPathId;
    cmd.Init(1, GL_UNSIGNED_INT, shared_memory_id_, shared_memory_offset_, 0,
             GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, GL_NONE, 0, 0);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::CoverStrokePathInstancedCHROMIUM cmd;
    GLuint* paths = GetSharedMemoryAs<GLuint*>();
    paths[0] = kClientPathId;
    cmd.Init(1, GL_UNSIGNED_INT, shared_memory_id_, shared_memory_offset_, 0,
             GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, GL_NONE, 0, 0);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::StencilThenCoverFillPathInstancedCHROMIUM cmd;
    GLuint* paths = GetSharedMemoryAs<GLuint*>();
    paths[0] = kClientPathId;
    cmd.Init(1, GL_UNSIGNED_INT, shared_memory_id_, shared_memory_offset_, 0,
             GL_COUNT_UP_CHROMIUM, 1,
             GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, GL_NONE, 0, 0);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::StencilThenCoverStrokePathInstancedCHROMIUM cmd;
    GLuint* paths = GetSharedMemoryAs<GLuint*>();
    paths[0] = kClientPathId;
    cmd.Init(1, GL_UNSIGNED_INT, shared_memory_id_, shared_memory_offset_, 0,
             0x80, 0x80, GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, GL_NONE, 0,
             0);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::BindFragmentInputLocationCHROMIUMBucket cmd;
    const uint32_t kBucketId = 123;
    const GLint kLocation = 2;
    const char* kName = "testing";
    SetBucketAsCString(kBucketId, kName);
    cmd.Init(client_program_id_, kLocation, kBucketId);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
  {
    cmds::ProgramPathFragmentInputGenCHROMIUM cmd;
    const GLint kLocation = 2;
    cmd.Init(client_program_id_, kLocation, 0, GL_NONE, 0, 0);
    EXPECT_EQ(error::kUnknownCommand, ExecuteCmd(cmd));
  }
}

class GLES2DecoderTestWithCHROMIUMPathRendering : public GLES2DecoderTest {
 public:
  GLES2DecoderTestWithCHROMIUMPathRendering() : client_path_id_(125) {}

  void SetUp() override {
    InitState init;
    init.gl_version = "OpenGL ES 3.1";
    init.has_alpha = true;
    init.has_depth = true;
    init.request_alpha = true;
    init.request_depth = true;
    init.bind_generates_resource = true;
    init.extensions = "GL_NV_path_rendering GL_NV_framebuffer_mixed_samples";
    InitDecoder(init);

    EXPECT_CALL(*gl_, GenPathsNV(1))
        .WillOnce(Return(kServicePathId))
        .RetiresOnSaturation();
    cmds::GenPathsCHROMIUM cmd;
    cmd.Init(client_path_id_, 1);
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));

    // The tests use client_path_id_ to test all sorts of drawing. The NVPR API
    // behaves differently with a path name that is "used" but not which does
    // not "allocate path object state" and a path name that is a name of a real
    // path object. The drawing with former causes GL error while latter works
    // ok, even if there is nothing in the actual path object. To remain
    // compatible with the API, we allocate path object state even when using
    // the mock API.
    EXPECT_CALL(*gl_,
                PathCommandsNV(kServicePathId, 0, NULL, 0, GL_FLOAT, NULL))
        .RetiresOnSaturation();
    cmds::PathCommandsCHROMIUM pcmd;
    pcmd.Init(client_path_id_, 0, 0, 0, 0, GL_FLOAT, 0, 0);
    EXPECT_EQ(error::kNoError, ExecuteCmd(pcmd));
  }

 protected:
  template <typename TypeParam>
  void TestPathCommandsCHROMIUMCoordTypes();

  struct InstancedTestcase {
    GLsizei num_paths;
    GLenum path_name_type;
    const void* paths;
    GLuint path_base;
    GLenum fill_mode;
    GLuint reference;
    GLuint mask;
    GLenum transform_type;
    const GLfloat* transform_values;
    size_t sizeof_paths;             // Used for copying to shm buffer.
    size_t sizeof_transform_values;  // Used for copying to shm buffer.
    error::Error expected_error;
    GLint expected_gl_error;
    bool expect_gl_call;
  };

  void CallAllInstancedCommands(const InstancedTestcase& testcase) {
    // Note: for testcases that expect a call, We do not compare the 'paths'
    // array during EXPECT_CALL due to it being void*. Instead, we rely on the
    // fact that if the path base was not added correctly, the paths wouldn't
    // exists and the call wouldn't come through.

    bool copy_paths = false;  // Paths are copied for each call that has paths,
                              // since the implementation modifies the memory
                              // area.
    void* paths = NULL;
    uint32_t paths_shm_id = 0;
    uint32_t paths_shm_offset = 0;
    GLfloat* transforms = NULL;
    uint32_t transforms_shm_id = 0;
    uint32_t transforms_shm_offset = 0;

    if (testcase.transform_values) {
      transforms = GetSharedMemoryAs<GLfloat*>();
      transforms_shm_id = shared_memory_id_;
      transforms_shm_offset = shared_memory_offset_;
      memcpy(transforms, testcase.transform_values,
             testcase.sizeof_transform_values);
    } else {
      DCHECK(testcase.sizeof_transform_values == 0);
    }
    if (testcase.paths) {
      paths =
          GetSharedMemoryAsWithOffset<void*>(testcase.sizeof_transform_values);
      paths_shm_id = shared_memory_id_;
      paths_shm_offset =
          shared_memory_offset_ + testcase.sizeof_transform_values;
      copy_paths = true;
    } else {
      DCHECK(testcase.sizeof_paths == 0);
    }

    if (testcase.expect_gl_call) {
      EXPECT_CALL(*gl_, StencilFillPathInstancedNV(
                            testcase.num_paths, GL_UNSIGNED_INT, _, 0,
                            testcase.fill_mode, testcase.mask,
                            testcase.transform_type, transforms))
          .RetiresOnSaturation();
    }
    if (copy_paths) {
      memcpy(paths, testcase.paths, testcase.sizeof_paths);
    }
    {
      cmds::StencilFillPathInstancedCHROMIUM sfi_cmd;
      sfi_cmd.Init(testcase.num_paths, testcase.path_name_type, paths_shm_id,
                   paths_shm_offset, testcase.path_base, testcase.fill_mode,
                   testcase.mask, testcase.transform_type, transforms_shm_id,
                   transforms_shm_offset);
      EXPECT_EQ(testcase.expected_error, ExecuteCmd(sfi_cmd));
      EXPECT_EQ(testcase.expected_gl_error, GetGLError());
    }

    if (testcase.expect_gl_call) {
      EXPECT_CALL(*gl_, StencilStrokePathInstancedNV(
                            testcase.num_paths, GL_UNSIGNED_INT, _, 0,
                            testcase.reference, testcase.mask,
                            testcase.transform_type, transforms))
          .RetiresOnSaturation();
    }
    if (copy_paths) {
      memcpy(paths, testcase.paths, testcase.sizeof_paths);
    }
    {
      cmds::StencilStrokePathInstancedCHROMIUM ssi_cmd;
      ssi_cmd.Init(testcase.num_paths, testcase.path_name_type, paths_shm_id,
                   paths_shm_offset, testcase.path_base, testcase.reference,
                   testcase.mask, testcase.transform_type, transforms_shm_id,
                   transforms_shm_offset);
      EXPECT_EQ(testcase.expected_error, ExecuteCmd(ssi_cmd));
      EXPECT_EQ(testcase.expected_gl_error, GetGLError());
    }

    if (testcase.expect_gl_call) {
      EXPECT_CALL(*gl_, CoverFillPathInstancedNV(
                            testcase.num_paths, GL_UNSIGNED_INT, _, 0,
                            GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                            testcase.transform_type, transforms))
          .RetiresOnSaturation();
    }
    if (copy_paths) {
      memcpy(paths, testcase.paths, testcase.sizeof_paths);
    }
    {
      cmds::CoverFillPathInstancedCHROMIUM cfi_cmd;
      cfi_cmd.Init(testcase.num_paths, testcase.path_name_type, paths_shm_id,
                   paths_shm_offset, testcase.path_base,
                   GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM,
                   testcase.transform_type, transforms_shm_id,
                   transforms_shm_offset);
      EXPECT_EQ(testcase.expected_error, ExecuteCmd(cfi_cmd));
      EXPECT_EQ(testcase.expected_gl_error, GetGLError());
    }
    if (testcase.expect_gl_call) {
      EXPECT_CALL(*gl_, CoverStrokePathInstancedNV(
                            testcase.num_paths, GL_UNSIGNED_INT, _, 0,
                            GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                            testcase.transform_type, transforms))
          .RetiresOnSaturation();
    }
    if (copy_paths) {
      memcpy(paths, testcase.paths, testcase.sizeof_paths);
    }

    {
      cmds::CoverStrokePathInstancedCHROMIUM csi_cmd;
      csi_cmd.Init(testcase.num_paths, testcase.path_name_type, paths_shm_id,
                   paths_shm_offset, testcase.path_base,
                   GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM,
                   testcase.transform_type, transforms_shm_id,
                   transforms_shm_offset);
      EXPECT_EQ(testcase.expected_error, ExecuteCmd(csi_cmd));
      EXPECT_EQ(testcase.expected_gl_error, GetGLError());
    }

    if (testcase.expect_gl_call) {
      EXPECT_CALL(*gl_, StencilThenCoverFillPathInstancedNV(
                            testcase.num_paths, GL_UNSIGNED_INT, _, 0,
                            testcase.fill_mode, testcase.mask,
                            GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                            testcase.transform_type, transforms))
          .RetiresOnSaturation();
    }
    if (copy_paths) {
      memcpy(paths, testcase.paths, testcase.sizeof_paths);
    }
    {
      cmds::StencilThenCoverFillPathInstancedCHROMIUM stcfi_cmd;
      stcfi_cmd.Init(testcase.num_paths, testcase.path_name_type, paths_shm_id,
                     paths_shm_offset, testcase.path_base, testcase.fill_mode,
                     testcase.mask, GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM,
                     testcase.transform_type, transforms_shm_id,
                     transforms_shm_offset);
      EXPECT_EQ(testcase.expected_error, ExecuteCmd(stcfi_cmd));
      EXPECT_EQ(testcase.expected_gl_error, GetGLError());
    }

    if (testcase.expect_gl_call) {
      EXPECT_CALL(*gl_, StencilThenCoverStrokePathInstancedNV(
                            testcase.num_paths, GL_UNSIGNED_INT, _, 0,
                            testcase.reference, testcase.mask,
                            GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
                            testcase.transform_type, transforms))
          .RetiresOnSaturation();
    }
    if (copy_paths) {
      memcpy(paths, testcase.paths, testcase.sizeof_paths);
    }
    {
      cmds::StencilThenCoverStrokePathInstancedCHROMIUM stcsi_cmd;
      stcsi_cmd.Init(testcase.num_paths, testcase.path_name_type, paths_shm_id,
                     paths_shm_offset, testcase.path_base, testcase.reference,
                     testcase.mask, GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM,
                     testcase.transform_type, transforms_shm_id,
                     transforms_shm_offset);
      EXPECT_EQ(testcase.expected_error, ExecuteCmd(stcsi_cmd));
      EXPECT_EQ(testcase.expected_gl_error, GetGLError());
    }
  }

  void CallAllInstancedCommandsWithInvalidSHM(GLsizei num_paths,
                                              const GLuint* paths,
                                              GLuint* paths_shm,
                                              uint32_t paths_shm_id,
                                              uint32_t paths_shm_offset,
                                              uint32_t transforms_shm_id,
                                              uint32_t transforms_shm_offset) {
    const GLuint kPathBase = 0;
    const GLenum kFillMode = GL_INVERT;
    const GLuint kMask = 0x80;
    const GLuint kReference = 0xFF;
    const GLuint kTransformType = GL_AFFINE_3D_CHROMIUM;
    memcpy(paths_shm, paths, sizeof(GLuint) * num_paths);
    {
      cmds::StencilFillPathInstancedCHROMIUM sfi_cmd;
      sfi_cmd.Init(num_paths, GL_UNSIGNED_INT, paths_shm_id, paths_shm_offset,
                   kPathBase, kFillMode, kMask, kTransformType,
                   transforms_shm_id, transforms_shm_offset);
      EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(sfi_cmd));
      EXPECT_EQ(GL_NO_ERROR, GetGLError());
    }
    memcpy(paths_shm, paths, sizeof(GLuint) * num_paths);
    {
      cmds::StencilStrokePathInstancedCHROMIUM ssi_cmd;
      ssi_cmd.Init(num_paths, GL_UNSIGNED_INT, paths_shm_id, paths_shm_offset,
                   kPathBase, kReference, kMask, kTransformType,
                   transforms_shm_id, transforms_shm_offset);
      EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(ssi_cmd));
      EXPECT_EQ(GL_NO_ERROR, GetGLError());
    }
    memcpy(paths_shm, paths, sizeof(GLuint) * num_paths);
    {
      cmds::CoverFillPathInstancedCHROMIUM cfi_cmd;
      cfi_cmd.Init(num_paths, GL_UNSIGNED_INT, paths_shm_id, paths_shm_offset,
                   kPathBase, GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM,
                   kTransformType, transforms_shm_id, transforms_shm_offset);
      EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cfi_cmd));
      EXPECT_EQ(GL_NO_ERROR, GetGLError());
    }
    memcpy(paths_shm, paths, sizeof(GLuint) * num_paths);
    {
      cmds::CoverStrokePathInstancedCHROMIUM csi_cmd;
      csi_cmd.Init(num_paths, GL_UNSIGNED_INT, paths_shm_id, paths_shm_offset,
                   kPathBase, GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM,
                   kTransformType, transforms_shm_id, transforms_shm_offset);
      EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(csi_cmd));
      EXPECT_EQ(GL_NO_ERROR, GetGLError());
    }
    memcpy(paths_shm, paths, sizeof(GLuint) * num_paths);
    {
      cmds::StencilThenCoverFillPathInstancedCHROMIUM stcfi_cmd;
      stcfi_cmd.Init(num_paths, GL_UNSIGNED_INT, paths_shm_id, paths_shm_offset,
                     kPathBase, kFillMode, kMask,
                     GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, kTransformType,
                     transforms_shm_id, transforms_shm_offset);
      EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(stcfi_cmd));
      EXPECT_EQ(GL_NO_ERROR, GetGLError());
    }
    memcpy(paths_shm, paths, sizeof(GLuint) * num_paths);
    {
      cmds::StencilThenCoverStrokePathInstancedCHROMIUM stcsi_cmd;
      stcsi_cmd.Init(num_paths, GL_UNSIGNED_INT, paths_shm_id, paths_shm_offset,
                     kPathBase, kReference, kMask,
                     GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, kTransformType,
                     transforms_shm_id, transforms_shm_offset);
      EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(stcsi_cmd));
      EXPECT_EQ(GL_NO_ERROR, GetGLError());
    }
  }

  GLuint client_path_id_;
  static const GLuint kServicePathId = 311;
};

INSTANTIATE_TEST_CASE_P(Service,
                        GLES2DecoderTestWithCHROMIUMPathRendering,
                        ::testing::Bool());

class GLES2DecoderTestWithBlendEquationAdvanced : public GLES2DecoderTest {
 public:
  GLES2DecoderTestWithBlendEquationAdvanced() = default;
  void SetUp() override {
    InitState init;
    init.gl_version = "OpenGL ES 2.0";
    init.has_alpha = true;
    init.has_depth = true;
    init.request_alpha = true;
    init.request_depth = true;
    init.bind_generates_resource = true;
    init.extensions = "GL_KHR_blend_equation_advanced";
    InitDecoder(init);
  }
};

INSTANTIATE_TEST_CASE_P(Service,
                        GLES2DecoderTestWithBlendEquationAdvanced,
                        ::testing::Bool());

class GLES2DecoderTestWithEXTMultisampleCompatibility
    : public GLES2DecoderTest {
 public:
  GLES2DecoderTestWithEXTMultisampleCompatibility() = default;

  void SetUp() override {
    InitState init;
    init.gl_version = "OpenGL ES 3.1";
    init.has_alpha = true;
    init.has_depth = true;
    init.request_alpha = true;
    init.request_depth = true;
    init.bind_generates_resource = true;
    init.extensions = "GL_EXT_multisample_compatibility ";
    InitDecoder(init);
  }
};
INSTANTIATE_TEST_CASE_P(Service,
                        GLES2DecoderTestWithEXTMultisampleCompatibility,
                        ::testing::Bool());

class GLES2DecoderTestWithBlendFuncExtended : public GLES2DecoderTest {
 public:
  GLES2DecoderTestWithBlendFuncExtended() = default;
  void SetUp() override {
    InitState init;
    init.gl_version = "OpenGL ES 3.0";
    init.has_alpha = true;
    init.has_depth = true;
    init.request_alpha = true;
    init.request_depth = true;
    init.bind_generates_resource = true;
    init.extensions = "GL_EXT_blend_func_extended";
    InitDecoder(init);
  }
};
INSTANTIATE_TEST_CASE_P(Service,
                        GLES2DecoderTestWithBlendFuncExtended,
                        ::testing::Bool());

class GLES2DecoderTestWithCHROMIUMFramebufferMixedSamples
    : public GLES2DecoderTest {
 public:
  GLES2DecoderTestWithCHROMIUMFramebufferMixedSamples() = default;
  void SetUp() override {
    InitState init;
    init.gl_version = "OpenGL ES 3.1";
    init.has_alpha = true;
    init.has_depth = true;
    init.request_alpha = true;
    init.request_depth = true;
    init.bind_generates_resource = true;
    init.extensions = "GL_NV_path_rendering GL_NV_framebuffer_mixed_samples ";
    InitDecoder(init);
  }
};

INSTANTIATE_TEST_CASE_P(Service,
                        GLES2DecoderTestWithCHROMIUMFramebufferMixedSamples,
                        ::testing::Bool());

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, GenDeletePaths) {
  static GLuint kFirstClientID = client_path_id_ + 88;
  static GLsizei kPathCount = 58;
  static GLuint kFirstCreatedServiceID = 8000;

  // GenPaths range 0 causes no calls.
  cmds::GenPathsCHROMIUM gen_cmd;
  gen_cmd.Init(kFirstClientID, 0);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // DeletePaths range 0 causes no calls.
  cmds::DeletePathsCHROMIUM delete_cmd;
  delete_cmd.Init(kFirstClientID, 0);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // DeletePaths client id 0 causes no calls and no errors.
  delete_cmd.Init(0, 1);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // DeletePaths with a big range should not cause any deletes.
  delete_cmd.Init(client_path_id_ + 1,
                  std::numeric_limits<GLsizei>::max() - client_path_id_ - 1);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  delete_cmd.Init(static_cast<GLuint>(std::numeric_limits<GLsizei>::max()) + 1,
                  std::numeric_limits<GLsizei>::max());
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Normal Gen and Delete should cause the normal calls.
  EXPECT_CALL(*gl_, GenPathsNV(kPathCount))
      .WillOnce(Return(kFirstCreatedServiceID))
      .RetiresOnSaturation();

  gen_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, DeletePathsNV(kFirstCreatedServiceID, kPathCount))
      .RetiresOnSaturation();

  delete_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, GenDeleteRanges) {
  static GLuint kFirstClientID = client_path_id_ + 77;
  static GLsizei kPathCount = 5800;
  static GLuint kFirstCreatedServiceID = 8000;

  // Create a range of path names, delete one in middle and then
  // the rest. Expect 3 DeletePath calls.
  EXPECT_CALL(*gl_, GenPathsNV(kPathCount))
      .WillOnce(Return(kFirstCreatedServiceID))
      .RetiresOnSaturation();
  cmds::GenPathsCHROMIUM gen_cmd;
  gen_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, DeletePathsNV(kFirstCreatedServiceID + (kPathCount / 2), 1))
      .RetiresOnSaturation();

  cmds::DeletePathsCHROMIUM delete_cmd;
  delete_cmd.Init(kFirstClientID + (kPathCount / 2), 1);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, DeletePathsNV(kFirstCreatedServiceID, (kPathCount / 2)))
      .RetiresOnSaturation();
  EXPECT_CALL(*gl_, DeletePathsNV(kFirstCreatedServiceID + (kPathCount / 2) + 1,
                                  (kPathCount / 2) - 1)).RetiresOnSaturation();

  delete_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, GenDeleteManyPaths) {
  static GLuint kFirstClientID = client_path_id_ + 1;
  static GLsizei kPathCount = std::numeric_limits<GLsizei>::max();
  static GLuint kFirstCreatedServiceID = 8000;

  EXPECT_CALL(*gl_, GenPathsNV(kPathCount))
      .WillOnce(Return(kFirstCreatedServiceID))
      .RetiresOnSaturation();

  // GenPaths with big range.
  cmds::GenPathsCHROMIUM gen_cmd;
  gen_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Path range wraps, so we get connection error.
  gen_cmd.Init(kFirstClientID + kPathCount, kPathCount);
  EXPECT_EQ(error::kInvalidArguments, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, DeletePathsNV(kFirstCreatedServiceID, kPathCount))
      .RetiresOnSaturation();

  cmds::DeletePathsCHROMIUM delete_cmd;
  delete_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Delete every possible path.
  // We run into the one created for client_path_id_.
  EXPECT_CALL(*gl_, DeletePathsNV(kServicePathId, 1)).RetiresOnSaturation();

  delete_cmd.Init(1u, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  delete_cmd.Init(static_cast<GLuint>(kPathCount) + 1u, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Allocate every possible path, delete few, allocate them back and
  // expect minimum amount of calls.
  EXPECT_CALL(*gl_, GenPathsNV(kPathCount))
      .WillOnce(Return(static_cast<GLuint>(1u)))
      .WillOnce(Return(static_cast<GLuint>(kPathCount) + 1u))
      .RetiresOnSaturation();

  gen_cmd.Init(1u, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  gen_cmd.Init(static_cast<GLuint>(kPathCount) + 1u, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  gen_cmd.Init(static_cast<GLuint>(kPathCount) * 2u + 2u, kPathCount);
  EXPECT_EQ(error::kInvalidArguments, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, DeletePathsNV(kFirstClientID, 4)).RetiresOnSaturation();

  delete_cmd.Init(kFirstClientID, 4);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, DeletePathsNV(kFirstClientID * 3, 1)).RetiresOnSaturation();

  delete_cmd.Init(kFirstClientID * 3, 1);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, GenPathsNV(1))
      .WillOnce(Return(kFirstClientID))
      .WillOnce(Return(kFirstClientID + 1))
      .WillOnce(Return(kFirstClientID + 2))
      .WillOnce(Return(kFirstClientID + 3))
      .RetiresOnSaturation();

  for (int i = 0; i < 4; ++i) {
    gen_cmd.Init(kFirstClientID + i, 1);
    EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
    EXPECT_EQ(GL_NO_ERROR, GetGLError());
  }

  EXPECT_CALL(*gl_, GenPathsNV(1))
      .WillOnce(Return(kFirstClientID * 3))
      .RetiresOnSaturation();
  gen_cmd.Init(kFirstClientID * 3, 1);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, DeletePathsNV(1u, kPathCount)).RetiresOnSaturation();
  EXPECT_CALL(*gl_, DeletePathsNV(static_cast<GLuint>(kPathCount) + 1u,
                                  kPathCount)).RetiresOnSaturation();

  delete_cmd.Init(1u, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  delete_cmd.Init(static_cast<GLuint>(kPathCount) + 1u, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Cleanup: return the client_path_id_ as a path.
  EXPECT_CALL(*gl_, GenPathsNV(1))
      .WillOnce(Return(static_cast<GLuint>(kServicePathId)))
      .RetiresOnSaturation();

  gen_cmd.Init(client_path_id_, 1);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       GenPathsCHROMIUMInvalidCalls) {
  static GLuint kFirstClientID = client_path_id_ + 88;
  static GLsizei kPathCount = 5800;
  static GLuint kFirstCreatedServiceID = 8000;

  // Range < 0 is causes gl error.
  cmds::GenPathsCHROMIUM gen_cmd;
  gen_cmd.Init(kFirstClientID, -1);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());

  // Path 0 is invalid client id, connection error.
  gen_cmd.Init(0, kPathCount);
  EXPECT_EQ(error::kInvalidArguments, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Too big range causes client id to wrap, connection error.
  gen_cmd.Init(static_cast<GLuint>(std::numeric_limits<GLsizei>::max()) + 3,
               std::numeric_limits<GLsizei>::max());
  EXPECT_EQ(error::kInvalidArguments, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Creating duplicate client_ids cause connection error.
  EXPECT_CALL(*gl_, GenPathsNV(kPathCount))
      .WillOnce(Return(kFirstCreatedServiceID))
      .RetiresOnSaturation();

  gen_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Create duplicate by executing the same cmd.
  EXPECT_EQ(error::kInvalidArguments, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Create duplicate by creating a range that contains
  // an already existing client path id.
  gen_cmd.Init(kFirstClientID - 1, 2);
  EXPECT_EQ(error::kInvalidArguments, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Cleanup.
  EXPECT_CALL(*gl_, DeletePathsNV(kFirstCreatedServiceID, kPathCount))
      .RetiresOnSaturation();
  cmds::DeletePathsCHROMIUM delete_cmd;
  delete_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       DeletePathsCHROMIUMInvalidCalls) {
  static GLuint kFirstClientID = client_path_id_ + 88;

  // Range < 0 is causes gl error.
  cmds::DeletePathsCHROMIUM delete_cmd;
  delete_cmd.Init(kFirstClientID, -1);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());

  // Too big range causes client id to wrap, connection error.
  delete_cmd.Init(static_cast<GLuint>(std::numeric_limits<GLsizei>::max()) + 3,
                  std::numeric_limits<GLsizei>::max());
  EXPECT_EQ(error::kInvalidArguments, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       PathCommandsCHROMIUMInvalidCalls) {
  static const GLsizei kCorrectCoordCount = 19;
  static const GLsizei kCorrectCommandCount = 6;
  static const GLenum kInvalidCoordType = GL_NONE;

  GLfloat* coords = GetSharedMemoryAs<GLfloat*>();
  unsigned commands_offset = sizeof(GLfloat) * kCorrectCoordCount;
  GLubyte* commands = GetSharedMemoryAsWithOffset<GLubyte*>(commands_offset);
  for (int i = 0; i < kCorrectCoordCount; ++i) {
    coords[i] = 5.0f * i;
  }
  commands[0] = GL_MOVE_TO_CHROMIUM;
  commands[1] = GL_CLOSE_PATH_CHROMIUM;
  commands[2] = GL_LINE_TO_CHROMIUM;
  commands[3] = GL_QUADRATIC_CURVE_TO_CHROMIUM;
  commands[4] = GL_CUBIC_CURVE_TO_CHROMIUM;
  commands[5] = GL_CONIC_CURVE_TO_CHROMIUM;

  EXPECT_CALL(*gl_, PathCommandsNV(kServicePathId, kCorrectCommandCount, _,
                                   kCorrectCoordCount, GL_FLOAT, coords))
      .RetiresOnSaturation();

  cmds::PathCommandsCHROMIUM cmd;

  // Reference call -- this succeeds.
  cmd.Init(client_path_id_, kCorrectCommandCount, shared_memory_id_,
           shared_memory_offset_ + commands_offset, kCorrectCoordCount,
           GL_FLOAT, shared_memory_id_, shared_memory_offset_);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, PathCommandsNV(_, _, _, _, _, _)).Times(0);

  // Invalid client id fails.
  cmd.Init(client_path_id_ - 1, kCorrectCommandCount, shared_memory_id_,
           shared_memory_offset_, kCorrectCoordCount, GL_FLOAT,
           shared_memory_id_, shared_memory_offset_);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());

  // The numCommands < 0.
  cmd.Init(client_path_id_, -1, shared_memory_id_, shared_memory_offset_,
           kCorrectCoordCount, GL_FLOAT, shared_memory_id_,
           shared_memory_offset_);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());

  // The numCoords < 0.
  cmd.Init(client_path_id_, kCorrectCommandCount, shared_memory_id_,
           shared_memory_offset_, -1, GL_FLOAT, shared_memory_id_,
           shared_memory_offset_);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());

  // Invalid coordType fails.
  cmd.Init(client_path_id_, kCorrectCommandCount, shared_memory_id_,
           shared_memory_offset_, kCorrectCoordCount, kInvalidCoordType,
           shared_memory_id_, shared_memory_offset_);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_ENUM, GetGLError());

  // Big command counts.
  cmd.Init(client_path_id_, std::numeric_limits<GLsizei>::max(),
           shared_memory_id_, shared_memory_offset_ + commands_offset,
           kCorrectCoordCount, GL_FLOAT, shared_memory_id_,
           shared_memory_offset_);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Invalid SHM cases.
  cmd.Init(client_path_id_, kCorrectCommandCount, kInvalidSharedMemoryId,
           shared_memory_offset_ + commands_offset, kCorrectCoordCount,
           GL_FLOAT, shared_memory_id_, shared_memory_offset_);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  cmd.Init(client_path_id_, kCorrectCommandCount, shared_memory_id_,
           kInvalidSharedMemoryOffset, kCorrectCoordCount, GL_FLOAT,
           shared_memory_id_, shared_memory_offset_);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  cmd.Init(client_path_id_, kCorrectCommandCount, shared_memory_id_,
           shared_memory_offset_ + commands_offset, kCorrectCoordCount,
           GL_FLOAT, kInvalidSharedMemoryId, shared_memory_offset_);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  cmd.Init(client_path_id_, kCorrectCommandCount, shared_memory_id_,
           shared_memory_offset_ + commands_offset, kCorrectCoordCount,
           GL_FLOAT, shared_memory_id_, kInvalidSharedMemoryOffset);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // NULL shm command id with non-zero command count.
  cmd.Init(client_path_id_, kCorrectCommandCount, 0, 0, kCorrectCoordCount,
           GL_FLOAT, shared_memory_id_, shared_memory_offset_);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // NULL shm coord id with non-zero coord count.
  cmd.Init(client_path_id_, kCorrectCommandCount, shared_memory_id_,
           shared_memory_offset_ + commands_offset, kCorrectCoordCount,
           GL_FLOAT, 0, 0);
  EXPECT_EQ(error::kOutOfBounds, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // The coordCount not matching what is in commands.
  // Expects kCorrectCoordCount+2 coords.
  commands[1] = GL_MOVE_TO_CHROMIUM;
  cmd.Init(client_path_id_, kCorrectCommandCount, shared_memory_id_,
           shared_memory_offset_ + commands_offset, kCorrectCoordCount,
           GL_FLOAT, shared_memory_id_, shared_memory_offset_);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());

  // The coordCount not matching what is in commands.
  // Expects kCorrectCoordCount-2 coords.
  commands[0] = GL_CLOSE_PATH_CHROMIUM;
  commands[1] = GL_CLOSE_PATH_CHROMIUM;
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());

  // NULL shm coord ids. Currently causes gl error, though client should not let
  // this through.
  cmd.Init(client_path_id_, kCorrectCommandCount, shared_memory_id_,
           shared_memory_offset_ + commands_offset, kCorrectCoordCount,
           GL_FLOAT, 0, 0);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       PathCommandsCHROMIUMEmptyCommands) {
  EXPECT_CALL(*gl_, PathCommandsNV(kServicePathId, 0, NULL, 0, GL_FLOAT, NULL))
      .RetiresOnSaturation();
  cmds::PathCommandsCHROMIUM cmd;
  cmd.Init(client_path_id_, 0, 0, 0, 0, GL_FLOAT, 0, 0);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       PathCommandsCHROMIUMInvalidCommands) {
  EXPECT_CALL(*gl_, PathCommandsNV(_, _, _, _, _, _)).Times(0);

  cmds::PathCommandsCHROMIUM cmd;

  {
    const GLsizei kCoordCount = 2;
    const GLsizei kCommandCount = 2;
    GLfloat* coords = GetSharedMemoryAs<GLfloat*>();
    unsigned commands_offset = sizeof(GLfloat) * kCoordCount;
    GLubyte* commands = GetSharedMemoryAsWithOffset<GLubyte*>(commands_offset);

    coords[0] = 5.0f;
    coords[1] = 5.0f;
    commands[0] = 0x3;  // Token MOVE_TO_RELATIVE in NV_path_rendering.
    commands[1] = GL_CLOSE_PATH_CHROMIUM;

    cmd.Init(client_path_id_ - 1, kCommandCount, shared_memory_id_,
             shared_memory_offset_, kCoordCount, GL_FLOAT, shared_memory_id_,
             shared_memory_offset_);
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
    EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());
  }
  {
    const GLsizei kCoordCount = 8;
    const GLsizei kCommandCount = 4;
    GLfloat* coords = GetSharedMemoryAs<GLfloat*>();
    unsigned commands_offset = sizeof(GLfloat) * kCoordCount;
    GLubyte* commands = GetSharedMemoryAsWithOffset<GLubyte*>(commands_offset);

    for (int i = 0; i < kCoordCount; ++i) {
      coords[i] = 5.0f * i;
    }
    commands[0] = GL_MOVE_TO_CHROMIUM;
    commands[1] = GL_MOVE_TO_CHROMIUM;
    commands[2] = 'M';  // Synonym to MOVE_TO in NV_path_rendering.
    commands[3] = GL_MOVE_TO_CHROMIUM;

    cmd.Init(client_path_id_ - 1, kCommandCount, shared_memory_id_,
             shared_memory_offset_, kCoordCount, GL_FLOAT, shared_memory_id_,
             shared_memory_offset_);
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
    EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());
  }
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, PathParameterXCHROMIUM) {
  static GLuint kFirstClientID = client_path_id_ + 88;
  static GLsizei kPathCount = 2;
  static GLuint kFirstCreatedServiceID = 8000;

  // Create a paths so that we do not modify client_path_id_
  EXPECT_CALL(*gl_, GenPathsNV(kPathCount))
      .WillOnce(Return(kFirstCreatedServiceID))
      .RetiresOnSaturation();
  cmds::GenPathsCHROMIUM gen_cmd;
  gen_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  cmds::PathParameterfCHROMIUM fcmd;
  cmds::PathParameteriCHROMIUM icmd;
  const struct {
    GLenum pname;
    GLfloat value;
    GLfloat expected_value;
  } kTestcases[] = {
      {GL_PATH_STROKE_WIDTH_CHROMIUM, 1.0f, 1.0f},
      {GL_PATH_STROKE_WIDTH_CHROMIUM, 0.0f, 0.0f},
      {GL_PATH_MITER_LIMIT_CHROMIUM, 500.0f, 500.0f},
      {GL_PATH_STROKE_BOUND_CHROMIUM, .80f, .80f},
      {GL_PATH_STROKE_BOUND_CHROMIUM, 1.80f, 1.0f},
      {GL_PATH_STROKE_BOUND_CHROMIUM, -1.0f, 0.0f},
      {GL_PATH_END_CAPS_CHROMIUM, GL_FLAT_CHROMIUM, GL_FLAT_CHROMIUM},
      {GL_PATH_END_CAPS_CHROMIUM, GL_SQUARE_CHROMIUM, GL_SQUARE_CHROMIUM},
      {GL_PATH_JOIN_STYLE_CHROMIUM,
       GL_MITER_REVERT_CHROMIUM,
       GL_MITER_REVERT_CHROMIUM},
  };

  for (auto& testcase : kTestcases) {
    EXPECT_CALL(*gl_, PathParameterfNV(kFirstCreatedServiceID, testcase.pname,
                                       testcase.expected_value))
        .Times(1)
        .RetiresOnSaturation();
    fcmd.Init(kFirstClientID, testcase.pname, testcase.value);
    EXPECT_EQ(error::kNoError, ExecuteCmd(fcmd));
    EXPECT_EQ(GL_NO_ERROR, GetGLError());

    EXPECT_CALL(*gl_,
                PathParameteriNV(kFirstCreatedServiceID + 1, testcase.pname,
                                 static_cast<GLint>(testcase.expected_value)))
        .Times(1)
        .RetiresOnSaturation();
    icmd.Init(kFirstClientID + 1, testcase.pname,
              static_cast<GLint>(testcase.value));
    EXPECT_EQ(error::kNoError, ExecuteCmd(icmd));
    EXPECT_EQ(GL_NO_ERROR, GetGLError());
  }

  // Cleanup.
  EXPECT_CALL(*gl_, DeletePathsNV(kFirstCreatedServiceID, kPathCount))
      .RetiresOnSaturation();

  cmds::DeletePathsCHROMIUM delete_cmd;
  delete_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       PathParameterXCHROMIUMInvalidArgs) {
  static GLuint kFirstClientID = client_path_id_ + 88;
  static GLsizei kPathCount = 2;
  static GLuint kFirstCreatedServiceID = 8000;

  // Create a paths so that we do not modify client_path_id_
  EXPECT_CALL(*gl_, GenPathsNV(kPathCount))
      .WillOnce(Return(kFirstCreatedServiceID))
      .RetiresOnSaturation();
  cmds::GenPathsCHROMIUM gen_cmd;
  gen_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(gen_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  cmds::PathParameterfCHROMIUM fcmd;
  cmds::PathParameteriCHROMIUM icmd;
  const struct {
    GLenum pname;
    GLfloat value;
    bool try_int_version;
    GLint error;
  } kTestcases[] = {
      {GL_PATH_STROKE_WIDTH_CHROMIUM, -1.0f, true, GL_INVALID_VALUE},
      {GL_PATH_MITER_LIMIT_CHROMIUM,
       std::numeric_limits<float>::infinity(),
       false,
       GL_INVALID_VALUE},
      {GL_PATH_MITER_LIMIT_CHROMIUM,
       std::numeric_limits<float>::quiet_NaN(),
       false,
       GL_INVALID_VALUE},
      {GL_PATH_END_CAPS_CHROMIUM, 0x4, true, GL_INVALID_VALUE},
      {GL_PATH_END_CAPS_CHROMIUM,
       GL_MITER_REVERT_CHROMIUM,
       true,
       GL_INVALID_VALUE},
      {GL_PATH_JOIN_STYLE_CHROMIUM, GL_FLAT_CHROMIUM, true, GL_INVALID_VALUE},
      {GL_PATH_MODELVIEW_CHROMIUM, GL_FLAT_CHROMIUM, true, GL_INVALID_ENUM},
  };

  EXPECT_CALL(*gl_, PathParameterfNV(_, _, _)).Times(0);
  EXPECT_CALL(*gl_, PathParameteriNV(_, _, _)).Times(0);

  for (auto& testcase : kTestcases) {
    fcmd.Init(kFirstClientID, testcase.pname, testcase.value);
    EXPECT_EQ(error::kNoError, ExecuteCmd(fcmd));
    EXPECT_EQ(testcase.error, GetGLError());
    if (!testcase.try_int_version)
      continue;

    icmd.Init(kFirstClientID + 1, testcase.pname,
              static_cast<GLint>(testcase.value));
    EXPECT_EQ(error::kNoError, ExecuteCmd(icmd));
    EXPECT_EQ(testcase.error, GetGLError());
  }

  // Cleanup.
  EXPECT_CALL(*gl_, DeletePathsNV(kFirstCreatedServiceID, kPathCount))
      .RetiresOnSaturation();

  cmds::DeletePathsCHROMIUM delete_cmd;
  delete_cmd.Init(kFirstClientID, kPathCount);
  EXPECT_EQ(error::kNoError, ExecuteCmd(delete_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, StencilFillPathCHROMIUM) {
  SetupExpectationsForApplyingDefaultDirtyState();

  cmds::StencilFillPathCHROMIUM cmd;
  cmds::StencilThenCoverFillPathCHROMIUM tcmd;

  static const GLenum kFillModes[] = {
      GL_INVERT, GL_COUNT_UP_CHROMIUM, GL_COUNT_DOWN_CHROMIUM};
  static const GLuint kMask = 0x7F;

  for (auto& fill_mode : kFillModes) {
    EXPECT_CALL(*gl_, StencilFillPathNV(kServicePathId, fill_mode, kMask))
        .RetiresOnSaturation();
    cmd.Init(client_path_id_, fill_mode, kMask);
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
    EXPECT_EQ(GL_NO_ERROR, GetGLError());

    EXPECT_CALL(*gl_, StencilThenCoverFillPathNV(kServicePathId, fill_mode,
                                                 kMask, GL_BOUNDING_BOX_NV))
        .RetiresOnSaturation();
    tcmd.Init(client_path_id_, fill_mode, kMask, GL_BOUNDING_BOX_CHROMIUM);
    EXPECT_EQ(error::kNoError, ExecuteCmd(tcmd));
    EXPECT_EQ(GL_NO_ERROR, GetGLError());
  }

  // Non-existent path: no error, no call.
  cmd.Init(client_path_id_ - 1, GL_INVERT, 0x80);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  tcmd.Init(client_path_id_ - 1, GL_INVERT, 0x80, GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(tcmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       StencilFillPathCHROMIUMInvalidArgs) {
  EXPECT_CALL(*gl_, StencilFillPathNV(_, _, _)).Times(0);
  EXPECT_CALL(*gl_, StencilThenCoverFillPathNV(_, _, _, GL_BOUNDING_BOX_NV))
      .Times(0);

  cmds::StencilFillPathCHROMIUM cmd;
  cmds::StencilThenCoverFillPathCHROMIUM tcmd;

  cmd.Init(client_path_id_, GL_INVERT - 1, 0x80);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_ENUM, GetGLError());

  tcmd.Init(client_path_id_, GL_INVERT - 1, 0x80, GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(tcmd));
  EXPECT_EQ(GL_INVALID_ENUM, GetGLError());

  // The /mask/+1 is not power of two -> invalid value.
  cmd.Init(client_path_id_, GL_COUNT_UP_CHROMIUM, 0x80);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());

  tcmd.Init(client_path_id_, GL_COUNT_UP_CHROMIUM, 0x80,
            GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(tcmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());

  cmd.Init(client_path_id_, GL_COUNT_DOWN_CHROMIUM, 5);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());

  tcmd.Init(client_path_id_, GL_COUNT_DOWN_CHROMIUM, 5,
            GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(tcmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, StencilStrokePathCHROMIUM) {
  SetupExpectationsForApplyingDefaultDirtyState();

  EXPECT_CALL(*gl_, StencilStrokePathNV(kServicePathId, 1, 0x80))
      .RetiresOnSaturation();
  EXPECT_CALL(*gl_, StencilThenCoverStrokePathNV(kServicePathId, 1, 0x80,
                                                 GL_BOUNDING_BOX_NV))
      .RetiresOnSaturation();

  cmds::StencilStrokePathCHROMIUM cmd;
  cmds::StencilThenCoverStrokePathCHROMIUM tcmd;

  cmd.Init(client_path_id_, 1, 0x80);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  tcmd.Init(client_path_id_, 1, 0x80, GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(tcmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, StencilThenCoverStrokePathNV(kServicePathId, 1, 0x80,
                                                 GL_CONVEX_HULL_NV))
      .RetiresOnSaturation();

  tcmd.Init(client_path_id_, 1, 0x80, GL_CONVEX_HULL_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(tcmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Non-existent path: no error, no call.
  cmd.Init(client_path_id_ - 1, 1, 0x80);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  tcmd.Init(client_path_id_ - 1, 1, 0x80, GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(tcmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, CoverFillPathCHROMIUM) {
  SetupExpectationsForApplyingDefaultDirtyState();

  EXPECT_CALL(*gl_, CoverFillPathNV(kServicePathId, GL_BOUNDING_BOX_NV))
      .RetiresOnSaturation();
  cmds::CoverFillPathCHROMIUM cmd;
  cmd.Init(client_path_id_, GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, CoverFillPathNV(kServicePathId, GL_CONVEX_HULL_NV))
      .RetiresOnSaturation();
  cmd.Init(client_path_id_, GL_CONVEX_HULL_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Non-existent path: no error, no call.
  cmd.Init(client_path_id_ - 1, GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, CoverStrokePathCHROMIUM) {
  SetupExpectationsForApplyingDefaultDirtyState();
  EXPECT_CALL(*gl_, CoverStrokePathNV(kServicePathId, GL_BOUNDING_BOX_NV))
      .RetiresOnSaturation();
  cmds::CoverStrokePathCHROMIUM cmd;
  cmd.Init(client_path_id_, GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  EXPECT_CALL(*gl_, CoverStrokePathNV(kServicePathId, GL_CONVEX_HULL_NV))
      .RetiresOnSaturation();
  cmd.Init(client_path_id_, GL_CONVEX_HULL_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Non-existent path: no error, no call.
  cmd.Init(client_path_id_ - 1, GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

namespace {
template <typename T>
struct gl_type_enum {};
template <>
struct gl_type_enum<GLbyte> {
  enum { kGLType = GL_BYTE };
};
template <>
struct gl_type_enum<GLubyte> {
  enum { kGLType = GL_UNSIGNED_BYTE };
};
template <>
struct gl_type_enum<GLshort> {
  enum { kGLType = GL_SHORT };
};
template <>
struct gl_type_enum<GLushort> {
  enum { kGLType = GL_UNSIGNED_SHORT };
};
template <>
struct gl_type_enum<GLfloat> {
  enum { kGLType = GL_FLOAT };
};
}

template <typename TypeParam>
void GLES2DecoderTestWithCHROMIUMPathRendering::
    TestPathCommandsCHROMIUMCoordTypes() {
  static const GLsizei kCorrectCoordCount = 19;
  static const GLsizei kCorrectCommandCount = 6;

  TypeParam* coords = GetSharedMemoryAs<TypeParam*>();
  unsigned commands_offset = sizeof(TypeParam) * kCorrectCoordCount;
  GLubyte* commands = GetSharedMemoryAsWithOffset<GLubyte*>(commands_offset);
  for (int i = 0; i < kCorrectCoordCount; ++i) {
    coords[i] = static_cast<TypeParam>(5 * i);
  }
  commands[0] = GL_MOVE_TO_CHROMIUM;
  commands[1] = GL_CLOSE_PATH_CHROMIUM;
  commands[2] = GL_LINE_TO_CHROMIUM;
  commands[3] = GL_QUADRATIC_CURVE_TO_CHROMIUM;
  commands[4] = GL_CUBIC_CURVE_TO_CHROMIUM;
  commands[5] = GL_CONIC_CURVE_TO_CHROMIUM;

  EXPECT_CALL(*gl_, PathCommandsNV(kServicePathId, kCorrectCommandCount, _,
                                   kCorrectCoordCount,
                                   gl_type_enum<TypeParam>::kGLType, coords))
      .RetiresOnSaturation();

  cmds::PathCommandsCHROMIUM cmd;

  cmd.Init(client_path_id_, kCorrectCommandCount, shared_memory_id_,
           shared_memory_offset_ + commands_offset, kCorrectCoordCount,
           gl_type_enum<TypeParam>::kGLType, shared_memory_id_,
           shared_memory_offset_);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       PathCommandsCHROMIUMCoordTypes) {
  // Not using a typed test case, because the base class is already parametrized
  // test case and uses GetParam.
  TestPathCommandsCHROMIUMCoordTypes<GLbyte>();
  TestPathCommandsCHROMIUMCoordTypes<GLubyte>();
  TestPathCommandsCHROMIUMCoordTypes<GLshort>();
  TestPathCommandsCHROMIUMCoordTypes<GLushort>();
  TestPathCommandsCHROMIUMCoordTypes<GLfloat>();
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       StencilXFillPathInstancedCHROMIUMInvalidArgs) {
  cmds::StencilFillPathInstancedCHROMIUM sfi_cmd;
  cmds::StencilThenCoverFillPathInstancedCHROMIUM stcfi_cmd;

  const GLuint kPaths[] = {client_path_id_, client_path_id_ + 5,
                           client_path_id_, client_path_id_ + 18};
  const GLsizei kPathCount = arraysize(kPaths);

  struct {
    GLenum fill_mode;
    GLuint mask;
    GLint expected_error;
  } testcases[] = {
      // Using invalid fill mode produces invalid enum.
      {GL_COUNT_UP_CHROMIUM - 1, 0x7F, GL_INVALID_ENUM},
      {GL_COUNT_DOWN_CHROMIUM + 1, 0x7F, GL_INVALID_ENUM},
      // Using /mask/+1 which is not power of two produces invalid value.
      {GL_COUNT_UP_CHROMIUM, 0x80, GL_INVALID_VALUE},
      {GL_COUNT_DOWN_CHROMIUM, 4, GL_INVALID_VALUE}};

  GLuint* paths = GetSharedMemoryAs<GLuint*>();

  for (size_t i = 0; i < arraysize(testcases); ++i) {
    memcpy(paths, kPaths, sizeof(kPaths));
    sfi_cmd.Init(kPathCount, GL_UNSIGNED_INT, shared_memory_id_,
                 shared_memory_offset_, 0, testcases[i].fill_mode,
                 testcases[i].mask, GL_NONE, 0, 0);
    EXPECT_EQ(error::kNoError, ExecuteCmd(sfi_cmd));
    EXPECT_EQ(testcases[i].expected_error, GetGLError());

    memcpy(paths, kPaths, sizeof(kPaths));
    stcfi_cmd.Init(kPathCount, GL_UNSIGNED_INT, shared_memory_id_,
                   shared_memory_offset_, 0, testcases[i].fill_mode,
                   testcases[i].mask,
                   GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, GL_NONE, 0, 0);
    EXPECT_EQ(error::kNoError, ExecuteCmd(stcfi_cmd));
    EXPECT_EQ(testcases[i].expected_error, GetGLError());
  }
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       StencilXFillPathInstancedCHROMIUMFillMode) {
  SetupExpectationsForApplyingDefaultDirtyState();

  // Test different fill modes.
  cmds::StencilFillPathInstancedCHROMIUM sfi_cmd;
  cmds::StencilThenCoverFillPathInstancedCHROMIUM stcfi_cmd;

  const GLuint kPaths[] = {client_path_id_, client_path_id_ + 5,
                           client_path_id_, client_path_id_ + 18};
  const GLsizei kPathCount = arraysize(kPaths);

  static const GLenum kFillModes[] = {GL_INVERT, GL_COUNT_UP_CHROMIUM,
                                      GL_COUNT_DOWN_CHROMIUM};
  const GLuint kMask = 0x7F;

  GLuint* paths = GetSharedMemoryAs<GLuint*>();

  for (size_t i = 0; i < arraysize(kFillModes); ++i) {
    memcpy(paths, kPaths, sizeof(kPaths));
    EXPECT_CALL(*gl_,
                StencilFillPathInstancedNV(kPathCount, GL_UNSIGNED_INT, _, 0,
                                           kFillModes[i], kMask, GL_NONE, NULL))
        .RetiresOnSaturation();
    sfi_cmd.Init(kPathCount, GL_UNSIGNED_INT, shared_memory_id_,
                 shared_memory_offset_, 0, kFillModes[i], kMask, GL_NONE, 0, 0);
    EXPECT_EQ(error::kNoError, ExecuteCmd(sfi_cmd));
    EXPECT_EQ(GL_NO_ERROR, GetGLError());

    memcpy(paths, kPaths, sizeof(kPaths));
    EXPECT_CALL(*gl_,
                StencilThenCoverFillPathInstancedNV(
                    kPathCount, GL_UNSIGNED_INT, _, 0, kFillModes[i], kMask,
                    GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV, GL_NONE, NULL))
        .RetiresOnSaturation();
    stcfi_cmd.Init(kPathCount, GL_UNSIGNED_INT, shared_memory_id_,
                   shared_memory_offset_, 0, kFillModes[i], kMask,
                   GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, GL_NONE, 0, 0);
    EXPECT_EQ(error::kNoError, ExecuteCmd(stcfi_cmd));
    EXPECT_EQ(GL_NO_ERROR, GetGLError());
  }
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, InstancedCalls) {
  SetupExpectationsForApplyingDefaultDirtyState();

  const GLuint kPaths[] = {0, client_path_id_, 15, client_path_id_};
  const GLsizei kPathCount = arraysize(kPaths);

  // The path base will be client_path_id_, and so 0 is a
  // valid path.
  const GLuint kPathBase = client_path_id_;
  const GLuint kPathsWithBase[] = {0, 5, 0, 18};

  const GLshort kShortPathBase = client_path_id_ * 2;
  const GLshort kShortPathsWithBase[] = {
      -static_cast<GLshort>(client_path_id_), 5,
      -static_cast<GLshort>(client_path_id_), 18};

  const GLenum kFillMode = GL_INVERT;
  const GLuint kMask = 0x80;
  const GLuint kReference = 0xFF;

  GLfloat transform_values[12 * kPathCount];
  for (GLsizei i = 0; i < kPathCount; ++i) {
    for (int j = 0; j < 12; ++j) {
      transform_values[i * 12 + j] = 0.1f * j + i;
    }
  }

  // Path name overflows to correct path.
  const GLuint kBigPathBase = std::numeric_limits<GLuint>::max();
  const GLuint kPathsWithBigBase[] = {client_path_id_ + 1, 5,
                                      client_path_id_ + 1, 18};

  // Path name underflows. As a technical limitation, we can not get to correct
  // path,
  // so test just tests that there is no GL error.
  const GLuint kNegativePathBase = 1;
  const GLbyte kNegativePathsWithBaseByte[] = {-1, -2, -5, -18};
  const GLint kNegativePathsWithBaseInt[] = {-2, -3, -4, -5};

  InstancedTestcase testcases[] = {
      // Test a normal call.
      {kPathCount, GL_UNSIGNED_INT, kPaths, 0, kFillMode, kReference, kMask,
       GL_NONE, NULL, sizeof(kPaths), 0, error::kNoError, GL_NO_ERROR, true},
      // Test that the path base is applied correctly for each instanced call.
      {kPathCount, GL_UNSIGNED_INT, kPathsWithBase, kPathBase, kFillMode,
       kReference, kMask, GL_NONE, NULL, sizeof(kPaths), 0, error::kNoError,
       GL_NO_ERROR, true},
      {kPathCount, GL_UNSIGNED_INT, kPathsWithBase, kPathBase, kFillMode,
       kReference, kMask,

       // Test all possible transform types. The float array is big enough for
       // all the variants.  The contents of the array in call is not checked,
       // though.
       GL_TRANSLATE_X_CHROMIUM, transform_values, sizeof(kPaths),
       sizeof(transform_values), error::kNoError, GL_NO_ERROR, true},
      {kPathCount, GL_UNSIGNED_INT, kPathsWithBase, kPathBase, kFillMode,
       kReference, kMask, GL_TRANSLATE_Y_CHROMIUM, transform_values,
       sizeof(kPaths), sizeof(transform_values), error::kNoError, GL_NO_ERROR,
       true},
      {kPathCount, GL_UNSIGNED_INT, kPathsWithBase, kPathBase, kFillMode,
       kReference, kMask, GL_TRANSLATE_2D_CHROMIUM, transform_values,
       sizeof(kPaths), sizeof(transform_values), error::kNoError, GL_NO_ERROR,
       true},
      {kPathCount, GL_UNSIGNED_INT, kPathsWithBase, kPathBase, kFillMode,
       kReference, kMask, GL_TRANSLATE_3D_CHROMIUM, transform_values,
       sizeof(kPaths), sizeof(transform_values), error::kNoError, GL_NO_ERROR,
       true},
      {kPathCount, GL_UNSIGNED_INT, kPathsWithBase, kPathBase, kFillMode,
       kReference, kMask, GL_AFFINE_2D_CHROMIUM, transform_values,
       sizeof(kPaths), sizeof(transform_values), error::kNoError, GL_NO_ERROR,
       true},
      {kPathCount, GL_UNSIGNED_INT, kPathsWithBase, kPathBase, kFillMode,
       kReference, kMask, GL_AFFINE_3D_CHROMIUM, transform_values,
       sizeof(kPaths), sizeof(transform_values), error::kNoError, GL_NO_ERROR,
       true},
      {kPathCount, GL_UNSIGNED_INT, kPathsWithBase, kPathBase, kFillMode,
       kReference, kMask, GL_TRANSPOSE_AFFINE_2D_CHROMIUM, transform_values,
       sizeof(kPaths), sizeof(transform_values), error::kNoError, GL_NO_ERROR,
       true},
      {kPathCount, GL_UNSIGNED_INT, kPathsWithBase, kPathBase, kFillMode,
       kReference, kMask, GL_TRANSPOSE_AFFINE_3D_CHROMIUM, transform_values,
       sizeof(kPaths), sizeof(transform_values), error::kNoError, GL_NO_ERROR,
       true},
      {kPathCount, GL_SHORT, kShortPathsWithBase, kShortPathBase, kFillMode,
       kReference, kMask, GL_TRANSPOSE_AFFINE_3D_CHROMIUM, transform_values,
       sizeof(kShortPathsWithBase), sizeof(transform_values), error::kNoError,
       GL_NO_ERROR, true},

      // Test that if using path base causes path id to overflow, we get no
      // error.
      {kPathCount, GL_UNSIGNED_INT, kPathsWithBigBase, kBigPathBase, kFillMode,
       kReference, kMask, GL_TRANSLATE_X_CHROMIUM, transform_values,
       sizeof(kPathsWithBigBase), sizeof(transform_values), error::kNoError,
       GL_NO_ERROR, true},
      // Test that if using path base causes path id to underflow, we get no
      // error.
      {kPathCount, GL_BYTE, kNegativePathsWithBaseByte, kNegativePathBase,
       kFillMode, kReference, kMask, GL_TRANSLATE_X_CHROMIUM, transform_values,
       sizeof(kNegativePathsWithBaseByte), sizeof(transform_values),
       error::kNoError, GL_NO_ERROR, false},
      {kPathCount, GL_INT, kNegativePathsWithBaseInt, kNegativePathBase,
       kFillMode, kReference, kMask, GL_TRANSLATE_X_CHROMIUM, transform_values,
       sizeof(kNegativePathsWithBaseInt), sizeof(transform_values),
       error::kNoError, GL_NO_ERROR, false},

  };

  for (size_t i = 0; i < arraysize(testcases); ++i) {
    SCOPED_TRACE(testing::Message() << "InstancedCalls testcase " << i);
    CallAllInstancedCommands(testcases[i]);
  }
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, InstancedNoCalls) {
  const GLuint kPaths[] = {1, client_path_id_, 5, client_path_id_};
  const GLsizei kPathCount = arraysize(kPaths);

  const GLenum kFillMode = GL_INVERT;
  const GLuint kMask = 0x80;
  const GLuint kReference = 0xFF;
  GLfloat transform_values[12 * kPathCount];
  for (GLsizei i = 0; i < kPathCount; ++i) {
    for (int j = 0; j < 12; ++j) {
      transform_values[i * 12 + j] = 0.1f * j + i;
    }
  }

  // The path base will be client_path_id_, and so 0 is a valid path and others
  // should be invalid.
  const GLuint kInvalidPathBase = client_path_id_;
  const GLuint kInvalidPathsWithBase[] = {1, client_path_id_, 5, 18};

  InstancedTestcase testcases[] = {
      // Zero path count produces no error, no call.
      {0, GL_UNSIGNED_INT, NULL, 0, kFillMode, kReference, kMask, GL_NONE, NULL,
       0, 0, error::kNoError, GL_NO_ERROR, false},

      // Zero path count, even with path data, produces no error, no call.
      {0, GL_UNSIGNED_INT, kPaths, 0, kFillMode, kReference, kMask,
       GL_TRANSLATE_X_CHROMIUM, transform_values, sizeof(kPaths),
       sizeof(transform_values), error::kNoError, GL_NO_ERROR, false},

      // Negative path count produces error.
      {-1, GL_UNSIGNED_INT, kPaths, 0, kFillMode, kReference, kMask,
       GL_TRANSLATE_X_CHROMIUM, transform_values, sizeof(kPaths),
       sizeof(transform_values), error::kNoError, GL_INVALID_VALUE, false},

      // Passing paths count but not having the shm data is a connection error.
      {kPathCount, GL_UNSIGNED_INT, NULL, 0, kFillMode, kReference, kMask,
       GL_TRANSLATE_X_CHROMIUM, transform_values, 0, sizeof(transform_values),
       error::kOutOfBounds, GL_NO_ERROR, false},

      // Huge path count would cause huge transfer buffer, it does not go
      // through.
      {std::numeric_limits<GLsizei>::max() - 3, GL_UNSIGNED_INT, kPaths, 0,
       kFillMode, kReference, kMask, GL_TRANSLATE_X_CHROMIUM, transform_values,
       sizeof(kPaths), sizeof(transform_values), error::kOutOfBounds,
       GL_NO_ERROR, false},

      // Test that the path base is applied correctly for each instanced call.
      // In this case no path is marked as used, and so no GL function should be
      // called and no error should be generated.
      {kPathCount, GL_UNSIGNED_INT, kInvalidPathsWithBase, kInvalidPathBase,
       kFillMode, kReference, kMask, GL_TRANSLATE_X_CHROMIUM, transform_values,
       sizeof(kInvalidPathsWithBase), sizeof(transform_values), error::kNoError,
       GL_NO_ERROR, false},

      // Test that using correct paths but invalid transform type produces
      // invalid enum.
      {kPathCount, GL_UNSIGNED_INT, kPaths, 0, kFillMode, kReference, kMask,
       GL_TRANSLATE_X_CHROMIUM - 1, transform_values, sizeof(kPaths),
       sizeof(transform_values), error::kNoError, GL_INVALID_ENUM, false},

      // Test that if we have transform, not having the shm data is a connection
      // error.
      {kPathCount, GL_UNSIGNED_INT, kPaths, 0, kFillMode, kReference, kMask,
       GL_TRANSLATE_X_CHROMIUM, NULL, sizeof(kPaths), 0, error::kOutOfBounds,
       GL_NO_ERROR, false},

  };
  for (size_t i = 0; i < arraysize(testcases); ++i) {
    SCOPED_TRACE(testing::Message() << "InstancedNoCalls testcase " << i);
    CallAllInstancedCommands(testcases[i]);
  }
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering, InstancedInvalidSHMValues) {
  const GLuint kPaths[] = {1, client_path_id_, 5, client_path_id_};
  const GLsizei kPathCount = arraysize(kPaths);
  GLfloat transform_values[12 * kPathCount];
  for (GLsizei i = 0; i < kPathCount; ++i) {
    for (int j = 0; j < 12; ++j) {
      transform_values[i * 12 + j] = 0.1f * j + i;
    }
  }
  enum {
    kPathsSHMIdInvalid = 1,
    kPathsSHMOffsetInvalid = 1 << 1,
    kTransformsHMIdInvalid = 1 << 2,
    kTransformsHMOffsetInvalid = 1 << 3,
    kFirstTestcase = kPathsSHMIdInvalid,
    kLastTestcase = kTransformsHMOffsetInvalid
  };

  for (int testcase = kFirstTestcase; testcase <= kLastTestcase; ++testcase) {
    GLfloat* transforms = GetSharedMemoryAs<GLfloat*>();
    uint32_t transforms_shm_id = shared_memory_id_;
    uint32_t transforms_shm_offset = shared_memory_offset_;
    memcpy(transforms, transform_values, sizeof(transform_values));

    GLuint* paths =
        GetSharedMemoryAsWithOffset<GLuint*>(sizeof(transform_values));
    uint32_t paths_shm_id = shared_memory_id_;
    uint32_t paths_shm_offset =
        shared_memory_offset_ + sizeof(transform_values);

    if (testcase & kPathsSHMIdInvalid) {
      paths_shm_id = kInvalidSharedMemoryId;
    }
    if (testcase & kPathsSHMOffsetInvalid) {
      paths_shm_offset = kInvalidSharedMemoryOffset;
    }
    if (testcase & kTransformsHMIdInvalid) {
      transforms_shm_id = kInvalidSharedMemoryId;
    }
    if (testcase & kTransformsHMOffsetInvalid) {
      transforms_shm_offset = kInvalidSharedMemoryOffset;
    }
    SCOPED_TRACE(testing::Message() << "InstancedInvalidSHMValues testcase "
                                    << testcase);
    CallAllInstancedCommandsWithInvalidSHM(
        kPathCount, kPaths, paths, paths_shm_id, paths_shm_offset,
        transforms_shm_id, transforms_shm_offset);
  }
}

TEST_P(GLES2DecoderTestWithCHROMIUMPathRendering,
       BindFragmentInputLocationCHROMIUM) {
  const uint32_t kBucketId = 123;
  const GLint kLocation = 2;
  const char* kName = "testing";
  const char* kBadName1 = "gl_testing";

  SetBucketAsCString(kBucketId, kName);
  cmds::BindFragmentInputLocationCHROMIUMBucket cmd;
  cmd.Init(client_program_id_, kLocation, kBucketId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
  // Check negative location.
  SetBucketAsCString(kBucketId, kName);
  cmd.Init(client_program_id_, -1, kBucketId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());
  // Check the highest location.
  SetBucketAsCString(kBucketId, kName);
  const GLint kMaxLocation = kMaxVaryingVectors * 4 - 1;
  cmd.Init(client_program_id_, kMaxLocation, kBucketId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
  // Check too high location.
  SetBucketAsCString(kBucketId, kName);
  cmd.Init(client_program_id_, kMaxLocation + 1, kBucketId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());
  // Check bad name "gl_...".
  SetBucketAsCString(kBucketId, kBadName1);
  cmd.Init(client_program_id_, kLocation, kBucketId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());
}

class GLES2DecoderTestWithCHROMIUMRasterTransport : public GLES2DecoderTest {
 public:
  GLES2DecoderTestWithCHROMIUMRasterTransport() = default;
  void SetUp() override {
    InitState init;
    init.gl_version = "OpenGL ES 2.0";
    init.has_alpha = true;
    init.has_depth = true;
    init.request_alpha = true;
    init.request_depth = true;
    init.bind_generates_resource = true;
    init.extensions = "chromium_raster_transport";
    InitDecoder(init);
  }
};

INSTANTIATE_TEST_CASE_P(Service,
                        GLES2DecoderTestWithCHROMIUMRasterTransport,
                        ::testing::Bool());

class GLES3DecoderTestWithEXTWindowRectangles : public GLES3DecoderTest {
 public:
  GLES3DecoderTestWithEXTWindowRectangles() = default;
  void SetUp() override {
    InitState init;
    init.context_type = CONTEXT_TYPE_OPENGLES3;
    init.gl_version = "OpenGL ES 3.0";
    init.has_alpha = true;
    init.has_depth = true;
    init.request_alpha = true;
    init.request_depth = true;
    init.bind_generates_resource = true;
    init.extensions = "GL_EXT_window_rectangles";
    InitDecoder(init);
  }
};

INSTANTIATE_TEST_CASE_P(Service,
                        GLES3DecoderTestWithEXTWindowRectangles,
                        ::testing::Bool());

TEST_P(GLES3DecoderTestWithEXTWindowRectangles,
       WindowRectanglesEXTImmediateValidArgs) {
  cmds::WindowRectanglesEXTImmediate& cmd =
      *GetImmediateAs<cmds::WindowRectanglesEXTImmediate>();
  SpecializedSetup<cmds::WindowRectanglesEXTImmediate, 0>(true);
  GLint temp[4 * 2] = {};

  // The backbuffer is still bound, so the expected result is actually a reset
  // to the default state. (Window rectangles don't affect the backbuffer.)
  EXPECT_CALL(*gl_, WindowRectanglesEXT(GL_EXCLUSIVE_EXT, 0, nullptr));
  cmd.Init(GL_INCLUSIVE_EXT, 2, &temp[0]);
  EXPECT_EQ(error::kNoError, ExecuteImmediateCmd(cmd, sizeof(temp)));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

#include "gpu/command_buffer/service/gles2_cmd_decoder_unittest_extensions_autogen.h"

}  // namespace gles2
}  // namespace gpu
