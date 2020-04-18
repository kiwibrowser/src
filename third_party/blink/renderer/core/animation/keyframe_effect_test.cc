// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/keyframe_effect.h"

#include <memory>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/unrestricted_double_or_keyframe_effect_options.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_keyframe_effect_options.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_object_builder.h"
#include "third_party/blink/renderer/core/animation/animation.h"
#include "third_party/blink/renderer/core/animation/animation_clock.h"
#include "third_party/blink/renderer/core/animation/animation_test_helper.h"
#include "third_party/blink/renderer/core/animation/document_timeline.h"
#include "third_party/blink/renderer/core/animation/effect_timing.h"
#include "third_party/blink/renderer/core/animation/keyframe_effect_model.h"
#include "third_party/blink/renderer/core/animation/optional_effect_timing.h"
#include "third_party/blink/renderer/core/animation/timing.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "v8/include/v8.h"

namespace blink {

class KeyframeEffectTest : public PageTestBase {
 protected:
  void SetUp() override {
    PageTestBase::SetUp(IntSize());
    element = GetDocument().CreateElementForBinding("foo");
    GetDocument().documentElement()->AppendChild(element.Get());
  }

  KeyframeEffectModelBase* CreateEmptyEffectModel() {
    return StringKeyframeEffectModel::Create(StringKeyframeVector());
  }

  Persistent<Element> element;
};

class AnimationKeyframeEffectV8Test : public KeyframeEffectTest {
 protected:
  static KeyframeEffect* CreateAnimation(ScriptState* script_state,
                                         Element* element,
                                         const ScriptValue& keyframe_object,
                                         double timing_input) {
    NonThrowableExceptionState exception_state;
    return KeyframeEffect::Create(
        script_state, element, keyframe_object,
        UnrestrictedDoubleOrKeyframeEffectOptions::FromUnrestrictedDouble(
            timing_input),
        exception_state);
  }
  static KeyframeEffect* CreateAnimation(
      ScriptState* script_state,
      Element* element,
      const ScriptValue& keyframe_object,
      const KeyframeEffectOptions& timing_input) {
    NonThrowableExceptionState exception_state;
    return KeyframeEffect::Create(
        script_state, element, keyframe_object,
        UnrestrictedDoubleOrKeyframeEffectOptions::FromKeyframeEffectOptions(
            timing_input),
        exception_state);
  }
  static KeyframeEffect* CreateAnimation(ScriptState* script_state,
                                         Element* element,
                                         const ScriptValue& keyframe_object) {
    NonThrowableExceptionState exception_state;
    return KeyframeEffect::Create(script_state, element, keyframe_object,
                                  exception_state);
  }
};

TEST_F(AnimationKeyframeEffectV8Test, CanCreateAnAnimation) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();
  NonThrowableExceptionState exception_state;

  Vector<ScriptValue> blink_keyframes = {
      V8ObjectBuilder(script_state)
          .AddString("width", "100px")
          .AddString("offset", "0")
          .AddString("easing", "ease-in-out")
          .GetScriptValue(),
      V8ObjectBuilder(script_state)
          .AddString("width", "0px")
          .AddString("offset", "1")
          .AddString("easing", "cubic-bezier(1, 1, 0.3, 0.3)")
          .GetScriptValue()};

  ScriptValue js_keyframes(
      script_state,
      ToV8(blink_keyframes, scope.GetContext()->Global(), scope.GetIsolate()));

  KeyframeEffect* animation =
      CreateAnimation(script_state, element.Get(), js_keyframes, 0);

  Element* target = animation->target();
  EXPECT_EQ(*element.Get(), *target);

  const KeyframeVector keyframes = animation->Model()->GetFrames();

  EXPECT_EQ(0, keyframes[0]->CheckedOffset());
  EXPECT_EQ(1, keyframes[1]->CheckedOffset());

  const CSSValue& keyframe1_width =
      ToStringKeyframe(keyframes[0].get())
          ->CssPropertyValue(PropertyHandle(GetCSSPropertyWidth()));
  const CSSValue& keyframe2_width =
      ToStringKeyframe(keyframes[1].get())
          ->CssPropertyValue(PropertyHandle(GetCSSPropertyWidth()));

  EXPECT_EQ("100px", keyframe1_width.CssText());
  EXPECT_EQ("0px", keyframe2_width.CssText());

