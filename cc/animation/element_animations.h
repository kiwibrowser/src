// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ELEMENT_ANIMATIONS_H_
#define CC_ANIMATION_ELEMENT_ANIMATIONS_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "cc/animation/animation_export.h"
#include "cc/animation/animation_target.h"
#include "cc/trees/element_id.h"
#include "cc/trees/property_animation_state.h"
#include "cc/trees/target_property.h"
#include "ui/gfx/geometry/scroll_offset.h"
#include "ui/gfx/transform.h"

namespace gfx {
class BoxF;
}

namespace cc {

class AnimationHost;
class FilterOperations;
class KeyframeEffect;
class TransformOperations;
enum class ElementListType;
struct AnimationEvent;

enum class UpdateTickingType { NORMAL, FORCE };

// An ElementAnimations owns a list of all KeyframeEffects attached to a single
// target (represented by an ElementId).
//
// Note that a particular target may not actually be an element in the web sense
// of the word; this naming is a legacy leftover. A target is just an amorphous
// blob that has properties that can be animated.
class CC_ANIMATION_EXPORT ElementAnimations
    : public AnimationTarget,
      public base::RefCounted<ElementAnimations> {
 public:
  static scoped_refptr<ElementAnimations> Create();

  ElementId element_id() const { return element_id_; }
  void SetElementId(ElementId element_id);

  // Parent AnimationHost.
  AnimationHost* animation_host() { return animation_host_; }
  const AnimationHost* animation_host() const { return animation_host_; }
  void SetAnimationHost(AnimationHost* host);

  void InitAffectedElementTypes();
  void ClearAffectedElementTypes();

  void ElementRegistered(ElementId element_id, ElementListType list_type);
  void ElementUnregistered(ElementId element_id, ElementListType list_type);

  void AddKeyframeEffect(KeyframeEffect* keyframe_effect);
  void RemoveKeyframeEffect(KeyframeEffect* keyframe_effect);
  bool IsEmpty() const;

  typedef base::ObserverList<KeyframeEffect> KeyframeEffectsList;
  const KeyframeEffectsList& keyframe_effects_list() const {
    return keyframe_effects_list_;
  }

  // Ensures that the list of active animations on the main thread and the impl
  // thread are kept in sync. This function does not take ownership of the impl
  // thread ElementAnimations.
  void PushPropertiesTo(
      scoped_refptr<ElementAnimations> element_animations_impl) const;

  // Returns true if there are any effects that have neither finished nor
  // aborted.
  bool HasTickingKeyframeEffect() const;

  // Returns true if there are any KeyframeModels at all to process.
  bool HasAnyKeyframeModel() const;

  bool HasAnyAnimationTargetingProperty(TargetProperty::Type property) const;

  // Returns true if there is an animation that is either currently animating
  // the given property or scheduled to animate this property in the future, and
  // that affects the given tree type.
  bool IsPotentiallyAnimatingProperty(TargetProperty::Type target_property,
                                      ElementListType list_type) const;

  // Returns true if there is an animation that is currently animating the given
  // property and that affects the given tree type.
  bool IsCurrentlyAnimatingProperty(TargetProperty::Type target_property,
                                    ElementListType list_type) const;

  void NotifyAnimationStarted(const AnimationEvent& event);
  void NotifyAnimationFinished(const AnimationEvent& event);
  void NotifyAnimationAborted(const AnimationEvent& event);
  void NotifyAnimationPropertyUpdate(const AnimationEvent& event);
  void NotifyAnimationTakeover(const AnimationEvent& event);

  bool has_element_in_active_list() const {
    return has_element_in_active_list_;
  }
  bool has_element_in_pending_list() const {
    return has_element_in_pending_list_;
  }
  bool has_element_in_any_list() const {
    return has_element_in_active_list_ || has_element_in_pending_list_;
  }

  void set_has_element_in_active_list(bool has_element_in_active_list) {
    has_element_in_active_list_ = has_element_in_active_list;
  }
  void set_has_element_in_pending_list(bool has_element_in_pending_list) {
    has_element_in_pending_list_ = has_element_in_pending_list;
  }

  bool TransformAnimationBoundsForBox(const gfx::BoxF& box,
                                      gfx::BoxF* bounds) const;

  bool HasOnlyTranslationTransforms(ElementListType list_type) const;

  bool AnimationsPreserveAxisAlignment() const;

  // Sets |start_scale| to the maximum of starting animation scale along any
  // dimension at any destination in active animations. Returns false if the
  // starting scale cannot be computed.
  bool AnimationStartScale(ElementListType list_type, float* start_scale) const;

  // Sets |max_scale| to the maximum scale along any dimension at any
  // destination in active animations. Returns false if the maximum scale cannot
  // be computed.
  bool MaximumTargetScale(ElementListType list_type, float* max_scale) const;

  bool ScrollOffsetAnimationWasInterrupted() const;

  void SetNeedsPushProperties();
  bool needs_push_properties() const { return needs_push_properties_; }

  void UpdateClientAnimationState();

  void NotifyClientFloatAnimated(float opacity,
                                 int target_property_id,
                                 KeyframeModel* keyframe_model) override;
  void NotifyClientFilterAnimated(const FilterOperations& filter,
                                  int target_property_id,
                                  KeyframeModel* keyframe_model) override;
  void NotifyClientTransformOperationsAnimated(
      const TransformOperations& operations,
      int target_property_id,
      KeyframeModel* keyframe_model) override;
  void NotifyClientScrollOffsetAnimated(const gfx::ScrollOffset& scroll_offset,
                                        int target_property_id,
                                        KeyframeModel* keyframe_model) override;

  gfx::ScrollOffset ScrollOffsetForAnimation() const;

 private:
  friend class base::RefCounted<ElementAnimations>;

  ElementAnimations();
  ~ElementAnimations() override;

  void OnFilterAnimated(ElementListType list_type,
                        const FilterOperations& filters);
  void OnOpacityAnimated(ElementListType list_type, float opacity);
  void OnTransformAnimated(ElementListType list_type,
                           const gfx::Transform& transform);
  void OnScrollOffsetAnimated(ElementListType list_type,
                              const gfx::ScrollOffset& scroll_offset);

  static TargetProperties GetPropertiesMaskForAnimationState();

  void UpdateKeyframeEffectsTickingState(
      UpdateTickingType update_ticking_type) const;
  void RemoveKeyframeEffectsFromTicking() const;

  bool KeyframeModelAffectsActiveElements(KeyframeModel* keyframe_model) const;
  bool KeyframeModelAffectsPendingElements(KeyframeModel* keyframe_model) const;

  KeyframeEffectsList keyframe_effects_list_;
  AnimationHost* animation_host_;
  ElementId element_id_;

  bool has_element_in_active_list_;
  bool has_element_in_pending_list_;

  mutable bool needs_push_properties_;

  PropertyAnimationState active_state_;
  PropertyAnimationState pending_state_;

  DISALLOW_COPY_AND_ASSIGN(ElementAnimations);
};

}  // namespace cc

#endif  // CC_ANIMATION_ELEMENT_ANIMATIONS_H_
