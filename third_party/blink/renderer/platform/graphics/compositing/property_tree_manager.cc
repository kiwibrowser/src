// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/compositing/property_tree_manager.h"

#include "cc/layers/layer.h"
#include "cc/trees/clip_node.h"
#include "cc/trees/effect_node.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/property_tree.h"
#include "cc/trees/scroll_node.h"
#include "cc/trees/transform_node.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_paint_property_node.h"
#include "third_party/blink/renderer/platform/graphics/paint/effect_paint_property_node.h"
#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper.h"
#include "third_party/blink/renderer/platform/graphics/paint/scroll_paint_property_node.h"
#include "third_party/blink/renderer/platform/graphics/paint/transform_paint_property_node.h"
#include "third_party/skia/include/effects/SkColorFilterImageFilter.h"
#include "third_party/skia/include/effects/SkLumaColorFilter.h"

namespace blink {

namespace {

static constexpr int kInvalidNodeId = -1;
// cc's property trees use 0 for the root node (always non-null).
static constexpr int kRealRootNodeId = 0;
// cc allocates special nodes for root effects such as the device scale.
static constexpr int kSecondaryRootNodeId = 1;

}  // namespace

PropertyTreeManager::PropertyTreeManager(PropertyTreeManagerClient& client,
                                         cc::PropertyTrees& property_trees,
                                         cc::Layer* root_layer,
                                         int sequence_number)
    : client_(client),
      property_trees_(property_trees),
      root_layer_(root_layer),
      sequence_number_(sequence_number) {
  SetupRootTransformNode();
  SetupRootClipNode();
  SetupRootEffectNode();
  SetupRootScrollNode();
}

void PropertyTreeManager::Finalize() {
  while (effect_stack_.size())
    CloseCcEffect();
}

cc::TransformTree& PropertyTreeManager::GetTransformTree() {
  return property_trees_.transform_tree;
}

cc::ClipTree& PropertyTreeManager::GetClipTree() {
  return property_trees_.clip_tree;
}

cc::EffectTree& PropertyTreeManager::GetEffectTree() {
  return property_trees_.effect_tree;
}

cc::ScrollTree& PropertyTreeManager::GetScrollTree() {
  return property_trees_.scroll_tree;
}

void PropertyTreeManager::SetupRootTransformNode() {
  // cc is hardcoded to use transform node index 1 for device scale and
  // transform.
  cc::TransformTree& transform_tree = property_trees_.transform_tree;
  transform_tree.clear();
  property_trees_.element_id_to_transform_node_index.clear();
  cc::TransformNode& transform_node = *transform_tree.Node(
      transform_tree.Insert(cc::TransformNode(), kRealRootNodeId));
  DCHECK_EQ(transform_node.id, kSecondaryRootNodeId);
  transform_node.source_node_id = transform_node.parent_id;

  // TODO(jaydasika): We shouldn't set ToScreen and FromScreen of root
  // transform node here. They should be set while updating transform tree in
  // cc.
  float device_scale_factor =
      root_layer_->layer_tree_host()->device_scale_factor();
  gfx::Transform to_screen;
  to_screen.Scale(device_scale_factor, device_scale_factor);
  transform_tree.SetToScreen(kRealRootNodeId, to_screen);
  gfx::Transform from_screen;
  bool invertible = to_screen.GetInverse(&from_screen);
  DCHECK(invertible);
  transform_tree.SetFromScreen(kRealRootNodeId, from_screen);
  transform_tree.set_needs_update(true);

  transform_node_map_.Set(TransformPaintPropertyNode::Root(),
                          transform_node.id);
  root_layer_->SetTransformTreeIndex(transform_node.id);
}

void PropertyTreeManager::SetupRootClipNode() {
  // cc is hardcoded to use clip node index 1 for viewport clip.
  cc::ClipTree& clip_tree = property_trees_.clip_tree;
  clip_tree.clear();
  cc::ClipNode& clip_node =
      *clip_tree.Node(clip_tree.Insert(cc::ClipNode(), kRealRootNodeId));
  DCHECK_EQ(clip_node.id, kSecondaryRootNodeId);

  clip_node.clip_type = cc::ClipNode::ClipType::APPLIES_LOCAL_CLIP;
  clip_node.clip = gfx::RectF(
      gfx::SizeF(root_layer_->layer_tree_host()->device_viewport_size()));
  clip_node.transform_id = kRealRootNodeId;

  clip_node_map_.Set(ClipPaintPropertyNode::Root(), clip_node.id);
  root_layer_->SetClipTreeIndex(clip_node.id);
}

void PropertyTreeManager::SetupRootEffectNode() {
  // cc is hardcoded to use effect node index 1 for root render surface.
  cc::EffectTree& effect_tree = property_trees_.effect_tree;
  effect_tree.clear();
  property_trees_.element_id_to_effect_node_index.clear();
  cc::EffectNode& effect_node =
      *effect_tree.Node(effect_tree.Insert(cc::EffectNode(), kInvalidNodeId));
  DCHECK_EQ(effect_node.id, kSecondaryRootNodeId);

  static UniqueObjectId unique_id = NewUniqueObjectId();

  effect_node.stable_id =
      CompositorElementIdFromUniqueObjectId(unique_id).ToInternalValue();
  effect_node.transform_id = kRealRootNodeId;
  effect_node.clip_id = kSecondaryRootNodeId;
  effect_node.has_render_surface = true;
  root_layer_->SetEffectTreeIndex(effect_node.id);

  current_effect_id_ = effect_node.id;
  current_effect_type_ = CcEffectType::kEffect;
  current_effect_ = EffectPaintPropertyNode::Root();
  current_clip_ = current_effect_->OutputClip();
}

void PropertyTreeManager::SetupRootScrollNode() {
  cc::ScrollTree& scroll_tree = property_trees_.scroll_tree;
  scroll_tree.clear();
  property_trees_.element_id_to_scroll_node_index.clear();
  cc::ScrollNode& scroll_node =
      *scroll_tree.Node(scroll_tree.Insert(cc::ScrollNode(), kRealRootNodeId));
  DCHECK_EQ(scroll_node.id, kSecondaryRootNodeId);
  scroll_node.transform_id = kSecondaryRootNodeId;

  scroll_node_map_.Set(ScrollPaintPropertyNode::Root(), scroll_node.id);
  root_layer_->SetScrollTreeIndex(scroll_node.id);
}

int PropertyTreeManager::EnsureCompositorTransformNode(
    const TransformPaintPropertyNode* transform_node) {
  DCHECK(transform_node);
  // TODO(crbug.com/645615): Remove the failsafe here.
  if (!transform_node)
    return kSecondaryRootNodeId;

  auto it = transform_node_map_.find(transform_node);
  if (it != transform_node_map_.end())
    return it->value;

  int parent_id = EnsureCompositorTransformNode(transform_node->Parent());
  int id = GetTransformTree().Insert(cc::TransformNode(), parent_id);

  cc::TransformNode& compositor_node = *GetTransformTree().Node(id);
  compositor_node.source_node_id = parent_id;

  FloatPoint3D origin = transform_node->Origin();
  compositor_node.pre_local.matrix().setTranslate(-origin.X(), -origin.Y(),
                                                  -origin.Z());
  compositor_node.local.matrix() =
      TransformationMatrix::ToSkMatrix44(transform_node->Matrix());
  compositor_node.post_local.matrix().setTranslate(origin.X(), origin.Y(),
                                                   origin.Z());
  compositor_node.needs_local_transform_update = true;
  compositor_node.flattens_inherited_transform =
      transform_node->FlattensInheritedTransform();
  compositor_node.sorting_context_id = transform_node->RenderingContextId();

  CompositorElementId compositor_element_id =
      transform_node->GetCompositorElementId();
  if (compositor_element_id) {
    property_trees_.element_id_to_transform_node_index[compositor_element_id] =
        id;
  }

  // If this transform is a scroll offset translation, create the associated
  // compositor scroll property node and adjust the compositor transform node's
  // scroll offset.
  if (auto* scroll_node = transform_node->ScrollNode()) {
    // Blink creates a 2d transform node just for scroll offset whereas cc's
    // transform node has a special scroll offset field. To handle this we
    // adjust cc's transform node to remove the 2d scroll translation and
    // instead set the scroll_offset field.
    auto scroll_offset_size = transform_node->Matrix().To2DTranslation();
    auto scroll_offset = gfx::ScrollOffset(-scroll_offset_size.Width(),
                                           -scroll_offset_size.Height());
    DCHECK(compositor_node.local.IsIdentityOr2DTranslation());
    compositor_node.scroll_offset = scroll_offset;
    compositor_node.local.MakeIdentity();
    compositor_node.scrolls = true;

    CreateCompositorScrollNode(scroll_node, compositor_node);
  }

  auto result = transform_node_map_.Set(transform_node, id);
  DCHECK(result.is_new_entry);
  GetTransformTree().set_needs_update(true);

  return id;
}

int PropertyTreeManager::EnsureCompositorClipNode(
    const ClipPaintPropertyNode* clip_node) {
  DCHECK(clip_node);
  // TODO(crbug.com/645615): Remove the failsafe here.
  if (!clip_node)
    return kSecondaryRootNodeId;

  auto it = clip_node_map_.find(clip_node);
  if (it != clip_node_map_.end())
    return it->value;

  int parent_id = EnsureCompositorClipNode(clip_node->Parent());
  int id = GetClipTree().Insert(cc::ClipNode(), parent_id);

  cc::ClipNode& compositor_node = *GetClipTree().Node(id);

  compositor_node.clip = clip_node->ClipRect().Rect();
  compositor_node.transform_id =
      EnsureCompositorTransformNode(clip_node->LocalTransformSpace());
  compositor_node.clip_type = cc::ClipNode::ClipType::APPLIES_LOCAL_CLIP;

  auto result = clip_node_map_.Set(clip_node, id);
  DCHECK(result.is_new_entry);
  GetClipTree().set_needs_update(true);
  return id;
}

void PropertyTreeManager::CreateCompositorScrollNode(
    const ScrollPaintPropertyNode* scroll_node,
    const cc::TransformNode& scroll_offset_translation) {
  DCHECK(!scroll_node_map_.Contains(scroll_node));

  auto parent_it = scroll_node_map_.find(scroll_node->Parent());
  // Compositor transform nodes up to scroll_offset_translation must exist.
  // Scrolling uses the transform tree for scroll offsets so this means all
  // ancestor scroll nodes must also exist.
  DCHECK(parent_it != scroll_node_map_.end());
  int parent_id = parent_it->value;
  int id = GetScrollTree().Insert(cc::ScrollNode(), parent_id);

  cc::ScrollNode& compositor_node = *GetScrollTree().Node(id);
  compositor_node.scrollable = true;

  compositor_node.container_bounds =
      static_cast<gfx::Size>(scroll_node->ContainerRect().Size());
  compositor_node.bounds =
      static_cast<gfx::Size>(scroll_node->ContentsRect().Size());
  compositor_node.user_scrollable_horizontal =
      scroll_node->UserScrollableHorizontal();
  compositor_node.user_scrollable_vertical =
      scroll_node->UserScrollableVertical();
  compositor_node.main_thread_scrolling_reasons =
      scroll_node->GetMainThreadScrollingReasons();

  auto compositor_element_id = scroll_node->GetCompositorElementId();
  if (compositor_element_id) {
    compositor_node.element_id = compositor_element_id;
    property_trees_.element_id_to_scroll_node_index[compositor_element_id] = id;
  }

  compositor_node.transform_id = scroll_offset_translation.id;

  // TODO(pdr): Set the scroll node's non_fast_scrolling_region value.

  auto result = scroll_node_map_.Set(scroll_node, id);
  DCHECK(result.is_new_entry);

  GetScrollTree().SetScrollOffset(compositor_element_id,
                                  scroll_offset_translation.scroll_offset);
  GetScrollTree().set_needs_update(true);
}

int PropertyTreeManager::EnsureCompositorScrollNode(
    const TransformPaintPropertyNode* scroll_offset_translation) {
  const auto* scroll_node = scroll_offset_translation->ScrollNode();
  DCHECK(scroll_node);
  EnsureCompositorTransformNode(scroll_offset_translation);
  auto it = scroll_node_map_.find(scroll_node);
  DCHECK(it != scroll_node_map_.end());
  return it->value;
}

void PropertyTreeManager::EmitClipMaskLayer() {
  int clip_id = EnsureCompositorClipNode(current_clip_);
  CompositorElementId mask_isolation_id, mask_effect_id;
  cc::Layer* mask_layer = client_.CreateOrReuseSynthesizedClipLayer(
      current_clip_, mask_isolation_id, mask_effect_id);

  cc::EffectNode& mask_isolation = *GetEffectTree().Node(current_effect_id_);
  // Assignment of mask_isolation.stable_id was delayed until now.
  // See PropertyTreeManager::SynthesizeCcEffectsForClipsIfNeeded().
  DCHECK_EQ(static_cast<uint64_t>(cc::EffectNode::INVALID_STABLE_ID),
            mask_isolation.stable_id);
  mask_isolation.stable_id = mask_isolation_id.ToInternalValue();

  cc::EffectNode& mask_effect = *GetEffectTree().Node(
      GetEffectTree().Insert(cc::EffectNode(), current_effect_id_));
  mask_effect.stable_id = mask_effect_id.ToInternalValue();
  mask_effect.clip_id = clip_id;
  mask_effect.has_render_surface = true;
  mask_effect.blend_mode = SkBlendMode::kDstIn;

  const TransformPaintPropertyNode* clip_space =
      current_clip_->LocalTransformSpace();
  root_layer_->AddChild(mask_layer);
  mask_layer->set_property_tree_sequence_number(sequence_number_);
  mask_layer->SetTransformTreeIndex(EnsureCompositorTransformNode(clip_space));
  // TODO(pdr): This could be a performance issue because it crawls up the
  // transform tree for each pending layer. If this is on profiles, we should
  // cache a lookup of transform node to scroll translation transform node.
  int scroll_id =
      EnsureCompositorScrollNode(&clip_space->NearestScrollTranslationNode());
  mask_layer->SetScrollTreeIndex(scroll_id);
  mask_layer->SetClipTreeIndex(clip_id);
  mask_layer->SetEffectTreeIndex(mask_effect.id);
}

void PropertyTreeManager::CloseCcEffect() {
  DCHECK(effect_stack_.size());
  const EffectStackEntry& previous_state = effect_stack_.back();

  // An effect with exotic blending that is masked by a synthesized clip must
  // have its blending to the outermost synthesized clip. It is because
  // blending needs access to the backdrop of the enclosing effect. With
  // the isolation for a synthesized clip, a blank backdrop will be seen.
  // Therefore the blending is delegated to the outermost synthesized clip,
  // thus the clip can't be shared with sibling layers, and must be closed now.
  bool clear_synthetic_effects =
      !IsCurrentCcEffectSynthetic() &&
      current_effect_->BlendMode() != SkBlendMode::kSrcOver;

  // We are about to close an effect that was synthesized for isolating
  // a clip mask. Now emit the actual clip mask that will be composited on
  // top of masked contents with SkBlendMode::kDstIn.
  if (IsCurrentCcEffectSynthetic())
    EmitClipMaskLayer();

  current_effect_id_ = previous_state.effect_id;
  current_effect_type_ = previous_state.effect_type;
  current_effect_ = previous_state.effect;
  current_clip_ = previous_state.clip;
  effect_stack_.pop_back();

  if (clear_synthetic_effects) {
    while (IsCurrentCcEffectSynthetic())
      CloseCcEffect();
  }
}

int PropertyTreeManager::SwitchToEffectNodeWithSynthesizedClip(
    const EffectPaintPropertyNode& next_effect,
    const ClipPaintPropertyNode& next_clip) {
  // This function is expected to be invoked right before emitting each layer.
  // It keeps track of the nesting of clip and effects, output a composited
  // effect node whenever an effect is entered, or a non-trivial clip is
  // entered. In the latter case, the generated composited effect node is
  // called a "synthetic effect", and the corresponding clip a "synthesized
  // clip". Upon exiting a synthesized clip, a mask layer will be appended,
  // which will be kDstIn blended on top of contents enclosed by the synthetic
  // effect, i.e. applying the clip as a mask.
  //
  // For example with the following clip and effect tree and pending layers:
  // E0 <-- E1
  // C0 <-- C1(rounded)
  // [P0(E1,C0), P1(E1,C1), P2(E0, C1)]
  // In effect stack diagram:
  // P0(C0) P1(C1)
  // [    E1     ] P2(C1)
  // [        E0        ]
  //
  // The following cc property trees and layers will be generated:
  // E0 <+- E1 <-- E_C1_1 <-- E_C1_1M
  //     +- E_C1_2 <-- E_C1_2M
  // C0 <-- C1
  // [L0(E1,C0), L1(E_C1_1, C1), L_C1_1(E_C1_1M, C1), L2(E0, C1),
  //  L_C1_2(E_C1_2M, C1)]
  // In effect stack diagram:
  //                 L_C1_1
  //        L1(C1) [ E_C1_1M ]          L_C2_2
  // L0(C0) [     E_C1_1     ] L2(C1) [ E_C1_2M ]
  // [          E1           ][     E_C1_2      ]
  // [                    E0                    ]
  //
  // As the caller iterates the layer list, the sequence of events happen in
  // the following order:
  // Prior to emitting P0, this method is invoked with (E1, C0). A compositor
  // effect node for E1 is generated as we are entering it. The caller emits P0.
  // Prior to emitting P1, this method is invoked with (E1, C1). A synthetic
  // compositor effect for C1 is generated as we are entering it. The caller
  // emits P1.
  // Prior to emitting P2, this method is invoked with (E0, C1). Both previously
  // entered effects must be closed, because synthetic effect for C1 is enclosed
  // by E1, thus must be closed before E1 can be closed. A mask layer L_C1_1
  // is generated along with an internal effect node for blending. After closing
  // both effects, C1 has to be entered again, thus generates another synthetic
  // compositor effect. The caller emits P2.
  // At last, the caller invokes Finalize() to close the unclosed synthetic
  // effect. Another mask layer L_C1_2 is generated, along with its internal
  // effect node for blending.
  const auto& ancestor = LowestCommonAncestor(*current_effect_, next_effect);
  while (current_effect_ != &ancestor)
    CloseCcEffect();

  bool newly_built = BuildEffectNodesRecursively(&next_effect);
  SynthesizeCcEffectsForClipsIfNeeded(&next_clip, SkBlendMode::kSrcOver,
                                      newly_built);

  return current_effect_id_;
}

static bool IsNodeOnAncestorChain(const ClipPaintPropertyNode& find,
                                  const ClipPaintPropertyNode& current,
                                  const ClipPaintPropertyNode& ancestor) {
  // Precondition: |ancestor| must be an (inclusive) ancestor of |current|
  // otherwise the behavior is undefined.
  // Returns true if node |find| is one of the node on the ancestor chain
  // [current, ancestor). Returns false otherwise.
  DCHECK(ancestor.IsAncestorOf(current));

  for (const auto* node = &current; node != &ancestor; node = node->Parent()) {
    if (node == &find)
      return true;
  }
  return false;
}

SkBlendMode PropertyTreeManager::SynthesizeCcEffectsForClipsIfNeeded(
    const ClipPaintPropertyNode* target_clip,
    SkBlendMode delegated_blend,
    bool effect_is_newly_built) {
  if (delegated_blend != SkBlendMode::kSrcOver) {
    // Exit all synthetic effect node for rounded clip if the next child has
    // exotic blending mode because it has to access the backdrop of enclosing
    // effect.
    while (IsCurrentCcEffectSynthetic())
      CloseCcEffect();

    // An effect node can't omit render surface if it has child with exotic
    // blending mode. See comments below for more detail.
    // TODO(crbug.com/504464): Remove premature optimization here.
    GetEffectTree().Node(current_effect_id_)->has_render_surface = true;
  } else {
    // Exit synthetic effects until there are no more synthesized clips below
    // our lowest common ancestor.
    const auto& lca = LowestCommonAncestor(*current_clip_, *target_clip);
    while (current_clip_ != &lca) {
      DCHECK(IsCurrentCcEffectSynthetic());
      const auto* pre_exit_clip = current_clip_;
      CloseCcEffect();
      // We may run past the lowest common ancestor because it may not have
      // been synthesized.
      if (IsNodeOnAncestorChain(lca, *pre_exit_clip, *current_clip_))
        break;
    }

    // If the effect is an existing node, i.e. already has at least one paint
    // chunk or child effect, and by reaching here it implies we are going to
    // attach either another paint chunk or child effect to it. We can no longer
    // omit render surface for it even for opacity-only node.
    // See comments in PropertyTreeManager::BuildEffectNodesRecursively().
    // TODO(crbug.com/504464): Remove premature optimization here.
    if (!effect_is_newly_built && !IsCurrentCcEffectSynthetic() &&
        current_effect_->Opacity() != 1.f)
      GetEffectTree().Node(current_effect_id_)->has_render_surface = true;
  }

  DCHECK(current_clip_->IsAncestorOf(*target_clip));

  Vector<const ClipPaintPropertyNode*> pending_clips;
  for (; target_clip != current_clip_; target_clip = target_clip->Parent()) {
    DCHECK(target_clip);
    bool should_synthesize =
        target_clip->ClipRect().IsRounded() || target_clip->ClipPath();
    if (should_synthesize)
      pending_clips.push_back(target_clip);
  }

  for (size_t i = pending_clips.size(); i--;) {
    const ClipPaintPropertyNode* next_clip = pending_clips[i];

    // For each of clip synthesized, an isolation effect node needs to be
    // created to enclose only the layers that should be masked by the clip.
    cc::EffectNode& mask_isolation = *GetEffectTree().Node(
        GetEffectTree().Insert(cc::EffectNode(), current_effect_id_));
    // mask_isolation.stable_id will be assigned later when the effect is
    // closed. For now the default value of INVALID_STABLE_ID is used.
    // See PropertyTreeManager::EmitClipMaskLayer().
    mask_isolation.clip_id = EnsureCompositorClipNode(next_clip);
    mask_isolation.has_render_surface = true;
    // Clip and kDstIn do not commute. This shall never be reached because
    // kDstIn is only used internally to implement CSS clip-path and mask,
    // and there is never a difference between the output clip of the effect
    // and the mask content.
    DCHECK(delegated_blend != SkBlendMode::kDstIn);
    mask_isolation.blend_mode = delegated_blend;
    delegated_blend = SkBlendMode::kSrcOver;

    effect_stack_.emplace_back(
        EffectStackEntry{current_effect_id_, current_effect_type_,
                         current_effect_, current_clip_});
    current_effect_id_ = mask_isolation.id;
    current_effect_type_ = CcEffectType::kSynthesizedClip;
    current_clip_ = next_clip;
  }

  return delegated_blend;
}

bool PropertyTreeManager::BuildEffectNodesRecursively(
    const EffectPaintPropertyNode* next_effect) {
  if (next_effect == current_effect_)
    return false;
  DCHECK(next_effect);

  bool newly_built = BuildEffectNodesRecursively(next_effect->Parent());
  DCHECK_EQ(next_effect->Parent(), current_effect_);

#if DCHECK_IS_ON()
  DCHECK(!effect_nodes_converted_.Contains(next_effect))
      << "Malformed paint artifact. Paint chunks under the same effect should "
         "be contiguous.";
  effect_nodes_converted_.insert(next_effect);
#endif

  SkBlendMode used_blend_mode;
  int output_clip_id;
  if (next_effect->OutputClip()) {
    used_blend_mode = SynthesizeCcEffectsForClipsIfNeeded(
        next_effect->OutputClip(), next_effect->BlendMode(), newly_built);
    output_clip_id = EnsureCompositorClipNode(next_effect->OutputClip());
  } else {
    while (IsCurrentCcEffectSynthetic())
      CloseCcEffect();
    // An effect node can't omit render surface if it has child with exotic
    // blending mode, nor being opacity-only node with more than one child.
    // TODO(crbug.com/504464): Remove premature optimization here.
    if (next_effect->BlendMode() != SkBlendMode::kSrcOver ||
        (!newly_built && current_effect_->Opacity() != 1.f))
      GetEffectTree().Node(current_effect_id_)->has_render_surface = true;

    used_blend_mode = next_effect->BlendMode();
    output_clip_id = GetEffectTree().Node(current_effect_id_)->clip_id;
  }

  cc::EffectNode& effect_node = *GetEffectTree().Node(
      GetEffectTree().Insert(cc::EffectNode(), current_effect_id_));
  effect_node.stable_id =
      next_effect->GetCompositorElementId().ToInternalValue();
  effect_node.clip_id = output_clip_id;
  // Every effect is supposed to have render surface enabled for grouping,
  // but we can get away without one if the effect is opacity-only and has only
  // one compositing child with kSrcOver blend mode. This is both for
  // optimization and not introducing sub-pixel differences in layout tests.
  // See PropertyTreeManager::switchToEffectNode() and above where we
  // retrospectively enable render surface when more than one compositing child
  // or a child with exotic blend mode is detected.
  // TODO(crbug.com/504464): There is ongoing work in cc to delay render surface
  // decision until later phase of the pipeline. Remove premature optimization
  // here once the work is ready.
  if (!next_effect->Filter().IsEmpty() ||
      used_blend_mode != SkBlendMode::kSrcOver)
    effect_node.has_render_surface = true;
  effect_node.opacity = next_effect->Opacity();
  if (next_effect->GetColorFilter() != kColorFilterNone) {
    // Currently color filter is only used by SVG masks.
    // We are cutting corner here by support only specific configuration.
    DCHECK(next_effect->GetColorFilter() == kColorFilterLuminanceToAlpha);
    DCHECK(used_blend_mode == SkBlendMode::kDstIn);
    DCHECK(next_effect->Filter().IsEmpty());
    effect_node.filters.Append(cc::FilterOperation::CreateReferenceFilter(
        sk_make_sp<ColorFilterPaintFilter>(SkLumaColorFilter::Make(),
                                           nullptr)));
  } else {
    effect_node.filters = next_effect->Filter().AsCcFilterOperations();
    effect_node.filters_origin = next_effect->PaintOffset();
    effect_node.transform_id =
        EnsureCompositorTransformNode(next_effect->LocalTransformSpace());
  }
  effect_node.blend_mode = used_blend_mode;
  CompositorElementId compositor_element_id =
      next_effect->GetCompositorElementId();
  if (compositor_element_id) {
    DCHECK(property_trees_.element_id_to_effect_node_index.find(
               compositor_element_id) ==
           property_trees_.element_id_to_effect_node_index.end());
    property_trees_.element_id_to_effect_node_index[compositor_element_id] =
        effect_node.id;
  }
  effect_stack_.emplace_back(EffectStackEntry{current_effect_id_,
                                              current_effect_type_,
                                              current_effect_, current_clip_});
  current_effect_id_ = effect_node.id;
  current_effect_type_ = CcEffectType::kEffect;
  current_effect_ = next_effect;
  if (next_effect->OutputClip())
    current_clip_ = next_effect->OutputClip();

  return true;
}

}  // namespace blink
