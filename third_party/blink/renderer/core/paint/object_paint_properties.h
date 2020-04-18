// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_OBJECT_PAINT_PROPERTIES_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_OBJECT_PAINT_PROPERTIES_H_

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_paint_property_node.h"
#include "third_party/blink/renderer/platform/graphics/paint/effect_paint_property_node.h"
#include "third_party/blink/renderer/platform/graphics/paint/scroll_paint_property_node.h"
#include "third_party/blink/renderer/platform/graphics/paint/transform_paint_property_node.h"

namespace blink {

// This class stores the paint property nodes created by a LayoutObject. The
// object owns each of the property nodes directly and RefPtrs are only used to
// harden against use-after-free bugs. These paint properties are built/updated
// by PaintPropertyTreeBuilder during the PrePaint lifecycle step.
//
// [update & clear implementation note] This class has Update[property](...) and
// Clear[property]() helper functions for efficiently creating and updating
// properties. The update functions returns a 3-state result to indicate whether
// the value or the existence of the node has changed. They use a create-or-
// update pattern of re-using existing properties for efficiency:
// 1. It avoids extra allocations.
// 2. It preserves existing child->parent pointers.
// The clear functions return true if an existing node is removed. Property
// nodes store parent pointers but not child pointers and these return values
// are important for catching property tree structure changes which require
// updating descendant's parent pointers.
class CORE_EXPORT ObjectPaintProperties {
  USING_FAST_MALLOC(ObjectPaintProperties);

 public:
  static std::unique_ptr<ObjectPaintProperties> Create() {
    return base::WrapUnique(new ObjectPaintProperties());
  }

  // The hierarchy of the transform subtree created by a LayoutObject is as
  // follows:
  // [ paintOffsetTranslation ]           Normally paint offset is accumulated
  // |                                    without creating a node until we see,
  // |                                    for example, transform or
  // |                                    position:fixed.
  // +---[ transform ]                    The space created by CSS transform.
  //     |                                This is the local border box space.
  //     +---[ perspective ]              The space created by CSS perspective.
  //         +---[ svgLocalToBorderBoxTransform ] Additional transform for
  //                                      children of the outermost root SVG.
  //                    OR                (SVG does not support scrolling.)
  //         +---[ scrollTranslation ]    The space created by overflow clip.
  const TransformPaintPropertyNode* PaintOffsetTranslation() const {
    return paint_offset_translation_.get();
  }
  const TransformPaintPropertyNode* Transform() const {
    return transform_.get();
  }
  const TransformPaintPropertyNode* Perspective() const {
    return perspective_.get();
  }
  const TransformPaintPropertyNode* SvgLocalToBorderBoxTransform() const {
    return svg_local_to_border_box_transform_.get();
  }
  const ScrollPaintPropertyNode* Scroll() const { return scroll_.get(); }
  const TransformPaintPropertyNode* ScrollTranslation() const {
    return scroll_translation_.get();
  }

  // The hierarchy of the effect subtree created by a LayoutObject is as
  // follows:
  // [ effect ]
  // |   Isolated group to apply various CSS effects, including opacity,
  // |   mix-blend-mode, and for isolation if a mask needs to be applied or
  // |   backdrop-dependent children are present.
  // +-[ filter ]
  // |     Isolated group for CSS filter.
  // +-[ mask ]
  // |     Isolated group for painting the CSS mask. This node will have
  // |     SkBlendMode::kDstIn and shall paint last, i.e. after masked contents.
  // +-[ clip path ]
  //       Isolated group for painting the CSS clip-path. This node will have
  //       SkBlendMode::kDstIn and shall paint last, i.e. after clipped
  //       contents.
  const EffectPaintPropertyNode* Effect() const { return effect_.get(); }
  const EffectPaintPropertyNode* Filter() const { return filter_.get(); }
  const EffectPaintPropertyNode* Mask() const { return mask_.get(); }
  const EffectPaintPropertyNode* ClipPath() const { return clip_path_.get(); }

