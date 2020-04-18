// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_icon.h"

#include "cc/paint/paint_flags.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/common/webui_url_constants.h"
#include "components/grit/components_scaled_resources.h"
#include "content/public/common/url_constants.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/linear_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/native_theme/native_theme.h"
#include "ui/resources/grit/ui_resources.h"
#include "url/gurl.h"

namespace {

constexpr int kAttentionIndicatorRadius = 3;

// Returns whether the favicon for the given URL should be colored according to
// the browser theme.
bool ShouldThemifyFaviconForUrl(const GURL& url) {
  return url.SchemeIs(content::kChromeUIScheme) &&
         url.host_piece() != chrome::kChromeUIHelpHost &&
         url.host_piece() != chrome::kChromeUIUberHost &&
         url.host_piece() != chrome::kChromeUIAppLauncherPageHost;
}

}  // namespace

// Helper class that manages the favicon crash animation.
class TabIcon::CrashAnimation : public gfx::LinearAnimation,
                                public gfx::AnimationDelegate {
 public:
  explicit CrashAnimation(TabIcon* target)
      : gfx::LinearAnimation(base::TimeDelta::FromSeconds(1), 25, this),
        target_(target) {}
  ~CrashAnimation() override = default;

  // gfx::Animation overrides:
  void AnimateToState(double state) override {
    if (state < .5) {
      // Animate the normal icon down.
      target_->hiding_fraction_ = state * 2.0;
    } else {
      // Animate the crashed icon up.
      target_->should_display_crashed_favicon_ = true;
      target_->hiding_fraction_ = 1.0 - (state - 0.5) * 2.0;
    }
    target_->SchedulePaint();
  }

 private:
  TabIcon* target_;

  DISALLOW_COPY_AND_ASSIGN(CrashAnimation);
};

TabIcon::TabIcon() {
  set_can_process_events_within_subtree(false);

  // The minimum size to avoid clipping the attention indicator.
  SetPreferredSize(gfx::Size(gfx::kFaviconSize + kAttentionIndicatorRadius,
                             gfx::kFaviconSize + kAttentionIndicatorRadius));
}

TabIcon::~TabIcon() = default;

void TabIcon::SetIcon(const GURL& url, const gfx::ImageSkia& icon) {
  // Detect when updating to the same icon. This avoids re-theming and
  // re-painting.
  if (favicon_.BackedBySameObjectAs(icon))
    return;
  favicon_ = icon;

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  if (icon.BackedBySameObjectAs(*rb.GetImageSkiaNamed(IDR_DEFAULT_FAVICON)) ||
      ShouldThemifyFaviconForUrl(url)) {
    themed_favicon_ = ThemeImage(icon);
  } else {
    themed_favicon_ = gfx::ImageSkia();
  }
  SchedulePaint();
}

void TabIcon::SetNetworkState(TabNetworkState network_state,
                              bool inhibit_loading_animation) {
  if (network_state_ == network_state &&
      inhibit_loading_animation_ == inhibit_loading_animation)
    return;

  bool old_showing_load = ShowingLoadingAnimation();
  network_state_ = network_state;
  inhibit_loading_animation_ = inhibit_loading_animation;
  bool new_showing_load = ShowingLoadingAnimation();

  RefreshLayer();
  if (old_showing_load && !new_showing_load) {
    // Loading animation transitioning from on to off.
    waiting_start_time_ = base::TimeTicks();
    loading_start_time_ = base::TimeTicks();
    waiting_state_ = gfx::ThrobberWaitingState();
    SchedulePaint();
  } else if (!old_showing_load && new_showing_load) {
    // Loading animation transitioning from off to on. The animation painting
    // function will lazily initialize the data.
    SchedulePaint();
  }
}

void TabIcon::SetIsCrashed(bool is_crashed) {
  if (is_crashed == is_crashed_)
    return;
  is_crashed_ = is_crashed;

  if (!is_crashed_) {
    // Transitioned from crashed to non-crashed.
    if (crash_animation_)
      crash_animation_->Stop();
    should_display_crashed_favicon_ = false;
    hiding_fraction_ = 0.0;
  } else {
    // Transitioned from non-crashed to crashed.
    if (!crash_animation_)
      crash_animation_ = std::make_unique<CrashAnimation>(this);
    if (!crash_animation_->is_animating())
      crash_animation_->Start();
  }
  SchedulePaint();
}

void TabIcon::SetAttention(AttentionType type, bool enabled) {
  int previous_attention_type = attention_types_;
  if (enabled)
    attention_types_ |= static_cast<int>(type);
  else
    attention_types_ &= ~static_cast<int>(type);

  if (attention_types_ != previous_attention_type)
    SchedulePaint();
}

bool TabIcon::ShowingLoadingAnimation() const {
  if (inhibit_loading_animation_)
    return false;
  return network_state_ != TabNetworkState::kNone &&
         network_state_ != TabNetworkState::kError;
}

bool TabIcon::ShowingAttentionIndicator() const {
  return attention_types_ > 0;
}

void TabIcon::SetCanPaintToLayer(bool can_paint_to_layer) {
  if (can_paint_to_layer == can_paint_to_layer_)
    return;
  can_paint_to_layer_ = can_paint_to_layer;
  RefreshLayer();
}

void TabIcon::StepLoadingAnimation() {
  if (ShowingLoadingAnimation())
    SchedulePaint();
}

