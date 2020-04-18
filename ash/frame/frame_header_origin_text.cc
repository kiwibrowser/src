// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/frame/frame_header_origin_text.h"

#include "base/i18n/rtl.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/compositor/layer_animation_element.h"
#include "ui/compositor/layer_animation_sequence.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"

namespace ash {

namespace {

constexpr base::TimeDelta kOriginSlideInDuration =
    base::TimeDelta::FromMilliseconds(800);
constexpr base::TimeDelta kOriginPauseDuration =
    base::TimeDelta::FromMilliseconds(2500);
constexpr base::TimeDelta kOriginSlideOutDuration =
    base::TimeDelta::FromMilliseconds(500);

constexpr gfx::Tween::Type kTweenType = gfx::Tween::FAST_OUT_SLOW_IN_2;

}  // namespace

FrameHeaderOriginText::FrameHeaderOriginText(const base::string16& origin,
                                             SkColor active_color,
                                             SkColor inactive_color)
    : active_color_(active_color), inactive_color_(inactive_color) {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  label_ = std::make_unique<views::Label>(origin).release();
  label_->SetElideBehavior(gfx::ELIDE_HEAD);
  label_->SetSubpixelRenderingEnabled(false);
  label_->SetEnabledColor(active_color);
  // Disable Label's auto readability to ensure consistent colors in the title
  // bar (see http://crbug.com/814121#c2).
  label_->SetAutoColorReadabilityEnabled(false);
  label_->SetPaintToLayer();
  label_->layer()->SetFillsBoundsOpaquely(false);
  label_->layer()->SetOpacity(0);
  AddChildView(label_);

  // Clip child views to this view.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  layer()->SetMasksToBounds(true);
}

FrameHeaderOriginText::~FrameHeaderOriginText() = default;

void FrameHeaderOriginText::SetPaintAsActive(bool active) {
  label_->SetEnabledColor(active ? active_color_ : inactive_color_);
}

void FrameHeaderOriginText::StartSlideAnimation() {
  ui::Layer* label_layer = label_->layer();

  // Current state will become the first animation keyframe.
  DCHECK_EQ(label_layer->opacity(), 0);
  gfx::Transform out_of_frame;
  out_of_frame.Translate(
      label_->bounds().width() * (base::i18n::IsRTL() ? -1 : 1), 0);
  label_layer->SetTransform(out_of_frame);

  auto opacity_sequence = std::make_unique<ui::LayerAnimationSequence>();
  auto transform_sequence = std::make_unique<ui::LayerAnimationSequence>();

  // Slide in.
  auto opacity_keyframe = ui::LayerAnimationElement::CreateOpacityElement(
      1, kOriginSlideInDuration);
  opacity_keyframe->set_tween_type(kTweenType);
  opacity_sequence->AddElement(std::move(opacity_keyframe));

  auto transform_keyframe = ui::LayerAnimationElement::CreateTransformElement(
      gfx::Transform(), kOriginSlideInDuration);
  transform_keyframe->set_tween_type(kTweenType);
  transform_sequence->AddElement(std::move(transform_keyframe));

  // Pause.
  opacity_sequence->AddElement(
      ui::LayerAnimationElement::CreatePauseElement(0, kOriginPauseDuration));
  transform_sequence->AddElement(
      ui::LayerAnimationElement::CreatePauseElement(0, kOriginPauseDuration));

  // Slide out.
  opacity_keyframe = ui::LayerAnimationElement::CreateOpacityElement(
      0, kOriginSlideOutDuration);
  opacity_keyframe->set_tween_type(kTweenType);
  opacity_sequence->AddElement(std::move(opacity_keyframe));

  transform_keyframe = ui::LayerAnimationElement::CreateTransformElement(
      out_of_frame, kOriginSlideOutDuration);
  transform_keyframe->set_tween_type(kTweenType);
  transform_sequence->AddElement(std::move(transform_keyframe));

  label_layer->GetAnimator()->StartTogether(
      {opacity_sequence.release(), transform_sequence.release()});

  NotifyAccessibilityEvent(ax::mojom::Event::kValueChanged, true);
}

base::TimeDelta FrameHeaderOriginText::AnimationDuration() {
  return kOriginSlideInDuration + kOriginPauseDuration +
         kOriginSlideOutDuration;
}

void FrameHeaderOriginText::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kApplication;
  node_data->SetName(label_->text());
}

}  // namespace ash
