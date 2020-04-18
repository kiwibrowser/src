// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"

#include <memory>
#include "base/auto_reset.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/logging_canvas.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_display_item.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

void PaintController::SetTracksRasterInvalidations(bool value) {
  if (value) {
    raster_invalidation_tracking_info_ =
        std::make_unique<RasterInvalidationTrackingInfo>();

    // This is called just after a full document cycle update, so all clients in
    // current_paint_artifact_ should be still alive.
    DCHECK(new_display_item_list_.IsEmpty());
    for (const auto& item : current_paint_artifact_.GetDisplayItemList()) {
      raster_invalidation_tracking_info_->old_client_debug_names.Set(
          &item.Client(), item.Client().DebugName());
    }
  } else if (!RasterInvalidationTracking::ShouldAlwaysTrack()) {
    raster_invalidation_tracking_info_ = nullptr;
  }
}

void PaintController::EnsureRasterInvalidationTracking() {
  if (!raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_ =
        std::make_unique<RasterInvalidationTrackingInfo>();
  }
}

const PaintArtifact& PaintController::GetPaintArtifact() const {
  DCHECK(new_display_item_list_.IsEmpty());
  DCHECK(new_paint_chunks_.IsInInitialState());
  return current_paint_artifact_;
}

bool PaintController::UseCachedDrawingIfPossible(
    const DisplayItemClient& client,
    DisplayItem::Type type) {
  DCHECK(DisplayItem::IsDrawingType(type));

  if (DisplayItemConstructionIsDisabled())
    return false;

  if (!ClientCacheIsValid(client))
    return false;

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() &&
      IsCheckingUnderInvalidation()) {
    // We are checking under-invalidation of a subsequence enclosing this
    // display item. Let the client continue to actually paint the display item.
    return false;
  }

  size_t cached_item =
      FindCachedItem(DisplayItem::Id(client, type, current_fragment_));
  if (cached_item == kNotFound) {
    // See FindOutOfOrderCachedItemForward() for explanation of the situation.
    return false;
  }

  ++num_cached_new_items_;
  EnsureNewDisplayItemListInitialCapacity();
  // Visual rect can change without needing invalidation of the client, e.g.
  // when ancestor clip changes. Update the visual rect to the current value.
  current_paint_artifact_.GetDisplayItemList()[cached_item].UpdateVisualRect();
  if (!RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled())
    ProcessNewItem(MoveItemFromCurrentListToNewList(cached_item));

  next_item_to_match_ = cached_item + 1;
  // Items before |next_item_to_match_| have been copied so we don't need to
  // index them.
  if (next_item_to_match_ > next_item_to_index_)
    next_item_to_index_ = next_item_to_match_;

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled()) {
    if (!IsCheckingUnderInvalidation()) {
      under_invalidation_checking_begin_ = cached_item;
      under_invalidation_checking_end_ = cached_item + 1;
      under_invalidation_message_prefix_ = "";
    }
    // Return false to let the painter actually paint. We will check if the new
    // painting is the same as the cached one.
    return false;
  }

  return true;
}

bool PaintController::UseCachedSubsequenceIfPossible(
    const DisplayItemClient& client) {
  if (DisplayItemConstructionIsDisabled() || SubsequenceCachingIsDisabled())
    return false;

  if (!ClientCacheIsValid(client))
    return false;

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() &&
      IsCheckingUnderInvalidation()) {
    // We are checking under-invalidation of an ancestor subsequence enclosing
    // this one. The ancestor subsequence is supposed to have already "copied",
    // so we should let the client continue to actually paint the descendant
    // subsequences without "copying".
    return false;
  }

  SubsequenceMarkers* markers = GetSubsequenceMarkers(client);
  if (!markers) {
    return false;
  }

  if (current_paint_artifact_.GetDisplayItemList()[markers->start]
          .IsTombstone()) {
    // The subsequence has already been copied, indicating that the same client
    // created multiple subsequences. If DCHECK_IS_ON(), then we should have
    // encountered the DCHECK at the end of EndSubsequence() during the previous
    // paint.
    NOTREACHED();
    return false;
  }

  EnsureNewDisplayItemListInitialCapacity();

  if (next_item_to_match_ == markers->start) {
    // We are matching new and cached display items sequentially. Skip the
    // subsequence for later sequential matching of individual display items.
    next_item_to_match_ = markers->end;
    // Items before |next_item_to_match_| have been copied so we don't need to
    // index them.
    if (next_item_to_match_ > next_item_to_index_)
      next_item_to_index_ = next_item_to_match_;
  }

  num_cached_new_items_ += markers->end - markers->start;

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled()) {
    DCHECK(!IsCheckingUnderInvalidation());
    under_invalidation_checking_begin_ = markers->start;
    under_invalidation_checking_end_ = markers->end;
    under_invalidation_message_prefix_ =
        "(In cached subsequence for " + client.DebugName() + ")";
    // Return false to let the painter actually paint. We will check if the new
    // painting is the same as the cached one.
    return false;
  }

  size_t start = BeginSubsequence();
  CopyCachedSubsequence(markers->start, markers->end);
  EndSubsequence(client, start);
  return true;
}

PaintController::SubsequenceMarkers* PaintController::GetSubsequenceMarkers(
    const DisplayItemClient& client) {
  auto result = current_cached_subsequences_.find(&client);
  if (result == current_cached_subsequences_.end())
    return nullptr;
  return &result->value;
}

