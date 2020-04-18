// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/compositing/composited_layer_raster_invalidator.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "third_party/blink/renderer/platform/graphics/compositing/paint_chunks_to_cc_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_artifact.h"

namespace blink {

void CompositedLayerRasterInvalidator::SetTracksRasterInvalidations(
    bool should_track) {
  if (should_track) {
    if (!tracking_info_)
      tracking_info_ = std::make_unique<RasterInvalidationTrackingInfo>();
    tracking_info_->tracking.ClearInvalidations();
    for (const auto& info : paint_chunks_info_) {
      tracking_info_->old_client_debug_names.Set(&info.id.client,
                                                 info.id.client.DebugName());
    }
  } else if (!RasterInvalidationTracking::ShouldAlwaysTrack()) {
    tracking_info_ = nullptr;
  } else if (tracking_info_) {
    tracking_info_->tracking.ClearInvalidations();
  }
}

size_t CompositedLayerRasterInvalidator::MatchNewChunkToOldChunk(
    const PaintChunk& new_chunk,
    size_t old_index) {
  for (size_t i = old_index; i < paint_chunks_info_.size(); i++) {
    if (paint_chunks_info_[i].Matches(new_chunk))
      return i;
  }
  for (size_t i = 0; i < old_index; i++) {
    if (paint_chunks_info_[i].Matches(new_chunk))
      return i;
  }
  return kNotFound;
}

static bool ApproximatelyEqual(const SkMatrix& a, const SkMatrix& b) {
  static constexpr float kTolerance = 1e-5f;
  for (int i = 0; i < 9; i++) {
    if (std::abs(a[i] - b[i]) > kTolerance)
      return false;
  }
  return true;
}

PaintInvalidationReason
CompositedLayerRasterInvalidator::ChunkPropertiesChanged(
    const RefCountedPropertyTreeState& new_chunk_state,
    const PaintChunkInfo& new_chunk,
    const PaintChunkInfo& old_chunk,
    const PropertyTreeState& layer_state) const {
  // Special case for transform changes because we may create or delete some
  // transform nodes when no raster invalidation is needed. For example, when
  // a composited layer previously not transformed now gets transformed.
  // Check for real accumulated transform change instead.
  if (!ApproximatelyEqual(new_chunk.chunk_to_layer_transform,
                          old_chunk.chunk_to_layer_transform))
    return PaintInvalidationReason::kPaintProperty;

  // Treat the chunk property as changed if the effect node pointer is
  // different, or the effect node's value changed between the layer state and
  // the chunk state.
  if (new_chunk_state.Effect() != old_chunk.effect_state ||
      new_chunk_state.Effect()->Changed(*layer_state.Effect()))
    return PaintInvalidationReason::kPaintProperty;

  // Check for accumulated clip rect change, if the clip rects are tight.
  if (new_chunk.chunk_to_layer_clip.IsTight() &&
      old_chunk.chunk_to_layer_clip.IsTight()) {
    if (new_chunk.chunk_to_layer_clip == old_chunk.chunk_to_layer_clip)
      return PaintInvalidationReason::kNone;
    // Ignore differences out of the current layer bounds.
    if (ClipByLayerBounds(new_chunk.chunk_to_layer_clip.Rect()) ==
        ClipByLayerBounds(old_chunk.chunk_to_layer_clip.Rect()))
      return PaintInvalidationReason::kNone;
    return PaintInvalidationReason::kIncremental;
  }

  // Otherwise treat the chunk property as changed if the clip node pointer is
  // different, or the clip node's value changed between the layer state and the
  // chunk state.
  if (new_chunk_state.Clip() != old_chunk.clip_state ||
      new_chunk_state.Clip()->Changed(*layer_state.Clip()))
    return PaintInvalidationReason::kPaintProperty;

  return PaintInvalidationReason::kNone;
}

// Generates raster invalidations by checking changes (appearing, disappearing,
// reordering, property changes) of chunks. The logic is similar to
// PaintController::GenerateRasterInvalidations(). The complexity is between
// O(n) and O(m*n) where m and n are the numbers of old and new chunks,
// respectively. Normally both m and n are small numbers. The best caseis that
// all old chunks have matching new chunks in the same order. The worst case is
// that no matching chunks except the first one (which always matches otherwise
// we won't reuse the CompositedLayerRasterInvalidator), which is rare. In
// common cases that most of the chunks can be matched in-order, the complexity
// is slightly larger than O(n).
void CompositedLayerRasterInvalidator::GenerateRasterInvalidations(
    const PaintArtifact& paint_artifact,
    const PaintChunkSubset& new_chunks,
    const PropertyTreeState& layer_state,
    const FloatSize& visual_rect_subpixel_offset,
    Vector<PaintChunkInfo>& new_chunks_info) {
  ChunkToLayerMapper mapper(layer_state, layer_bounds_.OffsetFromOrigin(),
                            visual_rect_subpixel_offset);
  Vector<bool> old_chunks_matched;
  old_chunks_matched.resize(paint_chunks_info_.size());
  size_t old_index = 0;
  size_t max_matched_old_index = 0;
  for (const auto& new_chunk : new_chunks) {
    mapper.SwitchToChunk(new_chunk);
    auto& new_chunk_info =
        new_chunks_info.emplace_back(*this, mapper, new_chunk);

    if (!new_chunk.is_cacheable) {
      FullyInvalidateNewChunk(new_chunk_info,
                              PaintInvalidationReason::kChunkUncacheable);
      continue;
    }

    size_t matched_old_index = MatchNewChunkToOldChunk(new_chunk, old_index);
    if (matched_old_index == kNotFound) {
      // The new chunk doesn't match any old chunk.
      FullyInvalidateNewChunk(new_chunk_info,
                              PaintInvalidationReason::kChunkAppeared);
      continue;
    }

    DCHECK(!old_chunks_matched[matched_old_index]);
    old_chunks_matched[matched_old_index] = true;

    auto& old_chunk_info = paint_chunks_info_[matched_old_index];
    // Clip the old chunk bounds by the new layer bounds.
    old_chunk_info.bounds_in_layer =
        ClipByLayerBounds(old_chunk_info.bounds_in_layer);

    PaintInvalidationReason reason =
        matched_old_index < max_matched_old_index
            ? PaintInvalidationReason::kChunkReordered
            : ChunkPropertiesChanged(new_chunk.properties, new_chunk_info,
                                     old_chunk_info, layer_state);

    if (IsFullPaintInvalidationReason(reason)) {
      // Invalidate both old and new bounds of the chunk if the chunk's paint
      // properties changed, or is moved backward and may expose area that was
      // previously covered by it.
      FullyInvalidateChunk(old_chunk_info, new_chunk_info, reason);
      // Ignore the display item raster invalidations because we have fully
      // invalidated the chunk.
    } else {
      // We may have ignored tiny changes of transform, in which case we should
      // use the old chunk_to_layer_transform for later comparison to correctly
      // invalidate animating transform in tiny increments when the accumulated
      // change exceeds the tolerance.
      new_chunk_info.chunk_to_layer_transform =
          old_chunk_info.chunk_to_layer_transform;

      if (reason == PaintInvalidationReason::kIncremental)
        IncrementallyInvalidateChunk(old_chunk_info, new_chunk_info);

      // Add the raster invalidations found by PaintController within the chunk.
      AddDisplayItemRasterInvalidations(paint_artifact, new_chunk, mapper);
    }

    old_index = matched_old_index + 1;
    if (old_index == paint_chunks_info_.size())
      old_index = 0;
    max_matched_old_index = std::max(max_matched_old_index, matched_old_index);
  }

  // Invalidate remaining unmatched (disappeared or uncacheable) old chunks.
  for (size_t i = 0; i < paint_chunks_info_.size(); ++i) {
    if (old_chunks_matched[i])
      continue;
    FullyInvalidateOldChunk(paint_chunks_info_[i],
                            paint_chunks_info_[i].is_cacheable
                                ? PaintInvalidationReason::kChunkDisappeared
                                : PaintInvalidationReason::kChunkUncacheable);
  }
}

void CompositedLayerRasterInvalidator::AddDisplayItemRasterInvalidations(
    const PaintArtifact& paint_artifact,
    const PaintChunk& chunk,
    const ChunkToLayerMapper& mapper) {
  const auto* rects = paint_artifact.GetRasterInvalidationRects(chunk);
  if (!rects || rects->IsEmpty())
    return;

  const auto* tracking = paint_artifact.GetRasterInvalidationTracking(chunk);
  DCHECK(!tracking || tracking->IsEmpty() || tracking->size() == rects->size());

  for (size_t i = 0; i < rects->size(); ++i) {
    auto rect = ClipByLayerBounds(mapper.MapVisualRect((*rects)[i]));
    if (rect.IsEmpty())
      continue;
    raster_invalidation_function_(rect);

    if (tracking && !tracking->IsEmpty()) {
      const auto& info = (*tracking)[i];
      tracking_info_->tracking.AddInvalidation(
          info.client, info.client_debug_name, rect, info.reason);
    }
  }
}

void CompositedLayerRasterInvalidator::IncrementallyInvalidateChunk(
    const PaintChunkInfo& old_chunk,
    const PaintChunkInfo& new_chunk) {
  SkRegion diff(old_chunk.bounds_in_layer);
  diff.op(new_chunk.bounds_in_layer, SkRegion::kXOR_Op);
  for (SkRegion::Iterator it(diff); !it.done(); it.next()) {
    const SkIRect& r = it.rect();
    AddRasterInvalidation(IntRect(r.x(), r.y(), r.width(), r.height()),
                          &new_chunk.id.client,
                          PaintInvalidationReason::kIncremental);
  }
}

void CompositedLayerRasterInvalidator::FullyInvalidateChunk(
    const PaintChunkInfo& old_chunk,
    const PaintChunkInfo& new_chunk,
    PaintInvalidationReason reason) {
  FullyInvalidateOldChunk(old_chunk, reason);
  if (old_chunk.bounds_in_layer != new_chunk.bounds_in_layer)
    FullyInvalidateNewChunk(new_chunk, reason);
}

void CompositedLayerRasterInvalidator::FullyInvalidateNewChunk(
    const PaintChunkInfo& info,
    PaintInvalidationReason reason) {
  AddRasterInvalidation(info.bounds_in_layer, &info.id.client, reason);
}

void CompositedLayerRasterInvalidator::FullyInvalidateOldChunk(
    const PaintChunkInfo& info,
    PaintInvalidationReason reason) {
  String debug_name;
  if (tracking_info_)
    debug_name = tracking_info_->old_client_debug_names.at(&info.id.client);
  AddRasterInvalidation(info.bounds_in_layer, &info.id.client, reason,
                        &debug_name);
}

void CompositedLayerRasterInvalidator::AddRasterInvalidation(
    const IntRect& rect,
    const DisplayItemClient* client,
    PaintInvalidationReason reason,
    const String* debug_name) {
  raster_invalidation_function_(rect);
  if (tracking_info_) {
    tracking_info_->tracking.AddInvalidation(
        client, debug_name ? *debug_name : client->DebugName(), rect, reason);
  }
}

RasterInvalidationTracking& CompositedLayerRasterInvalidator::EnsureTracking() {
  if (!tracking_info_)
    tracking_info_ = std::make_unique<RasterInvalidationTrackingInfo>();
  return tracking_info_->tracking;
}

void CompositedLayerRasterInvalidator::Generate(
    const PaintArtifact& paint_artifact,
    const gfx::Rect& layer_bounds,
    const PropertyTreeState& layer_state,
    const FloatSize& visual_rect_subpixel_offset) {
  Generate(paint_artifact, paint_artifact.PaintChunks(), layer_bounds,
           layer_state, visual_rect_subpixel_offset);
}

void CompositedLayerRasterInvalidator::Generate(
    const PaintArtifact& paint_artifact,
    const PaintChunkSubset& paint_chunks,
    const gfx::Rect& layer_bounds,
    const PropertyTreeState& layer_state,
    const FloatSize& visual_rect_subpixel_offset) {
  if (RuntimeEnabledFeatures::DisableRasterInvalidationEnabled())
    return;

  if (RasterInvalidationTracking::ShouldAlwaysTrack())
    EnsureTracking();

  if (tracking_info_) {
    for (const auto& chunk : paint_chunks) {
      tracking_info_->new_client_debug_names.insert(
          &chunk.id.client, chunk.id.client.DebugName());
    }
  }

  bool layer_bounds_was_empty = layer_bounds_.IsEmpty();
  layer_bounds_ = layer_bounds;

  Vector<PaintChunkInfo> new_chunks_info;
  new_chunks_info.ReserveCapacity(paint_chunks.size());

  if (layer_bounds_was_empty || layer_bounds_.IsEmpty()) {
    // No raster invalidation is needed if either the old bounds or the new
    // bounds is empty, but we still need to update new_chunks_info for the
    // next cycle.
    ChunkToLayerMapper mapper(layer_state, layer_bounds.OffsetFromOrigin(),
                              visual_rect_subpixel_offset);
    for (const auto& chunk : paint_chunks) {
      mapper.SwitchToChunk(chunk);
      new_chunks_info.emplace_back(*this, mapper, chunk);
    }
  } else {
    GenerateRasterInvalidations(paint_artifact, paint_chunks, layer_state,
                                visual_rect_subpixel_offset, new_chunks_info);
  }

  paint_chunks_info_ = std::move(new_chunks_info);

  if (tracking_info_) {
    tracking_info_->old_client_debug_names =
        std::move(tracking_info_->new_client_debug_names);
  }
}

size_t CompositedLayerRasterInvalidator::ApproximateUnsharedMemoryUsage()
    const {
  return sizeof(*this) + paint_chunks_info_.capacity() * sizeof(PaintChunkInfo);
}

}  // namespace blink
