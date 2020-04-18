// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_compositor.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppapi/c/ppb_opengles2.h"
#include "ppapi/cpp/compositor.h"
#include "ppapi/cpp/compositor_layer.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/lib/gl/gles2/gl2ext_ppapi.h"
#include "ppapi/lib/gl/include/GLES2/gl2.h"
#include "ppapi/lib/gl/include/GLES2/gl2ext.h"
#include "ppapi/tests/test_utils.h"

namespace {

const float kMatrix[16] = {
  1.0f, 0.0f, 0.0f, 0.0f,
  0.0f, 1.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 1.0f, 0.0f,
  0.0f, 0.0f, 0.0f, 1.0f,
};

}  // namespace

REGISTER_TEST_CASE(Compositor);

#define VERIFY(r) do { \
    std::string result = (r); \
    if (!result.empty()) \
      return result; \
  } while (false)

bool TestCompositor::Init() {
  if (!CheckTestingInterface())
    return false;

  if (!glInitializePPAPI(pp::Module::Get()->get_browser_interface()))
    return false;

  return true;
}

void TestCompositor::RunTests(const std::string& filter) {
  RUN_CALLBACK_TEST(TestCompositor, Release, filter);
  RUN_CALLBACK_TEST(TestCompositor, ReleaseWithoutCommit, filter);
  RUN_CALLBACK_TEST(TestCompositor, CommitTwoTimesWithoutChange, filter);
  RUN_CALLBACK_TEST(TestCompositor, General, filter);

  RUN_CALLBACK_TEST(TestCompositor, ReleaseUnbound, filter);
  RUN_CALLBACK_TEST(TestCompositor, ReleaseWithoutCommitUnbound, filter);
  RUN_CALLBACK_TEST(TestCompositor, CommitTwoTimesWithoutChangeUnbound, filter);
  RUN_CALLBACK_TEST(TestCompositor, GeneralUnbound, filter);

  RUN_CALLBACK_TEST(TestCompositor, BindUnbind, filter);
}

std::string TestCompositor::TestRelease() {
  return TestReleaseInternal(true);
}

std::string TestCompositor::TestReleaseWithoutCommit() {
  return TestReleaseWithoutCommitInternal(true);
}

std::string TestCompositor::TestCommitTwoTimesWithoutChange() {
  return TestCommitTwoTimesWithoutChangeInternal(true);
}

std::string TestCompositor::TestGeneral() {
  return TestGeneralInternal(true);
}

std::string TestCompositor::TestReleaseUnbound() {
  return TestReleaseInternal(false);
}

std::string TestCompositor::TestReleaseWithoutCommitUnbound() {
  return TestReleaseWithoutCommitInternal(false);
}

std::string TestCompositor::TestCommitTwoTimesWithoutChangeUnbound() {
  return TestCommitTwoTimesWithoutChangeInternal(false);
}

std::string TestCompositor::TestGeneralUnbound() {
  return TestGeneralInternal(false);
}