size_t PaintController::BeginSubsequence() {
  // Force new paint chunk which is required for subsequence caching.
  new_paint_chunks_.ForceNewChunk();
  return new_display_item_list_.size();
}

void PaintController::EndSubsequence(const DisplayItemClient& client,
                                     size_t start) {
  size_t end = new_display_item_list_.size();

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() &&
      IsCheckingUnderInvalidation()) {
    SubsequenceMarkers* markers = GetSubsequenceMarkers(client);
    if (!markers && start != end) {
      ShowSequenceUnderInvalidationError(
          "under-invalidation : unexpected subsequence", client, start, end);
      CHECK(false);
    }
    if (markers && markers->end - markers->start != end - start) {
      ShowSequenceUnderInvalidationError(
          "under-invalidation: new subsequence wrong length", client, start,
          end);
      CHECK(false);
    }
  }

  if (start == end) {
    // Omit the empty subsequence. The forcing-new-chunk flag set by
    // BeginSubsequence() still applies, but this not a big deal because empty
    // subsequences are not common. Also we should not clear the flag because
    // there might be unhandled flag that was set before this empty subsequence.
    return;
  }

  // Force new paint chunk which is required for subsequence caching.
  new_paint_chunks_.ForceNewChunk();

  DCHECK(!new_cached_subsequences_.Contains(&client))
      << "Multiple subsequences for client: " << client.DebugName();

  new_cached_subsequences_.insert(&client, SubsequenceMarkers(start, end));
  last_cached_subsequence_end_ = end;
}

bool PaintController::LastDisplayItemIsNoopBegin() const {
  DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());

  if (new_display_item_list_.IsEmpty())
    return false;

  const auto& last_display_item = new_display_item_list_.Last();
  return last_display_item.IsBegin() && !last_display_item.DrawsContent();
}

bool PaintController::LastDisplayItemIsSubsequenceEnd() const {
  return !new_cached_subsequences_.IsEmpty() &&
         last_cached_subsequence_end_ == new_display_item_list_.size();
}

void PaintController::RemoveLastDisplayItem() {
  DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());

  if (new_display_item_list_.IsEmpty())
    return;

#if DCHECK_IS_ON()
  // Also remove the index pointing to the removed display item.
  IndicesByClientMap::iterator it = new_display_item_indices_by_client_.find(
      &new_display_item_list_.Last().Client());
  if (it != new_display_item_indices_by_client_.end()) {
    Vector<size_t>& indices = it->value;
    if (!indices.IsEmpty() &&
        indices.back() == (new_display_item_list_.size() - 1))
      indices.pop_back();
  }
#endif

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() &&
      IsCheckingUnderInvalidation()) {
    if (skipped_probable_under_invalidation_count_) {
      --skipped_probable_under_invalidation_count_;
    } else {
      DCHECK(under_invalidation_checking_begin_);
      --under_invalidation_checking_begin_;
      // The old display item is a tombstone because it was matched by the begin
      // display item being removed. Restore the tombstone so that we can match
      // the next new display item against it.
      current_paint_artifact_.GetDisplayItemList().RestoreTombstone(
          under_invalidation_checking_begin_, new_display_item_list_.Last());
    }
  }
  new_display_item_list_.RemoveLast();
}

const DisplayItem* PaintController::LastDisplayItem(unsigned offset) {
  if (offset < new_display_item_list_.size())
    return &new_display_item_list_[new_display_item_list_.size() - offset - 1];
  return nullptr;
}

void PaintController::ProcessNewItem(DisplayItem& display_item) {
  DCHECK(!construction_disabled_);

  if (IsSkippingCache())
    display_item.SetSkippedCache();

  if (raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_->new_client_debug_names.insert(
        &display_item.Client(), display_item.Client().DebugName());
  }

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    size_t last_chunk_index = new_paint_chunks_.LastChunkIndex();
    bool chunk_added =
        new_paint_chunks_.IncrementDisplayItemIndex(display_item);
    auto& last_chunk = new_paint_chunks_.LastChunk();

#if DCHECK_IS_ON()
    if (chunk_added && last_chunk.is_cacheable) {
      AddToIndicesByClientMap(last_chunk.id.client,
                              new_paint_chunks_.LastChunkIndex(),
                              new_paint_chunk_indices_by_client_);
    }
#endif

    if (chunk_added && last_chunk_index != kNotFound) {
      DCHECK(last_chunk_index != new_paint_chunks_.LastChunkIndex());
      GenerateRasterInvalidations(
          new_paint_chunks_.PaintChunkAt(last_chunk_index));
    }

    last_chunk.outset_for_raster_effects =
        std::max(last_chunk.outset_for_raster_effects,
                 display_item.OutsetForRasterEffects());
  }

#if DCHECK_IS_ON()
  // Verify noop begin/end pairs have been removed.
  if (new_display_item_list_.size() >= 2 && display_item.IsEnd()) {
    const auto& begin_display_item =
        new_display_item_list_[new_display_item_list_.size() - 2];
    if (begin_display_item.IsBegin() && !begin_display_item.DrawsContent())
      DCHECK(!display_item.IsEndAndPairedWith(begin_display_item.GetType()));
  }

  if (display_item.IsCacheable()) {
    size_t index = FindMatchingItemFromIndex(
        display_item.GetId(), new_display_item_indices_by_client_,
        new_display_item_list_);
    if (index != kNotFound) {
      ShowDebugData();
      NOTREACHED()
          << "DisplayItem " << display_item.AsDebugString().Utf8().data()
          << " has duplicated id with previous "
          << new_display_item_list_[index].AsDebugString().Utf8().data()
          << " (index=" << index << ")";
    }
    AddToIndicesByClientMap(display_item.Client(),
                            new_display_item_list_.size() - 1,
                            new_display_item_indices_by_client_);
  }
