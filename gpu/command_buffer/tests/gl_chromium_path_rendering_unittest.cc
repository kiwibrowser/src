// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extchromium.h>
#include <stddef.h>
#include <stdint.h>
#include <cmath>

#include "base/command_line.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "gpu/command_buffer/tests/gl_manager.h"
#include "gpu/command_buffer/tests/gl_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#define SHADER(Src) #Src

namespace {
void ExpectEqualMatrix(const GLfloat* expected, const GLfloat* actual) {
  for (size_t i = 0; i < 16; ++i) {
    EXPECT_EQ(expected[i], actual[i]);
  }
}
void ExpectEqualMatrix(const GLfloat* expected, const GLint* actual) {
  for (size_t i = 0; i < 16; ++i) {
    EXPECT_EQ(static_cast<GLint>(roundf(expected[i])), actual[i]);
  }
}
}
namespace gpu {

class CHROMIUMPathRenderingTest : public testing::Test {
 protected:
  static const GLsizei kResolution = 300;

  void SetUp() override {
    GLManager::Options options;
    InitializeContextFeatures(&options);
    gl_.Initialize(options);
  }

  virtual void InitializeContextFeatures(GLManager::Options* options) {
    options->size = gfx::Size(kResolution, kResolution);
  }

  void TearDown() override { gl_.Destroy(); }

  bool IsApplicable() const {
    return GLTestHelper::HasExtension("GL_CHROMIUM_path_rendering");
  }

  void TryAllDrawFunctions(GLuint path, GLenum expected_error) {
    glStencilFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM, 0x7F);
    EXPECT_EQ(expected_error, glGetError());

    glStencilFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM, 0x7F);
    EXPECT_EQ(expected_error, glGetError());

    glStencilStrokePathCHROMIUM(path, 0x80, 0x80);
    EXPECT_EQ(expected_error, glGetError());

    glCoverFillPathCHROMIUM(path, GL_BOUNDING_BOX_CHROMIUM);
    EXPECT_EQ(expected_error, glGetError());

    glCoverStrokePathCHROMIUM(path, GL_BOUNDING_BOX_CHROMIUM);
    EXPECT_EQ(expected_error, glGetError());

    glStencilThenCoverStrokePathCHROMIUM(path, 0x80, 0x80,
                                         GL_BOUNDING_BOX_CHROMIUM);
    EXPECT_EQ(expected_error, glGetError());

    glStencilThenCoverFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM, 0x7F,
                                       GL_BOUNDING_BOX_CHROMIUM);
    EXPECT_EQ(expected_error, glGetError());
  }

  GLManager gl_;
};

class CHROMIUMPathRenderingDrawTest : public CHROMIUMPathRenderingTest {
 protected:
  void SetupStateForTestPattern() {
    glViewport(0, 0, kResolution, kResolution);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glStencilMask(0xffffffff);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    static const char* kVertexShaderSource =
        SHADER(void main() { gl_Position = vec4(1); });
    static const char* kFragmentShaderSource =
        SHADER(precision mediump float; uniform vec4 color;
               void main() { gl_FragColor = color; });

    GLuint program =
        GLTestHelper::LoadProgram(kVertexShaderSource, kFragmentShaderSource);
    glUseProgram(program);
    color_loc_ = glGetUniformLocation(program, "color");
    glDeleteProgram(program);

    // Set up orthogonal projection with near/far plane distance of 2.
    glMatrixLoadfCHROMIUM(GL_PATH_PROJECTION_CHROMIUM, kProjectionMatrix);
    glMatrixLoadIdentityCHROMIUM(GL_PATH_MODELVIEW_CHROMIUM);

    glEnable(GL_STENCIL_TEST);

    GLTestHelper::CheckGLError("no errors at state setup", __LINE__);
  }

  void SetupPathStateForTestPattern(GLuint path) {
    static const GLubyte kCommands[] = {GL_MOVE_TO_CHROMIUM,
                                        GL_LINE_TO_CHROMIUM,
                                        GL_QUADRATIC_CURVE_TO_CHROMIUM,
                                        GL_CUBIC_CURVE_TO_CHROMIUM,
                                        GL_CLOSE_PATH_CHROMIUM};

    static const GLfloat kCoords[] = {50.0f,
                                      50.0f,
                                      75.0f,
                                      75.0f,
                                      100.0f,
                                      62.5f,
                                      50.0f,
                                      25.5f,
                                      0.0f,
                                      62.5f,
                                      50.0f,
                                      50.0f,
                                      25.0f,
                                      75.0f};

    glPathCommandsCHROMIUM(path, arraysize(kCommands), kCommands,
                           arraysize(kCoords), GL_FLOAT, kCoords);

    glPathParameterfCHROMIUM(path, GL_PATH_STROKE_WIDTH_CHROMIUM, 5.0f);
    glPathParameterfCHROMIUM(path, GL_PATH_MITER_LIMIT_CHROMIUM, 1.0f);
    glPathParameterfCHROMIUM(path, GL_PATH_STROKE_BOUND_CHROMIUM, .02f);
    glPathParameteriCHROMIUM(path, GL_PATH_JOIN_STYLE_CHROMIUM,
                             GL_ROUND_CHROMIUM);
    glPathParameteriCHROMIUM(path, GL_PATH_END_CAPS_CHROMIUM,
                             GL_SQUARE_CHROMIUM);
  }

  void VerifyTestPatternFill(float x, float y) {
    SCOPED_TRACE(testing::Message() << "Verifying fill at " << x << "," << y);
    static const float kFillCoords[] = {55.0f, 54.0f, 50.0f,
                                        28.0f, 66.0f, 63.0f};
    static const uint8_t kBlue[] = {0, 0, 255, 255};

    for (size_t i = 0; i < arraysize(kFillCoords); i += 2) {
      float fx = kFillCoords[i];
      float fy = kFillCoords[i + 1];

      EXPECT_TRUE(
          GLTestHelper::CheckPixels(x + fx, y + fy, 1, 1, 0, kBlue, nullptr));
    }
  }

  void VerifyTestPatternBg(float x, float y) {
    SCOPED_TRACE(testing::Message() << "Verifying background at " << x << ","
                                    << y);
    const float kBackgroundCoords[] = {80.0f, 80.0f, 20.0f, 20.0f, 90.0f, 1.0f};
    const uint8_t kExpectedColor[] = {0, 0, 0, 0};

    for (size_t i = 0; i < arraysize(kBackgroundCoords); i += 2) {
      float bx = kBackgroundCoords[i];
      float by = kBackgroundCoords[i + 1];

      EXPECT_TRUE(GLTestHelper::CheckPixels(x + bx, y + by, 1, 1, 0,
                                            kExpectedColor, nullptr));
    }
  }

  void VerifyTestPatternStroke(float x, float y) {
    SCOPED_TRACE(testing::Message() << "Verifying stroke at " << x << "," << y);
    // Inside the stroke we should have green.
    const uint8_t kGreen[] = {0, 255, 0, 255};
    EXPECT_TRUE(
        GLTestHelper::CheckPixels(x + 50, y + 53, 1, 1, 0, kGreen, nullptr));
    EXPECT_TRUE(
        GLTestHelper::CheckPixels(x + 26, y + 76, 1, 1, 0, kGreen, nullptr));

    // Outside the path we should have black.
    const uint8_t black[] = {0, 0, 0, 0};
    EXPECT_TRUE(
        GLTestHelper::CheckPixels(x + 10, y + 10, 1, 1, 0, black, nullptr));
    EXPECT_TRUE(
        GLTestHelper::CheckPixels(x + 80, y + 80, 1, 1, 0, black, nullptr));
  }
  static const GLfloat kProjectionMatrix[16];
  GLint color_loc_;
};

const GLfloat CHROMIUMPathRenderingDrawTest::kProjectionMatrix[16] = {
    2.0f / (CHROMIUMPathRenderingTest::kResolution - 1),
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    2.0f / (CHROMIUMPathRenderingTest::kResolution - 1),
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    -1.0f,
    0.0f,
    -1.0f,
    -1.0f,
    0.0f,
    1.0f};