// TODO(penghuang): refactor common part in all tests to a member function.
std::string TestCompositor::TestBindUnbind() {
  // Setup GLES2
  const int32_t attribs[] = {
    PP_GRAPHICS3DATTRIB_WIDTH, 16,
    PP_GRAPHICS3DATTRIB_HEIGHT, 16,
    PP_GRAPHICS3DATTRIB_NONE
  };
  pp::Graphics3D graphics_3d(instance_, attribs);
  ASSERT_FALSE(graphics_3d.is_null());
  glSetCurrentContextPPAPI(graphics_3d.pp_resource());

  pp::Compositor compositor = pp::Compositor(instance_);
  ASSERT_FALSE(compositor.is_null());

  // Add layers on an unbound compositor.
  pp::CompositorLayer color_layer = compositor.AddLayer();
  ASSERT_FALSE(color_layer.is_null());

  VERIFY(SetColorLayer(color_layer, PP_OK));

  uint32_t texture = 0;
  VERIFY(CreateTexture(&texture));
  pp::CompositorLayer texture_layer = compositor.AddLayer();
  ASSERT_FALSE(texture_layer.is_null());
  TestCompletionCallback texture_release_callback(instance_->pp_instance(),
                                                  PP_REQUIRED);
  ASSERT_EQ(PP_OK_COMPLETIONPENDING,
            texture_layer.SetTexture(graphics_3d, GL_TEXTURE_2D, texture,
                                     pp::Size(100, 100),
                                     texture_release_callback.GetCallback()));

  pp::ImageData image;
  VERIFY(CreateImage(&image));
  pp::CompositorLayer image_layer = compositor.AddLayer();
  TestCompletionCallback image_release_callback(instance_->pp_instance(),
                                                PP_REQUIRED);
  ASSERT_EQ(PP_OK_COMPLETIONPENDING,
            image_layer.SetImage(image, pp::Size(100, 100),
                                 image_release_callback.GetCallback()));

  // Commit layers to the chromium compositor.
  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(compositor.CommitLayers(callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  // Bind the compositor and call CommitLayers() again.
  ASSERT_TRUE(instance_->BindGraphics(compositor));
  callback.WaitForResult(compositor.CommitLayers(callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  // Unbind the compositor and call CommitLayers() again.
  ASSERT_TRUE(instance_->BindGraphics(pp::Compositor()));
  callback.WaitForResult(compositor.CommitLayers(callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  // Reset layers and call CommitLayers() again.
  ASSERT_EQ(PP_OK, compositor.ResetLayers());
  callback.WaitForResult(compositor.CommitLayers(callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  texture_release_callback.WaitForResult(PP_OK_COMPLETIONPENDING);
  ASSERT_EQ(PP_OK, texture_release_callback.result());
  ReleaseTexture(texture);

  image_release_callback.WaitForResult(PP_OK_COMPLETIONPENDING);
  ASSERT_EQ(PP_OK, image_release_callback.result());

  // Reset
  glSetCurrentContextPPAPI(0);

  PASS();
}

std::string TestCompositor::TestReleaseInternal(bool bind) {
  // Setup GLES2
  const int32_t attribs[] = {
    PP_GRAPHICS3DATTRIB_WIDTH, 16,
    PP_GRAPHICS3DATTRIB_HEIGHT, 16,
    PP_GRAPHICS3DATTRIB_NONE
  };
  pp::Graphics3D graphics_3d(instance_, attribs);
  ASSERT_FALSE(graphics_3d.is_null());
  glSetCurrentContextPPAPI(graphics_3d.pp_resource());

  pp::Compositor compositor = pp::Compositor(instance_);
  ASSERT_FALSE(compositor.is_null());

  // Bind the compositor to the instance
  if (bind)
    ASSERT_TRUE(instance_->BindGraphics(compositor));

  pp::CompositorLayer color_layer = compositor.AddLayer();
  ASSERT_FALSE(color_layer.is_null());

  VERIFY(SetColorLayer(color_layer, PP_OK));

  uint32_t texture = 0;
  VERIFY(CreateTexture(&texture));
  pp::CompositorLayer texture_layer = compositor.AddLayer();
  ASSERT_FALSE(texture_layer.is_null());
  TestCompletionCallback texture_release_callback(instance_->pp_instance(),
                                                  PP_REQUIRED);
  ASSERT_EQ(PP_OK_COMPLETIONPENDING,
            texture_layer.SetTexture(graphics_3d, GL_TEXTURE_2D, texture,
                                     pp::Size(100, 100),
                                     texture_release_callback.GetCallback()));

  pp::ImageData image;
  VERIFY(CreateImage(&image));
  pp::CompositorLayer image_layer = compositor.AddLayer();
  TestCompletionCallback image_release_callback(instance_->pp_instance(),
                                                PP_REQUIRED);
  ASSERT_EQ(PP_OK_COMPLETIONPENDING,
            image_layer.SetImage(image, pp::Size(100, 100),
                                 image_release_callback.GetCallback()));

  // Commit layers to the chromium compositor.
  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(compositor.CommitLayers(callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  // Release the compositor, and then release_callback will be aborted.
  compositor = pp::Compositor();

  texture_release_callback.WaitForResult(PP_OK_COMPLETIONPENDING);
  ASSERT_EQ(PP_ERROR_ABORTED, texture_release_callback.result());
  ReleaseTexture(texture);

  image_release_callback.WaitForResult(PP_OK_COMPLETIONPENDING);
  ASSERT_EQ(PP_ERROR_ABORTED, image_release_callback.result());

  // Reset
  glSetCurrentContextPPAPI(0);

  PASS();
}

std::string TestCompositor::TestReleaseWithoutCommitInternal(bool bind) {
  // Setup GLES2
  const int32_t attribs[] = {
    PP_GRAPHICS3DATTRIB_WIDTH, 16,
    PP_GRAPHICS3DATTRIB_HEIGHT, 16,
    PP_GRAPHICS3DATTRIB_NONE
  };
  pp::Graphics3D graphics_3d(instance_, attribs);
  ASSERT_FALSE(graphics_3d.is_null());
  glSetCurrentContextPPAPI(graphics_3d.pp_resource());

  pp::Compositor compositor = pp::Compositor(instance_);
  ASSERT_FALSE(compositor.is_null());

  // Bind the compositor to the instance
  if (bind)
    ASSERT_TRUE(instance_->BindGraphics(compositor));

  pp::CompositorLayer color_layer = compositor.AddLayer();
  ASSERT_FALSE(color_layer.is_null());

  VERIFY(SetColorLayer(color_layer, PP_OK));

  uint32_t texture = 0;
  VERIFY(CreateTexture(&texture));
  pp::CompositorLayer texture_layer = compositor.AddLayer();
  ASSERT_FALSE(texture_layer.is_null());
  TestCompletionCallback texture_release_callback(instance_->pp_instance(),
                                                  PP_REQUIRED);
  ASSERT_EQ(PP_OK_COMPLETIONPENDING,
            texture_layer.SetTexture(graphics_3d, GL_TEXTURE_2D, texture,
                                     pp::Size(100, 100),
                                     texture_release_callback.GetCallback()));

  pp::ImageData image;
  VERIFY(CreateImage(&image));
  pp::CompositorLayer image_layer = compositor.AddLayer();
  TestCompletionCallback image_release_callback(instance_->pp_instance(),
                                                PP_REQUIRED);
  ASSERT_EQ(PP_OK_COMPLETIONPENDING,
            image_layer.SetImage(image, pp::Size(100, 100),
                                 image_release_callback.GetCallback()));

  // Release the compositor, and then release_callback will be aborted.
  compositor = pp::Compositor();

  // All release_callbacks should be called.
  texture_release_callback.WaitForResult(PP_OK_COMPLETIONPENDING);
  ASSERT_EQ(PP_ERROR_ABORTED, texture_release_callback.result());
  ReleaseTexture(texture);

  image_release_callback.WaitForResult(PP_OK_COMPLETIONPENDING);
  ASSERT_EQ(PP_ERROR_ABORTED, image_release_callback.result());

  // The layer associated to the compositor will become invalidated.
  VERIFY(SetColorLayer(color_layer, PP_ERROR_BADRESOURCE));

  // Reset
  glSetCurrentContextPPAPI(0);

  PASS();
}

std::string TestCompositor::TestCommitTwoTimesWithoutChangeInternal(bool bind) {
  pp::Compositor compositor(instance_);
  ASSERT_FALSE(compositor.is_null());
  if (bind)
    ASSERT_TRUE(instance_->BindGraphics(compositor));
  pp::CompositorLayer layer = compositor.AddLayer();
  ASSERT_FALSE(layer.is_null());
  VERIFY(SetColorLayer(layer, PP_OK));

  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(compositor.CommitLayers(callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  // CommitLayers() without any change.
  callback.WaitForResult(compositor.CommitLayers(callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  PASS();
}

std::string TestCompositor::TestGeneralInternal(bool bind) {
  // Setup GLES2
  const int32_t attribs[] = {
    PP_GRAPHICS3DATTRIB_WIDTH, 16,
    PP_GRAPHICS3DATTRIB_HEIGHT, 16,
    PP_GRAPHICS3DATTRIB_NONE
  };
  pp::Graphics3D graphics_3d(instance_, attribs);
  ASSERT_FALSE(graphics_3d.is_null());
  glSetCurrentContextPPAPI(graphics_3d.pp_resource());

  // All functions should work with a bound compositor
  pp::Compositor compositor(instance_);
  ASSERT_FALSE(compositor.is_null());
  if (bind)
    ASSERT_TRUE(instance_->BindGraphics(compositor));

  pp::CompositorLayer color_layer = compositor.AddLayer();
  ASSERT_FALSE(color_layer.is_null());
  VERIFY(SetColorLayer(color_layer, PP_OK));

  uint32_t texture = 0;
  VERIFY(CreateTexture(&texture));
  pp::CompositorLayer texture_layer = compositor.AddLayer();
  ASSERT_FALSE(texture_layer.is_null());
  TestCompletionCallback texture_release_callback(instance_->pp_instance(),
                                                  PP_REQUIRED);
  ASSERT_EQ(PP_OK_COMPLETIONPENDING,
            texture_layer.SetTexture(graphics_3d, texture, GL_TEXTURE_2D,
                                     pp::Size(100, 100),
                                     texture_release_callback.GetCallback()));

  pp::ImageData image;
  VERIFY(CreateImage(&image));
  pp::CompositorLayer image_layer = compositor.AddLayer();
  TestCompletionCallback image_release_callback(instance_->pp_instance(),
                                                PP_REQUIRED);
  ASSERT_EQ(PP_OK_COMPLETIONPENDING,
            image_layer.SetImage(image, pp::Size(100, 100),
                                 image_release_callback.GetCallback()));

  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(compositor.CommitLayers(callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  // After ResetLayers(), all layers should be invalidated.
  ASSERT_EQ(PP_OK, compositor.ResetLayers());
  VERIFY(SetColorLayer(color_layer, PP_ERROR_BADRESOURCE));

  // Commit empty layer stack to the chromium compositor, and then the texture
  // and the image will be released by the chromium compositor soon.
  callback.WaitForResult(compositor.CommitLayers(callback.GetCallback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  texture_release_callback.WaitForResult(PP_OK_COMPLETIONPENDING);
  ASSERT_EQ(PP_OK, texture_release_callback.result());
  ReleaseTexture(texture);

  image_release_callback.WaitForResult(PP_OK_COMPLETIONPENDING);
  ASSERT_EQ(PP_OK, image_release_callback.result());

  // Reset
  glSetCurrentContextPPAPI(0);

  PASS();
}

std::string TestCompositor::CreateTexture(uint32_t* texture) {
  glGenTextures(1, texture);
  ASSERT_NE(0, *texture);
  glBindTexture(GL_TEXTURE_2D, *texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 400, 400, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  return std::string();
}

std::string TestCompositor::ReleaseTexture(uint32_t texture) {
  ASSERT_NE(0u, texture);
  glDeleteTextures(1, &texture);

  return std::string();
}

std::string TestCompositor::CreateImage(pp::ImageData* image) {
  *image = pp::ImageData(instance_, PP_IMAGEDATAFORMAT_RGBA_PREMUL,
                         pp::Size(400, 400), false);
  ASSERT_FALSE(image->is_null());

  return std::string();
}

std::string TestCompositor::SetColorLayer(
    pp::CompositorLayer layer, int32_t result) {
  ASSERT_EQ(result, layer.SetColor(255, 255, 255, 255, pp::Size(100, 100)));
  ASSERT_EQ(result, layer.SetClipRect(pp::Rect(0, 0, 50, 50)));
  ASSERT_EQ(result, layer.SetTransform(kMatrix));
  ASSERT_EQ(result, layer.SetOpacity(128));

  return std::string();
}