void TabIcon::OnPaint(gfx::Canvas* canvas) {
  // Compute the bounds adjusted for the hiding fraction.
  gfx::Rect contents_bounds = GetContentsBounds();
  if (contents_bounds.IsEmpty())
    return;
  gfx::Rect icon_bounds(
      GetMirroredXWithWidthInView(0, gfx::kFaviconSize),
      static_cast<int>(contents_bounds.height() * hiding_fraction_),
      std::min(gfx::kFaviconSize, contents_bounds.width()),
      std::min(gfx::kFaviconSize, contents_bounds.height()));

  // Loading animation.
  if (ShowingLoadingAnimation()) {
    PaintLoadingAnimation(canvas, icon_bounds);
    return;
  }

  // Figure out which icon to paint.
  gfx::ImageSkia* icon_to_paint = nullptr;
  if (should_display_crashed_favicon_) {
    if (crashed_icon_.isNull()) {
      // Lazily create a themed sad tab icon.
      ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
      crashed_icon_ = ThemeImage(*rb.GetImageSkiaNamed(IDR_CRASH_SAD_FAVICON));
    }
    icon_to_paint = &crashed_icon_;
  } else {
    if (themed_favicon_.isNull())
      icon_to_paint = &favicon_;
    else
      icon_to_paint = &themed_favicon_;
  }

  if (attention_types_ != 0 && !should_display_crashed_favicon_) {
    PaintAttentionIndicatorAndIcon(canvas, *icon_to_paint, icon_bounds);
  } else if (!icon_to_paint->isNull()) {
    canvas->DrawImageInt(*icon_to_paint, 0, 0, icon_bounds.width(),
                         icon_bounds.height(), icon_bounds.x(), icon_bounds.y(),
                         icon_bounds.width(), icon_bounds.height(), false);
  }
}

void TabIcon::OnThemeChanged() {
  crashed_icon_ = gfx::ImageSkia();  // Force recomputation if crashed.
  if (!themed_favicon_.isNull())
    themed_favicon_ = ThemeImage(favicon_);
}

void TabIcon::PaintAttentionIndicatorAndIcon(gfx::Canvas* canvas,
                                             const gfx::ImageSkia& icon,
                                             const gfx::Rect& bounds) {
  gfx::Point circle_center(
      bounds.x() + (base::i18n::IsRTL() ? 0 : gfx::kFaviconSize),
      bounds.y() + gfx::kFaviconSize);

  // The attention indicator consists of two parts:
  // . a clear (totally transparent) part over the bottom right (or left in rtl)
  //   of the favicon. This is done by drawing the favicon to a layer, then
  //   drawing the clear part on top of the favicon.
  // . a circle in the bottom right (or left in rtl) of the favicon.
  if (!icon.isNull()) {
    canvas->SaveLayerAlpha(0xff);
    canvas->DrawImageInt(icon, 0, 0, bounds.width(), bounds.height(),
                         bounds.x(), bounds.y(), bounds.width(),
                         bounds.height(), false);
    cc::PaintFlags clear_flags;
    clear_flags.setAntiAlias(true);
    clear_flags.setBlendMode(SkBlendMode::kClear);
    const float kIndicatorCropRadius = 4.5f;
    canvas->DrawCircle(circle_center, kIndicatorCropRadius, clear_flags);
    canvas->Restore();
  }

  // Draws the actual attention indicator.
  cc::PaintFlags indicator_flags;
  indicator_flags.setColor(GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_ProminentButtonColor));
  indicator_flags.setAntiAlias(true);
  canvas->DrawCircle(circle_center, kAttentionIndicatorRadius, indicator_flags);
}

void TabIcon::PaintLoadingAnimation(gfx::Canvas* canvas,
                                    const gfx::Rect& bounds) {
  const ui::ThemeProvider* tp = GetThemeProvider();
  if (network_state_ == TabNetworkState::kWaiting) {
    if (waiting_start_time_ == base::TimeTicks())
      waiting_start_time_ = base::TimeTicks::Now();

    waiting_state_.elapsed_time = base::TimeTicks::Now() - waiting_start_time_;
    gfx::PaintThrobberWaiting(
        canvas, bounds,
        tp->GetColor(ThemeProperties::COLOR_TAB_THROBBER_WAITING),
        waiting_state_.elapsed_time);
  } else {
    if (loading_start_time_ == base::TimeTicks())
      loading_start_time_ = base::TimeTicks::Now();

    waiting_state_.color =
        tp->GetColor(ThemeProperties::COLOR_TAB_THROBBER_WAITING);
    gfx::PaintThrobberSpinningAfterWaiting(
        canvas, bounds,
        tp->GetColor(ThemeProperties::COLOR_TAB_THROBBER_SPINNING),
        base::TimeTicks::Now() - loading_start_time_, &waiting_state_);
  }
}

void TabIcon::RefreshLayer() {
  // Since the loading animation can run for a long time, paint animation to a
  // separate layer when possible to reduce repaint overhead.
  bool should_paint_to_layer = can_paint_to_layer_ && ShowingLoadingAnimation();
  if (should_paint_to_layer == !!layer())
    return;

  // Change layer mode.
  if (should_paint_to_layer) {
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
  } else {
    DestroyLayer();
  }
}

gfx::ImageSkia TabIcon::ThemeImage(const gfx::ImageSkia& source) {
  return gfx::ImageSkiaOperations::CreateHSLShiftedImage(
      source, GetThemeProvider()->GetTint(ThemeProperties::TINT_BUTTONS));
}
