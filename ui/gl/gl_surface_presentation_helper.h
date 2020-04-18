// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_SURFACE_PRESENTATION_HELPER_H_
#define UI_GL_GL_SURFACE_PRESENTATION_HELPER_H_

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "ui/gfx/swap_result.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_surface.h"

namespace gfx {
class VSyncProvider;
}

namespace gl {

class GLContext;
class GPUTimingClient;
class GPUTimer;

// Helper class for managering and invoke presentation callbacks for GLSurface
// implementations.
class GL_EXPORT GLSurfacePresentationHelper {
 public:
  class GL_EXPORT ScopedSwapBuffers {
   public:
    ScopedSwapBuffers(GLSurfacePresentationHelper* helper,
                      const GLSurface::PresentationCallback& callback);
    ~ScopedSwapBuffers();

    void set_result(gfx::SwapResult result) { result_ = result; }
    gfx::SwapResult result() const { return result_; }

   private:
    GLSurfacePresentationHelper* const helper_;
    gfx::SwapResult result_ = gfx::SwapResult::SWAP_ACK;

    DISALLOW_COPY_AND_ASSIGN(ScopedSwapBuffers);
  };

  explicit GLSurfacePresentationHelper(gfx::VSyncProvider* vsync_provider);

  // For using fixed VSync provider.
  GLSurfacePresentationHelper(const base::TimeTicks timebase,
                              const base::TimeDelta interval);
  ~GLSurfacePresentationHelper();

  void OnMakeCurrent(GLContext* context, GLSurface* surface);
  void PreSwapBuffers(const GLSurface::PresentationCallback& callback);
  void PostSwapBuffers(gfx::SwapResult result);

 private:
  struct Frame {
    Frame(Frame&& other);
    Frame(std::unique_ptr<GPUTimer>&& timer,
          const GLSurface::PresentationCallback& callback);
    ~Frame();
    Frame& operator=(Frame&& other);
    std::unique_ptr<GPUTimer> timer;
    GLSurface::PresentationCallback callback;
    gfx::SwapResult result = gfx::SwapResult::SWAP_ACK;
  };

  // Check |pending_frames_| and run presentation callbacks.
  void CheckPendingFrames();

  // Callback used by PostDelayedTask for running CheckPendingFrames().
  void CheckPendingFramesCallback();

  void UpdateVSyncCallback(const base::TimeTicks timebase,
                           const base::TimeDelta interval);

  void ScheduleCheckPendingFrames(bool align_with_next_vsync);

  gfx::VSyncProvider* const vsync_provider_;
  scoped_refptr<GLContext> gl_context_;
  GLSurface* surface_ = nullptr;
  scoped_refptr<GPUTimingClient> gpu_timing_client_;
  base::circular_deque<Frame> pending_frames_;
  base::TimeTicks vsync_timebase_;
  base::TimeDelta vsync_interval_;
  bool check_pending_frame_scheduled_ = false;

  base::WeakPtrFactory<GLSurfacePresentationHelper> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfacePresentationHelper);
};

}  // namespace gl

#endif  // UI_GL_GL_SURFACE_PRESENTATION_HELPER_H_
