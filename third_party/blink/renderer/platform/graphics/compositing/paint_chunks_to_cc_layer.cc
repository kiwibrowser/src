// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/compositing/paint_chunks_to_cc_layer.h"

#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/paint/render_surface_filters.h"
#include "third_party/blink/renderer/platform/graphics/compositing/chunk_to_layer_mapper.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item_list.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_chunk.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_chunk_subset.h"
#include "third_party/blink/renderer/platform/graphics/paint/property_tree_state.h"
#include "third_party/blink/renderer/platform/graphics/paint/raster_invalidation_tracking.h"

namespace blink {

namespace {

class ConversionContext {
 public:
  ConversionContext(const PropertyTreeState& layer_state,
                    const gfx::Vector2dF& layer_offset,
                    const FloatSize& visual_rect_subpixel_offset,
                    cc::DisplayItemList& cc_list)
      : layer_state_(layer_state),
        layer_offset_(layer_offset),
        current_transform_(layer_state.Transform()),
        current_clip_(layer_state.Clip()),
        current_effect_(layer_state.Effect()),
        chunk_to_layer_mapper_(layer_state_,
                               layer_offset_,
                               visual_rect_subpixel_offset),
        cc_list_(cc_list) {}
  ~ConversionContext();

  // The main function of this class. It converts a list of paint chunks into
  // non-pair display items, and paint properties associated with them are
  // implemented by paired display items.
  // This is done by closing and opening paired items to adjust the current
  // property state to the chunk's state when each chunk is consumed.
  // Note that the clip/effect state is "lazy" in the sense that it stays
  // in whatever state the last chunk left with, and only adjusted when
  // a new chunk is consumed. The class implemented a few helpers to manage
  // state switching so that paired display items are nested properly.
  //
  // State management example (transform tree omitted).
  // Corresponds to unit test PaintChunksToCcLayerTest.InterleavedClipEffect:
  //   Clip tree: C0 <-- C1 <-- C2 <-- C3 <-- C4
  //   Effect tree: E0(clip=C0) <-- E1(clip=C2) <-- E2(clip=C4)
  //   Layer state: C0, E0
  //   Paint chunks: P0(C3, E0), P1(C4, E2), P2(C3, E1), P3(C4, E0)
  // Initialization:
  //   The current state is initalized with the layer state, and starts with
  //   an empty state stack.
  //   current_clip = C0
  //   current_effect = E0
  //   state_stack = []
  // When P0 is consumed, C1, C2 and C3 need to be applied to the state:
  //   Output: Begin_C1 Begin_C2 Begin_C3 Draw_P0
  //   current_clip = C3
  //   state_stack = [C0, C1, C2]
  // When P1 is consumed, C3 needs to be closed before E1 can be entered,
  // then C3 and C4 need to be entered before E2 can be entered:
  //   Output: End_C3 Begin_E1 Begin_C3 Begin_C4 Begin_E2 Draw_P1
  //   current_clip = C4
  //   current_effect = E2
  //   state_stack = [C0, C1, E0, C2, C3, E1]
  // When P2 is consumed, E2 then C4 need to be exited:
  //   Output: End_E2 End_C4 Draw_P2
  //   current_clip = C3
  //   current_effect = E1
  //   state_stack = [C0, C1, E0, C2]
  // When P3 is consumed, C3 must exit before E1 can be exited, then we can
  // enter C3 and C4:
  //   Output: End_C3 End_E1 Enter_C3 Enter_C4 Draw_P3
  //   current_clip = C4
  //   current_effect = E0
  //   state_stack = [C0, C1, C2, C3]
  // At last, close all pushed states to balance pairs (this happens when the
  // context object is destructed):
  //   Output: End_C4 End_C3 End_C2 End_C1
  void Convert(const PaintChunkSubset&, const DisplayItemList&);

 private:
  // Adjust the translation of the whole display list relative to layer offset.
  // It's only called if we actually paint anything.
  void TranslateForLayerOffsetOnce();

  // Switch the current property tree state to the chunk's state. It's only
  // called if we actually paint anything, and should execute for a chunk
  // only once.
  void SwitchToChunkState(const PaintChunk&);

