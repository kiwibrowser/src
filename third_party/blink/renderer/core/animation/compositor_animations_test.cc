/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/animation/compositor_animations.h"

#include <limits>
#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/memory/scoped_refptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/renderer/core/animation/animatable/animatable_double.h"
#include "third_party/blink/renderer/core/animation/animatable/animatable_filter_operations.h"
#include "third_party/blink/renderer/core/animation/animatable/animatable_transform.h"
#include "third_party/blink/renderer/core/animation/animation.h"
#include "third_party/blink/renderer/core/animation/document_timeline.h"
#include "third_party/blink/renderer/core/animation/element_animations.h"
#include "third_party/blink/renderer/core/animation/keyframe_effect.h"
#include "third_party/blink/renderer/core/animation/pending_animations.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/frame_test_helpers.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/paint/object_paint_properties.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/style/filter_operations.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/animation/compositor_animation_host.h"
#include "third_party/blink/renderer/platform/animation/compositor_float_animation_curve.h"
#include "third_party/blink/renderer/platform/animation/compositor_float_keyframe.h"
#include "third_party/blink/renderer/platform/animation/compositor_keyframe_model.h"
#include "third_party/blink/renderer/platform/geometry/float_box.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/testing/histogram_tester.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"
#include "third_party/blink/renderer/platform/transforms/transform_operations.h"
#include "third_party/blink/renderer/platform/transforms/translate_transform_operation.h"
#include "third_party/blink/renderer/platform/wtf/hash_functions.h"

namespace blink {

class AnimationCompositorAnimationsTest : public RenderingTest {
 protected:
  scoped_refptr<TimingFunction> linear_timing_function_;
  scoped_refptr<TimingFunction> cubic_ease_timing_function_;
  scoped_refptr<TimingFunction> cubic_custom_timing_function_;
  scoped_refptr<TimingFunction> step_timing_function_;
  scoped_refptr<TimingFunction> frames_timing_function_;

  Timing timing_;
  CompositorAnimations::CompositorTiming compositor_timing_;
  std::unique_ptr<StringKeyframeVector> keyframe_vector2_;
  Persistent<StringKeyframeEffectModel> keyframe_animation_effect2_;
  std::unique_ptr<StringKeyframeVector> keyframe_vector5_;
  Persistent<StringKeyframeEffectModel> keyframe_animation_effect5_;

  Persistent<Element> element_;
  Persistent<DocumentTimeline> timeline_;

  void SetUp() override {
    RenderingTest::SetUp();
    EnableCompositing();
    linear_timing_function_ = LinearTimingFunction::Shared();
    cubic_ease_timing_function_ = CubicBezierTimingFunction::Preset(
        CubicBezierTimingFunction::EaseType::EASE);
    cubic_custom_timing_function_ =
        CubicBezierTimingFunction::Create(1, 2, 3, 4);
    step_timing_function_ =
        StepsTimingFunction::Create(1, StepsTimingFunction::StepPosition::END);
    frames_timing_function_ = FramesTimingFunction::Create(2);

    timing_ = CreateCompositableTiming();
    compositor_timing_ = CompositorAnimations::CompositorTiming();
    // Make sure the CompositableTiming is really compositable, otherwise
    // most other tests will fail.
    ASSERT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));

    keyframe_vector2_ = CreateCompositableFloatKeyframeVector(2);
    keyframe_animation_effect2_ =
        StringKeyframeEffectModel::Create(*keyframe_vector2_);

    keyframe_vector5_ = CreateCompositableFloatKeyframeVector(5);
    keyframe_animation_effect5_ =
        StringKeyframeEffectModel::Create(*keyframe_vector5_);

    GetAnimationClock().ResetTimeForTesting();

    timeline_ = DocumentTimeline::Create(&GetDocument());
    timeline_->ResetForTesting();
    element_ = GetDocument().CreateElementForBinding("test");