  // The hierarchy of the clip subtree created by a LayoutObject is as follows:
  // [ fragment clip ]
  // |    Clips to a fragment's bounds.
  // |    This is only present for content under a fragmentation
  // |    container.
  // | NOTE: for composited SPv1/SPv175 clip path clips, we move clip path clip
  // |       below mask.
  // +-[ clip path clip ]
  //   |  Clip created by path-based CSS clip-path. Only exists if the
  //  /   clip-path is "simple" that can be applied geometrically. This and
  // /    the clip path effect node are mutually exclusive.
  // +-[ mask clip ]
  //   |   Clip created by CSS mask or CSS clip-path. It serves two purposes:
  //   |   1. Cull painting of the masked subtree. Because anything outside of
  //   |      the mask is never visible, it is pointless to paint them.
  //   |   2. Raster clip of the masked subtree. Because the mask implemented
  //   |      as SkBlendMode::kDstIn, pixels outside of mask's bound will be
  //   |      intact when they shall be masked out. This clip ensures no pixels
  //   |      leak out.
  //   +-[ css clip ]
  //     |   Clip created by CSS clip. CSS clip applies to all descendants, this
  //     |   node only applies to containing block descendants. For descendants
  //     |   not contained by this object, use [ css clip fixed position ].
  //     +-[ overflow controls clip ]
  //     |   Clip created by overflow clip to clip overflow controls
  //     |   (scrollbars, resizer, scroll corner) that would overflow the box.
  //     +-[ inner border radius clip]
  //       |   Clip created by a rounded border with overflow clip. This clip is
  //       |   not inset by scrollbars.
  //       +-[ overflow clip ]
  //             Clip created by overflow clip and is inset by the scrollbar.
  //   [ css clip fixed position ]
  //       Clip created by CSS clip. Only exists if the current clip includes
  //       some clip that doesn't apply to our fixed position descendants.
  const ClipPaintPropertyNode* FragmentClip() const {
    return fragment_clip_.get();
  }
  const ClipPaintPropertyNode* ClipPathClip() const {
    return clip_path_clip_.get();
  }
  const ClipPaintPropertyNode* MaskClip() const { return mask_clip_.get(); }
  const ClipPaintPropertyNode* CssClip() const { return css_clip_.get(); }
  const ClipPaintPropertyNode* CssClipFixedPosition() const {
    return css_clip_fixed_position_.get();
  }
  const ClipPaintPropertyNode* OverflowControlsClip() const {
    return overflow_controls_clip_.get();
  }
  const ClipPaintPropertyNode* InnerBorderRadiusClip() const {
    return inner_border_radius_clip_.get();
  }
  const ClipPaintPropertyNode* OverflowClip() const {
    return overflow_clip_.get();
  }

  // The following clear* functions return true if the property tree structure
  // changes (an existing node was deleted), and false otherwise. See the
  // class-level comment ("update & clear implementation note") for details
  // about why this is needed for efficient updates.
  bool ClearPaintOffsetTranslation() {
    return Clear(paint_offset_translation_);
  }
  bool ClearTransform() { return Clear(transform_); }
  bool ClearEffect() { return Clear(effect_); }
  bool ClearFilter() { return Clear(filter_); }
  bool ClearMask() { return Clear(mask_); }
  bool ClearClipPath() { return Clear(clip_path_); }
  bool ClearFragmentClip() { return Clear(fragment_clip_); }
  bool ClearClipPathClip() { return Clear(clip_path_clip_); }
  bool ClearMaskClip() { return Clear(mask_clip_); }
  bool ClearCssClip() { return Clear(css_clip_); }
  bool ClearCssClipFixedPosition() { return Clear(css_clip_fixed_position_); }
  bool ClearOverflowControlsClip() { return Clear(overflow_controls_clip_); }
  bool ClearInnerBorderRadiusClip() { return Clear(inner_border_radius_clip_); }
  bool ClearOverflowClip() { return Clear(overflow_clip_); }
  bool ClearPerspective() { return Clear(perspective_); }
  bool ClearSvgLocalToBorderBoxTransform() {
    return Clear(svg_local_to_border_box_transform_);
  }
  bool ClearScroll() { return Clear(scroll_); }
  bool ClearScrollTranslation() { return Clear(scroll_translation_); }

  class UpdateResult {
   public:
    bool Unchanged() const { return result_ == kUnchanged; }
    bool NewNodeCreated() const { return result_ == kNewNodeCreated; }

   private:
    friend class ObjectPaintProperties;
    enum Result { kUnchanged, kValueChanged, kNewNodeCreated };
    UpdateResult(Result r) : result_(r) {}
    Result result_;
  };

