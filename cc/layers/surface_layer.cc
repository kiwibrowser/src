// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/surface_layer.h"

#include <stdint.h>

#include "base/macros.h"
#include "base/trace_event/trace_event.h"
#include "cc/layers/surface_layer_impl.h"
#include "cc/trees/layer_tree_host.h"

namespace cc {

scoped_refptr<SurfaceLayer> SurfaceLayer::Create() {
  return base::WrapRefCounted(new SurfaceLayer());
}

SurfaceLayer::SurfaceLayer() = default;

SurfaceLayer::~SurfaceLayer() {
  DCHECK(!layer_tree_host());
}

void SurfaceLayer::SetPrimarySurfaceId(const viz::SurfaceId& surface_id,
                                       const DeadlinePolicy& deadline_policy) {
  if (primary_surface_id_ == surface_id &&
      deadline_policy.use_existing_deadline()) {
    return;
  }
  primary_surface_id_ = surface_id;
  if (!deadline_policy.use_existing_deadline())
    deadline_in_frames_ = deadline_policy.deadline_in_frames();
  UpdateDrawsContent(HasDrawableContent());
  SetNeedsCommit();
}

void SurfaceLayer::SetFallbackSurfaceId(const viz::SurfaceId& surface_id) {
  if (fallback_surface_id_ == surface_id)
    return;

  if (layer_tree_host())
    layer_tree_host()->RemoveSurfaceLayerId(fallback_surface_id_);

  fallback_surface_id_ = surface_id;

  if (layer_tree_host() && fallback_surface_id_.is_valid())
    layer_tree_host()->AddSurfaceLayerId(fallback_surface_id_);

  SetNeedsCommit();
}

void SurfaceLayer::SetStretchContentToFillBounds(
    bool stretch_content_to_fill_bounds) {
  if (stretch_content_to_fill_bounds_ == stretch_content_to_fill_bounds)
    return;
  stretch_content_to_fill_bounds_ = stretch_content_to_fill_bounds;
  SetNeedsPushProperties();
}

void SurfaceLayer::SetSurfaceHitTestable(bool surface_hit_testable) {
  if (surface_hit_testable_ == surface_hit_testable)
    return;
  surface_hit_testable_ = surface_hit_testable;
  SetNeedsPushProperties();
}

std::unique_ptr<LayerImpl> SurfaceLayer::CreateLayerImpl(
    LayerTreeImpl* tree_impl) {
  return SurfaceLayerImpl::Create(tree_impl, id());
}

bool SurfaceLayer::HasDrawableContent() const {
  return primary_surface_id_.is_valid() && Layer::HasDrawableContent();
}

void SurfaceLayer::SetLayerTreeHost(LayerTreeHost* host) {
  if (layer_tree_host() == host) {
    return;
  }

  if (layer_tree_host() && fallback_surface_id_.is_valid())
    layer_tree_host()->RemoveSurfaceLayerId(fallback_surface_id_);

  Layer::SetLayerTreeHost(host);

  if (layer_tree_host() && fallback_surface_id_.is_valid())
    layer_tree_host()->AddSurfaceLayerId(fallback_surface_id_);
}

void SurfaceLayer::PushPropertiesTo(LayerImpl* layer) {
  Layer::PushPropertiesTo(layer);
  TRACE_EVENT0("cc", "SurfaceLayer::PushPropertiesTo");
  SurfaceLayerImpl* layer_impl = static_cast<SurfaceLayerImpl*>(layer);
  layer_impl->SetPrimarySurfaceId(primary_surface_id_,
                                  std::move(deadline_in_frames_));
  // Unless the client explicitly calls SetPrimarySurfaceId again after this
  // commit, don't block on |primary_surface_id_| again.
  deadline_in_frames_ = 0u;
  layer_impl->SetFallbackSurfaceId(fallback_surface_id_);
  layer_impl->SetStretchContentToFillBounds(stretch_content_to_fill_bounds_);
  layer_impl->SetSurfaceHitTestable(surface_hit_testable_);
}

}  // namespace cc
