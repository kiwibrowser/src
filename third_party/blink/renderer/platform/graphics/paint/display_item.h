// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_DISPLAY_ITEM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_DISPLAY_ITEM_H_

#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/graphics/contiguous_container.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item_client.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

#if DCHECK_IS_ON()
#include "third_party/blink/renderer/platform/json/json_values.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#endif

namespace cc {
class DisplayItemList;
}

namespace blink {

class GraphicsContext;
class FloatSize;
enum class PaintPhase;

class PLATFORM_EXPORT DisplayItem {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  enum {
    // Must be kept in sync with core/paint/PaintPhase.h.
    kPaintPhaseMax = 11,
  };

  // A display item type uniquely identifies a display item of a client.
  // Some display item types can be categorized using the following directives:
  // - In enum Type:
  //   - enum value <Category>First;
  //   - enum values of the category, first of which should equal
  //     <Category>First (for ease of maintenance, the values should be in
  //     alphabetic order);
  //   - enum value <Category>Last which should be equal to the last of the enum
  //     values of the category
  // - DEFINE_CATEGORY_METHODS(<Category>) to define is<Category>Type(Type) and
  //   is<Category>() methods.
  //
  // A category or subset of a category can contain types each of which
  // corresponds to a PaintPhase:
  // - In enum Type:
  //   - enum value <Category>[<Subset>]PaintPhaseFirst;
  //   - enum value <Category>[<Subset>]PaintPhaseLast =
  //     <Category>[<Subset>]PaintPhaseFirst + PaintPhaseMax;
  // - DEFINE_PAINT_PHASE_CONVERSION_METHOD(<Category>[<Subset>]) to define
  //   paintPhaseTo<Category>[<Subset>]Type(PaintPhase) method.
  //
  // A category can be derived from another category, containing types each of
  // which corresponds to a value of the latter category:
  // - In enum Type:
  //   - enum value <Category>First;
  //   - enum value <Category>Last =
  //     <Category>First + <BaseCategory>Last - <BaseCategory>First;
  // - DEFINE_CONVERSION_METHODS(<Category>,
  //                             <category>,
  //                             <BaseCategory>,
  //                             <baseCategory>)
  //   to define methods to convert types between the categories.
  enum Type {
    kDrawingFirst,
    kDrawingPaintPhaseFirst = kDrawingFirst,
    kDrawingPaintPhaseLast = kDrawingFirst + kPaintPhaseMax,
    kBoxDecorationBackground,
    kCaret,
    kColumnRules,
    kDebugDrawing,
    kDocumentBackground,
    kDragImage,
    kDragCaret,
    kEmptyContentForFilters,
    kSVGImage,
    kLinkHighlight,
    kImageAreaFocusRing,
    kPageOverlay,
    kPopupContainerBorder,
    kPopupListBoxBackground,
    kPopupListBoxRow,
    kPrintedContentDestinationLocations,
    kPrintedContentPDFURLRect,
    kResizer,
    kSVGClip,
    kSVGFilter,
    kSVGMask,
    kScrollbarBackButtonEnd,
    kScrollbarBackButtonStart,
    kScrollbarBackground,
    kScrollbarBackTrack,
    kScrollbarCorner,
    kScrollbarForwardButtonEnd,
    kScrollbarForwardButtonStart,
    kScrollbarForwardTrack,
    kScrollbarThumb,
    kScrollbarTickmarks,
    kScrollbarTrackBackground,
    kScrollbarCompositedScrollbar,
    kSelectionTint,
    kTableCollapsedBorders,
    kVideoBitmap,
    kWebPlugin,
    kWebFont,
    kReflectionMask,
    kDrawingLast = kReflectionMask,

    kForeignLayerFirst,
    kForeignLayerCanvas = kForeignLayerFirst,
    kForeignLayerPlugin,
    kForeignLayerVideo,
    kForeignLayerWrapper,
    kForeignLayerContentsWrapper,
    kForeignLayerLast = kForeignLayerContentsWrapper,

