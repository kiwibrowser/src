// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extchromium.h>
#include <stdint.h>

#include <memory>

#include "gpu/command_buffer/tests/gl_manager.h"
#include "gpu/command_buffer/tests/gl_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#define SHADER(src) #src

namespace gpu {

namespace {

const uint8_t kCompressedImageColor[4] = {255u, 0u, 0u, 255u};

// Single compressed ATC block of source pixels all set to:
// kCompressedImageColor.
const uint8_t kCompressedImageATC[8] = {0x0,  0x7c, 0x0,  0xf8,
                                        0x55, 0x55, 0x55, 0x55};

// Single compressed ATCIA block of source pixels all set to:
// kCompressedImageColor.
const uint8_t kCompressedImageATCIA[16] = {0xff, 0xff, 0x0,  0x0,  0x0, 0x0,
                                           0x0,  0x0,  0x0,  0x7c, 0x0, 0xf8,
                                           0x55, 0x55, 0x55, 0x55};

// Single compressed DXT1 block of source pixels all set to:
// kCompressedImageColor.
const uint8_t kCompressedImageDXT1[8] = {0x00, 0xf8, 0x00, 0xf8,
                                         0xaa, 0xaa, 0xaa, 0xaa};

// Single compressed DXT5 block of source pixels all set to:
// kCompressedImageColor.
const uint8_t kCompressedImageDXT5[16] = {0xff, 0xff, 0x0,  0x0,  0x0, 0x0,
                                          0x0,  0x0,  0x0,  0xf8, 0x0, 0xf8,
                                          0xaa, 0xaa, 0xaa, 0xaa};

// Single compressed DXT1 block of source pixels all set to:
// kCompressedImageColor.
const uint8_t kCompressedImageETC1[8] = {0x0,  0x0,  0xf8, 0x2,
                                         0xff, 0xff, 0x0,  0x0};

void glEnableDisable(GLint param, GLboolean value) {
  if (value)
    glEnable(param);
  else
    glDisable(param);
}

}  // unnamed namespace

// A collection of tests that exercise the GL_CHROMIUM_copy_texture extension.
class GLCompressedCopyTextureCHROMIUMTest
    : public testing::Test {
 protected:
  void SetUp() override {
    gl_.Initialize(GLManager::Options());

    glGenTextures(2, textures_);
  }

  void TearDown() override {
    glDeleteTextures(2, textures_);
    gl_.Destroy();
  }

  GLuint LoadProgram() {
    const char* v_shader_src = SHADER(
        attribute vec2 a_position;
        varying vec2 v_texcoord;
        void main() {
          gl_Position = vec4(a_position, 0.0, 1.0);
          v_texcoord = (a_position + 1.0) * 0.5;
        }
    );
    const char* f_shader_src = SHADER(
        precision mediump float;
        uniform sampler2D u_texture;
        varying vec2 v_texcoord;
        void main() {
          gl_FragColor = texture2D(u_texture, v_texcoord);
        }
    );
    return GLTestHelper::LoadProgram(v_shader_src, f_shader_src);
  }

  GLManager gl_;
  GLuint textures_[2];
  GLuint framebuffer_id_;
};

// Test to ensure that the basic functionality of the extension works.
TEST_F(GLCompressedCopyTextureCHROMIUMTest, Basic) {
  if (!GLTestHelper::HasExtension("GL_EXT_texture_compression_dxt1")) {
    LOG(INFO) <<
        "GL_EXT_texture_compression_dxt1 not supported. Skipping test...";
    return;
  }

  glBindTexture(GL_TEXTURE_2D, textures_[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
                         4, 4, 0,
                         sizeof(kCompressedImageDXT1), kCompressedImageDXT1);
  EXPECT_TRUE(glGetError() == GL_NO_ERROR);

  glBindTexture(GL_TEXTURE_2D, textures_[1]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glCompressedCopyTextureCHROMIUM(textures_[0], textures_[1]);
  EXPECT_TRUE(glGetError() == GL_NO_ERROR);

  // Load shader program.
  GLuint program = LoadProgram();
  ASSERT_NE(program, 0u);
  GLint position_loc = glGetAttribLocation(program, "a_position");
  GLint texture_loc = glGetUniformLocation(program, "u_texture");
  ASSERT_NE(position_loc, -1);
  ASSERT_NE(texture_loc, -1);
  glUseProgram(program);

  // Load geometry.
  GLuint vbo = GLTestHelper::SetupUnitQuad(position_loc);
  ASSERT_NE(vbo, 0u);

  // Load texture.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textures_[1]);
  glUniform1i(texture_loc, 0);

  // Draw.
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glFlush();

  GLTestHelper::CheckPixels(0, 0, 4, 4, 0, kCompressedImageColor, nullptr);
  EXPECT_TRUE(GL_NO_ERROR == glGetError());
}

TEST_F(GLCompressedCopyTextureCHROMIUMTest, InternalFormat) {
  struct Image {
    const GLint format;
    const uint8_t* data;
    const GLsizei data_size;

    Image(const GLint format, const uint8_t* data, const GLsizei data_size)
        : format(format), data(data), data_size(data_size) {}
  };
  std::vector<std::unique_ptr<Image>> supported_formats;

  if (GLTestHelper::HasExtension("GL_AMD_compressed_ATC_texture") ||
      GLTestHelper::HasExtension("GL_ATI_texture_compression_atitc")) {
    supported_formats.push_back(std::make_unique<Image>(
        GL_ATC_RGB_AMD, kCompressedImageATC, sizeof(kCompressedImageATC)));
    supported_formats.push_back(std::make_unique<Image>(
        GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD, kCompressedImageATCIA,
        sizeof(kCompressedImageATCIA)));
  }
  if (GLTestHelper::HasExtension("GL_EXT_texture_compression_dxt1")) {
    supported_formats.push_back(std::make_unique<Image>(
        GL_COMPRESSED_RGB_S3TC_DXT1_EXT, kCompressedImageDXT1,
        sizeof(kCompressedImageDXT1)));
  }
  if (GLTestHelper::HasExtension("GL_ANGLE_texture_compression_dxt5") ||
      GLTestHelper::HasExtension("GL_EXT_texture_compression_s3tc")) {
    supported_formats.push_back(std::make_unique<Image>(
        GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, kCompressedImageDXT5,
        sizeof(kCompressedImageDXT5)));
  }
  if (GLTestHelper::HasExtension("GL_OES_compressed_ETC1_RGB8_texture")) {
    supported_formats.push_back(std::make_unique<Image>(
        GL_ETC1_RGB8_OES, kCompressedImageETC1, sizeof(kCompressedImageETC1)));
  }

  for (const auto& image : supported_formats) {
    glBindTexture(GL_TEXTURE_2D, textures_[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, image->format,
                           4, 4, 0, image->data_size, image->data);
    EXPECT_TRUE(GL_NO_ERROR == glGetError());

    glBindTexture(GL_TEXTURE_2D, textures_[1]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glCompressedCopyTextureCHROMIUM(textures_[0], textures_[1]);
    EXPECT_TRUE(GL_NO_ERROR == glGetError());
  }
}

TEST_F(GLCompressedCopyTextureCHROMIUMTest, InternalFormatNotSupported) {
  if (!GLTestHelper::HasExtension("GL_EXT_texture_compression_dxt1")) {
    LOG(INFO) <<
        "GL_EXT_texture_compression_dxt1 not supported. Skipping test...";
    return;
  }

  const uint8_t kUncompressedPixels[1 * 4] = {255u, 0u, 0u, 255u};

  glBindTexture(GL_TEXTURE_2D, textures_[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               kUncompressedPixels);
  EXPECT_TRUE(glGetError() == GL_NO_ERROR);

  glBindTexture(GL_TEXTURE_2D, textures_[1]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // Check that the GL_RGBA format reports an error.
  glCompressedCopyTextureCHROMIUM(textures_[0], textures_[1]);
  EXPECT_TRUE(GL_INVALID_OPERATION == glGetError());
}

TEST_F(GLCompressedCopyTextureCHROMIUMTest, InvalidTextureIds) {
  if (!GLTestHelper::HasExtension("GL_EXT_texture_compression_dxt1")) {
    LOG(INFO)
        << "GL_EXT_texture_compression_dxt1 not supported. Skipping test...";
    return;
  }

  glBindTexture(GL_TEXTURE_2D, textures_[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 4,
                         4, 0, sizeof(kCompressedImageDXT1),
                         kCompressedImageDXT1);
  EXPECT_TRUE(glGetError() == GL_NO_ERROR);

  glBindTexture(GL_TEXTURE_2D, textures_[1]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glCompressedCopyTextureCHROMIUM(textures_[0], 99993);
  EXPECT_TRUE(glGetError() == GL_INVALID_VALUE);

  glCompressedCopyTextureCHROMIUM(99994, textures_[1]);
  EXPECT_TRUE(glGetError() == GL_INVALID_VALUE);

  glCompressedCopyTextureCHROMIUM(99995, 99996);
  EXPECT_TRUE(glGetError() == GL_INVALID_VALUE);

  glCompressedCopyTextureCHROMIUM(textures_[0], textures_[1]);
  EXPECT_TRUE(glGetError() == GL_NO_ERROR);
}

TEST_F(GLCompressedCopyTextureCHROMIUMTest, InvalidTextureTarget) {
  glBindTexture(GL_TEXTURE_CUBE_MAP, textures_[0]);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textures_[1]);
  glCompressedCopyTextureCHROMIUM(textures_[0], textures_[1]);
  EXPECT_TRUE(glGetError() == GL_INVALID_VALUE);
}

// Validate that some basic GL state is not touched upon execution of
// the extension.
TEST_F(GLCompressedCopyTextureCHROMIUMTest, BasicStatePreservation) {
  if (!GLTestHelper::HasExtension("GL_EXT_texture_compression_dxt1")) {
    LOG(INFO) <<
        "GL_EXT_texture_compression_dxt1 not supported. Skipping test...";
    return;
  }

  glBindTexture(GL_TEXTURE_2D, textures_[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
                         4, 4, 0,
                         sizeof(kCompressedImageDXT1), kCompressedImageDXT1);
  EXPECT_TRUE(glGetError() == GL_NO_ERROR);

  glBindTexture(GL_TEXTURE_2D, textures_[1]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  GLboolean reference_settings[2] = { GL_TRUE, GL_FALSE };
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
    glCompressedCopyTextureCHROMIUM(textures_[0], textures_[1]);
    EXPECT_TRUE(glGetError() == GL_NO_ERROR);

    EXPECT_EQ(setting, glIsEnabled(GL_DEPTH_TEST));
    EXPECT_EQ(setting, glIsEnabled(GL_SCISSOR_TEST));
    EXPECT_EQ(setting, glIsEnabled(GL_STENCIL_TEST));
    EXPECT_EQ(setting, glIsEnabled(GL_CULL_FACE));
    EXPECT_EQ(setting, glIsEnabled(GL_BLEND));

    GLboolean bool_array[4] = { GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE };
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

  EXPECT_TRUE(GL_NO_ERROR == glGetError());
};

// Verify that invocation of the extension does not modify the bound
// texture state.
TEST_F(GLCompressedCopyTextureCHROMIUMTest, TextureStatePreserved) {
  if (!GLTestHelper::HasExtension("GL_EXT_texture_compression_dxt1")) {
    LOG(INFO) <<
        "GL_EXT_texture_compression_dxt1 not supported. Skipping test...";
    return;
  }

  glBindTexture(GL_TEXTURE_2D, textures_[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
                         4, 4, 0,
                         sizeof(kCompressedImageDXT1), kCompressedImageDXT1);
  EXPECT_TRUE(glGetError() == GL_NO_ERROR);

  glBindTexture(GL_TEXTURE_2D, textures_[1]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  GLuint texture_ids[2];
  glGenTextures(2, texture_ids);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_ids[0]);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, texture_ids[1]);

  glCompressedCopyTextureCHROMIUM(textures_[0], textures_[1]);
  EXPECT_TRUE(GL_NO_ERROR == glGetError());

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

  EXPECT_TRUE(GL_NO_ERROR == glGetError());
}

}  // namespace gpu
