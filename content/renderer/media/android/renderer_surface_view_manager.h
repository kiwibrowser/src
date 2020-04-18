// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_ANDROID_RENDERER_SURFACE_VIEW_MANAGER_H_
#define CONTENT_RENDERER_MEDIA_ANDROID_RENDERER_SURFACE_VIEW_MANAGER_H_

#include "base/callback.h"
#include "content/common/content_export.h"
#include "content/public/renderer/render_frame_observer.h"
#include "media/base/surface_manager.h"
#include "ui/gfx/geometry/size.h"

namespace content {

// RendererSurfaceViewManager creates delegates requests for video SurfaceViews
// to BrowserSurfaceViewManager. The returned surface ids may be invalidated
// at any time (e.g., when the app is backgrounded), so clients should handle
// this gracefully. It should be created and used on a single thread only.
class CONTENT_EXPORT RendererSurfaceViewManager : public media::SurfaceManager,
                                                  public RenderFrameObserver {
 public:
  explicit RendererSurfaceViewManager(RenderFrame* render_frame);
  ~RendererSurfaceViewManager() override;

  // RenderFrameObserver override.
  bool OnMessageReceived(const IPC::Message& msg) override;

  // SurfaceManager overrides.
  void CreateFullscreenSurface(
      const gfx::Size& video_natural_size,
      const media::SurfaceCreatedCB& surface_created_cb) override;
  void NaturalSizeChanged(const gfx::Size& size) override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  void OnFullscreenSurfaceCreated(int surface_id);

  // Set when a surface request is in progress.
  media::SurfaceCreatedCB pending_surface_created_cb_;

  DISALLOW_COPY_AND_ASSIGN(RendererSurfaceViewManager);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_ANDROID_RENDERER_SURFACE_VIEW_MANAGER_H_
