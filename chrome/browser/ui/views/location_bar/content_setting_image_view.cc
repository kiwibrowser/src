// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/content_setting_image_view.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"
#include "chrome/browser/ui/content_settings/content_setting_image_model.h"
#include "chrome/browser/ui/views/content_setting_bubble_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/theme_provider.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/widget/widget.h"

namespace {
// Time spent with animation fully open.
const int kStayOpenTimeMS = 3200;
}

// static
const int ContentSettingImageView::kAnimationDurationMS =
    (IconLabelBubbleView::kOpenTimeMS * 2) + kStayOpenTimeMS;

ContentSettingImageView::ContentSettingImageView(
    std::unique_ptr<ContentSettingImageModel> image_model,
    Delegate* delegate,
    const gfx::FontList& font_list)
    : IconLabelBubbleView(font_list),
      delegate_(delegate),
      content_setting_image_model_(std::move(image_model)),
      slide_animator_(this),
      pause_animation_(false),
      pause_animation_state_(0.0),
      bubble_view_(nullptr) {
  DCHECK(delegate_);
  SetInkDropMode(InkDropMode::ON);
  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
  image()->EnableCanvasFlippingForRTLUI(true);
  label()->SetElideBehavior(gfx::NO_ELIDE);
  label()->SetVisible(false);

  slide_animator_.SetSlideDuration(kAnimationDurationMS);
  slide_animator_.SetTweenType(gfx::Tween::LINEAR);
}

ContentSettingImageView::~ContentSettingImageView() {
  if (bubble_view_ && bubble_view_->GetWidget())
    bubble_view_->GetWidget()->RemoveObserver(this);
}

void ContentSettingImageView::Update() {
  content::WebContents* web_contents =
      delegate_->GetContentSettingWebContents();
  // Note: We explicitly want to call this even if |web_contents| is NULL, so we
  // get hidden properly while the user is editing the omnibox.
  content_setting_image_model_->UpdateFromWebContents(web_contents);

  if (!content_setting_image_model_->is_visible()) {
    SetVisible(false);
    return;
  }

  UpdateImage();
  SetVisible(true);

  // If the content usage or blockage should be indicated to the user, start the
  // animation and record that the icon has been shown.
  if (!can_animate_ ||
      !content_setting_image_model_->ShouldRunAnimation(web_contents)) {
    return;
  }

  // We just ignore this blockage if we're already showing some other string to
  // the user.  If this becomes a problem, we could design some sort of queueing
  // mechanism to show one after the other, but it doesn't seem important now.
  int string_id = content_setting_image_model_->explanatory_string_id();
  AnimateInkDrop(views::InkDropState::HIDDEN, nullptr /* event */);
  if (string_id && !label()->visible()) {
    SetLabel(l10n_util::GetStringUTF16(string_id));
    label()->SetVisible(true);
    AnimateIn();
  }

  content_setting_image_model_->SetAnimationHasRun(web_contents);
}

void ContentSettingImageView::SetIconColor(SkColor color) {
  icon_color_ = color;
  if (content_setting_image_model_->is_visible())
    UpdateImage();
}

const char* ContentSettingImageView::GetClassName() const {
  return "ContentSettingsImageView";
}

void ContentSettingImageView::OnBoundsChanged(
    const gfx::Rect& previous_bounds) {
  if (bubble_view_)
    bubble_view_->OnAnchorBoundsChanged();
  IconLabelBubbleView::OnBoundsChanged(previous_bounds);
}

bool ContentSettingImageView::GetTooltipText(const gfx::Point& p,
                                             base::string16* tooltip) const {
  *tooltip = content_setting_image_model_->get_tooltip();
  return !tooltip->empty();
}

void ContentSettingImageView::OnNativeThemeChanged(
    const ui::NativeTheme* native_theme) {
  UpdateImage();
  IconLabelBubbleView::OnNativeThemeChanged(native_theme);
}

SkColor ContentSettingImageView::GetTextColor() const {
  return GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_TextfieldDefaultColor);
}

bool ContentSettingImageView::ShouldShowLabel() const {
  return (!IsShrinking() || (width() > image()->GetPreferredSize().width())) &&
         (slide_animator_.is_animating() || pause_animation_);
}

bool ContentSettingImageView::ShouldShowSeparator() const {
  return false;
}

double ContentSettingImageView::WidthMultiplier() const {
  double state = pause_animation_ ? pause_animation_state_
                                  : slide_animator_.GetCurrentValue();
  // The fraction of the animation we'll spend animating the string into view,
  // which is also the fraction we'll spend animating it closed; total
  // animation (slide out, show, then slide in) is 1.0.
  const double kOpenFraction =
      static_cast<double>(kOpenTimeMS) / kAnimationDurationMS;
  double size_fraction = 1.0;
  if (state < kOpenFraction)
    size_fraction = state / kOpenFraction;
  if (state > (1.0 - kOpenFraction))
    size_fraction = (1.0 - state) / kOpenFraction;
  return size_fraction;
}

