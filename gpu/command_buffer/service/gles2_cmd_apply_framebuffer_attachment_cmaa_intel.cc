// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gles2_cmd_apply_framebuffer_attachment_cmaa_intel.h"

#include "base/logging.h"
#include "gpu/command_buffer/service/framebuffer_manager.h"
#include "gpu/command_buffer/service/gles2_cmd_copy_texture_chromium.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_version_info.h"

#define SHADER(Src) #Src

namespace gpu {
namespace gles2 {

ApplyFramebufferAttachmentCMAAINTELResourceManager::
    ApplyFramebufferAttachmentCMAAINTELResourceManager()
    : initialized_(false),
      textures_initialized_(false),
      is_in_gamma_correct_mode_(false),
      supports_usampler_(true),
      supports_r8_image_(true),
      is_gles31_compatible_(false),
      frame_id_(0),
      width_(0),
      height_(0),
      edges0_shader_(0),
      edges1_shader_(0),
      edges_combine_shader_(0),
      process_and_apply_shader_(0),
      debug_display_edges_shader_(0),
      cmaa_framebuffer_(0),
      rgba8_texture_(0),
      working_color_texture_(0),
      edges0_texture_(0),
      edges1_texture_(0),
      mini4_edge_texture_(0),
      mini4_edge_depth_texture_(0),
      edges0_shader_result_rgba_texture_slot1_(0),
      edges0_shader_target_texture_slot2_(0),
      edges1_shader_result_edge_texture_(0),
      process_and_apply_shader_result_rgba_texture_slot1_(0),
      edges_combine_shader_result_edge_texture_(0) {}

ApplyFramebufferAttachmentCMAAINTELResourceManager::
    ~ApplyFramebufferAttachmentCMAAINTELResourceManager() {
  Destroy();
}

void ApplyFramebufferAttachmentCMAAINTELResourceManager::Initialize(
    gles2::GLES2Decoder* decoder) {
  DCHECK(decoder);
  is_gles31_compatible_ =
      decoder->GetGLContext()->GetVersionInfo()->IsAtLeastGLES(3, 1);

  if (is_gles31_compatible_) {
    supports_r8_image_ =
        decoder->GetGLContext()->HasExtension("GL_NV_image_formats");

    // ES 3.0 requires GL_RGBA8UI is color renderable.
    supports_usampler_ = true;
  } else {
    // CMAA requires GL_ARB_shader_image_load_store for GL, and it requires r8
    // image texture.
    DCHECK(decoder->GetGLContext()->HasExtension(
        "GL_ARB_shader_image_load_store"));
    supports_r8_image_ = true;

    // Check if RGBA8UI is supported as an FBO colour target with depth.
    // If not supported, GLSL needs to convert the data to/from float so there
    // is a small extra cost.
    {
      glActiveTexture(GL_TEXTURE0);

      GLuint rgba8ui_texture = 0, depth_texture = 0;
      glGenTextures(1, &rgba8ui_texture);
      glBindTexture(GL_TEXTURE_2D, rgba8ui_texture);
      glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8UI, 4, 4);

      glGenTextures(1, &depth_texture);
      glBindTexture(GL_TEXTURE_2D, depth_texture);
      glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, 4, 4);

      // Create the FBO
      GLuint rgba8ui_framebuffer = 0;
      glGenFramebuffersEXT(1, &rgba8ui_framebuffer);
      glBindFramebufferEXT(GL_FRAMEBUFFER, rgba8ui_framebuffer);

      // Bind to the FBO to test support
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, rgba8ui_texture, 0);
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                GL_TEXTURE_2D, depth_texture, 0);
      GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);

      supports_usampler_ = (status == GL_FRAMEBUFFER_COMPLETE);

      glDeleteFramebuffersEXT(1, &rgba8ui_framebuffer);
      glDeleteTextures(1, &rgba8ui_texture);
      glDeleteTextures(1, &depth_texture);

      decoder->RestoreTextureUnitBindings(0);
      decoder->RestoreActiveTexture();
      decoder->RestoreFramebufferBindings();
    }
  }

  VLOG(1) << "ApplyFramebufferAttachmentCMAAINTEL: "
          << "Supports USampler is " << (supports_usampler_ ? "true" : "false");
  VLOG(1) << "ApplyFramebufferAttachmentCMAAINTEL: "
          << "Supports R8 Images is "
          << (supports_r8_image_ ? "true" : "false");

  // Create the shaders
  std::ostringstream defines, edge1, edge2, combineEdges, blur, displayEdges,
      cmaa_frag;

  cmaa_frag << cmaa_frag_s1_ << cmaa_frag_s2_;
  std::string cmaa_frag_string = cmaa_frag.str();
  const char* cmaa_frag_c_str = cmaa_frag_string.c_str();

  if (supports_usampler_) {
    defines << "#define SUPPORTS_USAMPLER2D\n";
  }

  if (is_in_gamma_correct_mode_) {
    defines << "#define IN_GAMMA_CORRECT_MODE\n";
  }

  if (supports_r8_image_) {
    defines << "#define EDGE_READ_FORMAT r8\n";
  } else {
    defines << "#define EDGE_READ_FORMAT r32f\n";
  }

  displayEdges << defines.str() << "#define DISPLAY_EDGES\n";
  debug_display_edges_shader_ =
      CreateProgram(displayEdges.str().c_str(), vert_str_, cmaa_frag_c_str);

  edge1 << defines.str() << "#define DETECT_EDGES1\n";
  edges0_shader_ =
      CreateProgram(edge1.str().c_str(), vert_str_, cmaa_frag_c_str);

  edge2 << defines.str() << "#define DETECT_EDGES2\n";
  edges1_shader_ =
      CreateProgram(edge2.str().c_str(), vert_str_, cmaa_frag_c_str);

  combineEdges << defines.str() << "#define COMBINE_EDGES\n";
  edges_combine_shader_ =
      CreateProgram(combineEdges.str().c_str(), vert_str_, cmaa_frag_c_str);

  blur << defines.str() << "#define BLUR_EDGES\n";
  process_and_apply_shader_ =
      CreateProgram(blur.str().c_str(), vert_str_, cmaa_frag_c_str);

  edges0_shader_result_rgba_texture_slot1_ =
      glGetUniformLocation(edges0_shader_, "g_resultRGBATextureSlot1");
  edges0_shader_target_texture_slot2_ =
      glGetUniformLocation(edges0_shader_, "g_targetTextureSlot2");
  edges1_shader_result_edge_texture_ =
      glGetUniformLocation(edges1_shader_, "g_resultEdgeTexture");
  edges_combine_shader_result_edge_texture_ =
      glGetUniformLocation(edges_combine_shader_, "g_resultEdgeTexture");
  process_and_apply_shader_result_rgba_texture_slot1_ = glGetUniformLocation(
      process_and_apply_shader_, "g_resultRGBATextureSlot1");

  initialized_ = true;
}

void ApplyFramebufferAttachmentCMAAINTELResourceManager::Destroy() {
  if (!initialized_)
    return;

  ReleaseTextures();

  glDeleteProgram(process_and_apply_shader_);
  glDeleteProgram(edges_combine_shader_);
  glDeleteProgram(edges1_shader_);
  glDeleteProgram(edges0_shader_);
  glDeleteProgram(debug_display_edges_shader_);

  initialized_ = false;
}

// Apply CMAA(Conservative Morphological Anti-Aliasing) algorithm to the
// color attachments of currently bound draw framebuffer.
// Reference GL_INTEL_framebuffer_CMAA for details.
void ApplyFramebufferAttachmentCMAAINTELResourceManager::
    ApplyFramebufferAttachmentCMAAINTEL(
        GLES2Decoder* decoder,
        Framebuffer* framebuffer,
        CopyTextureCHROMIUMResourceManager* copier,
        TextureManager* texture_manager) {
  DCHECK(decoder);
  DCHECK(initialized_);
  if (!framebuffer)
    return;

  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  if (decoder->GetFeatureInfo()->feature_flags().ext_window_rectangles) {
    glWindowRectanglesEXT(GL_EXCLUSIVE_EXT, 0, nullptr);
  }

  // Process each color attachment of the current draw framebuffer.
  uint32_t max_draw_buffers = decoder->GetContextGroup()->max_draw_buffers();
  for (uint32_t i = 0; i < max_draw_buffers; i++) {
    const gles2::Framebuffer::Attachment* attachment =
        framebuffer->GetAttachment(GL_COLOR_ATTACHMENT0 + i);
    if (attachment && attachment->IsTextureAttachment()) {
      // Get the texture info.
      GLuint source_texture_client_id = attachment->object_name();
      GLuint source_texture = 0;
      if (!decoder->GetServiceTextureId(source_texture_client_id,
                                        &source_texture))
        continue;
      GLsizei width = attachment->width();
      GLsizei height = attachment->height();
      GLenum internal_format = attachment->internal_format();

      // Resize internal structures - only if needed.
      OnSize(width, height);

      // CMAA internally expects GL_RGBA8 textures.
      // Process using a GL_RGBA8 copy if this is not the case.
      DCHECK(attachment->object_name());
      TextureRef* texture =
          texture_manager->GetTexture(attachment->object_name());
      const bool rgba_immutable =
          texture->texture()->IsImmutable() &&
          TextureManager::ExtractFormatFromStorageFormat(internal_format) ==
              GL_RGBA;
      const bool do_copy = !rgba_immutable;

      // CMAA Effect
      if (do_copy) {
        ApplyCMAAEffectTexture(source_texture, rgba8_texture_, do_copy);

        // Source format for DoCopySubTexture is always GL_RGBA8.
        CopyTextureMethod method = CopyTextureMethod::DIRECT_COPY;
        bool copy_tex_image_format_valid =
            !GLES2Util::IsIntegerFormat(internal_format) &&
            GLES2Util::GetColorEncodingFromInternalFormat(internal_format) !=
                GL_SRGB &&
            internal_format != GL_BGRA_EXT && internal_format != GL_BGRA8_EXT;
        if (GLES2Util::IsSizedColorFormat(internal_format)) {
          int dr, dg, db, da;
          GLES2Util::GetColorFormatComponentSizes(internal_format, 0, &dr, &dg,
                                                  &db, &da);
          if ((dr > 0 && dr != 8) || (dg > 0 && dg != 8) ||
              (db > 0 && db != 8) || (da > 0 && da != 8)) {
            copy_tex_image_format_valid = false;
          }
        }
        if (!copy_tex_image_format_valid)
          method = CopyTextureMethod::DIRECT_DRAW;
        bool color_renderable =
            Texture::ColorRenderable(decoder->GetFeatureInfo(), internal_format,
                                     texture->texture()->IsImmutable());
#if defined(OS_CHROMEOS) && defined(ARCH_CPU_X86_FAMILY)
        // glDrawArrays is faster than glCopyTexSubImage2D on IA Mesa driver,
        // although opposite in Android.
        // TODO(dshwang): After Mesa fixes this issue, remove this hack.
        // https://bugs.freedesktop.org/show_bug.cgi?id=98478, crbug.com/535198.
        if (color_renderable)
          method = CopyTextureMethod::DIRECT_DRAW;
#endif
        if (method == CopyTextureMethod::DIRECT_DRAW && !color_renderable)
          method = CopyTextureMethod::DRAW_AND_COPY;

        // LUMINANCE, LUMINANCE_ALPHA and ALPHA textures aren't
        // renderable, so we don't need to pass in the luma emulation
        // blitter to this point.
        copier->DoCopySubTexture(decoder, GL_TEXTURE_2D, rgba8_texture_, 0,
                                 GL_RGBA8, GL_TEXTURE_2D, source_texture, 0,
                                 internal_format, 0, 0, 0, 0, width_, height_,
                                 width_, height_, width_, height_, false, false,
                                 false, false, method, nullptr);
      } else {
        ApplyCMAAEffectTexture(source_texture, source_texture, do_copy);
      }

      decoder->RestoreTextureState(source_texture);
    }
  }

  // Restore state
  decoder->RestoreAllAttributes();
  decoder->RestoreTextureUnitBindings(0);
  decoder->RestoreTextureUnitBindings(1);
  decoder->RestoreActiveTexture();
  decoder->RestoreProgramBindings();
  decoder->RestoreBufferBindings();
  decoder->RestoreFramebufferBindings();
  decoder->RestoreGlobalState();
}