    helper_.Initialize(nullptr, nullptr, nullptr);
    base_url_ = "http://www.test.com/";
  }

 public:
  bool ConvertTimingForCompositor(const Timing& t,
                                  CompositorAnimations::CompositorTiming& out) {
    return CompositorAnimations::ConvertTimingForCompositor(t, 0, out, 1);
  }
  bool CanStartEffectOnCompositor(const Timing& timing,
                                  const KeyframeEffectModelBase& effect) {
    // As the compositor code only understands AnimatableValues, we must
    // snapshot the effect to make those available.
    // TODO(crbug.com/725385): Remove once compositor uses InterpolationTypes.
    auto style = ComputedStyle::Create();
    effect.SnapshotAllCompositorKeyframesIfNecessary(*element_.Get(), *style,
                                                     nullptr);
    return CompositorAnimations::CheckCanStartEffectOnCompositor(
               timing, *element_.Get(), nullptr, effect, 1)
        .Ok();
  }
  void GetAnimationOnCompositor(
      Timing& timing,
      StringKeyframeEffectModel& effect,
      Vector<std::unique_ptr<CompositorKeyframeModel>>& keyframe_models) {
    GetAnimationOnCompositor(timing, effect, keyframe_models, 1);
  }
  void GetAnimationOnCompositor(
      Timing& timing,
      StringKeyframeEffectModel& effect,
      Vector<std::unique_ptr<CompositorKeyframeModel>>& keyframe_models,
      double animation_playback_rate) {
    CompositorAnimations::GetAnimationOnCompositor(timing, 0, base::nullopt, 0,
                                                   effect, keyframe_models,
                                                   animation_playback_rate);
  }

  bool DuplicateSingleKeyframeAndTestIsCandidateOnResult(
      StringKeyframe* frame) {
    EXPECT_EQ(frame->CheckedOffset(), 0);
    StringKeyframeVector frames;
    scoped_refptr<Keyframe> second = frame->CloneWithOffset(1);

    frames.push_back(frame);
    frames.push_back(ToStringKeyframe(second.get()));
    return CanStartEffectOnCompositor(
        timing_, *StringKeyframeEffectModel::Create(frames));
  }

  // -------------------------------------------------------------------

  Timing CreateCompositableTiming() {
    Timing timing;
    timing.start_delay = 0;
    timing.fill_mode = Timing::FillMode::NONE;
    timing.iteration_start = 0;
    timing.iteration_count = 1;
    timing.iteration_duration = 1.0;
    timing.playback_rate = 1.0;
    timing.direction = Timing::PlaybackDirection::NORMAL;
    timing.timing_function = linear_timing_function_;
    return timing;
  }

  scoped_refptr<StringKeyframe> CreateReplaceOpKeyframe(CSSPropertyID id,
                                                        const String& value,
                                                        double offset = 0) {
    scoped_refptr<StringKeyframe> keyframe = StringKeyframe::Create();
    keyframe->SetCSSPropertyValue(id, value,
                                  SecureContextMode::kInsecureContext, nullptr);
    keyframe->SetComposite(EffectModel::kCompositeReplace);
    keyframe->SetOffset(offset);
    keyframe->SetEasing(LinearTimingFunction::Shared());
    return keyframe;
  }

  scoped_refptr<StringKeyframe> CreateDefaultKeyframe(
      CSSPropertyID id,
      EffectModel::CompositeOperation op,
      double offset = 0) {
    String value = "0.1";
    if (id == CSSPropertyTransform)
      value = "none";  // AnimatableTransform::Create(TransformOperations(), 1);
    else if (id == CSSPropertyColor)
      value = "red";

    scoped_refptr<StringKeyframe> keyframe =
        CreateReplaceOpKeyframe(id, value, offset);
    keyframe->SetComposite(op);
    return keyframe;
  }

  std::unique_ptr<StringKeyframeVector> CreateCompositableFloatKeyframeVector(
      size_t n) {
    Vector<double> values;
    for (size_t i = 0; i < n; i++) {
      values.push_back(static_cast<double>(i));
    }
    return CreateCompositableFloatKeyframeVector(values);
  }

  std::unique_ptr<StringKeyframeVector> CreateCompositableFloatKeyframeVector(
      Vector<double>& values) {
    std::unique_ptr<StringKeyframeVector> frames =
        base::WrapUnique(new StringKeyframeVector);
    for (size_t i = 0; i < values.size(); i++) {
      double offset = 1.0 / (values.size() - 1) * i;
      String value = String::Number(values[i]);
      frames->push_back(
          CreateReplaceOpKeyframe(CSSPropertyOpacity, value, offset).get());
    }
    return frames;
  }

  std::unique_ptr<StringKeyframeVector>
  CreateCompositableTransformKeyframeVector(const Vector<String>& values) {
    std::unique_ptr<StringKeyframeVector> frames =
        base::WrapUnique(new StringKeyframeVector);
    for (size_t i = 0; i < values.size(); ++i) {
      double offset = 1.0f / (values.size() - 1) * i;
      frames->push_back(
          CreateReplaceOpKeyframe(CSSPropertyTransform, values[i], offset)
              .get());
    }
    return frames;
  }

  StringKeyframeEffectModel* CreateKeyframeEffectModel(
      scoped_refptr<StringKeyframe> from,
      scoped_refptr<StringKeyframe> to,
      scoped_refptr<StringKeyframe> c = nullptr,
      scoped_refptr<StringKeyframe> d = nullptr) {
    EXPECT_EQ(from->CheckedOffset(), 0);
    StringKeyframeVector frames;
    frames.push_back(from);
    EXPECT_LE(from->Offset(), to->Offset());
    frames.push_back(to);
    if (c) {
      EXPECT_LE(to->Offset(), c->Offset());
      frames.push_back(c);
    }
    if (d) {
      frames.push_back(d);
      EXPECT_LE(c->Offset(), d->Offset());
      EXPECT_EQ(d->CheckedOffset(), 1.0);
    } else {
      EXPECT_EQ(to->CheckedOffset(), 1.0);
    }
    if (!HasFatalFailure()) {
      return StringKeyframeEffectModel::Create(frames);
    }
    return nullptr;
  }

  void SimulateFrame(double time) {
    GetAnimationClock().UpdateTime(base::TimeTicks() +
                                   base::TimeDelta::FromSecondsD(time));
    GetPendingAnimations().Update(base::Optional<CompositorElementIdSet>(),
                                  false);
    timeline_->ServiceAnimations(kTimingUpdateForAnimationFrame);
  }

  std::unique_ptr<CompositorKeyframeModel> ConvertToCompositorAnimation(
      StringKeyframeEffectModel& effect,
      double animation_playback_rate) {
    // As the compositor code only understands AnimatableValues, we must
    // snapshot the effect to make those available.
    // TODO(crbug.com/725385): Remove once compositor uses InterpolationTypes.
    auto style = ComputedStyle::Create();
    effect.SnapshotAllCompositorKeyframesIfNecessary(*element_.Get(), *style,
                                                     nullptr);

    Vector<std::unique_ptr<CompositorKeyframeModel>> result;
    GetAnimationOnCompositor(timing_, effect, result, animation_playback_rate);
    DCHECK_EQ(1U, result.size());
    return std::move(result[0]);
  }

  std::unique_ptr<CompositorKeyframeModel> ConvertToCompositorAnimation(
      StringKeyframeEffectModel& effect) {
    return ConvertToCompositorAnimation(effect, 1.0);
  }

  void ExpectKeyframeTimingFunctionCubic(
      const CompositorFloatKeyframe& keyframe,
      const CubicBezierTimingFunction::EaseType ease_type) {
    auto keyframe_timing_function = keyframe.GetTimingFunctionForTesting();
    DCHECK_EQ(keyframe_timing_function->GetType(),
              TimingFunction::Type::CUBIC_BEZIER);
    const auto& cubic_timing_function =
        ToCubicBezierTimingFunction(*keyframe_timing_function);
    EXPECT_EQ(cubic_timing_function.GetEaseType(), ease_type);
  }

  void LoadTestData(const std::string& file_name) {
    String testing_path = test::BlinkRootDir();
    testing_path.append("/renderer/core/animation/test_data/");
    WebURL url = URLTestHelpers::RegisterMockedURLLoadFromBase(
        WebString::FromUTF8(base_url_), testing_path,
        WebString::FromUTF8(file_name));
    FrameTestHelpers::LoadFrame(helper_.GetWebView()->MainFrameImpl(),
                                base_url_ + file_name);
    ForceFullCompositingUpdate();
    URLTestHelpers::RegisterMockedURLUnregister(url);
  }

  LocalFrame* GetFrame() const { return helper_.LocalMainFrame()->GetFrame(); }

  void BeginFrame() {
    helper_.GetWebView()->BeginFrame(WTF::CurrentTimeTicks());
  }

  void ForceFullCompositingUpdate() {
    helper_.GetWebView()->UpdateAllLifecyclePhases();
  }

 private:
  FrameTestHelpers::WebViewHelper helper_;
  std::string base_url_;
};

class LayoutObjectProxy : public LayoutObject {
 public:
  static LayoutObjectProxy* Create(Node* node) {
    return new LayoutObjectProxy(node);
  }

  static void Dispose(LayoutObjectProxy* proxy) { proxy->Destroy(); }

  const char* GetName() const override { return nullptr; }
  void UpdateLayout() override {}
  FloatRect LocalBoundingBoxRectForAccessibility() const override {
    return FloatRect();
  }

 private:
  explicit LayoutObjectProxy(Node* node) : LayoutObject(node) {}
};

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorKeyframeMultipleCSSProperties) {
  scoped_refptr<StringKeyframe> keyframe_good_multiple =
      CreateDefaultKeyframe(CSSPropertyOpacity, EffectModel::kCompositeReplace);
  keyframe_good_multiple->SetCSSPropertyValue(
      CSSPropertyTransform, "none", SecureContextMode::kInsecureContext,
      nullptr);
  EXPECT_TRUE(DuplicateSingleKeyframeAndTestIsCandidateOnResult(
      keyframe_good_multiple.get()));

  scoped_refptr<StringKeyframe> keyframe_bad_multiple_id =
      CreateDefaultKeyframe(CSSPropertyColor, EffectModel::kCompositeReplace);
  keyframe_bad_multiple_id->SetCSSPropertyValue(
      CSSPropertyOpacity, "0.1", SecureContextMode::kInsecureContext, nullptr);
  EXPECT_FALSE(DuplicateSingleKeyframeAndTestIsCandidateOnResult(
      keyframe_bad_multiple_id.get()));
}

