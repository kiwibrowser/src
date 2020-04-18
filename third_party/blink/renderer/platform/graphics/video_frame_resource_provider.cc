// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/video_frame_resource_provider.h"

#include <memory>
#include "base/bind.h"
#include "base/trace_event/trace_event.h"
#include "cc/resources/layer_tree_resource_provider.h"
#include "cc/resources/video_resource_updater.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "components/viz/common/quads/texture_draw_quad.h"
#include "components/viz/common/quads/yuv_video_draw_quad.h"
#include "media/base/video_frame.h"

namespace blink {

VideoFrameResourceProvider::VideoFrameResourceProvider(
    const cc::LayerTreeSettings& settings)
    : settings_(settings) {}

VideoFrameResourceProvider::~VideoFrameResourceProvider() = default;

void VideoFrameResourceProvider::Initialize(
    viz::ContextProvider* media_context_provider,
    viz::SharedBitmapReporter* shared_bitmap_reporter) {
  context_provider_ = media_context_provider;
  resource_provider_ = std::make_unique<cc::LayerTreeResourceProvider>(
      media_context_provider, true);

  int max_texture_size;
  if (context_provider_) {
    max_texture_size =
        context_provider_->ContextCapabilities().max_texture_size;
  } else {
    // Pick an arbitrary limit here similar to what hardware might.
    max_texture_size = 16 * 1024;
  }

  resource_updater_ = std::make_unique<cc::VideoResourceUpdater>(
      media_context_provider, shared_bitmap_reporter, resource_provider_.get(),
      settings_.use_stream_video_draw_quad,
      settings_.resource_settings.use_gpu_memory_buffer_resources,
      settings_.resource_settings.use_r16_texture, max_texture_size);
}

void VideoFrameResourceProvider::OnContextLost() {
  resource_updater_ = nullptr;
  resource_provider_ = nullptr;
  context_provider_ = nullptr;
}

void VideoFrameResourceProvider::AppendQuads(
    viz::RenderPass* render_pass,
    scoped_refptr<media::VideoFrame> frame,
    media::VideoRotation rotation) {
  TRACE_EVENT0("media", "VideoFrameResourceProvider::AppendQuads");
  DCHECK(resource_updater_);
  DCHECK(resource_provider_);

  gfx::Transform transform = gfx::Transform();
  gfx::Size rotated_size = frame->coded_size();

  switch (rotation) {
    case media::VIDEO_ROTATION_90:
      rotated_size = gfx::Size(rotated_size.height(), rotated_size.width());
      transform.Rotate(90.0);
      transform.Translate(0.0, -rotated_size.height());
      break;
    case media::VIDEO_ROTATION_180:
      transform.Rotate(180.0);
      transform.Translate(-rotated_size.width(), -rotated_size.height());
      break;
    case media::VIDEO_ROTATION_270:
      rotated_size = gfx::Size(rotated_size.height(), rotated_size.width());
      transform.Rotate(270.0);
      transform.Translate(-rotated_size.width(), 0);
      break;
    case media::VIDEO_ROTATION_0:
      break;
  }

  resource_updater_->ObtainFrameResources(frame);
  // TODO(lethalantidote) : update with true value;
  bool contents_opaque = true;
  gfx::Rect visible_layer_rect = gfx::Rect(rotated_size);
  gfx::Rect clip_rect = gfx::Rect(frame->coded_size());
  bool is_clipped = false;
  float draw_opacity = 1.0f;
  int sorting_context_id = 0;

  // Internal to this compositor frame, this video quad is never occluded,
  // thus the full quad is visible.
  gfx::Rect visible_quad_rect = gfx::Rect(rotated_size);

  resource_updater_->AppendQuads(render_pass, std::move(frame), transform,
                                 rotated_size, visible_layer_rect, clip_rect,
                                 is_clipped, contents_opaque, draw_opacity,
                                 sorting_context_id, visible_quad_rect);
}

void VideoFrameResourceProvider::ReleaseFrameResources() {
  resource_updater_->ReleaseFrameResources();
}

void VideoFrameResourceProvider::PrepareSendToParent(
    const std::vector<viz::ResourceId>& resource_ids,
    std::vector<viz::TransferableResource>* transferable_resources) {
  resource_provider_->PrepareSendToParent(resource_ids, transferable_resources,
                                          context_provider_);
}

void VideoFrameResourceProvider::ReceiveReturnsFromParent(
    const std::vector<viz::ReturnedResource>& transferable_resources) {
  resource_provider_->ReceiveReturnsFromParent(transferable_resources);
}

}  // namespace blink