  // Switch the current clip to the target state, staying in the same effect.
  // It is no-op if the context is already in the target state.
  // Otherwise zero or more clips will be popped from or pushed onto the
  // current state stack.
  // INPUT:
  // The target clip must be a descendant of the input clip of current effect.
  // OUTPUT:
  // The current transform may be changed.
  // The current clip will change to the target clip.
  // The current effect will not change.
  void SwitchToClip(const ClipPaintPropertyNode*);

  // Switch the current effect to the target state.
  // It is no-op if the context is already in the target state.
  // Otherwise zero or more effect effects will be popped from or pushed onto
  // the state stack. As effects getting popped from the stack, clips applied
  // on top of them will be popped as well. Also clips will be pushed at
  // appropriate steps to apply output clip to newly pushed effects.
  // INPUT:
  // The target effect must be a descendant of the layer's effect.
  // OUTPUT:
  // The current transform may be changed.
  // The current clip may be changed, and is guaranteed to be a descendant of
  // the output clip of the target effect.
  // The current effect will change to the target effect.
  void SwitchToEffect(const EffectPaintPropertyNode*);

  // Switch the current transform to the target state.
  void SwitchToTransform(const TransformPaintPropertyNode*);
  // End the transform state that is estalished by SwitchToTransform().
  // Called when the next chunk has different property tree state and when we
  // have processed all chunks.
  void EndTransform();

  // Applies combined transform from |current_transform_| to |target_transform|
  // This function doesn't change |current_transform_|.
  void ApplyTransform(const TransformPaintPropertyNode* target_transform) {
    if (target_transform == current_transform_)
      return;
    auto sk_matrix = GetSkMatrix(target_transform);
    if (!sk_matrix.isIdentity())
      cc_list_.push<cc::ConcatOp>(sk_matrix);
  }

  SkMatrix GetSkMatrix(
      const TransformPaintPropertyNode* target_transform) const {
    return SkMatrix(TransformationMatrix::ToSkMatrix44(
        GeometryMapper::SourceToDestinationProjection(target_transform,
                                                      current_transform_)));
  }

  void AppendRestore(size_t n) {
    cc_list_.StartPaint();
    while (n--)
      cc_list_.push<cc::RestoreOp>();
    cc_list_.EndPaintOfPairedEnd();
  }

  // Starts an effect state by adjusting clip and transform state, applying
  // the effect as a SaveLayer[Alpha]Op (whose bounds will be updated in
  // EndEffect()), and updating the current state.
  void StartEffect(const EffectPaintPropertyNode*);
  // Ends the effect on the top of the state stack if the stack is not empty,
  // and update the bounds of the SaveLayer[Alpha]Op of the effect.
  void EndEffect();
  void UpdateEffectBounds(const FloatRect&, const TransformPaintPropertyNode*);

  // Starts a clip state by adjusting the transform state, applying
  // |combined_clip_rect| which is combined from one or more consecutive clips,
  // and updating the current state. |lowest_combined_clip_node| is the lowest
  // node of the combined clips.
  void StartClip(const FloatRoundedRect& combined_clip_rect,
                 const ClipPaintPropertyNode* lowest_combined_clip_node);
  // Pop one clip state from the top of the stack.
  void EndClip();
  // Pop clip states from the top of the stack until the top is an effect state
  // or the stack is empty.
  void EndClips();

  // State stack.
  // The size of the stack is the number of nested paired items that are
  // currently nested. Note that this is a "restore stack", i.e. the top
  // element does not represent the current state, but the state prior to
  // applying the last paired begin.
  struct StateEntry {
    // Remembers the type of paired begin that caused a state to be saved.
    // This is for checking integrity of the algorithm.
    enum PairedType { kClip, kEffect } type;
    int saved_count;

    const TransformPaintPropertyNode* transform;
    const ClipPaintPropertyNode* clip;
    const EffectPaintPropertyNode* effect;
    // See ConversionContext::previous_transform_.
    const TransformPaintPropertyNode* previous_transform;
  };
  void PushState(StateEntry::PairedType, int saved_count);
  void PopState();
  Vector<StateEntry> state_stack_;

  const PropertyTreeState& layer_state_;
  gfx::Vector2dF layer_offset_;
  bool translated_for_layer_offset_ = false;

  const TransformPaintPropertyNode* current_transform_;
  const ClipPaintPropertyNode* current_clip_;
  const EffectPaintPropertyNode* current_effect_;

