/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/paint/link_highlight_impl.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "cc/layers/layer.h"
#include "cc/layers/picture_layer.h"
#include "cc/paint/display_item_list.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_float_point.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/renderer/core/dom/layout_tree_builder_traversal.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/exported/web_settings_impl.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/web_frame_widget_base.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/animation/compositor_animation_curve.h"
#include "third_party/blink/renderer/platform/animation/compositor_float_animation_curve.h"
#include "third_party/blink/renderer/platform/animation/compositor_keyframe_model.h"
#include "third_party/blink/renderer/platform/animation/compositor_target_property.h"
#include "third_party/blink/renderer/platform/animation/timing_function.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_canvas.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_recorder.h"
#include "third_party/blink/renderer/platform/layout_test_support.h"
#include "third_party/blink/renderer/platform/wtf/time.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "third_party/skia/include/core/SkMatrix44.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/size_f.h"

namespace blink {

std::unique_ptr<LinkHighlightImpl> LinkHighlightImpl::Create(
    Node* node,
    WebViewImpl* owning_web_view) {
  return base::WrapUnique(new LinkHighlightImpl(node, owning_web_view));
}

LinkHighlightImpl::LinkHighlightImpl(Node* node, WebViewImpl* owning_web_view)
    : node_(node),
      owning_web_view_(owning_web_view),
      current_graphics_layer_(nullptr),
      is_scrolling_graphics_layer_(false),
      geometry_needs_update_(false),
      is_animating_(false),
      start_time_(CurrentTimeTicksInSeconds()),
      unique_id_(NewUniqueObjectId()) {
  DCHECK(node_);
  DCHECK(owning_web_view);
  content_layer_ = cc::PictureLayer::Create(this);
  clip_layer_ = cc::Layer::Create();
  clip_layer_->SetTransformOrigin(FloatPoint3D());
  clip_layer_->AddChild(content_layer_);

  compositor_animation_ = CompositorAnimation::Create();
  DCHECK(compositor_animation_);
  compositor_animation_->SetAnimationDelegate(this);
  if (owning_web_view_->LinkHighlightsTimeline())
    owning_web_view_->LinkHighlightsTimeline()->AnimationAttached(*this);

  CompositorElementId element_id =
      CompositorElementIdFromUniqueObjectId(unique_id_);
  compositor_animation_->AttachElement(element_id);
  content_layer_->SetIsDrawable(true);
  content_layer_->SetOpacity(1);
  content_layer_->SetElementId(element_id);
  geometry_needs_update_ = true;
}

LinkHighlightImpl::~LinkHighlightImpl() {
  if (compositor_animation_->IsElementAttached())
    compositor_animation_->DetachElement();
  if (owning_web_view_->LinkHighlightsTimeline())
    owning_web_view_->LinkHighlightsTimeline()->AnimationDestroyed(*this);
  compositor_animation_->SetAnimationDelegate(nullptr);
  compositor_animation_.reset();

  ClearGraphicsLayerLinkHighlightPointer();
  ReleaseResources();
}

cc::PictureLayer* LinkHighlightImpl::ContentLayer() {
  return content_layer_.get();
}

cc::Layer* LinkHighlightImpl::ClipLayer() {
  return clip_layer_.get();
}

void LinkHighlightImpl::ReleaseResources() {
  node_.Clear();
}

void LinkHighlightImpl::AttachLinkHighlightToCompositingLayer(
    const LayoutBoxModelObject& paint_invalidation_container) {
  GraphicsLayer* new_graphics_layer =
      paint_invalidation_container.Layer()->GraphicsLayerBacking(
          node_->GetLayoutObject());
  is_scrolling_graphics_layer_ = false;
  // FIXME: There should always be a GraphicsLayer. See crbug.com/431961.
  if (paint_invalidation_container.Layer()->NeedsCompositedScrolling() &&
      node_->GetLayoutObject() != &paint_invalidation_container) {
    is_scrolling_graphics_layer_ = true;
  }
  if (!new_graphics_layer)
    return;

  clip_layer_->SetTransform(gfx::Transform());

  if (current_graphics_layer_ != new_graphics_layer) {
    if (current_graphics_layer_)
      ClearGraphicsLayerLinkHighlightPointer();

    current_graphics_layer_ = new_graphics_layer;
    current_graphics_layer_->AddLinkHighlight(this);
  }
}

static void AddQuadToPath(const FloatQuad& quad, Path& path) {
  // FIXME: Make this create rounded quad-paths, just like the axis-aligned
  // case.
  path.MoveTo(quad.P1());
  path.AddLineTo(quad.P2());
  path.AddLineTo(quad.P3());
  path.AddLineTo(quad.P4());
  path.CloseSubpath();
}

void LinkHighlightImpl::ComputeQuads(const Node& node,
                                     Vector<FloatQuad>& out_quads) const {
  if (!node.GetLayoutObject())
    return;

  LayoutObject* layout_object = node.GetLayoutObject();

  // For inline elements, absoluteQuads will return a line box based on the
  // line-height and font metrics, which is technically incorrect as replaced
  // elements like images should use their intristic height and expand the
  // linebox  as needed. To get an appropriately sized highlight we descend
  // into the children and have them add their boxes.
  if (layout_object->IsLayoutInline()) {
    for (Node* child = LayoutTreeBuilderTraversal::FirstChild(node); child;
         child = LayoutTreeBuilderTraversal::NextSibling(*child))
      ComputeQuads(*child, out_quads);
  } else {
    // FIXME: this does not need to be absolute, just in the paint invalidation
    // container's space.
    layout_object->AbsoluteQuads(out_quads, kTraverseDocumentBoundaries);
  }
}

bool LinkHighlightImpl::ComputeHighlightLayerPathAndPosition(
    const LayoutBoxModelObject& paint_invalidation_container) {
  if (!node_ || !node_->GetLayoutObject() || !current_graphics_layer_)
    return false;

  // FIXME: This is defensive code to avoid crashes such as those described in
  // crbug.com/440887. This should be cleaned up once we fix the root cause of
  // of the paint invalidation container not being composited.
  if (!paint_invalidation_container.Layer()->GetCompositedLayerMapping() &&
      !paint_invalidation_container.Layer()->GroupedMapping())
    return false;

  // Get quads for node in absolute coordinates.
  Vector<FloatQuad> quads;
  ComputeQuads(*node_, quads);
  DCHECK(quads.size());
  Path new_path;

  for (size_t quad_index = 0; quad_index < quads.size(); ++quad_index) {
    FloatQuad absolute_quad = quads[quad_index];

    // Scrolling content layers have the same offset from layout object as the
    // non-scrolling layers. Thus we need to adjust for their scroll offset.
    if (is_scrolling_graphics_layer_) {
      FloatPoint scroll_position = paint_invalidation_container.Layer()
                                       ->GetScrollableArea()
                                       ->ScrollPosition();
      absolute_quad.Move(ToScrollOffset(scroll_position));
    }

    absolute_quad.SetP1(RoundedIntPoint(absolute_quad.P1()));
    absolute_quad.SetP2(RoundedIntPoint(absolute_quad.P2()));
    absolute_quad.SetP3(RoundedIntPoint(absolute_quad.P3()));
    absolute_quad.SetP4(RoundedIntPoint(absolute_quad.P4()));
    FloatQuad transformed_quad =
        paint_invalidation_container.AbsoluteToLocalQuad(
            absolute_quad, kUseTransforms | kTraverseDocumentBoundaries);
    FloatPoint offset_to_backing;

    PaintLayer::MapPointInPaintInvalidationContainerToBacking(
        paint_invalidation_container, offset_to_backing);

    // Adjust for offset from LayoutObject.
    offset_to_backing.Move(-current_graphics_layer_->OffsetFromLayoutObject());

    transformed_quad.Move(ToFloatSize(offset_to_backing));

    // FIXME: for now, we'll only use rounded paths if we have a single node
    // quad. The reason for this is that we may sometimes get a chain of
    // adjacent boxes (e.g. for text nodes) which end up looking like sausage
    // links: these should ideally be merged into a single rect before creating
    // the path, but that's another CL.
    if (quads.size() == 1 && transformed_quad.IsRectilinear() &&
        !owning_web_view_->SettingsImpl()->MockGestureTapHighlightsEnabled()) {
      FloatSize rect_rounding_radii(3, 3);
      new_path.AddRoundedRect(transformed_quad.BoundingBox(),
                              rect_rounding_radii);
    } else {
      AddQuadToPath(transformed_quad, new_path);
    }
  }

  FloatRect bounding_rect = new_path.BoundingRect();
  new_path.Translate(-ToFloatSize(bounding_rect.Location()));

  bool path_has_changed = !(new_path == path_);
  if (path_has_changed) {
    path_ = new_path;
    content_layer_->SetBounds(
        static_cast<gfx::Size>(EnclosingIntRect(bounding_rect).Size()));
  }

  content_layer_->SetPosition(bounding_rect.Location());

  return path_has_changed;
}

gfx::Rect LinkHighlightImpl::PaintableRegion() {
  return gfx::Rect(content_layer_->bounds());
}

scoped_refptr<cc::DisplayItemList>
LinkHighlightImpl::PaintContentsToDisplayList(
    PaintingControlSetting painting_control) {
  auto display_list = base::MakeRefCounted<cc::DisplayItemList>();
  if (!node_ || !node_->GetLayoutObject()) {
    display_list->Finalize();
    return display_list;
  }

  PaintRecorder recorder;
  gfx::Rect record_bounds = PaintableRegion();
  PaintCanvas* canvas =
      recorder.beginRecording(record_bounds.width(), record_bounds.height());

  PaintFlags flags;
  flags.setStyle(PaintFlags::kFill_Style);
  flags.setAntiAlias(true);
  flags.setColor(node_->GetLayoutObject()->Style()->TapHighlightColor().Rgb());
  canvas->drawPath(path_.GetSkPath(), flags);

  display_list->StartPaint();
  display_list->push<cc::DrawRecordOp>(recorder.finishRecordingAsPicture());
  display_list->EndPaintOfUnpaired(record_bounds);

  display_list->Finalize();
  return display_list;
}

void LinkHighlightImpl::StartHighlightAnimationIfNeeded() {
  if (is_animating_)
    return;

  is_animating_ = true;
  const float kStartOpacity = 1;
  // FIXME: Should duration be configurable?
  const float kFadeDuration = 0.1f;
  const float kMinPreFadeDuration = 0.1f;

  content_layer_->SetOpacity(kStartOpacity);

  std::unique_ptr<CompositorFloatAnimationCurve> curve =
      CompositorFloatAnimationCurve::Create();

  const auto& timing_function = *CubicBezierTimingFunction::Preset(
      CubicBezierTimingFunction::EaseType::EASE);

  curve->AddKeyframe(
      CompositorFloatKeyframe(0, kStartOpacity, timing_function));
  // Make sure we have displayed for at least minPreFadeDuration before starting
  // to fade out.
  float extra_duration_required = std::max(
      0.f, kMinPreFadeDuration -
               static_cast<float>(CurrentTimeTicksInSeconds() - start_time_));
  if (extra_duration_required) {
    curve->AddKeyframe(CompositorFloatKeyframe(extra_duration_required,
                                               kStartOpacity, timing_function));
  }
  // For layout tests we don't fade out.
  curve->AddKeyframe(CompositorFloatKeyframe(
      kFadeDuration + extra_duration_required,
      LayoutTestSupport::IsRunningLayoutTest() ? kStartOpacity : 0,
      timing_function));

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      CompositorKeyframeModel::Create(*curve, CompositorTargetProperty::OPACITY,
                                      0, 0);

  content_layer_->SetIsDrawable(true);
  compositor_animation_->AddKeyframeModel(std::move(keyframe_model));

  Invalidate();
  owning_web_view_->MainFrameImpl()->FrameWidgetImpl()->ScheduleAnimation();
}

void LinkHighlightImpl::ClearGraphicsLayerLinkHighlightPointer() {
  if (current_graphics_layer_) {
    current_graphics_layer_->RemoveLinkHighlight(this);
    current_graphics_layer_ = nullptr;
  }
}

void LinkHighlightImpl::NotifyAnimationStarted(double, int) {}

void LinkHighlightImpl::NotifyAnimationFinished(double, int) {
  // Since WebViewImpl may hang on to us for a while, make sure we
  // release resources as soon as possible.
  ClearGraphicsLayerLinkHighlightPointer();
  ReleaseResources();
}

class LinkHighlightDisplayItemClientForTracking : public DisplayItemClient {
  String DebugName() const final { return "LinkHighlight"; }
  LayoutRect VisualRect() const final { return LayoutRect(); }
};

void LinkHighlightImpl::UpdateGeometry() {
  // To avoid unnecessary updates (e.g. other entities have requested animations
  // from our WebViewImpl), only proceed if we actually requested an update.
  if (!geometry_needs_update_)
    return;

  geometry_needs_update_ = false;

  bool has_layout_object = node_ && node_->GetLayoutObject();
  if (has_layout_object) {
    const LayoutBoxModelObject& paint_invalidation_container =
        node_->GetLayoutObject()->ContainerForPaintInvalidation();
    AttachLinkHighlightToCompositingLayer(paint_invalidation_container);
    if (ComputeHighlightLayerPathAndPosition(paint_invalidation_container)) {
      // We only need to invalidate the layer if the highlight size has changed,
      // otherwise we can just re-position the layer without needing to
      // repaint.
      content_layer_->SetNeedsDisplay();

      if (current_graphics_layer_) {
        gfx::Rect rect = gfx::ToEnclosingRect(
            gfx::RectF(Layer()->position(), gfx::SizeF(Layer()->bounds())));
        current_graphics_layer_->TrackRasterInvalidation(
            LinkHighlightDisplayItemClientForTracking(), IntRect(rect),
            PaintInvalidationReason::kFull);
      }
    }
  } else {
    ClearGraphicsLayerLinkHighlightPointer();
    ReleaseResources();
  }
}

void LinkHighlightImpl::ClearCurrentGraphicsLayer() {
  current_graphics_layer_ = nullptr;
  geometry_needs_update_ = true;
}

void LinkHighlightImpl::Invalidate() {
  // Make sure we update geometry on the next callback from
  // WebViewImpl::layout().
  geometry_needs_update_ = true;
}

cc::Layer* LinkHighlightImpl::Layer() {
  return ClipLayer();
}

CompositorAnimation* LinkHighlightImpl::GetCompositorAnimation() const {
  return compositor_animation_.get();
}

}  // namespace blink
