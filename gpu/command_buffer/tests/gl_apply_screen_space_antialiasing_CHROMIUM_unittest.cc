// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extchromium.h>
#include <GLES3/gl3.h>
#include <stddef.h>
#include <stdint.h>

#include "gpu/command_buffer/tests/gl_manager.h"
#include "gpu/command_buffer/tests/gl_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {

// A collection of tests that exercise the GL_CHROMIUM_copy_texture extension.
class GLApplyScreenSpaceAntialiasingCHROMIUMTest : public testing::Test {
 protected:
  void CreateAndBindDestinationTextureAndFBO(GLenum target) {
    glGenTextures(1, &textures_);
    glBindTexture(target, textures_);

    // Some drivers (NVidia/SGX) require texture settings to be a certain way or
    // they won't report FRAMEBUFFER_COMPLETE.
    glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &framebuffer_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target,
                           textures_, 0);
  }

  void CheckStatus() {
    available_ =
        GLTestHelper::HasExtension("GL_CHROMIUM_screen_space_antialiasing");
    if (!available_) {
      LOG(INFO) << "GL_CHROMIUM_screen_space_antialiasing not supported. "
                   "Skipping test...";
      return;
    }

    CreateAndBindDestinationTextureAndFBO(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    EXPECT_EQ(static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE),
              glCheckFramebufferStatus(GL_FRAMEBUFFER));

    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glApplyScreenSpaceAntialiasingCHROMIUM();
    if (glGetError() == GL_NO_ERROR)
      return;

    // For example, linux NVidia fails with this log.
    // ApplyFramebufferAttachmentCMAAINTEL: shader compilation failed:
    // GL_FRAGMENT_SHADER shader compilation failed: 0(98) : error C3013:
    // input/output layout qualifiers supported above GL version 130
    available_ = false;
    LOG(ERROR) << "GL_CHROMIUM_screen_space_antialiasing maybe not supported "
                  "in non-Intel GPU.";
    DeleteResources();
  }

  void SetUp() override {
    GLManager::Options options;
    gl_.Initialize(options);
    CheckStatus();
  }

  void TearDown() override {
    if (available_)
      DeleteResources();
    gl_.Destroy();
  }

  void DeleteResources() {
    glDeleteTextures(1, &textures_);
    glDeleteFramebuffers(1, &framebuffer_id_);
  }

  GLManager gl_;
  GLuint textures_ = 0;
  GLuint framebuffer_id_ = 0;
  bool available_ = false;
};

class GLApplyScreenSpaceAntialiasingCHROMIUMES3Test
    : public GLApplyScreenSpaceAntialiasingCHROMIUMTest {
 protected:
  void SetUp() override {
    GLManager::Options options;
    options.context_type = CONTEXT_TYPE_OPENGLES3;
    gl_.Initialize(options);
    CheckStatus();
  }
  void CheckStatus() {
    // Not applicable for devices not supporting OpenGLES3.
    if (!gl_.IsInitialized()) {
      LOG(INFO) << "CONTEXT_TYPE_OPENGLES3 not supported. "
                   "Skipping test...";
      return;
    }
    GLApplyScreenSpaceAntialiasingCHROMIUMTest::CheckStatus();
  }
};

// TODO(dongseong.hwang): This test fails on the Nexus 9 GPU fyi bot.
// crbug.com/659638
#if defined(OS_ANDROID)
#define MAYBE_Basic DISABLED_Basic
#else
#define MAYBE_Basic Basic
#endif
// Test to ensure that the basic functionality of the extension works.
TEST_F(GLApplyScreenSpaceAntialiasingCHROMIUMTest, MAYBE_Basic) {
  if (!available_)
    return;

  glApplyScreenSpaceAntialiasingCHROMIUM();
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // Check the FB is still bound.
  GLint value = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &value);
  GLuint fb_id = value;
  EXPECT_EQ(framebuffer_id_, fb_id);

  // Check that FB is complete.
  EXPECT_EQ(static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE),
            glCheckFramebufferStatus(GL_FRAMEBUFFER));

  uint8_t pixels[1 * 4] = {0u, 255u, 0u, 255u};
  GLTestHelper::CheckPixels(0, 0, 1, 1, 0, pixels, nullptr);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
}

