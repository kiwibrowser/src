// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_COPY_TEXTURE_CHROMIUM_MOCK_H_
#define GPU_COMMAND_BUFFER_SERVICE_COPY_TEXTURE_CHROMIUM_MOCK_H_

#include "gpu/command_buffer/service/gles2_cmd_copy_texture_chromium.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace gpu {
namespace gles2 {

class MockCopyTextureResourceManager
    : public CopyTextureCHROMIUMResourceManager {
 public:
  MockCopyTextureResourceManager();
  ~MockCopyTextureResourceManager() final;

  MOCK_METHOD2(Initialize,
               void(const DecoderContext* decoder,
                    const gles2::FeatureInfo::FeatureFlags& feature_flags));
  MOCK_METHOD0(Destroy, void());

  // Cannot MOCK_METHOD more than 10 args.
  void DoCopyTexture(
      const DecoderContext* decoder,
      GLenum source_target,
      GLuint source_id,
      GLint source_level,
      GLenum source_internal_format,
      GLenum dest_target,
      GLuint dest_id,
      GLint dest_level,
      GLenum dest_internal_format,
      GLsizei width,
      GLsizei height,
      bool flip_y,
      bool premultiply_alpha,
      bool unpremultiply_alpha,
      bool dither,
      CopyTextureMethod method,
      CopyTexImageResourceManager* luma_emulation_blitter) override {}
  void DoCopySubTexture(
      const DecoderContext* decoder,
      GLenum source_target,
      GLuint source_id,
      GLint source_level,
      GLenum source_internal_format,
      GLenum dest_target,
      GLuint dest_id,
      GLint dest_level,
      GLenum dest_internal_format,
      GLint xoffset,
      GLint yoffset,
      GLint x,
      GLint y,
      GLsizei width,
      GLsizei height,
      GLsizei dest_width,
      GLsizei dest_height,
      GLsizei source_width,
      GLsizei source_height,
      bool flip_y,
      bool premultiply_alpha,
      bool unpremultiply_alpha,
      bool dither,
      CopyTextureMethod method,
      CopyTexImageResourceManager* luma_emulation_blitter) override {}
  void DoCopySubTextureWithTransform(
      const DecoderContext* decoder,
      GLenum source_target,
      GLuint source_id,
      GLint source_level,
      GLenum source_internal_format,
      GLenum dest_target,
      GLuint dest_id,
      GLint dest_level,
      GLenum dest_internal_format,
      GLint xoffset,
      GLint yoffset,
      GLint x,
      GLint y,
      GLsizei width,
      GLsizei height,
      GLsizei dest_width,
      GLsizei dest_height,
      GLsizei source_width,
      GLsizei source_height,
      bool flip_y,
      bool premultiply_alpha,
      bool unpremultiply_alpha,
      bool dither,
      const GLfloat transform_matrix[16],
      CopyTexImageResourceManager* luma_emulation_blitter) override{};
  void DoCopyTextureWithTransform(
      const DecoderContext* decoder,
      GLenum source_target,
      GLuint source_id,
      GLint source_level,
      GLenum source_format,
      GLenum dest_target,
      GLuint dest_id,
      GLint dest_level,
      GLenum dest_format,
      GLsizei width,
      GLsizei height,
      bool flip_y,
      bool premultiply_alpha,
      bool unpremultiply_alpha,
      bool dither,
      const GLfloat transform_matrix[16],
      CopyTexImageResourceManager* luma_emulation_blitter) override{};

 private:
  DISALLOW_COPY_AND_ASSIGN(MockCopyTextureResourceManager);
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_COPY_TEXTURE_CHROMIUM_MOCK_H_
