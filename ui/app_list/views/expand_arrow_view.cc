// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/expand_arrow_view.h"

#include <memory>
#include <utility>

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/cpp/app_list/vector_icons/vector_icons.h"
#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/animation/ink_drop_painted_layer_delegates.h"
#include "ui/views/controls/image_view.h"

namespace app_list {

namespace {

constexpr int kExpandArrowTileSize = 60;
constexpr int kExpandArrowIconSize = 12;
constexpr int kClipViewSize = 36;
constexpr int kInkDropRadius = 18;
constexpr int kSelectedRadius = 18;

// Animation related constants
constexpr int kPulseMinRadius = 2;
constexpr int kPulseMaxRadius = 30;
constexpr float kPulseMinOpacity = 0.f;
constexpr float kPulseMaxOpacity = 0.3f;
constexpr int kAnimationInitialWaitTimeInSec = 3;
constexpr int kAnimationIntervalInSec = 10;
constexpr int kCycleDurationInMs = 1000;
constexpr int kCycleIntervalInMs = 500;
constexpr int kPulseOpacityShowBeginTimeInMs = 100;
constexpr int kPulseOpacityShowEndTimeInMs = 200;
constexpr int kPulseOpacityHideBeginTimeInMs = 800;
constexpr int kPulseOpacityHideEndTimeInMs = 1000;
constexpr int kArrowMoveOutBeginTimeInMs = 100;
constexpr int kArrowMoveOutEndTimeInMs = 500;
constexpr int kArrowMoveInBeginTimeInMs = 500;
constexpr int kArrowMoveInEndTimeInMs = 900;

constexpr SkColor kExpandArrowColor = SK_ColorWHITE;
constexpr SkColor kPulseColor = SK_ColorWHITE;
constexpr SkColor kUnFocusedBackgroundColor =
    SkColorSetARGB(0xF, 0xFF, 0xFF, 0xFF);
constexpr SkColor kFocusedBackgroundColor =
    SkColorSetARGB(0x3D, 0xFF, 0xFF, 0xFF);
constexpr SkColor kInkDropRippleColor = SkColorSetARGB(0x14, 0xFF, 0xFF, 0xFF);

}  // namespace

ExpandArrowView::ExpandArrowView(ContentsView* contents_view,
                                 AppListView* app_list_view)
    : views::Button(this),
      contents_view_(contents_view),
      app_list_view_(app_list_view),
      weak_ptr_factory_(this) {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  icon_ = new views::ImageView;
  icon_->SetVerticalAlignment(views::ImageView::CENTER);
  icon_->SetImage(gfx::CreateVectorIcon(kIcArrowUpIcon, kExpandArrowIconSize,
                                        kExpandArrowColor));
  clip_view_ = new views::View;
  clip_view_->AddChildView(icon_);
  AddChildView(clip_view_);

  SetInkDropMode(InkDropHostView::InkDropMode::ON);

  SetAccessibleName(l10n_util::GetStringUTF16(IDS_APP_LIST_EXPAND_BUTTON));

  animation_.reset(new gfx::SlideAnimation(this));
  animation_->SetTweenType(gfx::Tween::LINEAR);
  animation_->SetSlideDuration(kCycleDurationInMs * 2 + kCycleIntervalInMs);
  ResetHintingAnimation();
  ScheduleHintingAnimation(true);
}

ExpandArrowView::~ExpandArrowView() = default;

void ExpandArrowView::PaintButtonContents(gfx::Canvas* canvas) {
  gfx::Rect rect(GetContentsBounds());

  // Draw focused or unfocused background.
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(HasFocus() ? kFocusedBackgroundColor
                            : kUnFocusedBackgroundColor);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->DrawCircle(gfx::PointF(rect.CenterPoint()), kSelectedRadius, flags);

  if (animation_->is_animating()) {
    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kStroke_Style);
    flags.setColor(SkColorSetA(kPulseColor, 255 * pulse_opacity_));
    flags.setAntiAlias(true);
    canvas->DrawCircle(rect.CenterPoint(), pulse_radius_, flags);
  }
}

