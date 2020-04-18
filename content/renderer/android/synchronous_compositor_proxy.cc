// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/android/synchronous_compositor_proxy.h"

#include "base/auto_reset.h"
#include "base/command_line.h"
#include "base/memory/shared_memory.h"
#include "content/common/android/sync_compositor_statics.h"
#include "content/common/input/sync_compositor_messages.h"
#include "content/public/common/content_switches.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_sender.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/skia_util.h"

namespace content {

SynchronousCompositorProxy::SynchronousCompositorProxy(
    ui::SynchronousInputHandlerProxy* input_handler_proxy)
    : input_handler_proxy_(input_handler_proxy),
      use_in_process_zero_copy_software_draw_(
          base::CommandLine::ForCurrentProcess()->HasSwitch(
              switches::kSingleProcess)),
      page_scale_factor_(0.f),
      min_page_scale_factor_(0.f),
      max_page_scale_factor_(0.f),
      need_animate_scroll_(false),
      need_invalidate_count_(0u),
      did_activate_pending_tree_count_(0u) {
  DCHECK(input_handler_proxy_);
}

SynchronousCompositorProxy::~SynchronousCompositorProxy() {
  // The LayerTreeFrameSink is destroyed/removed by the compositor before
  // shutting down everything.
  DCHECK_EQ(layer_tree_frame_sink_, nullptr);
  input_handler_proxy_->SetOnlySynchronouslyAnimateRootFlings(nullptr);
}

void SynchronousCompositorProxy::Init() {
  input_handler_proxy_->SetOnlySynchronouslyAnimateRootFlings(this);
}

void SynchronousCompositorProxy::SetLayerTreeFrameSink(
    SynchronousLayerTreeFrameSink* layer_tree_frame_sink) {
  DCHECK_NE(layer_tree_frame_sink_, layer_tree_frame_sink);
  DCHECK(layer_tree_frame_sink);
  if (layer_tree_frame_sink_) {
    layer_tree_frame_sink_->SetSyncClient(nullptr);
  }
  layer_tree_frame_sink_ = layer_tree_frame_sink;
  layer_tree_frame_sink_->SetSyncClient(this);
  LayerTreeFrameSinkCreated();
  if (begin_frame_paused_)
    layer_tree_frame_sink_->SetBeginFrameSourcePaused(true);
}

void SynchronousCompositorProxy::SetNeedsSynchronousAnimateInput() {
  if (compute_scroll_called_via_ipc_) {
    needs_begin_frame_for_animate_input_ = true;
    SendSetNeedsBeginFramesIfNeeded();
  } else {
    need_animate_scroll_ = true;
    Invalidate();
  }
}

void SynchronousCompositorProxy::UpdateRootLayerState(
    const gfx::ScrollOffset& total_scroll_offset,
    const gfx::ScrollOffset& max_scroll_offset,
    const gfx::SizeF& scrollable_size,
    float page_scale_factor,
    float min_page_scale_factor,
    float max_page_scale_factor) {
  if (total_scroll_offset_ != total_scroll_offset ||
      max_scroll_offset_ != max_scroll_offset ||
      scrollable_size_ != scrollable_size ||
      page_scale_factor_ != page_scale_factor ||
      min_page_scale_factor_ != min_page_scale_factor ||
      max_page_scale_factor_ != max_page_scale_factor) {
    total_scroll_offset_ = total_scroll_offset;
    max_scroll_offset_ = max_scroll_offset;
    scrollable_size_ = scrollable_size;
    page_scale_factor_ = page_scale_factor;
    min_page_scale_factor_ = min_page_scale_factor;
    max_page_scale_factor_ = max_page_scale_factor;

    SendAsyncRendererStateIfNeeded();
  }
}

void SynchronousCompositorProxy::Invalidate() {
  ++need_invalidate_count_;
  SendAsyncRendererStateIfNeeded();
}

void SynchronousCompositorProxy::DidActivatePendingTree() {
  ++did_activate_pending_tree_count_;
  SendAsyncRendererStateIfNeeded();
}

void SynchronousCompositorProxy::PopulateCommonParams(
    SyncCompositorCommonRendererParams* params) {
  params->version = ++version_;
  params->total_scroll_offset = total_scroll_offset_;
  params->max_scroll_offset = max_scroll_offset_;
  params->scrollable_size = scrollable_size_;
  params->page_scale_factor = page_scale_factor_;
  params->min_page_scale_factor = min_page_scale_factor_;
  params->max_page_scale_factor = max_page_scale_factor_;
  params->need_invalidate_count = need_invalidate_count_;
  params->did_activate_pending_tree_count = did_activate_pending_tree_count_;
  if (!compute_scroll_called_via_ipc_)
    params->need_animate_scroll = need_animate_scroll_;
}

void SynchronousCompositorProxy::DemandDrawHwAsync(
    const SyncCompositorDemandDrawHwParams& params) {
  DemandDrawHw(
      params,
      base::BindOnce(&SynchronousCompositorProxy::SendDemandDrawHwAsyncReply,
                     base::Unretained(this)));
}

void SynchronousCompositorProxy::DemandDrawHw(
    const SyncCompositorDemandDrawHwParams& params,
    DemandDrawHwCallback callback) {
  hardware_draw_reply_ = std::move(callback);

  if (layer_tree_frame_sink_) {
    layer_tree_frame_sink_->DemandDrawHw(params.viewport_size,
                                         params.viewport_rect_for_tile_priority,
                                         params.transform_for_tile_priority);
  }

  // Ensure that a response is always sent even if the reply hasn't
  // generated a compostior frame.
  if (hardware_draw_reply_) {
    SyncCompositorCommonRendererParams common_renderer_params;
    PopulateCommonParams(&common_renderer_params);
    // Did not swap.
    std::move(hardware_draw_reply_)
        .Run(common_renderer_params, 0u, 0u, base::nullopt);
  }
}

struct SynchronousCompositorProxy::SharedMemoryWithSize {
  base::SharedMemory shm;
  const size_t buffer_size;
  bool zeroed;