  EXPECT_EQ(*(CubicBezierTimingFunction::Preset(
                CubicBezierTimingFunction::EaseType::EASE_IN_OUT)),
            keyframes[0]->Easing());
  EXPECT_EQ(*(CubicBezierTimingFunction::Create(1, 1, 0.3, 0.3).get()),
            keyframes[1]->Easing());
}

TEST_F(AnimationKeyframeEffectV8Test, SetAndRetrieveEffectComposite) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();
  NonThrowableExceptionState exception_state;

  v8::Local<v8::Object> effect_options = v8::Object::New(scope.GetIsolate());
  SetV8ObjectPropertyAsString(scope.GetIsolate(), effect_options, "composite",
                              "add");
  KeyframeEffectOptions effect_options_dictionary;
  V8KeyframeEffectOptions::ToImpl(scope.GetIsolate(), effect_options,
                                  effect_options_dictionary, exception_state);
  EXPECT_FALSE(exception_state.HadException());

  ScriptValue js_keyframes = ScriptValue::CreateNull(script_state);
  KeyframeEffect* effect = CreateAnimation(
      script_state, element.Get(), js_keyframes, effect_options_dictionary);
  EXPECT_EQ("add", effect->composite());

  effect->setComposite("replace");
  EXPECT_EQ("replace", effect->composite());

  // TODO(crbug.com/788440): Once accumulate is supported as a composite
  // property, setting it here should work.
  effect->setComposite("accumulate");
  EXPECT_EQ("replace", effect->composite());
}

TEST_F(AnimationKeyframeEffectV8Test, KeyframeCompositeOverridesEffect) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();
  NonThrowableExceptionState exception_state;

  v8::Local<v8::Object> effect_options = v8::Object::New(scope.GetIsolate());
  SetV8ObjectPropertyAsString(scope.GetIsolate(), effect_options, "composite",
                              "add");
  KeyframeEffectOptions effect_options_dictionary;
  V8KeyframeEffectOptions::ToImpl(scope.GetIsolate(), effect_options,
                                  effect_options_dictionary, exception_state);
  EXPECT_FALSE(exception_state.HadException());

  Vector<ScriptValue> blink_keyframes = {
      V8ObjectBuilder(script_state)
          .AddString("width", "100px")
          .AddString("composite", "replace")
          .GetScriptValue(),
      V8ObjectBuilder(script_state).AddString("width", "0px").GetScriptValue()};

  ScriptValue js_keyframes(
      script_state,
      ToV8(blink_keyframes, scope.GetContext()->Global(), scope.GetIsolate()));

  KeyframeEffect* effect = CreateAnimation(
      script_state, element.Get(), js_keyframes, effect_options_dictionary);
  EXPECT_EQ("add", effect->composite());

  PropertyHandle property(GetCSSPropertyWidth());
  const PropertySpecificKeyframeVector& keyframes =
      effect->Model()->GetPropertySpecificKeyframes(property);

  EXPECT_EQ(EffectModel::kCompositeReplace, keyframes[0]->Composite());
  EXPECT_EQ(EffectModel::kCompositeAdd, keyframes[1]->Composite());
}

TEST_F(AnimationKeyframeEffectV8Test, CanSetDuration) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();
  ScriptValue js_keyframes = ScriptValue::CreateNull(script_state);
  double duration = 2000;

  KeyframeEffect* animation =
      CreateAnimation(script_state, element.Get(), js_keyframes, duration);

  EXPECT_EQ(duration / 1000, animation->SpecifiedTiming().iteration_duration);
}

TEST_F(AnimationKeyframeEffectV8Test, CanOmitSpecifiedDuration) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();
  ScriptValue js_keyframes = ScriptValue::CreateNull(script_state);
  KeyframeEffect* animation =
      CreateAnimation(script_state, element.Get(), js_keyframes);
  EXPECT_TRUE(std::isnan(animation->SpecifiedTiming().iteration_duration));
}