TEST_F(AnimationCompositorAnimationsTest,
       isNotCandidateForCompositorAnimationTransformDependsOnBoxSize) {
  // Absolute transforms can be animated on the compositor.
  String transform = "translateX(2px) translateY(2px)";
  scoped_refptr<StringKeyframe> good_keyframe =
      CreateReplaceOpKeyframe(CSSPropertyTransform, transform);
  EXPECT_TRUE(
      DuplicateSingleKeyframeAndTestIsCandidateOnResult(good_keyframe.get()));

  // Transforms that rely on the box size, such as percent calculations, cannot
  // be animated on the compositor (as the box size may change).
  String transform2 = "translateX(50%) translateY(2px)";
  scoped_refptr<StringKeyframe> bad_keyframe =
      CreateReplaceOpKeyframe(CSSPropertyTransform, transform2);
  EXPECT_FALSE(
      DuplicateSingleKeyframeAndTestIsCandidateOnResult(bad_keyframe.get()));

  // Similarly, calc transforms cannot be animated on the compositor.
  String transform3 = "translateX(calc(100% + (0.5 * 100px)))";
  scoped_refptr<StringKeyframe> bad_keyframe2 =
      CreateReplaceOpKeyframe(CSSPropertyTransform, transform3);
  EXPECT_FALSE(
      DuplicateSingleKeyframeAndTestIsCandidateOnResult(bad_keyframe2.get()));
}

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorKeyframeEffectModel) {
  StringKeyframeVector frames_same;
  frames_same.push_back(CreateDefaultKeyframe(CSSPropertyColor,
                                              EffectModel::kCompositeReplace,
                                              0.0)
                            .get());
  frames_same.push_back(CreateDefaultKeyframe(CSSPropertyColor,
                                              EffectModel::kCompositeReplace,
                                              1.0)
                            .get());
  EXPECT_FALSE(CanStartEffectOnCompositor(
      timing_, *StringKeyframeEffectModel::Create(frames_same)));

  StringKeyframeVector frames_mixed_properties;
  scoped_refptr<StringKeyframe> keyframe = StringKeyframe::Create();
  keyframe->SetOffset(0);
  keyframe->SetCSSPropertyValue(CSSPropertyColor, "red",
                                SecureContextMode::kInsecureContext, nullptr);
  keyframe->SetCSSPropertyValue(CSSPropertyOpacity, "0",
                                SecureContextMode::kInsecureContext, nullptr);
  frames_mixed_properties.push_back(std::move(keyframe));
  keyframe = StringKeyframe::Create();
  keyframe->SetOffset(1);
  keyframe->SetCSSPropertyValue(CSSPropertyColor, "green",
                                SecureContextMode::kInsecureContext, nullptr);
  keyframe->SetCSSPropertyValue(CSSPropertyOpacity, "1",
                                SecureContextMode::kInsecureContext, nullptr);
  frames_mixed_properties.push_back(std::move(keyframe));
  EXPECT_FALSE(CanStartEffectOnCompositor(
      timing_, *StringKeyframeEffectModel::Create(frames_mixed_properties)));
}

TEST_F(AnimationCompositorAnimationsTest,
       ConvertTimingForCompositorStartDelay) {
  timing_.iteration_duration = 20.0;

  timing_.start_delay = 2.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(-2.0, compositor_timing_.scaled_time_offset);

  timing_.start_delay = -2.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(2.0, compositor_timing_.scaled_time_offset);
}

TEST_F(AnimationCompositorAnimationsTest,
       ConvertTimingForCompositorIterationStart) {
  timing_.iteration_start = 2.2;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
}

TEST_F(AnimationCompositorAnimationsTest,
       ConvertTimingForCompositorIterationCount) {
  timing_.iteration_count = 5.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_EQ(5, compositor_timing_.adjusted_iteration_count);

  timing_.iteration_count = 5.5;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_EQ(5.5, compositor_timing_.adjusted_iteration_count);

  timing_.iteration_count = std::numeric_limits<double>::infinity();
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_EQ(-1, compositor_timing_.adjusted_iteration_count);

  timing_.iteration_count = std::numeric_limits<double>::infinity();
  timing_.iteration_duration = 5.0;
  timing_.start_delay = -6.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(6.0, compositor_timing_.scaled_time_offset);
  EXPECT_EQ(-1, compositor_timing_.adjusted_iteration_count);
}

TEST_F(AnimationCompositorAnimationsTest,
       ConvertTimingForCompositorIterationsAndStartDelay) {
  timing_.iteration_count = 4.0;
  timing_.iteration_duration = 5.0;

  timing_.start_delay = 6.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(-6.0, compositor_timing_.scaled_time_offset);
  EXPECT_DOUBLE_EQ(4.0, compositor_timing_.adjusted_iteration_count);

  timing_.start_delay = -6.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(6.0, compositor_timing_.scaled_time_offset);
  EXPECT_DOUBLE_EQ(4.0, compositor_timing_.adjusted_iteration_count);

  timing_.start_delay = 21.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
}

TEST_F(AnimationCompositorAnimationsTest,
       ConvertTimingForCompositorPlaybackRate) {
  timing_.playback_rate = 1.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(1.0, compositor_timing_.playback_rate);

  timing_.playback_rate = -2.3;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(-2.3, compositor_timing_.playback_rate);

  timing_.playback_rate = 1.6;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(1.6, compositor_timing_.playback_rate);
}

TEST_F(AnimationCompositorAnimationsTest, ConvertTimingForCompositorDirection) {
  timing_.direction = Timing::PlaybackDirection::NORMAL;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_EQ(compositor_timing_.direction, Timing::PlaybackDirection::NORMAL);

  timing_.direction = Timing::PlaybackDirection::ALTERNATE_NORMAL;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_EQ(compositor_timing_.direction,
            Timing::PlaybackDirection::ALTERNATE_NORMAL);

  timing_.direction = Timing::PlaybackDirection::ALTERNATE_REVERSE;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_EQ(compositor_timing_.direction,
            Timing::PlaybackDirection::ALTERNATE_REVERSE);

  timing_.direction = Timing::PlaybackDirection::REVERSE;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_EQ(compositor_timing_.direction, Timing::PlaybackDirection::REVERSE);
}

