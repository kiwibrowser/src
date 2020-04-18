// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CONTROLLER_H_

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/geometry/layout_point.h"
#include "third_party/blink/renderer/platform/graphics/contiguous_container.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item_list.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_artifact.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_chunk.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_chunker.h"
#include "third_party/blink/renderer/platform/graphics/paint/raster_invalidation_tracking.h"
#include "third_party/blink/renderer/platform/graphics/paint/transform_3d_display_item.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/alignment.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace blink {

static const size_t kInitialDisplayItemListCapacityBytes = 512;

// FrameFirstPaint stores first-paint, text or image painted for the
// corresponding frame. They are never reset to false. First-paint is defined in
// https://github.com/WICG/paint-timing. It excludes default background paint.
struct FrameFirstPaint {
  FrameFirstPaint(const void* frame)
      : frame(frame),
        first_painted(false),
        text_painted(false),
        image_painted(false) {}

  const void* frame;
  bool first_painted : 1;
  bool text_painted : 1;
  bool image_painted : 1;
};

// Responsible for processing display items as they are produced, and producing
// a final paint artifact when complete. This class includes logic for caching,
// cache invalidation, and merging.
class PLATFORM_EXPORT PaintController {
  WTF_MAKE_NONCOPYABLE(PaintController);
  USING_FAST_MALLOC(PaintController);

 public:
  static std::unique_ptr<PaintController> Create() {
    return base::WrapUnique(new PaintController());
  }

  ~PaintController() {
    // New display items should be committed before PaintController is
    // destructed.
    DCHECK(new_display_item_list_.IsEmpty());
  }

  // For SPv1 only.
  void InvalidateAll();
  bool CacheIsAllInvalid() const;

  // These methods are called during painting.

  // Provide a new set of paint chunk properties to apply to recorded display
  // items, for Slimming Paint v175+.
  void UpdateCurrentPaintChunkProperties(
      const base::Optional<PaintChunk::Id>& id,
      const PropertyTreeState& properties) {
    if (id) {
      PaintChunk::Id id_with_fragment(*id, current_fragment_);
      UpdateCurrentPaintChunkPropertiesUsingIdWithFragment(id_with_fragment,
                                                           properties);
#if DCHECK_IS_ON()
      CheckDuplicatePaintChunkId(id_with_fragment);
#endif
    } else {
      new_paint_chunks_.UpdateCurrentPaintChunkProperties(base::nullopt,
                                                          properties);
    }
  }

  const PropertyTreeState& CurrentPaintChunkProperties() const {
    return new_paint_chunks_.CurrentPaintChunkProperties();
  }

  void ForceNewChunk(const DisplayItemClient& client, DisplayItem::Type type) {
    new_paint_chunks_.ForceNewChunk();
    new_paint_chunks_.UpdateCurrentPaintChunkProperties(
        PaintChunk::Id(client, type), CurrentPaintChunkProperties());
  }

  template <typename DisplayItemClass, typename... Args>
  void CreateAndAppend(Args&&... args) {
    static_assert(WTF::IsSubclass<DisplayItemClass, DisplayItem>::value,
                  "Can only createAndAppend subclasses of DisplayItem.");
    static_assert(
        sizeof(DisplayItemClass) <= kMaximumDisplayItemSize,
        "DisplayItem subclass is larger than kMaximumDisplayItemSize.");

    if (DisplayItemConstructionIsDisabled())
      return;

    EnsureNewDisplayItemListInitialCapacity();
    DisplayItemClass& display_item =
        new_display_item_list_.AllocateAndConstruct<DisplayItemClass>(
            std::forward<Args>(args)...);
    display_item.SetFragment(current_fragment_);
    ProcessNewItem(display_item);
  }

  // Creates and appends an ending display item to pair with a preceding
  // beginning item iff the display item actually draws content. For no-op
  // items, rather than creating an ending item, the begin item will
  // instead be removed, thereby maintaining brevity of the list. If display
  // item construction is disabled, no list mutations will be performed.
  template <typename DisplayItemClass, typename... Args>
  void EndItem(Args&&... args) {
    DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());