void ApplyFramebufferAttachmentCMAAINTELResourceManager::ApplyCMAAEffectTexture(
    GLuint source_texture,
    GLuint dest_texture,
    bool do_copy) {
  frame_id_++;

  GLuint edge_texture_a;
  GLuint edge_texture_b;

  // Flip flop - One pass clears the texture that needs clearing for the other
  // one (actually it's only important that it clears the highest bit)
  if ((frame_id_ % 2) == 0) {
    edge_texture_a = edges0_texture_;
    edge_texture_b = edges1_texture_;
  } else {
    edge_texture_a = edges1_texture_;
    edge_texture_b = edges0_texture_;
  }

  // Setup the main fbo
  glBindFramebufferEXT(GL_FRAMEBUFFER, cmaa_framebuffer_);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            mini4_edge_texture_, 0);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                            mini4_edge_depth_texture_, 0);
#if DCHECK_IS_ON()
  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    DLOG(ERROR) << "ApplyFramebufferAttachmentCMAAINTEL: "
                << "Incomplete framebuffer.";
    Destroy();
    return;
  }
#endif

  // Setup the viewport to match the fbo
  glViewport(0, 0, (width_ + 1) / 2, (height_ + 1) / 2);
  glEnable(GL_DEPTH_TEST);

  // Detect edges Pass 0
  //   - For every pixel detect edges to the right and down and output depth
  //   mask where edges detected (1 - far, for detected, 0-near for empty
  //   pixels)

  // Inputs
  //  g_screenTexture                     source_texture               tex0
  // Outputs
  //  gl_FragDepth                        mini4_edge_depth_texture_    fbo.depth
  //  out uvec4 outEdges                  mini4_edge_texture_          fbo.col
  //  image2D g_resultRGBATextureSlot1    working_color_texture_       image1
  GLenum edge_format = supports_r8_image_ ? GL_R8 : GL_R32F;

  {
    glUseProgram(edges0_shader_);
    glUniform2f(0, 1.0f / width_, 1.0f / height_);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_ALWAYS);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    if (!is_gles31_compatible_) {
      glUniform1i(edges0_shader_result_rgba_texture_slot1_, 1);
      glUniform1i(edges0_shader_target_texture_slot2_, 2);
    }
    glBindImageTextureEXT(1, working_color_texture_, 0, GL_FALSE, 0,
                          GL_WRITE_ONLY, GL_RGBA8);
    if (do_copy) {
      glUniform1i(2, GL_TRUE);
      glBindImageTextureEXT(2, dest_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                            GL_RGBA8);
    } else {
      glUniform1i(2, GL_FALSE);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  // Detect edges Pass 1 (finish the previous pass edge processing).
  // Do the culling of non-dominant local edges (leave mainly locally dominant
  // edges) and merge Right and Bottom edges into TopRightBottomLeft

  // Inputs
  //  g_src0Texture4Uint                  mini4_edge_texture_          tex1
  // Outputs
  //  image2D g_resultEdgeTexture         edge_texture_b               image0
  {
    glUseProgram(edges1_shader_);
    glUniform2f(0, 1.0f / width_, 1.0f / height_);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LESS);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    if (!is_gles31_compatible_) {
      glUniform1i(edges1_shader_result_edge_texture_, 0);
    }
    glBindImageTextureEXT(0, edge_texture_b, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                          edge_format);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mini4_edge_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  //  - Combine RightBottom (.xy) edges from previous pass into
  //    RightBottomLeftTop (.xyzw) edges and output it into the mask.
  //  - On all pixels with any edge, input buffer into a temporary color buffer
  //    needed for correct blending in the next pass (other pixels not needed
  //    so not copied to avoid bandwidth use).
  //  - On all pixels with 2 or more edges output positive depth mask for the
  //    next pass.

  // Inputs
  //  g_src0TextureFlt                    edge_texture_b               tex1 //ps
  // Outputs
  //  image2D g_resultEdgeTexture         edge_texture_a               image2
  //  gl_FragDepth                        mini4_edge_texture_          fbo.depth
  {
    // Combine edges: each pixel will now contain info on all (top, right,
    // bottom, left) edges; also mark depth 1 value on all pixels with any edge
    // and also copy source color data but only on edge pixels
    glUseProgram(edges_combine_shader_);
    glUniform2f(0, 1.0f / width_, 1.0f / height_);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    if (!is_gles31_compatible_) {
      glUniform1i(edges_combine_shader_result_edge_texture_, 0);
    }
    glBindImageTextureEXT(0, edge_texture_a, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                          edge_format);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, edge_texture_b);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  // Using depth mask and [earlydepthstencil] to work on pixels with 2, 3, 4
  // edges:
  //    - First blend simple blur map for 2,3,4 edge pixels
  //    - Then do the lines (line length counter -should- guarantee no overlap
  //      with other pixels - pixels with 1 edge are excluded in the previous
  //      pass and the pixels with 2 parallel edges are excluded in the simple
  //      blur)

  // Inputs
  //  g_screenTexture                      working_color_texture_      tex0
  //  g_src0TextureFlt                     edge_texture_a              tex1 //ps
  //  sampled
  // Outputs
  //  g_resultRGBATextureSlot1             dest_texture                image1
  //  gl_FragDepth                         mini4_edge_texture_         fbo.depth
  {
    glUseProgram(process_and_apply_shader_);
    glUniform2f(0, 1.0f / width_, 1.0f / height_);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LESS);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    if (!is_gles31_compatible_) {
      glUniform1i(process_and_apply_shader_result_rgba_texture_slot1_, 1);
    }
    glBindImageTextureEXT(1, dest_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                          GL_RGBA8);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, working_color_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, edge_texture_a);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glActiveTexture(GL_TEXTURE0);
}

void ApplyFramebufferAttachmentCMAAINTELResourceManager::OnSize(GLint width,
                                                                GLint height) {
  if (height_ == height && width_ == width)
    return;

  ReleaseTextures();

  height_ = height;
  width_ = width;

  glGenTextures(1, &rgba8_texture_);
  glBindTexture(GL_TEXTURE_2D, rgba8_texture_);
  glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);

  // Edges texture - R8
  // OpenGLES has no single component 8/16-bit image support, so needs to be R32
  // Although CHT does support R8.
  GLenum edge_format = supports_r8_image_ ? GL_R8 : GL_R32F;
  glGenTextures(1, &edges0_texture_);
  glBindTexture(GL_TEXTURE_2D, edges0_texture_);
  glTexStorage2DEXT(GL_TEXTURE_2D, 1, edge_format, width, height);

  glGenTextures(1, &edges1_texture_);
  glBindTexture(GL_TEXTURE_2D, edges1_texture_);
  glTexStorage2DEXT(GL_TEXTURE_2D, 1, edge_format, width, height);

  // Color working texture - RGBA8
  glGenTextures(1, &working_color_texture_);
  glBindTexture(GL_TEXTURE_2D, working_color_texture_);
  glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);

  // Half*half compressed 4-edge-per-pixel texture - RGBA8
  glGenTextures(1, &mini4_edge_texture_);
  glBindTexture(GL_TEXTURE_2D, mini4_edge_texture_);
  GLenum format = GL_RGBA8UI;
  if (!supports_usampler_) {
    format = GL_RGBA8;
  }
  glTexStorage2DEXT(GL_TEXTURE_2D, 1, format, (width + 1) / 2,
                    (height + 1) / 2);

  // Depth
  glGenTextures(1, &mini4_edge_depth_texture_);
  glBindTexture(GL_TEXTURE_2D, mini4_edge_depth_texture_);
  glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, (width + 1) / 2,
                    (height + 1) / 2);

  // Create the FBO
  glGenFramebuffersEXT(1, &cmaa_framebuffer_);
  glBindFramebufferEXT(GL_FRAMEBUFFER, cmaa_framebuffer_);

  // We need to clear the textures before they are first used.
  // The algorithm self-clears them later.
  glViewport(0, 0, width_, height_);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  glBindFramebufferEXT(GL_FRAMEBUFFER, cmaa_framebuffer_);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            edges0_texture_, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            edges1_texture_, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  textures_initialized_ = true;
}

