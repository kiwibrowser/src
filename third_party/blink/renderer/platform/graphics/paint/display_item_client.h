// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_DISPLAY_ITEM_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_DISPLAY_ITEM_CLIENT_H_

#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint_invalidation_reason.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// The class for objects that can be associated with display items. A
// DisplayItemClient object should live at least longer than the document cycle
// in which its display items are created during painting. After the document
// cycle, a pointer/reference to DisplayItemClient should be no longer
// dereferenced unless we can make sure the client is still valid.
class PLATFORM_EXPORT DisplayItemClient {
 public:
#if DCHECK_IS_ON()
  DisplayItemClient();
  virtual ~DisplayItemClient();

  // Tests if this DisplayItemClient object has been created and has not been
  // deleted yet.
  bool IsAlive() const;
  static String SafeDebugName(const DisplayItemClient&, bool known_to_be_safe);
#else
  DisplayItemClient() {}
  virtual ~DisplayItemClient() {}
#endif

  virtual String DebugName() const = 0;

  // The visual rect of this DisplayItemClient. For SPv1, it's in the object
  // space of the object that owns the GraphicsLayer, i.e. offset by
  // GraphicsLayer::OffsetFromLayoutObjectWithSubpixelAccumulation().
  // For SPv2, it's in the space of the parent transform node.
  virtual LayoutRect VisualRect() const = 0;

  // The outset will be used to inflate visual rect after the visual rect is
  // mapped into the space of the composited layer, for any special raster
  // effects that might expand the rastered pixel area.
  virtual float VisualRectOutsetForRasterEffects() const { return 0; }

  // The rect that needs to be invalidated partially in this client. It's in the
  // same coordinate space as VisualRect().
  virtual LayoutRect PartialInvalidationRect() const { return LayoutRect(); }

  // Called by PaintController::CommitNewDisplayItems() for all clients after
  // painting.
  virtual void ClearPartialInvalidationRect() const {}

  // This is declared here instead of in LayoutObject for verifying the
  // condition in DrawingRecorder.
  // Returns true if the object itself will not generate any effective painted
  // output no matter what size the object is. For example, this function can
  // return false for an object whose size is currently 0x0 but would have
  // effective painted output if it was set a non-empty size. It's used to skip
  // unforced paint invalidation of LayoutObjects (which is when
  // shouldDoFullPaintInvalidation is false, but mayNeedPaintInvalidation or
  // childShouldCheckForPaintInvalidation is true) to avoid unnecessary paint
  // invalidations of empty areas covered by such objects.
  virtual bool PaintedOutputOfObjectHasNoEffectRegardlessOfSize() const {
    return false;
  }

  // Indicates that the client will paint display items different from the ones
  // cached by PaintController. However, PaintController allows a client to
  // paint new display items that are not cached or to no longer paint some
  // cached display items without calling this method.
  // See PaintController::ClientCacheIsValid() for more details.
  void SetDisplayItemsUncached(
      PaintInvalidationReason reason = PaintInvalidationReason::kFull) const {
    cache_generation_or_invalidation_reason_.Invalidate(reason);
  }

  PaintInvalidationReason GetPaintInvalidationReason() const {
    return cache_generation_or_invalidation_reason_
        .GetPaintInvalidationReason();
  }

  // A client is considered "just created" if its display items have never been
  // committed.
  bool IsJustCreated() const {
    return cache_generation_or_invalidation_reason_.IsJustCreated();
  }
  void ClearIsJustCreated() const {
    cache_generation_or_invalidation_reason_.ClearIsJustCreated();
  }

 private:
  friend class FakeDisplayItemClient;
  friend class PaintController;

  // Holds a unique cache generation id of DisplayItemClients and
  // PaintControllers, or PaintInvalidationReason if the DisplayItemClient or
  // PaintController is invalidated.
  //
  // A paint controller sets its cache generation to
  // DisplayItemCacheGeneration::next() at the end of each
  // commitNewDisplayItems, and updates the cache generation of each client with
  // cached drawings by calling DisplayItemClient::setDisplayItemsCached(). A
  // display item is treated as validly cached in a paint controller if its
  // cache generation matches the paint controller's cache generation.
  //
  // SPv1 only: If a display item is painted on multiple paint controllers,
  // because cache generations are unique, the client's cache generation matches
  // the last paint controller only. The client will be treated as invalid on
  // other paint controllers regardless if it's validly cached by these paint
  // controllers. This situation is very rare (about 0.07% of clients were
  // painted on multiple paint controllers) so the performance penalty is
  // trivial.
  class PLATFORM_EXPORT CacheGenerationOrInvalidationReason {
    DISALLOW_NEW();

   public:
    CacheGenerationOrInvalidationReason() : value_(kJustCreated) {}

    void Invalidate(
        PaintInvalidationReason reason = PaintInvalidationReason::kFull) {
      if (value_ != kJustCreated)
        value_ = static_cast<ValueType>(reason);
    }

    static CacheGenerationOrInvalidationReason Next() {
      // In case the value overflowed in the previous call.
      if (next_generation_ < kFirstValidGeneration)
        next_generation_ = kFirstValidGeneration;
      return CacheGenerationOrInvalidationReason(next_generation_++);
    }

    bool Matches(const CacheGenerationOrInvalidationReason& other) const {
      return value_ >= kFirstValidGeneration &&
             other.value_ >= kFirstValidGeneration && value_ == other.value_;
    }

    PaintInvalidationReason GetPaintInvalidationReason() const {
      if (value_ == kJustCreated)
        return PaintInvalidationReason::kAppeared;
      if (value_ < kJustCreated)
        return static_cast<PaintInvalidationReason>(value_);
      return PaintInvalidationReason::kNone;
    }

    bool IsJustCreated() const { return value_ == kJustCreated; }
    void ClearIsJustCreated() {
      value_ = static_cast<ValueType>(PaintInvalidationReason::kFull);
    }

   private:
    typedef uint32_t ValueType;
    explicit CacheGenerationOrInvalidationReason(ValueType value)
        : value_(value) {}

    static const ValueType kJustCreated =
        static_cast<ValueType>(PaintInvalidationReason::kMax) + 1;
    static const ValueType kFirstValidGeneration =
        static_cast<ValueType>(PaintInvalidationReason::kMax) + 2;
    static ValueType next_generation_;
    ValueType value_;
  };

  bool DisplayItemsAreCached(CacheGenerationOrInvalidationReason other) const {
    return cache_generation_or_invalidation_reason_.Matches(other);
  }
  void SetDisplayItemsCached(
      CacheGenerationOrInvalidationReason cache_generation) const {
    cache_generation_or_invalidation_reason_ = cache_generation;
  }

  mutable CacheGenerationOrInvalidationReason
      cache_generation_or_invalidation_reason_;

  DISALLOW_COPY_AND_ASSIGN(DisplayItemClient);
};

inline bool operator==(const DisplayItemClient& client1,
                       const DisplayItemClient& client2) {
  return &client1 == &client2;
}
inline bool operator!=(const DisplayItemClient& client1,
                       const DisplayItemClient& client2) {
  return &client1 != &client2;
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_DISPLAY_ITEM_CLIENT_H_
