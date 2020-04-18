// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/media_session_service_impl.h"

#include "content/browser/media/session/media_metadata_sanitizer.h"
#include "content/browser/media/session/media_session_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

MediaSessionServiceImpl::MediaSessionServiceImpl(
    RenderFrameHost* render_frame_host)
    : render_frame_process_id_(render_frame_host->GetProcess()->GetID()),
      render_frame_routing_id_(render_frame_host->GetRoutingID()),
      playback_state_(blink::mojom::MediaSessionPlaybackState::NONE) {
  MediaSessionImpl* session = GetMediaSession();
  if (session)
    session->OnServiceCreated(this);
}

MediaSessionServiceImpl::~MediaSessionServiceImpl() {
  MediaSessionImpl* session = GetMediaSession();
  if (session)
    session->OnServiceDestroyed(this);
}

// static
void MediaSessionServiceImpl::Create(
    RenderFrameHost* render_frame_host,
    blink::mojom::MediaSessionServiceRequest request) {
  MediaSessionServiceImpl* impl =
      new MediaSessionServiceImpl(render_frame_host);
  impl->Bind(std::move(request));
}

RenderFrameHost* MediaSessionServiceImpl::GetRenderFrameHost() {
  return RenderFrameHost::FromID(render_frame_process_id_,
                                 render_frame_routing_id_);
}

void MediaSessionServiceImpl::DidFinishNavigation() {
  // At this point the BrowsingContext of the frame has changed, so the members
  // need to be reset, and notify MediaSessionImpl.
  SetPlaybackState(blink::mojom::MediaSessionPlaybackState::NONE);
  SetMetadata(base::nullopt);
  ClearActions();
}

void MediaSessionServiceImpl::SetClient(
    blink::mojom::MediaSessionClientPtr client) {
  client_ = std::move(client);
}

void MediaSessionServiceImpl::SetPlaybackState(
    blink::mojom::MediaSessionPlaybackState state) {
  playback_state_ = state;
  MediaSessionImpl* session = GetMediaSession();
  if (session)
    session->OnMediaSessionPlaybackStateChanged(this);
}

void MediaSessionServiceImpl::SetMetadata(
    const base::Optional<content::MediaMetadata>& metadata) {
  // When receiving a MediaMetadata, the browser process can't trust that it is
  // coming from a known and secure source. It must be processed accordingly.
  if (metadata.has_value() &&
      !MediaMetadataSanitizer::CheckSanity(metadata.value())) {
    RenderFrameHost* rfh = GetRenderFrameHost();
    if (rfh) {
      rfh->GetProcess()->ShutdownForBadMessage(
          RenderProcessHost::CrashReportMode::GENERATE_CRASH_DUMP);
    }
    return;
  }
  metadata_ = metadata;

  MediaSessionImpl* session = GetMediaSession();
  if (session)
    session->OnMediaSessionMetadataChanged(this);
}

void MediaSessionServiceImpl::EnableAction(
    blink::mojom::MediaSessionAction action) {
  actions_.insert(action);
  MediaSessionImpl* session = GetMediaSession();
  if (session)
    session->OnMediaSessionActionsChanged(this);
}

void MediaSessionServiceImpl::DisableAction(
    blink::mojom::MediaSessionAction action) {
  actions_.erase(action);
  MediaSessionImpl* session = GetMediaSession();
  if (session)
    session->OnMediaSessionActionsChanged(this);
}

void MediaSessionServiceImpl::ClearActions() {
  actions_.clear();
  MediaSessionImpl* session = GetMediaSession();
  if (session)
    session->OnMediaSessionActionsChanged(this);
}

MediaSessionImpl* MediaSessionServiceImpl::GetMediaSession() {
  RenderFrameHost* rfh = GetRenderFrameHost();
  if (!rfh)
    return nullptr;

  WebContentsImpl* contents =
      static_cast<WebContentsImpl*>(WebContentsImpl::FromRenderFrameHost(rfh));
  if (!contents)
    return nullptr;

  return MediaSessionImpl::Get(contents);
}

void MediaSessionServiceImpl::Bind(
    blink::mojom::MediaSessionServiceRequest request) {
  binding_.reset(new mojo::Binding<blink::mojom::MediaSessionService>(
      this, std::move(request)));
}

}  // namespace content