bool ContentSettingImageView::IsShrinking() const {
  const double kOpenFraction =
      static_cast<double>(kOpenTimeMS) / kAnimationDurationMS;
  return (!pause_animation_ && slide_animator_.is_animating() &&
          slide_animator_.GetCurrentValue() > (1.0 - kOpenFraction));
}

bool ContentSettingImageView::ShowBubble(const ui::Event& event) {
  if (slide_animator_.is_animating()) {
    // If the user clicks while we're animating, the bubble arrow will be
    // pointing to the image, and if we allow the animation to keep running, the
    // image will move away from the arrow (or we'll have to move the bubble,
    // which is even worse). So we want to stop the animation.  We have two
    // choices: jump to the final post-animation state (no label visible), or
    // pause the animation where we are and continue running after the bubble
    // closes. The former looks more jerky, so we avoid it unless the animation
    // hasn't even fully exposed the image yet, in which case pausing with half
    // an image visible will look broken.
    if (!pause_animation_ && ShouldShowLabel()) {
      pause_animation_ = true;
      pause_animation_state_ = slide_animator_.GetCurrentValue();
    }
    slide_animator_.Reset();
  }

  content::WebContents* web_contents =
      delegate_->GetContentSettingWebContents();
  if (web_contents && !bubble_view_) {
    views::View* anchor = this;
    if (ui::MaterialDesignController::IsSecondaryUiMaterial())
      anchor = parent();
    bubble_view_ = new ContentSettingBubbleContents(
        content_setting_image_model_->CreateBubbleModel(
            delegate_->GetContentSettingBubbleModelDelegate(), web_contents,
            Profile::FromBrowserContext(web_contents->GetBrowserContext())),
        web_contents, anchor, views::BubbleBorder::TOP_RIGHT);
    views::Widget* bubble_widget =
        views::BubbleDialogDelegateView::CreateBubble(bubble_view_);
    bubble_widget->AddObserver(this);
    // This is triggered by an input event. If the user clicks the icon while
    // it's not animating, the icon will be placed in an active state, so the
    // bubble doesn't need an arrow. If the user clicks during an animation,
    // the animation simply pauses and no other visible state change occurs, so
    // show the arrow in this case.
    if (!pause_animation_) {
      AnimateInkDrop(views::InkDropState::ACTIVATED,
                     ui::LocatedEvent::FromIfValid(&event));
      bubble_view_->SetArrowPaintType(views::BubbleBorder::PAINT_TRANSPARENT);
    }
    bubble_widget->Show();
    delegate_->OnContentSettingImageBubbleShown(
        content_setting_image_model_->image_type());
  }

  return true;
}

bool ContentSettingImageView::IsBubbleShowing() const {
  return bubble_view_ != nullptr;
}

ContentSettingImageModel::ImageType ContentSettingImageView::GetTypeForTesting()
    const {
  return content_setting_image_model_->image_type();
}

SkColor ContentSettingImageView::GetInkDropBaseColor() const {
  return icon_color_ ? icon_color_.value()
                     : IconLabelBubbleView::GetInkDropBaseColor();
}

void ContentSettingImageView::AnimationEnded(const gfx::Animation* animation) {
  slide_animator_.Reset();
  if (!pause_animation_) {
    label()->SetVisible(false);
    parent()->Layout();
    parent()->SchedulePaint();
  }

  GetInkDrop()->SetShowHighlightOnHover(true);
  GetInkDrop()->SetShowHighlightOnFocus(true);
}

void ContentSettingImageView::AnimationProgressed(
    const gfx::Animation* animation) {
  if (!pause_animation_) {
    parent()->Layout();
    parent()->SchedulePaint();
  }
}

void ContentSettingImageView::AnimationCanceled(
    const gfx::Animation* animation) {
  AnimationEnded(animation);
}

void ContentSettingImageView::OnWidgetDestroying(views::Widget* widget) {
  DCHECK(bubble_view_);
  DCHECK_EQ(bubble_view_->GetWidget(), widget);
  widget->RemoveObserver(this);
  bubble_view_ = nullptr;

  if (pause_animation_) {
    slide_animator_.Reset(pause_animation_state_);
    pause_animation_ = false;
    AnimateIn();
  }
}

void ContentSettingImageView::OnWidgetVisibilityChanged(views::Widget* widget,
                                                        bool visible) {
  // |widget| is a bubble that has just got shown / hidden.
  if (!visible)
    AnimateInkDrop(views::InkDropState::DEACTIVATED, nullptr /* event */);
}

void ContentSettingImageView::UpdateImage() {
  SetImage(content_setting_image_model_
               ->GetIcon(icon_color_ ? icon_color_.value()
                                     : color_utils::DeriveDefaultIconColor(
                                           GetTextColor()))
               .AsImageSkia());
}

void ContentSettingImageView::AnimateIn() {
  slide_animator_.Show();
  GetInkDrop()->SetShowHighlightOnHover(false);
  GetInkDrop()->SetShowHighlightOnFocus(false);
}