TEST_F(AnimationCompositorAnimationsTest,
       ConvertTimingForCompositorDirectionIterationsAndStartDelay) {
  timing_.direction = Timing::PlaybackDirection::ALTERNATE_NORMAL;
  timing_.iteration_count = 4.0;
  timing_.iteration_duration = 5.0;
  timing_.start_delay = -6.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(6.0, compositor_timing_.scaled_time_offset);
  EXPECT_EQ(4, compositor_timing_.adjusted_iteration_count);
  EXPECT_EQ(compositor_timing_.direction,
            Timing::PlaybackDirection::ALTERNATE_NORMAL);

  timing_.direction = Timing::PlaybackDirection::ALTERNATE_NORMAL;
  timing_.iteration_count = 4.0;
  timing_.iteration_duration = 5.0;
  timing_.start_delay = -11.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(11.0, compositor_timing_.scaled_time_offset);
  EXPECT_EQ(4, compositor_timing_.adjusted_iteration_count);
  EXPECT_EQ(compositor_timing_.direction,
            Timing::PlaybackDirection::ALTERNATE_NORMAL);

  timing_.direction = Timing::PlaybackDirection::ALTERNATE_REVERSE;
  timing_.iteration_count = 4.0;
  timing_.iteration_duration = 5.0;
  timing_.start_delay = -6.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(6.0, compositor_timing_.scaled_time_offset);
  EXPECT_EQ(4, compositor_timing_.adjusted_iteration_count);
  EXPECT_EQ(compositor_timing_.direction,
            Timing::PlaybackDirection::ALTERNATE_REVERSE);

  timing_.direction = Timing::PlaybackDirection::ALTERNATE_REVERSE;
  timing_.iteration_count = 4.0;
  timing_.iteration_duration = 5.0;
  timing_.start_delay = -11.0;
  EXPECT_TRUE(ConvertTimingForCompositor(timing_, compositor_timing_));
  EXPECT_DOUBLE_EQ(11.0, compositor_timing_.scaled_time_offset);
  EXPECT_EQ(4, compositor_timing_.adjusted_iteration_count);
  EXPECT_EQ(compositor_timing_.direction,
            Timing::PlaybackDirection::ALTERNATE_REVERSE);
}

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorTimingFunctionLinear) {
  timing_.timing_function = linear_timing_function_;
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));
}

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorTimingFunctionCubic) {
  timing_.timing_function = cubic_ease_timing_function_;
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));

  timing_.timing_function = cubic_custom_timing_function_;
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));
}

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorTimingFunctionSteps) {
  timing_.timing_function = step_timing_function_;
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));
}

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorTimingFunctionFrames) {
  timing_.timing_function = frames_timing_function_;
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));
}

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorTimingFunctionChainedLinear) {
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));
}

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorNonLinearTimingFunctionOnFirstOrLastFrame) {
  (*keyframe_vector2_)[0]->SetEasing(cubic_ease_timing_function_.get());
  keyframe_animation_effect2_ =
      StringKeyframeEffectModel::Create(*keyframe_vector2_);

  (*keyframe_vector5_)[3]->SetEasing(cubic_ease_timing_function_.get());
  keyframe_animation_effect5_ =
      StringKeyframeEffectModel::Create(*keyframe_vector5_);

  timing_.timing_function = cubic_ease_timing_function_;
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));

  timing_.timing_function = cubic_custom_timing_function_;
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));
}

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorTimingFunctionChainedCubicMatchingOffsets) {
  (*keyframe_vector2_)[0]->SetEasing(cubic_ease_timing_function_.get());
  keyframe_animation_effect2_ =
      StringKeyframeEffectModel::Create(*keyframe_vector2_);
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));

  (*keyframe_vector2_)[0]->SetEasing(cubic_custom_timing_function_.get());
  keyframe_animation_effect2_ =
      StringKeyframeEffectModel::Create(*keyframe_vector2_);
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));

  (*keyframe_vector5_)[0]->SetEasing(cubic_ease_timing_function_.get());
  (*keyframe_vector5_)[1]->SetEasing(cubic_custom_timing_function_.get());
  (*keyframe_vector5_)[2]->SetEasing(cubic_custom_timing_function_.get());
  (*keyframe_vector5_)[3]->SetEasing(cubic_custom_timing_function_.get());
  keyframe_animation_effect5_ =
      StringKeyframeEffectModel::Create(*keyframe_vector5_);
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));
}

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorTimingFunctionMixedGood) {
  (*keyframe_vector5_)[0]->SetEasing(linear_timing_function_.get());
  (*keyframe_vector5_)[1]->SetEasing(cubic_ease_timing_function_.get());
  (*keyframe_vector5_)[2]->SetEasing(cubic_ease_timing_function_.get());
  (*keyframe_vector5_)[3]->SetEasing(linear_timing_function_.get());
  keyframe_animation_effect5_ =
      StringKeyframeEffectModel::Create(*keyframe_vector5_);
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));
}

TEST_F(AnimationCompositorAnimationsTest,
       CanStartEffectOnCompositorTimingFunctionWithStepOrFrameOkay) {
  (*keyframe_vector2_)[0]->SetEasing(step_timing_function_.get());
  keyframe_animation_effect2_ =
      StringKeyframeEffectModel::Create(*keyframe_vector2_);
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));

  (*keyframe_vector2_)[0]->SetEasing(frames_timing_function_.get());
  keyframe_animation_effect2_ =
      StringKeyframeEffectModel::Create(*keyframe_vector2_);
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect2_));

  (*keyframe_vector5_)[0]->SetEasing(step_timing_function_.get());
  (*keyframe_vector5_)[1]->SetEasing(linear_timing_function_.get());
  (*keyframe_vector5_)[2]->SetEasing(cubic_ease_timing_function_.get());
  (*keyframe_vector5_)[3]->SetEasing(frames_timing_function_.get());
  keyframe_animation_effect5_ =
      StringKeyframeEffectModel::Create(*keyframe_vector5_);
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));

  (*keyframe_vector5_)[0]->SetEasing(frames_timing_function_.get());
  (*keyframe_vector5_)[1]->SetEasing(step_timing_function_.get());
  (*keyframe_vector5_)[2]->SetEasing(cubic_ease_timing_function_.get());
  (*keyframe_vector5_)[3]->SetEasing(linear_timing_function_.get());
  keyframe_animation_effect5_ =
      StringKeyframeEffectModel::Create(*keyframe_vector5_);
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));

  (*keyframe_vector5_)[0]->SetEasing(linear_timing_function_.get());
  (*keyframe_vector5_)[1]->SetEasing(frames_timing_function_.get());
  (*keyframe_vector5_)[2]->SetEasing(cubic_ease_timing_function_.get());
  (*keyframe_vector5_)[3]->SetEasing(step_timing_function_.get());
  keyframe_animation_effect5_ =
      StringKeyframeEffectModel::Create(*keyframe_vector5_);
  EXPECT_TRUE(
      CanStartEffectOnCompositor(timing_, *keyframe_animation_effect5_));
}