  // The previous transform state before SwitchToTransform() within the current
  // clip/effect state. When the next chunk's transform is different from the
  // current transform we should restore to this transform using EndTransform()
  // which will set this field to nullptr. When a new clip/effect state starts,
  // the value of this field will be saved into the state stack and set to
  // nullptr. When the clip/effect state ends, this field will be restored to
  // the saved value.
  const TransformPaintPropertyNode* previous_transform_ = nullptr;

  // This structure accumulates bounds of all chunks under an effect. When an
  // effect starts, we emit a SaveLayer[Alpha]Op with null bounds starts, and
  // push a new |EffectBoundsInfo| onto |effect_bounds_stack_|. When the effect
  // ends, we update the bounds of the op.
  struct EffectBoundsInfo {
    // The id of the SaveLayer[Alpha]Op for this effect. It's recorded when we
    // push the op for this effect, and used when this effect ends in
    // UpdateSaveLayerBounds().
    size_t save_layer_id;
    // The transform space when the SaveLayer[Alpha]Op was emitted.
    const TransformPaintPropertyNode* transform;
    // Records the bounds of the effect which initiated the entry. Note that
    // the effect is not |this->effect| (which is the previous effect), but the
    // |current_effect_| when this entry is the top of the stack.
    FloatRect bounds;
  };
  Vector<EffectBoundsInfo> effect_bounds_stack_;

  ChunkToLayerMapper chunk_to_layer_mapper_;