void ApplyFramebufferAttachmentCMAAINTELResourceManager::ReleaseTextures() {
  if (textures_initialized_) {
    glDeleteFramebuffersEXT(1, &cmaa_framebuffer_);
    glDeleteTextures(1, &rgba8_texture_);
    glDeleteTextures(1, &edges0_texture_);
    glDeleteTextures(1, &edges1_texture_);
    glDeleteTextures(1, &mini4_edge_texture_);
    glDeleteTextures(1, &mini4_edge_depth_texture_);
    glDeleteTextures(1, &working_color_texture_);
  }
  textures_initialized_ = false;
}

GLuint ApplyFramebufferAttachmentCMAAINTELResourceManager::CreateProgram(
    const char* defines,
    const char* vs_source,
    const char* fs_source) {
  GLuint program = glCreateProgram();

  GLuint vs = CreateShader(GL_VERTEX_SHADER, defines, vs_source);
  GLuint fs = CreateShader(GL_FRAGMENT_SHADER, defines, fs_source);

  glAttachShader(program, vs);
  glDeleteShader(vs);
  glAttachShader(program, fs);
  glDeleteShader(fs);

  glLinkProgram(program);
  GLint link_status;
  glGetProgramiv(program, GL_LINK_STATUS, &link_status);

  if (link_status == 0) {
#if DCHECK_IS_ON()
    GLint info_log_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
    std::vector<GLchar> info_log(info_log_length);
    glGetProgramInfoLog(program, static_cast<GLsizei>(info_log.size()), NULL,
                        &info_log[0]);
    DLOG(ERROR) << "ApplyFramebufferAttachmentCMAAINTEL: "
                << "program link failed: " << &info_log[0];
#endif
    glDeleteProgram(program);
    program = 0;
  }

  return program;
}

GLuint ApplyFramebufferAttachmentCMAAINTELResourceManager::CreateShader(
    GLenum type,
    const char* defines,
    const char* source) {
  GLuint shader = glCreateShader(type);

  const char header_es31[] =
      "#version 310 es                                                      \n";
  const char header_gl130[] =
      "#version 130                                                         \n"
      "#extension GL_ARB_shading_language_420pack  : require                \n"
      "#extension GL_ARB_texture_gather            : require                \n"
      "#extension GL_ARB_explicit_uniform_location : require                \n"
      "#extension GL_ARB_explicit_attrib_location  : require                \n"
      "#extension GL_ARB_shader_image_load_store   : require                \n";

  std::ostringstream header;
  if (is_gles31_compatible_) {
    header << header_es31;
    if (supports_r8_image_)
      header << "#extension GL_NV_image_formats : require\n";
  } else {
    header << header_gl130;
  }

  std::string header_str = header.str();
  const char* source_array[4] = {header_str.c_str(), defines, "\n", source};
  glShaderSource(shader, 4, source_array, NULL);

  glCompileShader(shader);

  GLint compile_result;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_result);
  if (compile_result == 0) {
#if DCHECK_IS_ON()
    GLint info_log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
    std::vector<GLchar> info_log(info_log_length);
    glGetShaderInfoLog(shader, static_cast<GLsizei>(info_log.size()), NULL,
                       &info_log[0]);
    DLOG(ERROR) << "ApplyFramebufferAttachmentCMAAINTEL: "
                << "shader compilation failed: "
                << (type == GL_VERTEX_SHADER
                        ? "GL_VERTEX_SHADER"
                        : (type == GL_FRAGMENT_SHADER ? "GL_FRAGMENT_SHADER"
                                                      : "UNKNOWN_SHADER"))
                << " shader compilation failed: " << &info_log[0];
#endif
    glDeleteShader(shader);
    shader = 0;
  }

  return shader;
}

/* clang-format off */
// Shaders used in the CMAA algorithm.
const char ApplyFramebufferAttachmentCMAAINTELResourceManager::vert_str_[] =
  SHADER(
    precision highp float;
    // No input data.
    // Verts are autogenerated.
    //
    // vertexID 0,1,2 should generate
    // POS: (-1,-1), (+3,-1), (-1,+3)
    //
    // This generates a triangle that completely covers the -1->1 viewport
    //
    void main()
    {
       float x = -1.0 + float((gl_VertexID & 1) << 2);
       float y = -1.0 + float((gl_VertexID & 2) << 1);
       gl_Position = vec4(x, y, 0.0, 1.0);
    }
  );

