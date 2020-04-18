// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/surface_texture_gl_owner.h"

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context_egl.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_share_group.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/init/gl_factory.h"

using testing::Invoke;
using testing::NiceMock;
using testing::_;

namespace media {

class SurfaceTextureGLOwnerTest : public testing::Test {
 public:
  SurfaceTextureGLOwnerTest() {}
  ~SurfaceTextureGLOwnerTest() override {}

 protected:
  void SetUp() override {
    gl::init::InitializeGLOneOffImplementation(gl::kGLImplementationEGLGLES2,
                                               false, false, false, true);
    surface_ = new gl::PbufferGLSurfaceEGL(gfx::Size(320, 240));
    surface_->Initialize();

    share_group_ = new gl::GLShareGroup();
    context_ = new gl::GLContextEGL(share_group_.get());
    context_->Initialize(surface_.get(), gl::GLContextAttribs());
    ASSERT_TRUE(context_->MakeCurrent(surface_.get()));

    surface_texture_ = SurfaceTextureGLOwner::Create();
    texture_id_ = surface_texture_->GetTextureId();
    // Bind and un-bind the texture, since that's required for glIsTexture to
    // return true.
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id_);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    ASSERT_TRUE(glIsTexture(texture_id_));
  }

  void TearDown() override {
    surface_texture_ = nullptr;
    context_ = nullptr;
    share_group_ = nullptr;
    surface_ = nullptr;
    gl::init::ShutdownGL(false);
  }

  scoped_refptr<TextureOwner> surface_texture_;
  GLuint texture_id_ = 0;

  scoped_refptr<gl::GLContext> context_;
  scoped_refptr<gl::GLShareGroup> share_group_;
  scoped_refptr<gl::GLSurface> surface_;
  base::MessageLoop message_loop_;
};

// Verify that SurfaceTextureGLOwner creates a bindable GL texture, and deletes
// it during destruction.
TEST_F(SurfaceTextureGLOwnerTest, GLTextureIsCreatedAndDestroyed) {
  // |texture_id| should not work anymore after we delete |surface_texture|.
  surface_texture_ = nullptr;
  ASSERT_FALSE(glIsTexture(texture_id_));
}

// Calling ReleaseBackBuffers shouldn't deallocate the texture handle.
TEST_F(SurfaceTextureGLOwnerTest, ReleaseDoesntDestroyTexture) {
  surface_texture_->ReleaseBackBuffers();
  ASSERT_TRUE(glIsTexture(texture_id_));
}

// Make sure that |surface_texture_| remembers the correct context and surface.
TEST_F(SurfaceTextureGLOwnerTest, ContextAndSurfaceAreCaptured) {
  ASSERT_EQ(context_, surface_texture_->GetContext());
  ASSERT_EQ(surface_, surface_texture_->GetSurface());
}

// Verify that destruction works even if some other context is current.
TEST_F(SurfaceTextureGLOwnerTest, DestructionWorksWithWrongContext) {
  scoped_refptr<gl::GLSurface> new_surface(
      new gl::PbufferGLSurfaceEGL(gfx::Size(320, 240)));
  new_surface->Initialize();

  scoped_refptr<gl::GLShareGroup> new_share_group(new gl::GLShareGroup());
  scoped_refptr<gl::GLContext> new_context(
      new gl::GLContextEGL(new_share_group.get()));
  new_context->Initialize(new_surface.get(), gl::GLContextAttribs());
  ASSERT_TRUE(new_context->MakeCurrent(new_surface.get()));

  surface_texture_ = nullptr;
  ASSERT_FALSE(glIsTexture(texture_id_));

  // |new_context| should still be current.
  ASSERT_TRUE(new_context->IsCurrent(new_surface.get()));

  new_context = nullptr;
  new_share_group = nullptr;
  new_surface = nullptr;
}

}  // namespace media