  cc::DisplayItemList& cc_list_;
};

ConversionContext::~ConversionContext() {
  // End all states.
  while (state_stack_.size()) {
    if (state_stack_.back().type == StateEntry::kEffect)
      EndEffect();
    else
      EndClip();
  }
  EndTransform();
  if (translated_for_layer_offset_)
    AppendRestore(1);
}

void ConversionContext::TranslateForLayerOffsetOnce() {
  if (translated_for_layer_offset_ || layer_offset_.IsZero())
    return;

  cc_list_.StartPaint();
  cc_list_.push<cc::SaveOp>();
  cc_list_.push<cc::TranslateOp>(-layer_offset_.x(), -layer_offset_.y());
  cc_list_.EndPaintOfPairedBegin();
  translated_for_layer_offset_ = true;
}

void ConversionContext::SwitchToChunkState(const PaintChunk& chunk) {
  chunk_to_layer_mapper_.SwitchToChunk(chunk);

  const auto& chunk_state = chunk.properties;
  SwitchToEffect(chunk_state.Effect());
  SwitchToClip(chunk_state.Clip());
  SwitchToTransform(chunk_state.Transform());
}

// Tries to combine a clip node's clip rect into |combined_clip_rect|.
// Returns whether the clip has been combined.
static bool CombineClip(const ClipPaintPropertyNode* clip,
                        FloatRoundedRect& combined_clip_rect) {
  // Don't combine into a clip with clip path.
  if (clip->Parent()->ClipPath())
    return false;

  // Don't combine clips in different transform spaces.
  const auto* transform_space = clip->LocalTransformSpace();
  const auto* parent_transform_space = clip->Parent()->LocalTransformSpace();
  if (transform_space != parent_transform_space &&
      (transform_space->Parent() != parent_transform_space ||
       !transform_space->Matrix().IsIdentity()))
    return false;

  // Don't combine two rounded clip rects.
  bool clip_is_rounded = clip->ClipRect().IsRounded();
  bool combined_is_rounded = combined_clip_rect.IsRounded();
  if (clip_is_rounded && combined_is_rounded)
    return false;

  // If one is rounded and the other contains the rounded bounds, use the
  // rounded as the combined.
  if (combined_is_rounded)
    return clip->ClipRect().Rect().Contains(combined_clip_rect.Rect());
  if (clip_is_rounded) {
    if (combined_clip_rect.Rect().Contains(clip->ClipRect().Rect())) {
      combined_clip_rect = clip->ClipRect();
      return true;
    }
    return false;
  }

  // The combined is the intersection if both are rectangular.
  DCHECK(!combined_is_rounded && !clip_is_rounded);
  combined_clip_rect = FloatRoundedRect(
      Intersection(combined_clip_rect.Rect(), clip->ClipRect().Rect()));
  return true;
}

void ConversionContext::SwitchToClip(const ClipPaintPropertyNode* target_clip) {
  if (target_clip == current_clip_)
    return;

  // Step 1: Exit all clips until the lowest common ancestor is found.
  const ClipPaintPropertyNode* lca_clip =
      &LowestCommonAncestor(*target_clip, *current_clip_);
  while (current_clip_ != lca_clip) {
#if DCHECK_IS_ON()
    DCHECK(state_stack_.size() && state_stack_.back().type == StateEntry::kClip)
        << "Error: Chunk has a clip that escaped its layer's or effect's clip."
        << "\ntarget_clip:\n"
        << target_clip->ToTreeString().Utf8().data() << "current_clip_:\n"
        << current_clip_->ToTreeString().Utf8().data();
#endif
    if (!state_stack_.size() || state_stack_.back().type != StateEntry::kClip)
      break;
    current_clip_ = current_clip_->Parent();
    StateEntry& previous_state = state_stack_.back();
    if (current_clip_ == lca_clip) {
      // |lca_clip| is an intermediate clip in a series of combined clips.
      // Jump to the first of the combined clips.
      current_clip_ = lca_clip = previous_state.clip;
    }
    if (current_clip_ == previous_state.clip)
      EndClip();
  }

  if (target_clip == current_clip_)
    return;

  // Step 2: Collect all clips between the target clip and the current clip.
  // At this point the current clip must be an ancestor of the target.
  Vector<const ClipPaintPropertyNode*, 1u> pending_clips;
  for (const ClipPaintPropertyNode* clip = target_clip; clip != current_clip_;
       clip = clip->Parent()) {
    // This should never happen unless the DCHECK in step 1 failed.
    if (!clip)
      break;
    pending_clips.push_back(clip);
  }

  // Step 3: Now apply the list of clips in top-down order.
  DCHECK(pending_clips.size());
  auto pending_combined_clip_rect = pending_clips.back()->ClipRect();
  const auto* lowest_combined_clip_node = pending_clips.back();
  for (size_t i = pending_clips.size() - 1; i--;) {
    const auto* sub_clip = pending_clips[i];
    if (CombineClip(sub_clip, pending_combined_clip_rect)) {
      // Continue to combine.
      lowest_combined_clip_node = sub_clip;
    } else {
      // |sub_clip| can't be combined to previous clips. Output the current
      // combined clip, and start new combination.
      StartClip(pending_combined_clip_rect, lowest_combined_clip_node);
      pending_combined_clip_rect = sub_clip->ClipRect();
      lowest_combined_clip_node = sub_clip;
    }
  }
  StartClip(pending_combined_clip_rect, lowest_combined_clip_node);

  DCHECK_EQ(current_clip_, target_clip);
}

void ConversionContext::StartClip(
    const FloatRoundedRect& combined_clip_rect,
    const ClipPaintPropertyNode* lowest_combined_clip_node) {
  if (lowest_combined_clip_node->LocalTransformSpace() != current_transform_)
    EndTransform();
  cc_list_.StartPaint();
  cc_list_.push<cc::SaveOp>();
  ApplyTransform(lowest_combined_clip_node->LocalTransformSpace());
  const bool antialias = true;
  if (combined_clip_rect.IsRounded()) {
    cc_list_.push<cc::ClipRRectOp>(combined_clip_rect, SkClipOp::kIntersect,
                                   antialias);
  } else {
    cc_list_.push<cc::ClipRectOp>(combined_clip_rect.Rect(),
                                  SkClipOp::kIntersect, antialias);
  }
  if (lowest_combined_clip_node->ClipPath()) {
    cc_list_.push<cc::ClipPathOp>(
        lowest_combined_clip_node->ClipPath()->GetSkPath(),
        SkClipOp::kIntersect, antialias);
  }
  cc_list_.EndPaintOfPairedBegin();

  PushState(StateEntry::kClip, 1);
  current_clip_ = lowest_combined_clip_node;
  current_transform_ = lowest_combined_clip_node->LocalTransformSpace();
}

void ConversionContext::SwitchToEffect(
    const EffectPaintPropertyNode* target_effect) {
  if (target_effect == current_effect_)
    return;

  // Step 1: Exit all effects until the lowest common ancestor is found.
  const EffectPaintPropertyNode* lca_effect =
      &LowestCommonAncestor(*target_effect, *current_effect_);
  while (current_effect_ != lca_effect) {
    // This EndClips() and the later EndEffect() pop to the parent effect.
    EndClips();
#if DCHECK_IS_ON()
    DCHECK(state_stack_.size())
        << "Error: Chunk has an effect that escapes layer's effect.\n"
        << "target_effect:\n"
        << target_effect->ToTreeString().Utf8().data() << "current_effect_:\n"
        << current_effect_->ToTreeString().Utf8().data();
#endif
    if (!state_stack_.size())
      break;
    EndEffect();
  }

  // Step 2: Collect all effects between the target effect and the current
  // effect. At this point the current effect must be an ancestor of the target.
  Vector<const EffectPaintPropertyNode*, 1u> pending_effects;
  for (const EffectPaintPropertyNode* effect = target_effect;
       effect != current_effect_; effect = effect->Parent()) {
    // This should never happen unless the DCHECK in step 1 failed.
    if (!effect)
      break;
    pending_effects.push_back(effect);
  }

  // Step 3: Now apply the list of effects in top-down order.
  for (size_t i = pending_effects.size(); i--;) {
    const EffectPaintPropertyNode* sub_effect = pending_effects[i];
    DCHECK_EQ(current_effect_, sub_effect->Parent());
    StartEffect(sub_effect);
  }
}

void ConversionContext::StartEffect(const EffectPaintPropertyNode* effect) {
  // Before each effect can be applied, we must enter its output clip first,
  // or exit all clips if it doesn't have one.
  if (effect->OutputClip())
    SwitchToClip(effect->OutputClip());
  else
    EndClips();

  int saved_count = 0;
  size_t save_layer_id = kNotFound;

  // Adjust transform first. Though a non-filter effect itself doesn't depend on
  // the transform, switching to the target transform before SaveLayer[Alpha]Op
  // will help the rasterizer optimize a non-filter SaveLayer[Alpha]Op/
  // DrawRecord/Restore sequence into a single DrawRecord which is much faster.
  // This also avoids multiple Save/Concat/.../Restore pairs for multiple
  // consecutive effects in the same transform space, by issuing only one pair
  // around all of the effects.
  SwitchToTransform(effect->LocalTransformSpace());

  // We always create separate effect nodes for normal effects and filter
  // effects, so we can handle them separately.
  bool has_filter = !effect->Filter().IsEmpty();
  bool has_opacity = effect->Opacity() != 1.f;
  bool has_other_effects = effect->BlendMode() != SkBlendMode::kSrcOver ||
                           effect->GetColorFilter() != kColorFilterNone;
  DCHECK(!has_filter || !(has_opacity || has_other_effects));

  // Apply effects.
  cc_list_.StartPaint();
  if (!has_filter) {
    // TODO(ajuma): This should really be rounding instead of flooring the
    // alpha value, but that breaks slimming paint reftests.
    auto alpha =
        static_cast<uint8_t>(gfx::ToFlooredInt(255 * effect->Opacity()));
    if (has_other_effects) {
      cc::PaintFlags flags;
      flags.setBlendMode(effect->BlendMode());
      flags.setAlpha(alpha);
      flags.setColorFilter(GraphicsContext::WebCoreColorFilterToSkiaColorFilter(
          effect->GetColorFilter()));
      save_layer_id = cc_list_.push<cc::SaveLayerOp>(nullptr, &flags);
    } else {
      constexpr bool preserve_lcd_text_requests = false;
      save_layer_id = cc_list_.push<cc::SaveLayerAlphaOp>(
          nullptr, alpha, preserve_lcd_text_requests);
    }
    saved_count++;
  } else {
    // Handle filter effect.
    FloatPoint filter_origin = effect->PaintOffset();
    if (filter_origin != FloatPoint()) {
      cc_list_.push<cc::SaveOp>();
      cc_list_.push<cc::TranslateOp>(filter_origin.X(), filter_origin.Y());
      saved_count++;
    }

    // The size parameter is only used to computed the origin of zoom
    // operation, which we never generate.
    gfx::SizeF empty;
    cc::PaintFlags filter_flags;
    filter_flags.setImageFilter(cc::RenderSurfaceFilters::BuildImageFilter(
        effect->Filter().AsCcFilterOperations(), empty));
    save_layer_id = cc_list_.push<cc::SaveLayerOp>(nullptr, &filter_flags);
    saved_count++;

    if (filter_origin != FloatPoint())
      cc_list_.push<cc::TranslateOp>(-filter_origin.X(), -filter_origin.Y());
  }
  cc_list_.EndPaintOfPairedBegin();

  DCHECK_GT(saved_count, 0);
  DCHECK_LE(saved_count, 2);
  DCHECK_NE(save_layer_id, kNotFound);

  // Adjust state and push previous state onto effect stack.
  // TODO(trchen): Change input clip to expansion hint once implemented.
  const ClipPaintPropertyNode* input_clip = current_clip_;
  PushState(StateEntry::kEffect, saved_count);
  effect_bounds_stack_.emplace_back(
      EffectBoundsInfo{save_layer_id, current_transform_});
  current_clip_ = input_clip;
  current_effect_ = effect;
}

void ConversionContext::UpdateEffectBounds(
    const FloatRect& bounds,
    const TransformPaintPropertyNode* transform) {
  if (effect_bounds_stack_.IsEmpty() || bounds.IsEmpty())
    return;

  auto& effect_bounds_info = effect_bounds_stack_.back();
  FloatRect mapped_bounds = bounds;
  GeometryMapper::SourceToDestinationRect(
      transform, effect_bounds_info.transform, mapped_bounds);
  effect_bounds_info.bounds.Unite(mapped_bounds);
}

void ConversionContext::EndEffect() {
  const auto& previous_state = state_stack_.back();
  DCHECK_EQ(previous_state.type, StateEntry::kEffect);
  DCHECK_EQ(current_effect_->Parent(), previous_state.effect);
  DCHECK_EQ(current_clip_, previous_state.clip);

  DCHECK(effect_bounds_stack_.size());
  const auto& bounds_info = effect_bounds_stack_.back();
  FloatRect bounds = bounds_info.bounds;
  if (!bounds.IsEmpty()) {
    if (current_effect_->Filter().IsEmpty()) {
      cc_list_.UpdateSaveLayerBounds(bounds_info.save_layer_id, bounds);
    } else {
      // The bounds for the SaveLayer[Alpha]Op should be the source bounds
      // before the filter is applied, in the space of the TranslateOp which was
      // emitted before the SaveLayer[Alpha]Op.
      auto save_layer_bounds = bounds;
      save_layer_bounds.MoveBy(-current_effect_->PaintOffset());
      cc_list_.UpdateSaveLayerBounds(bounds_info.save_layer_id,
                                     save_layer_bounds);
      // We need to propagate the filtered bounds to the parent.
      bounds = current_effect_->MapRect(bounds);
    }
  }

  effect_bounds_stack_.pop_back();
  EndTransform();
  // Propagate the bounds to the parent effect.
  UpdateEffectBounds(bounds, current_transform_);
  PopState();
}

void ConversionContext::EndClips() {
  while (state_stack_.size() && state_stack_.back().type == StateEntry::kClip)
    EndClip();
}

void ConversionContext::EndClip() {
  DCHECK_EQ(state_stack_.back().type, StateEntry::kClip);
  DCHECK_EQ(state_stack_.back().effect, current_effect_);
  EndTransform();
  PopState();
}

void ConversionContext::PushState(StateEntry::PairedType type,
                                  int saved_count) {
  state_stack_.emplace_back(StateEntry{type, saved_count, current_transform_,
                                       current_clip_, current_effect_,
                                       previous_transform_});
  previous_transform_ = nullptr;
}

void ConversionContext::PopState() {
  DCHECK_EQ(nullptr, previous_transform_);

  const auto& previous_state = state_stack_.back();
  AppendRestore(previous_state.saved_count);
  current_transform_ = previous_state.transform;
  previous_transform_ = previous_state.previous_transform;
  current_clip_ = previous_state.clip;
  current_effect_ = previous_state.effect;
  state_stack_.pop_back();
}

void ConversionContext::SwitchToTransform(
    const TransformPaintPropertyNode* target_transform) {
  if (target_transform == current_transform_)
    return;

  EndTransform();
  if (target_transform == current_transform_)
    return;

  auto sk_matrix = GetSkMatrix(target_transform);
  if (sk_matrix.isIdentity())
    return;

  cc_list_.StartPaint();
  cc_list_.push<cc::SaveOp>();
  cc_list_.push<cc::ConcatOp>(sk_matrix);
  cc_list_.EndPaintOfPairedBegin();
  previous_transform_ = current_transform_;
  current_transform_ = target_transform;
}

void ConversionContext::EndTransform() {
  if (!previous_transform_)
    return;

  cc_list_.StartPaint();
  cc_list_.push<cc::RestoreOp>();
  cc_list_.EndPaintOfPairedEnd();
  current_transform_ = previous_transform_;
  previous_transform_ = nullptr;
}

void ConversionContext::Convert(const PaintChunkSubset& paint_chunks,
                                const DisplayItemList& display_items) {
  for (const auto& chunk : paint_chunks) {
    const auto& chunk_state = chunk.properties;
    bool switched_to_chunk_state = false;

    for (const auto& item : display_items.ItemsInPaintChunk(chunk)) {
      DCHECK(item.IsDrawing());
      auto record =
          static_cast<const DrawingDisplayItem&>(item).GetPaintRecord();
      // If we have an empty paint record, then we would prefer not to draw it.
      // However, if we also have a non-root effect, it means that the filter
      // applied might draw something even if the record is empty. We need to
      // "draw" this record in order to ensure that the effect has correct
      // visual rects.
      if ((!record || record->size() == 0) &&
          chunk_state.Effect() == EffectPaintPropertyNode::Root()) {
        continue;
      }

      TranslateForLayerOffsetOnce();
      if (!switched_to_chunk_state) {
        SwitchToChunkState(chunk);
        switched_to_chunk_state = true;
      }

      cc_list_.StartPaint();
      if (record && record->size() != 0)
        cc_list_.push<cc::DrawRecordOp>(std::move(record));
      cc_list_.EndPaintOfUnpaired(
          chunk_to_layer_mapper_.MapVisualRect(item.VisualRect()));
    }
    UpdateEffectBounds(chunk.bounds, chunk_state.Transform());
  }
}

}  // unnamed namespace

void PaintChunksToCcLayer::ConvertInto(
    const PaintChunkSubset& paint_chunks,
    const PropertyTreeState& layer_state,
    const gfx::Vector2dF& layer_offset,
    const FloatSize& visual_rect_subpixel_offset,
    const DisplayItemList& display_items,
    cc::DisplayItemList& cc_list) {
  if (RuntimeEnabledFeatures::DisablePaintChunksToCcLayerEnabled())
    return;
  ConversionContext(layer_state, layer_offset, visual_rect_subpixel_offset,
                    cc_list)
      .Convert(paint_chunks, display_items);
}

scoped_refptr<cc::DisplayItemList> PaintChunksToCcLayer::Convert(
    const PaintChunkSubset& paint_chunks,
    const PropertyTreeState& layer_state,
    const gfx::Vector2dF& layer_offset,
    const DisplayItemList& display_items,
    cc::DisplayItemList::UsageHint hint,
    RasterUnderInvalidationCheckingParams* under_invalidation_checking_params) {
  auto cc_list = base::MakeRefCounted<cc::DisplayItemList>(hint);
  ConvertInto(paint_chunks, layer_state, layer_offset, FloatSize(),
              display_items, *cc_list);

  if (under_invalidation_checking_params) {
    auto& params = *under_invalidation_checking_params;
    PaintRecorder recorder;
    recorder.beginRecording(params.interest_rect);
    // Create a complete cloned list for under-invalidation checking. We can't
    // use cc_list because it is not finalized yet.
    auto list_clone = base::MakeRefCounted<cc::DisplayItemList>(
        cc::DisplayItemList::kToBeReleasedAsPaintOpBuffer);
    ConvertInto(paint_chunks, layer_state, layer_offset, FloatSize(),
                display_items, *list_clone);
    recorder.getRecordingCanvas()->drawPicture(list_clone->ReleaseAsRecord());
    params.tracking.CheckUnderInvalidations(params.debug_name,
                                            recorder.finishRecordingAsPicture(),
                                            params.interest_rect);
    if (auto record = params.tracking.UnderInvalidationRecord()) {
      cc_list->StartPaint();
      cc_list->push<cc::DrawRecordOp>(std::move(record));
      cc_list->EndPaintOfUnpaired(params.interest_rect);
    }
  }

  cc_list->Finalize();
  return cc_list;
}

}  // namespace blink