TEST_F(CHROMIUMPathRenderingTest, TestMatrix) {
  if (!IsApplicable())
    return;

  static const GLfloat kIdentityMatrix[16] = {
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  static const GLfloat kSeqMatrix[16] = {
      0.5f, -0.5f, -0.1f,  -0.8f,  4.4f,   5.5f,   6.6f,   7.7f,
      8.8f, 9.9f,  10.11f, 11.22f, 12.33f, 13.44f, 14.55f, 15.66f};
  static const GLenum kMatrixModes[] = {GL_PATH_MODELVIEW_CHROMIUM,
                                        GL_PATH_PROJECTION_CHROMIUM};
  static const GLenum kGetMatrixModes[] = {GL_PATH_MODELVIEW_MATRIX_CHROMIUM,
                                           GL_PATH_PROJECTION_MATRIX_CHROMIUM};

  for (size_t i = 0; i < arraysize(kMatrixModes); ++i) {
    GLfloat mf[16];
    GLint mi[16];
    memset(mf, 0, sizeof(mf));
    memset(mi, 0, sizeof(mi));
    glGetFloatv(kGetMatrixModes[i], mf);
    glGetIntegerv(kGetMatrixModes[i], mi);
    ExpectEqualMatrix(kIdentityMatrix, mf);
    ExpectEqualMatrix(kIdentityMatrix, mi);

    glMatrixLoadfCHROMIUM(kMatrixModes[i], kSeqMatrix);
    memset(mf, 0, sizeof(mf));
    memset(mi, 0, sizeof(mi));
    glGetFloatv(kGetMatrixModes[i], mf);
    glGetIntegerv(kGetMatrixModes[i], mi);
    ExpectEqualMatrix(kSeqMatrix, mf);
    ExpectEqualMatrix(kSeqMatrix, mi);

    glMatrixLoadIdentityCHROMIUM(kMatrixModes[i]);
    memset(mf, 0, sizeof(mf));
    memset(mi, 0, sizeof(mi));
    glGetFloatv(kGetMatrixModes[i], mf);
    glGetIntegerv(kGetMatrixModes[i], mi);
    ExpectEqualMatrix(kIdentityMatrix, mf);
    ExpectEqualMatrix(kIdentityMatrix, mi);

    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  }
}

TEST_F(CHROMIUMPathRenderingTest, TestMatrixErrors) {
  if (!IsApplicable())
    return;

  GLfloat mf[16];
  memset(mf, 0, sizeof(mf));

  glMatrixLoadfCHROMIUM(GL_PATH_MODELVIEW_CHROMIUM, mf);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  glMatrixLoadIdentityCHROMIUM(GL_PATH_PROJECTION_CHROMIUM);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // Test that invalid matrix targets fail.
  glMatrixLoadfCHROMIUM(GL_PATH_MODELVIEW_CHROMIUM - 1, mf);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());

  // Test that invalid matrix targets fail.
  glMatrixLoadIdentityCHROMIUM(GL_PATH_PROJECTION_CHROMIUM + 1);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());
}

TEST_F(CHROMIUMPathRenderingTest, TestSimpleCalls) {
  if (!IsApplicable())
    return;

  // This is unspecified in NV_path_rendering.
  EXPECT_EQ(0u, glGenPathsCHROMIUM(0));

  GLuint path = glGenPathsCHROMIUM(1);
  EXPECT_NE(path, 0u);
  glDeletePathsCHROMIUM(path, 1);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  GLuint first_path = glGenPathsCHROMIUM(5);
  EXPECT_NE(first_path, 0u);
  glDeletePathsCHROMIUM(first_path, 5);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // Test deleting paths that are not actually allocated:
  // "unused names in /paths/ are silently ignored".
  first_path = glGenPathsCHROMIUM(5);
  EXPECT_NE(first_path, 0u);
  glDeletePathsCHROMIUM(first_path, 6);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  GLsizei big_range = 0xffff;
  // Setting big_range = std::numeric_limits<GLsizei>::max() should go through
  // too, as far as NV_path_rendering is concerned. Current chromium side id
  // allocator will use too much memory.
  first_path = glGenPathsCHROMIUM(big_range);
  EXPECT_NE(first_path, 0u);
  glDeletePathsCHROMIUM(first_path, big_range);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // Test glIsPathCHROMIUM().
  path = glGenPathsCHROMIUM(1);
  EXPECT_FALSE(glIsPathCHROMIUM(path));
  GLubyte commands[] = {GL_MOVE_TO_CHROMIUM, GL_CLOSE_PATH_CHROMIUM};
  GLfloat coords[] = {50.0f, 50.0f};
  glPathCommandsCHROMIUM(path, arraysize(commands), commands, arraysize(coords),
                         GL_FLOAT, coords);
  EXPECT_TRUE(glIsPathCHROMIUM(path));
  glDeletePathsCHROMIUM(path, 1);
  EXPECT_FALSE(glIsPathCHROMIUM(path));
}

TEST_F(CHROMIUMPathRenderingTest, TestGenDeleteErrors) {
  if (!IsApplicable())
    return;

  // GenPaths / DeletePaths tests.
  // std::numeric_limits<GLuint>::max() is wrong for GLsizei.
  GLuint first_path = glGenPathsCHROMIUM(std::numeric_limits<GLuint>::max());
  EXPECT_EQ(first_path, 0u);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());

  first_path = glGenPathsCHROMIUM(-1);
  EXPECT_EQ(first_path, 0u);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());

  glDeletePathsCHROMIUM(1, -5);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());

  first_path = glGenPathsCHROMIUM(-1);
  EXPECT_EQ(first_path, 0u);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());

  // Test that delete with first_id and range such that first_id + range
  // overflows the GLuint. Example:
  // Range is 0x7fffffff. First id is X. Last id will be X + 0x7ffffffe.
  // X = 0x80000001 would succeed, where as X = 0x80000002 would fail.
  // To get 0x80000002, we need to allocate first 0x7fffffff and then
  // 3 (0x80000000, 0x80000001 and 0x80000002).
  // While not guaranteed by the API, we expect the implementation
  // hands us deterministic ids.
  first_path = glGenPathsCHROMIUM(std::numeric_limits<GLsizei>::max());
  EXPECT_EQ(first_path, 1u);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  GLuint additional_paths = glGenPathsCHROMIUM(3);
  EXPECT_EQ(additional_paths,
            static_cast<GLuint>(std::numeric_limits<GLsizei>::max()) + 1u);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // Test that passing a range so big that it would overflow client_id
  // + range - 1 check causes an error.
  glDeletePathsCHROMIUM(additional_paths + 2u,
                        std::numeric_limits<GLsizei>::max());
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());

  // Cleanup the above allocations. Also test that passing max value still
  // works.
  glDeletePathsCHROMIUM(1, std::numeric_limits<GLsizei>::max());
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  glDeletePathsCHROMIUM(std::numeric_limits<GLsizei>::max(),
                        std::numeric_limits<GLsizei>::max());
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
}

TEST_F(CHROMIUMPathRenderingTest, TestPathParameterErrors) {
  if (!IsApplicable())
    return;

  GLuint path = glGenPathsCHROMIUM(1);
  // PathParameter*: Wrong value for the pname should fail.
  glPathParameteriCHROMIUM(path, GL_PATH_JOIN_STYLE_CHROMIUM, GL_FLAT_CHROMIUM);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());
  glPathParameterfCHROMIUM(path, GL_PATH_END_CAPS_CHROMIUM,
                           GL_MITER_REVERT_CHROMIUM);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());

  // PathParameter*: Wrong floating-point value should fail.
  glPathParameterfCHROMIUM(path, GL_PATH_STROKE_WIDTH_CHROMIUM, -0.1f);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());
  glPathParameterfCHROMIUM(path, GL_PATH_MITER_LIMIT_CHROMIUM,
                           std::numeric_limits<float>::quiet_NaN());
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());
  glPathParameterfCHROMIUM(path, GL_PATH_MITER_LIMIT_CHROMIUM,
                           std::numeric_limits<float>::infinity());
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());

  // PathParameter*: Wrong pname should fail.
  glPathParameteriCHROMIUM(path, GL_PATH_STROKE_WIDTH_CHROMIUM - 1, 5);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());
  glDeletePathsCHROMIUM(path, 1);
}