#endif  // DCHECK_IS_ON()

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled())
    CheckUnderInvalidation();

  if (!frame_first_paints_.back().first_painted && display_item.IsDrawing() &&
      // Here we ignore all document-background paintings because we don't
      // know if the background is default. ViewPainter should have called
      // setFirstPainted() if this display item is for non-default
      // background.
      display_item.GetType() != DisplayItem::kDocumentBackground &&
      display_item.DrawsContent()) {
    SetFirstPainted();
  }
}

DisplayItem& PaintController::MoveItemFromCurrentListToNewList(size_t index) {
  items_moved_into_new_list_.resize(
      current_paint_artifact_.GetDisplayItemList().size());
  items_moved_into_new_list_[index] = new_display_item_list_.size();
  return new_display_item_list_.AppendByMoving(
      current_paint_artifact_.GetDisplayItemList()[index]);
}

void PaintController::InvalidateAll() {
  DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV2Enabled());
  InvalidateAllInternal();
}

void PaintController::InvalidateAllInternal() {
  // TODO(wangxianzhu): Rename this to InvalidateAllForTesting() for SPv2.
  // Can only be called during layout/paintInvalidation, not during painting.
  DCHECK(new_display_item_list_.IsEmpty());
  current_paint_artifact_.Reset();
  current_cache_generation_.Invalidate();
}

bool PaintController::CacheIsAllInvalid() const {
  DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV2Enabled());
  return current_paint_artifact_.IsEmpty() &&
         current_cache_generation_.GetPaintInvalidationReason() !=
             PaintInvalidationReason::kNone;
}

bool PaintController::ClientCacheIsValid(
    const DisplayItemClient& client) const {
#if DCHECK_IS_ON()
  DCHECK(client.IsAlive());
#endif
  if (IsSkippingCache())
    return false;
  return client.DisplayItemsAreCached(current_cache_generation_);
}

size_t PaintController::FindMatchingItemFromIndex(
    const DisplayItem::Id& id,
    const IndicesByClientMap& display_item_indices_by_client,
    const DisplayItemList& list) {
  IndicesByClientMap::const_iterator it =
      display_item_indices_by_client.find(&id.client);
  if (it == display_item_indices_by_client.end())
    return kNotFound;

  const Vector<size_t>& indices = it->value;
  for (size_t index : indices) {
    const DisplayItem& existing_item = list[index];
    if (existing_item.IsTombstone())
      continue;
    DCHECK(existing_item.Client() == id.client);
    if (id == existing_item.GetId())
      return index;
  }

  return kNotFound;
}

void PaintController::AddToIndicesByClientMap(const DisplayItemClient& client,
                                              size_t index,
                                              IndicesByClientMap& map) {
  auto it = map.find(&client);
  auto& indices =
      it == map.end()
          ? map.insert(&client, Vector<size_t>()).stored_value->value
          : it->value;
  indices.push_back(index);
}

size_t PaintController::FindCachedItem(const DisplayItem::Id& id) {
  DCHECK(ClientCacheIsValid(id.client));

  // Try to find the item sequentially first. This is fast if the current list
  // and the new list are in the same order around the new item. If found, we
  // don't need to update and lookup the index.
  for (size_t i = next_item_to_match_;
       i < current_paint_artifact_.GetDisplayItemList().size(); ++i) {
    // We encounter an item that has already been copied which indicates we
    // can't do sequential matching.
    const DisplayItem& item = current_paint_artifact_.GetDisplayItemList()[i];
    if (item.IsTombstone())
      break;
    if (id == item.GetId()) {
#if DCHECK_IS_ON()
      ++num_sequential_matches_;
#endif
      return i;
    }
    // We encounter a different cacheable item which also indicates we can't do
    // sequential matching.
    if (item.IsCacheable())
      break;
  }

  size_t found_index =
      FindMatchingItemFromIndex(id, out_of_order_item_indices_,
                                current_paint_artifact_.GetDisplayItemList());
  if (found_index != kNotFound) {
#if DCHECK_IS_ON()
    ++num_out_of_order_matches_;
#endif
    return found_index;
  }

  return FindOutOfOrderCachedItemForward(id);
}

// Find forward for the item and index all skipped indexable items.
size_t PaintController::FindOutOfOrderCachedItemForward(
    const DisplayItem::Id& id) {
  for (size_t i = next_item_to_index_;
       i < current_paint_artifact_.GetDisplayItemList().size(); ++i) {
    const DisplayItem& item = current_paint_artifact_.GetDisplayItemList()[i];
    if (item.IsTombstone())
      continue;
    if (id == item.GetId()) {
#if DCHECK_IS_ON()
      ++num_sequential_matches_;
#endif
      return i;
    }
    if (item.IsCacheable()) {
#if DCHECK_IS_ON()
      ++num_indexed_items_;
#endif
      AddToIndicesByClientMap(item.Client(), i, out_of_order_item_indices_);
    }
  }

  // The display item newly appears while the client is not invalidated. The
  // situation alone (without other kinds of under-invalidations) won't corrupt
  // rendering, but causes AddItemToIndexIfNeeded() for all remaining display
  // item, which is not the best for performance. In this case, the caller
  // should fall back to repaint the display item.
  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled()) {
#if DCHECK_IS_ON()
    ShowDebugData();
#endif
    // Ensure our paint invalidation tests don't trigger the less performant
    // situation which should be rare.
    LOG(WARNING) << "Can't find cached display item: " << id.client.DebugName()
                 << " " << id.ToString();
  }
  return kNotFound;
}

