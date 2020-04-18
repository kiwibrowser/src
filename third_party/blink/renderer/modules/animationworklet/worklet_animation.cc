// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/animationworklet/worklet_animation.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/modules/v8/animation_effect_or_animation_effect_sequence.h"
#include "third_party/blink/renderer/core/animation/element_animations.h"
#include "third_party/blink/renderer/core/animation/keyframe_effect_model.h"
#include "third_party/blink/renderer/core/animation/scroll_timeline.h"
#include "third_party/blink/renderer/core/animation/timing.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {
bool ConvertAnimationEffects(
    const AnimationEffectOrAnimationEffectSequence& effects,
    HeapVector<Member<KeyframeEffect>>& keyframe_effects,
    String& error_string) {
  DCHECK(keyframe_effects.IsEmpty());

  // Currently we only support KeyframeEffect.
  if (effects.IsAnimationEffect()) {
    auto* const effect = effects.GetAsAnimationEffect();
    if (!effect->IsKeyframeEffect()) {
      error_string = "Effect must be a KeyframeEffect object";
      return false;
    }
    keyframe_effects.push_back(ToKeyframeEffect(effect));
  } else {
    const HeapVector<Member<AnimationEffect>>& effect_sequence =
        effects.GetAsAnimationEffectSequence();
    keyframe_effects.ReserveInitialCapacity(effect_sequence.size());
    for (const auto& effect : effect_sequence) {
      if (!effect->IsKeyframeEffect()) {
        error_string = "Effects must all be KeyframeEffect objects";
        return false;
      }
      keyframe_effects.push_back(ToKeyframeEffect(effect));
    }
  }

  if (keyframe_effects.IsEmpty()) {
    error_string = "Effects array must be non-empty";
    return false;
  }

  if (keyframe_effects.size() > 1) {
    // TODO(yigu): We should allow group effects eventually by spec. See
    // crbug.com/767043.
    error_string = "Multiple effects are not currently supported";
    return false;
  }

  // TODO(crbug.com/781816): Allow using effects with no target.
  for (const auto& effect : keyframe_effects) {
    if (!effect->target()) {
      error_string = "All effect targets must exist";
      return false;
    }
  }

  Document& target_document = keyframe_effects.at(0)->target()->GetDocument();
  for (const auto& effect : keyframe_effects) {
    if (effect->target()->GetDocument() != target_document) {
      error_string = "All effects must target elements in the same document";
      return false;
    }
  }
  return true;
}

bool ValidateTimeline(const DocumentTimelineOrScrollTimeline& timeline,
                      String& error_string) {
  if (timeline.IsScrollTimeline()) {
    DoubleOrScrollTimelineAutoKeyword time_range;
    timeline.GetAsScrollTimeline()->timeRange(time_range);
    if (time_range.IsScrollTimelineAutoKeyword()) {
      error_string = "ScrollTimeline timeRange must have non-auto value";
      return false;
    }
  }
  return true;
}

AnimationTimeline* ConvertAnimationTimeline(
    const Document& document,
    const DocumentTimelineOrScrollTimeline& timeline) {
  if (timeline.IsScrollTimeline())
    return timeline.GetAsScrollTimeline();

  if (timeline.IsDocumentTimeline())
    return timeline.GetAsDocumentTimeline();

  return &document.Timeline();
}

bool CheckElementComposited(const Element& target) {
  return target.GetLayoutObject() &&
         target.GetLayoutObject()->GetCompositingState() ==
             kPaintsIntoOwnBacking;
}

CompositorElementId GetCompositorScrollElementId(const Element& element) {
  DCHECK(element.GetLayoutObject());
  DCHECK(element.GetLayoutObject()->HasLayer());
  return CompositorElementIdFromUniqueObjectId(
      element.GetLayoutObject()->UniqueId(),
      CompositorElementIdNamespace::kScroll);
}

