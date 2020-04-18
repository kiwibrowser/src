// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/paint_layer_painter.h"

#include "base/optional.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/clip_path_clipper.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/filter_painter.h"
#include "third_party/blink/renderer/core/paint/layer_clip_recorder.h"
#include "third_party/blink/renderer/core/paint/object_paint_properties.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/scroll_recorder.h"
#include "third_party/blink/renderer/core/paint/scrollable_area_painter.h"
#include "third_party/blink/renderer/core/paint/transform_3d_recorder.h"
#include "third_party/blink/renderer/platform/geometry/float_point_3d.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/compositing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item_cache_skipper.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper.h"
#include "third_party/blink/renderer/platform/graphics/paint/scoped_display_item_fragment.h"
#include "third_party/blink/renderer/platform/graphics/paint/scoped_paint_chunk_properties.h"
#include "third_party/blink/renderer/platform/graphics/paint/subsequence_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/transform_3d_display_item.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

static inline bool ShouldSuppressPaintingLayer(const PaintLayer& layer) {
  // Avoid painting descendants of the root layer when stylesheets haven't
  // loaded. This avoids some FOUC.  It's ok not to draw, because later on, when
  // all the stylesheets do load, Document::styleResolverMayHaveChanged() will
  // invalidate all painted output via a call to
  // LayoutView::invalidatePaintForViewAndCompositedLayers().  We also avoid
  // caching subsequences in this mode; see shouldCreateSubsequence().
  return layer.GetLayoutObject()
             .GetDocument()
             .DidLayoutWithPendingStylesheets() &&
         !layer.IsRootLayer() && !layer.GetLayoutObject().IsDocumentElement();
}

void PaintLayerPainter::Paint(GraphicsContext& context,
                              const LayoutRect& damage_rect,
                              const GlobalPaintFlags global_paint_flags,
                              PaintLayerFlags paint_flags) {
  PaintLayerPaintingInfo painting_info(
      &paint_layer_, LayoutRect(EnclosingIntRect(damage_rect)),
      global_paint_flags, LayoutSize());
  if (!paint_layer_.PaintsIntoOwnOrGroupedBacking(global_paint_flags))
    Paint(context, painting_info, paint_flags);
}

static ShouldRespectOverflowClipType ShouldRespectOverflowClip(
    PaintLayerFlags paint_flags,
    const LayoutObject& layout_object) {
  return (paint_flags & kPaintLayerPaintingOverflowContents ||
          (paint_flags & kPaintLayerPaintingChildClippingMaskPhase &&
           layout_object.HasClipPath()))
             ? kIgnoreOverflowClip
             : kRespectOverflowClip;
}

bool PaintLayerPainter::PaintedOutputInvisible(
    const ComputedStyle& style,
    GlobalPaintFlags global_paint_flags) const {
  if (style.HasBackdropFilter())
    return false;

  // Always paint when 'will-change: opacity' is present. Reduces jank for
  // common animation implementation approaches, for example, an element that
  // starts with opacity zero and later begins to animate.
  if (style.HasWillChangeOpacityHint())
    return false;

  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
    if (style.Opacity())
      return false;

    const auto* effect = paint_layer_.GetLayoutObject()
                             .FirstFragment()
                             .PaintProperties()
                             ->Effect();
    if (effect && effect->RequiresCompositingForAnimation()) {
      return false;
    }
  }

  // 0.0004f < 1/2048. With 10-bit color channels (only available on the
  // newest Macs; otherwise it's 8-bit), we see that an alpha of 1/2048 or
  // less leads to a color output of less than 0.5 in all channels, hence
  // not visible.
  static const float kMinimumVisibleOpacity = 0.0004f;
  if (paint_layer_.PaintsWithTransparency(global_paint_flags)) {
    if (style.Opacity() < kMinimumVisibleOpacity) {
      return true;
    }
  }
  return false;
}

bool PaintLayerPainter::ShouldAdjustPaintingRoot(
    const PaintLayerPaintingInfo& painting_info,
    PaintLayerFlags paint_flags) {
  // Cull rects and clips can't be propagated into a different 2D transform
  // space.
  if (paint_layer_.PaintsWithTransform(painting_info.GetGlobalPaintFlags()) &&
      !(paint_flags & kPaintLayerAppliedTransform))
    return true;

  // Cull rects and clips can't be propagated across a filter which moves
  // pixels, since the input of the filter may be outside the cull rect/clips
  // yet still result in painted output.
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
      paint_layer_.HasFilterThatMovesPixels())
    return true;

  return false;
}

PaintResult PaintLayerPainter::Paint(
    GraphicsContext& context,
    const PaintLayerPaintingInfo& painting_info,
    PaintLayerFlags paint_flags) {
  if (paint_layer_.GetLayoutObject().GetFrameView()->ShouldThrottleRendering())
    return kFullyPainted;

  // https://code.google.com/p/chromium/issues/detail?id=343772
  DisableCompositingQueryAsserts disabler;

  if (paint_layer_.GetCompositingState() != kNotComposited) {
    if (painting_info.GetGlobalPaintFlags() &
        kGlobalPaintFlattenCompositingLayers) {
      // FIXME: ok, but what about GlobalPaintFlattenCompositingLayers? That's
      // for printing and drag-image.
      // FIXME: why isn't the code here global, as opposed to being set on each
      // paint() call?
      paint_flags |= kPaintLayerUncachedClipRects;
    }
  }

  // Non self-painting layers without self-painting descendants don't need to be
  // painted as their layoutObject() should properly paint itself.
  if (!paint_layer_.IsSelfPaintingLayer() &&
      !paint_layer_.HasSelfPaintingLayerDescendant())
    return kFullyPainted;

  if (ShouldSuppressPaintingLayer(paint_layer_))
    return kFullyPainted;

  // If this layer is totally invisible then there is nothing to paint. In SPv2
  // we simplify this optimization by painting even when effectively invisible
  // but skipping the painted content during layerization in
  // PaintArtifactCompositor.
  if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled() &&
      PaintedOutputInvisible(paint_layer_.GetLayoutObject().StyleRef(),
                             painting_info.GetGlobalPaintFlags())) {
    return kFullyPainted;
  }

  if (paint_layer_.PaintsWithTransparency(painting_info.GetGlobalPaintFlags()))
    paint_flags |= kPaintLayerHaveTransparency;

  // The painting root should be adjusted if clips or cull rects for the
  // current root don't make sense for content underneath |paint_layer_|.
  // See ShouldAdjustPaintingRoot for examples when this is the case.
  // In these cases, we reset the cull rect to infinite, collect fragments,
  // and paint each fragment's subtree separately.
  if (ShouldAdjustPaintingRoot(painting_info, paint_flags))
    return PaintLayerWithAdjustedRoot(context, painting_info, paint_flags);

  return PaintLayerContentsCompositingAllPhases(context, painting_info,
                                                paint_flags);
}

PaintResult PaintLayerPainter::PaintLayerContentsCompositingAllPhases(
    GraphicsContext& context,
    const PaintLayerPaintingInfo& painting_info,
    PaintLayerFlags paint_flags,
    const PaintLayerFragment* fragment) {
  DCHECK(paint_layer_.IsSelfPaintingLayer() ||
         paint_layer_.HasSelfPaintingLayerDescendant());

  PaintLayerFlags local_paint_flags =
      paint_flags & ~(kPaintLayerAppliedTransform);
  local_paint_flags |= kPaintLayerPaintingCompositingAllPhases;
  return PaintLayerContents(context, painting_info, local_paint_flags,
                            fragment);
}

static bool ShouldCreateSubsequence(const PaintLayer& paint_layer,
                                    const GraphicsContext& context,
                                    const PaintLayerPaintingInfo& painting_info,
                                    PaintLayerFlags paint_flags) {
  // Caching is not needed during printing.
  if (context.Printing())
    return false;

  if (context.GetPaintController().IsSkippingCache())
    return false;

  if (!paint_layer.SupportsSubsequenceCaching())
    return false;

  // Don't create subsequence for a composited layer because if it can be
  // cached, we can skip the whole painting in GraphicsLayer::paint() with
  // CachedDisplayItemList.  This also avoids conflict of
  // PaintLayer::previousXXX() when paintLayer is composited scrolling and is
  // painted twice for GraphicsLayers of container and scrolling contents.
  if (paint_layer.GetCompositingState() == kPaintsIntoOwnBacking)
    return false;

  // Don't create subsequence during special painting to avoid cache conflict
  // with normal painting.
  if (painting_info.GetGlobalPaintFlags() &
      kGlobalPaintFlattenCompositingLayers)
    return false;
  if (paint_flags &
      (kPaintLayerPaintingOverlayScrollbars | kPaintLayerUncachedClipRects))
    return false;

  // When in FOUC-avoidance mode, don't cache any subsequences, to avoid having
  // to invalidate all of them when leaving this mode. There is an early-out in
  // BlockPainter::paintContents that may result in nothing getting painted in
  // this mode, in addition to early-out logic in PaintLayerPainter.
  if (paint_layer.GetLayoutObject()
          .GetDocument()
          .DidLayoutWithPendingStylesheets())
    return false;

  return true;
}