const char ApplyFramebufferAttachmentCMAAINTELResourceManager::cmaa_frag_s1_[] =
  SHADER(
    precision highp float;
    precision highp int;

    \n#define SETTINGS_ALLOW_SHORT_Zs 1\n
    \n#define EDGE_DETECT_THRESHOLD 13.0f\n
    \n#define saturate(x) clamp((x), 0.0, 1.0)\n

    // bind to a uniform buffer bind point 0
    layout(location = 0) uniform vec2 g_OneOverScreenSize;
    \n#ifndef EDGE_DETECT_THRESHOLD\n
    layout(location = 1) uniform float g_ColorThreshold;
    \n#endif\n
    \n#ifdef DETECT_EDGES1\n
    layout(location = 2) uniform int g_DoCopy;
    \n#endif\n

    \n#ifdef SUPPORTS_USAMPLER2D\n
    \n#define USAMPLER usampler2D\n
    \n#define UVEC4 uvec4\n
    \n#define LOAD_UINT(arg) arg\n
    \n#define STORE_UVEC4(arg) arg\n
    \n#else\n
    \n#define USAMPLER sampler2D\n
    \n#define UVEC4 vec4\n
    \n#define LOAD_UINT(arg) uint(arg * 255.0f)\n
    \n#define STORE_UVEC4(arg) vec4(float(arg.x) / 255.0f,
                                    float(arg.y) / 255.0f,
                                    float(arg.z) / 255.0f,
                                    float(arg.w) / 255.0f)\n
    \n#endif\n

    // bind to texture stage 0/1
    layout(binding = 0) uniform highp sampler2D g_screenTexture;
    layout(binding = 1) uniform highp sampler2D g_src0TextureFlt;
    layout(binding = 1) uniform highp USAMPLER g_src0Texture4Uint;

    // bind to image stage 0/1/2
    \n#ifdef GL_ES\n
    layout(binding = 0, EDGE_READ_FORMAT) restrict writeonly uniform highp
        image2D g_resultEdgeTexture;
    layout(binding = 1, rgba8) restrict writeonly uniform highp
        image2D g_resultRGBATextureSlot1;
    layout(binding = 2, rgba8) restrict writeonly uniform highp
        image2D g_targetTextureSlot2;
    \n#else\n
    layout(EDGE_READ_FORMAT) restrict writeonly uniform highp
        image2D g_resultEdgeTexture;
    layout(rgba8) restrict writeonly uniform highp
        image2D g_resultRGBATextureSlot1;
    layout(rgba8) restrict writeonly uniform highp
        image2D g_targetTextureSlot2;
    \n#endif\n

    // Constants
    const vec4 c_lumWeights = vec4(0.2126f, 0.7152f, 0.0722f, 0.0000f);

    \n#ifdef EDGE_DETECT_THRESHOLD\n
    const float c_ColorThreshold = 1.0f / EDGE_DETECT_THRESHOLD;
    \n#endif\n

    // Must be even number; Will work with ~16 pretty good too for
    // additional performance, or with ~64 for highest quality.
    const int c_maxLineLength = 64;

    const vec4 c_edgeDebugColours[5] = vec4[5](vec4(0.5, 0.5, 0.5, 0.4),
                                               vec4(1.0, 0.1, 1.0, 0.8),
                                               vec4(0.9, 0.0, 0.0, 0.8),
                                               vec4(0.0, 0.9, 0.0, 0.8),
                                               vec4(0.0, 0.0, 0.9, 0.8));

    // this isn't needed if colour UAV is _SRGB but that doesn't work
    // everywhere
    \n#ifdef IN_GAMMA_CORRECT_MODE\n
    ///////////////////////////////////////////////////////////////////////
    //
    // SRGB Helper Functions taken from D3DX_DXGIFormatConvert.inl
    float D3DX_FLOAT_to_SRGB(float val) {
      if (val < 0.0031308f)
        val *= 12.92f;
      else {
        val = 1.055f * pow(val, 1.0f / 2.4f) - 0.055f;
      }
      return val;
    }
    //
    vec3 D3DX_FLOAT3_to_SRGB(vec3 val) {
      vec3 outVal;
      outVal.x = D3DX_FLOAT_to_SRGB(val.x);
      outVal.y = D3DX_FLOAT_to_SRGB(val.y);
      outVal.z = D3DX_FLOAT_to_SRGB(val.z);
      return outVal;
    }
    ///////////////////////////////////////////////////////////////////////
    \n#endif\n  // IN_GAMMA_CORRECT_MODE

    // how .rgba channels from the edge texture maps to pixel edges:
    //
    //                   A - 0x02
    //              |¯¯¯¯¯¯¯¯¯|
    //              |         |
    //     0x04 - B |  pixel  | R - 0x01
    //              |         |
    //              |_________|
    //                   G - 0x08
    //
    // (A - there's an edge between us and a pixel at the bottom)
    // (R - there's an edge between us and a pixel to the right)
    // (G - there's an edge between us and a pixel above us)
    // (B - there's an edge between us and a pixel to the left)

    // Expecting values of 1 and 0 only!
    uint PackEdge(uvec4 edges) {
      return (edges.x << 0u) | (edges.y << 1u) | (edges.z << 2u) |
             (edges.w << 3u);
    }

    uvec4 UnpackEdge(uint value) {
      uvec4 ret;
      ret.x = (value & 0x01u) != 0u ? 1u : 0u;
      ret.y = (value & 0x02u) != 0u ? 1u : 0u;
      ret.z = (value & 0x04u) != 0u ? 1u : 0u;
      ret.w = (value & 0x08u) != 0u ? 1u : 0u;
      return ret;
    }

    vec4 PackBlurAAInfo(ivec2 pixelPos, uint shapeType) {
      uint packedEdges = uint(
          texelFetch(g_src0TextureFlt, pixelPos, 0).r * 255.5);

      float retval = float(packedEdges + (shapeType << 4u));

      return vec4(retval / 255.0);
    }

    void UnpackBlurAAInfo(float packedValue, out uint edges,
                          out uint shapeType) {
      uint packedValueInt = uint(packedValue * 255.5);
      edges = packedValueInt & 0xFu;
      shapeType = packedValueInt >> 4u;
    }

    float EdgeDetectColorCalcDiff(vec3 colorA, vec3 colorB) {
    \n#ifdef IN_BGR_MODE\n
      vec3 LumWeights = c_lumWeights.bgr;
    \n#else\n
      vec3 LumWeights = c_lumWeights.rgb;
    \n#endif\n

      return dot(abs(colorA.rgb - colorB.rgb), LumWeights);
    }

    bool EdgeDetectColor(vec3 colorA, vec3 colorB) {
    \n#ifdef EDGE_DETECT_THRESHOLD\n
      return EdgeDetectColorCalcDiff(colorA, colorB) > c_ColorThreshold;
    \n#else\n
      return EdgeDetectColorCalcDiff(colorA, colorB) > g_ColorThreshold;
    \n#endif\n
    }

    void FindLineLength(out int lineLengthLeft,
                        out int lineLengthRight,
                        ivec2 screenPos,
                        const bool horizontal,
                        const bool invertedZShape,
                        const ivec2 stepRight) {
      // TODO: there must be a cleaner and faster way to get to these -
      // a precalculated array indexing maybe?
      uint maskLeft = uint(0);
      uint bitsContinueLeft = uint(0);
      uint maskRight = uint(0);
      uint bitsContinueRight = uint(0);
      {
      // Horizontal (vertical is the same, just rotated 90º counter-clockwise)
      // Inverted Z case:              // Normal Z case:
      //   __                          // __
      //  X|                           //  X|
      // --                            //    --
      //
      // Vertical
      // Inverted Z case:              // Normal Z case:
      //  |                            //   |
      //   --                          //  --
      //   X|                          // |X
        uint maskTraceLeft = uint(0);
        uint maskTraceRight = uint(0);
        uint maskStopLeft = uint(0);
        uint maskStopRight = uint(0);
        if (horizontal) {
          if (invertedZShape) {
            maskTraceLeft = 0x08u;  // tracing bottom edge
            maskTraceRight = 0x02u; // tracing top edge
          } else {
            maskTraceLeft = 0x02u;  // tracing top edge
            maskTraceRight = 0x08u; // tracing bottom edge
          }
          maskStopLeft = 0x01u;  // stop on right edge
          maskStopRight = 0x04u; // stop on left edge
        } else {
          if (invertedZShape) {
            maskTraceLeft = 0x01u;  // tracing right edge
            maskTraceRight = 0x04u; // tracing left edge
          } else {
            maskTraceLeft = 0x04u;  // tracing left edge
            maskTraceRight = 0x01u; // tracing right edge
          }
          maskStopLeft = 0x02u;  // stop on top edge
          maskStopRight = 0x08u; // stop on bottom edge
        }

        maskLeft = maskTraceLeft | maskStopLeft;
        bitsContinueLeft = maskTraceLeft;
        maskRight = maskTraceRight | maskStopRight;
        bitsContinueRight = maskTraceRight;
      }
    ///////////////////////////////////////////////////////////////////////

    \n#ifdef SETTINGS_ALLOW_SHORT_Zs\n
      int i = 1;
    \n#else\n
      int i = 2; // starting from 2 because we already know it's at least 2
    \n#endif\n
      for (; i < c_maxLineLength; i++) {
        uint edgeLeft = uint(
            texelFetch(g_src0TextureFlt,
                       ivec2(screenPos.xy - stepRight * i), 0).r * 255.5);
        uint edgeRight = uint(
            texelFetch(g_src0TextureFlt,
                       ivec2(screenPos.xy + stepRight * (i + 1)),
                       0).r * 255.5);

        // stop on encountering 'stopping' edge (as defined by masks)
        int stopLeft = (edgeLeft & maskLeft) != bitsContinueLeft ? 1 : 0;
        int stopRight =
            (edgeRight & maskRight) != bitsContinueRight ? 1 : 0;

        if (bool(stopLeft) || bool(stopRight)) {
          lineLengthLeft = 1 + i - stopLeft;
          lineLengthRight = 1 + i - stopRight;
          return;
        }
      }
      lineLengthLeft = lineLengthRight = i;
      return;
    }

    void ProcessDetectedZ(ivec2 screenPos, bool horizontal,
                          bool invertedZShape) {
      int lineLengthLeft = 0;
      int lineLengthRight = 0;

      ivec2 stepRight = (horizontal) ? (ivec2(1, 0)) : (ivec2(0, 1));
      vec2 blendDir = (horizontal) ? (vec2(0, -1)) : (vec2(1, 0));

      FindLineLength(lineLengthLeft, lineLengthRight, screenPos,
                     horizontal, invertedZShape, stepRight);

      vec2 pixelSize = g_OneOverScreenSize;

      float leftOdd = 0.15 * float(lineLengthLeft % 2);
      float rightOdd = 0.15 * float(lineLengthRight % 2);

      int loopFrom = -int((lineLengthLeft + 1) / 2) + 1;
      int loopTo = int((lineLengthRight + 1) / 2);

      float totalLength = float(loopTo - loopFrom) + 1.0 - leftOdd -
                          rightOdd;

      for (int i = loopFrom; i <= loopTo; i++) {
        highp ivec2 pixelPos = screenPos + stepRight * i;
        vec2 pixelPosFlt = vec2(float(pixelPos.x) + 0.5,
                                float(pixelPos.y) + 0.5);

    \n#ifdef DEBUG_OUTPUT_AAINFO\n
        imageStore(g_resultEdgeTexture, pixelPos,
                   PackBlurAAInfo(pixelPos, 1u));
    \n#endif\n

        float m = (float(i) + 0.5 - leftOdd - float(loopFrom)) /
                   totalLength;
        m = saturate(m);
        float k = m - ((i > 0) ? 1.0 : 0.0);
        k = (invertedZShape) ? (k) : (-k);

        vec4 color = textureLod(g_screenTexture,
                                (pixelPosFlt + blendDir * k) * pixelSize,
                                0.0);

    \n#ifdef IN_GAMMA_CORRECT_MODE\n
        color.rgb = D3DX_FLOAT3_to_SRGB(color.rgb);
    \n#endif\n
        imageStore(g_resultRGBATextureSlot1, pixelPos, color);
      }
    }

    vec4 CalcDbgDisplayColor(const vec4 blurMap) {
      vec3 pixelC = vec3(0.0, 0.0, 0.0);
      vec3 pixelL = vec3(0.0, 0.0, 1.0);
      vec3 pixelT = vec3(1.0, 0.0, 0.0);
      vec3 pixelR = vec3(0.0, 1.0, 0.0);
      vec3 pixelB = vec3(0.8, 0.8, 0.0);

      const float centerWeight = 1.0;
      float fromBelowWeight = (1.0 / (1.0 - blurMap.x)) - 1.0;
      float fromAboveWeight = (1.0 / (1.0 - blurMap.y)) - 1.0;
      float fromRightWeight = (1.0 / (1.0 - blurMap.z)) - 1.0;
      float fromLeftWeight = (1.0 / (1.0 - blurMap.w)) - 1.0;

      float weightSum = centerWeight + dot(vec4(fromBelowWeight,
                                                fromAboveWeight,
                                                fromRightWeight,
                                                fromLeftWeight),
                                           vec4(1, 1, 1, 1));

      vec4 pixel;

      pixel.rgb = pixelC.rgb + fromAboveWeight * pixelT +
                               fromBelowWeight * pixelB +
                               fromLeftWeight * pixelL +
                               fromRightWeight * pixelR;
      pixel.rgb /= weightSum;

      pixel.a = dot(pixel.rgb, vec3(1, 1, 1)) * 100.0;

      return saturate(pixel);
    }

    \n#ifdef DETECT_EDGES1\n
    layout(location = 0) out UVEC4 outEdges;
    void DetectEdges1() {
      uvec4 outputEdges;
      ivec2 screenPosI = ivec2(gl_FragCoord.xy) * ivec2(2, 2);

      // .rgb contains colour, .a contains flag whether to output it to
      // working colour texture
      vec4 pixel00 = texelFetch(g_screenTexture, screenPosI.xy, 0);
      vec4 pixel10 =
          texelFetchOffset(g_screenTexture, screenPosI.xy, 0, ivec2(1, 0));
      vec4 pixel20 =
          texelFetchOffset(g_screenTexture, screenPosI.xy, 0, ivec2(2, 0));
      vec4 pixel01 =
          texelFetchOffset(g_screenTexture, screenPosI.xy, 0, ivec2(0, 1));
      vec4 pixel11 =
          texelFetchOffset(g_screenTexture, screenPosI.xy, 0, ivec2(1, 1));
      vec4 pixel21 =
          texelFetchOffset(g_screenTexture, screenPosI.xy, 0, ivec2(2, 1));
      vec4 pixel02 =
          texelFetchOffset(g_screenTexture, screenPosI.xy, 0, ivec2(0, 2));
      vec4 pixel12 =
          texelFetchOffset(g_screenTexture, screenPosI.xy, 0, ivec2(1, 2));

      if (g_DoCopy == 1) {
        imageStore(g_targetTextureSlot2, screenPosI.xy + ivec2(0, 0), pixel00);
        imageStore(g_targetTextureSlot2, screenPosI.xy + ivec2(1, 0), pixel10);
        imageStore(g_targetTextureSlot2, screenPosI.xy + ivec2(0, 1), pixel01);
        imageStore(g_targetTextureSlot2, screenPosI.xy + ivec2(1, 1), pixel11);
      }

      float storeFlagPixel00 = 0.0;
      float storeFlagPixel10 = 0.0;
      float storeFlagPixel20 = 0.0;
      float storeFlagPixel01 = 0.0;
      float storeFlagPixel11 = 0.0;
      float storeFlagPixel21 = 0.0;
      float storeFlagPixel02 = 0.0;
      float storeFlagPixel12 = 0.0;

      vec2 et;

    \n#ifdef EDGE_DETECT_THRESHOLD\n
      float threshold = c_ColorThreshold;
    \n#else\n
      float threshold = g_ColorThreshold;
    \n#endif\n

      {
        et.x = EdgeDetectColorCalcDiff(pixel00.rgb, pixel10.rgb);
        et.y = EdgeDetectColorCalcDiff(pixel00.rgb, pixel01.rgb);
        et = saturate(et - threshold);
        ivec2 eti = ivec2(et * 15.0 + 0.99);
        outputEdges.x = uint(eti.x | (eti.y << 4));

        storeFlagPixel00 += et.x;
        storeFlagPixel00 += et.y;
        storeFlagPixel10 += et.x;
        storeFlagPixel01 += et.y;
      }

      {
        et.x = EdgeDetectColorCalcDiff(pixel10.rgb, pixel20.rgb);
        et.y = EdgeDetectColorCalcDiff(pixel10.rgb, pixel11.rgb);
        et = saturate(et - threshold);
        ivec2 eti = ivec2(et * 15.0 + 0.99);
        outputEdges.y = uint(eti.x | (eti.y << 4));

        storeFlagPixel10 += et.x;
        storeFlagPixel10 += et.y;
        storeFlagPixel20 += et.x;
        storeFlagPixel11 += et.y;
      }

      {
        et.x = EdgeDetectColorCalcDiff(pixel01.rgb, pixel11.rgb);
        et.y = EdgeDetectColorCalcDiff(pixel01.rgb, pixel02.rgb);
        et = saturate(et - threshold);
        ivec2 eti = ivec2(et * 15.0 + 0.99);
        outputEdges.z = uint(eti.x | (eti.y << 4));

        storeFlagPixel01 += et.x;
        storeFlagPixel01 += et.y;
        storeFlagPixel11 += et.x;
        storeFlagPixel02 += et.y;
      }

      {
        et.x = EdgeDetectColorCalcDiff(pixel11.rgb, pixel21.rgb);
        et.y = EdgeDetectColorCalcDiff(pixel11.rgb, pixel12.rgb);
        et = saturate(et - threshold);
        ivec2 eti = ivec2(et * 15.0 + 0.99);
        outputEdges.w = uint(eti.x | (eti.y << 4));

        storeFlagPixel11 += et.x;
        storeFlagPixel11 += et.y;
        storeFlagPixel21 += et.x;
        storeFlagPixel12 += et.y;
      }

      gl_FragDepth = any(bvec4(outputEdges)) ? 1.0 : 0.0;

      if (gl_FragDepth != 0.0) {
        if (storeFlagPixel00 != 0.0)
          imageStore(g_resultRGBATextureSlot1, screenPosI.xy + ivec2(0, 0),
                     pixel00);
        if (storeFlagPixel10 != 0.0)
          imageStore(g_resultRGBATextureSlot1, screenPosI.xy + ivec2(1, 0),
                     pixel10);
        if (storeFlagPixel20 != 0.0)
          imageStore(g_resultRGBATextureSlot1, screenPosI.xy + ivec2(2, 0),
                     pixel20);
        if (storeFlagPixel01 != 0.0)
          imageStore(g_resultRGBATextureSlot1, screenPosI.xy + ivec2(0, 1),
                     pixel01);
        if (storeFlagPixel02 != 0.0)
          imageStore(g_resultRGBATextureSlot1, screenPosI.xy + ivec2(0, 2),
                     pixel02);
        if (storeFlagPixel11 != 0.0)
          imageStore(g_resultRGBATextureSlot1, screenPosI.xy + ivec2(1, 1),
                     pixel11);
        if (storeFlagPixel21 != 0.0)
          imageStore(g_resultRGBATextureSlot1, screenPosI.xy + ivec2(2, 1),
                     pixel21);
        if (storeFlagPixel12 != 0.0)
          imageStore(g_resultRGBATextureSlot1, screenPosI.xy + ivec2(1, 2),
                     pixel12);
      }
      outEdges = STORE_UVEC4(outputEdges);
    }
    \n#endif\n  // DETECT_EDGES1

    vec2 UnpackThresholds(uint val) {
      return vec2(val & 0x0Fu, val >> 4u) / 15.0f;
    }

    uvec4 PruneNonDominantEdges(vec4 edges[3]) {
      vec4 maxE4 = vec4(0.0, 0.0, 0.0, 0.0);

      float avg = 0.0;

      for (int i = 0; i < 3; i++) {
        maxE4 = max(maxE4, edges[i]);

        avg = dot(edges[i], vec4(1, 1, 1, 1) / (3.0 * 4.0));
      }

      vec2 maxE2 = max(maxE4.xy, maxE4.zw);
      float maxE = max(maxE2.x, maxE2.y);

      float threshold = avg * 0.65 + maxE * 0.35;

      // threshold = 0.0001; // this disables non-dominant edge pruning!

      uint cx = edges[0].x >= threshold ? 1u : 0u;
      uint cy = edges[0].y >= threshold ? 1u : 0u;
      return uvec4(cx, cy, 0, 0);
    }

    void CollectEdges(int offX,
                      int offY,
                      out vec4 edges[3],
                      const uint packedVals[6 * 6]) {
      vec2 pixelP0P0 = UnpackThresholds(packedVals[(offX)*6+(offY)]);
      vec2 pixelP1P0 = UnpackThresholds(packedVals[(offX+1)*6+(offY)]);
      vec2 pixelP0P1 = UnpackThresholds(packedVals[(offX)*6+(offY+1)]);
      vec2 pixelM1P0 = UnpackThresholds(packedVals[(offX-1)*6 +(offY)]);
      vec2 pixelP0M1 = UnpackThresholds(packedVals[(offX)*6+(offY-1)]);
      vec2 pixelP1M1 = UnpackThresholds(packedVals[(offX+1)*6 +(offY-1)]);
      vec2 pixelM1P1 = UnpackThresholds(packedVals[(offX-1)*6+(offY+1)]);

      edges[0].x = pixelP0P0.x;
      edges[0].y = pixelP0P0.y;
      edges[0].z = pixelP1P0.x;
      edges[0].w = pixelP1P0.y;
      edges[1].x = pixelP0P1.x;
      edges[1].y = pixelP0P1.y;
      edges[1].z = pixelM1P0.x;
      edges[1].w = pixelM1P0.y;
      edges[2].x = pixelP0M1.x;
      edges[2].y = pixelP0M1.y;
      edges[2].z = pixelP1M1.y;
      edges[2].w = pixelM1P1.x;
    }
  );

