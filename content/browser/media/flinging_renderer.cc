// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/flinging_renderer.h"

#include "base/memory/ptr_util.h"
#include "content/browser/frame_host/render_frame_host_delegate.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/presentation_service_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_client.h"

namespace content {

FlingingRenderer::FlingingRenderer(std::unique_ptr<MediaController> controller)
    : controller_(std::move(controller)) {}

FlingingRenderer::~FlingingRenderer() = default;

// static
std::unique_ptr<FlingingRenderer> FlingingRenderer::Create(
    RenderFrameHost* render_frame_host,
    const std::string& presentation_id) {
  DVLOG(1) << __func__;

  ContentClient* content_client = GetContentClient();
  if (!content_client)
    return nullptr;

  ContentBrowserClient* browser_client = content_client->browser();
  if (!browser_client)
    return nullptr;

  ControllerPresentationServiceDelegate* presentation_delegate =
      browser_client->GetControllerPresentationServiceDelegate(
          static_cast<RenderFrameHostImpl*>(render_frame_host)
              ->delegate()
              ->GetAsWebContents());

  if (!presentation_delegate)
    return nullptr;

  auto media_controller = presentation_delegate->GetMediaController(
      render_frame_host->GetProcess()->GetID(),
      render_frame_host->GetRoutingID(), presentation_id);

  if (!media_controller)
    return nullptr;

  return base::WrapUnique<FlingingRenderer>(
      new FlingingRenderer(std::move(media_controller)));
}

// media::Renderer implementation
void FlingingRenderer::Initialize(media::MediaResource* media_resource,
                                  media::RendererClient* client,
                                  const media::PipelineStatusCB& init_cb) {
  DVLOG(2) << __func__;
  init_cb.Run(media::PIPELINE_OK);
}

void FlingingRenderer::SetCdm(media::CdmContext* cdm_context,
                              const media::CdmAttachedCB& cdm_attached_cb) {
  // The flinging renderer does not support playing encrypted content.
  NOTREACHED();
}

void FlingingRenderer::Flush(const base::Closure& flush_cb) {
  DVLOG(2) << __func__;
  // There is nothing to reset, we can no-op the call.
  flush_cb.Run();
}

void FlingingRenderer::StartPlayingFrom(base::TimeDelta time) {
  DVLOG(2) << __func__;
  controller_->Seek(time);
  controller_->Play();
}

void FlingingRenderer::SetPlaybackRate(double playback_rate) {
  DVLOG(2) << __func__;
  if (playback_rate == 0)
    controller_->Pause();
  else
    controller_->Play();
}

void FlingingRenderer::SetVolume(float volume) {
  DVLOG(2) << __func__;
  controller_->SetVolume(volume);
}

base::TimeDelta FlingingRenderer::GetMediaTime() {
  // TODO(https://crbug.com/830871): return correct media time.
  return base::TimeDelta();
}

}  // namespace content