// Convert the blink concept of a ScrollTimeline orientation into the cc one.
//
// The compositor does not know about writing modes, so we have to convert the
// web concepts of 'block' and 'inline' direction into absolute vertical or
// horizontal directions.
//
// TODO(smcgruer): If the writing mode of a scroller changes, we have to update
// any related cc::ScrollTimeline somehow.
CompositorScrollTimeline::ScrollDirection ConvertOrientation(
    ScrollTimeline::ScrollDirection orientation,
    bool is_horizontal_writing_mode) {
  switch (orientation) {
    case ScrollTimeline::Block:
      return is_horizontal_writing_mode ? CompositorScrollTimeline::Vertical
                                        : CompositorScrollTimeline::Horizontal;
    case ScrollTimeline::Inline:
      return is_horizontal_writing_mode ? CompositorScrollTimeline::Horizontal
                                        : CompositorScrollTimeline::Vertical;
    default:
      NOTREACHED();
      return CompositorScrollTimeline::Vertical;
  }
}

// Converts a blink::ScrollTimeline into a cc::ScrollTimeline.
//
// If the timeline cannot be converted, returns nullptr.
std::unique_ptr<CompositorScrollTimeline> ToCompositorScrollTimeline(
    AnimationTimeline* timeline) {
  if (!timeline || timeline->IsDocumentTimeline())
    return nullptr;

  ScrollTimeline* scroll_timeline = ToScrollTimeline(timeline);
  Element* scroll_source = scroll_timeline->scrollSource();
  CompositorElementId element_id = GetCompositorScrollElementId(*scroll_source);

  DoubleOrScrollTimelineAutoKeyword time_range;
  scroll_timeline->timeRange(time_range);
  // TODO(smcgruer): Handle 'auto' time range value.
  DCHECK(time_range.IsDouble());

  LayoutBox* box = scroll_source->GetLayoutBox();
  DCHECK(box);
  CompositorScrollTimeline::ScrollDirection orientation = ConvertOrientation(
      scroll_timeline->GetOrientation(), box->IsHorizontalWritingMode());

  return std::make_unique<CompositorScrollTimeline>(element_id, orientation,
                                                    time_range.GetAsDouble());
}

void StartEffectOnCompositor(CompositorAnimation* animation,
                             KeyframeEffect* effect) {
  DCHECK(effect);
  Element& target = *effect->target();
  effect->Model()->SnapshotAllCompositorKeyframesIfNecessary(
      target, target.ComputedStyleRef(), target.ParentComputedStyle());

  int group = 0;
  base::Optional<double> start_time = base::nullopt;
  double time_offset = 0;
  double playback_rate = 1;

  effect->StartAnimationOnCompositor(group, start_time, time_offset,
                                     playback_rate, animation);
}

unsigned NextSequenceNumber() {
  // TODO(majidvp): This should actually come from the same source as other
  // animation so that they have the correct ordering.
  static unsigned next = 0;
  return ++next;
}
}  // namespace

WorkletAnimation* WorkletAnimation::Create(
    String animator_name,
    const AnimationEffectOrAnimationEffectSequence& effects,
    ExceptionState& exception_state) {
  return Create(animator_name, effects, DocumentTimelineOrScrollTimeline(),
                nullptr, exception_state);
}

WorkletAnimation* WorkletAnimation::Create(
    String animator_name,
    const AnimationEffectOrAnimationEffectSequence& effects,
    DocumentTimelineOrScrollTimeline timeline,
    ExceptionState& exception_state) {
  return Create(animator_name, effects, timeline, nullptr, exception_state);
}
WorkletAnimation* WorkletAnimation::Create(
    String animator_name,
    const AnimationEffectOrAnimationEffectSequence& effects,
    DocumentTimelineOrScrollTimeline timeline,
    scoped_refptr<SerializedScriptValue> options,
    ExceptionState& exception_state) {
  DCHECK(IsMainThread());

  if (!Platform::Current()->IsThreadedAnimationEnabled()) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "AnimationWorklet requires threaded animations to be enabled");
    return nullptr;
  }

  HeapVector<Member<KeyframeEffect>> keyframe_effects;
  String error_string;
  if (!ConvertAnimationEffects(effects, keyframe_effects, error_string)) {
    exception_state.ThrowDOMException(kNotSupportedError, error_string);
    return nullptr;
  }

  if (!ValidateTimeline(timeline, error_string)) {
    exception_state.ThrowDOMException(kNotSupportedError, error_string);
    return nullptr;
  }

  Document& document = keyframe_effects.at(0)->target()->GetDocument();
  AnimationTimeline* animation_timeline =
      ConvertAnimationTimeline(document, timeline);

  WorkletAnimation* animation =
      new WorkletAnimation(animator_name, document, keyframe_effects,
                           animation_timeline, std::move(options));

  return animation;
}