static bool ShouldRepaintSubsequence(
    PaintLayer& paint_layer,
    const PaintLayerPaintingInfo& painting_info,
    ShouldRespectOverflowClipType respect_overflow_clip,
    bool& should_clear_empty_paint_phase_flags) {
  bool needs_repaint = false;

  // We should set shouldResetEmptyPaintPhaseFlags if some previously unpainted
  // objects may begin to be painted, causing a previously empty paint phase to
  // become non-empty.

  // Repaint subsequence if the layer is marked for needing repaint.
  // We don't set needsResetEmptyPaintPhase here, but clear the empty paint
  // phase flags in PaintLayer::setNeedsPaintPhaseXXX(), to ensure that we won't
  // clear previousPaintPhaseXXXEmpty flags when unrelated things changed which
  // won't cause the paint phases to become non-empty.
  if (paint_layer.NeedsRepaint())
    needs_repaint = true;

  // Repaint if previously the layer might be clipped by paintDirtyRect and
  // paintDirtyRect changes.
  if ((paint_layer.PreviousPaintResult() == kMayBeClippedByPaintDirtyRect ||
       // When PaintUnderInvalidationChecking is enabled, always repaint the
       // subsequence when the paint rect changes because we will strictly match
       // new and cached subsequences. Normally we can reuse the cached fully
       // painted subsequence even if we would partially paint this time.
       RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled()) &&
      paint_layer.PreviousPaintDirtyRect() != painting_info.paint_dirty_rect) {
    needs_repaint = true;
    should_clear_empty_paint_phase_flags = true;
  }
  paint_layer.SetPreviousPaintDirtyRect(painting_info.paint_dirty_rect);

  // TODO(wangxianzhu): Get rid of scroll_offset_accumulation for SPv175.
  // Repaint if scroll offset accumulation changes.
  if (painting_info.scroll_offset_accumulation !=
      paint_layer.PreviousScrollOffsetAccumulationForPainting()) {
    needs_repaint = true;
    should_clear_empty_paint_phase_flags = true;
  }
  paint_layer.SetPreviousScrollOffsetAccumulationForPainting(
      painting_info.scroll_offset_accumulation);

  return needs_repaint;
}

void PaintLayerPainter::AdjustForPaintProperties(
    PaintLayerPaintingInfo& painting_info,
    PaintLayerFlags& paint_flags) {
  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  // Paint properties for transforms, composited layers or LayoutView is already
  // taken care of.
  // TODO(wangxianzhu): Also use this for PaintsWithTransform() and remove
  // PaintLayerWithTransform() for SPv175.
  if (&paint_layer_ == painting_info.root_layer ||
      paint_layer_.PaintsWithTransform(painting_info.GetGlobalPaintFlags()) ||
      paint_layer_.PaintsIntoOwnOrGroupedBacking(
          painting_info.GetGlobalPaintFlags()) ||
      paint_layer_.GetLayoutObject().IsLayoutView())
    return;

  const auto& current_fragment = paint_layer_.GetLayoutObject().FirstFragment();
  const auto* current_transform =
      current_fragment.LocalBorderBoxProperties().Transform();
  const auto& root_fragment =
      painting_info.root_layer->GetLayoutObject().FirstFragment();
  const auto* root_transform =
      root_fragment.LocalBorderBoxProperties().Transform();
  if (current_transform == root_transform)
    return;

  // painting_info.paint_dirty_rect is currently in |painting_info.root_layer|'s
  // pixel-snapped border box space. We need to adjust it into |paint_layer_|'s
  // space. This handles the following cases:
  // - The current layer has PaintOffsetTranslation;
  // - The current layer's transform state escapes the root layers contents
  //   transform, e.g. a fixed-position layer;
  // - Scroll offsets.
  const auto& matrix = GeometryMapper::SourceToDestinationProjection(
      root_transform, current_transform);
  painting_info.paint_dirty_rect.MoveBy(
      RoundedIntPoint(root_fragment.PaintOffset()));
  painting_info.paint_dirty_rect =
      matrix.MapRect(painting_info.paint_dirty_rect);
  painting_info.paint_dirty_rect.MoveBy(
      -RoundedIntPoint(current_fragment.PaintOffset()));

  // Make the current layer the new root layer.
  painting_info.root_layer = &paint_layer_;
  // These flags no longer apply for the new root layer.
  paint_flags &= ~kPaintLayerPaintingSkipRootBackground;
  paint_flags &= ~kPaintLayerPaintingOverflowContents;
  paint_flags &= ~kPaintLayerPaintingCompositingScrollingPhase;

  // TODO(chrishtr): is this correct for fragmentation?
  if (current_fragment.PaintProperties() &&
      current_fragment.PaintProperties()->PaintOffsetTranslation()) {
    painting_info.sub_pixel_accumulation =
        ToLayoutSize(current_fragment.PaintOffset());
  }
}

