// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_ARTIFACT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_ARTIFACT_H_

#include "third_party/blink/renderer/platform/graphics/paint/display_item_list.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_canvas.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_chunk.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_chunk_subset.h"
#include "third_party/blink/renderer/platform/graphics/paint/raster_invalidation_tracking.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class GraphicsContext;
class PaintChunkSubset;

using ChunkRasterInvalidationRects = Vector<FloatRect, 2>;
using ChunkRasterInvalidationTracking = Vector<RasterInvalidationInfo>;

struct PaintChunksAndRasterInvalidations {
  void Clear() {
    chunks.clear();
    raster_invalidation_rects.clear();
    raster_invalidation_trackings.clear();
  }

  Vector<PaintChunk> chunks;
  Vector<ChunkRasterInvalidationRects> raster_invalidation_rects;
  Vector<ChunkRasterInvalidationTracking> raster_invalidation_trackings;
};

// The output of painting, consisting of
// - display item list (in DisplayItemList),
// - paint chunks and their paint invalidations (in
// PaintChunksAndRasterInvalidations).
//
// Display item list and paint chunks not only represent the output of the
// current painting, but also serve as cache of individual display items and
// paint chunks for later paintings as long as the display items and chunks are
// valid.
//
// Raster invalidations are consumed by CompositedLayerRasterInvalidator and
// will be cleared after a cycle is complete via |FinishCycle()|.
//
// It represents a particular state of the world, and should be immutable
// (const) to most of its users.
//
// Unless its dangerous accessors are used, it promises to be in a reasonable
// state (e.g. chunk bounding boxes computed).
//
// Reminder: moved-from objects may not be in a known state. They can only
// safely be assigned to or destroyed.
class PLATFORM_EXPORT PaintArtifact final {
  DISALLOW_NEW();
  WTF_MAKE_NONCOPYABLE(PaintArtifact);

 public:
  PaintArtifact();
  PaintArtifact(DisplayItemList, PaintChunksAndRasterInvalidations);
  PaintArtifact(PaintArtifact&&);
  ~PaintArtifact();

  PaintArtifact& operator=(PaintArtifact&&);

  bool IsEmpty() const { return display_item_list_.IsEmpty(); }

  DisplayItemList& GetDisplayItemList() { return display_item_list_; }
  const DisplayItemList& GetDisplayItemList() const {
    return display_item_list_;
  }

  Vector<PaintChunk>& PaintChunks() { return chunks_and_invalidations_.chunks; }
  const Vector<PaintChunk>& PaintChunks() const {
    return chunks_and_invalidations_.chunks;
  }

  PaintChunkSubset GetPaintChunkSubset(
      const Vector<size_t>& subset_indices) const {
    return PaintChunkSubset(PaintChunks(), subset_indices);
  }

  Vector<PaintChunk>::const_iterator FindChunkByDisplayItemIndex(
      size_t index) const {
    return FindChunkInVectorByDisplayItemIndex(PaintChunks(), index);
  }

  // Resets to an empty paint artifact.
  void Reset();

  // Returns the approximate memory usage, excluding memory likely to be
  // shared with the embedder after copying to cc::DisplayItemList.
  size_t ApproximateUnsharedMemoryUsage() const;

  // Draws the paint artifact to a GraphicsContext.
  // In SPv175+ mode, replays into the ancestor state given by |replay_state|.
  void Replay(GraphicsContext&,
              const PropertyTreeState& replay_state,
              const IntPoint& offset = IntPoint()) const;

  // Draws the paint artifact to a PaintCanvas, into the ancestor state given
  // by |replay_state|. For SPv175+ only.
  void Replay(PaintCanvas&,
              const PropertyTreeState& replay_state,
              const IntPoint& offset = IntPoint()) const;

  // Writes the paint artifact into a cc::DisplayItemList.
  void AppendToDisplayItemList(const FloatSize& visual_rect_offset,
                               cc::DisplayItemList& display_list) const;

  const ChunkRasterInvalidationRects* GetRasterInvalidationRects(
      size_t chunk_index) const {
    return chunk_index <
                   chunks_and_invalidations_.raster_invalidation_rects.size()
               ? &chunks_and_invalidations_
                      .raster_invalidation_rects[chunk_index]
               : nullptr;
  }

  const ChunkRasterInvalidationRects* GetRasterInvalidationRects(
      const PaintChunk& chunk) const {
    return GetRasterInvalidationRects(
        &chunk - &chunks_and_invalidations_.chunks.front());
  }

  const ChunkRasterInvalidationTracking* GetRasterInvalidationTracking(
      size_t chunk_index) const {
    return chunk_index < chunks_and_invalidations_.raster_invalidation_trackings
                             .size()
               ? &chunks_and_invalidations_
                      .raster_invalidation_trackings[chunk_index]
               : nullptr;
  }

  const ChunkRasterInvalidationTracking* GetRasterInvalidationTracking(
      const PaintChunk& chunk) const {
    return GetRasterInvalidationTracking(
        &chunk - &chunks_and_invalidations_.chunks.front());
  }

  // Called when the caller finishes updating a full document life cycle.
  // Will cleanup data (e.g. raster invalidations) that will no longer be used
  // for the next cycle, and update status to be ready for the next cycle.
  void FinishCycle();

 private:
  DisplayItemList display_item_list_;
  PaintChunksAndRasterInvalidations chunks_and_invalidations_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_ARTIFACT_H_