  UpdateResult UpdatePaintOffsetTranslation(
      scoped_refptr<const TransformPaintPropertyNode> parent,
      TransformPaintPropertyNode::State&& state) {
    return Update(paint_offset_translation_, std::move(parent),
                  std::move(state));
  }
  UpdateResult UpdateTransform(
      scoped_refptr<const TransformPaintPropertyNode> parent,
      TransformPaintPropertyNode::State&& state) {
    return Update(transform_, std::move(parent), std::move(state));
  }
  UpdateResult UpdatePerspective(
      scoped_refptr<const TransformPaintPropertyNode> parent,
      TransformPaintPropertyNode::State&& state) {
    return Update(perspective_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateSvgLocalToBorderBoxTransform(
      scoped_refptr<const TransformPaintPropertyNode> parent,
      TransformPaintPropertyNode::State&& state) {
    DCHECK(!ScrollTranslation()) << "SVG elements cannot scroll so there "
                                    "should never be both a scroll translation "
                                    "and an SVG local to border box transform.";
    return Update(svg_local_to_border_box_transform_, std::move(parent),
                  std::move(state));
  }
  UpdateResult UpdateScroll(scoped_refptr<const ScrollPaintPropertyNode> parent,
                            ScrollPaintPropertyNode::State&& state) {
    return Update(scroll_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateScrollTranslation(
      scoped_refptr<const TransformPaintPropertyNode> parent,
      TransformPaintPropertyNode::State&& state) {
    DCHECK(!SvgLocalToBorderBoxTransform())
        << "SVG elements cannot scroll so there should never be both a scroll "
           "translation and an SVG local to border box transform.";
    return Update(scroll_translation_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateEffect(scoped_refptr<const EffectPaintPropertyNode> parent,
                            EffectPaintPropertyNode::State&& state) {
    return Update(effect_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateFilter(scoped_refptr<const EffectPaintPropertyNode> parent,
                            EffectPaintPropertyNode::State&& state) {
    return Update(filter_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateMask(scoped_refptr<const EffectPaintPropertyNode> parent,
                          EffectPaintPropertyNode::State&& state) {
    return Update(mask_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateClipPath(
      scoped_refptr<const EffectPaintPropertyNode> parent,
      EffectPaintPropertyNode::State&& state) {
    return Update(clip_path_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateFragmentClip(
      scoped_refptr<const ClipPaintPropertyNode> parent,
      ClipPaintPropertyNode::State&& state) {
    return Update(fragment_clip_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateClipPathClip(
      scoped_refptr<const ClipPaintPropertyNode> parent,
      ClipPaintPropertyNode::State&& state) {
    return Update(clip_path_clip_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateMaskClip(scoped_refptr<const ClipPaintPropertyNode> parent,
                              ClipPaintPropertyNode::State&& state) {
    return Update(mask_clip_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateCssClip(scoped_refptr<const ClipPaintPropertyNode> parent,
                             ClipPaintPropertyNode::State&& state) {
    return Update(css_clip_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateCssClipFixedPosition(
      scoped_refptr<const ClipPaintPropertyNode> parent,
      ClipPaintPropertyNode::State&& state) {
    return Update(css_clip_fixed_position_, std::move(parent),
                  std::move(state));
  }
  UpdateResult UpdateOverflowControlsClip(
      scoped_refptr<const ClipPaintPropertyNode> parent,
      ClipPaintPropertyNode::State&& state) {
    return Update(overflow_controls_clip_, std::move(parent), std::move(state));
  }
  UpdateResult UpdateInnerBorderRadiusClip(
      scoped_refptr<const ClipPaintPropertyNode> parent,
      ClipPaintPropertyNode::State&& state) {
    return Update(inner_border_radius_clip_, std::move(parent),
                  std::move(state));
  }
  UpdateResult UpdateOverflowClip(
      scoped_refptr<const ClipPaintPropertyNode> parent,
      ClipPaintPropertyNode::State&& state) {
    return Update(overflow_clip_, std::move(parent), std::move(state));
  }

#if DCHECK_IS_ON()
  // Used by FindPropertiesNeedingUpdate.h for recording the current properties.
  std::unique_ptr<ObjectPaintProperties> Clone() const {
    std::unique_ptr<ObjectPaintProperties> cloned = Create();
    if (paint_offset_translation_)
      cloned->paint_offset_translation_ = paint_offset_translation_->Clone();
    if (transform_)
      cloned->transform_ = transform_->Clone();
    if (effect_)
      cloned->effect_ = effect_->Clone();
    if (filter_)
      cloned->filter_ = filter_->Clone();
    if (mask_)
      cloned->mask_ = mask_->Clone();
    if (clip_path_)
      cloned->clip_path_ = clip_path_->Clone();
    if (fragment_clip_)
      cloned->fragment_clip_ = fragment_clip_->Clone();
    if (clip_path_clip_)
      cloned->clip_path_clip_ = clip_path_clip_->Clone();
    if (mask_clip_)
      cloned->mask_clip_ = mask_clip_->Clone();
    if (css_clip_)
      cloned->css_clip_ = css_clip_->Clone();
    if (css_clip_fixed_position_)
      cloned->css_clip_fixed_position_ = css_clip_fixed_position_->Clone();
    if (overflow_controls_clip_)
      cloned->overflow_controls_clip_ = overflow_controls_clip_->Clone();
    if (inner_border_radius_clip_)
      cloned->inner_border_radius_clip_ = inner_border_radius_clip_->Clone();
    if (overflow_clip_)
      cloned->overflow_clip_ = overflow_clip_->Clone();
    if (perspective_)
      cloned->perspective_ = perspective_->Clone();
    if (svg_local_to_border_box_transform_) {
      cloned->svg_local_to_border_box_transform_ =
          svg_local_to_border_box_transform_->Clone();
    }
    if (scroll_)
      cloned->scroll_ = scroll_->Clone();
    if (scroll_translation_)
      cloned->scroll_translation_ = scroll_translation_->Clone();
    return cloned;
  }
#endif

 private:
  ObjectPaintProperties() = default;

  // Return true if the property tree structure changes (an existing node was
  // deleted), and false otherwise. See the class-level comment ("update & clear
  // implementation note") for details about why this is needed for efficiency.
  template <typename PaintPropertyNode>
  bool Clear(scoped_refptr<PaintPropertyNode>& field) {
    if (field) {
      field = nullptr;
      return true;
    }
    return false;
  }

  // Return true if the property tree structure changes (a new node was
  // created), and false otherwise. See the class-level comment ("update & clear
  // implementation note") for details about why this is needed for efficiency.
  template <typename PaintPropertyNode>
  UpdateResult Update(scoped_refptr<PaintPropertyNode>& field,
                      scoped_refptr<const PaintPropertyNode> parent,
                      typename PaintPropertyNode::State&& state) {
    if (field) {
      return field->Update(std::move(parent), std::move(state))
                 ? UpdateResult::kValueChanged
                 : UpdateResult::kUnchanged;
    }
    field = PaintPropertyNode::Create(std::move(parent), std::move(state));
    return UpdateResult::kNewNodeCreated;
  }

  // ATTENTION! Make sure to keep FindPropertiesNeedingUpdate.h in sync when
  // new properites are added!
  scoped_refptr<TransformPaintPropertyNode> paint_offset_translation_;
  scoped_refptr<TransformPaintPropertyNode> transform_;
  scoped_refptr<EffectPaintPropertyNode> effect_;
  scoped_refptr<EffectPaintPropertyNode> filter_;
  scoped_refptr<EffectPaintPropertyNode> mask_;
  scoped_refptr<EffectPaintPropertyNode> clip_path_;
  scoped_refptr<ClipPaintPropertyNode> fragment_clip_;
  scoped_refptr<ClipPaintPropertyNode> clip_path_clip_;
  scoped_refptr<ClipPaintPropertyNode> mask_clip_;
  scoped_refptr<ClipPaintPropertyNode> css_clip_;
  scoped_refptr<ClipPaintPropertyNode> css_clip_fixed_position_;
  scoped_refptr<ClipPaintPropertyNode> overflow_controls_clip_;
  scoped_refptr<ClipPaintPropertyNode> inner_border_radius_clip_;
  scoped_refptr<ClipPaintPropertyNode> overflow_clip_;
  scoped_refptr<TransformPaintPropertyNode> perspective_;
  // TODO(pdr): Only LayoutSVGRoot needs this and it should be moved there.
  scoped_refptr<TransformPaintPropertyNode> svg_local_to_border_box_transform_;
  scoped_refptr<ScrollPaintPropertyNode> scroll_;
  scoped_refptr<TransformPaintPropertyNode> scroll_translation_;

  DISALLOW_COPY_AND_ASSIGN(ObjectPaintProperties);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_OBJECT_PAINT_PROPERTIES_H_