    kClipFirst,
    kClipBoxPaintPhaseFirst = kClipFirst,
    kClipBoxPaintPhaseLast = kClipBoxPaintPhaseFirst + kPaintPhaseMax,
    kClipColumnBoundsPaintPhaseFirst,
    kClipColumnBoundsPaintPhaseLast =
        kClipColumnBoundsPaintPhaseFirst + kPaintPhaseMax,
    kClipLayerFragmentPaintPhaseFirst,
    kClipLayerFragmentPaintPhaseLast =
        kClipLayerFragmentPaintPhaseFirst + kPaintPhaseMax,
    kClipFileUploadControlRect,
    kClipFrameToVisibleContentRect,
    kClipFrameScrollbars,
    kClipLayerBackground,
    kClipLayerColumnBounds,
    kClipLayerFilter,
    kClipLayerForeground,
    kClipLayerParent,
    kClipLayerOverflowControls,
    kClipPopupListBoxFrame,
    kClipScrollbarsToBoxBounds,
    kClipSelectionImage,
    kClipLast = kClipSelectionImage,

    kEndClipFirst,
    kEndClipLast = kEndClipFirst + kClipLast - kClipFirst,

    kFloatClipFirst,
    kFloatClipPaintPhaseFirst = kFloatClipFirst,
    kFloatClipPaintPhaseLast = kFloatClipFirst + kPaintPhaseMax,
    kFloatClipClipPathBounds,
    kFloatClipLast = kFloatClipClipPathBounds,
    kEndFloatClipFirst,
    kEndFloatClipLast = kEndFloatClipFirst + kFloatClipLast - kFloatClipFirst,

    kScrollFirst,
    kScrollPaintPhaseFirst = kScrollFirst,
    kScrollPaintPhaseLast = kScrollPaintPhaseFirst + kPaintPhaseMax,
    kScrollOverflowControls,
    kScrollLast = kScrollOverflowControls,
    kEndScrollFirst,
    kEndScrollLast = kEndScrollFirst + kScrollLast - kScrollFirst,

    kSVGTransformPaintPhaseFirst,
    kSVGTransformPaintPhaseLast = kSVGTransformPaintPhaseFirst + kPaintPhaseMax,

    kSVGEffectPaintPhaseFirst,
    kSVGEffectPaintPhaseLast = kSVGEffectPaintPhaseFirst + kPaintPhaseMax,

    kTransform3DFirst,
    kTransform3DElementTransform = kTransform3DFirst,
    kTransform3DLast = kTransform3DElementTransform,
    kEndTransform3DFirst,
    kEndTransform3DLast =
        kEndTransform3DFirst + kTransform3DLast - kTransform3DFirst,

    kBeginFilter,
    kEndFilter,
    kBeginCompositing,
    kEndCompositing,
    kBeginTransform,
    kEndTransform,
    kBeginClipPath,
    kEndClipPath,
    kScrollHitTest,

    kLayerChunkBackground,
    kLayerChunkNegativeZOrderChildren,
    kLayerChunkDescendantBackgrounds,
    kLayerChunkFloat,
    kLayerChunkForeground,
    kLayerChunkNormalFlowAndPositiveZOrderChildren,

    kUninitializedType,
    kTypeLast = kUninitializedType
  };

  DisplayItem(const DisplayItemClient& client, Type type, size_t derived_size)
      : client_(&client),
        visual_rect_(client.VisualRect()),
        outset_for_raster_effects_(client.VisualRectOutsetForRasterEffects()),
        type_(type),
        derived_size_(derived_size),
        fragment_(0),
        skipped_cache_(false),
        is_tombstone_(false) {
    // |derived_size| must fit in |derived_size_|.
    // If it doesn't, enlarge |derived_size_| and fix this assert.
    SECURITY_DCHECK(derived_size < (1 << 8));
    SECURITY_DCHECK(derived_size >= sizeof(*this));
  }

  virtual ~DisplayItem() = default;

  // Ids are for matching new DisplayItems with existing DisplayItems.
  struct Id {
    DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();
    Id(const DisplayItemClient& client, const Type type, unsigned fragment = 0)
        : client(client), type(type), fragment(fragment) {}
    Id(const Id& id, unsigned fragment)
        : client(id.client), type(id.type), fragment(fragment) {}

    String ToString() const;

    const DisplayItemClient& client;
    const Type type;
    const unsigned fragment;
  };

  Id GetId() const { return Id(*client_, GetType(), fragment_); }

  virtual void Replay(GraphicsContext&) const {}

  const DisplayItemClient& Client() const {
    DCHECK(client_);
    return *client_;
  }