TEST_F(AnimationCompositorAnimationsTest, CanStartEffectOnCompositor) {
  StringKeyframeVector basic_frames_vector;
  basic_frames_vector.push_back(
      CreateDefaultKeyframe(CSSPropertyOpacity, EffectModel::kCompositeReplace,
                            0.0)
          .get());
  basic_frames_vector.push_back(
      CreateDefaultKeyframe(CSSPropertyOpacity, EffectModel::kCompositeReplace,
                            1.0)
          .get());

  StringKeyframeVector non_basic_frames_vector;
  non_basic_frames_vector.push_back(
      CreateDefaultKeyframe(CSSPropertyOpacity, EffectModel::kCompositeReplace,
                            0.0)
          .get());
  non_basic_frames_vector.push_back(
      CreateDefaultKeyframe(CSSPropertyOpacity, EffectModel::kCompositeReplace,
                            0.5)
          .get());
  non_basic_frames_vector.push_back(
      CreateDefaultKeyframe(CSSPropertyOpacity, EffectModel::kCompositeReplace,
                            1.0)
          .get());

  basic_frames_vector[0]->SetEasing(linear_timing_function_.get());
  StringKeyframeEffectModel* basic_frames =
      StringKeyframeEffectModel::Create(basic_frames_vector);
  EXPECT_TRUE(CanStartEffectOnCompositor(timing_, *basic_frames));

  basic_frames_vector[0]->SetEasing(CubicBezierTimingFunction::Preset(
      CubicBezierTimingFunction::EaseType::EASE_IN));
  basic_frames = StringKeyframeEffectModel::Create(basic_frames_vector);
  EXPECT_TRUE(CanStartEffectOnCompositor(timing_, *basic_frames));

  non_basic_frames_vector[0]->SetEasing(linear_timing_function_.get());
  non_basic_frames_vector[1]->SetEasing(CubicBezierTimingFunction::Preset(
      CubicBezierTimingFunction::EaseType::EASE_IN));
  StringKeyframeEffectModel* non_basic_frames =
      StringKeyframeEffectModel::Create(non_basic_frames_vector);
  EXPECT_TRUE(CanStartEffectOnCompositor(timing_, *non_basic_frames));
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

TEST_F(AnimationCompositorAnimationsTest, createSimpleOpacityAnimation) {
  // KeyframeEffect to convert
  StringKeyframeEffectModel* effect = CreateKeyframeEffectModel(
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect);
  EXPECT_EQ(CompositorTargetProperty::OPACITY,
            keyframe_model->TargetProperty());
  EXPECT_EQ(1.0, keyframe_model->Iterations());
  EXPECT_EQ(0, keyframe_model->TimeOffset());
  EXPECT_EQ(CompositorKeyframeModel::Direction::NORMAL,
            keyframe_model->GetDirection());
  EXPECT_EQ(1.0, keyframe_model->PlaybackRate());

  std::unique_ptr<CompositorFloatAnimationCurve> keyframed_float_curve =
      keyframe_model->FloatCurveForTesting();

  CompositorFloatAnimationCurve::Keyframes keyframes =
      keyframed_float_curve->KeyframesForTesting();
  ASSERT_EQ(2UL, keyframes.size());

  EXPECT_EQ(0, keyframes[0]->Time());
  EXPECT_EQ(0.2f, keyframes[0]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[0]->GetTimingFunctionForTesting()->GetType());

  EXPECT_EQ(1.0, keyframes[1]->Time());
  EXPECT_EQ(0.5f, keyframes[1]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[1]->GetTimingFunctionForTesting()->GetType());
}

TEST_F(AnimationCompositorAnimationsTest,
       createSimpleOpacityAnimationDuration) {
  // KeyframeEffect to convert
  StringKeyframeEffectModel* effect = CreateKeyframeEffectModel(
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));

  const double kDuration = 10.0;
  timing_.iteration_duration = kDuration;

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect);
  std::unique_ptr<CompositorFloatAnimationCurve> keyframed_float_curve =
      keyframe_model->FloatCurveForTesting();

  CompositorFloatAnimationCurve::Keyframes keyframes =
      keyframed_float_curve->KeyframesForTesting();
  ASSERT_EQ(2UL, keyframes.size());

  EXPECT_EQ(kDuration, keyframes[1]->Time() * kDuration);
}

TEST_F(AnimationCompositorAnimationsTest,
       createMultipleKeyframeOpacityAnimationLinear) {
  // KeyframeEffect to convert
  StringKeyframeEffectModel* effect = CreateKeyframeEffectModel(
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.0", 0.25),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.25", 0.5),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));

  timing_.iteration_count = 5;
  timing_.direction = Timing::PlaybackDirection::ALTERNATE_NORMAL;
  timing_.playback_rate = 2.0;

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect);
  EXPECT_EQ(CompositorTargetProperty::OPACITY,
            keyframe_model->TargetProperty());
  EXPECT_EQ(5.0, keyframe_model->Iterations());
  EXPECT_EQ(0, keyframe_model->TimeOffset());
  EXPECT_EQ(CompositorKeyframeModel::Direction::ALTERNATE_NORMAL,
            keyframe_model->GetDirection());
  EXPECT_EQ(2.0, keyframe_model->PlaybackRate());

  std::unique_ptr<CompositorFloatAnimationCurve> keyframed_float_curve =
      keyframe_model->FloatCurveForTesting();

  CompositorFloatAnimationCurve::Keyframes keyframes =
      keyframed_float_curve->KeyframesForTesting();
  ASSERT_EQ(4UL, keyframes.size());

  EXPECT_EQ(0, keyframes[0]->Time());
  EXPECT_EQ(0.2f, keyframes[0]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[0]->GetTimingFunctionForTesting()->GetType());

  EXPECT_EQ(0.25, keyframes[1]->Time());
  EXPECT_EQ(0, keyframes[1]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[1]->GetTimingFunctionForTesting()->GetType());

  EXPECT_EQ(0.5, keyframes[2]->Time());
  EXPECT_EQ(0.25f, keyframes[2]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[2]->GetTimingFunctionForTesting()->GetType());

  EXPECT_EQ(1.0, keyframes[3]->Time());
  EXPECT_EQ(0.5f, keyframes[3]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[3]->GetTimingFunctionForTesting()->GetType());
}

TEST_F(AnimationCompositorAnimationsTest,
       createSimpleOpacityAnimationStartDelay) {
  // KeyframeEffect to convert
  StringKeyframeEffectModel* effect = CreateKeyframeEffectModel(
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));

  const double kStartDelay = 3.25;

  timing_.iteration_count = 5.0;
  timing_.iteration_duration = 1.75;
  timing_.start_delay = kStartDelay;

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect);

  EXPECT_EQ(CompositorTargetProperty::OPACITY,
            keyframe_model->TargetProperty());
  EXPECT_EQ(5.0, keyframe_model->Iterations());
  EXPECT_EQ(-kStartDelay, keyframe_model->TimeOffset());

  std::unique_ptr<CompositorFloatAnimationCurve> keyframed_float_curve =
      keyframe_model->FloatCurveForTesting();

  CompositorFloatAnimationCurve::Keyframes keyframes =
      keyframed_float_curve->KeyframesForTesting();
  ASSERT_EQ(2UL, keyframes.size());

  EXPECT_EQ(1.75, keyframes[1]->Time() * timing_.iteration_duration);
  EXPECT_EQ(0.5f, keyframes[1]->Value());
}