// Copies a cached subsequence from current list to the new list.
// When paintUnderInvaldiationCheckingEnabled() we'll not actually
// copy the subsequence, but mark the begin and end of the subsequence for
// under-invalidation checking.
void PaintController::CopyCachedSubsequence(size_t begin_index,
                                            size_t end_index) {
  DCHECK(!RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled());

  base::AutoReset<size_t> subsequence_begin_index(
      &current_cached_subsequence_begin_index_in_new_list_,
      new_display_item_list_.size());
  DisplayItem* cached_item =
      &current_paint_artifact_.GetDisplayItemList()[begin_index];

  Vector<PaintChunk>::const_iterator cached_chunk;
  base::Optional<PropertyTreeState> properties_before_subsequence;
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    cached_chunk =
        current_paint_artifact_.FindChunkByDisplayItemIndex(begin_index);
    DCHECK(cached_chunk != current_paint_artifact_.PaintChunks().end());

    properties_before_subsequence =
        new_paint_chunks_.CurrentPaintChunkProperties();
    UpdateCurrentPaintChunkPropertiesUsingIdWithFragment(
        cached_chunk->id, cached_chunk->properties.GetPropertyTreeState());
  } else {
    // Avoid uninitialized variable error on Windows.
    cached_chunk = current_paint_artifact_.PaintChunks().begin();
  }

  for (size_t current_index = begin_index; current_index < end_index;
       ++current_index) {
    cached_item = &current_paint_artifact_.GetDisplayItemList()[current_index];
    SECURITY_CHECK(!cached_item->IsTombstone());
#if DCHECK_IS_ON()
    DCHECK(cached_item->Client().IsAlive());
#endif

    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
        current_index == cached_chunk->end_index) {
      ++cached_chunk;
      DCHECK(cached_chunk != current_paint_artifact_.PaintChunks().end());
      new_paint_chunks_.ForceNewChunk();
      UpdateCurrentPaintChunkPropertiesUsingIdWithFragment(
          cached_chunk->id, cached_chunk->properties.GetPropertyTreeState());
    }

#if DCHECK_IS_ON()
    // Visual rect change should not happen in a cached subsequence.
    // However, because of different method of pixel snapping in different
    // paths, there are false positives. Just log an error.
    if (cached_item->VisualRect() !=
        FloatRect(cached_item->Client().VisualRect())) {
      LOG(ERROR) << "Visual rect changed in a cached subsequence: "
                 << cached_item->Client().DebugName()
                 << " old=" << cached_item->VisualRect().ToString()
                 << " new=" << cached_item->Client().VisualRect().ToString();
    }
#endif

    ProcessNewItem(MoveItemFromCurrentListToNewList(current_index));
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      DCHECK((!new_paint_chunks_.LastChunk().is_cacheable &&
              !cached_chunk->is_cacheable) ||
             new_paint_chunks_.LastChunk().Matches(*cached_chunk));
    }
  }

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled()) {
    under_invalidation_checking_end_ = end_index;
    DCHECK(IsCheckingUnderInvalidation());
  } else if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    // Restore properties and force new chunk for any trailing display items
    // after the cached subsequence without new properties.
    new_paint_chunks_.ForceNewChunk();
    UpdateCurrentPaintChunkProperties(base::nullopt,
                                      *properties_before_subsequence);
  }
}

void PaintController::ResetCurrentListIndices() {
  next_item_to_match_ = 0;
  next_item_to_index_ = 0;
  next_chunk_to_match_ = 0;
  under_invalidation_checking_begin_ = 0;
  under_invalidation_checking_end_ = 0;
  skipped_probable_under_invalidation_count_ = 0;
}

DISABLE_CFI_PERF
void PaintController::CommitNewDisplayItems() {
  TRACE_EVENT2("blink,benchmark", "PaintController::commitNewDisplayItems",
               "current_display_list_size",
               (int)current_paint_artifact_.GetDisplayItemList().size(),
               "num_non_cached_new_items",
               (int)new_display_item_list_.size() - num_cached_new_items_);

  num_cached_new_items_ = 0;
#if DCHECK_IS_ON()
  new_display_item_indices_by_client_.clear();
  new_paint_chunk_indices_by_client_.clear();
#endif

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
      !new_display_item_list_.IsEmpty())
    GenerateRasterInvalidations(new_paint_chunks_.LastChunk());

  auto old_cache_generation = current_cache_generation_;
  current_cache_generation_ =
      DisplayItemClient::CacheGenerationOrInvalidationReason::Next();

  new_cached_subsequences_.swap(current_cached_subsequences_);
  new_cached_subsequences_.clear();
  last_cached_subsequence_end_ = 0;
  for (auto& item : current_cached_subsequences_)
    item.key->SetDisplayItemsCached(current_cache_generation_);

  Vector<const DisplayItemClient*> skipped_cache_clients;
  for (const auto& item : new_display_item_list_) {
    const auto& client = item.Client();
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
      client.ClearPartialInvalidationRect();

    if (item.IsCacheable()) {
      client.SetDisplayItemsCached(current_cache_generation_);
    } else {
      if (client.IsJustCreated())
        client.ClearIsJustCreated();
      if (item.SkippedCache())
        skipped_cache_clients.push_back(&item.Client());
    }
  }

  for (auto* client : skipped_cache_clients) {
    // Set client uncached only if it is cached by this PaintController. The
    // client may be still validly cached in another PaintController which
    // should not be affected by skipping cache in this PaintController.
    if (client->DisplayItemsAreCached(old_cache_generation) ||
        // The client was set cached because it just painted some cacheable
        // items in this PaintController. Need to set it uncached.
        client->DisplayItemsAreCached(current_cache_generation_))
      client->SetDisplayItemsUncached();
  }

  // The new list will not be appended to again so we can release unused memory.
  new_display_item_list_.ShrinkToFit();

  current_paint_artifact_ = PaintArtifact(std::move(new_display_item_list_),
                                          new_paint_chunks_.ReleaseData());

  ResetCurrentListIndices();
  out_of_order_item_indices_.clear();
  out_of_order_chunk_indices_.clear();
  items_moved_into_new_list_.clear();

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    for (const auto& chunk : current_paint_artifact_.PaintChunks()) {
      if (chunk.id.client.IsJustCreated())
        chunk.id.client.ClearIsJustCreated();
    }
  }

  // We'll allocate the initial buffer when we start the next paint.
  new_display_item_list_ = DisplayItemList(0);

