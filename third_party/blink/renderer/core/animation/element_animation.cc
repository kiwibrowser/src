// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/element_animation.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/animation/animation.h"
#include "third_party/blink/renderer/core/animation/document_timeline.h"
#include "third_party/blink/renderer/core/animation/effect_input.h"
#include "third_party/blink/renderer/core/animation/effect_model.h"
#include "third_party/blink/renderer/core/animation/keyframe_effect.h"
#include "third_party/blink/renderer/core/animation/keyframe_effect_model.h"
#include "third_party/blink/renderer/core/animation/timing.h"
#include "third_party/blink/renderer/core/animation/timing_input.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

Animation* ElementAnimation::animate(
    ScriptState* script_state,
    Element& element,
    const ScriptValue& keyframes,
    UnrestrictedDoubleOrKeyframeAnimationOptions options,
    ExceptionState& exception_state) {
  EffectModel::CompositeOperation composite = EffectModel::kCompositeReplace;
  if (options.IsKeyframeAnimationOptions()) {
    composite = EffectModel::StringToCompositeOperation(
        options.GetAsKeyframeAnimationOptions().composite());
  }

  KeyframeEffectModelBase* effect = EffectInput::Convert(
      &element, keyframes, composite, script_state, exception_state);
  if (exception_state.HadException())
    return nullptr;

  Timing timing =
      TimingInput::Convert(options, &element.GetDocument(), exception_state);
  if (exception_state.HadException())
    return nullptr;

  Animation* animation = animateInternal(element, effect, timing);
  if (options.IsKeyframeAnimationOptions())
    animation->setId(options.GetAsKeyframeAnimationOptions().id());
  return animation;
}

Animation* ElementAnimation::animate(ScriptState* script_state,
                                     Element& element,
                                     const ScriptValue& keyframes,
                                     ExceptionState& exception_state) {
  KeyframeEffectModelBase* effect =
      EffectInput::Convert(&element, keyframes, EffectModel::kCompositeReplace,
                           script_state, exception_state);
  if (exception_state.HadException())
    return nullptr;
  return animateInternal(element, effect, Timing());
}

HeapVector<Member<Animation>> ElementAnimation::getAnimations(
    Element& element) {
  element.GetDocument().UpdateStyleAndLayoutTreeForNode(&element);

  HeapVector<Member<Animation>> animations;
  if (!element.HasAnimations())
    return animations;

  for (const auto& animation :
       element.GetDocument().Timeline().getAnimations()) {
    DCHECK(animation->effect());
    if (ToKeyframeEffect(animation->effect())->target() == element &&
        (animation->effect()->IsCurrent() || animation->effect()->IsInEffect()))
      animations.push_back(animation);
  }
  return animations;
}

Animation* ElementAnimation::animateInternal(Element& element,
                                             KeyframeEffectModelBase* effect,
                                             const Timing& timing) {
  KeyframeEffect* keyframe_effect =
      KeyframeEffect::Create(&element, effect, timing);
  return element.GetDocument().Timeline().Play(keyframe_effect);
}

}  // namespace blink
