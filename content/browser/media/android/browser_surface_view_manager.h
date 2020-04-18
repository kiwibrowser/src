// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_ANDROID_BROWSER_SURFACE_VIEW_MANAGER_H_
#define CONTENT_BROWSER_MEDIA_ANDROID_BROWSER_SURFACE_VIEW_MANAGER_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "content/browser/android/content_video_view.h"
#include "content/common/content_export.h"
#include "ui/gfx/geometry/size.h"

namespace content {

class RenderFrameHost;

// BrowserSurfaceViewManager creates and owns a ContentVideoView on behalf of
// a fullscreen media player. Its SurfaceView is registered so that a decoder
// in the GPU process can look it up and render to it.
class CONTENT_EXPORT BrowserSurfaceViewManager final
    : public ContentVideoView::Client {
 public:
  explicit BrowserSurfaceViewManager(RenderFrameHost* render_frame_host);
  ~BrowserSurfaceViewManager();

  // ContentVideoView::Client overrides.
  void SetVideoSurface(gl::ScopedJavaSurface surface) override;
  void DidExitFullscreen(bool release_media_player) override;

  void OnCreateFullscreenSurface(const gfx::Size& video_natural_size);
  void OnNaturalSizeChanged(const gfx::Size& size);

 private:
  // Send a message to return the surface id to the caller.
  bool SendSurfaceID(int surface_id);

  // Synchronously notify the decoder that the surface is being destroyed so it
  // can stop rendering to it. This sends a message to the GPU process. Without
  // this, the MediaCodec decoder will start throwing IllegalStateException, and
  // crash on some devices (http://crbug.com/598408, http://crbug.com/600454).
  void SendDestroyingVideoSurface(int surface_id);

  RenderFrameHost* const render_frame_host_;

  // The surface id of the ContentVideoView surface.
  int surface_id_;

  // The fullscreen view that contains a SurfaceView.
  std::unique_ptr<ContentVideoView> content_video_view_;

  DISALLOW_COPY_AND_ASSIGN(BrowserSurfaceViewManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_ANDROID_BROWSER_SURFACE_VIEW_MANAGER_H_