TEST_F(CHROMIUMPathRenderingTest, TestPathObjectState) {
  if (!IsApplicable())
    return;

  glViewport(0, 0, kResolution, kResolution);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glStencilMask(0xffffffff);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glPathStencilFuncCHROMIUM(GL_ALWAYS, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

  // Test that trying to draw non-existing paths does not produce errors or
  // results.
  GLuint non_existing_paths[] = {0, 55, 74744};
  for (auto& p : non_existing_paths) {
    EXPECT_FALSE(glIsPathCHROMIUM(p));
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    TryAllDrawFunctions(p, GL_NO_ERROR);
  }

  // Path name marked as used but without path object state causes
  // a GL error upon any draw command.
  GLuint path = glGenPathsCHROMIUM(1);
  EXPECT_FALSE(glIsPathCHROMIUM(path));
  TryAllDrawFunctions(path, GL_INVALID_OPERATION);
  glDeletePathsCHROMIUM(path, 1);

  // Document a bit of an inconsistency: path name marked as used but without
  // path object state causes a GL error upon any draw command (tested above).
  // Path name that had path object state, but then was "cleared", still has a
  // path object state, even though the state is empty.
  path = glGenPathsCHROMIUM(1);
  EXPECT_FALSE(glIsPathCHROMIUM(path));
  GLubyte commands[] = {GL_MOVE_TO_CHROMIUM, GL_CLOSE_PATH_CHROMIUM};
  GLfloat coords[] = {50.0f, 50.0f};
  glPathCommandsCHROMIUM(path, arraysize(commands), commands, arraysize(coords),
                         GL_FLOAT, coords);
  EXPECT_TRUE(glIsPathCHROMIUM(path));
  glPathCommandsCHROMIUM(path, 0, NULL, 0, GL_FLOAT, NULL);
  EXPECT_TRUE(glIsPathCHROMIUM(path));  // The surprise.
  TryAllDrawFunctions(path, GL_NO_ERROR);
  glDeletePathsCHROMIUM(path, 1);

  // Document a bit of an inconsistency: "clearing" a used path name causes
  // path to acquire state.
  path = glGenPathsCHROMIUM(1);
  EXPECT_FALSE(glIsPathCHROMIUM(path));
  glPathCommandsCHROMIUM(path, 0, NULL, 0, GL_FLOAT, NULL);
  EXPECT_TRUE(glIsPathCHROMIUM(path));  // The surprise.
  glDeletePathsCHROMIUM(path, 1);

  // Make sure nothing got drawn by the drawing commands that should not produce
  // anything.
  const uint8_t black[] = {0, 0, 0, 0};
  EXPECT_TRUE(GLTestHelper::CheckPixels(0, 0, kResolution, kResolution, 0,
                                        black, nullptr));
}

TEST_F(CHROMIUMPathRenderingTest, TestUnnamedPathsErrors) {
  if (!IsApplicable())
    return;

  // Unnamed paths: Trying to create a path object with non-existing path name
  // produces error.  (Not a error in real NV_path_rendering).
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  GLubyte commands[] = {GL_MOVE_TO_CHROMIUM, GL_CLOSE_PATH_CHROMIUM};
  GLfloat coords[] = {50.0f, 50.0f};
  glPathCommandsCHROMIUM(555, arraysize(commands), commands, arraysize(coords),
                         GL_FLOAT, coords);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());

  // PathParameter*: Using non-existing path object produces error.
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  glPathParameterfCHROMIUM(555, GL_PATH_STROKE_WIDTH_CHROMIUM, 5.0f);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());

  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  glPathParameteriCHROMIUM(555, GL_PATH_JOIN_STYLE_CHROMIUM, GL_ROUND_CHROMIUM);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());
}

TEST_F(CHROMIUMPathRenderingTest, TestPathCommandsErrors) {
  if (!IsApplicable())
    return;

  static const GLenum kInvalidCoordType = GL_NONE;

  GLuint path = glGenPathsCHROMIUM(1);
  GLubyte commands[] = {GL_MOVE_TO_CHROMIUM, GL_CLOSE_PATH_CHROMIUM};
  GLfloat coords[] = {50.0f, 50.0f};

  glPathCommandsCHROMIUM(path, arraysize(commands), commands, -4, GL_FLOAT,
                         coords);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());

  glPathCommandsCHROMIUM(path, -1, commands, arraysize(coords), GL_FLOAT,
                         coords);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());

  glPathCommandsCHROMIUM(path, arraysize(commands), commands, arraysize(coords),
                         kInvalidCoordType, coords);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());

  // These can not distinquish between the check that should fail them.
  // This should fail due to coord count * float size overflow.
  glPathCommandsCHROMIUM(path, arraysize(commands), commands,
                         std::numeric_limits<GLsizei>::max(), GL_FLOAT, coords);
  // This should fail due to cmd count + coord count * short size.
  glPathCommandsCHROMIUM(path, arraysize(commands), commands,
                         std::numeric_limits<GLsizei>::max(), GL_SHORT, coords);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());

  glDeletePathsCHROMIUM(path, 1);
}

TEST_F(CHROMIUMPathRenderingTest, TestPathRenderingInvalidArgs) {
  if (!IsApplicable())
    return;

  GLuint path = glGenPathsCHROMIUM(1);
  glPathCommandsCHROMIUM(path, 0, NULL, 0, GL_FLOAT, NULL);

  // Verify that normal calls work.
  glStencilFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM, 0x7F);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  glStencilThenCoverFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM, 0x7F,
                                     GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // Using invalid fill mode causes INVALID_ENUM.
  glStencilFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM - 1, 0x7F);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());
  glStencilThenCoverFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM - 1, 0x7F,
                                     GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());

  // Using invalid cover mode causes INVALID_ENUM.
  glCoverFillPathCHROMIUM(path, GL_CONVEX_HULL_CHROMIUM - 1);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());
  glStencilThenCoverFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM, 0x7F,
                                     GL_BOUNDING_BOX_CHROMIUM + 1);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());
  // For instanced variants, we need this to error the same way
  // regardless of whether # of paths == 0 would cause an early return.
  for (int path_count = 0; path_count <= 1; ++path_count) {
    SCOPED_TRACE(testing::Message()
                 << "Invalid fillmode instanced test for path count "
                 << path_count);
    glStencilFillPathInstancedCHROMIUM(path_count, GL_UNSIGNED_INT, &path, 0,
                                       GL_COUNT_UP_CHROMIUM - 1, 0x7F, GL_NONE,
                                       NULL);
    EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());
    glStencilThenCoverFillPathInstancedCHROMIUM(
        path_count, GL_UNSIGNED_INT, &path, 0, GL_COUNT_UP_CHROMIUM - 1, 0x7F,
        GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, GL_NONE, NULL);
    EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());
  }

  // Using mask+1 not being power of two causes INVALID_VALUE with up/down fill
  // mode.
  glStencilFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM, 0x40);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());
  glStencilThenCoverFillPathCHROMIUM(path, GL_COUNT_DOWN_CHROMIUM, 12,
                                     GL_BOUNDING_BOX_CHROMIUM);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());
  for (int path_count = 0; path_count <= 1; ++path_count) {
    SCOPED_TRACE(testing::Message()
                 << "Invalid mask instanced test for path count "
                 << path_count);
    glStencilFillPathInstancedCHROMIUM(path_count, GL_UNSIGNED_INT, &path, 0,
                                       GL_COUNT_UP_CHROMIUM, 0x30, GL_NONE,
                                       NULL);
    EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());
    glStencilThenCoverFillPathInstancedCHROMIUM(
        path_count, GL_UNSIGNED_INT, &path, 0, GL_COUNT_DOWN_CHROMIUM, 0xFE,
        GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, GL_NONE, NULL);
    EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());
  }

  glDeletePathsCHROMIUM(path, 1);
}

// Tests that drawing with CHROMIUM_path_rendering functions work.
TEST_F(CHROMIUMPathRenderingDrawTest, TestPathRendering) {
  if (!IsApplicable())
    return;

  static const float kBlue[] = {0.0f, 0.0f, 1.0f, 1.0f};
  static const float kGreen[] = {0.0f, 1.0f, 0.0f, 1.0f};

  SetupStateForTestPattern();

  GLuint path = glGenPathsCHROMIUM(1);
  SetupPathStateForTestPattern(path);

  // Do the stencil fill, cover fill, stencil stroke, cover stroke
  // in unconventional order:
  // 1) stencil the stroke in stencil high bit
  // 2) stencil the fill in low bits
  // 3) cover the fill
  // 4) cover the stroke
  // This is done to check that glPathStencilFunc works, eg the mask
  // goes through. Stencil func is not tested ATM, for simplicity.

  glPathStencilFuncCHROMIUM(GL_ALWAYS, 0, 0xFF);
  glStencilStrokePathCHROMIUM(path, 0x80, 0x80);

  glPathStencilFuncCHROMIUM(GL_ALWAYS, 0, 0x7F);
  glStencilFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM, 0x7F);

  glStencilFunc(GL_LESS, 0, 0x7F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glUniform4fv(color_loc_, 1, kBlue);
  glCoverFillPathCHROMIUM(path, GL_BOUNDING_BOX_CHROMIUM);

  glStencilFunc(GL_EQUAL, 0x80, 0x80);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glUniform4fv(color_loc_, 1, kGreen);
  glCoverStrokePathCHROMIUM(path, GL_CONVEX_HULL_CHROMIUM);

  glDeletePathsCHROMIUM(path, 1);

  // Verify the image.
  VerifyTestPatternFill(0.0f, 0.0f);
  VerifyTestPatternBg(0.0f, 0.0f);
  VerifyTestPatternStroke(0.0f, 0.0f);
}