#if DCHECK_IS_ON()
  num_sequential_matches_ = 0;
  num_out_of_order_matches_ = 0;
  num_indexed_items_ = 0;
#endif

  if (raster_invalidation_tracking_info_) {
    raster_invalidation_tracking_info_->old_client_debug_names.clear();
    std::swap(raster_invalidation_tracking_info_->old_client_debug_names,
              raster_invalidation_tracking_info_->new_client_debug_names);
  }

#if DCHECK_IS_ON()
  if (VLOG_IS_ON(2)) {
    LOG(ERROR) << "PaintController::CommitNewDisplayItems() done";
    if (VLOG_IS_ON(3))
      ShowDebugDataWithRecords();
    else
      ShowDebugData();
  }
#endif
}

void PaintController::FinishCycle() {
  DCHECK(new_display_item_list_.IsEmpty());
  DCHECK(new_paint_chunks_.IsInInitialState());

  current_paint_artifact_.FinishCycle();
}

size_t PaintController::ApproximateUnsharedMemoryUsage() const {
  size_t memory_usage = sizeof(*this);

  // Memory outside this class due to current_paint_artifact_.
  memory_usage += current_paint_artifact_.ApproximateUnsharedMemoryUsage() -
                  sizeof(current_paint_artifact_);

  // TODO(jbroman): If display items begin to have significant external memory
  // usage that's not shared with the embedder, we should account for it here.
  //
  // External objects, shared with the embedder, such as PaintRecord, should be
  // excluded to avoid double counting. It is the embedder's responsibility to
  // count such objects.
  //
  // At time of writing, the only known case of unshared external memory was
  // the rounded clips vector in ClipDisplayItem, which is not expected to
  // contribute significantly to memory usage.

  // Memory outside this class due to new_display_item_list_.
  DCHECK(new_display_item_list_.IsEmpty());
  memory_usage += new_display_item_list_.MemoryUsageInBytes();

  // Memory outside this class due to current_cached_subsequences_ and
  // new_cached_subsequences_.
  memory_usage += current_cached_subsequences_.Capacity() *
                  sizeof(*current_cached_subsequences_.begin());
  DCHECK(new_cached_subsequences_.IsEmpty());
  memory_usage += new_cached_subsequences_.Capacity() *
                  sizeof(*new_cached_subsequences_.begin());

  return memory_usage;
}

void PaintController::AppendDebugDrawingAfterCommit(
    const DisplayItemClient& display_item_client,
    sk_sp<const PaintRecord> record,
    const PropertyTreeState* property_tree_state) {
  DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV2Enabled());
  DCHECK(new_display_item_list_.IsEmpty());
  auto& display_item_list = current_paint_artifact_.GetDisplayItemList();
  auto& display_item =
      display_item_list.AllocateAndConstruct<DrawingDisplayItem>(
          display_item_client, DisplayItem::kDebugDrawing, std::move(record));
  display_item.SetSkippedCache();

  if (property_tree_state) {
    DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
    // Create a PaintChunk for the debug drawing.
    current_paint_artifact_.PaintChunks().emplace_back(
        display_item_list.size() - 1, display_item_list.size(),
        display_item.GetId(), *property_tree_state);
  }
}

void PaintController::GenerateRasterInvalidations(PaintChunk& new_chunk) {
  if (RuntimeEnabledFeatures::DisableRasterInvalidationEnabled())
    return;

  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
  if (new_chunk.begin_index >=
      current_cached_subsequence_begin_index_in_new_list_)
    return;

  // Uncacheable chunks will be invalidated in ContentLayerClientImpl.
  if (!new_chunk.is_cacheable)
    return;

  // Try to match old chunk sequentially first.
  const auto& old_chunks = current_paint_artifact_.PaintChunks();
  while (next_chunk_to_match_ < old_chunks.size()) {
    const PaintChunk& old_chunk = old_chunks[next_chunk_to_match_];
    if (new_chunk.Matches(old_chunk)) {
      GenerateRasterInvalidationsComparingChunks(new_chunk, old_chunk);
      ++next_chunk_to_match_;
      return;
    }

    // Add skipped old chunks into the index.
    if (old_chunk.is_cacheable) {
      auto it = out_of_order_chunk_indices_.find(&old_chunk.id.client);
      Vector<size_t>& indices =
          it == out_of_order_chunk_indices_.end()
              ? out_of_order_chunk_indices_
                    .insert(&old_chunk.id.client, Vector<size_t>())
                    .stored_value->value
              : it->value;
      indices.push_back(next_chunk_to_match_);
    }
    ++next_chunk_to_match_;
  }

  // Sequential matching reaches the end. Find from the out-of-order index.
  auto it = out_of_order_chunk_indices_.find(&new_chunk.id.client);
  if (it != out_of_order_chunk_indices_.end()) {
    for (size_t i : it->value) {
      if (new_chunk.Matches(old_chunks[i])) {
        GenerateRasterInvalidationsComparingChunks(new_chunk, old_chunks[i]);
        return;
      }
    }
  }
}

