// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_SURFACE_TEXTURE_GL_OWNER_H_
#define MEDIA_GPU_ANDROID_SURFACE_TEXTURE_GL_OWNER_H_

#include "media/gpu/android/texture_owner.h"

#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "media/gpu/media_gpu_export.h"
#include "ui/gl/android/surface_texture.h"

namespace media {

struct FrameAvailableEvent;

class MEDIA_GPU_EXPORT SurfaceTextureGLOwner : public TextureOwner {
 public:
  // Creates a GL texture using the current platform GL context and returns a
  // new SurfaceTextureGLOwner attached to it. Returns null on failure.
  static scoped_refptr<TextureOwner> Create();

  GLuint GetTextureId() const override;
  gl::GLContext* GetContext() const override;
  gl::GLSurface* GetSurface() const override;
  gl::ScopedJavaSurface CreateJavaSurface() const override;
  void UpdateTexImage() override;
  void GetTransformMatrix(float mtx[16]) override;
  void ReleaseBackBuffers() override;
  void SetReleaseTimeToNow() override;
  void IgnorePendingRelease() override;
  bool IsExpectingFrameAvailable() override;
  void WaitForFrameAvailable() override;

 private:
  SurfaceTextureGLOwner(GLuint texture_id);
  ~SurfaceTextureGLOwner() override;

  scoped_refptr<gl::SurfaceTexture> surface_texture_;
  GLuint texture_id_;

  // The context and surface that were used to create |texture_id_|.
  scoped_refptr<gl::GLContext> context_;
  scoped_refptr<gl::GLSurface> surface_;

  // When SetReleaseTimeToNow() was last called. i.e., when the last
  // codec buffer was released to this surface. Or null if
  // IgnorePendingRelease() or WaitForFrameAvailable() have been called since.
  base::TimeTicks release_time_;
  scoped_refptr<FrameAvailableEvent> frame_available_event_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(SurfaceTextureGLOwner);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_SURFACE_TEXTURE_GL_OWNER_H_