// Tests that drawing with CHROMIUM_path_rendering
// StencilThenCover{Stroke,Fill}Path functions work.
TEST_F(CHROMIUMPathRenderingDrawTest, TestPathRenderingThenFunctions) {
  if (!IsApplicable())
    return;

  static float kBlue[] = {0.0f, 0.0f, 1.0f, 1.0f};
  static float kGreen[] = {0.0f, 1.0f, 0.0f, 1.0f};

  SetupStateForTestPattern();

  GLuint path = glGenPathsCHROMIUM(1);
  SetupPathStateForTestPattern(path);

  glPathStencilFuncCHROMIUM(GL_ALWAYS, 0, 0xFF);
  glStencilFunc(GL_EQUAL, 0x80, 0x80);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glUniform4fv(color_loc_, 1, kGreen);
  glStencilThenCoverStrokePathCHROMIUM(path, 0x80, 0x80,
                                       GL_BOUNDING_BOX_CHROMIUM);

  glPathStencilFuncCHROMIUM(GL_ALWAYS, 0, 0x7F);
  glStencilFunc(GL_LESS, 0, 0x7F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glUniform4fv(color_loc_, 1, kBlue);
  glStencilThenCoverFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM, 0x7F,
                                     GL_CONVEX_HULL_CHROMIUM);

  glDeletePathsCHROMIUM(path, 1);

  // Verify the image.
  VerifyTestPatternFill(0.0f, 0.0f);
  VerifyTestPatternBg(0.0f, 0.0f);
  VerifyTestPatternStroke(0.0f, 0.0f);
}