PaintResult PaintLayerPainter::PaintLayerContents(
    GraphicsContext& context,
    const PaintLayerPaintingInfo& painting_info_arg,
    PaintLayerFlags paint_flags_arg,
    const PaintLayerFragment* fragment) {
  PaintLayerFlags paint_flags = paint_flags_arg;
  PaintResult result = kFullyPainted;

  if (paint_layer_.GetLayoutObject().GetFrameView()->ShouldThrottleRendering())
    return result;

  // A paint layer should always have LocalBorderBoxProperties when it's ready
  // for paint.
  if (!paint_layer_.GetLayoutObject()
           .FirstFragment()
           .HasLocalBorderBoxProperties()) {
    // TODO(crbug.com/848056): This can happen e.g. when we paint a filter
    // referencing a SVG foreign object through feImage, especially when there
    // is circular references. Should find a better solution.
    paint_layer_.SetPreviousPaintDirtyRect(LayoutRect());
    return kMayBeClippedByPaintDirtyRect;
  }

  DCHECK(paint_layer_.IsSelfPaintingLayer() ||
         paint_layer_.HasSelfPaintingLayerDescendant());
  DCHECK(!(paint_flags & kPaintLayerAppliedTransform));

  bool is_self_painting_layer = paint_layer_.IsSelfPaintingLayer();
  bool is_painting_overlay_scrollbars =
      paint_flags & kPaintLayerPaintingOverlayScrollbars;
  bool is_painting_scrolling_content =
      paint_flags & kPaintLayerPaintingCompositingScrollingPhase;
  bool is_painting_composited_foreground =
      paint_flags & kPaintLayerPaintingCompositingForegroundPhase;
  bool is_painting_composited_background =
      paint_flags & kPaintLayerPaintingCompositingBackgroundPhase;
  bool is_painting_composited_decoration =
      paint_flags & kPaintLayerPaintingCompositingDecorationPhase;
  bool is_painting_overflow_contents =
      paint_flags & kPaintLayerPaintingOverflowContents;
  bool is_painting_mask = paint_flags & kPaintLayerPaintingCompositingMaskPhase;

  // Outline always needs to be painted even if we have no visible content.
  // It is painted as part of the decoration phase which paints content that
  // is not scrolled and should be above scrolled content.
  bool should_paint_self_outline =
      is_self_painting_layer && !is_painting_overlay_scrollbars &&
      (is_painting_composited_decoration ||
       (!is_painting_scrolling_content && !is_painting_mask)) &&
      paint_layer_.GetLayoutObject().StyleRef().HasOutline();

  // Ensure our lists are up to date.
  paint_layer_.StackingNode()->UpdateLayerListsIfNeeded();

  LayoutSize subpixel_accumulation =
      paint_layer_.GetCompositingState() == kPaintsIntoOwnBacking
          ? paint_layer_.SubpixelAccumulation()
          : painting_info_arg.sub_pixel_accumulation;
  ShouldRespectOverflowClipType respect_overflow_clip =
      ShouldRespectOverflowClip(paint_flags, paint_layer_.GetLayoutObject());

  PaintLayerPaintingInfo painting_info = painting_info_arg;
  AdjustForPaintProperties(painting_info, paint_flags);

  bool should_create_subsequence = ShouldCreateSubsequence(
      paint_layer_, context, painting_info_arg, paint_flags);

  base::Optional<SubsequenceRecorder> subsequence_recorder;
  bool should_clear_empty_paint_phase_flags = false;
  if (should_create_subsequence) {
    if (!ShouldRepaintSubsequence(paint_layer_, painting_info,
                                  respect_overflow_clip,
                                  should_clear_empty_paint_phase_flags) &&
        SubsequenceRecorder::UseCachedSubsequenceIfPossible(context,
                                                            paint_layer_))
      return result;
    DCHECK(paint_layer_.SupportsSubsequenceCaching());
    subsequence_recorder.emplace(context, paint_layer_);
  } else {
    should_clear_empty_paint_phase_flags = true;
  }

  if (should_clear_empty_paint_phase_flags) {
    paint_layer_.SetPreviousPaintPhaseDescendantOutlinesEmpty(false);
    paint_layer_.SetPreviousPaintPhaseFloatEmpty(false);
    paint_layer_.SetPreviousPaintPhaseDescendantBlockBackgroundsEmpty(false);
  }

  LayoutPoint offset_from_root;
  paint_layer_.ConvertToLayerCoords(painting_info.root_layer, offset_from_root);
  offset_from_root.Move(subpixel_accumulation);

  LayoutRect bounds = paint_layer_.PhysicalBoundingBox(offset_from_root);
  if (!painting_info.paint_dirty_rect.Contains(bounds))
    result = kMayBeClippedByPaintDirtyRect;

  // These helpers output clip and compositing operations using a RAII pattern.
  // Stack-allocated-varibles are destructed in the reverse order of
  // construction, so they are nested properly.
  base::Optional<ClipPathClipper> clip_path_clipper;
  bool should_paint_clip_path =
      is_painting_mask && paint_layer_.GetLayoutObject().HasClipPath();
  if (should_paint_clip_path) {
    LayoutPoint visual_offset_from_root =
        paint_layer_.EnclosingPaginationLayer()
            ? paint_layer_.VisualOffsetFromAncestor(
                  painting_info.root_layer, LayoutPoint(subpixel_accumulation))
            : offset_from_root;
    clip_path_clipper.emplace(context, paint_layer_.GetLayoutObject(),
                              visual_offset_from_root);
  }

  base::Optional<CompositingRecorder> compositing_recorder;
  // FIXME: this should be unified further into
  // PaintLayer::paintsWithTransparency().
  bool compositing_already_applied =
      painting_info.root_layer == &paint_layer_ &&
      is_painting_overflow_contents;
  bool should_composite_for_blend_mode =
      paint_layer_.StackingNode()->IsStackingContext() &&
      paint_layer_.HasNonIsolatedDescendantWithBlendMode();
  if (!compositing_already_applied &&
      (should_composite_for_blend_mode ||
       paint_layer_.PaintsWithTransparency(
           painting_info.GetGlobalPaintFlags()))) {
    FloatRect compositing_bounds = EnclosingIntRect(paint_layer_.PaintingExtent(
        painting_info.root_layer, painting_info.sub_pixel_accumulation,
        painting_info.GetGlobalPaintFlags()));
    compositing_recorder.emplace(
        context, paint_layer_.GetLayoutObject(),
        WebCoreCompositeToSkiaComposite(
            kCompositeSourceOver,
            paint_layer_.GetLayoutObject().Style()->GetBlendMode()),
        paint_layer_.GetLayoutObject().Opacity(), &compositing_bounds);
  }

  PaintLayerPaintingInfo local_painting_info(painting_info);
  local_painting_info.sub_pixel_accumulation = subpixel_accumulation;

  sk_sp<PaintFilter> image_filter = FilterPainter::GetImageFilter(paint_layer_);

  bool should_paint_content =
      paint_layer_.HasVisibleContent() &&
      // Content under a LayoutSVGHiddenContainer is auxiliary resources for
      // painting. Foreign content should never paint in this situation, as it
      // is primary, not auxiliary.
      !paint_layer_.IsUnderSVGHiddenContainer() && is_self_painting_layer &&
      !is_painting_overlay_scrollbars;

  PaintLayerFragments layer_fragments;

  if (should_paint_content || should_paint_self_outline ||
      is_painting_overlay_scrollbars) {
    // Collect the fragments. This will compute the clip rectangles and paint
    // offsets for each layer fragment.
    LayoutPoint offset_to_clipper;
    const PaintLayer* paint_layer_for_fragments = &paint_layer_;
    if (paint_flags & kPaintLayerPaintingAncestorClippingMaskPhase) {
      // Compute fragments and their clips with respect to the outermost
      // clipping container. This handles nested border radius by including
      // all of them in the mask.
      //
      // The paint rect is in this layer's space, so convert it to the clipper's
      // layer's space. The root_layer is also changed to the clipper's layer to
      // simplify coordinate system adjustments. The change to root_layer must
      // persist to correctly record the clips.
      paint_layer_for_fragments =
          paint_layer_.EnclosingLayerWithCompositedLayerMapping(kExcludeSelf);
      local_painting_info.root_layer = paint_layer_for_fragments;
      paint_layer_.ConvertToLayerCoords(local_painting_info.root_layer,
                                        offset_to_clipper);
      local_painting_info.paint_dirty_rect.MoveBy(offset_to_clipper);
      // Overflow clip of the compositing container is irrelevant.
      respect_overflow_clip = kIgnoreOverflowClip;
    }

    if (fragment) {
      // We are painting a single specified fragment.
      // TODO(wangxianzhu): we could use |fragment| directly, but because
      // the value of |fragment| is for SPv175 only for descendants to find the
      // correct fragment state, we don't bother to modify SPv1 code with
      // the risk of regressions. Eventually we will remove the whole
      // PaintLayerWithTransform() path.
      paint_layer_for_fragments->AppendSingleFragmentIgnoringPagination(
          layer_fragments, local_painting_info.root_layer,
          local_painting_info.paint_dirty_rect,
          kIgnorePlatformOverlayScrollbarSize, respect_overflow_clip,
          &offset_from_root, local_painting_info.sub_pixel_accumulation);
      layer_fragments[0].fragment_data = fragment->fragment_data;
    } else if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
               paint_layer_.GetLayoutObject()
                   .IsFixedPositionObjectInPagedMedia()) {
      PaintLayerFragments single_fragment;
      paint_layer_for_fragments->AppendSingleFragmentIgnoringPagination(
          single_fragment, local_painting_info.root_layer,
          local_painting_info.paint_dirty_rect,
          kIgnorePlatformOverlayScrollbarSize, respect_overflow_clip,
          &offset_from_root, local_painting_info.sub_pixel_accumulation);
      RepeatFixedPositionObjectInPages(single_fragment[0], painting_info,
                                       layer_fragments);
    } else if (image_filter && !paint_layer_.EnclosingPaginationLayer()) {
      // Clipping in the presence of filters needs to happen in two phases.
      // It proceeds like this:
      // 1. Apply overflow clip to normal-flow contents.
      // 2. Paint layer contents into the filter.
      // 3. Apply all inherited clips (including dirty rect) plus CSS clip
      //    of the current layer to the filter output.
      //
      // It is critical to avoid clipping to the dirty rect or any clips
      // above the filter before applying the filter, because content which
      // appears to be clipped may affect visual output if the filter moves
      // pixels.
      //
      // #1 is applied in the lines below.  #3 is applied in FilterPainter,
      // and computed just before construction of the FilterPainter.
      LayoutRect foreground_clip(LayoutRect::InfiniteIntRect());
      if (paint_layer_.GetLayoutObject().IsBox()) {
        const LayoutBox& box = ToLayoutBox(paint_layer_.GetLayoutObject());
        if (box.ShouldClipOverflow())
          foreground_clip = box.OverflowClipRect(offset_from_root);
      }
      PaintLayerFragment& fragment = layer_fragments.emplace_back();
      fragment.SetRects(
          LayoutRect(offset_from_root, LayoutSize(paint_layer_.Size())),
          LayoutRect(LayoutRect::InfiniteIntRect()), foreground_clip);
    } else {
      paint_layer_for_fragments->CollectFragments(
          layer_fragments, local_painting_info.root_layer,
          local_painting_info.paint_dirty_rect,
          kIgnorePlatformOverlayScrollbarSize, respect_overflow_clip,
          &offset_from_root, local_painting_info.sub_pixel_accumulation);

      // PaintLayer::CollectFragments depends on the paint dirty rect in
      // complicated ways. For now, always assume a partially painted output
      // for fragmented content.
      if (layer_fragments.size() > 1)
        result = kMayBeClippedByPaintDirtyRect;
    }

    if (paint_flags & kPaintLayerPaintingAncestorClippingMaskPhase) {
      // Fragment offsets have been computed in the clipping container's
      // layer's coordinate system, but for the rest of painting we need
      // them in the layer coordinate. So move them and the
      // foreground/background rects that are also in the clipper's space.
      LayoutSize negative_offset(-offset_to_clipper.X(),
                                 -offset_to_clipper.Y());
      for (auto& fragment : layer_fragments) {
        fragment.background_rect.Move(negative_offset);
        fragment.foreground_rect.Move(negative_offset);
        fragment.pagination_offset.Move(negative_offset);
      }
    } else if (should_paint_content) {
      should_paint_content = AtLeastOneFragmentIntersectsDamageRect(
          layer_fragments, local_painting_info, paint_flags, offset_from_root);
      if (!should_paint_content)
        result = kMayBeClippedByPaintDirtyRect;
    }
  }

  bool selection_only =
      local_painting_info.GetGlobalPaintFlags() & kGlobalPaintSelectionOnly;

  {  // Begin block for the lifetime of any filter.
    size_t display_item_list_size_before_painting =
        context.GetPaintController().NewDisplayItemList().size();

    base::Optional<FilterPainter> filter_painter;
    if (image_filter) {
      DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
      // Compute clips outside the filter (#3, see above for discussion).
      PaintLayerFragments filter_fragments;
      paint_layer_.AppendSingleFragmentIgnoringPagination(
          filter_fragments, local_painting_info.root_layer,
          local_painting_info.paint_dirty_rect,
          kIgnorePlatformOverlayScrollbarSize, respect_overflow_clip,
          &offset_from_root, local_painting_info.sub_pixel_accumulation);

      filter_painter.emplace(paint_layer_, context, offset_from_root,
                             filter_fragments.IsEmpty()
                                 ? ClipRect()
                                 : filter_fragments[0].background_rect,
                             local_painting_info, paint_flags);
    }

    bool is_painting_root_layer = (&paint_layer_) == painting_info.root_layer;
    bool should_paint_background =
        should_paint_content && !selection_only &&
        (is_painting_composited_background ||
         (is_painting_root_layer &&
          !(paint_flags & kPaintLayerPaintingSkipRootBackground)));
    bool should_paint_neg_z_order_list =
        (is_painting_scrolling_content && is_painting_overflow_contents) ||
        (!is_painting_scrolling_content && is_painting_composited_background);
    bool should_paint_own_contents =
        is_painting_composited_foreground && should_paint_content;
    bool should_paint_normal_flow_and_pos_z_order_lists =
        is_painting_composited_foreground;
    bool should_paint_overlay_scrollbars = is_painting_overlay_scrollbars;

    base::Optional<ScopedPaintChunkProperties>
        subsequence_forced_chunk_properties;
    if (subsequence_recorder && paint_layer_.HasSelfPaintingLayerDescendant() &&
        RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      // Prepare for forced paint chunks to ensure chunk id stability to avoid
      // unnecessary full chunk raster invalidations on changed chunk ids.
      // TODO(crbug.com/834606): This may be unnecessary after we refactor
      // raster invalidation not to depend on chunk ids too much.
      subsequence_forced_chunk_properties.emplace(
          context.GetPaintController(),
          paint_layer_.GetLayoutObject()
              .FirstFragment()
              .LocalBorderBoxProperties(),
          paint_layer_, DisplayItem::kUninitializedType);
    }

    if (should_paint_background) {
      if (subsequence_forced_chunk_properties) {
        context.GetPaintController().ForceNewChunk(
            paint_layer_, DisplayItem::kLayerChunkBackground);
      }
      PaintBackgroundForFragments(layer_fragments, context,
                                  local_painting_info, paint_flags);
    }

    if (should_paint_neg_z_order_list) {
      if (subsequence_forced_chunk_properties) {
        context.GetPaintController().ForceNewChunk(
            paint_layer_, DisplayItem::kLayerChunkNegativeZOrderChildren);
      }
      if (PaintChildren(kNegativeZOrderChildren, context, painting_info,
                        paint_flags) == kMayBeClippedByPaintDirtyRect)
        result = kMayBeClippedByPaintDirtyRect;
    }

    if (should_paint_own_contents) {
      PaintForegroundForFragments(
          layer_fragments, context, local_painting_info, selection_only,
          !!subsequence_forced_chunk_properties, paint_flags);
    }

    if (should_paint_self_outline) {
      PaintSelfOutlineForFragments(layer_fragments, context,
                                   local_painting_info, paint_flags);
    }

    if (should_paint_normal_flow_and_pos_z_order_lists) {
      if (subsequence_forced_chunk_properties) {
        context.GetPaintController().ForceNewChunk(
            paint_layer_,
            DisplayItem::kLayerChunkNormalFlowAndPositiveZOrderChildren);
      }
      if (PaintChildren(kNormalFlowChildren | kPositiveZOrderChildren, context,
                        painting_info,
                        paint_flags) == kMayBeClippedByPaintDirtyRect)
        result = kMayBeClippedByPaintDirtyRect;
    }

    if (should_paint_overlay_scrollbars) {
      PaintOverflowControlsForFragments(layer_fragments, context,
                                        local_painting_info, paint_flags);
    }

    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
        !is_painting_overlay_scrollbars && paint_layer_.PaintsWithFilters() &&
        display_item_list_size_before_painting ==
            context.GetPaintController().NewDisplayItemList().size()) {
      // If a layer with filters painted nothing, we need to issue a no-op
      // display item to ensure the filters won't be ignored.
      PaintEmptyContentForFilters(context);
    }
  }  // FilterPainter block

  bool should_paint_mask = is_painting_mask && should_paint_content &&
                           paint_layer_.GetLayoutObject().HasMask() &&
                           !selection_only;
  if (should_paint_mask) {
    PaintMaskForFragments(layer_fragments, context, local_painting_info,
                          paint_flags);
  } else if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled() &&
             is_painting_mask &&
             !(painting_info.GetGlobalPaintFlags() &
               kGlobalPaintFlattenCompositingLayers) &&
             paint_layer_.GetCompositedLayerMapping() &&
             paint_layer_.GetCompositedLayerMapping()->MaskLayer()) {
    // In SPv1 it is possible for CompositedLayerMapping to create a mask layer
    // for just CSS clip-path but without a CSS mask. In that case we need to
    // paint a fully filled mask (which will subsequently clipped by the
    // clip-path), otherwise the mask layer will be empty.

    base::Optional<ScopedPaintChunkProperties> path_based_clip_path_scope;
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      const auto& fragment_data =
          paint_layer_.GetLayoutObject().FirstFragment();
      auto state = fragment_data.LocalBorderBoxProperties();
      const auto* properties = fragment_data.PaintProperties();
      DCHECK(properties && properties->Mask());
      state.SetEffect(properties->Mask());
      if (properties && properties->ClipPathClip()) {
        DCHECK_EQ(properties->ClipPathClip()->Parent(), properties->MaskClip());
        state.SetClip(properties->ClipPathClip());
      }
      path_based_clip_path_scope.emplace(
          context.GetPaintController(), state,
          *paint_layer_.GetCompositedLayerMapping()->MaskLayer(),
          DisplayItem::PaintPhaseToDrawingType(PaintPhase::kClippingMask));
    }
    const GraphicsLayer* mask_layer =
        paint_layer_.GetCompositedLayerMapping()->MaskLayer();
    ClipRect layer_rect = LayoutRect(
        LayoutPoint(LayoutSize(mask_layer->OffsetFromLayoutObject())),
        LayoutSize(mask_layer->Size()));
    FillMaskingFragment(context, layer_rect, *mask_layer);
  }

  clip_path_clipper = base::nullopt;

  if (should_paint_content && !selection_only) {
    // Paint the border radius mask for the fragments.
    if (paint_flags & kPaintLayerPaintingAncestorClippingMaskPhase) {
      // |layer_fragments| comes from the compositing container which doesn't
      // have multiple fragments.
      DCHECK_EQ(1u, layer_fragments.size());
      PaintAncestorClippingMask(layer_fragments[0], context,
                                local_painting_info, paint_flags);
    }
    if (paint_flags & kPaintLayerPaintingChildClippingMaskPhase) {
      PaintChildClippingMaskForFragments(layer_fragments, context,
                                         local_painting_info, paint_flags);
    }
  }

  if (subsequence_recorder)
    paint_layer_.SetPreviousPaintResult(result);
  return result;
}

