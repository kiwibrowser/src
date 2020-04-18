// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/paint_artifact.h"

#include "cc/paint/display_item_list.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/graphics/compositing/paint_chunks_to_cc_layer.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/skia/include/core/SkRegion.h"

namespace blink {

namespace {

void ComputeChunkBoundsAndOpaqueness(const DisplayItemList& display_items,
                                     Vector<PaintChunk>& paint_chunks) {
  // This happens in tests testing paint chunks without display items.
  if (display_items.IsEmpty())
    return;

  for (PaintChunk& chunk : paint_chunks) {
    FloatRect bounds;
    SkRegion known_to_be_opaque_region;
    for (const DisplayItem& item : display_items.ItemsInPaintChunk(chunk)) {
      bounds.Unite(item.VisualRect());
      if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled() ||
          !item.IsDrawing())
        continue;
      const auto& drawing = static_cast<const DrawingDisplayItem&>(item);
      if (drawing.GetPaintRecord() && drawing.KnownToBeOpaque()) {
        known_to_be_opaque_region.op(
            SkIRect(EnclosedIntRect(drawing.VisualRect())),
            SkRegion::kUnion_Op);
      }
    }
    chunk.bounds = bounds;
    if (known_to_be_opaque_region.contains(EnclosingIntRect(bounds)))
      chunk.known_to_be_opaque = true;
  }
}

}  // namespace

PaintArtifact::PaintArtifact() : display_item_list_(0) {}

PaintArtifact::PaintArtifact(DisplayItemList display_items,
                             PaintChunksAndRasterInvalidations data)
    : display_item_list_(std::move(display_items)),
      chunks_and_invalidations_(std::move(data)) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    ComputeChunkBoundsAndOpaqueness(display_item_list_,
                                    chunks_and_invalidations_.chunks);
  }
}

PaintArtifact::PaintArtifact(PaintArtifact&& source)
    : display_item_list_(std::move(source.display_item_list_)),
      chunks_and_invalidations_(std::move(source.chunks_and_invalidations_)) {}

PaintArtifact::~PaintArtifact() = default;

PaintArtifact& PaintArtifact::operator=(PaintArtifact&& source) {
  display_item_list_ = std::move(source.display_item_list_);
  chunks_and_invalidations_ = std::move(source.chunks_and_invalidations_);
  return *this;
}

void PaintArtifact::Reset() {
  display_item_list_.Clear();
  chunks_and_invalidations_.Clear();
}

size_t PaintArtifact::ApproximateUnsharedMemoryUsage() const {
  // This function must be called after a full document life cycle update,
  // when we have cleared raster invalidation information.
  DCHECK(chunks_and_invalidations_.raster_invalidation_rects.IsEmpty());
  DCHECK(chunks_and_invalidations_.raster_invalidation_trackings.IsEmpty());
  size_t total_size = sizeof(*this) + display_item_list_.MemoryUsageInBytes() +
                      chunks_and_invalidations_.chunks.capacity() *
                          sizeof(chunks_and_invalidations_.chunks[0]);
  for (const auto& chunk : chunks_and_invalidations_.chunks)
    total_size += chunk.MemoryUsageInBytes();
  return total_size;
}

void PaintArtifact::Replay(GraphicsContext& graphics_context,
                           const PropertyTreeState& replay_state,
                           const IntPoint& offset) const {
  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    DCHECK(offset == IntPoint());
    TRACE_EVENT0("blink,benchmark", "PaintArtifact::replay");
    for (const DisplayItem& display_item : display_item_list_)
      display_item.Replay(graphics_context);
  } else {
    Replay(*graphics_context.Canvas(), replay_state, offset);
  }
}

void PaintArtifact::Replay(PaintCanvas& canvas,
                           const PropertyTreeState& replay_state,
                           const IntPoint& offset) const {
  TRACE_EVENT0("blink,benchmark", "PaintArtifact::replay");
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
  scoped_refptr<cc::DisplayItemList> display_item_list =
      PaintChunksToCcLayer::Convert(
          PaintChunks(), replay_state, gfx::Vector2dF(offset.X(), offset.Y()),
          GetDisplayItemList(),
          cc::DisplayItemList::kToBeReleasedAsPaintOpBuffer);
  canvas.drawPicture(display_item_list->ReleaseAsRecord());
}

DISABLE_CFI_PERF
void PaintArtifact::AppendToDisplayItemList(const FloatSize& visual_rect_offset,
                                            cc::DisplayItemList& list) const {
  TRACE_EVENT0("blink,benchmark", "PaintArtifact::AppendToDisplayItemList");
  for (const DisplayItem& item : display_item_list_)
    item.AppendToDisplayItemList(visual_rect_offset, list);
}

void PaintArtifact::FinishCycle() {
  for (auto& chunk : chunks_and_invalidations_.chunks) {
    chunk.client_is_just_created = false;
    chunk.properties.ClearChangedToRoot();
  }
  chunks_and_invalidations_.raster_invalidation_rects.clear();
  chunks_and_invalidations_.raster_invalidation_trackings.clear();
}

}  // namespace blink
