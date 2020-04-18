// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/android/renderer_surface_view_manager.h"

#include "content/common/media/surface_view_manager_messages_android.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/geometry/size.h"

namespace content {

RendererSurfaceViewManager::RendererSurfaceViewManager(
    RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {}

RendererSurfaceViewManager::~RendererSurfaceViewManager() {}

bool RendererSurfaceViewManager::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RendererSurfaceViewManager, msg)
    IPC_MESSAGE_HANDLER(SurfaceViewManagerMsg_FullscreenSurfaceCreated,
                        OnFullscreenSurfaceCreated)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RendererSurfaceViewManager::CreateFullscreenSurface(
    const gfx::Size& video_natural_size,
    const media::SurfaceCreatedCB& surface_created_cb) {
  DCHECK(!surface_created_cb.is_null());
  pending_surface_created_cb_ = surface_created_cb;
  Send(new SurfaceViewManagerHostMsg_CreateFullscreenSurface(
      routing_id(), video_natural_size));
}

void RendererSurfaceViewManager::NaturalSizeChanged(const gfx::Size& size) {
  DVLOG(3) << __func__ << ": size: " << size.ToString();
  Send(new SurfaceViewManagerHostMsg_NaturalSizeChanged(routing_id(), size));
}

void RendererSurfaceViewManager::OnFullscreenSurfaceCreated(int surface_id) {
  DVLOG(3) << __func__ << ": surface_id: " << surface_id;
  if (!pending_surface_created_cb_.is_null()) {
    pending_surface_created_cb_.Run(surface_id);
    pending_surface_created_cb_.Reset();
  }
}

void RendererSurfaceViewManager::OnDestruct() {
  delete this;
}

}  // namespace content