TEST_F(GLApplyScreenSpaceAntialiasingCHROMIUMTest, DefaultFBO) {
  if (!available_)
    return;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glApplyScreenSpaceAntialiasingCHROMIUM();
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());
}

TEST_F(GLApplyScreenSpaceAntialiasingCHROMIUMTest, InternalFormat) {
  if (!available_)
    return;

  GLint formats[] = {GL_RGB, GL_RGBA, GL_ALPHA, GL_LUMINANCE,
                     GL_LUMINANCE_ALPHA};
  for (size_t index = 0; index < arraysize(formats); index++) {
    glTexImage2D(GL_TEXTURE_2D, 0, formats[index], 1, 1, 0, formats[index],
                 GL_UNSIGNED_BYTE, nullptr);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           textures_, 0);

    // Only if the format is supported by FBO, supported by this extension.
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      continue;

    glClear(GL_COLOR_BUFFER_BIT);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

    glApplyScreenSpaceAntialiasingCHROMIUM();
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError()) << "index:"
                                                              << index;

    uint8_t pixels[1 * 4] = {0u, 255u, 0u, 255u};
    GLTestHelper::CheckPixels(0, 0, 1, 1, 0, pixels, nullptr);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  }
}

struct FormatType {
  GLenum internal_format;
  GLenum format;
  GLenum type;
};

TEST_F(GLApplyScreenSpaceAntialiasingCHROMIUMES3Test, InternalFormat) {
  if (!available_)
    return;

  FormatType format_type = {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};

  uint8_t pixels[1 * 4] = {0u, 255u, 0u, 255u};
  glTexImage2D(GL_TEXTURE_2D, 0, format_type.internal_format, 1, 1, 0,
               format_type.format, format_type.type, pixels);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         textures_, 0);

  // Only if the format is supported by FBO, supported by this extension.
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    return;

  glApplyScreenSpaceAntialiasingCHROMIUM();
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError())
      << "internal_format: "
      << gles2::GLES2Util::GetStringEnum(format_type.internal_format);

  GLTestHelper::CheckPixels(0, 0, 1, 1, 0, pixels, nullptr);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
}

TEST_F(GLApplyScreenSpaceAntialiasingCHROMIUMES3Test,
       InternalFormatNotSupported) {
  if (!available_)
    return;

  FormatType format_type = {GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT};

  uint32_t pixels[1 * 4] = {0u, 255u, 0u, 255u};
  glTexImage2D(GL_TEXTURE_2D, 0, format_type.internal_format, 1, 1, 0,
               format_type.format, format_type.type, pixels);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         textures_, 0);

  // Only if the format is supported by FBO, supported by this extension.
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    return;

  glApplyScreenSpaceAntialiasingCHROMIUM();
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError())
      << "internal_format: "
      << gles2::GLES2Util::GetStringEnum(format_type.internal_format);
}

TEST_F(GLApplyScreenSpaceAntialiasingCHROMIUMTest, ImmutableTexture) {
  if (!available_)
    return;

  if (!GLTestHelper::HasExtension("GL_EXT_texture_storage")) {
    LOG(INFO) << "GL_EXT_texture_storage not supported. Skipping test...";
    return;
  }
  GLenum internal_formats[] = {GL_RGB8_OES, GL_RGBA8_OES, GL_BGRA8_EXT};
  for (auto internal_format : internal_formats) {
    glDeleteTextures(1, &textures_);
    glDeleteFramebuffers(1, &framebuffer_id_);
    CreateAndBindDestinationTextureAndFBO(GL_TEXTURE_2D);
    glTexStorage2DEXT(GL_TEXTURE_2D, 1, internal_format, 1, 1);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_EQ(static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE),
              glCheckFramebufferStatus(GL_FRAMEBUFFER));

    glClear(GL_COLOR_BUFFER_BIT);

    glApplyScreenSpaceAntialiasingCHROMIUM();
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

    uint8_t pixels[1 * 4] = {0u, 255u, 0u, 255u};
    GLTestHelper::CheckPixels(0, 0, 1, 1, 0, pixels, nullptr);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  }
}