TEST_F(AnimationKeyframeEffectV8Test, SpecifiedGetters) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();
  ScriptValue js_keyframes = ScriptValue::CreateNull(script_state);

  v8::Local<v8::Object> timing_input = v8::Object::New(scope.GetIsolate());
  SetV8ObjectPropertyAsNumber(scope.GetIsolate(), timing_input, "delay", 2);
  SetV8ObjectPropertyAsNumber(scope.GetIsolate(), timing_input, "endDelay",
                              0.5);
  SetV8ObjectPropertyAsString(scope.GetIsolate(), timing_input, "fill",
                              "backwards");
  SetV8ObjectPropertyAsNumber(scope.GetIsolate(), timing_input,
                              "iterationStart", 2);
  SetV8ObjectPropertyAsNumber(scope.GetIsolate(), timing_input, "iterations",
                              10);
  SetV8ObjectPropertyAsString(scope.GetIsolate(), timing_input, "direction",
                              "reverse");
  SetV8ObjectPropertyAsString(scope.GetIsolate(), timing_input, "easing",
                              "ease-in-out");
  KeyframeEffectOptions timing_input_dictionary;
  DummyExceptionStateForTesting exception_state;
  V8KeyframeEffectOptions::ToImpl(scope.GetIsolate(), timing_input,
                                  timing_input_dictionary, exception_state);
  EXPECT_FALSE(exception_state.HadException());

  KeyframeEffect* animation = CreateAnimation(
      script_state, element.Get(), js_keyframes, timing_input_dictionary);

  EffectTiming timing;
  animation->getTiming(timing);
  EXPECT_EQ(2, timing.delay());
  EXPECT_EQ(0.5, timing.endDelay());
  EXPECT_EQ("backwards", timing.fill());
  EXPECT_EQ(2, timing.iterationStart());
  EXPECT_EQ(10, timing.iterations());
  EXPECT_EQ("reverse", timing.direction());
  EXPECT_EQ("ease-in-out", timing.easing());
}

TEST_F(AnimationKeyframeEffectV8Test, SpecifiedDurationGetter) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();
  ScriptValue js_keyframes = ScriptValue::CreateNull(script_state);

  v8::Local<v8::Object> timing_input_with_duration =
      v8::Object::New(scope.GetIsolate());
  SetV8ObjectPropertyAsNumber(scope.GetIsolate(), timing_input_with_duration,
                              "duration", 2.5);
  KeyframeEffectOptions timing_input_dictionary_with_duration;
  DummyExceptionStateForTesting exception_state;
  V8KeyframeEffectOptions::ToImpl(
      scope.GetIsolate(), timing_input_with_duration,
      timing_input_dictionary_with_duration, exception_state);
  EXPECT_FALSE(exception_state.HadException());

  KeyframeEffect* animation_with_duration =
      CreateAnimation(script_state, element.Get(), js_keyframes,
                      timing_input_dictionary_with_duration);

  EffectTiming specified_with_duration;
  animation_with_duration->getTiming(specified_with_duration);
  UnrestrictedDoubleOrString duration = specified_with_duration.duration();
  EXPECT_TRUE(duration.IsUnrestrictedDouble());
  EXPECT_EQ(2.5, duration.GetAsUnrestrictedDouble());
  EXPECT_FALSE(duration.IsString());

  v8::Local<v8::Object> timing_input_no_duration =
      v8::Object::New(scope.GetIsolate());
  KeyframeEffectOptions timing_input_dictionary_no_duration;
  V8KeyframeEffectOptions::ToImpl(scope.GetIsolate(), timing_input_no_duration,
                                  timing_input_dictionary_no_duration,
                                  exception_state);
  EXPECT_FALSE(exception_state.HadException());

  KeyframeEffect* animation_no_duration =
      CreateAnimation(script_state, element.Get(), js_keyframes,
                      timing_input_dictionary_no_duration);

  EffectTiming specified_no_duration;
  animation_no_duration->getTiming(specified_no_duration);
  UnrestrictedDoubleOrString duration2 = specified_no_duration.duration();
  EXPECT_FALSE(duration2.IsUnrestrictedDouble());
  EXPECT_TRUE(duration2.IsString());
  EXPECT_EQ("auto", duration2.GetAsString());
}

