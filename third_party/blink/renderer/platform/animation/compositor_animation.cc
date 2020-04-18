// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/animation/compositor_animation.h"

#include "cc/animation/animation_id_provider.h"
#include "cc/animation/animation_timeline.h"
#include "third_party/blink/renderer/platform/animation/compositor_animation_delegate.h"
#include "third_party/blink/renderer/platform/animation/compositor_keyframe_model.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

std::unique_ptr<CompositorAnimation> CompositorAnimation::Create() {
  return std::make_unique<CompositorAnimation>(
      cc::SingleKeyframeEffectAnimation::Create(
          cc::AnimationIdProvider::NextAnimationId()));
}

std::unique_ptr<CompositorAnimation>
CompositorAnimation::CreateWorkletAnimation(
    const String& name,
    std::unique_ptr<CompositorScrollTimeline> scroll_timeline) {
  return std::make_unique<CompositorAnimation>(cc::WorkletAnimation::Create(
      cc::AnimationIdProvider::NextAnimationId(),
      std::string(name.Ascii().data(), name.length()),
      std::move(scroll_timeline)));
}

CompositorAnimation::CompositorAnimation(
    scoped_refptr<cc::SingleKeyframeEffectAnimation> animation)
    : animation_(animation), delegate_() {}

CompositorAnimation::~CompositorAnimation() {
  SetAnimationDelegate(nullptr);
  // Detach animation from timeline, otherwise it stays there (leaks) until
  // compositor shutdown.
  if (animation_->animation_timeline())
    animation_->animation_timeline()->DetachAnimation(animation_);
}

cc::SingleKeyframeEffectAnimation* CompositorAnimation::CcAnimation() const {
  return animation_.get();
}

void CompositorAnimation::SetAnimationDelegate(
    CompositorAnimationDelegate* delegate) {
  delegate_ = delegate;
  animation_->set_animation_delegate(delegate ? this : nullptr);
}

void CompositorAnimation::AttachElement(const CompositorElementId& id) {
  animation_->AttachElement(id);
}

void CompositorAnimation::DetachElement() {
  animation_->DetachElement();
}

bool CompositorAnimation::IsElementAttached() const {
  return !!animation_->element_id();
}

void CompositorAnimation::AddKeyframeModel(
    std::unique_ptr<CompositorKeyframeModel> keyframe_model) {
  animation_->AddKeyframeModel(keyframe_model->ReleaseCcKeyframeModel());
}

void CompositorAnimation::RemoveKeyframeModel(int keyframe_model_id) {
  animation_->RemoveKeyframeModel(keyframe_model_id);
}

void CompositorAnimation::PauseKeyframeModel(int keyframe_model_id,
                                             double time_offset) {
  animation_->PauseKeyframeModel(keyframe_model_id, time_offset);
}

void CompositorAnimation::AbortKeyframeModel(int keyframe_model_id) {
  animation_->AbortKeyframeModel(keyframe_model_id);
}

void CompositorAnimation::NotifyAnimationStarted(base::TimeTicks monotonic_time,
                                                 int target_property,
                                                 int group) {
  if (delegate_) {
    delegate_->NotifyAnimationStarted(
        (monotonic_time - base::TimeTicks()).InSecondsF(), group);
  }
}

void CompositorAnimation::NotifyAnimationFinished(
    base::TimeTicks monotonic_time,
    int target_property,
    int group) {
  if (delegate_) {
    delegate_->NotifyAnimationFinished(
        (monotonic_time - base::TimeTicks()).InSecondsF(), group);
  }
}

void CompositorAnimation::NotifyAnimationAborted(base::TimeTicks monotonic_time,
                                                 int target_property,
                                                 int group) {
  if (delegate_) {
    delegate_->NotifyAnimationAborted(
        (monotonic_time - base::TimeTicks()).InSecondsF(), group);
  }
}

void CompositorAnimation::NotifyAnimationTakeover(
    base::TimeTicks monotonic_time,
    int target_property,
    base::TimeTicks animation_start_time,
    std::unique_ptr<cc::AnimationCurve> curve) {
  if (delegate_) {
    delegate_->NotifyAnimationTakeover(
        (monotonic_time - base::TimeTicks()).InSecondsF(),
        (animation_start_time - base::TimeTicks()).InSecondsF(),
        std::move(curve));
  }
}

}  // namespace blink
