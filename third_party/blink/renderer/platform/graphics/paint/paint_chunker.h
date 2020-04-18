// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CHUNKER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CHUNKER_H_

#include "base/optional.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_artifact.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_chunk.h"
#include "third_party/blink/renderer/platform/graphics/paint/property_tree_state.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// Accepts information about changes to chunk properties as drawings are
// accumulated, and produces a series of paint chunks: contiguous ranges of the
// display list with identical properties.
class PLATFORM_EXPORT PaintChunker final {
  DISALLOW_NEW();
  WTF_MAKE_NONCOPYABLE(PaintChunker);

 public:
  PaintChunker();
  ~PaintChunker();

  bool IsInInitialState() const;

  const PropertyTreeState& CurrentPaintChunkProperties() const {
    return current_properties_;
  }
  void UpdateCurrentPaintChunkProperties(const base::Optional<PaintChunk::Id>&,
                                         const PropertyTreeState&);

  void ForceNewChunk();

  // Returns true if a new chunk is created.
  bool IncrementDisplayItemIndex(const DisplayItem&);

  const Vector<PaintChunk>& PaintChunks() const { return data_.chunks; }

  PaintChunk& PaintChunkAt(size_t i) { return data_.chunks[i]; }
  size_t LastChunkIndex() const {
    return data_.chunks.IsEmpty() ? kNotFound : data_.chunks.size() - 1;
  }
  PaintChunk& LastChunk() { return data_.chunks.back(); }

  PaintChunk& FindChunkByDisplayItemIndex(size_t index) {
    auto* chunk = FindChunkInVectorByDisplayItemIndex(data_.chunks, index);
    DCHECK(chunk != data_.chunks.end());
    return *chunk;
  }

  void AddRasterInvalidation(const PaintChunk& chunk, const FloatRect& rect) {
    size_t index = ChunkIndex(chunk);
    auto& rects = data_.raster_invalidation_rects;
    if (rects.size() <= index)
      rects.resize(index + 1);
    rects[index].push_back(rect);
  }

  void TrackRasterInvalidation(const PaintChunk&,
                               const RasterInvalidationInfo&);

  void Clear();

  // Releases the generated paint chunk list and raster invalidations and
  // resets the state of this object.
  PaintChunksAndRasterInvalidations ReleaseData();

 private:
  size_t ChunkIndex(const PaintChunk& chunk) const {
    size_t index = &chunk - &data_.chunks.front();
    DCHECK_LT(index, data_.chunks.size());
    return index;
  }

  PaintChunksAndRasterInvalidations data_;

  // The id specified by UpdateCurrentPaintChunkProperties(). If it is not
  // nullopt, we will use it as the id of the next new chunk. Otherwise we will
  // use the id of the first display item of the new chunk as the id.
  // It's cleared when we create a new chunk with the id, or decide not to
  // create a chunk with it (e.g. when properties don't change and we are not
  // forced to create a new chunk).
  base::Optional<PaintChunk::Id> next_chunk_id_;

  PropertyTreeState current_properties_;

  // True when an item forces a new chunk (e.g., foreign display items), and for
  // the item following a forced chunk. PaintController also forces new chunks
  // before and after subsequences by calling ForceNewChunk().
  bool force_new_chunk_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CHUNKER_H_