void PaintController::AddRasterInvalidation(const DisplayItemClient& client,
                                            PaintChunk& chunk,
                                            const FloatRect& rect,
                                            PaintInvalidationReason reason) {
  new_paint_chunks_.AddRasterInvalidation(chunk, rect);
  if (RasterInvalidationTracking::ShouldAlwaysTrack())
    EnsureRasterInvalidationTracking();
  if (raster_invalidation_tracking_info_)
    TrackRasterInvalidation(client, chunk, reason);
}

void PaintController::TrackRasterInvalidation(const DisplayItemClient& client,
                                              PaintChunk& chunk,
                                              PaintInvalidationReason reason) {
  DCHECK(raster_invalidation_tracking_info_);

  RasterInvalidationInfo info;
  info.client = &client;
  if (reason == PaintInvalidationReason::kNone) {
    // The client was validated by another PaintController, but not valid in
    // this PaintController.
    DCHECK(!ClientCacheIsValid(client));
    info.reason = PaintInvalidationReason::kFull;
  } else {
    info.reason = reason;
  }

  if (reason == PaintInvalidationReason::kDisappeared) {
    info.client_debug_name =
        raster_invalidation_tracking_info_->old_client_debug_names.at(&client);
  } else {
    info.client_debug_name = client.DebugName();
  }

  new_paint_chunks_.TrackRasterInvalidation(chunk, info);
}

void PaintController::GenerateRasterInvalidationsComparingChunks(
    PaintChunk& new_chunk,
    const PaintChunk& old_chunk) {
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());

  // TODO(wangxianzhu): Optimize paint offset change.

  struct OldAndNewDisplayItems {
    const DisplayItem* old_item = nullptr;
    const DisplayItem* new_item = nullptr;
  };
  HashMap<const DisplayItemClient*, OldAndNewDisplayItems>
      clients_to_invalidate;

  size_t highest_moved_to_index = 0;
  // Find clients to invalidate the old visual rects from the old chunk.
  for (size_t old_index = old_chunk.begin_index;
       old_index < old_chunk.end_index; ++old_index) {
    const DisplayItem& old_item =
        current_paint_artifact_.GetDisplayItemList()[old_index];
    const DisplayItemClient* client_to_invalidate_old_visual_rect = nullptr;

    if (old_item.IsTombstone()) {
      // old_item has been moved into new_display_item_list_ as a cached item.
      size_t moved_to_index = items_moved_into_new_list_[old_index];
      if (new_display_item_list_[moved_to_index].DrawsContent()) {
        if (moved_to_index < new_chunk.begin_index ||
            moved_to_index >= new_chunk.end_index) {
          // The item has been moved into another chunk, so need to invalidate
          // it in the chunk into which the item was moved.
          const auto& new_item = new_display_item_list_[moved_to_index];
          PaintChunk& moved_to_chunk =
              new_paint_chunks_.FindChunkByDisplayItemIndex(moved_to_index);
          AddRasterInvalidation(new_item.Client(), moved_to_chunk,
                                new_item.VisualRect(),
                                PaintInvalidationReason::kAppeared);
          // And invalidate the old visual rect in this chunk.
          client_to_invalidate_old_visual_rect = &new_item.Client();
        } else if (moved_to_index < highest_moved_to_index) {
          // The item has been moved behind other cached items, so need to
          // invalidate the area that is probably exposed by the item moved
          // earlier.
          client_to_invalidate_old_visual_rect =
              &new_display_item_list_[moved_to_index].Client();
        } else {
          highest_moved_to_index = moved_to_index;
        }
      }
    } else if (old_item.DrawsContent()) {
      // old_item has either changed or disappeared.
      client_to_invalidate_old_visual_rect = &old_item.Client();
    }

    if (client_to_invalidate_old_visual_rect) {
      clients_to_invalidate
          .insert(client_to_invalidate_old_visual_rect, OldAndNewDisplayItems())
          .stored_value->value.old_item = &old_item;
    }
  }

  // Find clients to invalidate the new visual rects from the new chunk.
  for (size_t new_index = new_chunk.begin_index;
       new_index < new_chunk.end_index; ++new_index) {
    const DisplayItem& new_item = new_display_item_list_[new_index];
    if (new_item.DrawsContent() && !ClientCacheIsValid(new_item.Client())) {
      clients_to_invalidate.insert(&new_item.Client(), OldAndNewDisplayItems())
          .stored_value->value.new_item = &new_item;
    }
  }

  for (const auto& item : clients_to_invalidate) {
    GenerateRasterInvalidation(*item.key, new_chunk, item.value.old_item,
                               item.value.new_item);
  }
}