// Similar to webgl conformance test 'testAntialias(true)' in
// context-attributes-alpha-depth-stencil-antialias.html
TEST_F(GLApplyScreenSpaceAntialiasingCHROMIUMTest, AntiAliasing) {
  if (!available_)
    return;

  // Fill colors in the FBO as follows
  //  +-----+
  //  |R|R| |
  //  +-----+
  //  |R| | |
  //  +-----+
  //  | | | |
  //  +-+-+-+
  const int length = 3;
  uint8_t rgba_pixels[4 * length * length] = {
      255u, 0u, 0u, 255u, 255u, 0u, 0u, 255u, 0u, 0u, 0u, 0u,
      255u, 0u, 0u, 255u, 0u,   0u, 0u, 0u,   0u, 0u, 0u, 0u,
      0u,   0u, 0u, 0u,   0u,   0u, 0u, 0u,   0u, 0u, 0u, 0u,
  };
  glBindTexture(GL_TEXTURE_2D, textures_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, length, length, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, rgba_pixels);

  uint8_t transparent[4] = {0u, 0u, 0u, 0u};
  uint8_t red[4] = {255u, 0u, 0u, 255u};
  GLTestHelper::CheckPixels(0, 0, 1, 1, 0, red, nullptr);
  GLTestHelper::CheckPixels(0, 1, 1, 1, 0, red, nullptr);
  GLTestHelper::CheckPixels(0, 2, 1, 1, 0, transparent, nullptr);
  GLTestHelper::CheckPixels(1, 0, 1, 1, 0, red, nullptr);
  GLTestHelper::CheckPixels(1, 1, 1, 1, 0, transparent, nullptr);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  glApplyScreenSpaceAntialiasingCHROMIUM();
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  GLTestHelper::CheckPixels(0, 0, 1, 1, 0, red, nullptr);
  GLTestHelper::CheckPixels(2, 2, 1, 1, 0, transparent, nullptr);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // Check if middle pixel is anti-aliased.
  uint8_t pixels[4] = {0u};
  glReadPixels(1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixels);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  EXPECT_NE(transparent, pixels);
  EXPECT_NE(red, pixels);
  EXPECT_TRUE(pixels[0] > transparent[0] && pixels[0] < red[0]);
  EXPECT_EQ(0u, pixels[1]);
  EXPECT_EQ(0u, pixels[2]);
  EXPECT_TRUE(pixels[3] > transparent[3] && pixels[3] < red[3]);
}

namespace {

void glEnableDisable(GLint param, GLboolean value) {
  if (value)
    glEnable(param);
  else
    glDisable(param);
}

}  // unnamed namespace

// Validate that some basic GL state is not touched upon execution of
// the extension.
TEST_F(GLApplyScreenSpaceAntialiasingCHROMIUMTest, BasicStatePreservation) {
  if (!available_)
    return;

  GLboolean reference_settings[2] = {GL_TRUE, GL_FALSE};
  for (int x = 0; x < 2; ++x) {
    GLboolean setting = reference_settings[x];
    glEnableDisable(GL_DEPTH_TEST, setting);
    glEnableDisable(GL_SCISSOR_TEST, setting);
    glEnableDisable(GL_STENCIL_TEST, setting);
    glEnableDisable(GL_CULL_FACE, setting);
    glEnableDisable(GL_BLEND, setting);
    glColorMask(setting, setting, setting, setting);
    glDepthMask(setting);

    glActiveTexture(GL_TEXTURE1 + x);

    glApplyScreenSpaceAntialiasingCHROMIUM();
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

    EXPECT_EQ(setting, glIsEnabled(GL_DEPTH_TEST));
    EXPECT_EQ(setting, glIsEnabled(GL_SCISSOR_TEST));
    EXPECT_EQ(setting, glIsEnabled(GL_STENCIL_TEST));
    EXPECT_EQ(setting, glIsEnabled(GL_CULL_FACE));
    EXPECT_EQ(setting, glIsEnabled(GL_BLEND));

    GLboolean bool_array[4] = {GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};
    glGetBooleanv(GL_DEPTH_WRITEMASK, bool_array);
    EXPECT_EQ(setting, bool_array[0]);

    bool_array[0] = GL_FALSE;
    glGetBooleanv(GL_COLOR_WRITEMASK, bool_array);
    EXPECT_EQ(setting, bool_array[0]);
    EXPECT_EQ(setting, bool_array[1]);
    EXPECT_EQ(setting, bool_array[2]);
    EXPECT_EQ(setting, bool_array[3]);

    GLint active_texture = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
    EXPECT_EQ(GL_TEXTURE1 + x, active_texture);
  }

  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
};

