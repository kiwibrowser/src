// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"

#include "cc/paint/display_item_list.h"

namespace blink {

struct SameSizeAsDisplayItem {
  virtual ~SameSizeAsDisplayItem() = default;  // Allocate vtable pointer.
  void* pointer;
  LayoutRect rect;
  LayoutUnit outset;
  int i;
};
static_assert(sizeof(DisplayItem) == sizeof(SameSizeAsDisplayItem),
              "DisplayItem should stay small");

#if DCHECK_IS_ON()

static WTF::String PaintPhaseAsDebugString(int paint_phase) {
  // Must be kept in sync with PaintPhase.
  switch (paint_phase) {
    case 0:
      return "PaintPhaseBlockBackground";
    case 1:
      return "PaintPhaseSelfBlockBackground";
    case 2:
      return "PaintPhaseChildBlockBackgrounds";
    case 3:
      return "PaintPhaseFloat";
    case 4:
      return "PaintPhaseForeground";
    case 5:
      return "PaintPhaseOutline";
    case 6:
      return "PaintPhaseSelfOutline";
    case 7:
      return "PaintPhaseChildOutlines";
    case 8:
      return "PaintPhaseSelection";
    case 9:
      return "PaintPhaseTextClip";
    case 10:
      return "PaintPhaseMask";
    case DisplayItem::kPaintPhaseMax:
      return "PaintPhaseClippingMask";
    default:
      NOTREACHED();
      return "Unknown";
  }
}

#define PAINT_PHASE_BASED_DEBUG_STRINGS(Category)          \
  if (type >= DisplayItem::k##Category##PaintPhaseFirst && \
      type <= DisplayItem::k##Category##PaintPhaseLast)    \
    return #Category + PaintPhaseAsDebugString(            \
                           type - DisplayItem::k##Category##PaintPhaseFirst);

#define DEBUG_STRING_CASE(DisplayItemName) \
  case DisplayItem::k##DisplayItemName:    \
    return #DisplayItemName

#define DEFAULT_CASE \
  default:           \
    NOTREACHED();    \
    return "Unknown"

static WTF::String SpecialDrawingTypeAsDebugString(DisplayItem::Type type) {
  switch (type) {
    DEBUG_STRING_CASE(BoxDecorationBackground);
    DEBUG_STRING_CASE(Caret);
    DEBUG_STRING_CASE(ColumnRules);
    DEBUG_STRING_CASE(DebugDrawing);
    DEBUG_STRING_CASE(DocumentBackground);
    DEBUG_STRING_CASE(DragImage);
    DEBUG_STRING_CASE(DragCaret);
    DEBUG_STRING_CASE(EmptyContentForFilters);
    DEBUG_STRING_CASE(SVGImage);
    DEBUG_STRING_CASE(LinkHighlight);
    DEBUG_STRING_CASE(ImageAreaFocusRing);
    DEBUG_STRING_CASE(PageOverlay);
    DEBUG_STRING_CASE(PopupContainerBorder);
    DEBUG_STRING_CASE(PopupListBoxBackground);
    DEBUG_STRING_CASE(PopupListBoxRow);
    DEBUG_STRING_CASE(PrintedContentDestinationLocations);
    DEBUG_STRING_CASE(PrintedContentPDFURLRect);
    DEBUG_STRING_CASE(Resizer);
    DEBUG_STRING_CASE(SVGClip);
    DEBUG_STRING_CASE(SVGFilter);
    DEBUG_STRING_CASE(SVGMask);
    DEBUG_STRING_CASE(ScrollbarBackButtonEnd);
    DEBUG_STRING_CASE(ScrollbarBackButtonStart);
    DEBUG_STRING_CASE(ScrollbarBackground);
    DEBUG_STRING_CASE(ScrollbarBackTrack);
    DEBUG_STRING_CASE(ScrollbarCorner);
    DEBUG_STRING_CASE(ScrollbarForwardButtonEnd);
    DEBUG_STRING_CASE(ScrollbarForwardButtonStart);
    DEBUG_STRING_CASE(ScrollbarForwardTrack);
    DEBUG_STRING_CASE(ScrollbarThumb);
    DEBUG_STRING_CASE(ScrollbarTickmarks);
    DEBUG_STRING_CASE(ScrollbarTrackBackground);
    DEBUG_STRING_CASE(ScrollbarCompositedScrollbar);
    DEBUG_STRING_CASE(SelectionTint);
    DEBUG_STRING_CASE(TableCollapsedBorders);
    DEBUG_STRING_CASE(VideoBitmap);
    DEBUG_STRING_CASE(WebPlugin);
    DEBUG_STRING_CASE(WebFont);
    DEBUG_STRING_CASE(ReflectionMask);

    DEFAULT_CASE;
  }
}

static WTF::String DrawingTypeAsDebugString(DisplayItem::Type type) {
  PAINT_PHASE_BASED_DEBUG_STRINGS(Drawing);
  return "Drawing" + SpecialDrawingTypeAsDebugString(type);
}

static String ForeignLayerTypeAsDebugString(DisplayItem::Type type) {
  switch (type) {
    DEBUG_STRING_CASE(ForeignLayerCanvas);
    DEBUG_STRING_CASE(ForeignLayerPlugin);
    DEBUG_STRING_CASE(ForeignLayerVideo);
    DEBUG_STRING_CASE(ForeignLayerWrapper);
    DEBUG_STRING_CASE(ForeignLayerContentsWrapper);
    DEFAULT_CASE;
  }
}

static String ScrollHitTestTypeAsDebugString(DisplayItem::Type type) {
  switch (type) {
    DEBUG_STRING_CASE(ScrollHitTest);
    DEFAULT_CASE;
  }
}

static WTF::String ClipTypeAsDebugString(DisplayItem::Type type) {
  PAINT_PHASE_BASED_DEBUG_STRINGS(ClipBox);
  PAINT_PHASE_BASED_DEBUG_STRINGS(ClipColumnBounds);
  PAINT_PHASE_BASED_DEBUG_STRINGS(ClipLayerFragment);

  switch (type) {
    DEBUG_STRING_CASE(ClipFileUploadControlRect);
    DEBUG_STRING_CASE(ClipFrameToVisibleContentRect);
    DEBUG_STRING_CASE(ClipFrameScrollbars);
    DEBUG_STRING_CASE(ClipLayerBackground);
    DEBUG_STRING_CASE(ClipLayerColumnBounds);
    DEBUG_STRING_CASE(ClipLayerFilter);
    DEBUG_STRING_CASE(ClipLayerForeground);
    DEBUG_STRING_CASE(ClipLayerParent);
    DEBUG_STRING_CASE(ClipLayerOverflowControls);
    DEBUG_STRING_CASE(ClipPopupListBoxFrame);
    DEBUG_STRING_CASE(ClipScrollbarsToBoxBounds);
    DEBUG_STRING_CASE(ClipSelectionImage);
    DEFAULT_CASE;
  }
}

static String ScrollTypeAsDebugString(DisplayItem::Type type) {
  PAINT_PHASE_BASED_DEBUG_STRINGS(Scroll);
  switch (type) {
    DEBUG_STRING_CASE(ScrollOverflowControls);
    DEFAULT_CASE;
  }
}

static String Transform3DTypeAsDebugString(DisplayItem::Type type) {
  switch (type) {
    DEBUG_STRING_CASE(Transform3DElementTransform);
    DEFAULT_CASE;
  }
}

WTF::String DisplayItem::TypeAsDebugString(Type type) {
  if (IsDrawingType(type))
    return DrawingTypeAsDebugString(type);

  if (IsForeignLayerType(type))
    return ForeignLayerTypeAsDebugString(type);

  if (IsClipType(type))
    return ClipTypeAsDebugString(type);
  if (IsEndClipType(type))
    return "End" + ClipTypeAsDebugString(endClipTypeToClipType(type));

  PAINT_PHASE_BASED_DEBUG_STRINGS(FloatClip);
  if (type == kFloatClipClipPathBounds)
    return "FloatClipClipPathBounds";
  if (IsEndFloatClipType(type))
    return "End" + TypeAsDebugString(endFloatClipTypeToFloatClipType(type));

  if (IsScrollType(type))
    return ScrollTypeAsDebugString(type);
  if (IsEndScrollType(type))
    return "End" + ScrollTypeAsDebugString(endScrollTypeToScrollType(type));

  PAINT_PHASE_BASED_DEBUG_STRINGS(SVGTransform);
  PAINT_PHASE_BASED_DEBUG_STRINGS(SVGEffect);

  if (IsTransform3DType(type))
    return Transform3DTypeAsDebugString(type);
  if (IsEndTransform3DType(type))
    return "End" + Transform3DTypeAsDebugString(
                       endTransform3DTypeToTransform3DType(type));

  if (IsScrollHitTestType(type))
    return ScrollHitTestTypeAsDebugString(type);

  switch (type) {
    DEBUG_STRING_CASE(BeginFilter);
    DEBUG_STRING_CASE(EndFilter);
    DEBUG_STRING_CASE(BeginCompositing);
    DEBUG_STRING_CASE(EndCompositing);
    DEBUG_STRING_CASE(BeginTransform);
    DEBUG_STRING_CASE(EndTransform);
    DEBUG_STRING_CASE(BeginClipPath);
    DEBUG_STRING_CASE(EndClipPath);
    DEBUG_STRING_CASE(LayerChunkBackground);
    DEBUG_STRING_CASE(LayerChunkNegativeZOrderChildren);
    DEBUG_STRING_CASE(LayerChunkDescendantBackgrounds);
    DEBUG_STRING_CASE(LayerChunkFloat);
    DEBUG_STRING_CASE(LayerChunkForeground);
    DEBUG_STRING_CASE(LayerChunkNormalFlowAndPositiveZOrderChildren);
    DEBUG_STRING_CASE(UninitializedType);
    DEFAULT_CASE;
  }
}

WTF::String DisplayItem::AsDebugString() const {
  auto json = JSONObject::Create();
  PropertiesAsJSON(*json);
  return json->ToPrettyJSONString();
}

void DisplayItem::PropertiesAsJSON(JSONObject& json) const {
  if (IsTombstone())
    json.SetBoolean("ISTOMBSTONE", true);

  json.SetString("id", GetId().ToString());
  json.SetString("visualRect", VisualRect().ToString());
  if (OutsetForRasterEffects())
    json.SetDouble("outset", OutsetForRasterEffects());
  if (skipped_cache_)
    json.SetBoolean("skippedCache", true);
}

#endif

String DisplayItem::Id::ToString() const {
#if DCHECK_IS_ON()
  return String::Format("%p:%s:%d", &client,
                        DisplayItem::TypeAsDebugString(type).Ascii().data(),
                        fragment);
#else
  return String::Format("%p:%d:%d", &client, static_cast<int>(type), fragment);
#endif
}

}  // namespace blink