const char ApplyFramebufferAttachmentCMAAINTELResourceManager::cmaa_frag_s2_[] =
  SHADER(
    \n#ifdef DETECT_EDGES2\n
    layout(early_fragment_tests) in;
    void DetectEdges2() {
      ivec2 screenPosI = ivec2(gl_FragCoord.xy);
      uvec2 notTopRight =
          uvec2(notEqual((screenPosI + 1), textureSize(g_src0Texture4Uint, 0)));

      // source : edge differences from previous pass
      uint packedVals[6 * 6];

      // center pixel (our output)
      UVEC4 packedQ4 = texelFetch(g_src0Texture4Uint, screenPosI.xy, 0);
      packedVals[(2) * 6 + (2)] = LOAD_UINT(packedQ4.x);
      packedVals[(3) * 6 + (2)] = LOAD_UINT(packedQ4.y);
      packedVals[(2) * 6 + (3)] = LOAD_UINT(packedQ4.z);
      packedVals[(3) * 6 + (3)] = LOAD_UINT(packedQ4.w);

      vec4 edges[3];
      if (bool(packedVals[(2) * 6 + (2)]) ||
          bool(packedVals[(3) * 6 + (2)])) {
        UVEC4 packedQ1 = texelFetchOffset(g_src0Texture4Uint,
                                          screenPosI.xy, 0, ivec2(0, -1));
        packedVals[(2) * 6 + (0)] = LOAD_UINT(packedQ1.x);
        packedVals[(3) * 6 + (0)] = LOAD_UINT(packedQ1.y);
        packedVals[(2) * 6 + (1)] = LOAD_UINT(packedQ1.z);
        packedVals[(3) * 6 + (1)] = LOAD_UINT(packedQ1.w);
      }

      if (bool(packedVals[(2) * 6 + (2)]) ||
          bool(packedVals[(2) * 6 + (3)])) {
        UVEC4 packedQ3 = texelFetchOffset(g_src0Texture4Uint,
                                          screenPosI.xy, 0, ivec2(-1, 0));
        packedVals[(0) * 6 + (2)] = LOAD_UINT(packedQ3.x);
        packedVals[(1) * 6 + (2)] = LOAD_UINT(packedQ3.y);
        packedVals[(0) * 6 + (3)] = LOAD_UINT(packedQ3.z);
        packedVals[(1) * 6 + (3)] = LOAD_UINT(packedQ3.w);
      }

      if (bool(packedVals[(2) * 6 + (2)])) {
        CollectEdges(2, 2, edges, packedVals);
        uint pe = PackEdge(PruneNonDominantEdges(edges));
        if (pe != 0u) {
          imageStore(g_resultEdgeTexture, 2 * screenPosI.xy + ivec2(0, 0),
                     vec4(float(0x80u | pe) / 255.0, 0, 0, 0));
        }
      }

      if (bool(packedVals[(3) * 6 + (2)]) ||
          bool(packedVals[(3) * 6 + (3)])) {
        UVEC4 packedQ5 = texelFetchOffset(g_src0Texture4Uint,
                                          screenPosI.xy, 0, ivec2(1, 0));
        packedVals[(4) * 6 + (2)] = LOAD_UINT(packedQ5.x);
        packedVals[(5) * 6 + (2)] = LOAD_UINT(packedQ5.y);
        packedVals[(4) * 6 + (3)] = LOAD_UINT(packedQ5.z);
        packedVals[(5) * 6 + (3)] = LOAD_UINT(packedQ5.w);
      }

      if (bool(packedVals[(3) * 6 + (2)])) {
        UVEC4 packedQ2 = texelFetchOffset(g_src0Texture4Uint,
                                          screenPosI.xy, 0, ivec2(1, -1));
        packedVals[(4) * 6 + (0)] = LOAD_UINT(packedQ2.x);
        packedVals[(5) * 6 + (0)] = LOAD_UINT(packedQ2.y);
        packedVals[(4) * 6 + (1)] = LOAD_UINT(packedQ2.z);
        packedVals[(5) * 6 + (1)] = LOAD_UINT(packedQ2.w);

        CollectEdges(3, 2, edges, packedVals);
        uvec4 dominant_edges = PruneNonDominantEdges(edges);
        // The rightmost edge of the texture is not edge.
        // Note: texelFetch() on out of range gives an undefined value.
        uint pe = PackEdge(dominant_edges * uvec4(notTopRight.x, 1, 1, 1));
        if (pe != 0u) {
          imageStore(g_resultEdgeTexture, 2 * screenPosI.xy + ivec2(1, 0),
                     vec4(float(0x80u | pe) / 255.0, 0, 0, 0));
        }
      }

      if (bool(packedVals[(2) * 6 + (3)]) ||
          bool(packedVals[(3) * 6 + (3)])) {
        UVEC4 packedQ7 = texelFetchOffset(g_src0Texture4Uint,
                                          screenPosI.xy, 0, ivec2(0, 1));
        packedVals[(2) * 6 + (4)] = LOAD_UINT(packedQ7.x);
        packedVals[(3) * 6 + (4)] = LOAD_UINT(packedQ7.y);
        packedVals[(2) * 6 + (5)] = LOAD_UINT(packedQ7.z);
        packedVals[(3) * 6 + (5)] = LOAD_UINT(packedQ7.w);
      }

      if (bool(packedVals[(2) * 6 + (3)])) {
        UVEC4 packedQ6 = texelFetchOffset(g_src0Texture4Uint,
                                          screenPosI.xy, 0, ivec2(-1, -1));
        packedVals[(0) * 6 + (4)] = LOAD_UINT(packedQ6.x);
        packedVals[(1) * 6 + (4)] = LOAD_UINT(packedQ6.y);
        packedVals[(0) * 6 + (5)] = LOAD_UINT(packedQ6.z);
        packedVals[(1) * 6 + (5)] = LOAD_UINT(packedQ6.w);

        CollectEdges(2, 3, edges, packedVals);
        uvec4 dominant_edges = PruneNonDominantEdges(edges);
        uint pe = PackEdge(dominant_edges * uvec4(1, notTopRight.y, 1, 1));
        if (pe != 0u) {
          imageStore(g_resultEdgeTexture, 2 * screenPosI.xy + ivec2(0, 1),
                     vec4(float(0x80u | pe) / 255.0, 0, 0, 0));
        }
      }

      if (bool(packedVals[(3) * 6 + (3)])) {
        CollectEdges(3, 3, edges, packedVals);
        uvec4 dominant_edges = PruneNonDominantEdges(edges);
        uint pe = PackEdge(dominant_edges * uvec4(notTopRight, 1, 1));
        if (pe != 0u) {
          imageStore(g_resultEdgeTexture, 2 * screenPosI.xy + ivec2(1, 1),
                     vec4(float(0x80u | pe) / 255.0, 0, 0, 0));
        }
      }
    }
    \n#endif\n  // DETECT_EDGES2

    \n#ifdef COMBINE_EDGES\n
    void CombineEdges() {
      ivec3 screenPosIBase = ivec3(ivec2(gl_FragCoord.xy) * 2, 0);
      vec3 screenPosBase = vec3(screenPosIBase);
      uvec2 notBottomLeft = uvec2(notEqual(screenPosIBase.xy, ivec2(0, 0)));
      uint packedEdgesArray[3 * 3];

      // use only if it has the 'prev frame' flag:[sample * 255.0 - 127.5]
      //-> if it has the last bit flag (128), it's going to stay above 0
      uvec4 sampA = uvec4(
          textureGatherOffset(g_src0TextureFlt,
                              screenPosBase.xy * g_OneOverScreenSize,
                              ivec2(1, 0)) * 255.0 - 127.5);
      uvec4 sampB = uvec4(
          textureGatherOffset(g_src0TextureFlt,
                              screenPosBase.xy * g_OneOverScreenSize,
                              ivec2(0, 1)) * 255.0 - 127.5);
      uint sampC = uint(
          texelFetchOffset(g_src0TextureFlt, screenPosIBase.xy, 0,
                           ivec2(1, 1)).r * 255.0 - 127.5);

      packedEdgesArray[(0) * 3 + (0)] = 0u;
      // The bottom-most edge of the texture is not edge.
      // Note: texelFetch() on out of range gives an undefined value.
      packedEdgesArray[(1) * 3 + (0)] = sampA.w * notBottomLeft.y;
      packedEdgesArray[(2) * 3 + (0)] = sampA.z * notBottomLeft.y;
      packedEdgesArray[(1) * 3 + (1)] = sampA.x;
      packedEdgesArray[(2) * 3 + (1)] = sampA.y;
      // The left-most edge of the texture is not edge.
      packedEdgesArray[(0) * 3 + (1)] = sampB.w * notBottomLeft.x;
      packedEdgesArray[(0) * 3 + (2)] = sampB.x * notBottomLeft.x;
      packedEdgesArray[(1) * 3 + (2)] = sampB.y;
      packedEdgesArray[(2) * 3 + (2)] = sampC;

      uvec4 pixelsC = uvec4(packedEdgesArray[(1 + 0) * 3 + (1 + 0)],
                            packedEdgesArray[(1 + 1) * 3 + (1 + 0)],
                            packedEdgesArray[(1 + 0) * 3 + (1 + 1)],
                            packedEdgesArray[(1 + 1) * 3 + (1 + 1)]);
      uvec4 pixelsL = uvec4(packedEdgesArray[(0 + 0) * 3 + (1 + 0)],
                            packedEdgesArray[(0 + 1) * 3 + (1 + 0)],
                            packedEdgesArray[(0 + 0) * 3 + (1 + 1)],
                            packedEdgesArray[(0 + 1) * 3 + (1 + 1)]);
      uvec4 pixelsU = uvec4(packedEdgesArray[(1 + 0) * 3 + (0 + 0)],
                            packedEdgesArray[(1 + 1) * 3 + (0 + 0)],
                            packedEdgesArray[(1 + 0) * 3 + (0 + 1)],
                            packedEdgesArray[(1 + 1) * 3 + (0 + 1)]);

      uvec4 outEdge4 =
          pixelsC | ((pixelsL & 0x01u) << 2u) | ((pixelsU & 0x02u) << 2u);
      vec4 outEdge4Flt = vec4(outEdge4) / 255.0;

      imageStore(g_resultEdgeTexture, screenPosIBase.xy + ivec2(0, 0),
                 outEdge4Flt.xxxx);
      imageStore(g_resultEdgeTexture, screenPosIBase.xy + ivec2(1, 0),
                 outEdge4Flt.yyyy);
      imageStore(g_resultEdgeTexture, screenPosIBase.xy + ivec2(0, 1),
                 outEdge4Flt.zzzz);
      imageStore(g_resultEdgeTexture, screenPosIBase.xy + ivec2(1, 1),
                 outEdge4Flt.wwww);

      // uvec4 numberOfEdges4 = uvec4(bitCount(outEdge4));
      // gl_FragDepth =
      //     any(greaterThan(numberOfEdges4, uvec4(1))) ? 1.0 : 0.0;

      gl_FragDepth =
          any(greaterThan(outEdge4, uvec4(1))) ? 1.0 : 0.0;
    }
    \n#endif\n  // COMBINE_EDGES

    \n#ifdef BLUR_EDGES\n
    layout(early_fragment_tests) in;
    void BlurEdges() {
      // Each |gl_FragCoord| updates 4 texels of the original texture, which are
      // 2x|gl_FragCoord| + (-1 or 0, -1 or 0) in the unnormalized texture
      // coordinate, which is the coordinate used by texelFetch().
      // e.g. when gl_FragCoord == (3.5, 3.5), this fragment shader covers
      // (6,6) (6,7) (7,6) (7,7) texels.
      // Note: gl_FragCoord == (0.5, 0.5) (i.e. left-bottom-most fragment)
      // covers (0,0) (0,1) (1,0) (1,1) texels
      // gl_FragCoord == ((w/2)-0.5, (h/2)-0.5) (i.e. right-top-most fragment)
      // covers (w-2,h-2) (w-2,h-1) (w-1,h-2) (w-1,h-1)
      ivec3 screenPosIBase = ivec3(ivec2(gl_FragCoord.xy) * 2, 0);
      vec3 screenPosBase = vec3(screenPosIBase);

      // When gl_FragCoord == (0.5, 0.5) (i.e. left-bottom-most fragment),
      // |sampA| textureGatherOffset() looks up (-1,-1), (-1,0), (0,-1), (0,0).
      // (-1,-1), (-1,0), (0,-1) must be handled.
      // Note: textureGatherOffset() on out of range gives an undefined value.
      uvec2 notBottomLeft = uvec2(notEqual(screenPosIBase.xy, ivec2(0, 0)));
      // When gl_FragCoord == ((w/2)-0.5, (h/2)-0.5) (i.e. right-top-most
      // fragment), |sampD| looks up (w-1, h-1), (w-1, h), (w, h-1), (w, h).
      // (w-1, h), (w, h-1), (w, h) must be handled.
      uvec2 notTopRight = uvec2(
          notEqual((screenPosIBase.xy + 2), textureSize(g_src0TextureFlt, 0)));

      uint forFollowUpCount = 0u;
      ivec4 forFollowUpCoords[4];

      uint packedEdgesArray[4 * 4];

      uvec4 sampA = uvec4(
          textureGatherOffset(g_src0TextureFlt,
                              screenPosBase.xy * g_OneOverScreenSize,
                              ivec2(0, 0)) *255.5);
      uvec4 sampB = uvec4(
          textureGatherOffset(g_src0TextureFlt,
                              screenPosBase.xy * g_OneOverScreenSize,
                              ivec2(2, 0)) *255.5);
      uvec4 sampC = uvec4(
          textureGatherOffset(g_src0TextureFlt,
                              screenPosBase.xy * g_OneOverScreenSize,
                              ivec2(0, 2)) *255.5);
      uvec4 sampD = uvec4(
          textureGatherOffset(g_src0TextureFlt,
                              screenPosBase.xy * g_OneOverScreenSize,
                              ivec2(2, 2)) *255.5);

      packedEdgesArray[(0) * 4 + (0)] =
          sampA.w * notBottomLeft.x * notBottomLeft.y;
      packedEdgesArray[(1) * 4 + (0)] = sampA.z * notBottomLeft.y;
      packedEdgesArray[(0) * 4 + (1)] = sampA.x * notBottomLeft.x;
      packedEdgesArray[(1) * 4 + (1)] = sampA.y;
      packedEdgesArray[(2) * 4 + (0)] = sampB.w * notBottomLeft.y;
      packedEdgesArray[(3) * 4 + (0)] =
          sampB.z * notBottomLeft.y * notTopRight.x;
      packedEdgesArray[(2) * 4 + (1)] = sampB.x;
      packedEdgesArray[(3) * 4 + (1)] = sampB.y * notTopRight.x;
      packedEdgesArray[(0) * 4 + (2)] = sampC.w * notBottomLeft.x;
      packedEdgesArray[(1) * 4 + (2)] = sampC.z;
      packedEdgesArray[(0) * 4 + (3)] =
          sampC.x * notBottomLeft.x * notTopRight.y;
      packedEdgesArray[(1) * 4 + (3)] = sampC.y * notTopRight.y;
      packedEdgesArray[(2) * 4 + (2)] = sampD.w;
      packedEdgesArray[(3) * 4 + (2)] = sampD.z * notTopRight.x;
      packedEdgesArray[(2) * 4 + (3)] = sampD.x * notTopRight.y;
      packedEdgesArray[(3) * 4 + (3)] = sampD.y * notTopRight.x * notTopRight.y;

      for (int _i = 0; _i < 4; _i++) {
        int _x = _i % 2;
        int _y = _i / 2;

        ivec3 screenPosI = screenPosIBase + ivec3(_x, _y, 0);

        uint packedEdgesC = packedEdgesArray[(1 + _x) * 4 + (1 + _y)];

        uvec4 edges = UnpackEdge(packedEdgesC);
        uint numberOfEdges = edges.x + edges.y + edges.z + edges.w;
        if (numberOfEdges <= 1u)
          continue;

        vec4 edgesFlt = vec4(edges);
        float fromRight = edgesFlt.r;
        float fromAbove = edgesFlt.g;
        float fromLeft = edgesFlt.b;
        float fromBelow = edgesFlt.a;

        vec4 xFroms = vec4(fromBelow, fromAbove, fromRight, fromLeft);

        float blurCoeff = 0.0;

        // These are additional blurs that complement the main line-based
        // blurring; Unlike line-based, these do not necessarily preserve
        // the total amount of screen colour as they will take
        // neighbouring pixel colours and apply them to the one currently
        // processed.

        // 1.) L-like shape.
        // For this shape, the total amount of screen colour will be
        // preserved when this is a part of a (zigzag) diagonal line as the
        // corners from the other side  will do the same and take some of
        // the current pixel's colour in return.
        // However, in the case when this is an actual corner, the pixel's
        // colour will be partially overwritten by it's 2 neighbours.
        if (numberOfEdges == 2u)
        {
          // with value of 0.15, the pixel will retain approx 77% of its
          // colour and the remaining 23% will come from its 2 neighbours
          // (which are likely to be blurred too in the opposite direction)
          blurCoeff = 0.15;

          // Only do blending if it's L shape - if we're between two
          // parallel edges, don't do anything
          blurCoeff *= (1.0 - fromBelow * fromAbove) *
                       (1.0 - fromRight * fromLeft);

          if (blurCoeff == 0.0)
            continue;

          uint packedEdgesL = packedEdgesArray[(0 + _x) * 4 + (1 + _y)];
          uint packedEdgesB = packedEdgesArray[(1 + _x) * 4 + (0 + _y)];
          uint packedEdgesR = packedEdgesArray[(2 + _x) * 4 + (1 + _y)];
          uint packedEdgesT = packedEdgesArray[(1 + _x) * 4 + (2 + _y)];

          // Don't blend large L shape because it would be the intended shape
          // with high probability. e.g. rectangle
          // large_l1 large_l2 large_l3 large_l4
          // _ _         |     |         _ _
          //   X|       X|     |X       |X
          //    |     ¯¯¯¯     ¯¯¯¯     |
          bool large_l1 = (packedEdgesC == (0x01u | 0x02u)) &&
                           bool(packedEdgesL & 0x02u) &&
                           bool(packedEdgesB & 0x01u);
          bool large_l2 = (packedEdgesC == (0x01u | 0x08u)) &&
                           bool(packedEdgesL & 0x08u) &&
                           bool(packedEdgesT & 0x01u);
          bool large_l3 = (packedEdgesC == (0x04u | 0x08u)) &&
                           bool(packedEdgesR & 0x08u) &&
                           bool(packedEdgesT & 0x04u);
          bool large_l4 = (packedEdgesC == (0x02u | 0x04u)) &&
                           bool(packedEdgesR & 0x02u) &&
                           bool(packedEdgesB & 0x04u);
          if (large_l1 || large_l2 || large_l3 || large_l4)
            continue;

          // Don't blend isolated L shape because it's not a closed geometry.
          // isolated_l1 isolated_l2 isolated_l3 isolated_l4
          //   _                                   _
          //   X|          X|         |X          |X
          //               ¯¯         ¯¯
          bool isolated_l1 = (packedEdgesC == (0x01u | 0x02u)) &&
                           bool((packedEdgesL & 0x02u) == 0x00u) &&
                           bool((packedEdgesT & 0x04u) == 0x00u) &&
                           bool((packedEdgesR & 0x08u) == 0x00u) &&
                           bool((packedEdgesB & 0x01u) == 0x00u);
          bool isolated_l2 = (packedEdgesC == (0x01u | 0x08u)) &&
                           bool((packedEdgesL & 0x08u) == 0x00u) &&
                           bool((packedEdgesT & 0x01u) == 0x00u) &&
                           bool((packedEdgesR & 0x02u) == 0x00u) &&
                           bool((packedEdgesB & 0x04u) == 0x00u);
          bool isolated_l3 = (packedEdgesC == (0x04u | 0x08u)) &&
                           bool((packedEdgesL & 0x02u) == 0x00u) &&
                           bool((packedEdgesT & 0x04u) == 0x00u) &&
                           bool((packedEdgesR & 0x08u) == 0x00u) &&
                           bool((packedEdgesB & 0x01u) == 0x00u);
          bool isolated_l4 = (packedEdgesC == (0x02u | 0x04u)) &&
                           bool((packedEdgesL & 0x08u) == 0x00u) &&
                           bool((packedEdgesT & 0x01u) == 0x00u) &&
                           bool((packedEdgesR & 0x02u) == 0x00u) &&
                           bool((packedEdgesB & 0x04u) == 0x00u);
          if (isolated_l1 || isolated_l2 || isolated_l3 || isolated_l4)
            continue;
        }

        // 2.) U-like shape (surrounded with edges from 3 sides)
        if (numberOfEdges == 3u) {
          // with value of 0.13, the pixel will retain approx 72% of its
          // colour and the remaining 28% will be picked from its 3
          // neighbours (which are unlikely to be blurred too but could be)
          blurCoeff = 0.13;
        }

        // 3.) Completely surrounded with edges from all 4 sides
        if (numberOfEdges == 4u) {
          // with value of 0.07, the pixel will retain 78% of its colour
          // and the remaining 22% will come from its 4 neighbours (which
          // are unlikely to be blurred)
          blurCoeff = 0.07;
        }

        // |blurCoeff| must be not zero at this point.
        vec4 blurMap = xFroms * blurCoeff;

        vec4 pixelC = texelFetch(g_screenTexture, screenPosI.xy, 0);

        const float centerWeight = 1.0;
        float fromBelowWeight = blurMap.x;
        float fromAboveWeight = blurMap.y;
        float fromRightWeight = blurMap.z;
        float fromLeftWeight  = blurMap.w;

        // this would be the proper math for blending if we were handling
        // lines (Zs) and mini kernel smoothing here, but since we're doing
        // lines separately, no need to complicate, just tweak the settings
        // float fromBelowWeight = (1.0 / (1.0 - blurMap.x)) - 1.0;
        // float fromAboveWeight = (1.0 / (1.0 - blurMap.y)) - 1.0;
        // float fromRightWeight = (1.0 / (1.0 - blurMap.z)) - 1.0;
        // float fromLeftWeight  = (1.0 / (1.0 - blurMap.w)) - 1.0;

        float fourWeightSum = dot(blurMap, vec4(1, 1, 1, 1));
        float allWeightSum = centerWeight + fourWeightSum;

        vec4 color = vec4(0, 0, 0, 0);
        if (fromLeftWeight > 0.0) {
          vec4 pixelL = texelFetchOffset(g_screenTexture, screenPosI.xy, 0,
                                         ivec2(-1, 0));
          color += fromLeftWeight * pixelL;
        }
        if (fromAboveWeight > 0.0) {
          vec4 pixelT = texelFetchOffset(g_screenTexture, screenPosI.xy, 0,
                                         ivec2(0, 1));
          color += fromAboveWeight * pixelT;
        }
        if (fromRightWeight > 0.0) {
          vec4 pixelR = texelFetchOffset(g_screenTexture, screenPosI.xy, 0,
                                         ivec2(1, 0));
          color += fromRightWeight * pixelR;
        }
        if (fromBelowWeight > 0.0) {
          vec4 pixelB = texelFetchOffset(g_screenTexture, screenPosI.xy, 0,
                                         ivec2(0, -1));
          color += fromBelowWeight * pixelB;
        }

        color /= fourWeightSum + 0.0001;

        color = mix(color, pixelC, centerWeight / allWeightSum);
    \n#ifdef IN_GAMMA_CORRECT_MODE\n
        color.rgb = D3DX_FLOAT3_to_SRGB(color.rgb);
    \n#endif\n

    \n#ifdef DEBUG_OUTPUT_AAINFO\n
        imageStore(g_resultEdgeTexture, screenPosI.xy,
                   PackBlurAAInfo(screenPosI.xy, numberOfEdges));
    \n#endif\n
        imageStore(g_resultRGBATextureSlot1, screenPosI.xy, color);

        if (numberOfEdges == 2u) {
          uint packedEdgesL = packedEdgesArray[(0 + _x) * 4 + (1 + _y)];
          uint packedEdgesB = packedEdgesArray[(1 + _x) * 4 + (0 + _y)];
          uint packedEdgesR = packedEdgesArray[(2 + _x) * 4 + (1 + _y)];
          uint packedEdgesT = packedEdgesArray[(1 + _x) * 4 + (2 + _y)];

          bool isHorizontalA = ((packedEdgesC) == (0x01u | 0x02u)) &&
             ((packedEdgesR & 0x08u) == 0x08u);
          bool isHorizontalB = ((packedEdgesC) == (0x01u | 0x08u)) &&
             ((packedEdgesR & 0x02u) == 0x02u);

          bool isHCandidate = isHorizontalA || isHorizontalB;

          bool isVerticalA = ((packedEdgesC) == (0x02u | 0x04u)) &&
             ((packedEdgesT & 0x01u) == 0x01u);
          bool isVerticalB = ((packedEdgesC) == (0x01u | 0x02u)) &&
             ((packedEdgesT & 0x04u) == 0x04u);
          bool isVCandidate = isVerticalA || isVerticalB;

          bool isCandidate = isHCandidate || isVCandidate;

          if (!isCandidate)
            continue;

          bool horizontal = isHCandidate;

          // what if both are candidates? do additional pruning (still not
          // 100% but gets rid of worst case errors)
          if (isHCandidate && isVCandidate)
            horizontal =
               (isHorizontalA && ((packedEdgesL & 0x02u) == 0x02u)) ||
               (isHorizontalB && ((packedEdgesL & 0x08u) == 0x08u));

          ivec2 offsetC;
          uint packedEdgesM1P0;
          uint packedEdgesP1P0;
          if (horizontal) {
            packedEdgesM1P0 = packedEdgesL;
            packedEdgesP1P0 = packedEdgesR;
            offsetC = ivec2(2, 0);
          } else {
            packedEdgesM1P0 = packedEdgesB;
            packedEdgesP1P0 = packedEdgesT;
            offsetC = ivec2(0, 2);
          }

          uvec4 edgesM1P0 = UnpackEdge(packedEdgesM1P0);
          uvec4 edgesP1P0 = UnpackEdge(packedEdgesP1P0);
          uvec4 edgesP2P0 = UnpackEdge(uint(texelFetch(
             g_src0TextureFlt, screenPosI.xy + offsetC, 0).r * 255.5));

          uvec4 arg0;
          uvec4 arg1;
          uvec4 arg2;
          uvec4 arg3;
          bool arg4;

          if (horizontal) {
            arg0 = uvec4(edges);
            arg1 = edgesM1P0;
            arg2 = edgesP1P0;
            arg3 = edgesP2P0;
            arg4 = true;
          } else {
            // Reuse the same code for vertical (used for horizontal above)
            // but rotate input data 90º counter-clockwise. See FindLineLength()
            // e.g. arg0.r (new top) must be mapped to edges.g (old top)
            arg0 = uvec4(edges.gbar);
            arg1 = edgesM1P0.gbar;
            arg2 = edgesP1P0.gbar;
            arg3 = edgesP2P0.gbar;
            arg4 = false;
          }

          {
            ivec2 screenPos = screenPosI.xy;
            uvec4 _edges = arg0;
            uvec4 _edgesM1P0 = arg1;
            uvec4 _edgesP1P0 = arg2;
            uvec4 _edgesP2P0 = arg3;
            bool horizontal = arg4;

            // Normal Z case:
            // __
            //  X|
            //   ¯¯
            bool isInvertedZ = false;
            bool isNormalZ = false;
            {
    \n#ifndef SETTINGS_ALLOW_SHORT_Zs\n
              // (1u-_edges.a) constraint can be removed; it was added for
              // some rare cases
              uint isZShape = _edges.r * _edges.g * _edgesM1P0.g *
                 _edgesP1P0.a *_edgesP2P0.a * (1u - _edges.b) *
                  (1u - _edgesP1P0.r) * (1u - _edges.a) *
                  (1u - _edgesP1P0.g);
    \n#else\n
              uint isZShape = _edges.r * _edges.g * _edgesP1P0.a *
                  (1u - _edges.b) * (1u - _edgesP1P0.r) * (1u - _edges.a) *
                              (1u - _edgesP1P0.g);
              isZShape *= (_edgesM1P0.g + _edgesP2P0.a);
                              // and at least one of these need to be there
    \n#endif\n
              if (isZShape > 0u) {
                isNormalZ = true;
              }
            }

            // Inverted Z case:
            //   __
            //  X|
            // ¯¯
            {
    \n#ifndef SETTINGS_ALLOW_SHORT_Zs\n
              uint isZShape = _edges.r * _edges.a * _edgesM1P0.a *
                  _edgesP1P0.g * _edgesP2P0.g * (1u - _edges.b) *
                  (1u - _edgesP1P0.r) * (1u - _edges.g) *
                  (1u - _edgesP1P0.a);
    \n#else\n
              uint isZShape = _edges.r * _edges.a * _edgesP1P0.g *
                  (1u - _edges.b) * (1u - _edgesP1P0.r) * (1u - _edges.g) *
                  (1u - _edgesP1P0.a);
              isZShape *=
                  (_edgesM1P0.a + _edgesP2P0.g);
                              // and at least one of these need to be there
    \n#endif\n

              if (isZShape > 0u) {
                isInvertedZ = true;
              }
            }

            bool isZ = isInvertedZ || isNormalZ;
            if (isZ) {
              forFollowUpCoords[forFollowUpCount++] =
                  ivec4(screenPosI.xy, horizontal, isInvertedZ);
            }
          }
        }
      }

      // This code below is the only potential bug with this algorithm :
      // it HAS to be executed after the simple shapes above. It used to be
      // executed as separate compute shader (by storing the packed
      // 'forFollowUpCoords' in an append buffer and  consuming it later)
      // but the whole thing (append/consume buffers, using CS) appears to
      // be too inefficient on most hardware.
      // However, it seems to execute fairly efficiently here and without
      // any issues, although there is no 100% guarantee that this code
      // below will execute across all pixels (it has a c_maxLineLength
      // wide kernel) after other shaders processing same pixels have done
      // solving simple shapes. It appears to work regardless, across all
      // hardware; pixels with 1-edge or two opposing edges are ignored by
      // simple  shapes anyway and other shapes stop the long line
      // algorithm from executing the only danger appears to be simple
      // shape L's colliding with Z shapes from neighbouring pixels but I
      // couldn't reproduce any problems on any hardware.
      for (uint _i = 0u; _i < forFollowUpCount; _i++) {
        ivec4 data = forFollowUpCoords[_i];
        ProcessDetectedZ(data.xy, bool(data.z), bool(data.w));
      }
    }
    \n#endif\n  // BLUR_EDGES

    \n#ifdef DISPLAY_EDGES\n
    layout(location = 0) out vec4 color;
    layout(location = 1) out vec4 hasEdges;
    void DisplayEdges() {
      ivec2 screenPosI = ivec2(gl_FragCoord.xy);

      uint packedEdges = uint(0);
      uint shapeType = uint(0);
      UnpackBlurAAInfo(texelFetch(g_src0TextureFlt, screenPosI, 0).r,
                       packedEdges, shapeType);

      vec4 edges = vec4(UnpackEdge(packedEdges));
      if (any(greaterThan(edges.xyzw, vec4(0)))) {
    \n#ifdef IN_BGR_MODE\n
        color = c_edgeDebugColours[shapeType].bgra;
    \n#else\n
        color = c_edgeDebugColours[shapeType];
    \n#endif\n
        hasEdges = vec4(1.0);
      } else {
        color = vec4(0);
        hasEdges = vec4(0.0);
      }
    }
    \n#endif\n  // DISPLAY_EDGES

    void main() {
    \n#ifdef DETECT_EDGES1\n
      DetectEdges1();
    \n#endif\n
    \n#if defined DETECT_EDGES2\n
      DetectEdges2();
    \n#endif\n
    \n#if defined COMBINE_EDGES\n
      CombineEdges();
    \n#endif\n
    \n#if defined BLUR_EDGES\n
      BlurEdges();
    \n#endif\n
    \n#if defined DISPLAY_EDGES\n
      DisplayEdges();
    \n#endif\n
    }
  );
/* clang-format on */

}  // namespace gles2
}  // namespace gpu