TEST_F(AnimationKeyframeEffectV8Test, SetKeyframesAdditiveCompositeOperation) {
  ScopedCSSAdditiveAnimationsForTest css_additive_animation(false);
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();
  ScriptValue js_keyframes = ScriptValue::CreateNull(script_state);
  v8::Local<v8::Object> timing_input = v8::Object::New(scope.GetIsolate());
  KeyframeEffectOptions timing_input_dictionary;
  DummyExceptionStateForTesting exception_state;
  V8KeyframeEffectOptions::ToImpl(scope.GetIsolate(), timing_input,
                                  timing_input_dictionary, exception_state);
  ASSERT_FALSE(exception_state.HadException());

  // Since there are no CSS-targeting keyframes, we can create a KeyframeEffect
  // with composite = 'add'.
  timing_input_dictionary.setComposite("add");
  KeyframeEffect* effect = CreateAnimation(
      script_state, element.Get(), js_keyframes, timing_input_dictionary);
  EXPECT_EQ(effect->Model()->Composite(), EffectModel::kCompositeAdd);

  // But if we then setKeyframes with CSS-targeting keyframes, the composite
  // should fallback to 'replace'.
  Vector<ScriptValue> blink_keyframes = {
      V8ObjectBuilder(script_state).AddString("width", "10px").GetScriptValue(),
      V8ObjectBuilder(script_state).AddString("width", "0px").GetScriptValue()};
  ScriptValue new_js_keyframes(
      script_state,
      ToV8(blink_keyframes, scope.GetContext()->Global(), scope.GetIsolate()));
  effect->setKeyframes(script_state, new_js_keyframes, exception_state);
  ASSERT_FALSE(exception_state.HadException());
  EXPECT_EQ(effect->Model()->Composite(), EffectModel::kCompositeReplace);
}

TEST_F(KeyframeEffectTest, TimeToEffectChange) {
  Timing timing;
  timing.iteration_duration = 100;
  timing.start_delay = 100;
  timing.end_delay = 100;
  timing.fill_mode = Timing::FillMode::NONE;
  KeyframeEffect* keyframe_effect =
      KeyframeEffect::Create(nullptr, CreateEmptyEffectModel(), timing);
  Animation* animation = GetDocument().Timeline().Play(keyframe_effect);
  double inf = std::numeric_limits<double>::infinity();

  EXPECT_EQ(100, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(inf, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(100);
  EXPECT_EQ(100, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(0, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(199);
  EXPECT_EQ(1, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(0, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(200);
  // End-exclusive.
  EXPECT_EQ(inf, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(0, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(300);
  EXPECT_EQ(inf, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(100, keyframe_effect->TimeToReverseEffectChange());
}

TEST_F(KeyframeEffectTest, TimeToEffectChangeWithPlaybackRate) {
  Timing timing;
  timing.iteration_duration = 100;
  timing.start_delay = 100;
  timing.end_delay = 100;
  timing.playback_rate = 2;
  timing.fill_mode = Timing::FillMode::NONE;
  KeyframeEffect* keyframe_effect =
      KeyframeEffect::Create(nullptr, CreateEmptyEffectModel(), timing);
  Animation* animation = GetDocument().Timeline().Play(keyframe_effect);
  double inf = std::numeric_limits<double>::infinity();

  EXPECT_EQ(100, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(inf, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(100);
  EXPECT_EQ(50, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(0, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(149);
  EXPECT_EQ(1, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(0, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(150);
  // End-exclusive.
  EXPECT_EQ(inf, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(0, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(200);
  EXPECT_EQ(inf, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(50, keyframe_effect->TimeToReverseEffectChange());
}

TEST_F(KeyframeEffectTest, TimeToEffectChangeWithNegativePlaybackRate) {
  Timing timing;
  timing.iteration_duration = 100;
  timing.start_delay = 100;
  timing.end_delay = 100;
  timing.playback_rate = -2;
  timing.fill_mode = Timing::FillMode::NONE;
  KeyframeEffect* keyframe_effect =
      KeyframeEffect::Create(nullptr, CreateEmptyEffectModel(), timing);
  Animation* animation = GetDocument().Timeline().Play(keyframe_effect);
  double inf = std::numeric_limits<double>::infinity();

  EXPECT_EQ(100, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(inf, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(100);
  EXPECT_EQ(50, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(0, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(149);
  EXPECT_EQ(1, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(0, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(150);
  EXPECT_EQ(inf, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(0, keyframe_effect->TimeToReverseEffectChange());

  animation->SetCurrentTimeInternal(200);
  EXPECT_EQ(inf, keyframe_effect->TimeToForwardsEffectChange());
  EXPECT_EQ(50, keyframe_effect->TimeToReverseEffectChange());
}

}  // namespace blink