WorkletAnimation::WorkletAnimation(
    const String& animator_name,
    Document& document,
    const HeapVector<Member<KeyframeEffect>>& effects,
    AnimationTimeline* timeline,
    scoped_refptr<SerializedScriptValue> options)
    : sequence_number_(NextSequenceNumber()),
      animator_name_(animator_name),
      play_state_(Animation::kIdle),
      document_(document),
      effects_(effects),
      timeline_(timeline),
      options_(std::move(options)) {
  DCHECK(IsMainThread());
  DCHECK(Platform::Current()->IsThreadedAnimationEnabled());

  AnimationEffect* target_effect = effects_.at(0);
  target_effect->Attach(this);

  if (timeline_->IsScrollTimeline())
    ToScrollTimeline(timeline_)->AttachAnimation();
}

String WorkletAnimation::playState() {
  DCHECK(IsMainThread());
  return Animation::PlayStateString(play_state_);
}

void WorkletAnimation::play() {
  DCHECK(IsMainThread());
  if (play_state_ == Animation::kPending)
    return;
  document_->GetWorkletAnimationController().AttachAnimation(*this);
  play_state_ = Animation::kPending;

  Element* target = GetEffect()->target();
  if (!target)
    return;
  target->EnsureElementAnimations().GetWorkletAnimations().insert(this);
  // TODO(majidvp): This should be removed once worklet animation correctly
  // updates its effect timing. https://crbug.com/814851.
  target->SetNeedsAnimationStyleRecalc();
}

void WorkletAnimation::cancel() {
  DCHECK(IsMainThread());
  if (play_state_ == Animation::kIdle)
    return;
  document_->GetWorkletAnimationController().DetachAnimation(*this);

  if (compositor_animation_) {
    GetEffect()->CancelAnimationOnCompositor(compositor_animation_.get());
    DestroyCompositorAnimation();
  }

  play_state_ = Animation::kIdle;

  Element* target = GetEffect()->target();
  if (!target)
    return;
  target->EnsureElementAnimations().GetWorkletAnimations().erase(this);
  // TODO(majidvp): This should be removed once worklet animation correctly
  // updates its effect timing. https://crbug.com/814851.
  target->SetNeedsAnimationStyleRecalc();
}

bool WorkletAnimation::Playing() const {
  return play_state_ == Animation::kRunning;
}

void WorkletAnimation::UpdateIfNecessary() {
  // TODO(crbug.com/833846): This is updating more often than necessary. This
  // gets fixed once WorkletAnimation becomes a subclass of Animation.
  Update(kTimingUpdateOnDemand);
}

void WorkletAnimation::EffectInvalidated() {
  document_->GetWorkletAnimationController().InvalidateAnimation(*this);
}

void WorkletAnimation::Update(TimingUpdateReason reason) {
  if (play_state_ != Animation::kRunning)
    return;

  if (!start_time_)
    return;

  // TODO(crbug.com/756359): For now we use 0 as inherited time in but we will
  // need to get the inherited time from worklet context.
  double inherited_time_seconds = 0;
  GetEffect()->UpdateInheritedTime(inherited_time_seconds, reason);
}

bool WorkletAnimation::UpdateCompositingState() {
  switch (play_state_) {
    case Animation::kPending: {
      String failure_message;
      if (StartOnCompositor(&failure_message))
        return true;
      document_->AddConsoleMessage(ConsoleMessage::Create(
          kOtherMessageSource, kWarningMessageLevel, failure_message));
      return false;
    }
    case Animation::kRunning: {
      UpdateOnCompositor();
      return false;
    }
    default:
      return false;
  }
}