    if (DisplayItemConstructionIsDisabled())
      return;
    if (LastDisplayItemIsNoopBegin())
      RemoveLastDisplayItem();
    else
      CreateAndAppend<DisplayItemClass>(std::forward<Args>(args)...);
  }

  // Tries to find the cached drawing display item corresponding to the given
  // parameters. If found, appends the cached display item to the new display
  // list and returns true. Otherwise returns false.
  bool UseCachedDrawingIfPossible(const DisplayItemClient&, DisplayItem::Type);

  // Tries to find the cached subsequence corresponding to the given parameters.
  // If found, copies the cache subsequence to the new display list and returns
  // true. Otherwise returns false.
  bool UseCachedSubsequenceIfPossible(const DisplayItemClient&);

  size_t BeginSubsequence();
  // The |start| parameter should be the return value of the corresponding
  // BeginSubsequence().
  void EndSubsequence(const DisplayItemClient&, size_t start);

  // True if the last display item is a begin that doesn't draw content.
  void RemoveLastDisplayItem();
  const DisplayItem* LastDisplayItem(unsigned offset);

  void BeginSkippingCache() { ++skipping_cache_count_; }
  void EndSkippingCache() {
    DCHECK(skipping_cache_count_ > 0);
    --skipping_cache_count_;
  }
  bool IsSkippingCache() const { return skipping_cache_count_; }

  // Must be called when a painting is finished.
  void CommitNewDisplayItems();

  // Called when the caller finishes updating a full document life cycle.
  // The PaintController will cleanup data that will no longer be used for the
  // next cycle, and update status to be ready for the next cycle.
  void FinishCycle();

  // Returns the approximate memory usage, excluding memory likely to be
  // shared with the embedder after copying to WebPaintController.
  // Should only be called after a full document life cycle update.
  size_t ApproximateUnsharedMemoryUsage() const;

  // Get the artifact generated after the last commit.
  const PaintArtifact& GetPaintArtifact() const;
  const DisplayItemList& GetDisplayItemList() const {
    return GetPaintArtifact().GetDisplayItemList();
  }
  const Vector<PaintChunk>& PaintChunks() const {
    return GetPaintArtifact().PaintChunks();
  }

  // For micro benchmarking of record time.
  bool DisplayItemConstructionIsDisabled() const {
    return construction_disabled_;
  }
  void SetDisplayItemConstructionIsDisabled(const bool disable) {
    construction_disabled_ = disable;
  }
  bool SubsequenceCachingIsDisabled() const {
    return subsequence_caching_disabled_;
  }
  void SetSubsequenceCachingIsDisabled(bool disable) {
    subsequence_caching_disabled_ = disable;
  }

  void SetFirstPainted();
  void SetTextPainted();
  void SetImagePainted();

  // Returns DisplayItemList added using CreateAndAppend() since beginning or
  // the last CommitNewDisplayItems(). Use with care.
  DisplayItemList& NewDisplayItemList() { return new_display_item_list_; }

  void AppendDebugDrawingAfterCommit(const DisplayItemClient&,
                                     sk_sp<const PaintRecord>,
                                     const PropertyTreeState*);

#if DCHECK_IS_ON()
  void ShowDebugData() const;
  void ShowDebugDataWithRecords() const;
#endif

  void SetTracksRasterInvalidations(bool);

  bool LastDisplayItemIsSubsequenceEnd() const;

  void BeginFrame(const void* frame);
  FrameFirstPaint EndFrame(const void* frame);

  // The current fragment will be part of the ids of all display items and
  // paint chunks, to uniquely identify display items in different fragments
  // for the same client and type.
  unsigned CurrentFragment() const { return current_fragment_; }
  void SetCurrentFragment(unsigned fragment) { current_fragment_ = fragment; }

 protected:
  PaintController()
      : new_display_item_list_(0),
        construction_disabled_(false),
        subsequence_caching_disabled_(false),
        skipping_cache_count_(0),
        num_cached_new_items_(0),
        current_cached_subsequence_begin_index_in_new_list_(kNotFound),
#if DCHECK_IS_ON()
        num_sequential_matches_(0),
        num_out_of_order_matches_(0),
        num_indexed_items_(0),