// Tests that drawing with *Instanced functions work.
TEST_F(CHROMIUMPathRenderingDrawTest, TestPathRenderingInstanced) {
  if (!IsApplicable())
    return;

  static const float kBlue[] = {0.0f, 0.0f, 1.0f, 1.0f};
  static const float kGreen[] = {0.0f, 1.0f, 0.0f, 1.0f};

  SetupStateForTestPattern();

  GLuint path = glGenPathsCHROMIUM(1);
  SetupPathStateForTestPattern(path);

  const GLuint kPaths[] = {1, 1, 1, 1, 1};
  const GLsizei kPathCount = arraysize(kPaths);
  const GLfloat kShapeSize = 80.0f;
  static const GLfloat kTransforms[kPathCount * 12] = {
      1.0f, 0.0f, 0.0f, 0.0f,           1.0f,       0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,           0.0f,       0.0f,
      1.0f, 0.0f, 0.0f, 0.0f,           1.0f,       0.0f,
      0.0f, 0.0f, 1.0f, kShapeSize,     0.0f,       0.0f,
      1.0f, 0.0f, 0.0f, 0.0f,           1.0f,       0.0f,
      0.0f, 0.0f, 1.0f, kShapeSize * 2, 0.0f,       0.0f,
      1.0f, 0.0f, 0.0f, 0.0f,           1.0f,       0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,           kShapeSize, 0.0f,
      1.0f, 0.0f, 0.0f, 0.0f,           1.0f,       0.0f,
      0.0f, 0.0f, 1.0f, kShapeSize,     kShapeSize, 0.0f};

  // The test pattern is the same as in the simple draw case above,
  // except that the path is drawn kPathCount times with different offsets.
  glPathStencilFuncCHROMIUM(GL_ALWAYS, 0, 0xFF);
  glStencilStrokePathInstancedCHROMIUM(kPathCount, GL_UNSIGNED_INT, kPaths,
                                       path - 1, 0x80, 0x80,
                                       GL_AFFINE_3D_CHROMIUM, kTransforms);

  glPathStencilFuncCHROMIUM(GL_ALWAYS, 0, 0x7F);
  glUniform4fv(color_loc_, 1, kBlue);
  glStencilFillPathInstancedCHROMIUM(kPathCount, GL_UNSIGNED_INT, kPaths,
                                     path - 1, GL_COUNT_UP_CHROMIUM, 0x7F,
                                     GL_AFFINE_3D_CHROMIUM, kTransforms);
  glStencilFunc(GL_LESS, 0, 0x7F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glCoverFillPathInstancedCHROMIUM(kPathCount, GL_UNSIGNED_INT, kPaths,
                                   path - 1,
                                   GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM,
                                   GL_AFFINE_3D_CHROMIUM, kTransforms);
  glStencilFunc(GL_EQUAL, 0x80, 0x80);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glUniform4fv(color_loc_, 1, kGreen);
  glCoverStrokePathInstancedCHROMIUM(kPathCount, GL_UNSIGNED_INT, kPaths,
                                     path - 1,
                                     GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM,
                                     GL_AFFINE_3D_CHROMIUM, kTransforms);

  glDeletePathsCHROMIUM(path, 1);

  // Verify the image.
  VerifyTestPatternFill(0.0f, 0.0f);
  VerifyTestPatternBg(0.0f, 0.0f);
  VerifyTestPatternStroke(0.0f, 0.0f);

  VerifyTestPatternFill(kShapeSize, 0.0f);
  VerifyTestPatternBg(kShapeSize, 0.0f);
  VerifyTestPatternStroke(kShapeSize, 0.0f);

  VerifyTestPatternFill(kShapeSize * 2, 0.0f);
  VerifyTestPatternBg(kShapeSize * 2, 0.0f);
  VerifyTestPatternStroke(kShapeSize * 2, 0.0f);

  VerifyTestPatternFill(0.0f, kShapeSize);
  VerifyTestPatternBg(0.0f, kShapeSize);
  VerifyTestPatternStroke(0.0f, kShapeSize);

  VerifyTestPatternFill(kShapeSize, kShapeSize);
  VerifyTestPatternBg(kShapeSize, kShapeSize);
  VerifyTestPatternStroke(kShapeSize, kShapeSize);
}

TEST_F(CHROMIUMPathRenderingDrawTest, TestPathRenderingThenFunctionsInstanced) {
  if (!IsApplicable())
    return;

  static const float kBlue[] = {0.0f, 0.0f, 1.0f, 1.0f};
  static const float kGreen[] = {0.0f, 1.0f, 0.0f, 1.0f};

  SetupStateForTestPattern();

  GLuint path = glGenPathsCHROMIUM(1);
  SetupPathStateForTestPattern(path);

  const GLuint kPaths[] = {1, 1, 1, 1, 1};
  const GLsizei kPathCount = arraysize(kPaths);
  const GLfloat kShapeSize = 80.0f;
  static const GLfloat kTransforms[] = {
      0.0f, 0.0f, kShapeSize, 0.0f,       kShapeSize * 2,
      0.0f, 0.0f, kShapeSize, kShapeSize, kShapeSize,
  };

  glPathStencilFuncCHROMIUM(GL_ALWAYS, 0, 0xFF);
  glStencilFunc(GL_EQUAL, 0x80, 0x80);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glUniform4fv(color_loc_, 1, kGreen);
  glStencilThenCoverStrokePathInstancedCHROMIUM(
      kPathCount, GL_UNSIGNED_INT, kPaths, path - 1, 0x80, 0x80,
      GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, GL_TRANSLATE_2D_CHROMIUM,
      kTransforms);

  glPathStencilFuncCHROMIUM(GL_ALWAYS, 0, 0x7F);
  glStencilFunc(GL_LESS, 0, 0x7F);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
  glUniform4fv(color_loc_, 1, kBlue);
  glStencilThenCoverFillPathInstancedCHROMIUM(
      kPathCount, GL_UNSIGNED_INT, kPaths, path - 1, GL_COUNT_UP_CHROMIUM, 0x7F,
      GL_BOUNDING_BOX_OF_BOUNDING_BOXES_CHROMIUM, GL_TRANSLATE_2D_CHROMIUM,
      kTransforms);

  glDeletePathsCHROMIUM(path, 1);

  // Verify the image.
  VerifyTestPatternFill(0.0f, 0.0f);
  VerifyTestPatternBg(0.0f, 0.0f);
  VerifyTestPatternStroke(0.0f, 0.0f);

  VerifyTestPatternFill(kShapeSize, 0.0f);
  VerifyTestPatternBg(kShapeSize, 0.0f);
  VerifyTestPatternStroke(kShapeSize, 0.0f);

  VerifyTestPatternFill(kShapeSize * 2, 0.0f);
  VerifyTestPatternBg(kShapeSize * 2, 0.0f);
  VerifyTestPatternStroke(kShapeSize * 2, 0.0f);

  VerifyTestPatternFill(0.0f, kShapeSize);
  VerifyTestPatternBg(0.0f, kShapeSize);
  VerifyTestPatternStroke(0.0f, kShapeSize);

  VerifyTestPatternFill(kShapeSize, kShapeSize);
  VerifyTestPatternBg(kShapeSize, kShapeSize);
  VerifyTestPatternStroke(kShapeSize, kShapeSize);
}

// This class implements a test that draws a grid of v-shapes. The grid is
// drawn so that even rows (from the bottom) are drawn with DrawArrays and odd
// rows are drawn with path rendering.  It can be used to test various texturing
// modes, comparing how the fill would work in normal GL rendering and how to
// setup same sort of fill with path rendering.
// The texturing test is parametrized to run the test with and without
// ANGLE name hashing.
class CHROMIUMPathRenderingWithTexturingTest
    : public CHROMIUMPathRenderingTest,
      public ::testing::WithParamInterface<bool> {
 protected:
  void InitializeContextFeatures(GLManager::Options* options) override {
    CHROMIUMPathRenderingTest::InitializeContextFeatures(options);
    options->force_shader_name_hashing = GetParam();
  }

  /** Sets up the GL program state for the test.
     Vertex shader needs at least following variables:
      uniform mat4 view_matrix;
      uniform mat? color_matrix; (accessible with kColorMatrixLocation)
      uniform vec2 model_translate;
      attribute vec2 position;
      varying vec4 color;

     Fragment shader needs at least following variables:
      varying vec4 color;

      (? can be anything)
  */
  void SetupProgramForTestPattern(const char* vertex_shader_source,
                                  const char* fragment_shader_source) {
    glViewport(0, 0, kResolution, kResolution);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glStencilMask(0xffffffff);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint vs =
        GLTestHelper::LoadShader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fs =
        GLTestHelper::LoadShader(GL_FRAGMENT_SHADER, fragment_shader_source);

    program_ = glCreateProgram();
    glBindAttribLocation(program_, kPositionLocation, "position");
    glBindUniformLocationCHROMIUM(program_, kViewMatrixLocation, "view_matrix");
    glBindUniformLocationCHROMIUM(program_, kColorMatrixLocation,
                                  "color_matrix");
    glBindUniformLocationCHROMIUM(program_, kModelTranslateLocation,
                                  "model_translate");
    glBindFragmentInputLocationCHROMIUM(program_, kColorFragmentInputLocation,
                                        "color");
    glAttachShader(program_, fs);
    glAttachShader(program_, vs);
    glDeleteShader(vs);
    glDeleteShader(fs);
  }

  void LinkProgramForTestPattern() {
    glLinkProgram(program_);
    GLint linked = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    EXPECT_TRUE(linked == GL_TRUE);
    glUseProgram(program_);

    glUniformMatrix4fv(kViewMatrixLocation, 1, GL_FALSE, kProjectionMatrix);
  }

  void DrawTestPattern() {
    // Setup state for drawing the shape with DrawArrays.

    // This v-shape is used both for DrawArrays and path rendering.
    static const GLfloat kVertices[] = {75.0f, 75.0f, 50.0f, 25.5f,
                                        50.0f, 50.0f, 25.0f, 75.0f};

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(kPositionLocation);
    glVertexAttribPointer(kPositionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // Setup state for drawing the shape with path rendering.
    glPathStencilFuncCHROMIUM(GL_ALWAYS, 0, 0x7F);
    glStencilFunc(GL_LESS, 0, 0x7F);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    glMatrixLoadfCHROMIUM(GL_PATH_PROJECTION_CHROMIUM, kProjectionMatrix);
    glMatrixLoadIdentityCHROMIUM(GL_PATH_MODELVIEW_CHROMIUM);

    static const GLubyte kCommands[] = {
        GL_MOVE_TO_CHROMIUM, GL_LINE_TO_CHROMIUM, GL_LINE_TO_CHROMIUM,
        GL_LINE_TO_CHROMIUM, GL_CLOSE_PATH_CHROMIUM};

    static const GLfloat kCoords[] = {
        kVertices[0], kVertices[1], kVertices[2], kVertices[3],
        kVertices[6], kVertices[7], kVertices[4], kVertices[5],
    };

    GLuint path = glGenPathsCHROMIUM(1);
    glPathCommandsCHROMIUM(path, arraysize(kCommands), kCommands,
                           arraysize(kCoords), GL_FLOAT, kCoords);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

    GLfloat path_model_translate[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    // Draws the shapes. Every even row from the bottom is drawn with
    // DrawArrays, odd row with path rendering. The shader program is
    // the same for the both draws.
    for (int j = 0; j < kTestRows; ++j) {
      for (int i = 0; i < kTestColumns; ++i) {
        if (j % 2 == 0) {
          glDisable(GL_STENCIL_TEST);
          glUniform2f(kModelTranslateLocation, i * kShapeWidth,
                      j * kShapeHeight);
          glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        } else {
          glEnable(GL_STENCIL_TEST);
          path_model_translate[12] = i * kShapeWidth;
          path_model_translate[13] = j * kShapeHeight;
          glMatrixLoadfCHROMIUM(GL_PATH_MODELVIEW_CHROMIUM,
                                path_model_translate);
          glStencilThenCoverFillPathCHROMIUM(path, GL_COUNT_UP_CHROMIUM, 0x7F,
                                             GL_BOUNDING_BOX_CHROMIUM);
        }
      }
    }

    glDisableVertexAttribArray(kPositionLocation);
    glDeleteBuffers(1, &vbo);
    glDeletePathsCHROMIUM(path, 1);
  }

  void TeardownStateForTestPattern() { glDeleteProgram(program_); }

  static const GLfloat kProjectionMatrix[16];

  // This uniform be can set by the test. It should be used to set the color for
  // drawing with DrawArrays.
  static const GLint kColorMatrixLocation = 4;

  // This fragment input can be set by the test. It should be used to set the
  // color for drawing with path rendering.
  static const GLint kColorFragmentInputLocation = 7;

  enum {
    kShapeWidth = 75,
    kShapeHeight = 75,
    kTestRows = kResolution / kShapeHeight,
    kTestColumns = kResolution / kShapeWidth,
  };

  // These coordinates are inside the shape fill. This can be used to verífy
  // fill color.
  static const float kFillCoords[6];

  GLint program_;

  static const GLint kModelTranslateLocation = 3;
  static const GLint kPositionLocation = 0;
  static const GLint kViewMatrixLocation = 7;
};

const GLfloat CHROMIUMPathRenderingWithTexturingTest::kProjectionMatrix[16] = {
    2.0f / (CHROMIUMPathRenderingWithTexturingTest::kResolution - 1),
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    2.0f / (CHROMIUMPathRenderingWithTexturingTest::kResolution - 1),
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    -1.0f,
    0.0f,
    -1.0f,
    -1.0f,
    0.0f,
    1.0f};

const GLfloat CHROMIUMPathRenderingWithTexturingTest::kFillCoords[6] = {
    59.0f, 50.0f, 50.0f, 28.0f, 66.0f, 63.0f};

// This test tests ProgramPathFragmentInputGenCHROMIUM and
// BindFragmentInputLocationCHROMIUM. The test draws a shape multiple times as a
// grid. Each shape is filled with a color pattern that has projection-space
// gradient of the fragment coordinates in r and g components of the color. The
// color slides as function of coordinates: x=0..kResolution --> r=0..1,
// y=0..kResolution --> g=0..1
TEST_P(CHROMIUMPathRenderingWithTexturingTest,
       TestProgramPathFragmentInputGenCHROMIUM_EYE) {
  if (!IsApplicable())
    return;

  // clang-format off
  static const char* kVertexShaderSource = SHADER(
      uniform mat4 view_matrix;
      uniform mat4 color_matrix;
      uniform vec2 model_translate;
      attribute vec2 position;
      varying vec3 color;
      void main() {
        vec4 p = vec4(model_translate + position, 1, 1);
        color = (color_matrix * p).rgb;
        gl_Position = view_matrix * p;
      }
  );

  static const char* kFragmentShaderSource = SHADER(
      precision mediump float;
      varying vec3 color;
      void main() {
        gl_FragColor = vec4(color, 1.0);
      }
  );
  // clang-format on

  SetupProgramForTestPattern(kVertexShaderSource, kFragmentShaderSource);
  LinkProgramForTestPattern();
  static const GLfloat kColorMatrix[16] = {
      1.0f / kResolution,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      1.0f / kResolution,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
  };
  glUniformMatrix4fv(kColorMatrixLocation, 1, GL_FALSE, kColorMatrix);
  // This is the functionality we are testing: ProgramPathFragmentInputGen
  // does the same work as the color transform in vertex shader.
  static const GLfloat kColorCoefficients[12] = {1.0f / kResolution,
                                                 0.0f,
                                                 0.0f,
                                                 0.0f,
                                                 0.0f,
                                                 1.0f / kResolution,
                                                 0.0f,
                                                 0.0f,
                                                 0.0f,
                                                 0.0f,
                                                 0.0f,
                                                 0.0f};
  glProgramPathFragmentInputGenCHROMIUM(program_, kColorFragmentInputLocation,
                                        GL_EYE_LINEAR_CHROMIUM, 3,
                                        kColorCoefficients);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  DrawTestPattern();

  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  for (int j = 0; j < kTestRows; ++j) {
    for (int i = 0; i < kTestColumns; ++i) {
      for (size_t k = 0; k < arraysize(kFillCoords); k += 2) {
        SCOPED_TRACE(testing::Message() << "Verifying fill for shape " << i
                                        << ", " << j << " coord " << k);
        float fx = kFillCoords[k];
        float fy = kFillCoords[k + 1];
        float px = i * kShapeWidth;
        float py = j * kShapeHeight;

        uint8_t color[4];
        color[0] = roundf((px + fx) / kResolution * 255.0f);
        color[1] = roundf((py + fy) / kResolution * 255.0f);
        color[2] = 0;
        color[3] = 255;

        EXPECT_TRUE(GLTestHelper::CheckPixels(px + fx, py + fy, 1, 1, 2, color,
                                              nullptr));
      }
    }
  }

  TeardownStateForTestPattern();

  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
}

// This test tests ProgramPathFragmentInputGenCHROMIUM and
// BindFragmentInputLocationCHROMIUM, same as above test.
// Each shape is filled with a color pattern that has object-space
// gradient of the fragment coordinates in r and g components of the color. The
// color slides as function of object coordinates: x=0..kShapeWidth --> r=0..1,
// y=0..kShapeWidth --> g=0..1
TEST_P(CHROMIUMPathRenderingWithTexturingTest,
       TestProgramPathFragmentInputGenCHROMIUM_OBJECT) {
  if (!IsApplicable())
    return;

  // clang-format off
  static const char* kVertexShaderSource = SHADER(
      uniform mat4 view_matrix;
      uniform mat4 color_matrix;
      uniform vec2 model_translate;
      attribute vec2 position;
      varying vec3 color;
      void main() {
        color = (color_matrix * vec4(position, 1, 1)).rgb;
        vec4 p = vec4(model_translate + position, 1, 1);
        gl_Position = view_matrix * p;
      }
  );

  static const char* kFragmentShaderSource = SHADER(
      precision mediump float;
      varying vec3 color;
      void main() {
        gl_FragColor = vec4(color.rgb, 1.0);
      }
  );
  // clang-format on

  SetupProgramForTestPattern(kVertexShaderSource, kFragmentShaderSource);
  LinkProgramForTestPattern();
  static const GLfloat kColorMatrix[16] = {1.0f / kShapeWidth,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           1.0f / kShapeHeight,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f};
  glUniformMatrix4fv(kColorMatrixLocation, 1, GL_FALSE, kColorMatrix);

  // This is the functionality we are testing: ProgramPathFragmentInputGen
  // does the same work as the color transform in vertex shader.
  static const GLfloat kColorCoefficients[9] = {1.0f / kShapeWidth,
                                                0.0f,
                                                0.0f,
                                                0.0f,
                                                1.0f / kShapeHeight,
                                                0.0f,
                                                0.0f,
                                                0.0f,
                                                0.0f};
  glProgramPathFragmentInputGenCHROMIUM(program_, kColorFragmentInputLocation,
                                        GL_OBJECT_LINEAR_CHROMIUM, 3,
                                        kColorCoefficients);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  DrawTestPattern();

  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  for (int j = 0; j < kTestRows; ++j) {
    for (int i = 0; i < kTestColumns; ++i) {
      for (size_t k = 0; k < arraysize(kFillCoords); k += 2) {
        SCOPED_TRACE(testing::Message() << "Verifying fill for shape " << i
                                        << ", " << j << " coord " << k);
        float fx = kFillCoords[k];
        float fy = kFillCoords[k + 1];
        float px = i * kShapeWidth;
        float py = j * kShapeHeight;

        uint8_t color[4];
        color[0] = roundf(fx / kShapeWidth * 255.0f);
        color[1] = roundf(fy / kShapeHeight * 255.0f);
        color[2] = 0;
        color[3] = 255;

        EXPECT_TRUE(GLTestHelper::CheckPixels(px + fx, py + fy, 1, 1, 2, color,
                                              nullptr));
      }
    }
  }

  TeardownStateForTestPattern();

  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
}

TEST_P(CHROMIUMPathRenderingWithTexturingTest,
       TestProgramPathFragmentInputGenArgs) {
  if (!IsApplicable())
    return;

  // clang-format off
  static const char* kVertexShaderSource = SHADER(
      varying vec2 vec2_var; varying vec3 vec3_var; varying vec4 vec4_var;
      varying float float_var; varying mat2 mat2_var; varying mat3 mat3_var;
      varying mat4 mat4_var; attribute float avoid_opt; void main() {
        vec2_var = vec2(1.0, 2.0 + avoid_opt);
        vec3_var = vec3(1.0, 2.0, 3.0 + avoid_opt);
        vec4_var = vec4(1.0, 2.0, 3.0, 4.0 + avoid_opt);
        float_var = 5.0 + avoid_opt;
        mat2_var = mat2(2.0 + avoid_opt);
        mat3_var = mat3(3.0 + avoid_opt);
        mat4_var = mat4(4.0 + avoid_opt);
        gl_Position = vec4(1.0);
      }
  );

  static const char* kFragmentShaderSource = SHADER(
      precision mediump float; varying vec2 vec2_var; varying vec3 vec3_var;
      varying vec4 vec4_var; varying float float_var; varying mat2 mat2_var;
      varying mat3 mat3_var; varying mat4 mat4_var; void main() {
        gl_FragColor = vec4(vec2_var, 0, 0) + vec4(vec3_var, 0) + vec4_var +
                       vec4(float_var) +
                       vec4(mat2_var[0][0], mat3_var[1][1], mat4_var[2][2], 1);
      }
  );
  // clang-format on
  GLuint vs = GLTestHelper::LoadShader(GL_VERTEX_SHADER, kVertexShaderSource);
  GLuint fs =
      GLTestHelper::LoadShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
  enum {
    kVec2Location = 0,
    kVec3Location,
    kVec4Location,
    kFloatLocation,
    kMat2Location,
    kMat3Location,
    kMat4Location,
  };
  struct {
    GLint location;
    const char* name;
    GLint components;
  } variables[] = {
      {kVec2Location, "vec2_var", 2},
      {kVec3Location, "vec3_var", 3},
      {kVec4Location, "vec4_var", 4},
      {kFloatLocation, "float_var", 1},
      // If a varying is not single-precision floating-point scalar or
      // vector, it always causes an invalid operation.
      {kMat2Location, "mat2_var", -1},
      {kMat3Location, "mat3_var", -1},
      {kMat4Location, "mat4_var", -1},
  };

  GLint program = glCreateProgram();
  for (size_t i = 0; i < sizeof(variables) / sizeof(variables[0]); ++i) {
    glBindFragmentInputLocationCHROMIUM(program, variables[i].location,
                                        variables[i].name);
  }
  glAttachShader(program, fs);
  glAttachShader(program, vs);
  glDeleteShader(vs);
  glDeleteShader(fs);

  // Test that using invalid (not linked) program is an invalid operation.
  // See similar calls at the end of the test for discussion about the
  // arguments.
  glProgramPathFragmentInputGenCHROMIUM(program, -1, GL_NONE, 0, NULL);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());

  glLinkProgram(program);

  GLint linked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  EXPECT_TRUE(linked == GL_TRUE);
  glUseProgram(program);

  const GLfloat kCoefficients16[] = {1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,
                                     7.0f,  8.0f,  9.0f,  10.0f, 11.0f, 12.0f,
                                     13.0f, 14.0f, 15.0f, 16.0f};
  const GLenum kGenModes[] = {GL_NONE, GL_EYE_LINEAR_CHROMIUM,
                              GL_OBJECT_LINEAR_CHROMIUM, GL_CONSTANT_CHROMIUM};

  for (size_t ii = 0; ii < sizeof(variables) / sizeof(variables[0]); ++ii) {
    for (GLint components = 0; components <= 4; ++components) {
      for (size_t jj = 0; jj < arraysize(kGenModes); ++jj) {
        GLenum gen_mode = kGenModes[jj];
        SCOPED_TRACE(testing::Message()
                     << "Testing glProgramPathFragmentInputGenCHROMIUM "
                     << "for fragment input '" << variables[ii].name
                     << "' with " << variables[ii].components << " components "
                     << " using genMode " << gen_mode << " and components "
                     << components);

        glProgramPathFragmentInputGenCHROMIUM(program, variables[ii].location,
                                              gen_mode, components,
                                              kCoefficients16);

        if (components == 0 && gen_mode == GL_NONE) {
          if (variables[ii].components == -1) {
            // Clearing a fragment input that is not single-precision floating
            // point scalar or vector is an invalid operation.
            EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());
          } else {
            // Clearing a valid fragment input is ok.
            EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
          }
        } else if (components == 0 || gen_mode == GL_NONE) {
          EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());
        } else {
          if (components == variables[ii].components) {
            // Setting a generator for a single-precision floating point
            // scalar or vector fragment input is ok.
            EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
          } else {
            // Setting a generator when components do not match is an invalid
            // operation.
            EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());
          }
        }
      }
    }
  }

  // The location == -1 would mean fragment input was optimized away. At the
  // time of writing, -1 can not happen because the only way to obtain the
  // location numbers is through bind. Test just to be consistent.

  enum {
    kValidGenMode = GL_CONSTANT_CHROMIUM,
    kValidComponents = 3,
    kInvalidGenMode = 0xAB,
    kInvalidComponents = 5,
  };

  glProgramPathFragmentInputGenCHROMIUM(program, -1, kValidGenMode,
                                        kValidComponents, kCoefficients16);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // Test that even though the spec says location == -1 causes the operation to
  // be skipped, the verification of other parameters is still done. This is a
  // GL policy.
  glProgramPathFragmentInputGenCHROMIUM(program, -1, kInvalidGenMode,
                                        kValidComponents, kCoefficients16);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());
  glProgramPathFragmentInputGenCHROMIUM(program, -1, kInvalidGenMode,
                                        kInvalidComponents, kCoefficients16);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_ENUM), glGetError());
  glProgramPathFragmentInputGenCHROMIUM(program, -1, kValidGenMode,
                                        kInvalidComponents, kCoefficients16);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_VALUE), glGetError());

  EXPECT_TRUE(glIsProgram(program));

  glDeleteProgram(program);

  EXPECT_FALSE(glIsProgram(program));

  // Test that using invalid (deleted) program is an invalid operation.
  glProgramPathFragmentInputGenCHROMIUM(program, -1, kValidGenMode,
                                        kValidComponents, kCoefficients16);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());
  glProgramPathFragmentInputGenCHROMIUM(program, -1, kInvalidGenMode,
                                        kValidComponents, kCoefficients16);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());
  glProgramPathFragmentInputGenCHROMIUM(program, -1, kInvalidGenMode,
                                        kInvalidComponents, kCoefficients16);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());
  glProgramPathFragmentInputGenCHROMIUM(program, -1, kValidGenMode,
                                        kInvalidComponents, kCoefficients16);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());
}