bool PaintLayerPainter::NeedsToClip(
    const PaintLayerPaintingInfo& local_painting_info,
    const ClipRect& clip_rect,
    const PaintLayerFlags& paint_flags,
    const LayoutBoxModelObject& layout_object) {
  // Other clipping will be applied by property nodes directly for SPv175+.
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return false;

  // Always clip if painting an ancestor clipping mask layer.
  if (paint_flags & kPaintLayerPaintingAncestorClippingMaskPhase)
    return true;

  // Embedded objects have a clip rect when border radius is present
  // because we need it for the border radius mask with composited
  // chidren. However, we do not want to apply the clip when painting
  // the embedded content itself. Doing so would clip out the
  // border because LayoutEmbeddedObject does not obey the painting phases
  // of a normal box object.
  if (layout_object.IsLayoutEmbeddedContent() &&
      layout_object.GetCompositingState() == kPaintsIntoOwnBacking)
    return paint_flags & kPaintLayerPaintingChildClippingMaskPhase;

  return clip_rect.Rect() != local_painting_info.paint_dirty_rect ||
         clip_rect.HasRadius();
}

bool PaintLayerPainter::AtLeastOneFragmentIntersectsDamageRect(
    PaintLayerFragments& fragments,
    const PaintLayerPaintingInfo& local_painting_info,
    PaintLayerFlags local_paint_flags,
    const LayoutPoint& offset_from_root) {
  if (paint_layer_.EnclosingPaginationLayer())
    return true;  // The fragments created have already been found to intersect
                  // with the damage rect.

  if (&paint_layer_ == local_painting_info.root_layer &&
      (local_paint_flags & kPaintLayerPaintingOverflowContents))
    return true;

  for (PaintLayerFragment& fragment : fragments) {
    LayoutPoint new_offset_from_root =
        offset_from_root + fragment.pagination_offset;
    // Note that this really only works reliably on the first fragment. If the
    // layer has visible overflow and a subsequent fragment doesn't intersect
    // with the border box of the layer (i.e. only contains an overflow portion
    // of the layer), intersection will fail. The reason for this is that
    // fragment.layerBounds is set to the border box, not the bounding box, of
    // the layer.
    if (paint_layer_.IntersectsDamageRect(fragment.layer_bounds,
                                          fragment.background_rect.Rect(),
                                          new_offset_from_root))
      return true;
  }
  return false;
}

