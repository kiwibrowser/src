// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/compositor_frame_helpers.h"

namespace viz {
namespace {

constexpr gfx::Rect kDefaultOutputRect(20, 20);
constexpr gfx::Rect kDefaultDamageRect(0, 0);

}  // namespace

CompositorFrameBuilder::CompositorFrameBuilder() {
  frame_ = MakeInitCompositorFrame();
}

CompositorFrameBuilder::~CompositorFrameBuilder() = default;

CompositorFrame CompositorFrameBuilder::Build() {
  CompositorFrame temp_frame(std::move(frame_.value()));
  frame_ = MakeInitCompositorFrame();
  return temp_frame;
}

CompositorFrameBuilder& CompositorFrameBuilder::AddDefaultRenderPass() {
  return AddRenderPass(kDefaultOutputRect, kDefaultDamageRect);
}

CompositorFrameBuilder& CompositorFrameBuilder::AddRenderPass(
    const gfx::Rect& output_rect,
    const gfx::Rect& damage_rect) {
  std::unique_ptr<RenderPass> pass = RenderPass::Create();
  pass->SetNew(next_render_pass_id_++, output_rect, damage_rect,
               gfx::Transform());
  frame_->render_pass_list.push_back(std::move(pass));
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::AddRenderPass(
    std::unique_ptr<RenderPass> render_pass) {
  // Give the render pass a unique id if one hasn't been assigned.
  if (render_pass->id == 0)
    render_pass->id = next_render_pass_id_++;
  frame_->render_pass_list.push_back(std::move(render_pass));
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::SetRenderPassList(
    RenderPassList render_pass_list) {
  DCHECK(frame_->render_pass_list.empty());
  frame_->render_pass_list = std::move(render_pass_list);
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::AddTransferableResource(
    TransferableResource resource) {
  frame_->resource_list.push_back(std::move(resource));
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::SetTransferableResources(
    std::vector<TransferableResource> resource_list) {
  DCHECK(frame_->resource_list.empty());
  frame_->resource_list = std::move(resource_list);
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::SetBeginFrameAck(
    const BeginFrameAck& ack) {
  frame_->metadata.begin_frame_ack = ack;
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::SetDeviceScaleFactor(
    float device_scale_factor) {
  frame_->metadata.device_scale_factor = device_scale_factor;
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::AddLatencyInfo(
    ui::LatencyInfo latency_info) {
  frame_->metadata.latency_info.push_back(std::move(latency_info));
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::AddLatencyInfos(
    std::vector<ui::LatencyInfo> latency_info) {
  if (frame_->metadata.latency_info.empty()) {
    frame_->metadata.latency_info.swap(latency_info);
  } else {
    for (auto& latency : latency_info) {
      frame_->metadata.latency_info.push_back(std::move(latency));
    }
  }
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::SetActivationDependencies(
    std::vector<SurfaceId> activation_dependencies) {
  frame_->metadata.activation_dependencies = std::move(activation_dependencies);
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::SetDeadline(
    const FrameDeadline& deadline) {
  frame_->metadata.deadline = deadline;
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::SetReferencedSurfaces(
    std::vector<SurfaceId> referenced_surfaces) {
  frame_->metadata.referenced_surfaces = std::move(referenced_surfaces);
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::SetFrameToken(
    uint32_t frame_token) {
  frame_->metadata.frame_token = frame_token;
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::SetContentSourceId(
    uint32_t content_source_id) {
  frame_->metadata.content_source_id = content_source_id;
  return *this;
}

CompositorFrameBuilder& CompositorFrameBuilder::SetPresentationToken(
    uint32_t presentation_token) {
  frame_->metadata.presentation_token = presentation_token;
  return *this;
}

CompositorFrame CompositorFrameBuilder::MakeInitCompositorFrame() const {
  CompositorFrame frame;
  frame.metadata.begin_frame_ack =
      BeginFrameAck(BeginFrameArgs::kManualSourceId,
                    BeginFrameArgs::kStartingFrameNumber, true);
  frame.metadata.device_scale_factor = 1.f;
  return frame;
}

CompositorFrame MakeDefaultCompositorFrame() {
  return CompositorFrameBuilder().AddDefaultRenderPass().Build();
}

CompositorFrame MakeEmptyCompositorFrame() {
  return CompositorFrameBuilder().Build();
}

}  // namespace viz
