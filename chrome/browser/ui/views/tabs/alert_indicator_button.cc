// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"

#include "base/macros.h"
#include "base/metrics/user_metrics.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_controller.h"
#include "chrome/browser/ui/views/tabs/tab_renderer_data.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image.h"
#include "ui/views/metrics.h"

using base::UserMetricsAction;

namespace {

// The minimum required click-to-select area of an inactive Tab before allowing
// the click-to-mute functionality to be enabled.  These values are in terms of
// some percentage of the AlertIndicatorButton's width.  See comments in
// UpdateEnabledForMuteToggle().
const int kMinMouseSelectableAreaPercent = 250;
const int kMinGestureSelectableAreaPercent = 400;

// Returns true if either Shift or Control are being held down.  In this case,
// mouse events are delegated to the Tab, to perform tab selection in the tab
// strip instead.
bool IsShiftOrControlDown(const ui::Event& event) {
  return (event.flags() & (ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN)) != 0;
}

}  // namespace

const char AlertIndicatorButton::kViewClassName[] = "AlertIndicatorButton";

class AlertIndicatorButton::FadeAnimationDelegate
    : public gfx::AnimationDelegate {
 public:
  explicit FadeAnimationDelegate(AlertIndicatorButton* button)
      : button_(button) {}
  ~FadeAnimationDelegate() override {}

 private:
  // gfx::AnimationDelegate
  void AnimationProgressed(const gfx::Animation* animation) override {
    button_->SchedulePaint();
  }

  void AnimationCanceled(const gfx::Animation* animation) override {
    AnimationEnded(animation);
  }

  void AnimationEnded(const gfx::Animation* animation) override {
    button_->showing_alert_state_ = button_->alert_state_;
    button_->SchedulePaint();
    button_->parent_tab_->AlertStateChanged();
  }

  AlertIndicatorButton* const button_;

  DISALLOW_COPY_AND_ASSIGN(FadeAnimationDelegate);
};

AlertIndicatorButton::AlertIndicatorButton(Tab* parent_tab)
    : views::ImageButton(nullptr),
      parent_tab_(parent_tab),
      alert_state_(TabAlertState::NONE),
      showing_alert_state_(TabAlertState::NONE) {
  DCHECK(parent_tab_);
  SetEventTargeter(
      std::unique_ptr<views::ViewTargeter>(new views::ViewTargeter(this)));

  // Disable animations of hover state change, to be consistent with the
  // behavior of the tab close button.
  set_animate_on_state_change(false);
}

AlertIndicatorButton::~AlertIndicatorButton() {}

void AlertIndicatorButton::TransitionToAlertState(TabAlertState next_state) {
  if (next_state == alert_state_)
    return;

  TabAlertState previous_alert_showing_state = showing_alert_state_;

  if (next_state != TabAlertState::NONE)
    ResetImages(next_state);

  if ((alert_state_ == TabAlertState::AUDIO_PLAYING &&
       next_state == TabAlertState::AUDIO_MUTING) ||
      (alert_state_ == TabAlertState::AUDIO_MUTING &&
       next_state == TabAlertState::AUDIO_PLAYING)) {
    // Instant user feedback: No fade animation.
    showing_alert_state_ = next_state;
    fade_animation_.reset();
  } else {
    if (next_state == TabAlertState::NONE)
      showing_alert_state_ = alert_state_;  // Fading-out indicator.
    else
      showing_alert_state_ = next_state;  // Fading-in to next indicator.
    fade_animation_ = chrome::CreateTabAlertIndicatorFadeAnimation(next_state);
    if (!fade_animation_delegate_)
      fade_animation_delegate_.reset(new FadeAnimationDelegate(this));
    fade_animation_->set_delegate(fade_animation_delegate_.get());
    fade_animation_->Start();
  }

  alert_state_ = next_state;

  if (previous_alert_showing_state != showing_alert_state_)
    parent_tab_->AlertStateChanged();

  UpdateEnabledForMuteToggle();
}