void PaintLayerPainter::RepeatFixedPositionObjectInPages(
    const PaintLayerFragment& single_fragment_ignored_pagination,
    const PaintLayerPaintingInfo& painting_info,
    PaintLayerFragments& layer_fragments) {
  DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
  DCHECK(paint_layer_.GetLayoutObject().IsFixedPositionObjectInPagedMedia());

  LayoutView* view = paint_layer_.GetLayoutObject().View();
  unsigned pages =
      ceilf(view->DocumentRect().Height() / view->PageLogicalHeight());

  // The fixed position object is offset from the top of the page, so remove
  // any scroll offset.
  LayoutPoint offset_from_root;
  paint_layer_.ConvertToLayerCoords(painting_info.root_layer, offset_from_root);
  LayoutSize offset_adjustment = paint_layer_.Location() - offset_from_root;
  layer_fragments.push_back(single_fragment_ignored_pagination);
  layer_fragments[0].pagination_offset += offset_adjustment;
  layer_fragments[0].layer_bounds.Move(offset_adjustment);

  LayoutPoint page_offset(LayoutUnit(), view->PageLogicalHeight());
  for (unsigned i = 1; i < pages; i++) {
    PaintLayerFragment fragment = layer_fragments[i - 1];
    fragment.pagination_offset += page_offset;
    fragment.layer_bounds.MoveBy(page_offset);
    layer_fragments.push_back(fragment);
  }
}

template <typename Function>
static void ForAllFragments(GraphicsContext& context,
                            const PaintLayerFragments& fragments,
                            const Function& function) {
  for (size_t i = 0; i < fragments.size(); ++i) {
    base::Optional<ScopedDisplayItemFragment> scoped_display_item_fragment;
    if (i)
      scoped_display_item_fragment.emplace(context, i);
    function(fragments[i]);
  }
}

PaintResult PaintLayerPainter::PaintLayerWithAdjustedRoot(
    GraphicsContext& context,
    const PaintLayerPaintingInfo& painting_info,
    PaintLayerFlags paint_flags) {
  TransformationMatrix layer_transform =
      paint_layer_.RenderableTransform(painting_info.GetGlobalPaintFlags());
  // If the transform can't be inverted, then don't paint anything.
  if (!layer_transform.IsInvertible())
    return kFullyPainted;

  // FIXME: We should make sure that we don't walk past paintingInfo.rootLayer
  // here.  m_paintLayer may be the "root", and then we should avoid looking at
  // its parent.
  PaintLayer* parent_layer = paint_layer_.Parent();

  PaintResult result = kFullyPainted;
  PaintLayerFragments layer_fragments;
  bool is_fixed_position_object_in_paged_media =
      !RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
      paint_layer_.GetLayoutObject().IsFixedPositionObjectInPagedMedia();

  // This works around a bug in squashed-layer painting.
  // Squashed layers paint into a backing in its compositing container's
  // space, but painting_info.root_layer points to the squashed layer
  // itself, thus PaintLayerClipper would return a clip rect in the
  // squashed layer's local space, instead of the backing's space.
  // Fortunately, CompositedLayerMapping::DoPaintTask already applied
  // appropriate ancestor clip for us, so we can simply skip it.
  bool is_squashed_layer = painting_info.root_layer == &paint_layer_;

  if (is_squashed_layer || is_fixed_position_object_in_paged_media) {
    // We don't need to collect any fragments in the regular way here. We have
    // already calculated a clip rectangle for the ancestry if it was needed,
    // and clipping this layer is something that can be done further down the
    // path, when the transform has been applied.
    PaintLayerFragment fragment;
    fragment.background_rect = painting_info.paint_dirty_rect;
    fragment.fragment_data = &paint_layer_.GetLayoutObject().FirstFragment();
    if (is_fixed_position_object_in_paged_media) {
      RepeatFixedPositionObjectInPages(fragment, painting_info,
                                       layer_fragments);
    } else {
      layer_fragments.push_back(fragment);
    }
  } else if (parent_layer) {
    ShouldRespectOverflowClipType respect_overflow_clip =
        ShouldRespectOverflowClip(paint_flags, paint_layer_.GetLayoutObject());
    paint_layer_.CollectFragments(
        layer_fragments, painting_info.root_layer,
        painting_info.paint_dirty_rect, kIgnorePlatformOverlayScrollbarSize,
        respect_overflow_clip, nullptr, painting_info.sub_pixel_accumulation);
    // PaintLayer::CollectFragments depends on the paint dirty rect in
    // complicated ways. For now, always assume a partially painted output
    // for fragmented content.
    if (layer_fragments.size() > 1)
      result = kMayBeClippedByPaintDirtyRect;
  }

  // We have to skip cache for fragments under transform because we will paint
  // all the fragments of sublayers in each fragment like the following:
  //  fragment 0 { sub-layer fragment 0; sub-layer fragment 1 }
  //  fragment 1 { sub-layer fragment 0; sub-layer fragment 1 }
  base::Optional<DisplayItemCacheSkipper> cache_skipper;
  if (layer_fragments.size() > 1)
    cache_skipper.emplace(context);

  ForAllFragments(
      context, layer_fragments, [&](const PaintLayerFragment& fragment) {
        base::Optional<LayerClipRecorder> clip_recorder;
        if (parent_layer &&
            !RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
          if (NeedsToClip(painting_info, fragment.background_rect, paint_flags,
                          paint_layer_.GetLayoutObject())) {
            clip_recorder.emplace(
                context, *parent_layer, DisplayItem::kClipLayerParent,
                fragment.background_rect, painting_info.root_layer,
                fragment.pagination_offset, paint_flags,
                paint_layer_.GetLayoutObject());
          }
        }
        if (paint_layer_.PaintsWithTransform(
                painting_info.GetGlobalPaintFlags())) {
          if (PaintFragmentByApplyingTransform(context, painting_info,
                                               paint_flags, fragment) ==
              kMayBeClippedByPaintDirtyRect)
            result = kMayBeClippedByPaintDirtyRect;
        } else {
          if (PaintSingleFragment(context, painting_info, paint_flags, fragment,
                                  painting_info.sub_pixel_accumulation) ==
              kMayBeClippedByPaintDirtyRect)
            result = kMayBeClippedByPaintDirtyRect;
        }
      });
  return result;
}

