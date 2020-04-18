// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_frame_metadata_provider_impl.h"

#include "base/bind.h"
#include "content/browser/renderer_host/frame_token_message_queue.h"

namespace content {

RenderFrameMetadataProviderImpl::RenderFrameMetadataProviderImpl(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    FrameTokenMessageQueue* frame_token_message_queue)
    : task_runner_(task_runner),
      frame_token_message_queue_(frame_token_message_queue),
      render_frame_metadata_observer_client_binding_(this),
      weak_factory_(this) {}

RenderFrameMetadataProviderImpl::~RenderFrameMetadataProviderImpl() = default;

void RenderFrameMetadataProviderImpl::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void RenderFrameMetadataProviderImpl::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void RenderFrameMetadataProviderImpl::Bind(
    mojom::RenderFrameMetadataObserverClientRequest client_request,
    mojom::RenderFrameMetadataObserverPtr observer) {
  render_frame_metadata_observer_ptr_ = std::move(observer);
  render_frame_metadata_observer_client_binding_.Close();
  render_frame_metadata_observer_client_binding_.Bind(std::move(client_request),
                                                      task_runner_);
}

void RenderFrameMetadataProviderImpl::ReportAllFrameSubmissionsForTesting(
    bool enabled) {
  DCHECK(render_frame_metadata_observer_ptr_);
  render_frame_metadata_observer_ptr_->ReportAllFrameSubmissionsForTesting(
      enabled);
}

const cc::RenderFrameMetadata&
RenderFrameMetadataProviderImpl::LastRenderFrameMetadata() const {
  return last_render_frame_metadata_;
}

void RenderFrameMetadataProviderImpl::OnFrameTokenRenderFrameMetadataChanged(
    cc::RenderFrameMetadata metadata) {
  last_render_frame_metadata_ = std::move(metadata);
  for (Observer& observer : observers_)
    observer.OnRenderFrameMetadataChanged();
}

void RenderFrameMetadataProviderImpl::OnFrameTokenFrameSubmissionForTesting() {
  for (Observer& observer : observers_)
    observer.OnRenderFrameSubmission();
}

void RenderFrameMetadataProviderImpl::SetLastRenderFrameMetadataForTest(
    cc::RenderFrameMetadata metadata) {
  last_render_frame_metadata_ = metadata;
}

void RenderFrameMetadataProviderImpl::OnRenderFrameMetadataChanged(
    uint32_t frame_token,
    const cc::RenderFrameMetadata& metadata) {
  if (metadata.local_surface_id != last_local_surface_id_) {
    last_local_surface_id_ = metadata.local_surface_id;
    for (Observer& observer : observers_)
      observer.OnLocalSurfaceIdChanged(metadata);
  }

  // Both RenderFrameMetadataProviderImpl and FrameTokenMessageQueue are owned
  // by the same RenderWidgetHostImpl. During shutdown the queue is cleared
  // without running the callbacks.
  frame_token_message_queue_->EnqueueOrRunFrameTokenCallback(
      frame_token,
      base::BindOnce(&RenderFrameMetadataProviderImpl::
                         OnFrameTokenRenderFrameMetadataChanged,
                     weak_factory_.GetWeakPtr(), std::move(metadata)));
}

void RenderFrameMetadataProviderImpl::OnFrameSubmissionForTesting(
    uint32_t frame_token) {
  frame_token_message_queue_->EnqueueOrRunFrameTokenCallback(
      frame_token, base::BindOnce(&RenderFrameMetadataProviderImpl::
                                      OnFrameTokenFrameSubmissionForTesting,
                                  weak_factory_.GetWeakPtr()));
}

}  // namespace content