void ExpandArrowView::ButtonPressed(views::Button* sender,
                                    const ui::Event& event) {
  button_pressed_ = true;
  ResetHintingAnimation();
  TransitToFullscreenAllAppsState();
  GetInkDrop()->AnimateToState(views::InkDropState::ACTION_TRIGGERED);
}

gfx::Size ExpandArrowView::CalculatePreferredSize() const {
  return gfx::Size(kExpandArrowTileSize, kExpandArrowTileSize);
}

void ExpandArrowView::Layout() {
  gfx::Rect clip_view_rect(GetContentsBounds());
  clip_view_rect.ClampToCenteredSize(gfx::Size(kClipViewSize, kClipViewSize));
  clip_view_->SetBoundsRect(clip_view_rect);

  gfx::Rect icon_rect(clip_view_->GetContentsBounds());
  icon_rect.ClampToCenteredSize(
      gfx::Size(kExpandArrowIconSize, kExpandArrowIconSize));
  icon_->SetBoundsRect(icon_rect);
}

bool ExpandArrowView::OnKeyPressed(const ui::KeyEvent& event) {
  if (event.key_code() != ui::VKEY_RETURN)
    return false;
  TransitToFullscreenAllAppsState();
  return true;
}

void ExpandArrowView::OnFocus() {
  SchedulePaint();
  Button::OnFocus();
}

void ExpandArrowView::OnBlur() {
  SchedulePaint();
  Button::OnBlur();
}

std::unique_ptr<views::InkDrop> ExpandArrowView::CreateInkDrop() {
  std::unique_ptr<views::InkDropImpl> ink_drop =
      Button::CreateDefaultInkDropImpl();
  ink_drop->SetShowHighlightOnHover(false);
  ink_drop->SetShowHighlightOnFocus(false);
  ink_drop->SetAutoHighlightMode(views::InkDropImpl::AutoHighlightMode::NONE);
  return std::move(ink_drop);
}

std::unique_ptr<views::InkDropMask> ExpandArrowView::CreateInkDropMask() const {
  return std::make_unique<views::CircleInkDropMask>(
      size(), GetLocalBounds().CenterPoint(), kInkDropRadius);
}

std::unique_ptr<views::InkDropRipple> ExpandArrowView::CreateInkDropRipple()
    const {
  gfx::Point center = GetLocalBounds().CenterPoint();
  gfx::Rect bounds(center.x() - kInkDropRadius, center.y() - kInkDropRadius,
                   2 * kInkDropRadius, 2 * kInkDropRadius);
  return std::make_unique<views::FloodFillInkDropRipple>(
      size(), GetLocalBounds().InsetsFrom(bounds),
      GetInkDropCenterBasedOnLastEvent(), kInkDropRippleColor, 1.0f);
}