// This test uses gl_FragCoord in a fragment shader. It is used to ensure
// that the internal implementation runs codepaths related to built-ins.
TEST_P(CHROMIUMPathRenderingWithTexturingTest,
       TestProgramPathFragmentInputGenBuiltinInFragShader) {
  if (!IsApplicable())
    return;

  static const int kColorLocation = 5;
  static const int kFragColorLocation = 6;

  // clang-format off
  static const char* kVertexShaderSource = SHADER(
      varying vec4 color;
      void main() {
        color = vec4(1.0);
        gl_Position = vec4(1.0);
      }
  );

  static const char* kFragmentShaderSource = SHADER(
      precision mediump float;
      varying vec4 color;
      void main() {
        gl_FragColor = gl_FragCoord + color;
      }
  );
  // clang-format on

  GLuint vs = GLTestHelper::LoadShader(GL_VERTEX_SHADER, kVertexShaderSource);
  GLuint fs =
      GLTestHelper::LoadShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);

  GLint program = glCreateProgram();
  glBindFragmentInputLocationCHROMIUM(program, kColorLocation, "color");
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  glBindFragmentInputLocationCHROMIUM(program, kFragColorLocation,
                                      "gl_FragColor");
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());

  glAttachShader(program, fs);
  glAttachShader(program, vs);
  glDeleteShader(vs);
  glDeleteShader(fs);

  glLinkProgram(program);

  GLint linked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (linked == 0) {
    char buffer[1024];
    GLsizei length = 0;
    glGetProgramInfoLog(program, sizeof(buffer), &length, buffer);
    std::string log(buffer, length);
    EXPECT_EQ(1, linked) << "Error linking program: " << log;
    glDeleteProgram(program);
    program = 0;
  }
  ASSERT_EQ(GL_TRUE, linked);

  glUseProgram(program);

  const GLfloat kCoefficients16[] = {1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,
                                     7.0f,  8.0f,  9.0f,  10.0f, 11.0f, 12.0f,
                                     13.0f, 14.0f, 15.0f, 16.0f};

  glProgramPathFragmentInputGenCHROMIUM(
      program, kColorLocation, GL_EYE_LINEAR_CHROMIUM, 4, kCoefficients16);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  glProgramPathFragmentInputGenCHROMIUM(
      program, kFragColorLocation, GL_EYE_LINEAR_CHROMIUM, 4, kCoefficients16);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());
}

