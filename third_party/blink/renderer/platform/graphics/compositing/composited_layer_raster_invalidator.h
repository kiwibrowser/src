// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITING_COMPOSITED_LAYER_RASTER_INVALIDATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITING_COMPOSITED_LAYER_RASTER_INVALIDATOR_H_

#include "third_party/blink/renderer/platform/graphics/compositing/chunk_to_layer_mapper.h"
#include "third_party/blink/renderer/platform/graphics/paint/float_clip_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_chunk.h"
#include "third_party/blink/renderer/platform/graphics/paint/raster_invalidation_tracking.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class PaintArtifact;
class PaintChunkSubset;
class IntRect;

class PLATFORM_EXPORT CompositedLayerRasterInvalidator {
 public:
  using RasterInvalidationFunction = std::function<void(const IntRect&)>;

  CompositedLayerRasterInvalidator(
      RasterInvalidationFunction raster_invalidation_function)
      : raster_invalidation_function_(raster_invalidation_function) {}

  void SetTracksRasterInvalidations(bool);
  RasterInvalidationTracking* GetTracking() const {
    return tracking_info_ ? &tracking_info_->tracking : nullptr;
  }

  RasterInvalidationTracking& EnsureTracking();

  // Generate raster invalidations for all of the paint chunks in the paint
  // artifact.
  void Generate(const PaintArtifact&,
                const gfx::Rect& layer_bounds,
                const PropertyTreeState& layer_state,
                const FloatSize& visual_rect_subpixel_offset = FloatSize());

  // Generate raster invalidations for a subset of the paint chunks in the
  // paint artifact.
  void Generate(const PaintArtifact&,
                const PaintChunkSubset&,
                const gfx::Rect& layer_bounds,
                const PropertyTreeState& layer_state,
                const FloatSize& visual_rect_subpixel_offset = FloatSize());

  bool Matches(const PaintChunk& paint_chunk) const {
    return paint_chunks_info_.size() && paint_chunks_info_[0].is_cacheable &&
           paint_chunk.Matches(paint_chunks_info_[0].id);
  }

  const gfx::Rect& LayerBounds() const { return layer_bounds_; }

  size_t ApproximateUnsharedMemoryUsage() const;

 private:
  friend class CompositedLayerRasterInvalidatorTest;

  struct PaintChunkInfo {
    PaintChunkInfo(const CompositedLayerRasterInvalidator& invalidator,
                   const ChunkToLayerMapper& mapper,
                   const PaintChunk& chunk)
        : id(chunk.id),
          clip_state(chunk.properties.Clip()),
          effect_state(chunk.properties.Effect()),
          is_cacheable(chunk.is_cacheable),
          bounds_in_layer(invalidator.ClipByLayerBounds(
              mapper.MapVisualRect(chunk.bounds))),
          chunk_to_layer_clip(mapper.ClipRect()),
          chunk_to_layer_transform(
              TransformationMatrix::ToSkMatrix44(mapper.Transform())) {}

    bool Matches(const PaintChunk& new_chunk) const {
      return is_cacheable && new_chunk.Matches(id);
    }

    PaintChunk::Id id;
    // These two pointers are for property change detection. The pointed
    // property nodes can be freed after this structure is created. As newly
    // created property nodes always have Changed() flag set, it's not a problem
    // that a new node is created at the address pointed by these pointers.
    const void* clip_state;
    const void* effect_state;
    bool is_cacheable;
    IntRect bounds_in_layer;
    FloatClipRect chunk_to_layer_clip;
    SkMatrix chunk_to_layer_transform;
  };

  void GenerateRasterInvalidations(const PaintArtifact&,
                                   const PaintChunkSubset&,
                                   const PropertyTreeState& layer_state,
                                   const FloatSize& visual_rect_subpixel_offset,
                                   Vector<PaintChunkInfo>& new_chunks_info);
  size_t MatchNewChunkToOldChunk(const PaintChunk& new_chunk, size_t old_index);
  void AddDisplayItemRasterInvalidations(const PaintArtifact&,
                                         const PaintChunk&,
                                         const ChunkToLayerMapper&);
  void IncrementallyInvalidateChunk(const PaintChunkInfo& old_chunk,
                                    const PaintChunkInfo& new_chunk);
  void FullyInvalidateChunk(const PaintChunkInfo& old_chunk,
                            const PaintChunkInfo& new_chunk,
                            PaintInvalidationReason);
  ALWAYS_INLINE void FullyInvalidateNewChunk(const PaintChunkInfo&,
                                             PaintInvalidationReason);
  ALWAYS_INLINE void FullyInvalidateOldChunk(const PaintChunkInfo&,
                                             PaintInvalidationReason);
  ALWAYS_INLINE void AddRasterInvalidation(const IntRect&,
                                           const DisplayItemClient*,
                                           PaintInvalidationReason,
                                           const String* debug_name = nullptr);
  PaintInvalidationReason ChunkPropertiesChanged(
      const RefCountedPropertyTreeState& new_chunk_state,
      const PaintChunkInfo& new_chunk,
      const PaintChunkInfo& old_chunk,
      const PropertyTreeState& layer_state) const;

  // Clip a rect in the layer space by the layer bounds.
  template <typename Rect>
  Rect ClipByLayerBounds(const Rect& r) const {
    return Intersection(
        r, Rect(0, 0, layer_bounds_.width(), layer_bounds_.height()));
  }

  RasterInvalidationFunction raster_invalidation_function_;
  gfx::Rect layer_bounds_;
  Vector<PaintChunkInfo> paint_chunks_info_;

  struct RasterInvalidationTrackingInfo {
    using ClientDebugNamesMap = HashMap<const DisplayItemClient*, String>;
    ClientDebugNamesMap new_client_debug_names;
    ClientDebugNamesMap old_client_debug_names;
    RasterInvalidationTracking tracking;
  };
  std::unique_ptr<RasterInvalidationTrackingInfo> tracking_info_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITING_COMPOSITED_LAYER_RASTER_INVALIDATOR_H_