void ExpandArrowView::AnimationProgressed(const gfx::Animation* animation) {
  // There are two cycles in one animation.
  const int animation_duration = kCycleDurationInMs * 2 + kCycleIntervalInMs;
  const int first_cycle_end_time = kCycleDurationInMs;
  const int interval_end_time = kCycleDurationInMs + kCycleIntervalInMs;
  const int second_cycle_end_time = kCycleDurationInMs * 2 + kCycleIntervalInMs;
  int time_in_ms = animation->GetCurrentValue() * animation_duration;

  if (time_in_ms > first_cycle_end_time && time_in_ms <= interval_end_time) {
    // There's no animation in the interval between cycles.
    return;
  } else if (time_in_ms > interval_end_time &&
             time_in_ms <= second_cycle_end_time) {
    // Convert to time in one single cycle.
    time_in_ms -= interval_end_time;
  }

  // Update pulse opacity.
  if (time_in_ms > kPulseOpacityShowBeginTimeInMs &&
      time_in_ms <= kPulseOpacityShowEndTimeInMs) {
    pulse_opacity_ =
        kPulseMinOpacity +
        (kPulseMaxOpacity - kPulseMinOpacity) *
            (time_in_ms - kPulseOpacityShowBeginTimeInMs) /
            (kPulseOpacityShowEndTimeInMs - kPulseOpacityShowBeginTimeInMs);
  } else if (time_in_ms > kPulseOpacityHideBeginTimeInMs &&
             time_in_ms <= kPulseOpacityHideEndTimeInMs) {
    pulse_opacity_ =
        kPulseMaxOpacity -
        (kPulseMaxOpacity - kPulseMinOpacity) *
            (time_in_ms - kPulseOpacityHideBeginTimeInMs) /
            (kPulseOpacityHideEndTimeInMs - kPulseOpacityHideBeginTimeInMs);
  }

  // Update pulse radius.
  pulse_radius_ = static_cast<float>(kPulseMaxRadius - kPulseMinRadius) *
                  gfx::Tween::CalculateValue(
                      gfx::Tween::EASE_IN_OUT,
                      static_cast<double>(time_in_ms) / kCycleDurationInMs);

  // Update y position of the arrow icon.
  int y_position = icon_->bounds().y();
  const int total_y_delta = (kExpandArrowTileSize - kExpandArrowIconSize) / 2;
  if (time_in_ms > kArrowMoveOutBeginTimeInMs &&
      time_in_ms <= kArrowMoveOutEndTimeInMs) {
    const double progress =
        static_cast<double>(time_in_ms - kArrowMoveOutBeginTimeInMs) /
        (kArrowMoveOutEndTimeInMs - kArrowMoveOutBeginTimeInMs);
    y_position = (kClipViewSize - kExpandArrowIconSize) / 2 -
                 total_y_delta *
                     gfx::Tween::CalculateValue(gfx::Tween::EASE_IN, progress);
  } else if (time_in_ms > kArrowMoveInBeginTimeInMs &&
             time_in_ms <= kArrowMoveInEndTimeInMs) {
    const double progress =
        static_cast<double>(time_in_ms - kArrowMoveInBeginTimeInMs) /
        (kArrowMoveInEndTimeInMs - kArrowMoveInBeginTimeInMs);
    y_position = (kClipViewSize - kExpandArrowIconSize) / 2 +
                 total_y_delta * (1 - gfx::Tween::CalculateValue(
                                          gfx::Tween::EASE_OUT, progress));
  }

  // Apply updates.
  gfx::Rect icon_rect(icon_->bounds());
  icon_rect.set_y(y_position);
  icon_->SetBoundsRect(icon_rect);
  SchedulePaint();
}

void ExpandArrowView::AnimationEnded(const gfx::Animation* animation) {
  ResetHintingAnimation();
  if (!button_pressed_)
    ScheduleHintingAnimation(false);
}

void ExpandArrowView::TransitToFullscreenAllAppsState() {
  UMA_HISTOGRAM_ENUMERATION(kPageOpenedHistogram, ash::AppListState::kStateApps,
                            ash::AppListState::kStateLast);
  UMA_HISTOGRAM_ENUMERATION(kAppListPeekingToFullscreenHistogram, kExpandArrow,
                            kMaxPeekingToFullscreen);
  contents_view_->SetActiveState(ash::AppListState::kStateApps);
  app_list_view_->SetState(AppListViewState::FULLSCREEN_ALL_APPS);
}

void ExpandArrowView::ScheduleHintingAnimation(bool is_first_time) {
  int delay_in_sec = kAnimationIntervalInSec;
  if (is_first_time)
    delay_in_sec = kAnimationInitialWaitTimeInSec;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ExpandArrowView::StartHintingAnimation,
                     weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(delay_in_sec));
}

void ExpandArrowView::StartHintingAnimation() {
  if (!button_pressed_)
    animation_->Show();
}

void ExpandArrowView::ResetHintingAnimation() {
  pulse_opacity_ = kPulseMinOpacity;
  pulse_radius_ = kPulseMinRadius;
  animation_->Reset();
  Layout();
}

}  // namespace app_list