  // This equals to Client().VisualRect() as long as the client is alive and is
  // not invalidated. Otherwise it saves the previous visual rect of the client.
  // See DisplayItemClient::VisualRect() about its coordinate space.
  const FloatRect& VisualRect() const { return visual_rect_; }
  float OutsetForRasterEffects() const { return outset_for_raster_effects_; }

  // Visual rect can change without needing invalidation of the client, e.g.
  // when ancestor clip changes. This is called from PaintController::
  // UseCachedDrawingIfPossible() to update the visual rect of a cached display
  // item.
  void UpdateVisualRect() { visual_rect_ = FloatRect(client_->VisualRect()); }

  Type GetType() const { return static_cast<Type>(type_); }

  // Size of this object in memory, used to move it with memcpy.
  // This is not sizeof(*this), because it needs to account for the size of
  // the derived class (i.e. runtime type). Derived classes are expected to
  // supply this to the DisplayItem constructor.
  size_t DerivedSize() const { return derived_size_; }

  // The fragment is part of the id, to uniquely identify display items in
  // different fragments for the same client and type.
  unsigned Fragment() const { return fragment_; }
  void SetFragment(unsigned fragment) {
    DCHECK(fragment < (1 << 14));
    fragment_ = fragment;
  }

  // For PaintController only. Painters should use DisplayItemCacheSkipper
  // instead.
  void SetSkippedCache() { skipped_cache_ = true; }
  bool SkippedCache() const { return skipped_cache_; }