TEST_P(CHROMIUMPathRenderingWithTexturingTest,
       BindFragmentInputConflictsDetection) {
  if (!IsApplicable())
    return;

  // clang-format off
  static const char* kVertexShaderSource = SHADER(
      attribute vec4 position;
      varying vec4 colorA;
      varying vec4 colorB;
      void main()
      {
         gl_Position = position;
         colorA = position + vec4(1);
         colorB = position + vec4(2);
      }
  );
  static const char* kFragmentShaderSource = SHADER(
      precision mediump float;
      varying vec4 colorA;
      varying vec4 colorB;
      void main()
      {
        gl_FragColor = colorA + colorB;
      }
  );
  // clang-format on
  const GLint kColorALocation = 3;
  const GLint kColorBLocation = 4;

  GLuint vertex_shader =
      GLTestHelper::LoadShader(GL_VERTEX_SHADER, kVertexShaderSource);
  GLuint fragment_shader =
      GLTestHelper::LoadShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);

  glBindFragmentInputLocationCHROMIUM(program, kColorALocation, "colorA");
  // Bind colorB to location a, causing conflicts, link should fail.
  glBindFragmentInputLocationCHROMIUM(program, kColorALocation, "colorB");
  glLinkProgram(program);
  GLint linked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  EXPECT_EQ(0, linked);

  // Bind colorB to location b, no conflicts, link should succeed.
  glBindFragmentInputLocationCHROMIUM(program, kColorBLocation, "colorB");
  glLinkProgram(program);
  linked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  EXPECT_EQ(1, linked);

  GLTestHelper::CheckGLError("no errors", __LINE__);
}

// Test binding with array variables, using zero indices. Tests that
// binding colorA[0] with explicit "colorA[0]" as well as "colorA" produces
// a correct location that can be used with PathProgramFragmentInputGen.
// For path rendering, colorA[0] is bound to a location. The input generator for
// the location is set to produce vec4(0, 0.1, 0, 0.1).
// The default varying, color, is bound to a location and its generator
// will produce vec4(10.0).  The shader program produces green pixels.
// For vertex-based rendering, the vertex shader produces the same effect as
// the input generator for path rendering.
TEST_P(CHROMIUMPathRenderingWithTexturingTest,
       BindFragmentInputSimpleArrayHandling) {
  if (!IsApplicable())
    return;

  // clang-format off
  static const char* kVertexShaderSource = SHADER(
      uniform mat4 view_matrix;
      uniform mat4 color_matrix;
      uniform vec2 model_translate;
      attribute vec2 position;
      varying vec4 color;

      varying vec4 colorA[4];
      void main()
      {
        vec4 p = vec4(model_translate + position, 1, 1);
        gl_Position = view_matrix * p;
        colorA[0] = vec4(0.0, 0.1, 0, 0.1);
        colorA[1] = vec4(0.2);
        colorA[2] = vec4(0.3);
        colorA[3] = vec4(0.4);
        color = vec4(10.0);
      }
  );
  static const char* kFragmentShaderSource = SHADER(
      precision mediump float;
      varying vec4 color;

      varying vec4 colorA[4];
      void main()
      {
        gl_FragColor = colorA[0] * color;
      }
  );
  // clang-format on
  const GLint kColorA0Location = 4;
  const GLint kUnusedLocation = 5;
  const GLfloat kColorA0[] = {0.0f, 0.1f, 0.0f, 0.1f};
  const GLfloat kColor[] = {10.0f, 10.0f, 10.0f, 10.0f};

  for (int pass = 0; pass < 2; ++pass) {
    SetupProgramForTestPattern(kVertexShaderSource, kFragmentShaderSource);
    if (pass == 0) {
      glBindFragmentInputLocationCHROMIUM(program_, kUnusedLocation,
                                          "colorA[0]");
      glBindFragmentInputLocationCHROMIUM(program_, kColorA0Location, "colorA");
    } else {
      glBindFragmentInputLocationCHROMIUM(program_, kUnusedLocation, "colorA");
      glBindFragmentInputLocationCHROMIUM(program_, kColorA0Location,
                                          "colorA[0]");
    }
    LinkProgramForTestPattern();
    glProgramPathFragmentInputGenCHROMIUM(program_, kColorA0Location,
                                          GL_CONSTANT_CHROMIUM, 4, kColorA0);
    glProgramPathFragmentInputGenCHROMIUM(program_, kColorFragmentInputLocation,
                                          GL_CONSTANT_CHROMIUM, 4, kColor);

    DrawTestPattern();
    for (int j = 0; j < kTestRows; ++j) {
      for (int i = 0; i < kTestColumns; ++i) {
        for (size_t k = 0; k < arraysize(kFillCoords); k += 2) {
          SCOPED_TRACE(testing::Message() << "Verifying fill for shape " << i
                                          << ", " << j << " coord " << k);
          float fx = kFillCoords[k];
          float fy = kFillCoords[k + 1];
          float px = i * kShapeWidth;
          float py = j * kShapeHeight;

          uint8_t color[4] = {0, 255, 0, 255};

          EXPECT_TRUE(GLTestHelper::CheckPixels(px + fx, py + fy, 1, 1, 2,
                                                color, nullptr));
        }
      }
    }
    TeardownStateForTestPattern();
  }

  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
}