PaintResult PaintLayerPainter::PaintFragmentByApplyingTransform(
    GraphicsContext& context,
    const PaintLayerPaintingInfo& painting_info,
    PaintLayerFlags paint_flags,
    const PaintLayerFragment& fragment) {
  // This involves subtracting out the position of the layer in our current
  // coordinate space, but preserving the accumulated error for sub-pixel
  // layout.
  LayoutPoint delta;
  paint_layer_.ConvertToLayerCoords(painting_info.root_layer, delta);
  delta.MoveBy(fragment.pagination_offset);
  delta += painting_info.sub_pixel_accumulation;
  IntPoint rounded_delta = RoundedIntPoint(delta);

  TransformationMatrix transform(
      paint_layer_.RenderableTransform(painting_info.GetGlobalPaintFlags()));
  transform.PostTranslate(rounded_delta.X(), rounded_delta.Y());

  LayoutSize new_sub_pixel_accumulation;
  if (transform.IsIdentityOrTranslation())
    new_sub_pixel_accumulation += delta - rounded_delta;
  // Otherwise discard the sub-pixel remainder because paint offset can't be
  // transformed by a non-translation transform.

  // TODO(jbroman): Put the real transform origin here, instead of using a
  // matrix with the origin baked in.
  FloatPoint3D transform_origin;
  Transform3DRecorder transform3d_recorder(
      context, paint_layer_.GetLayoutObject(),
      DisplayItem::kTransform3DElementTransform, transform, transform_origin);

  return PaintSingleFragment(context, painting_info, paint_flags, fragment,
                             new_sub_pixel_accumulation);
}

PaintResult PaintLayerPainter::PaintSingleFragment(
    GraphicsContext& context,
    const PaintLayerPaintingInfo& painting_info,
    PaintLayerFlags paint_flags,
    const PaintLayerFragment& fragment,
    const LayoutSize& subpixel_accumulation) {
  // Now do a paint with the root layer shifted to be us.
  PaintLayerPaintingInfo new_paint_info(
      &paint_layer_, LayoutRect(LayoutRect::InfiniteIntRect()),
      painting_info.GetGlobalPaintFlags(), subpixel_accumulation);

  if (&paint_layer_ != painting_info.root_layer) {
    // Remove skip root background flag when we're painting with a new root.
    paint_flags &= ~kPaintLayerPaintingSkipRootBackground;
    // When painting a new root we are no longer painting overflow contents.
    paint_flags &= ~kPaintLayerPaintingOverflowContents;
    paint_flags &= ~kPaintLayerPaintingCompositingScrollingPhase;
  }

  return PaintLayerContentsCompositingAllPhases(context, new_paint_info,
                                                paint_flags, &fragment);
}

PaintResult PaintLayerPainter::PaintChildren(
    unsigned children_to_visit,
    GraphicsContext& context,
    const PaintLayerPaintingInfo& painting_info,
    PaintLayerFlags paint_flags) {
  PaintResult result = kFullyPainted;
  if (!paint_layer_.HasSelfPaintingLayerDescendant())
    return result;

#if DCHECK_IS_ON()
  LayerListMutationDetector mutation_checker(paint_layer_.StackingNode());
#endif

  PaintLayerStackingNodeIterator iterator(*paint_layer_.StackingNode(),
                                          children_to_visit);
  PaintLayerStackingNode* child = iterator.Next();
  if (!child)
    return result;

  IntSize scroll_offset_accumulation_for_children =
      painting_info.scroll_offset_accumulation;
  if (paint_layer_.GetLayoutObject().HasOverflowClip()) {
    scroll_offset_accumulation_for_children +=
        paint_layer_.GetLayoutBox()->ScrolledContentOffset();
  }

  for (; child; child = iterator.Next()) {
    // If this Layer should paint into its own backing or a grouped backing,
    // that will be done via CompositedLayerMapping::PaintContents() and
    // CompositedLayerMapping::DoPaintTask().
    if (child->Layer()->PaintsIntoOwnOrGroupedBacking(
            painting_info.GetGlobalPaintFlags()))
      continue;

    if (child->Layer()->IsReplacedNormalFlowStacking())
      continue;

    PaintLayerPaintingInfo child_painting_info = painting_info;
    child_painting_info.scroll_offset_accumulation =
        scroll_offset_accumulation_for_children;
    // Rare case: accumulate scroll offset of non-stacking-context ancestors up
    // to m_paintLayer.
    Vector<PaintLayer*> scroll_parents;
    for (PaintLayer* parent_layer = child->Layer()->Parent();
         parent_layer != &paint_layer_; parent_layer = parent_layer->Parent()) {
      if (parent_layer->GetLayoutObject().HasOverflowClip())
        scroll_parents.push_back(parent_layer);
    }

    for (const auto* scroller : scroll_parents) {
      child_painting_info.scroll_offset_accumulation +=
          scroller->GetLayoutBox()->ScrolledContentOffset();
    }

    if (PaintLayerPainter(*child->Layer())
            .Paint(context, child_painting_info, paint_flags) ==
        kMayBeClippedByPaintDirtyRect)
      result = kMayBeClippedByPaintDirtyRect;
  }

  return result;
}

void PaintLayerPainter::PaintOverflowControlsForFragments(
    const PaintLayerFragments& layer_fragments,
    GraphicsContext& context,
    const PaintLayerPaintingInfo& painting_info,
    PaintLayerFlags paint_flags) {
  PaintLayerScrollableArea* scrollable_area = paint_layer_.GetScrollableArea();
  if (!scrollable_area)
    return;

  ForAllFragments(
      context, layer_fragments, [&](const PaintLayerFragment& fragment) {
        base::Optional<ScopedPaintChunkProperties>
            fragment_paint_chunk_properties;
        if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
          fragment_paint_chunk_properties.emplace(
              context.GetPaintController(),
              fragment.fragment_data->LocalBorderBoxProperties(), paint_layer_,
              DisplayItem::kScrollOverflowControls);
        }

        // We need to apply the same clips and transforms that
        // paintFragmentWithPhase would have.
        LayoutRect cull_rect = fragment.background_rect.Rect();

        base::Optional<LayerClipRecorder> clip_recorder;
        if (NeedsToClip(painting_info, fragment.background_rect, paint_flags,
                        paint_layer_.GetLayoutObject())) {
          clip_recorder.emplace(
              context, paint_layer_, DisplayItem::kClipLayerOverflowControls,
              fragment.background_rect, painting_info.root_layer,
              fragment.pagination_offset, paint_flags,
              paint_layer_.GetLayoutObject());
        }

        base::Optional<ScrollRecorder> scroll_recorder;
        if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
            !painting_info.scroll_offset_accumulation.IsZero()) {
          cull_rect.Move(painting_info.scroll_offset_accumulation);
          scroll_recorder.emplace(context, paint_layer_.GetLayoutObject(),
                                  DisplayItem::kScrollOverflowControls,
                                  painting_info.scroll_offset_accumulation);
        }

        PaintInfo paint_info(
            context, PixelSnappedIntRect(cull_rect),
            PaintPhase::kSelfBlockBackgroundOnly,
            painting_info.GetGlobalPaintFlags(), paint_flags,
            &painting_info.root_layer->GetLayoutObject(),
            fragment.fragment_data
                ? fragment.fragment_data->LogicalTopInFlowThread()
                : LayoutUnit());
        // We pass IntPoint() as the paint offset here, because
        // ScrollableArea::paintOverflowControls just ignores it and uses the
        // offset found in a previous pass.
        ScrollableAreaPainter(*scrollable_area)
            .PaintOverflowControls(paint_info, IntPoint(),
                                   true /* painting_overlay_controls */);
      });
}