  SharedMemoryWithSize(base::SharedMemoryHandle shm_handle, size_t buffer_size)
      : shm(shm_handle, false), buffer_size(buffer_size), zeroed(true) {}
};

void SynchronousCompositorProxy::ZeroSharedMemory() {
  // It is possible for this to get called twice, eg. if draw is called before
  // the LayerTreeFrameSink is ready. Just ignore duplicated calls rather than
  // inventing a complicated system to avoid it.
  if (software_draw_shm_->zeroed)
    return;

  memset(software_draw_shm_->shm.memory(), 0, software_draw_shm_->buffer_size);
  software_draw_shm_->zeroed = true;
}

void SynchronousCompositorProxy::DemandDrawSw(
    const SyncCompositorDemandDrawSwParams& params,
    DemandDrawSwCallback callback) {
  software_draw_reply_ = std::move(callback);
  if (layer_tree_frame_sink_) {
    SkCanvas* sk_canvas_for_draw = SynchronousCompositorGetSkCanvas();
    if (use_in_process_zero_copy_software_draw_) {
      DCHECK(sk_canvas_for_draw);
      layer_tree_frame_sink_->DemandDrawSw(sk_canvas_for_draw);
    } else {
      DCHECK(!sk_canvas_for_draw);
      DoDemandDrawSw(params);
    }
  }

  // Ensure that a response is always sent even if the reply hasn't
  // generated a compostior frame.
  if (software_draw_reply_) {
    SyncCompositorCommonRendererParams common_renderer_params;
    PopulateCommonParams(&common_renderer_params);
    // Did not swap.
    std::move(software_draw_reply_)
        .Run(common_renderer_params, 0u, base::nullopt);
  }
}

void SynchronousCompositorProxy::DoDemandDrawSw(
    const SyncCompositorDemandDrawSwParams& params) {
  DCHECK(layer_tree_frame_sink_);
  DCHECK(software_draw_shm_->zeroed);
  software_draw_shm_->zeroed = false;

  SkImageInfo info =
      SkImageInfo::MakeN32Premul(params.size.width(), params.size.height());
  size_t stride = info.minRowBytes();
  size_t buffer_size = info.computeByteSize(stride);
  DCHECK_EQ(software_draw_shm_->buffer_size, buffer_size);

  SkBitmap bitmap;
  if (!bitmap.installPixels(info, software_draw_shm_->shm.memory(), stride))
    return;
  SkCanvas canvas(bitmap);
  canvas.clipRect(gfx::RectToSkRect(params.clip));
  canvas.concat(params.transform.matrix());

  layer_tree_frame_sink_->DemandDrawSw(&canvas);
}

void SynchronousCompositorProxy::SubmitCompositorFrame(
    uint32_t layer_tree_frame_sink_id,
    viz::CompositorFrame frame) {
  // Verify that exactly one of these is true.
  DCHECK(hardware_draw_reply_.is_null() ^ software_draw_reply_.is_null());
  SyncCompositorCommonRendererParams common_renderer_params;
  PopulateCommonParams(&common_renderer_params);

  if (hardware_draw_reply_) {
    std::move(hardware_draw_reply_)
        .Run(common_renderer_params, layer_tree_frame_sink_id,
             NextMetadataVersion(), std::move(frame));
  } else if (software_draw_reply_) {
    std::move(software_draw_reply_)
        .Run(common_renderer_params, NextMetadataVersion(),
             std::move(frame.metadata));
  } else {
    NOTREACHED();
  }
}

void SynchronousCompositorProxy::SendSetNeedsBeginFramesIfNeeded() {
  bool needs_begin_frames =
      needs_begin_frame_for_frame_sink_ || needs_begin_frame_for_animate_input_;
  if (browser_needs_begin_frame_state_ != needs_begin_frames)
    SendSetNeedsBeginFrames(needs_begin_frames);
  browser_needs_begin_frame_state_ = needs_begin_frames;
}

void SynchronousCompositorProxy::SetNeedsBeginFrames(bool needs_begin_frames) {
  needs_begin_frame_for_frame_sink_ = needs_begin_frames;
  SendSetNeedsBeginFramesIfNeeded();
}

void SynchronousCompositorProxy::SinkDestroyed() {
  layer_tree_frame_sink_ = nullptr;
}

void SynchronousCompositorProxy::ComputeScroll(base::TimeTicks animation_time) {
  compute_scroll_called_via_ipc_ = true;

  if (need_animate_scroll_) {
    need_animate_scroll_ = false;
    input_handler_proxy_->SynchronouslyAnimate(animation_time);
  }
}

void SynchronousCompositorProxy::SetBeginFrameSourcePaused(bool paused) {
  begin_frame_paused_ = paused;
  if (layer_tree_frame_sink_)
    layer_tree_frame_sink_->SetBeginFrameSourcePaused(paused);
}

void SynchronousCompositorProxy::BeginFrame(const viz::BeginFrameArgs& args) {
  if (needs_begin_frame_for_animate_input_) {
    needs_begin_frame_for_animate_input_ = false;
    input_handler_proxy_->SynchronouslyAnimate(args.frame_time);
  }
  if (needs_begin_frame_for_frame_sink_ && layer_tree_frame_sink_)
    layer_tree_frame_sink_->BeginFrame(args);

  SyncCompositorCommonRendererParams param;
  PopulateCommonParams(&param);
  SendBeginFrameResponse(param);
}

void SynchronousCompositorProxy::SetScroll(
    const gfx::ScrollOffset& new_total_scroll_offset) {
  if (total_scroll_offset_ == new_total_scroll_offset)
    return;
  total_scroll_offset_ = new_total_scroll_offset;
  input_handler_proxy_->SynchronouslySetRootScrollOffset(total_scroll_offset_);
}

void SynchronousCompositorProxy::SetMemoryPolicy(uint32_t bytes_limit) {
  if (!layer_tree_frame_sink_)
    return;
  layer_tree_frame_sink_->SetMemoryPolicy(bytes_limit);
}

void SynchronousCompositorProxy::ReclaimResources(
    uint32_t layer_tree_frame_sink_id,
    const std::vector<viz::ReturnedResource>& resources) {
  if (!layer_tree_frame_sink_)
    return;
  layer_tree_frame_sink_->ReclaimResources(layer_tree_frame_sink_id, resources);
}

void SynchronousCompositorProxy::SetSharedMemory(
    const SyncCompositorSetSharedMemoryParams& params,
    SetSharedMemoryCallback callback) {
  bool success = false;
  SyncCompositorCommonRendererParams common_renderer_params;
  if (base::SharedMemory::IsHandleValid(params.shm_handle)) {
    software_draw_shm_.reset(
        new SharedMemoryWithSize(params.shm_handle, params.buffer_size));
    if (software_draw_shm_->shm.Map(params.buffer_size)) {
      DCHECK(software_draw_shm_->shm.memory());
      PopulateCommonParams(&common_renderer_params);
      success = true;
    }
  }
  std::move(callback).Run(success, common_renderer_params);
}

void SynchronousCompositorProxy::ZoomBy(float zoom_delta,
                                        const gfx::Point& anchor,
                                        ZoomByCallback callback) {
  zoom_by_reply_ = std::move(callback);
  input_handler_proxy_->SynchronouslyZoomBy(zoom_delta, anchor);
  SyncCompositorCommonRendererParams common_renderer_params;
  PopulateCommonParams(&common_renderer_params);
  std::move(zoom_by_reply_).Run(common_renderer_params);
}

uint32_t SynchronousCompositorProxy::NextMetadataVersion() {
  return ++metadata_version_;
}

}  // namespace content