TEST_F(AnimationCompositorAnimationsTest,
       createMultipleKeyframeOpacityAnimationChained) {
  // KeyframeEffect to convert
  StringKeyframeVector frames;
  frames.push_back(CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0));
  frames.push_back(CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.0", 0.25));
  frames.push_back(CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.35", 0.5));
  frames.push_back(CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));
  frames[0]->SetEasing(cubic_ease_timing_function_.get());
  frames[1]->SetEasing(linear_timing_function_.get());
  frames[2]->SetEasing(cubic_custom_timing_function_.get());
  StringKeyframeEffectModel* effect = StringKeyframeEffectModel::Create(frames);

  timing_.timing_function = linear_timing_function_.get();
  timing_.iteration_duration = 2.0;
  timing_.iteration_count = 10;
  timing_.direction = Timing::PlaybackDirection::ALTERNATE_NORMAL;

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect);
  EXPECT_EQ(CompositorTargetProperty::OPACITY,
            keyframe_model->TargetProperty());
  EXPECT_EQ(10.0, keyframe_model->Iterations());
  EXPECT_EQ(0, keyframe_model->TimeOffset());
  EXPECT_EQ(CompositorKeyframeModel::Direction::ALTERNATE_NORMAL,
            keyframe_model->GetDirection());
  EXPECT_EQ(1.0, keyframe_model->PlaybackRate());

  std::unique_ptr<CompositorFloatAnimationCurve> keyframed_float_curve =
      keyframe_model->FloatCurveForTesting();

  CompositorFloatAnimationCurve::Keyframes keyframes =
      keyframed_float_curve->KeyframesForTesting();
  ASSERT_EQ(4UL, keyframes.size());

  EXPECT_EQ(0, keyframes[0]->Time() * timing_.iteration_duration);
  EXPECT_EQ(0.2f, keyframes[0]->Value());
  ExpectKeyframeTimingFunctionCubic(*keyframes[0],
                                    CubicBezierTimingFunction::EaseType::EASE);

  EXPECT_EQ(0.5, keyframes[1]->Time() * timing_.iteration_duration);
  EXPECT_EQ(0, keyframes[1]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[1]->GetTimingFunctionForTesting()->GetType());

  EXPECT_EQ(1.0, keyframes[2]->Time() * timing_.iteration_duration);
  EXPECT_EQ(0.35f, keyframes[2]->Value());
  ExpectKeyframeTimingFunctionCubic(
      *keyframes[2], CubicBezierTimingFunction::EaseType::CUSTOM);

  EXPECT_EQ(2.0, keyframes[3]->Time() * timing_.iteration_duration);
  EXPECT_EQ(0.5f, keyframes[3]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[3]->GetTimingFunctionForTesting()->GetType());
}

TEST_F(AnimationCompositorAnimationsTest, createReversedOpacityAnimation) {
  scoped_refptr<TimingFunction> cubic_easy_flip_timing_function =
      CubicBezierTimingFunction::Create(0.0, 0.0, 0.0, 1.0);

  // KeyframeEffect to convert
  StringKeyframeVector frames;
  frames.push_back(CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0));
  frames.push_back(CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.0", 0.25));
  frames.push_back(CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.25", 0.5));
  frames.push_back(CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));
  frames[0]->SetEasing(CubicBezierTimingFunction::Preset(
      CubicBezierTimingFunction::EaseType::EASE_IN));
  frames[1]->SetEasing(linear_timing_function_.get());
  frames[2]->SetEasing(cubic_easy_flip_timing_function.get());
  StringKeyframeEffectModel* effect = StringKeyframeEffectModel::Create(frames);

  timing_.timing_function = linear_timing_function_.get();
  timing_.iteration_count = 10;
  timing_.direction = Timing::PlaybackDirection::ALTERNATE_REVERSE;

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect);
  EXPECT_EQ(CompositorTargetProperty::OPACITY,
            keyframe_model->TargetProperty());
  EXPECT_EQ(10.0, keyframe_model->Iterations());
  EXPECT_EQ(0, keyframe_model->TimeOffset());
  EXPECT_EQ(CompositorKeyframeModel::Direction::ALTERNATE_REVERSE,
            keyframe_model->GetDirection());
  EXPECT_EQ(1.0, keyframe_model->PlaybackRate());

  std::unique_ptr<CompositorFloatAnimationCurve> keyframed_float_curve =
      keyframe_model->FloatCurveForTesting();

  CompositorFloatAnimationCurve::Keyframes keyframes =
      keyframed_float_curve->KeyframesForTesting();
  ASSERT_EQ(4UL, keyframes.size());

  EXPECT_EQ(keyframed_float_curve->GetTimingFunctionForTesting()->GetType(),
            TimingFunction::Type::LINEAR);

  EXPECT_EQ(0, keyframes[0]->Time());
  EXPECT_EQ(0.2f, keyframes[0]->Value());
  ExpectKeyframeTimingFunctionCubic(
      *keyframes[0], CubicBezierTimingFunction::EaseType::EASE_IN);

  EXPECT_EQ(0.25, keyframes[1]->Time());
  EXPECT_EQ(0, keyframes[1]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[1]->GetTimingFunctionForTesting()->GetType());

  EXPECT_EQ(0.5, keyframes[2]->Time());
  EXPECT_EQ(0.25f, keyframes[2]->Value());
  ExpectKeyframeTimingFunctionCubic(
      *keyframes[2], CubicBezierTimingFunction::EaseType::CUSTOM);

  EXPECT_EQ(1.0, keyframes[3]->Time());
  EXPECT_EQ(0.5f, keyframes[3]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[3]->GetTimingFunctionForTesting()->GetType());
}

TEST_F(AnimationCompositorAnimationsTest,
       createReversedOpacityAnimationNegativeStartDelay) {
  // KeyframeEffect to convert
  StringKeyframeEffectModel* effect = CreateKeyframeEffectModel(
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));

  const double kNegativeStartDelay = -3;

  timing_.iteration_count = 5.0;
  timing_.iteration_duration = 1.5;
  timing_.start_delay = kNegativeStartDelay;
  timing_.direction = Timing::PlaybackDirection::ALTERNATE_REVERSE;

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect);
  EXPECT_EQ(CompositorTargetProperty::OPACITY,
            keyframe_model->TargetProperty());
  EXPECT_EQ(5.0, keyframe_model->Iterations());
  EXPECT_EQ(-kNegativeStartDelay, keyframe_model->TimeOffset());
  EXPECT_EQ(CompositorKeyframeModel::Direction::ALTERNATE_REVERSE,
            keyframe_model->GetDirection());
  EXPECT_EQ(1.0, keyframe_model->PlaybackRate());

  std::unique_ptr<CompositorFloatAnimationCurve> keyframed_float_curve =
      keyframe_model->FloatCurveForTesting();

  CompositorFloatAnimationCurve::Keyframes keyframes =
      keyframed_float_curve->KeyframesForTesting();
  ASSERT_EQ(2UL, keyframes.size());
}

TEST_F(AnimationCompositorAnimationsTest,
       createSimpleOpacityAnimationPlaybackRates) {
  // KeyframeEffect to convert
  StringKeyframeEffectModel* effect = CreateKeyframeEffectModel(
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));

  const double kPlaybackRate = 2;
  const double kAnimationPlaybackRate = -1.5;

  timing_.playback_rate = kPlaybackRate;

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect, kAnimationPlaybackRate);
  EXPECT_EQ(CompositorTargetProperty::OPACITY,
            keyframe_model->TargetProperty());
  EXPECT_EQ(1.0, keyframe_model->Iterations());
  EXPECT_EQ(0, keyframe_model->TimeOffset());
  EXPECT_EQ(CompositorKeyframeModel::Direction::NORMAL,
            keyframe_model->GetDirection());
  EXPECT_EQ(kPlaybackRate * kAnimationPlaybackRate,
            keyframe_model->PlaybackRate());

  std::unique_ptr<CompositorFloatAnimationCurve> keyframed_float_curve =
      keyframe_model->FloatCurveForTesting();

  CompositorFloatAnimationCurve::Keyframes keyframes =
      keyframed_float_curve->KeyframesForTesting();
  ASSERT_EQ(2UL, keyframes.size());
}

TEST_F(AnimationCompositorAnimationsTest,
       createSimpleOpacityAnimationFillModeNone) {
  // KeyframeEffect to convert
  StringKeyframeEffectModel* effect = CreateKeyframeEffectModel(
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));

  timing_.fill_mode = Timing::FillMode::NONE;

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect);
  EXPECT_EQ(CompositorKeyframeModel::FillMode::NONE,
            keyframe_model->GetFillMode());
}