void PaintLayerPainter::PaintFragmentWithPhase(
    PaintPhase phase,
    const PaintLayerFragment& fragment,
    GraphicsContext& context,
    const ClipRect& clip_rect,
    const PaintLayerPaintingInfo& painting_info,
    PaintLayerFlags paint_flags,
    ClipState clip_state) {
  DCHECK(paint_layer_.IsSelfPaintingLayer());

  base::Optional<ScopedPaintChunkProperties> fragment_paint_chunk_properties;
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    DCHECK(phase != PaintPhase::kClippingMask);
    auto chunk_properties = fragment.fragment_data->LocalBorderBoxProperties();
    if (phase == PaintPhase::kMask) {
      const auto* properties = fragment.fragment_data->PaintProperties();
      DCHECK(properties && properties->Mask());
      chunk_properties.SetEffect(properties->Mask());
      // Special case for SPv1 composited mask layer. Path-based clip-path
      // is only applies to the mask chunk, but not to the layer property
      // or local box box property.
      if (properties->ClipPathClip() &&
          properties->ClipPathClip()->Parent() == properties->MaskClip()) {
        chunk_properties.SetClip(properties->ClipPathClip());
      }
    }
    fragment_paint_chunk_properties.emplace(
        context.GetPaintController(), chunk_properties, paint_layer_,
        DisplayItem::PaintPhaseToDrawingType(phase));
  }

  DisplayItemClient* client = &paint_layer_.GetLayoutObject();
  base::Optional<LayerClipRecorder> clip_recorder;
  if (clip_state != kHasClipped &&
      NeedsToClip(painting_info, clip_rect, paint_flags,
                  paint_layer_.GetLayoutObject())) {
    DisplayItem::Type clip_type =
        DisplayItem::PaintPhaseToClipLayerFragmentType(phase);
    LayerClipRecorder::BorderRadiusClippingRule clipping_rule;
    switch (phase) {
      case PaintPhase::kSelfBlockBackgroundOnly:  // Background painting will
                                                  // handle clipping to self.
      case PaintPhase::kSelfOutlineOnly:
      case PaintPhase::kMask:  // Mask painting will handle clipping to self.
        clipping_rule = LayerClipRecorder::kDoNotIncludeSelfForBorderRadius;
        break;
      case PaintPhase::kClippingMask:
        if (paint_flags & kPaintLayerPaintingAncestorClippingMaskPhase) {
          // The ancestor is the thing that needs to clip, so do not include
          // this layer's clips.
          clipping_rule = LayerClipRecorder::kDoNotIncludeSelfForBorderRadius;
          // The ancestor clipping mask may have a larger visual rect than
          // paint_layer_, since it includes ancestor clips.
          client = paint_layer_.GetCompositedLayerMapping()
                       ->AncestorClippingMaskLayer();
          break;
        }
        FALLTHROUGH;
      default:
        clipping_rule = LayerClipRecorder::kIncludeSelfForBorderRadius;
        break;
    }
    clip_recorder.emplace(context, paint_layer_, clip_type, clip_rect,
                          painting_info.root_layer, fragment.pagination_offset,
                          paint_flags, *client, clipping_rule);
  }

  // If we are painting a mask for any reason and we have already processed the
  // clips, there is no need to go through the remaining painting pipeline.
  // We know that the mask just needs the area bounded by the clip rects to be
  // filled with black.
  if (clip_recorder && phase == PaintPhase::kClippingMask) {
    DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
    FillMaskingFragment(context, clip_rect, *client);
    return;
  }

  LayoutRect new_cull_rect(clip_rect.Rect());
  base::Optional<ScrollRecorder> scroll_recorder;
  LayoutPoint paint_offset = -paint_layer_.LayoutBoxLocation();
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    paint_offset += fragment.fragment_data->PaintOffset();
    // For SPv175+, we paint in the containing transform node's space. Now
    // |new_cull_rect| is in the pixel-snapped border box space of
    // |painting_info.root_layer|. Adjust it to the correct space.
    // |paint_offset| is already in the correct space.
    new_cull_rect.MoveBy(
        RoundedIntPoint(painting_info.root_layer->GetLayoutObject()
                            .FirstFragment()
                            .PaintOffset()));
  } else {
    paint_offset += ToSize(fragment.layer_bounds.Location());
    if (!painting_info.scroll_offset_accumulation.IsZero()) {
      // As a descendant of the root layer, m_paintLayer's painting is not
      // controlled by the ScrollRecorders created by BlockPainter of the
      // ancestor layers up to the root layer, so we need to issue
      // ScrollRecorder for this layer seperately, with the scroll offset
      // accumulated from the root layer to the parent of this layer, to get the
      // same result as ScrollRecorder in BlockPainter.
      paint_offset += painting_info.scroll_offset_accumulation;

      new_cull_rect.Move(painting_info.scroll_offset_accumulation);
      scroll_recorder.emplace(context, paint_layer_.GetLayoutObject(), phase,
                              painting_info.scroll_offset_accumulation);
    }
  }
  PaintInfo paint_info(context, PixelSnappedIntRect(new_cull_rect), phase,
                       painting_info.GetGlobalPaintFlags(), paint_flags,
                       &painting_info.root_layer->GetLayoutObject(),
                       fragment.fragment_data
                           ? fragment.fragment_data->LogicalTopInFlowThread()
                           : LayoutUnit(),
                       // If we had pending stylesheets, we should avoid
                       // painting descendants of layout view to avoid FOUC.
                       paint_layer_.GetLayoutObject()
                           .GetDocument()
                           .DidLayoutWithPendingStylesheets());

  paint_layer_.GetLayoutObject().Paint(paint_info, paint_offset);
}

void PaintLayerPainter::PaintBackgroundForFragments(
    const PaintLayerFragments& layer_fragments,
    GraphicsContext& context,
    const PaintLayerPaintingInfo& local_painting_info,
    PaintLayerFlags paint_flags) {
  ForAllFragments(
      context, layer_fragments, [&](const PaintLayerFragment& fragment) {
        PaintFragmentWithPhase(PaintPhase::kSelfBlockBackgroundOnly, fragment,
                               context, fragment.background_rect,
                               local_painting_info, paint_flags,
                               kHasNotClipped);
      });
}

void PaintLayerPainter::PaintForegroundForFragments(
    const PaintLayerFragments& layer_fragments,
    GraphicsContext& context,
    const PaintLayerPaintingInfo& local_painting_info,
    bool selection_only,
    bool force_paint_chunks,
    PaintLayerFlags paint_flags) {
  // Optimize clipping for the single fragment case.
  bool should_clip = layer_fragments.size() == 1 &&
                     !layer_fragments[0].foreground_rect.IsEmpty();
  ClipState clip_state = kHasNotClipped;
  base::Optional<LayerClipRecorder> clip_recorder;
  if (should_clip &&
      NeedsToClip(local_painting_info, layer_fragments[0].foreground_rect,
                  paint_flags, paint_layer_.GetLayoutObject())) {
    clip_recorder.emplace(
        context, paint_layer_, DisplayItem::kClipLayerForeground,
        layer_fragments[0].foreground_rect, local_painting_info.root_layer,
        layer_fragments[0].pagination_offset, paint_flags,
        paint_layer_.GetLayoutObject());
    clip_state = kHasClipped;
  }

  if (selection_only) {
    PaintForegroundForFragmentsWithPhase(
        PaintPhase::kSelection, layer_fragments, context, local_painting_info,
        paint_flags, clip_state);
  } else {
    if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() ||
        paint_layer_.NeedsPaintPhaseDescendantBlockBackgrounds()) {
      if (force_paint_chunks) {
        context.GetPaintController().ForceNewChunk(
            paint_layer_, DisplayItem::kLayerChunkDescendantBackgrounds);
      }
      size_t size_before =
          context.GetPaintController().NewDisplayItemList().size();
      PaintForegroundForFragmentsWithPhase(
          PaintPhase::kDescendantBlockBackgroundsOnly, layer_fragments, context,
          local_painting_info, paint_flags, clip_state);
      // Don't set the empty flag if we are not painting the whole background.
      if (!(paint_flags & kPaintLayerPaintingSkipRootBackground)) {
        bool phase_is_empty =
            context.GetPaintController().NewDisplayItemList().size() ==
            size_before;
        DCHECK(phase_is_empty ||
               paint_layer_.NeedsPaintPhaseDescendantBlockBackgrounds());
        paint_layer_.SetPreviousPaintPhaseDescendantBlockBackgroundsEmpty(
            phase_is_empty);
      }
    }

    if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() ||
        paint_layer_.NeedsPaintPhaseFloat()) {
      if (force_paint_chunks) {
        context.GetPaintController().ForceNewChunk(
            paint_layer_, DisplayItem::kLayerChunkFloat);
      }
      size_t size_before =
          context.GetPaintController().NewDisplayItemList().size();
      PaintForegroundForFragmentsWithPhase(PaintPhase::kFloat, layer_fragments,
                                           context, local_painting_info,
                                           paint_flags, clip_state);
      bool phase_is_empty =
          context.GetPaintController().NewDisplayItemList().size() ==
          size_before;
      DCHECK(phase_is_empty || paint_layer_.NeedsPaintPhaseFloat());
      paint_layer_.SetPreviousPaintPhaseFloatEmpty(phase_is_empty);
    }

    if (force_paint_chunks) {
      context.GetPaintController().ForceNewChunk(
          paint_layer_, DisplayItem::kLayerChunkForeground);
    }

    PaintForegroundForFragmentsWithPhase(
        PaintPhase::kForeground, layer_fragments, context, local_painting_info,
        paint_flags, clip_state);

    if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() ||
        paint_layer_.NeedsPaintPhaseDescendantOutlines()) {
      size_t size_before =
          context.GetPaintController().NewDisplayItemList().size();
      PaintForegroundForFragmentsWithPhase(
          PaintPhase::kDescendantOutlinesOnly, layer_fragments, context,
          local_painting_info, paint_flags, clip_state);
      bool phase_is_empty =
          context.GetPaintController().NewDisplayItemList().size() ==
          size_before;
      DCHECK(phase_is_empty ||
             paint_layer_.NeedsPaintPhaseDescendantOutlines());
      paint_layer_.SetPreviousPaintPhaseDescendantOutlinesEmpty(phase_is_empty);
    }
  }
}

