// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_SHARED_QUAD_STATE_STRUCT_TRAITS_H_
#define SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_SHARED_QUAD_STATE_STRUCT_TRAITS_H_

#include "components/viz/common/quads/shared_quad_state.h"
#include "services/viz/public/interfaces/compositing/shared_quad_state.mojom-shared.h"

namespace mojo {

struct OptSharedQuadState {
  const viz::SharedQuadState* sqs;
};

template <>
struct StructTraits<viz::mojom::SharedQuadStateDataView, OptSharedQuadState> {
  static bool IsNull(const OptSharedQuadState& input) { return !input.sqs; }

  static void SetToNull(OptSharedQuadState* output) { output->sqs = nullptr; }

  static const gfx::Transform& quad_to_target_transform(
      const OptSharedQuadState& input) {
    return input.sqs->quad_to_target_transform;
  }

  static const gfx::Rect& quad_layer_rect(const OptSharedQuadState& input) {
    return input.sqs->quad_layer_rect;
  }

  static const gfx::Rect& visible_quad_layer_rect(
      const OptSharedQuadState& input) {
    return input.sqs->visible_quad_layer_rect;
  }

  static const gfx::Rect& clip_rect(const OptSharedQuadState& input) {
    return input.sqs->clip_rect;
  }

  static bool is_clipped(const OptSharedQuadState& input) {
    return input.sqs->is_clipped;
  }

  static bool are_contents_opaque(const OptSharedQuadState& input) {
    return input.sqs->are_contents_opaque;
  }

  static float opacity(const OptSharedQuadState& input) {
    return input.sqs->opacity;
  }

  static uint32_t blend_mode(const OptSharedQuadState& input) {
    return static_cast<uint32_t>(input.sqs->blend_mode);
  }

  static int32_t sorting_context_id(const OptSharedQuadState& input) {
    return input.sqs->sorting_context_id;
  }
};

template <>
struct StructTraits<viz::mojom::SharedQuadStateDataView, viz::SharedQuadState> {
  static const gfx::Transform& quad_to_target_transform(
      const viz::SharedQuadState& sqs) {
    return sqs.quad_to_target_transform;
  }

  static const gfx::Rect& quad_layer_rect(const viz::SharedQuadState& sqs) {
    return sqs.quad_layer_rect;
  }

  static const gfx::Rect& visible_quad_layer_rect(
      const viz::SharedQuadState& sqs) {
    return sqs.visible_quad_layer_rect;
  }

  static const gfx::Rect& clip_rect(const viz::SharedQuadState& sqs) {
    return sqs.clip_rect;
  }

  static bool is_clipped(const viz::SharedQuadState& sqs) {
    return sqs.is_clipped;
  }

  static bool are_contents_opaque(const viz::SharedQuadState& sqs) {
    return sqs.are_contents_opaque;
  }

  static float opacity(const viz::SharedQuadState& sqs) { return sqs.opacity; }

  static uint32_t blend_mode(const viz::SharedQuadState& sqs) {
    return static_cast<uint32_t>(sqs.blend_mode);
  }

  static int32_t sorting_context_id(const viz::SharedQuadState& sqs) {
    return sqs.sorting_context_id;
  }

  static bool Read(viz::mojom::SharedQuadStateDataView data,
                   viz::SharedQuadState* out) {
    if (!data.ReadQuadToTargetTransform(&out->quad_to_target_transform) ||
        !data.ReadQuadLayerRect(&out->quad_layer_rect) ||
        !data.ReadVisibleQuadLayerRect(&out->visible_quad_layer_rect) ||
        !data.ReadClipRect(&out->clip_rect)) {
      return false;
    }

    out->is_clipped = data.is_clipped();
    out->are_contents_opaque = data.are_contents_opaque();
    out->opacity = data.opacity();
    if (data.blend_mode() > static_cast<int>(SkBlendMode::kLastMode))
      return false;
    out->blend_mode = static_cast<SkBlendMode>(data.blend_mode());
    out->sorting_context_id = data.sorting_context_id();
    return true;
  }
};

}  // namespace mojo

#endif  // SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_SHARED_QUAD_STATE_STRUCT_TRAITS_H_