void PaintController::GenerateRasterInvalidation(
    const DisplayItemClient& client,
    PaintChunk& chunk,
    const DisplayItem* old_item,
    const DisplayItem* new_item) {
  if (!new_item || new_item->VisualRect().IsEmpty()) {
    if (old_item && !old_item->VisualRect().IsEmpty()) {
      AddRasterInvalidation(client, chunk, old_item->VisualRect(),
                            PaintInvalidationReason::kDisappeared);
    }
    return;
  }

  DCHECK(&client == &new_item->Client());
  if (!old_item || old_item->VisualRect().IsEmpty()) {
    AddRasterInvalidation(client, chunk, new_item->VisualRect(),
                          PaintInvalidationReason::kAppeared);
    return;
  }

  if (client.IsJustCreated()) {
    // The old client has been deleted and the new client happens to be at the
    // same address. They have no relationship.
    AddRasterInvalidation(client, chunk, old_item->VisualRect(),
                          PaintInvalidationReason::kDisappeared);
    AddRasterInvalidation(client, chunk, new_item->VisualRect(),
                          PaintInvalidationReason::kAppeared);
    return;
  }

  auto reason = client.GetPaintInvalidationReason();
  bool partial_raster_invalidation =
      RuntimeEnabledFeatures::PartialRasterInvalidationEnabled() &&
      (reason == PaintInvalidationReason::kRectangle ||
       reason == PaintInvalidationReason::kSelection);
  if ((!partial_raster_invalidation &&
       reason != PaintInvalidationReason::kIncremental) ||
      // Need full invalidation when visual rect location changed.
      old_item->VisualRect().Location() != new_item->VisualRect().Location()) {
    GenerateFullRasterInvalidation(chunk, *old_item, *new_item);
    return;
  }

  GenerateIncrementalRasterInvalidation(chunk, *old_item, *new_item);

  if (RuntimeEnabledFeatures::PartialRasterInvalidationEnabled()) {
    auto partial_rect = client.PartialInvalidationRect();
    if (!partial_rect.IsEmpty())
      AddRasterInvalidation(client, chunk, FloatRect(partial_rect), reason);
  }
}

static FloatRect ComputeRightDelta(const FloatPoint& location,
                                   const FloatSize& old_size,
                                   const FloatSize& new_size) {
  float delta = new_size.Width() - old_size.Width();
  if (delta > 0) {
    return FloatRect(location.X() + old_size.Width(), location.Y(), delta,
                     new_size.Height());
  }
  if (delta < 0) {
    return FloatRect(location.X() + new_size.Width(), location.Y(), -delta,
                     old_size.Height());
  }
  return FloatRect();
}

static FloatRect ComputeBottomDelta(const FloatPoint& location,
                                    const FloatSize& old_size,
                                    const FloatSize& new_size) {
  float delta = new_size.Height() - old_size.Height();
  if (delta > 0) {
    return FloatRect(location.X(), location.Y() + old_size.Height(),
                     new_size.Width(), delta);
  }
  if (delta < 0) {
    return FloatRect(location.X(), location.Y() + new_size.Height(),
                     old_size.Width(), -delta);
  }
  return FloatRect();
}

void PaintController::GenerateIncrementalRasterInvalidation(
    PaintChunk& chunk,
    const DisplayItem& old_item,
    const DisplayItem& new_item) {
  DCHECK(&old_item.Client() == &new_item.Client());
  FloatRect old_visual_rect(old_item.VisualRect());
  FloatRect new_visual_rect(new_item.VisualRect());
  DCHECK(old_visual_rect.Location() == new_visual_rect.Location());

  FloatRect right_delta =
      ComputeRightDelta(new_visual_rect.Location(), old_visual_rect.Size(),
                        new_visual_rect.Size());
  if (!right_delta.IsEmpty()) {
    AddRasterInvalidation(new_item.Client(), chunk, right_delta,
                          PaintInvalidationReason::kIncremental);
  }

  FloatRect bottom_delta =
      ComputeBottomDelta(new_visual_rect.Location(), old_visual_rect.Size(),
                         new_visual_rect.Size());
  if (!bottom_delta.IsEmpty()) {
    AddRasterInvalidation(new_item.Client(), chunk, bottom_delta,
                          PaintInvalidationReason::kIncremental);
  }
}

void PaintController::GenerateFullRasterInvalidation(
    PaintChunk& chunk,
    const DisplayItem& old_item,
    const DisplayItem& new_item) {
  DCHECK(&old_item.Client() == &new_item.Client());
  FloatRect old_visual_rect(old_item.VisualRect());
  FloatRect new_visual_rect(new_item.VisualRect());

  if (!new_visual_rect.Contains(old_visual_rect)) {
    AddRasterInvalidation(new_item.Client(), chunk, old_visual_rect,
                          new_item.Client().GetPaintInvalidationReason());
    if (old_visual_rect.Contains(new_visual_rect))
      return;
  }

  AddRasterInvalidation(new_item.Client(), chunk, new_visual_rect,
                        new_item.Client().GetPaintInvalidationReason());
}