// Test binding with non-zero indices.
// Currently this is disabled, as the drivers seem to have a bug with the
// behavior.
TEST_P(CHROMIUMPathRenderingWithTexturingTest,
       DISABLED_BindFragmentInputArrayHandling) {
  if (!IsApplicable())
    return;

  // clang-format off
  static const char* kVertexShaderSource = SHADER(
      uniform mat4 view_matrix;
      uniform mat4 color_matrix;
      uniform vec2 model_translate;
      attribute vec2 position;
      varying vec4 color;

      varying vec4 colorA[4];
      void main()
      {
        vec4 p = vec4(model_translate + position, 1, 1);
        gl_Position = view_matrix * p;

        colorA[0] = vec4(0, 0.1, 0, 0.1);
        colorA[1] = vec4(0, 1, 0, 1);
        colorA[2] = vec4(0, 0.8, 0, 0.8);
        colorA[3] = vec4(0, 0.5, 0, 0.5);
        color = vec4(0.2);
      }
  );
  static const char* kFragmentShaderSource = SHADER(
      precision mediump float;
      varying vec4 colorA[4];
      varying vec4 color;
      void main()
      {
        gl_FragColor = (colorA[0] * colorA[1]) +
            colorA[2] + (colorA[3] * color);
      }
  );
  // clang-format on
  const GLint kColorA0Location = 4;
  const GLint kColorA1Location = 1;
  const GLint kColorA2Location = 2;
  const GLint kColorA3Location = 3;
  const GLint kUnusedLocation = 5;
  const GLfloat kColorA0[] = {0.0f, 0.1f, 0.0f, 0.1f};
  const GLfloat kColorA1[] = {0.0f, 1.0f, 0.0f, 1.0f};
  const GLfloat kColorA2[] = {0.0f, 0.8f, 0.0f, 0.8f};
  const GLfloat kColorA3[] = {0.0f, 0.5f, 0.0f, 0.5f};
  const GLfloat kColor[] = {0.2f, 0.2f, 0.2f, 0.2f};

  SetupProgramForTestPattern(kVertexShaderSource, kFragmentShaderSource);
  glBindFragmentInputLocationCHROMIUM(program_, kUnusedLocation, "colorA[0]");
  glBindFragmentInputLocationCHROMIUM(program_, kColorA1Location, "colorA[1]");
  glBindFragmentInputLocationCHROMIUM(program_, kColorA2Location, "colorA[2]");
  glBindFragmentInputLocationCHROMIUM(program_, kColorA3Location, "colorA[3]");
  glBindFragmentInputLocationCHROMIUM(program_, kColorA0Location, "colorA");
  LinkProgramForTestPattern();

  glProgramPathFragmentInputGenCHROMIUM(program_, kColorA0Location,
                                        GL_CONSTANT_CHROMIUM, 4, kColorA0);
  glProgramPathFragmentInputGenCHROMIUM(program_, kColorA1Location,
                                        GL_CONSTANT_CHROMIUM, 4, kColorA1);
  glProgramPathFragmentInputGenCHROMIUM(program_, kColorA2Location,
                                        GL_CONSTANT_CHROMIUM, 4, kColorA2);
  glProgramPathFragmentInputGenCHROMIUM(program_, kColorA3Location,
                                        GL_CONSTANT_CHROMIUM, 4, kColorA3);
  glProgramPathFragmentInputGenCHROMIUM(program_, kColorFragmentInputLocation,
                                        GL_CONSTANT_CHROMIUM, 4, kColor);
  DrawTestPattern();

  for (int j = 0; j < kTestRows; ++j) {
    for (int i = 0; i < kTestColumns; ++i) {
      for (size_t k = 0; k < arraysize(kFillCoords); k += 2) {
        SCOPED_TRACE(testing::Message() << "Verifying fill for shape " << i
                                        << ", " << j << " coord " << k);
        float fx = kFillCoords[k];
        float fy = kFillCoords[k + 1];
        float px = i * kShapeWidth;
        float py = j * kShapeHeight;

        uint8_t color[4] = {0, 255, 0, 255};

        EXPECT_TRUE(GLTestHelper::CheckPixels(px + fx, py + fy, 1, 1, 2, color,
                                              nullptr));
      }
    }
  }

  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  TeardownStateForTestPattern();
}

TEST_P(CHROMIUMPathRenderingWithTexturingTest, UnusedFragmentInputUpdate) {
  if (!IsApplicable())
    return;

  // clang-format off
  static const char* kVertexShaderString = SHADER(
      attribute vec4 a_position;
      void main() {
        gl_Position = a_position;
      }
  );
  static const char* kFragmentShaderString = SHADER(
      precision mediump float;
      uniform vec4 u_colorA;
      uniform float u_colorU;
      uniform vec4 u_colorC;
      void main() {
        gl_FragColor = u_colorA + u_colorC;
      }
  );
  // clang-format on
  const GLint kColorULocation = 1;
  const GLint kNonexistingLocation = 5;
  const GLint kUnboundLocation = 6;

  GLuint vertex_shader =
      GLTestHelper::LoadShader(GL_VERTEX_SHADER, kVertexShaderString);
  GLuint fragment_shader =
      GLTestHelper::LoadShader(GL_FRAGMENT_SHADER, kFragmentShaderString);
  GLuint program = glCreateProgram();
  glBindFragmentInputLocationCHROMIUM(program, kColorULocation, "u_colorU");
  // The non-existing uniform should behave like existing, but optimized away
  // uniform.
  glBindFragmentInputLocationCHROMIUM(program, kNonexistingLocation,
                                      "nonexisting");
  // Let A and C be assigned automatic locations.
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  GLint linked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  EXPECT_EQ(1, linked);
  glUseProgram(program);

  GLfloat kColor[16] = {
      0.0f,
  };
  // No errors on bound locations, since caller does not know
  // if the driver optimizes them away or not.
  glProgramPathFragmentInputGenCHROMIUM(program, kColorULocation,
                                        GL_CONSTANT_CHROMIUM, 1, kColor);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // No errors on bound locations of names that do not exist
  // in the shader. Otherwise it would be inconsistent wrt the
  // optimization case.
  glProgramPathFragmentInputGenCHROMIUM(program, kNonexistingLocation,
                                        GL_CONSTANT_CHROMIUM, 1, kColor);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // The above are equal to updating -1.
  glProgramPathFragmentInputGenCHROMIUM(program, -1, GL_CONSTANT_CHROMIUM, 1,
                                        kColor);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // No errors when updating with other type either.
  // The type can not be known with the non-existing case.
  glProgramPathFragmentInputGenCHROMIUM(program, kColorULocation,
                                        GL_CONSTANT_CHROMIUM, 4, kColor);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  glProgramPathFragmentInputGenCHROMIUM(program, kNonexistingLocation,
                                        GL_CONSTANT_CHROMIUM, 4, kColor);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  glProgramPathFragmentInputGenCHROMIUM(program, -1, GL_CONSTANT_CHROMIUM, 4,
                                        kColor);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  // Updating an unbound, non-existing location still causes
  // an error.
  glProgramPathFragmentInputGenCHROMIUM(program, kUnboundLocation,
                                        GL_CONSTANT_CHROMIUM, 4, kColor);
  EXPECT_EQ(static_cast<GLenum>(GL_INVALID_OPERATION), glGetError());
}

INSTANTIATE_TEST_CASE_P(WithAndWithoutShaderNameMapping,
                        CHROMIUMPathRenderingWithTexturingTest,
                        ::testing::Bool());

}  // namespace gpu