bool WorkletAnimation::StartOnCompositor(String* failure_message) {
  DCHECK(IsMainThread());
  Element& target = *GetEffect()->target();

  // TODO(crbug.com/836393): This should not be possible but it is currently
  // happening and needs to be investigated/fixed.
  if (!target.GetComputedStyle()) {
    if (failure_message)
      *failure_message = "The target element does not have style.";
    return false;
  }
  // CheckCanStartAnimationOnCompositor requires that the property-specific
  // keyframe groups have been created. To ensure this we manually snapshot the
  // frames in the target effect.
  // TODO(smcgruer): This shouldn't be necessary - Animation doesn't do this.
  GetEffect()->Model()->SnapshotAllCompositorKeyframesIfNecessary(
      target, target.ComputedStyleRef(), target.ParentComputedStyle());

  if (!CheckElementComposited(target)) {
    if (failure_message)
      *failure_message = "The target element is not composited.";
    return false;
  }

  if (timeline_->IsScrollTimeline() &&
      !CheckElementComposited(*ToScrollTimeline(timeline_)->scrollSource())) {
    if (failure_message)
      *failure_message = "The ScrollTimeline scrollSource is not composited.";
    return false;
  }

  double playback_rate = 1;
  CompositorAnimations::FailureCode failure_code =
      GetEffect()->CheckCanStartAnimationOnCompositor(playback_rate);

  if (!failure_code.Ok()) {
    play_state_ = Animation::kIdle;
    if (failure_message)
      *failure_message = failure_code.reason;
    return false;
  }

  if (!compositor_animation_) {
    compositor_animation_ = CompositorAnimation::CreateWorkletAnimation(
        animator_name_, ToCompositorScrollTimeline(timeline_));
    compositor_animation_->SetAnimationDelegate(this);
  }

  // Register ourselves on the compositor timeline. This will cause our cc-side
  // animation animation to be registered.
  if (CompositorAnimationTimeline* compositor_timeline =
          document_->Timeline().CompositorTimeline())
    compositor_timeline->AnimationAttached(*this);

  CompositorAnimations::AttachCompositedLayers(target,
                                               compositor_animation_.get());

  // TODO(smcgruer): We need to start all of the effects, not just the first.
  StartEffectOnCompositor(compositor_animation_.get(), GetEffect());
  play_state_ = Animation::kRunning;

  bool is_null;
  double time = timeline_->currentTime(is_null);
  if (!is_null)
    start_time_ = time;

  return true;
}

void WorkletAnimation::UpdateOnCompositor() {
  // We want to update the keyframe effect on compositor animation without
  // destroying the compositor animation instance. This is achieved by
  // canceling, and start the blink keyframe effect on compositor.
  GetEffect()->CancelAnimationOnCompositor(compositor_animation_.get());
  StartEffectOnCompositor(compositor_animation_.get(), GetEffect());
}

void WorkletAnimation::DestroyCompositorAnimation() {
  if (compositor_animation_ && compositor_animation_->IsElementAttached())
    compositor_animation_->DetachElement();

  if (CompositorAnimationTimeline* compositor_timeline =
          document_->Timeline().CompositorTimeline())
    compositor_timeline->AnimationDestroyed(*this);

  if (compositor_animation_) {
    compositor_animation_->SetAnimationDelegate(nullptr);
    compositor_animation_ = nullptr;
  }
}

KeyframeEffect* WorkletAnimation::GetEffect() const {
  DCHECK(effects_.at(0));
  return effects_.at(0);
}

void WorkletAnimation::Dispose() {
  DCHECK(IsMainThread());
  if (timeline_->IsScrollTimeline())
    ToScrollTimeline(timeline_)->DetachAnimation();
  DestroyCompositorAnimation();
}

void WorkletAnimation::Trace(blink::Visitor* visitor) {
  visitor->Trace(document_);
  visitor->Trace(effects_);
  visitor->Trace(timeline_);
  WorkletAnimationBase::Trace(visitor);
}

}  // namespace blink