TEST_F(AnimationCompositorAnimationsTest,
       createSimpleOpacityAnimationFillModeAuto) {
  // KeyframeEffect to convert
  StringKeyframeEffectModel* effect = CreateKeyframeEffectModel(
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));

  timing_.fill_mode = Timing::FillMode::AUTO;

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect);
  EXPECT_EQ(CompositorTargetProperty::OPACITY,
            keyframe_model->TargetProperty());
  EXPECT_EQ(1.0, keyframe_model->Iterations());
  EXPECT_EQ(0, keyframe_model->TimeOffset());
  EXPECT_EQ(CompositorKeyframeModel::Direction::NORMAL,
            keyframe_model->GetDirection());
  EXPECT_EQ(1.0, keyframe_model->PlaybackRate());
  EXPECT_EQ(CompositorKeyframeModel::FillMode::NONE,
            keyframe_model->GetFillMode());
}

TEST_F(AnimationCompositorAnimationsTest,
       createSimpleOpacityAnimationWithTimingFunction) {
  // KeyframeEffect to convert
  StringKeyframeEffectModel* effect = CreateKeyframeEffectModel(
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.2", 0),
      CreateReplaceOpKeyframe(CSSPropertyOpacity, "0.5", 1.0));

  timing_.timing_function = cubic_custom_timing_function_;

  std::unique_ptr<CompositorKeyframeModel> keyframe_model =
      ConvertToCompositorAnimation(*effect);

  std::unique_ptr<CompositorFloatAnimationCurve> keyframed_float_curve =
      keyframe_model->FloatCurveForTesting();

  auto curve_timing_function =
      keyframed_float_curve->GetTimingFunctionForTesting();
  EXPECT_EQ(curve_timing_function->GetType(),
            TimingFunction::Type::CUBIC_BEZIER);
  const auto& cubic_timing_function =
      ToCubicBezierTimingFunction(*curve_timing_function);
  EXPECT_EQ(cubic_timing_function.GetEaseType(),
            CubicBezierTimingFunction::EaseType::CUSTOM);
  EXPECT_EQ(cubic_timing_function.X1(), 1.0);
  EXPECT_EQ(cubic_timing_function.Y1(), 2.0);
  EXPECT_EQ(cubic_timing_function.X2(), 3.0);
  EXPECT_EQ(cubic_timing_function.Y2(), 4.0);

  CompositorFloatAnimationCurve::Keyframes keyframes =
      keyframed_float_curve->KeyframesForTesting();
  ASSERT_EQ(2UL, keyframes.size());

  EXPECT_EQ(0, keyframes[0]->Time());
  EXPECT_EQ(0.2f, keyframes[0]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[0]->GetTimingFunctionForTesting()->GetType());

  EXPECT_EQ(1.0, keyframes[1]->Time());
  EXPECT_EQ(0.5f, keyframes[1]->Value());
  EXPECT_EQ(TimingFunction::Type::LINEAR,
            keyframes[1]->GetTimingFunctionForTesting()->GetType());
}

TEST_F(AnimationCompositorAnimationsTest,
       cancelIncompatibleCompositorAnimations) {
  Persistent<Element> element = GetDocument().CreateElementForBinding("shared");

  LayoutObjectProxy* layout_object = LayoutObjectProxy::Create(element.Get());
  element->SetLayoutObject(layout_object);

  StringKeyframeVector key_frames;
  key_frames.push_back(CreateDefaultKeyframe(CSSPropertyOpacity,
                                             EffectModel::kCompositeReplace,
                                             0.0)
                           .get());
  key_frames.push_back(CreateDefaultKeyframe(CSSPropertyOpacity,
                                             EffectModel::kCompositeReplace,
                                             1.0)
                           .get());
  KeyframeEffectModelBase* animation_effect1 =
      StringKeyframeEffectModel::Create(key_frames);
  KeyframeEffectModelBase* animation_effect2 =
      StringKeyframeEffectModel::Create(key_frames);

  Timing timing;
  timing.iteration_duration = 1.f;

  // The first animation for opacity is ok to run on compositor.
  KeyframeEffect* keyframe_effect1 =
      KeyframeEffect::Create(element.Get(), animation_effect1, timing);
  Animation* animation1 = timeline_->Play(keyframe_effect1);
  auto style = ComputedStyle::Create();
  animation_effect1->SnapshotAllCompositorKeyframesIfNecessary(*element_.Get(),
                                                               *style, nullptr);
  EXPECT_TRUE(CompositorAnimations::CheckCanStartEffectOnCompositor(
                  timing, *element.Get(), animation1, *animation_effect1, 1)
                  .Ok());

  // simulate KeyframeEffect::maybeStartAnimationOnCompositor
  Vector<int> compositor_animation_ids;
  compositor_animation_ids.push_back(1);
  keyframe_effect1->SetCompositorAnimationIdsForTesting(
      compositor_animation_ids);
  EXPECT_TRUE(animation1->HasActiveAnimationsOnCompositor());

  // The second animation for opacity is not ok to run on compositor.
  KeyframeEffect* keyframe_effect2 =
      KeyframeEffect::Create(element.Get(), animation_effect2, timing);
  Animation* animation2 = timeline_->Play(keyframe_effect2);
  animation_effect2->SnapshotAllCompositorKeyframesIfNecessary(*element_.Get(),
                                                               *style, nullptr);
  EXPECT_FALSE(CompositorAnimations::CheckCanStartEffectOnCompositor(
                   timing, *element.Get(), animation2, *animation_effect2, 1)
                   .Ok());
  EXPECT_FALSE(animation2->HasActiveAnimationsOnCompositor());

  // A fallback to blink implementation needed, so cancel all compositor-side
  // opacity animations for this element.
  animation2->CancelIncompatibleAnimationsOnCompositor();

  EXPECT_FALSE(animation1->HasActiveAnimationsOnCompositor());
  EXPECT_FALSE(animation2->HasActiveAnimationsOnCompositor());

  SimulateFrame(0);
  EXPECT_EQ(2U, element->GetElementAnimations()->Animations().size());
  SimulateFrame(1.);

  element->SetLayoutObject(nullptr);
  LayoutObjectProxy::Dispose(layout_object);

  ThreadState::Current()->CollectAllGarbage();
  EXPECT_TRUE(element->GetElementAnimations()->Animations().IsEmpty());
}

namespace {

void UpdateDummyTransformNode(ObjectPaintProperties& properties,
                              CompositingReasons reasons) {
  TransformPaintPropertyNode::State state;
  state.direct_compositing_reasons = reasons;
  properties.UpdateTransform(TransformPaintPropertyNode::Root(),
                             std::move(state));
}

void UpdateDummyEffectNode(ObjectPaintProperties& properties,
                           CompositingReasons reasons) {
  EffectPaintPropertyNode::State state;
  state.direct_compositing_reasons = reasons;
  properties.UpdateEffect(EffectPaintPropertyNode::Root(), std::move(state));
}

}  // namespace