// Verify that invocation of the extension does not modify the bound
// texture state.
TEST_F(GLApplyScreenSpaceAntialiasingCHROMIUMTest, TextureStatePreserved) {
  if (!available_)
    return;

  GLuint texture_ids[2];
  glGenTextures(2, texture_ids);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_ids[0]);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, texture_ids[1]);

  glApplyScreenSpaceAntialiasingCHROMIUM();
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  GLint active_texture = 0;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
  EXPECT_EQ(GL_TEXTURE1, active_texture);

  GLint bound_texture = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound_texture);
  EXPECT_EQ(texture_ids[1], static_cast<GLuint>(bound_texture));
  glBindTexture(GL_TEXTURE_2D, 0);

  bound_texture = 0;
  glActiveTexture(GL_TEXTURE0);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound_texture);
  EXPECT_EQ(texture_ids[0], static_cast<GLuint>(bound_texture));
  glBindTexture(GL_TEXTURE_2D, 0);

  glDeleteTextures(2, texture_ids);

  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
}

TEST_F(GLApplyScreenSpaceAntialiasingCHROMIUMTest, ProgramStatePreservation) {
  if (!available_)
    return;

  // unbind the one created in setup.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  GLManager gl2;
  GLManager::Options options;
  options.size = gfx::Size(16, 16);
  options.share_group_manager = &gl_;
  gl2.Initialize(options);
  gl_.MakeCurrent();

  static const char* v_shader_str =
      "attribute vec4 g_Position;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = g_Position;\n"
      "}\n";
  static const char* f_shader_str =
      "precision mediump float;\n"
      "void main()\n"
      "{\n"
      "  gl_FragColor = vec4(0,1,0,1);\n"
      "}\n";

  GLuint program = GLTestHelper::LoadProgram(v_shader_str, f_shader_str);
  glUseProgram(program);
  GLuint position_loc = glGetAttribLocation(program, "g_Position");
  glFlush();

  // Delete program from other context.
  gl2.MakeCurrent();
  glDeleteProgram(program);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  glFlush();

  // Program should still be usable on this context.
  gl_.MakeCurrent();

  GLTestHelper::SetupUnitQuad(position_loc);

  // test using program before
  uint8_t expected[] = {
      0, 255, 0, 255,
  };
  uint8_t zero[] = {
      0, 0, 0, 0,
  };
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  EXPECT_TRUE(GLTestHelper::CheckPixels(0, 0, 1, 1, 0, zero, nullptr));
  glDrawArrays(GL_TRIANGLES, 0, 6);
  EXPECT_TRUE(GLTestHelper::CheckPixels(0, 0, 1, 1, 0, expected, nullptr));

  // Call copyTextureCHROMIUM
  uint8_t pixels[1 * 4] = {255u, 0u, 0u, 255u};
  glBindTexture(GL_TEXTURE_2D, textures_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               pixels);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_);
  glApplyScreenSpaceAntialiasingCHROMIUM();
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // test using program after
  glClear(GL_COLOR_BUFFER_BIT);
  EXPECT_TRUE(GLTestHelper::CheckPixels(0, 0, 1, 1, 0, zero, nullptr));
  glDrawArrays(GL_TRIANGLES, 0, 6);
  EXPECT_TRUE(GLTestHelper::CheckPixels(0, 0, 1, 1, 0, expected, nullptr));

  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  gl2.MakeCurrent();
  gl2.Destroy();
  gl_.MakeCurrent();
}

}  // namespace gpu
