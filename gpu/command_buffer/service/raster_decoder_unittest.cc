// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/raster_decoder.h"

#include "base/command_line.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/raster_cmd_format.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/memory_tracking.h"
#include "gpu/command_buffer/service/program_manager.h"
#include "gpu/command_buffer/service/query_manager.h"
#include "gpu/command_buffer/service/raster_decoder_unittest_base.h"
#include "gpu/command_buffer/service/test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_image_stub.h"
#include "ui/gl/gl_mock.h"

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgPointee;

using namespace gpu::raster::cmds;

namespace gpu {
namespace raster {

namespace {
const GLsizei kWidth = 10;
const GLsizei kHeight = 20;
const GLint kImageId = 1;

class MockMemoryTracker : public gles2::MemoryTracker {
 public:
  MockMemoryTracker() {}

  // Ensure a certain amount of GPU memory is free. Returns true on success.
  MOCK_METHOD1(EnsureGPUMemoryAvailable, bool(size_t size_needed));

  void TrackMemoryAllocatedChange(size_t old_size, size_t new_size) override {}

  uint64_t ClientTracingId() const override { return 0; }
  int ClientId() const override { return 0; }
  uint64_t ShareGroupTracingGUID() const override { return 0; }

 private:
  ~MockMemoryTracker() override = default;
};

}  // namespace

class RasterDecoderTest : public RasterDecoderTestBase {
 public:
  RasterDecoderTest() = default;
};

INSTANTIATE_TEST_CASE_P(Service, RasterDecoderTest, ::testing::Bool());
INSTANTIATE_TEST_CASE_P(Service,
                        RasterDecoderManualInitTest,
                        ::testing::Bool());

TEST_P(RasterDecoderTest, TexParameteriValidArgs) {
  SetScopedTextureBinderExpectations(GL_TEXTURE_2D);
  EXPECT_CALL(*gl_,
              TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  cmds::TexParameteri cmd;
  cmd.Init(client_texture_id_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  SetScopedTextureBinderExpectations(GL_TEXTURE_2D);
  EXPECT_CALL(*gl_,
              TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  cmd.Init(client_texture_id_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  SetScopedTextureBinderExpectations(GL_TEXTURE_2D);
  EXPECT_CALL(
      *gl_, TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  cmd.Init(client_texture_id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  SetScopedTextureBinderExpectations(GL_TEXTURE_2D);
  EXPECT_CALL(
      *gl_, TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  cmd.Init(client_texture_id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_P(RasterDecoderTest, TexParameteriInvalidArgsMipMap) {
  EXPECT_CALL(*gl_, TexParameteri(_, _, _)).Times(0);
  cmds::TexParameteri cmd;
  cmd.Init(client_texture_id_, GL_GENERATE_MIPMAP, GL_NEAREST);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_ENUM, GetGLError());
}

TEST_P(RasterDecoderTest, TexParameteriInvalidArgsMagFilter) {
  EXPECT_CALL(*gl_, TexParameteri(_, _, _)).Times(0);
  cmds::TexParameteri cmd;
  cmd.Init(client_texture_id_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_ENUM, GetGLError());
}

TEST_P(RasterDecoderTest, TexParameteriInvalidArgsMinFilter) {
  EXPECT_CALL(*gl_, TexParameteri(_, _, _)).Times(0);
  cmds::TexParameteri cmd;
  cmd.Init(client_texture_id_, GL_TEXTURE_MIN_FILTER,
           GL_NEAREST_MIPMAP_NEAREST);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_ENUM, GetGLError());
}

TEST_P(RasterDecoderTest, TexParameteriInvalidArgsWrapS) {
  EXPECT_CALL(*gl_, TexParameteri(_, _, _)).Times(0);
  cmds::TexParameteri cmd;
  cmd.Init(client_texture_id_, GL_TEXTURE_WRAP_S, GL_REPEAT);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_ENUM, GetGLError());
}

TEST_P(RasterDecoderTest, TexParameteriInvalidArgsWrapT) {
  EXPECT_CALL(*gl_, TexParameteri(_, _, _)).Times(0);
  cmds::TexParameteri cmd;
  cmd.Init(client_texture_id_, GL_TEXTURE_WRAP_T, GL_REPEAT);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_ENUM, GetGLError());
}

const GLsync kGlSync = reinterpret_cast<GLsync>(0xdeadbeef);

TEST_P(RasterDecoderTest, BeginEndQueryEXTCommandsCompletedCHROMIUM) {
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
  EXPECT_TRUE(query->IsPending());

#if DCHECK_IS_ON()
  EXPECT_CALL(*gl_, IsSync(kGlSync))
      .WillOnce(Return(GL_TRUE))
      .RetiresOnSaturation();
#endif
  EXPECT_CALL(*gl_, ClientWaitSync(kGlSync, _, _))
      .WillOnce(Return(GL_TIMEOUT_EXPIRED))
      .RetiresOnSaturation();
  query_manager->ProcessPendingQueries(false);

  EXPECT_TRUE(query->IsPending());

#if DCHECK_IS_ON()
  EXPECT_CALL(*gl_, IsSync(kGlSync))
      .WillOnce(Return(GL_TRUE))
      .RetiresOnSaturation();
#endif
  EXPECT_CALL(*gl_, ClientWaitSync(kGlSync, _, _))
      .WillOnce(Return(GL_ALREADY_SIGNALED))
      .RetiresOnSaturation();
  query_manager->ProcessPendingQueries(false);

  EXPECT_FALSE(query->IsPending());

#if DCHECK_IS_ON()
  EXPECT_CALL(*gl_, IsSync(kGlSync))
      .WillOnce(Return(GL_TRUE))
      .RetiresOnSaturation();
#endif
  EXPECT_CALL(*gl_, DeleteSync(kGlSync)).Times(1).RetiresOnSaturation();
  ResetDecoder();
}

TEST_P(RasterDecoderTest, BeginEndQueryEXTCommandsIssuedCHROMIUM) {
  BeginQueryEXT begin_cmd;

  GenHelper<GenQueriesEXTImmediate>(kNewClientId);

  // Test valid parameters work.
  begin_cmd.Init(GL_COMMANDS_ISSUED_CHROMIUM, kNewClientId, shared_memory_id_,
                 kSharedMemoryOffset);
  EXPECT_EQ(error::kNoError, ExecuteCmd(begin_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  QueryManager* query_manager = decoder_->GetQueryManager();
  ASSERT_TRUE(query_manager != nullptr);
  QueryManager::Query* query = query_manager->GetQuery(kNewClientId);
  ASSERT_TRUE(query != nullptr);
  EXPECT_FALSE(query->IsPending());

  // Test end succeeds
  EndQueryEXT end_cmd;
  end_cmd.Init(GL_COMMANDS_ISSUED_CHROMIUM, 1);
  EXPECT_EQ(error::kNoError, ExecuteCmd(end_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
  EXPECT_FALSE(query->IsPending());
}

TEST_P(RasterDecoderTest, TexStorage2D) {
  DoTexStorage2D(client_texture_id_, 1 /* levels */, kWidth, kHeight);

  gles2::TextureRef* texture_ref =
      group().texture_manager()->GetTexture(client_texture_id_);
  ASSERT_TRUE(texture_ref != nullptr);
  gles2::Texture* texture = texture_ref->texture();

  EXPECT_EQ(kServiceTextureId, texture->service_id());

  GLsizei width;
  GLsizei height;
  EXPECT_TRUE(
      texture->GetLevelSize(GL_TEXTURE_2D, 0, &width, &height, nullptr));
  EXPECT_EQ(width, kWidth);
  EXPECT_EQ(height, kHeight);
}

TEST_P(RasterDecoderManualInitTest, TexStorage2DWithEXTTextureStorage) {
  InitState init;
  init.extensions.push_back("GL_EXT_texture_storage");
  InitDecoder(init);

  DoTexStorage2D(client_texture_id_, 2 /* levels */, kWidth, kHeight);

  gles2::TextureRef* texture_ref =
      group().texture_manager()->GetTexture(client_texture_id_);
  ASSERT_TRUE(texture_ref != nullptr);
  gles2::Texture* texture = texture_ref->texture();

  EXPECT_EQ(kServiceTextureId, texture->service_id());

  GLsizei width;
  GLsizei height;
  EXPECT_TRUE(
      texture->GetLevelSize(GL_TEXTURE_2D, 0, &width, &height, nullptr));
  EXPECT_EQ(width, kWidth);
  EXPECT_EQ(height, kHeight);

  EXPECT_TRUE(
      texture->GetLevelSize(GL_TEXTURE_2D, 1, &width, &height, nullptr));
  EXPECT_EQ(width, kWidth / 2);
  EXPECT_EQ(height, kHeight / 2);
}

TEST_P(RasterDecoderManualInitTest, TexStorage2DOutOfMemory) {
  scoped_refptr<MockMemoryTracker> memory_tracker = new MockMemoryTracker();
  set_memory_tracker(memory_tracker.get());

  InitDecoder(InitState());

  EXPECT_CALL(*memory_tracker.get(), EnsureGPUMemoryAvailable(_))
      .WillOnce(Return(false))
      .RetiresOnSaturation();

  cmds::TexStorage2D cmd;
  cmd.Init(client_texture_id_, 1 /* levels */, kWidth, kHeight);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_OUT_OF_MEMORY, GetGLError());
}

TEST_P(RasterDecoderTest, TexStorage2DInvalid) {
  // Bad client id
  cmds::TexStorage2D cmd;
  cmd.Init(kInvalidClientId, 1 /* levels */, kWidth, kHeight);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());

  // Bad levels
  cmd.Init(client_texture_id_, 0 /* levels */, kWidth, kHeight);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());

  // Bad width
  cmd.Init(client_texture_id_, 1 /* levels */, 0, kHeight);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());

  // Bad height
  cmd.Init(client_texture_id_, 1 /* levels */, kWidth, 0);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_VALUE, GetGLError());
}

TEST_P(RasterDecoderTest, ProduceAndConsumeTexture) {
  Mailbox mailbox = Mailbox::Generate();
  GLuint new_texture_id = kNewClientId;

  DoTexStorage2D(client_texture_id_, 1 /* levels */, kWidth, kHeight);

  ProduceTextureDirectImmediate& produce_cmd =
      *GetImmediateAs<ProduceTextureDirectImmediate>();
  produce_cmd.Init(client_texture_id_, mailbox.name);
  EXPECT_EQ(error::kNoError,
            ExecuteImmediateCmd(produce_cmd, sizeof(mailbox.name)));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Check that ProduceTextureDirect did not change attributes.
  gles2::TextureRef* texture_ref =
      group().texture_manager()->GetTexture(client_texture_id_);
  ASSERT_TRUE(texture_ref != nullptr);
  gles2::Texture* texture = texture_ref->texture();

  GLsizei width, height;
  EXPECT_TRUE(
      texture->GetLevelSize(GL_TEXTURE_2D, 0, &width, &height, nullptr));
  EXPECT_EQ(width, kWidth);
  EXPECT_EQ(height, kHeight);
  EXPECT_EQ(kServiceTextureId, texture->service_id());

  CreateAndConsumeTextureINTERNALImmediate& consume_cmd =
      *GetImmediateAs<CreateAndConsumeTextureINTERNALImmediate>();
  consume_cmd.Init(new_texture_id, false /* use_buffer */,
                   gfx::BufferUsage::GPU_READ, viz::ResourceFormat::RGBA_8888,
                   mailbox.name);
  EXPECT_EQ(error::kNoError,
            ExecuteImmediateCmd(consume_cmd, sizeof(mailbox.name)));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Check that new_texture_id has appropriate attributes like width and height.
  texture_ref = group().texture_manager()->GetTexture(new_texture_id);
  ASSERT_NE(texture_ref, nullptr);
  texture = texture_ref->texture();
  EXPECT_TRUE(
      texture->GetLevelSize(GL_TEXTURE_2D, 0, &width, &height, nullptr));
  EXPECT_EQ(width, kWidth);
  EXPECT_EQ(height, kHeight);
  EXPECT_EQ(kServiceTextureId, texture->service_id());
}

TEST_P(RasterDecoderTest, ReleaseTexImage2DCHROMIUM) {
  scoped_refptr<gl::GLImage> image(new gl::GLImageStub);
  GetImageManagerForTest()->AddImage(image.get(), kImageId);
  EXPECT_FALSE(GetImageManagerForTest()->LookupImage(kImageId) == nullptr);

  // Bind image to texture.
  SetScopedTextureBinderExpectations(GL_TEXTURE_2D);
  cmds::BindTexImage2DCHROMIUM bind_tex_image_2d_cmd;
  bind_tex_image_2d_cmd.Init(client_texture_id_, kImageId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(bind_tex_image_2d_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  gles2::TextureRef* texture_ref =
      group().texture_manager()->GetTexture(client_texture_id_);
  ASSERT_TRUE(texture_ref != nullptr);
  gles2::Texture* texture = texture_ref->texture();
  GLsizei width;
  GLsizei height;
  EXPECT_TRUE(
      texture->GetLevelSize(GL_TEXTURE_2D, 0, &width, &height, nullptr));
  // Dimensions from |image| above.
  EXPECT_EQ(1, width);
  EXPECT_EQ(1, height);

  // Image should now be set.
  EXPECT_FALSE(texture->GetLevelImage(GL_TEXTURE_2D, 0) == nullptr);

  // Release image from texture.
  SetScopedTextureBinderExpectations(GL_TEXTURE_2D);
  cmds::ReleaseTexImage2DCHROMIUM release_tex_image_2d_cmd;
  release_tex_image_2d_cmd.Init(client_texture_id_, kImageId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(release_tex_image_2d_cmd));

  EXPECT_TRUE(
      texture->GetLevelSize(GL_TEXTURE_2D, 0, &width, &height, nullptr));
  // Size reset.
  EXPECT_EQ(0, width);
  EXPECT_EQ(0, height);

  // Image should no longer be set.
  EXPECT_TRUE(texture->GetLevelImage(GL_TEXTURE_2D, 0) == nullptr);
}

TEST_P(RasterDecoderTest, CopyTexSubImage2DTwiceClearsUnclearedTexture) {
  // Create uninitialized source texture.
  GLuint source_texture_id = kNewClientId;
  EXPECT_CALL(*gl_, GenTextures(1, _))
      .WillOnce(SetArgPointee<1>(kNewServiceId))
      .RetiresOnSaturation();
  cmds::CreateTexture cmd;
  cmd.Init(false /* use_buffer */, gfx::BufferUsage::GPU_READ,
           viz::ResourceFormat::RGBA_8888, source_texture_id);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));

  // Set dimensions on source and dest textures.
  DoTexStorage2D(source_texture_id, 1 /* levels */, 2, 2);
  DoTexStorage2D(client_texture_id_, 1 /* levels */, 2, 2);

  // This will initialize the top half of destination.
  {
    // Source is undefined, so first call to CopySubTexture will clear the
    // source.
    SetupClearTextureExpectations(kNewServiceId, kServiceTextureId,
                                  GL_TEXTURE_2D, GL_TEXTURE_2D, 0, GL_RGBA,
                                  GL_UNSIGNED_BYTE, 0, 0, 2, 2, 0);
    SetScopedTextureBinderExpectations(GL_TEXTURE_2D);
    CopySubTexture cmd;
    cmd.Init(source_texture_id, client_texture_id_, 0, 0, 0, 0, 2, 1);
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  }

  // This will initialize bottom right corner of the destination.
  // CopySubTexture will clear the bottom half of the destination because a
  // single rectangle is insufficient to keep track of the initialized area.
  {
    SetupClearTextureExpectations(kServiceTextureId, kServiceTextureId,
                                  GL_TEXTURE_2D, GL_TEXTURE_2D, 0, GL_RGBA,
                                  GL_UNSIGNED_BYTE, 0, 1, 2, 1, 0);
    SetScopedTextureBinderExpectations(GL_TEXTURE_2D);
    CopySubTexture cmd;
    cmd.Init(source_texture_id, client_texture_id_, 1, 1, 0, 0, 1, 1);
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  }

  gles2::TextureManager* manager = group().texture_manager();
  gles2::TextureRef* texture_ref = manager->GetTexture(client_texture_id_);
  ASSERT_TRUE(texture_ref != NULL);
  gles2::Texture* texture = texture_ref->texture();
  EXPECT_TRUE(texture->SafeToRenderFrom());
}

TEST_P(RasterDecoderTest, GLImageAttachedAfterClearLevel) {
  scoped_refptr<gl::GLImage> image(new gl::GLImageStub);
  GetImageManagerForTest()->AddImage(image.get(), kImageId);

  // Bind image to texture.
  SetScopedTextureBinderExpectations(GL_TEXTURE_2D);
  cmds::BindTexImage2DCHROMIUM bind_tex_image_2d_cmd;
  bind_tex_image_2d_cmd.Init(client_texture_id_, kImageId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(bind_tex_image_2d_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  // Check binding.
  gles2::TextureRef* texture_ref =
      group().texture_manager()->GetTexture(client_texture_id_);
  ASSERT_TRUE(texture_ref != nullptr);
  gles2::Texture* texture = texture_ref->texture();
  EXPECT_TRUE(texture->GetLevelImage(GL_TEXTURE_2D, 0) == image.get());

  GLenum target = GL_TEXTURE_2D;
  GLint level = 0;
  GLint xoffset = 0;
  GLint yoffset = 0;
  GLsizei width = 1;
  GLsizei height = 1;
  GLenum format = GL_RGBA;
  GLenum type = GL_UNSIGNED_BYTE;

  // ClearLevel should use glTexSubImage2D to avoid unbinding GLImage.
  SetupClearTextureExpectations(kServiceTextureId, 0 /* old_service_id */,
                                GL_TEXTURE_2D, GL_TEXTURE_2D, level, format,
                                type, xoffset, yoffset, width, height, 0);

  GetDecoder()->ClearLevel(texture, target, level, format, type, xoffset,
                           yoffset, width, height);
  EXPECT_TRUE(texture->GetLevelImage(GL_TEXTURE_2D, 0) == image.get());
}

}  // namespace raster
}  // namespace gpu