#endif
        under_invalidation_checking_begin_(0),
        under_invalidation_checking_end_(0),
        last_cached_subsequence_end_(0),
        current_fragment_(0) {
    ResetCurrentListIndices();
    // frame_first_paints_ should have one null frame since the beginning, so
    // that PaintController is robust even if it paints outside of BeginFrame
    // and EndFrame cycles. It will also enable us to combine the first paint
    // data in this PaintController into another PaintController on which we
    // replay the recorded results in the future.
    frame_first_paints_.push_back(FrameFirstPaint(nullptr));
  }

 private:
  friend class PaintControllerTestBase;
  friend class PaintControllerPaintTestBase;

  // True if all display items associated with the client are validly cached.
  // However, the current algorithm allows the following situations even if
  // ClientCacheIsValid() is true for a client during painting:
  // 1. The client paints a new display item that is not cached:
  //    UseCachedDrawingIfPossible() returns false for the display item and the
  //    newly painted display item will be added into the cache. This situation
  //    has slight performance hit (see FindOutOfOrderCachedItemForward()) so we
  //    print a warning in the situation and should keep it rare.
  // 2. the client no longer paints a display item that is cached: the cached
  //    display item will be removed. This doesn't affect performance.
  bool ClientCacheIsValid(const DisplayItemClient&) const;

  void InvalidateAllForTesting() { InvalidateAllInternal(); }
  void InvalidateAllInternal();

  bool LastDisplayItemIsNoopBegin() const;

  void EnsureNewDisplayItemListInitialCapacity() {
    if (new_display_item_list_.IsEmpty()) {
      // TODO(wangxianzhu): Consider revisiting this heuristic.
      new_display_item_list_ =
          DisplayItemList(current_paint_artifact_.GetDisplayItemList().IsEmpty()
                              ? kInitialDisplayItemListCapacityBytes
                              : current_paint_artifact_.GetDisplayItemList()
                                    .UsedCapacityInBytes());
    }
  }

  // Set new item state (cache skipping, etc) for a new item.
  void ProcessNewItem(DisplayItem&);
  DisplayItem& MoveItemFromCurrentListToNewList(size_t);

  // Maps clients to indices of display items or chunks of each client.
  using IndicesByClientMap = HashMap<const DisplayItemClient*, Vector<size_t>>;

  static size_t FindMatchingItemFromIndex(const DisplayItem::Id&,
                                          const IndicesByClientMap&,
                                          const DisplayItemList&);
  static void AddToIndicesByClientMap(const DisplayItemClient&,
                                      size_t index,
                                      IndicesByClientMap&);

  size_t FindCachedItem(const DisplayItem::Id&);
  size_t FindOutOfOrderCachedItemForward(const DisplayItem::Id&);
  void CopyCachedSubsequence(size_t begin_index, size_t end_index);

  void UpdateCurrentPaintChunkPropertiesUsingIdWithFragment(
      const PaintChunk::Id& id_with_fragment,
      const PropertyTreeState& properties) {
    new_paint_chunks_.UpdateCurrentPaintChunkProperties(id_with_fragment,
                                                        properties);
  }

  // Resets the indices (e.g. next_item_to_match_) of
  // current_paint_artifact_.GetDisplayItemList() to their initial values. This
  // should be called when the DisplayItemList in current_paint_artifact_ is
  // newly created, or is changed causing the previous indices to be invalid.
  void ResetCurrentListIndices();

  void GenerateRasterInvalidations(PaintChunk& new_chunk);
  void GenerateRasterInvalidationsComparingChunks(PaintChunk& new_chunk,
                                                  const PaintChunk& old_chunk);
  inline void GenerateRasterInvalidation(const DisplayItemClient&,
                                         PaintChunk&,
                                         const DisplayItem* old_item,
                                         const DisplayItem* new_item);
  inline void GenerateIncrementalRasterInvalidation(
      PaintChunk&,
      const DisplayItem& old_item,
      const DisplayItem& new_item);
  inline void GenerateFullRasterInvalidation(PaintChunk&,
                                             const DisplayItem& old_item,
                                             const DisplayItem& new_item);
  inline void AddRasterInvalidation(const DisplayItemClient&,
                                    PaintChunk&,
                                    const FloatRect&,
                                    PaintInvalidationReason);
  void EnsureRasterInvalidationTracking();
  void TrackRasterInvalidation(const DisplayItemClient&,
                               PaintChunk&,
                               PaintInvalidationReason);

  // The following two methods are for checking under-invalidations
  // (when RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled).
  void ShowUnderInvalidationError(const char* reason,
                                  const DisplayItem& new_item,
                                  const DisplayItem* old_item) const;

  void ShowSequenceUnderInvalidationError(const char* reason,
                                          const DisplayItemClient&,
                                          int start,
                                          int end);

  void CheckUnderInvalidation();
  bool IsCheckingUnderInvalidation() const {
    return under_invalidation_checking_end_ >
           under_invalidation_checking_begin_;
  }

  struct SubsequenceMarkers {
    SubsequenceMarkers() : start(0), end(0) {}
    SubsequenceMarkers(size_t start_arg, size_t end_arg)
        : start(start_arg), end(end_arg) {}
    // The start and end (not included) index within current_paint_artifact_
    // of this subsequence.
    size_t start;
    size_t end;
  };

  SubsequenceMarkers* GetSubsequenceMarkers(const DisplayItemClient&);