void PaintController::ShowUnderInvalidationError(
    const char* reason,
    const DisplayItem& new_item,
    const DisplayItem* old_item) const {
  LOG(ERROR) << under_invalidation_message_prefix_ << " " << reason;
#if DCHECK_IS_ON()
  LOG(ERROR) << "New display item: " << new_item.AsDebugString();
  LOG(ERROR) << "Old display item: "
             << (old_item ? old_item->AsDebugString() : "None");
  LOG(ERROR) << "See http://crbug.com/619103.";

  const PaintRecord* new_record = nullptr;
  if (new_item.IsDrawing()) {
    new_record =
        static_cast<const DrawingDisplayItem&>(new_item).GetPaintRecord().get();
  }
  const PaintRecord* old_record = nullptr;
  if (old_item->IsDrawing()) {
    old_record = static_cast<const DrawingDisplayItem*>(old_item)
                     ->GetPaintRecord()
                     .get();
  }
  LOG(INFO) << "new record:\n"
            << (new_record ? RecordAsDebugString(*new_record).Utf8().data()
                           : "None");
  LOG(INFO) << "old record:\n"
            << (old_record ? RecordAsDebugString(*old_record).Utf8().data()
                           : "None");

  ShowDebugData();
#else
  LOG(ERROR) << "Run a build with DCHECK on to get more details.";
  LOG(ERROR) << "See http://crbug.com/619103.";
#endif
}

void PaintController::ShowSequenceUnderInvalidationError(
    const char* reason,
    const DisplayItemClient& client,
    int start,
    int end) {
  LOG(ERROR) << under_invalidation_message_prefix_ << " " << reason;
  LOG(ERROR) << "Subsequence client: " << client.DebugName();
#if DCHECK_IS_ON()
  ShowDebugData();
#else
  LOG(ERROR) << "Run a build with DCHECK on to get more details.";
#endif
  LOG(ERROR) << "See http://crbug.com/619103.";
}

void PaintController::CheckUnderInvalidation() {
  DCHECK(RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled());

  if (!IsCheckingUnderInvalidation())
    return;

  const DisplayItem& new_item = new_display_item_list_.Last();
  if (new_item.SkippedCache()) {
    // We allow cache skipping and temporary under-invalidation in cached
    // subsequences. See the usage of DisplayItemCacheSkipper in BoxPainter.
    under_invalidation_checking_end_ = 0;
    // Match the remaining display items in the subsequence normally.
    next_item_to_match_ = next_item_to_index_ =
        under_invalidation_checking_begin_;
    return;
  }

  size_t old_item_index = under_invalidation_checking_begin_ +
                          skipped_probable_under_invalidation_count_;
  DisplayItem* old_item =
      old_item_index < current_paint_artifact_.GetDisplayItemList().size()
          ? &current_paint_artifact_.GetDisplayItemList()[old_item_index]
          : nullptr;

  bool old_and_new_equal = (old_item && new_item.Equals(*old_item));
  if (!old_and_new_equal) {
    if (new_item.IsBegin()) {
      // Temporarily skip mismatching begin display item which may be removed
      // when we remove a no-op pair.
      ++skipped_probable_under_invalidation_count_;
      return;
    }
    if (new_item.IsDrawing() &&
        skipped_probable_under_invalidation_count_ == 1) {
      DCHECK_GE(new_display_item_list_.size(), 2u);
      if (new_display_item_list_[new_display_item_list_.size() - 2].GetType() ==
          DisplayItem::kBeginCompositing) {
        // This might be a drawing item between a pair of begin/end compositing
        // display items that will be folded into a single drawing display item.
        ++skipped_probable_under_invalidation_count_;
        return;
      }
    }
  }

  if (skipped_probable_under_invalidation_count_ || !old_and_new_equal) {
    // If we ever skipped reporting any under-invalidations, report the earliest
    // one.
    ShowUnderInvalidationError(
        "under-invalidation: display item changed",
        new_display_item_list_[new_display_item_list_.size() -
                               skipped_probable_under_invalidation_count_ - 1],
        &current_paint_artifact_
             .GetDisplayItemList()[under_invalidation_checking_begin_]);
    CHECK(false);
  }

  // Discard the forced repainted display item and move the cached item into
  // new_display_item_list_. This is to align with the
  // non-under-invalidation-checking path to empty the original cached slot,
  // leaving only disappeared or invalidated display items in the old list after
  // painting.
  new_display_item_list_.RemoveLast();
  MoveItemFromCurrentListToNewList(old_item_index);

  ++under_invalidation_checking_begin_;
}

void PaintController::SetFirstPainted() {
  frame_first_paints_.back().first_painted = true;
}

void PaintController::SetTextPainted() {
  frame_first_paints_.back().text_painted = true;
}

void PaintController::SetImagePainted() {
  frame_first_paints_.back().image_painted = true;
}

void PaintController::BeginFrame(const void* frame) {
  frame_first_paints_.push_back(FrameFirstPaint(frame));
}

FrameFirstPaint PaintController::EndFrame(const void* frame) {
  FrameFirstPaint result = frame_first_paints_.back();
  DCHECK(result.frame == frame);
  frame_first_paints_.pop_back();
  return result;
}

#if DCHECK_IS_ON()
void PaintController::CheckDuplicatePaintChunkId(const PaintChunk::Id& id) {
  if (IsSkippingCache())
    return;

  auto it = new_paint_chunk_indices_by_client_.find(&id.client);
  if (it != new_paint_chunk_indices_by_client_.end()) {
    const auto& indices = it->value;
    for (auto index : indices) {
      const auto& chunk = new_paint_chunks_.PaintChunkAt(index);
      if (chunk.id == id) {
        ShowDebugData();
        NOTREACHED() << "New paint chunk id " << id.ToString().Utf8().data()
                     << " has duplicated id with previous chuck "
                     << chunk.ToString().Utf8().data();
      }
    }
  }
}
#endif

}  // namespace blink
