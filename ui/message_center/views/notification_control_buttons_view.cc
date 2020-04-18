// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/notification_control_buttons_view.h"

#include <memory>

#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/layer.h"
#include "ui/events/event.h"
#include "ui/gfx/animation/linear_animation.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/vector_icons.h"
#include "ui/message_center/views/message_view.h"
#include "ui/message_center/views/padded_button.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/background.h"
#include "ui/views/layout/box_layout.h"

namespace message_center {

namespace {

// This value should be the same as the duration of reveal animation of
// the settings view of an Android notification.
constexpr auto kBackgroundColorChangeDuration =
    base::TimeDelta::FromMilliseconds(360);

// The initial background color of the view.
constexpr SkColor kInitialBackgroundColor = kControlButtonBackgroundColor;

}  // anonymous namespace

const char NotificationControlButtonsView::kViewClassName[] =
    "NotificationControlButtonsView";

NotificationControlButtonsView::NotificationControlButtonsView(
    MessageView* message_view)
    : message_view_(message_view),
      bgcolor_origin_(kInitialBackgroundColor),
      bgcolor_target_(kInitialBackgroundColor) {
  DCHECK(message_view);
  SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kHorizontal));

  // Use layer to change the opacity.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  SetBackground(views::CreateSolidBackground(kInitialBackgroundColor));
}

NotificationControlButtonsView::~NotificationControlButtonsView() = default;

void NotificationControlButtonsView::ShowCloseButton(bool show) {
  if (show && !close_button_) {
    close_button_ = std::make_unique<PaddedButton>(this);
    close_button_->set_owned_by_client();
    close_button_->SetImage(views::Button::STATE_NORMAL,
                            gfx::CreateVectorIcon(kNotificationCloseButtonIcon,
                                                  gfx::kChromeIconGrey));
    close_button_->SetAccessibleName(l10n_util::GetStringUTF16(
        IDS_MESSAGE_CENTER_CLOSE_NOTIFICATION_BUTTON_ACCESSIBLE_NAME));
    close_button_->SetTooltipText(l10n_util::GetStringUTF16(
        IDS_MESSAGE_CENTER_CLOSE_NOTIFICATION_BUTTON_TOOLTIP));
    close_button_->SetBackground(
        views::CreateSolidBackground(SK_ColorTRANSPARENT));

    // Add the button at the last.
    DCHECK_LE(child_count(), 1);
    AddChildView(close_button_.get());
  } else if (!show && close_button_) {
    DCHECK(Contains(close_button_.get()));
    close_button_.reset();
  }
}

void NotificationControlButtonsView::ShowSettingsButton(bool show) {
  if (show && !settings_button_) {
    settings_button_ = std::make_unique<PaddedButton>(this);
    settings_button_->set_owned_by_client();
    settings_button_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(kNotificationSettingsButtonIcon,
                              gfx::kChromeIconGrey));
    settings_button_->SetAccessibleName(l10n_util::GetStringUTF16(
        IDS_MESSAGE_NOTIFICATION_SETTINGS_BUTTON_ACCESSIBLE_NAME));
    settings_button_->SetTooltipText(l10n_util::GetStringUTF16(
        IDS_MESSAGE_NOTIFICATION_SETTINGS_BUTTON_ACCESSIBLE_NAME));
    settings_button_->SetBackground(
        views::CreateSolidBackground(SK_ColorTRANSPARENT));

    // Add the button at the first.
    DCHECK_LE(child_count(), 1);
    AddChildViewAt(settings_button_.get(), 0);
  } else if (!show && settings_button_) {
    DCHECK(Contains(settings_button_.get()));
    settings_button_.reset();
  }
}

void NotificationControlButtonsView::SetBackgroundColor(
    const SkColor& target_bgcolor) {
  DCHECK(background());
  if (background()->get_color() != target_bgcolor) {
    bgcolor_origin_ = background()->get_color();
    bgcolor_target_ = target_bgcolor;

    if (bgcolor_animation_)
      bgcolor_animation_->End();
    bgcolor_animation_.reset(new gfx::LinearAnimation(this));
    bgcolor_animation_->SetDuration(kBackgroundColorChangeDuration);
    bgcolor_animation_->Start();
  }
}

void NotificationControlButtonsView::SetVisible(bool visible) {
  DCHECK(layer());
  // Manipulate the opacity instead of changing the visibility to keep the tab
  // order even when the view is invisible.
  layer()->SetOpacity(visible ? 1. : 0.);
  set_can_process_events_within_subtree(visible);
}

void NotificationControlButtonsView::RequestFocusOnCloseButton() {
  if (close_button_)
    close_button_->RequestFocus();
}

bool NotificationControlButtonsView::IsCloseButtonFocused() const {
  return close_button_ && close_button_->HasFocus();
}

bool NotificationControlButtonsView::IsSettingsButtonFocused() const {
  return settings_button_ && settings_button_->HasFocus();
}

views::Button* NotificationControlButtonsView::close_button() const {
  return close_button_.get();
}

views::Button* NotificationControlButtonsView::settings_button() const {
  return settings_button_.get();
}

const char* NotificationControlButtonsView::GetClassName() const {
  return kViewClassName;
}

void NotificationControlButtonsView::ButtonPressed(views::Button* sender,
                                                   const ui::Event& event) {
  if (close_button_ && sender == close_button_.get()) {
    message_view_->OnCloseButtonPressed();
  } else if (settings_button_ && sender == settings_button_.get()) {
    message_view_->OnSettingsButtonPressed(event);
  }
}

void NotificationControlButtonsView::AnimationProgressed(
    const gfx::Animation* animation) {
  DCHECK_EQ(animation, bgcolor_animation_.get());

  const SkColor color = gfx::Tween::ColorValueBetween(
      animation->GetCurrentValue(), bgcolor_origin_, bgcolor_target_);
  SetBackground(views::CreateSolidBackground(color));
  SchedulePaint();
}

void NotificationControlButtonsView::AnimationEnded(
    const gfx::Animation* animation) {
  DCHECK_EQ(animation, bgcolor_animation_.get());
  bgcolor_animation_.reset();
  bgcolor_origin_ = bgcolor_target_;
}

void NotificationControlButtonsView::AnimationCanceled(
    const gfx::Animation* animation) {
  // The animation is never cancelled explicitly.
  NOTREACHED();

  bgcolor_origin_ = bgcolor_target_;
  SetBackground(views::CreateSolidBackground(bgcolor_target_));
  SchedulePaint();
}

}  // namespace message_center
