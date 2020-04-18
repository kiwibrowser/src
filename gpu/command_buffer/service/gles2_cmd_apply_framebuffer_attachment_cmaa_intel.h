// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_GLES2_CMD_APPLY_FRAMEBUFFER_ATTACHMENT_CMAA_INTEL_H_
#define GPU_COMMAND_BUFFER_SERVICE_GLES2_CMD_APPLY_FRAMEBUFFER_ATTACHMENT_CMAA_INTEL_H_

#include "gpu/command_buffer/service/gl_utils.h"
#include "gpu/gpu_gles2_export.h"

namespace gpu {
namespace gles2 {
class CopyTextureCHROMIUMResourceManager;
class GLES2Decoder;
class Framebuffer;
class TextureManager;

// This class encapsulates the resources required to implement the
// GL_INTEL_framebuffer_CMAA extension via shaders.
//
// The CMAA Conservative Morphological Anti-Aliasing) algorithm is applied to
// all color attachments of the currently bound draw framebuffer.
//
// Reference GL_INTEL_framebuffer_CMAA for details.
class GPU_GLES2_EXPORT ApplyFramebufferAttachmentCMAAINTELResourceManager {
 public:
  ApplyFramebufferAttachmentCMAAINTELResourceManager();
  ~ApplyFramebufferAttachmentCMAAINTELResourceManager();

  void Initialize(gles2::GLES2Decoder* decoder);
  void Destroy();

  // Applies the algorithm to the color attachments of the currently bound draw
  // framebuffer.
  void ApplyFramebufferAttachmentCMAAINTEL(
      GLES2Decoder* decoder,
      Framebuffer* framebuffer,
      CopyTextureCHROMIUMResourceManager* copier,
      TextureManager* texture_manager);

 private:
  // Applies the CMAA algorithm to a texture.
  void ApplyCMAAEffectTexture(GLuint source_texture,
                              GLuint dest_texture,
                              bool do_copy);

  void OnSize(GLint width, GLint height);
  void ReleaseTextures();

  GLuint CreateProgram(const char* defines,
                       const char* vs_source,
                       const char* fs_source);
  GLuint CreateShader(GLenum type, const char* defines, const char* source);

  bool initialized_;
  bool textures_initialized_;
  bool is_in_gamma_correct_mode_;
  bool supports_usampler_;
  bool supports_r8_image_;
  bool is_gles31_compatible_;

  int frame_id_;

  GLint width_;
  GLint height_;

  GLuint edges0_shader_;
  GLuint edges1_shader_;
  GLuint edges_combine_shader_;
  GLuint process_and_apply_shader_;
  GLuint debug_display_edges_shader_;

  GLuint cmaa_framebuffer_;

  GLuint rgba8_texture_;
  GLuint working_color_texture_;
  GLuint edges0_texture_;
  GLuint edges1_texture_;
  GLuint mini4_edge_texture_;
  GLuint mini4_edge_depth_texture_;

  GLuint edges0_shader_result_rgba_texture_slot1_;
  GLuint edges0_shader_target_texture_slot2_;
  GLuint edges1_shader_result_edge_texture_;
  GLuint process_and_apply_shader_result_rgba_texture_slot1_;
  GLuint edges_combine_shader_result_edge_texture_;

  static const char vert_str_[];
  static const char cmaa_frag_s1_[];
  static const char cmaa_frag_s2_[];

  DISALLOW_COPY_AND_ASSIGN(ApplyFramebufferAttachmentCMAAINTELResourceManager);
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_GLES2_CMD_APPLY_FRAMEBUFFER_ATTACHMENT_CMAA_INTEL_H_