  // Appends this display item to the cc::DisplayItemList, if applicable.
  // |visual_rect_offset| is the offset between the space of the GraphicsLayer
  // which owns the display item and the coordinate space of VisualRect().
  // TODO(wangxianzhu): Remove the parameter for slimming paint v2.
  virtual void AppendToDisplayItemList(const FloatSize& visual_rect_offset,
                                       cc::DisplayItemList&) const {}

// See comments of enum Type for usage of the following macros.
#define DEFINE_CATEGORY_METHODS(Category)                           \
  static bool Is##Category##Type(Type type) {                       \
    return type >= k##Category##First && type <= k##Category##Last; \
  }                                                                 \
  bool Is##Category() const { return Is##Category##Type(GetType()); }

#define DEFINE_CONVERSION_METHODS(Category1, category1, Category2, category2) \
  static Type Category1##TypeTo##Category2##Type(Type type) {                 \
    static_assert(k##Category1##Last - k##Category1##First ==                 \
                      k##Category2##Last - k##Category2##First,               \
                  "Categories " #Category1 " and " #Category2                 \
                  " should have same number of enum values. See comments of " \
                  "DisplayItem::Type");                                       \
    DCHECK(Is##Category1##Type(type));                                        \
    return static_cast<Type>(type - k##Category1##First +                     \
                             k##Category2##First);                            \
  }                                                                           \
  static Type category2##TypeTo##Category1##Type(Type type) {                 \
    DCHECK(Is##Category2##Type(type));                                        \
    return static_cast<Type>(type - k##Category2##First +                     \
                             k##Category1##First);                            \
  }

#define DEFINE_PAIRED_CATEGORY_METHODS(Category, category) \
  DEFINE_CATEGORY_METHODS(Category)                        \
  DEFINE_CATEGORY_METHODS(End##Category)                   \
  DEFINE_CONVERSION_METHODS(Category, category, End##Category, end##Category)

#define DEFINE_PAINT_PHASE_CONVERSION_METHOD(Category)                \
  static Type PaintPhaseTo##Category##Type(PaintPhase paint_phase) {  \
    static_assert(                                                    \
        k##Category##PaintPhaseLast - k##Category##PaintPhaseFirst == \
            kPaintPhaseMax,                                           \
        "Invalid paint-phase-based category " #Category               \
        ". See comments of DisplayItem::Type");                       \
    return static_cast<Type>(static_cast<int>(paint_phase) +          \
                             k##Category##PaintPhaseFirst);           \
  }

  DEFINE_CATEGORY_METHODS(Drawing)
  DEFINE_PAINT_PHASE_CONVERSION_METHOD(Drawing)

  DEFINE_CATEGORY_METHODS(ForeignLayer)

  DEFINE_PAIRED_CATEGORY_METHODS(Clip, clip)
  DEFINE_PAINT_PHASE_CONVERSION_METHOD(ClipLayerFragment)
  DEFINE_PAINT_PHASE_CONVERSION_METHOD(ClipBox)
  DEFINE_PAINT_PHASE_CONVERSION_METHOD(ClipColumnBounds)

  DEFINE_PAIRED_CATEGORY_METHODS(FloatClip, floatClip)
  DEFINE_PAINT_PHASE_CONVERSION_METHOD(FloatClip)

  DEFINE_PAIRED_CATEGORY_METHODS(Scroll, scroll)
  DEFINE_PAINT_PHASE_CONVERSION_METHOD(Scroll)

  DEFINE_PAINT_PHASE_CONVERSION_METHOD(SVGTransform)
  DEFINE_PAINT_PHASE_CONVERSION_METHOD(SVGEffect)

  DEFINE_PAIRED_CATEGORY_METHODS(Transform3D, transform3D)

  static bool IsScrollHitTestType(Type type) { return type == kScrollHitTest; }
  bool IsScrollHitTest() const { return IsScrollHitTestType(GetType()); }

  // TODO(pdr): Should this return true for IsScrollHitTestType too?
  static bool IsCacheableType(Type type) { return IsDrawingType(type); }
  bool IsCacheable() const {
    return !SkippedCache() && IsCacheableType(GetType());
  }

  virtual bool IsBegin() const { return false; }
  virtual bool IsEnd() const { return false; }

#if DCHECK_IS_ON()
  virtual bool IsEndAndPairedWith(DisplayItem::Type other_type) const {
    return false;
  }
#endif

  virtual bool Equals(const DisplayItem& other) const {
    // Failure of this DCHECK would cause bad casts in subclasses.
    SECURITY_CHECK(!is_tombstone_);
    return client_ == other.client_ && type_ == other.type_ &&
           fragment_ == other.fragment_ &&
           derived_size_ == other.derived_size_ &&
           skipped_cache_ == other.skipped_cache_;
  }

  // True if this DisplayItem is the tombstone/"dead display item" as part of
  // moving an item from one list to another. See the default constructor of
  // DisplayItem.
  bool IsTombstone() const { return is_tombstone_; }

  virtual bool DrawsContent() const { return false; }

#if DCHECK_IS_ON()
  static WTF::String TypeAsDebugString(DisplayItem::Type);
  WTF::String AsDebugString() const;
  virtual void PropertiesAsJSON(JSONObject&) const;
#endif

 private:
  template <typename T, unsigned alignment>
  friend class ContiguousContainer;
  friend class DisplayItemList;

  // The default DisplayItem constructor is only used by ContiguousContainer::
  // AppendByMoving() where a tombstone DisplayItem is constructed at the source
  // location. Only set is_tombstone_ to true, leaving other fields as-is so
  // that we can get their original values for debugging. |visual_rect_| and
  // |outset_for_raster_effects_| are special, see DisplayItemList::
  // AppendByMoving().
  DisplayItem() : is_tombstone_(true) {}

  const DisplayItemClient* client_;
  FloatRect visual_rect_;
  float outset_for_raster_effects_;

  static_assert(kTypeLast < (1 << 8), "DisplayItem::Type should fit in 8 bits");
  unsigned type_ : 8;
  unsigned derived_size_ : 8;  // size of the actual derived class
  unsigned fragment_ : 14;
  unsigned skipped_cache_ : 1;
  unsigned is_tombstone_ : 1;
};

inline bool operator==(const DisplayItem::Id& a, const DisplayItem::Id& b) {
  return a.client == b.client && a.type == b.type && a.fragment == b.fragment;
}

inline bool operator!=(const DisplayItem::Id& a, const DisplayItem::Id& b) {
  return !(a == b);
}

class PLATFORM_EXPORT PairedBeginDisplayItem : public DisplayItem {
 protected:
  PairedBeginDisplayItem(const DisplayItemClient& client,
                         Type type,
                         size_t derived_size)
      : DisplayItem(client, type, derived_size) {
    DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
  }

 private:
  bool IsBegin() const final { return true; }
};

class PLATFORM_EXPORT PairedEndDisplayItem : public DisplayItem {
 protected:
  PairedEndDisplayItem(const DisplayItemClient& client,
                       Type type,
                       size_t derived_size)
      : DisplayItem(client, type, derived_size) {
    DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
  }

#if DCHECK_IS_ON()
  bool IsEndAndPairedWith(DisplayItem::Type other_type) const override = 0;
#endif

 private:
  bool IsEnd() const final { return true; }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_DISPLAY_ITEM_H_