#if DCHECK_IS_ON()
  void CheckDuplicatePaintChunkId(const PaintChunk::Id&);
  void ShowDebugDataInternal(DisplayItemList::JsonFlags) const;
#endif

  // The last complete paint artifact.
  // In SPv2, this includes paint chunks as well as display items.
  PaintArtifact current_paint_artifact_;

  // Data being used to build the next paint artifact.
  DisplayItemList new_display_item_list_;
  PaintChunker new_paint_chunks_;

  // Stores indices into new_display_item_list_ for display items that have been
  // moved from current_paint_artifact_.GetDisplayItemList(), indexed by the
  // positions of the display items before the move. The values are undefined
  // for display items that are not moved.
  Vector<size_t> items_moved_into_new_list_;

  // Allows display item construction to be disabled to isolate the costs of
  // construction in performance metrics.
  bool construction_disabled_;

  // Allows subsequence caching to be disabled to test the cost of display item
  // caching.
  bool subsequence_caching_disabled_;

  // A stack recording current frames' first paints.
  Vector<FrameFirstPaint> frame_first_paints_;

  int skipping_cache_count_;

  int num_cached_new_items_;

  // Stores indices to valid cacheable display items in
  // current_paint_artifact_.GetDisplayItemList() that have not been matched by
  // requests of cached display items (using UseCachedDrawingIfPossible() and
  // UseCachedSubsequenceIfPossible()) during sequential matching. The indexed
  // items will be matched by later out-of-order requests of cached display
  // items. This ensures that when out-of-order cached display items are
  // requested, we only traverse at most once over the current display list
  // looking for potential matches. Thus we can ensure that the algorithm runs
  // in linear time.
  IndicesByClientMap out_of_order_item_indices_;

  // The next item in the current list for sequential match.
  size_t next_item_to_match_;

  // The next item in the current list to be indexed for out-of-order cache
  // requests.
  size_t next_item_to_index_;

  // Similar to out_of_order_item_indices_ but
  // - the indices are chunk indices in current_paint_artifacts_.PaintChunks();
  // - chunks are matched not only for requests of cached display items, but
  //   also non-cached display items.
  IndicesByClientMap out_of_order_chunk_indices_;

  size_t current_cached_subsequence_begin_index_in_new_list_;
  size_t next_chunk_to_match_;

  DisplayItemClient::CacheGenerationOrInvalidationReason
      current_cache_generation_;

#if DCHECK_IS_ON()
  int num_sequential_matches_;
  int num_out_of_order_matches_;
  int num_indexed_items_;

  // This is used to check duplicated ids during CreateAndAppend().
  IndicesByClientMap new_display_item_indices_by_client_;
  // This is used to check duplicated ids for new paint chunks.
  IndicesByClientMap new_paint_chunk_indices_by_client_;
#endif

  // These are set in UseCachedDrawingIfPossible() and
  // UseCachedSubsequenceIfPossible() when we could use cached drawing or
  // subsequence and under-invalidation checking is on, indicating the begin and
  // end of the cached drawing or subsequence in the current list. The functions
  // return false to let the client do actual painting, and PaintController will
  // check if the actual painting results are the same as the cached.
  size_t under_invalidation_checking_begin_;
  size_t under_invalidation_checking_end_;

  // Number of probable under-invalidations that have been skipped temporarily
  // because the mismatching display items may be removed in the future because
  // of no-op pairs or compositing folding.
  int skipped_probable_under_invalidation_count_;
  String under_invalidation_message_prefix_;

  struct RasterInvalidationTrackingInfo {
    using ClientDebugNamesMap = HashMap<const DisplayItemClient*, String>;
    ClientDebugNamesMap new_client_debug_names;
    ClientDebugNamesMap old_client_debug_names;
  };
  std::unique_ptr<RasterInvalidationTrackingInfo>
      raster_invalidation_tracking_info_;

  using CachedSubsequenceMap =
      HashMap<const DisplayItemClient*, SubsequenceMarkers>;
  CachedSubsequenceMap current_cached_subsequences_;
  CachedSubsequenceMap new_cached_subsequences_;
  size_t last_cached_subsequence_end_;

  unsigned current_fragment_;

  class DisplayItemListAsJSON;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CONTROLLER_H_