void AlertIndicatorButton::UpdateEnabledForMuteToggle() {
  const bool was_enabled = enabled();

  bool enable = chrome::AreExperimentalMuteControlsEnabled() &&
                (alert_state_ == TabAlertState::AUDIO_PLAYING ||
                 alert_state_ == TabAlertState::AUDIO_MUTING);

  // If the tab is not the currently-active tab, make sure it is wide enough
  // before enabling click-to-mute.  This ensures that there is enough click
  // area for the user to activate a tab rather than unintentionally muting it.
  // Note that IsTriggerableEvent() is also overridden to provide an even wider
  // requirement for tap gestures.
  if (enable && !GetTab()->IsActive()) {
    const int required_width = width() * kMinMouseSelectableAreaPercent / 100;
    enable = (GetTab()->GetWidthOfLargestSelectableRegion() >= required_width);
  }

  if (enable == was_enabled)
    return;

  SetEnabled(enable);

  // If the button has become enabled, check whether the mouse is currently
  // hovering.  If it is, enter a dormant period where extra user clicks are
  // prevented from having an effect (i.e., before the user has realized the
  // button has become enabled underneath their cursor).
  if (!was_enabled && state() == views::Button::STATE_HOVERED)
    EnterDormantPeriod();
  else if (!enabled())
    ExitDormantPeriod();
}

void AlertIndicatorButton::OnParentTabButtonColorChanged() {
  if (alert_state_ == TabAlertState::AUDIO_PLAYING ||
      alert_state_ == TabAlertState::AUDIO_MUTING)
    ResetImages(alert_state_);
}

const char* AlertIndicatorButton::GetClassName() const {
  return kViewClassName;
}

views::View* AlertIndicatorButton::GetTooltipHandlerForPoint(
    const gfx::Point& point) {
  return nullptr;  // Tab (the parent View) provides the tooltip.
}

bool AlertIndicatorButton::OnMousePressed(const ui::MouseEvent& event) {
  // Do not handle this mouse event when anything but the left mouse button is
  // pressed or when any modifier keys are being held down.  Instead, the Tab
  // should react (e.g., middle-click for close, right-click for context menu).
  if (!event.IsOnlyLeftMouseButton() || IsShiftOrControlDown(event)) {
    if (state() != views::Button::STATE_DISABLED)
      SetState(views::Button::STATE_NORMAL);  // Turn off hover.
    return false;  // Event to be handled by Tab.
  }
  return ImageButton::OnMousePressed(event);
}

bool AlertIndicatorButton::OnMouseDragged(const ui::MouseEvent& event) {
  const ButtonState previous_state = state();
  const bool ret = ImageButton::OnMouseDragged(event);
  if (previous_state != views::Button::STATE_NORMAL &&
      state() == views::Button::STATE_NORMAL)
    base::RecordAction(UserMetricsAction("AlertIndicatorButton_Dragged"));
  return ret;
}

void AlertIndicatorButton::OnMouseEntered(const ui::MouseEvent& event) {
  // If any modifier keys are being held down, do not turn on hover.
  if (state() != views::Button::STATE_DISABLED && IsShiftOrControlDown(event)) {
    SetState(views::Button::STATE_NORMAL);
    return;
  }
  ImageButton::OnMouseEntered(event);
}

void AlertIndicatorButton::OnMouseExited(const ui::MouseEvent& event) {
  ExitDormantPeriod();
  ImageButton::OnMouseExited(event);
}

void AlertIndicatorButton::OnMouseMoved(const ui::MouseEvent& event) {
  // If any modifier keys are being held down, turn off hover.
  if (state() != views::Button::STATE_DISABLED && IsShiftOrControlDown(event)) {
    SetState(views::Button::STATE_NORMAL);
    return;
  }
  ImageButton::OnMouseMoved(event);
}

void AlertIndicatorButton::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  UpdateEnabledForMuteToggle();
}

bool AlertIndicatorButton::DoesIntersectRect(const views::View* target,
                                             const gfx::Rect& rect) const {
  // If this button is not enabled, Tab (the parent View) handles all mouse
  // events.
  return enabled() &&
         views::ViewTargeterDelegate::DoesIntersectRect(target, rect);
}