void PaintLayerPainter::PaintForegroundForFragmentsWithPhase(
    PaintPhase phase,
    const PaintLayerFragments& layer_fragments,
    GraphicsContext& context,
    const PaintLayerPaintingInfo& local_painting_info,
    PaintLayerFlags paint_flags,
    ClipState clip_state) {
  ForAllFragments(
      context, layer_fragments, [&](const PaintLayerFragment& fragment) {
        if (!fragment.foreground_rect.IsEmpty()) {
          PaintFragmentWithPhase(phase, fragment, context,
                                 fragment.foreground_rect, local_painting_info,
                                 paint_flags, clip_state);
        }
      });
}

void PaintLayerPainter::PaintSelfOutlineForFragments(
    const PaintLayerFragments& layer_fragments,
    GraphicsContext& context,
    const PaintLayerPaintingInfo& local_painting_info,
    PaintLayerFlags paint_flags) {
  ForAllFragments(
      context, layer_fragments, [&](const PaintLayerFragment& fragment) {
        if (!fragment.background_rect.IsEmpty()) {
          PaintFragmentWithPhase(PaintPhase::kSelfOutlineOnly, fragment,
                                 context, fragment.background_rect,
                                 local_painting_info, paint_flags,
                                 kHasNotClipped);
        }
      });
}

void PaintLayerPainter::PaintMaskForFragments(
    const PaintLayerFragments& layer_fragments,
    GraphicsContext& context,
    const PaintLayerPaintingInfo& local_painting_info,
    PaintLayerFlags paint_flags) {
  ForAllFragments(
      context, layer_fragments, [&](const PaintLayerFragment& fragment) {
        PaintFragmentWithPhase(PaintPhase::kMask, fragment, context,
                               fragment.background_rect, local_painting_info,
                               paint_flags, kHasNotClipped);
      });
}

void PaintLayerPainter::PaintAncestorClippingMask(
    const PaintLayerFragment& fragment,
    GraphicsContext& context,
    const PaintLayerPaintingInfo& local_painting_info,
    PaintLayerFlags paint_flags) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    const DisplayItemClient& client =
        *paint_layer_.GetCompositedLayerMapping()->AncestorClippingMaskLayer();
    const auto& layer_fragment = paint_layer_.GetLayoutObject().FirstFragment();
    auto state = layer_fragment.PreEffectProperties();
    // This is a hack to incorporate mask-based clip-path.
    // See CompositingLayerPropertyUpdater.cpp about AncestorClippingMaskLayer.
    state.SetEffect(layer_fragment.PreFilter());
    ScopedPaintChunkProperties properties(
        context.GetPaintController(), state, client,
        DisplayItem::PaintPhaseToDrawingType(PaintPhase::kClippingMask));
    ClipRect mask_rect = fragment.background_rect;
    mask_rect.MoveBy(layer_fragment.PaintOffset());
    FillMaskingFragment(context, mask_rect, client);
    return;
  }

  PaintFragmentWithPhase(PaintPhase::kClippingMask, fragment, context,
                         fragment.background_rect, local_painting_info,
                         paint_flags, kHasNotClipped);
}

void PaintLayerPainter::PaintChildClippingMaskForFragments(
    const PaintLayerFragments& layer_fragments,
    GraphicsContext& context,
    const PaintLayerPaintingInfo& local_painting_info,
    PaintLayerFlags paint_flags) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    const DisplayItemClient& client =
        *paint_layer_.GetCompositedLayerMapping()->ChildClippingMaskLayer();
    ForAllFragments(
        context, layer_fragments, [&](const PaintLayerFragment& fragment) {
          // Use the LocalBorderboxProperties as a starting point to ensure that
          // we don't include the scroll offset when painting the mask layer.
          auto state = fragment.fragment_data->LocalBorderBoxProperties();
          // This is a hack to incorporate mask-based clip-path.
          // See CompositingLayerPropertyUpdater.cpp about
          // ChildClippingMaskLayer.
          state.SetEffect(fragment.fragment_data->PreFilter());
          // Update the clip to be the ContentsProperties clip, since it
          // includes the InnerBorderRadiusClip.
          state.SetClip(fragment.fragment_data->ContentsProperties().Clip());
          ScopedPaintChunkProperties fragment_paint_chunk_properties(
              context.GetPaintController(), state, client,
              DisplayItem::PaintPhaseToDrawingType(PaintPhase::kClippingMask));
          ClipRect mask_rect = fragment.background_rect;
          FillMaskingFragment(context, mask_rect, client);
        });
    return;
  }

  ForAllFragments(
      context, layer_fragments, [&](const PaintLayerFragment& fragment) {
        PaintFragmentWithPhase(PaintPhase::kClippingMask, fragment, context,
                               fragment.foreground_rect, local_painting_info,
                               paint_flags, kHasNotClipped);
      });
}

void PaintLayerPainter::PaintOverlayScrollbars(
    GraphicsContext& context,
    const LayoutRect& damage_rect,
    const GlobalPaintFlags paint_flags) {
  if (!paint_layer_.ContainsDirtyOverlayScrollbars())
    return;

  PaintLayerPaintingInfo painting_info(
      &paint_layer_, LayoutRect(EnclosingIntRect(damage_rect)), paint_flags,
      LayoutSize());
  Paint(context, painting_info, kPaintLayerPaintingOverlayScrollbars);

  paint_layer_.SetContainsDirtyOverlayScrollbars(false);
}

void PaintLayerPainter::FillMaskingFragment(GraphicsContext& context,
                                            const ClipRect& clip_rect,
                                            const DisplayItemClient& client) {
  DisplayItem::Type type =
      DisplayItem::PaintPhaseToDrawingType(PaintPhase::kClippingMask);
  if (DrawingRecorder::UseCachedDrawingIfPossible(context, client, type))
    return;

  DrawingRecorder recorder(context, client, type);
  IntRect snapped_clip_rect = PixelSnappedIntRect(clip_rect.Rect());
  context.FillRect(snapped_clip_rect, Color::kBlack);
}

// Generate a no-op DrawingDisplayItem to ensure a non-empty chunk for the
// filter without content.
void PaintLayerPainter::PaintEmptyContentForFilters(GraphicsContext& context) {
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
  DCHECK(paint_layer_.PaintsWithFilters());

  ScopedPaintChunkProperties paint_chunk_properties(
      context.GetPaintController(),
      paint_layer_.GetLayoutObject().FirstFragment().LocalBorderBoxProperties(),
      paint_layer_, DisplayItem::kEmptyContentForFilters);
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, paint_layer_, DisplayItem::kEmptyContentForFilters))
    return;
  DrawingRecorder recorder(context, paint_layer_,
                           DisplayItem::kEmptyContentForFilters);
}

}  // namespace blink