TEST_F(AnimationCompositorAnimationsTest,
       canStartElementOnCompositorTransformSPv2) {
  Persistent<Element> element = GetDocument().CreateElementForBinding("shared");
  LayoutObjectProxy* layout_object = LayoutObjectProxy::Create(element.Get());
  element->SetLayoutObject(layout_object);

  ScopedSlimmingPaintV2ForTest enable_s_pv2(true);
  auto& properties = layout_object->GetMutableForPainting()
                         .FirstFragment()
                         .EnsurePaintProperties();

  // Add a transform with a compositing reason, which should allow starting
  // animation.
  UpdateDummyTransformNode(properties,
                           CompositingReason::kActiveTransformAnimation);
  EXPECT_TRUE(
      CompositorAnimations::CheckCanStartElementOnCompositor(*element).Ok());

  // Setting to CompositingReasonNone should produce false.
  UpdateDummyTransformNode(properties, CompositingReason::kNone);
  EXPECT_FALSE(
      CompositorAnimations::CheckCanStartElementOnCompositor(*element).Ok());

  // Clearing the transform node entirely should also produce false.
  properties.ClearTransform();
  EXPECT_FALSE(
      CompositorAnimations::CheckCanStartElementOnCompositor(*element).Ok());

  element->SetLayoutObject(nullptr);
  LayoutObjectProxy::Dispose(layout_object);
}

TEST_F(AnimationCompositorAnimationsTest,
       canStartElementOnCompositorEffectSPv2) {
  Persistent<Element> element = GetDocument().CreateElementForBinding("shared");
  LayoutObjectProxy* layout_object = LayoutObjectProxy::Create(element.Get());
  element->SetLayoutObject(layout_object);

  ScopedSlimmingPaintV2ForTest enable_s_pv2(true);
  auto& properties = layout_object->GetMutableForPainting()
                         .FirstFragment()
                         .EnsurePaintProperties();

  // Add an effect with a compositing reason, which should allow starting
  // animation.
  UpdateDummyEffectNode(properties,
                        CompositingReason::kActiveTransformAnimation);
  EXPECT_TRUE(
      CompositorAnimations::CheckCanStartElementOnCompositor(*element).Ok());

  // Setting to CompositingReasonNone should produce false.
  UpdateDummyEffectNode(properties, CompositingReason::kNone);
  EXPECT_FALSE(
      CompositorAnimations::CheckCanStartElementOnCompositor(*element).Ok());

  // Clearing the effect node entirely should also produce false.
  properties.ClearEffect();
  EXPECT_FALSE(
      CompositorAnimations::CheckCanStartElementOnCompositor(*element).Ok());

  element->SetLayoutObject(nullptr);
  LayoutObjectProxy::Dispose(layout_object);
}

TEST_F(AnimationCompositorAnimationsTest, trackRafAnimation) {
  LoadTestData("raf-countdown.html");

  CompositorAnimationHost* host =
      GetFrame()->GetDocument()->View()->GetCompositorAnimationHost();

  // The test file registers two rAF 'animations'; one which ends after 5
  // iterations and the other that ends after 10.
  for (int i = 0; i < 9; i++) {
    BeginFrame();
    ForceFullCompositingUpdate();
    EXPECT_TRUE(host->CurrentFrameHadRAFForTesting());
    EXPECT_TRUE(host->NextFrameHasPendingRAFForTesting());
  }

  // On the 10th iteration, there should be a current rAF, but no more pending
  // rAFs.
  BeginFrame();
  ForceFullCompositingUpdate();
  EXPECT_TRUE(host->CurrentFrameHadRAFForTesting());
  EXPECT_FALSE(host->NextFrameHasPendingRAFForTesting());

  // On the 11th iteration, there should be no more rAFs firing.
  BeginFrame();
  ForceFullCompositingUpdate();
  EXPECT_FALSE(host->CurrentFrameHadRAFForTesting());
  EXPECT_FALSE(host->NextFrameHasPendingRAFForTesting());
}

TEST_F(AnimationCompositorAnimationsTest, trackRafAnimationTimeout) {
  LoadTestData("raf-timeout.html");

  CompositorAnimationHost* host =
      GetFrame()->GetDocument()->View()->GetCompositorAnimationHost();

  // The test file executes a rAF, which fires a setTimeout for the next rAF.
  // Even with setTimeout(func, 0), the next rAF is not considered pending.
  BeginFrame();
  ForceFullCompositingUpdate();
  EXPECT_TRUE(host->CurrentFrameHadRAFForTesting());
  EXPECT_FALSE(host->NextFrameHasPendingRAFForTesting());
}

TEST_F(AnimationCompositorAnimationsTest, trackRafAnimationNoneRegistered) {
  SetBodyInnerHTML("<div id='box'></div>");

  // Run a full frame after loading the test data so that scripted animations
  // are serviced and data propagated.
  BeginFrame();
  ForceFullCompositingUpdate();

  // The HTML does not have any rAFs.
  CompositorAnimationHost* host =
      GetFrame()->GetDocument()->View()->GetCompositorAnimationHost();
  EXPECT_FALSE(host->CurrentFrameHadRAFForTesting());
  EXPECT_FALSE(host->NextFrameHasPendingRAFForTesting());

  // And still shouldn't after another frame.
  BeginFrame();
  ForceFullCompositingUpdate();
  EXPECT_FALSE(host->CurrentFrameHadRAFForTesting());
  EXPECT_FALSE(host->NextFrameHasPendingRAFForTesting());
}

TEST_F(AnimationCompositorAnimationsTest, canStartElementOnCompositorEffect) {
  LoadTestData("transform-animation.html");
  Document* document = GetFrame()->GetDocument();
  Element* target = document->getElementById("target");
  const ObjectPaintProperties* properties =
      target->GetLayoutObject()->FirstFragment().PaintProperties();
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    EXPECT_TRUE(properties->Transform()->HasDirectCompositingReasons());
  CompositorAnimations::FailureCode code =
      CompositorAnimations::CheckCanStartElementOnCompositor(*target);
  EXPECT_EQ(code, CompositorAnimations::FailureCode::None());
  EXPECT_EQ(document->Timeline().PendingAnimationsCount(), 1u);
  CompositorAnimationHost* host =
      document->View()->GetCompositorAnimationHost();
  EXPECT_EQ(host->GetMainThreadAnimationsCountForTesting(), 0u);
  EXPECT_EQ(host->GetCompositedAnimationsCountForTesting(), 1u);
}

// Regression test for https://crbug.com/781305. When we have a transform
// animation on a SVG element, the effect can be started on compositor but the
// element itself cannot.
TEST_F(AnimationCompositorAnimationsTest,
       cannotStartElementOnCompositorEffectSVG) {
  LoadTestData("transform-animation-on-svg.html");
  Document* document = GetFrame()->GetDocument();
  Element* target = document->getElementById("dots");
  CompositorAnimations::FailureCode code =
      CompositorAnimations::CheckCanStartElementOnCompositor(*target);
  EXPECT_EQ(code, CompositorAnimations::FailureCode::NonActionable(
                      "Element does not paint into own backing"));
  EXPECT_EQ(document->Timeline().PendingAnimationsCount(), 4u);
  CompositorAnimationHost* host =
      document->View()->GetCompositorAnimationHost();
  EXPECT_EQ(host->GetMainThreadAnimationsCountForTesting(), 4u);
  EXPECT_EQ(host->GetCompositedAnimationsCountForTesting(), 0u);
}

}  // namespace blink