void AlertIndicatorButton::NotifyClick(const ui::Event& event) {
  EnterDormantPeriod();

  // Call TransitionToAlertState() to change the image, providing the user with
  // instant feedback.  In the very unlikely event that the mute toggle fails,
  // TransitionToAlertState() will be called again, via another code path, to
  // set the image to be consistent with the final outcome.
  if (alert_state_ == TabAlertState::AUDIO_PLAYING) {
    base::RecordAction(UserMetricsAction("AlertIndicatorButton_Mute"));
    TransitionToAlertState(TabAlertState::AUDIO_MUTING);
  } else {
    DCHECK(alert_state_ == TabAlertState::AUDIO_MUTING);
    base::RecordAction(UserMetricsAction("AlertIndicatorButton_Unmute"));
    TransitionToAlertState(TabAlertState::AUDIO_PLAYING);
  }

  GetTab()->controller()->ToggleTabAudioMute(GetTab());
}

bool AlertIndicatorButton::IsTriggerableEvent(const ui::Event& event) {
  if (is_dormant())
    return false;

  // For mouse events, only trigger on the left mouse button and when no
  // modifier keys are being held down.
  if (event.IsMouseEvent() &&
      (!static_cast<const ui::MouseEvent*>(&event)->IsOnlyLeftMouseButton() ||
       IsShiftOrControlDown(event)))
    return false;

  // For gesture events on an inactive tab, require an even wider tab before
  // click-to-mute can be triggered.  See comments in
  // UpdateEnabledForMuteToggle().
  if (event.IsGestureEvent() && !GetTab()->IsActive()) {
    const int required_width = width() * kMinGestureSelectableAreaPercent / 100;
    if (GetTab()->GetWidthOfLargestSelectableRegion() < required_width)
      return false;
  }

  return views::ImageButton::IsTriggerableEvent(event);
}

void AlertIndicatorButton::PaintButtonContents(gfx::Canvas* canvas) {
  double opaqueness = 1.0;
  if (fade_animation_) {
    opaqueness = fade_animation_->GetCurrentValue();
    if (alert_state_ == TabAlertState::NONE)
      opaqueness = 1.0 - opaqueness;  // Fading out, not in.
  } else if (is_dormant()) {
    opaqueness = 0.5;
  }
  if (opaqueness < 1.0)
    canvas->SaveLayerAlpha(opaqueness * SK_AlphaOPAQUE);
  ImageButton::PaintButtonContents(canvas);
  if (opaqueness < 1.0)
    canvas->Restore();
}

gfx::ImageSkia AlertIndicatorButton::GetImageToPaint() {
  if (is_dormant())
    return views::ImageButton::images_[views::Button::STATE_NORMAL];
  return views::ImageButton::GetImageToPaint();
}

Tab* AlertIndicatorButton::GetTab() const {
  DCHECK_EQ(static_cast<views::View*>(parent_tab_), parent());
  return parent_tab_;
}

void AlertIndicatorButton::ResetImages(TabAlertState state) {
  SkColor color = parent_tab_->GetAlertIndicatorColor(state);
  gfx::ImageSkia indicator_image =
      chrome::GetTabAlertIndicatorImage(state, color).AsImageSkia();
  SetImage(views::Button::STATE_NORMAL, &indicator_image);
  SetImage(views::Button::STATE_DISABLED, &indicator_image);
  gfx::ImageSkia affordance_image =
      chrome::GetTabAlertIndicatorAffordanceImage(state, color).AsImageSkia();
  SetImage(views::Button::STATE_HOVERED, &affordance_image);
  SetImage(views::Button::STATE_PRESSED, &affordance_image);
}

void AlertIndicatorButton::EnterDormantPeriod() {
  wake_up_timer_.reset(new base::OneShotTimer());
  wake_up_timer_->Start(
      FROM_HERE,
      base::TimeDelta::FromMilliseconds(views::GetDoubleClickInterval()),
      this,
      &AlertIndicatorButton::ExitDormantPeriod);
  SchedulePaint();
}

void AlertIndicatorButton::ExitDormantPeriod() {
  const bool needs_repaint = is_dormant();
  wake_up_timer_.reset();
  if (needs_repaint)
    SchedulePaint();
}
