// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/toolbar/app_toolbar_button.h"

#include "base/macros.h"
#include "base/metrics/field_trial_params.h"
#include "chrome/app/vector_icons/vector_icons.h"
#import "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/ui/cocoa/animated_icon.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/chromium_strings.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

namespace {

// The duration of the delay in seconds.
constexpr NSTimeInterval kAnimationDelay = 1.5;

}  // namespace.

@interface AppToolbarButton ()
- (void)commonInit;
- (void)updateAnimatedIconColor;
- (SkColor)vectorIconBaseColor:(BOOL)themeIsDark;
- (void)updateAnimatedIconColor;
- (void)animateAndResetTimer;
@end

@implementation AppToolbarButton

- (instancetype)initWithFrame:(NSRect)frame {
  if ((self = [super initWithFrame:frame])) {
    [self commonInit];

    if (base::FeatureList::IsEnabled(features::kAnimatedAppMenuIcon)) {
      animatedIcon_.reset(new AnimatedIcon(kBrowserToolsAnimatedIcon, self));
      [self updateAnimatedIconColor];

      NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
      [center addObserver:self
                 selector:@selector(themeDidChangeNotification:)
                     name:kBrowserThemeDidChangeNotification
                   object:nil];

      isDelayEnabled_ = base::GetFieldTrialParamByFeatureAsBool(
          features::kAnimatedAppMenuIcon, "HasDelay", false);
    }
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];

  // Release the timer.
  [animationDelayTimer_ invalidate];
  animationDelayTimer_ = nil;

  [super dealloc];
}

- (void)awakeFromNib {
  [self commonInit];
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  [self updateAnimatedIconColor];
}

- (void)drawRect:(NSRect)frame {
  [super drawRect:frame];

  if (animatedIcon_) {
    gfx::ScopedNSGraphicsContextSaveGState save_graphics_state;
    NSAffineTransform* transform = [NSAffineTransform transform];
    [transform translateXBy:-0.5 yBy:0];
    [transform concat];
    animatedIcon_->PaintIcon(frame);
  }
}

- (void)commonInit {
  view_id_util::SetID(self, VIEW_ID_APP_MENU);
  severity_ = AppMenuIconController::Severity::NONE;
  type_ = AppMenuIconController::IconType::NONE;
  [self setToolTip:l10n_util::GetNSString(IDS_APPMENU_TOOLTIP)];
}

- (SkColor)vectorIconBaseColor:(BOOL)themeIsDark {
  const ui::ThemeProvider* provider = [[self window] themeProvider];
  return themeIsDark ? SK_ColorWHITE
                     : (provider && provider->ShouldIncreaseContrast()
                            ? SK_ColorBLACK
                            : gfx::kChromeIconGrey);
}

- (void)updateAnimatedIconColor {
  if (!animatedIcon_)
    return;

  const ui::ThemeProvider* provider = [[self window] themeProvider];
  BOOL themeIsDark = [[self window] hasDarkTheme];
  SkColor color = provider && provider->UsingSystemTheme()
                      ? [self vectorIconColor:themeIsDark]
                      : [self vectorIconBaseColor:themeIsDark];
  animatedIcon_->set_color(color);
}

- (const gfx::VectorIcon*)vectorIcon {
  if (animatedIcon_)
    return nullptr;

  switch (type_) {
    case AppMenuIconController::IconType::NONE:
      DCHECK_EQ(severity_, AppMenuIconController::Severity::NONE);
      return &kBrowserToolsIcon;
    case AppMenuIconController::IconType::UPGRADE_NOTIFICATION:
      return &kBrowserToolsUpdateIcon;
    case AppMenuIconController::IconType::GLOBAL_ERROR:
    case AppMenuIconController::IconType::INCOMPATIBILITY_WARNING:
      return &kBrowserToolsErrorIcon;
  }

  return nullptr;
}

- (SkColor)vectorIconColor:(BOOL)themeIsDark {
  switch (severity_) {
    case AppMenuIconController::Severity::NONE:
      return [self vectorIconBaseColor:themeIsDark];
      break;

    case AppMenuIconController::Severity::LOW:
      return themeIsDark ? gfx::kGoogleGreen300 : gfx::kGoogleGreen700;
      break;

    case AppMenuIconController::Severity::MEDIUM:
      return themeIsDark ? gfx::kGoogleYellow300 : gfx::kGoogleYellow700;
      break;

    case AppMenuIconController::Severity::HIGH:
      return themeIsDark ? gfx::kGoogleRed300 : gfx::kGoogleRed700;
      break;

    default:
      break;
  }
}

- (void)setSeverity:(AppMenuIconController::Severity)severity
           iconType:(AppMenuIconController::IconType)type
      shouldAnimate:(BOOL)shouldAnimate {
  if (severity != severity_ || type != type_) {
    severity_ = severity;
    type_ = type;

    [self setToolTip:severity_ == AppMenuIconController::Severity::NONE
                         ? l10n_util::GetNSString(IDS_APPMENU_TOOLTIP)
                         : l10n_util::GetNSString(
                               IDS_APPMENU_TOOLTIP_UPDATE_AVAILABLE)];

    if (animatedIcon_) {
      [self updateAnimatedIconColor];
      [self animateIfPossibleWithDelay:YES];
    }
    // Update the button state images with the new severity color or icon
    // type.
    [self resetButtonStateImages];
  }
}

- (void)themeDidChangeNotification:(NSNotification*)aNotification {
  [self updateAnimatedIconColor];
}

- (void)animateAndResetTimer {
  animatedIcon_->Animate();

  [animationDelayTimer_ invalidate];
  animationDelayTimer_ = nil;
}

- (void)animateIfPossibleWithDelay:(BOOL)delay {
  if (!animatedIcon_ || severity_ == AppMenuIconController::Severity::NONE)
    return;

  if (!isDelayEnabled_ || animatedIcon_->IsAnimating()) {
    // The timer should be nil if the icon is already animating or
    // the feature's "HasDelay" param is false.
    DCHECK(!animationDelayTimer_);
    animatedIcon_->Animate();
    return;
  }

  // If there's no delay, fire the animation and remove the timer.
  if (!delay) {
    [self animateAndResetTimer];
    return;
  }

  // Set the delay timer.
  if (!animationDelayTimer_) {
    NSTimeInterval delayInterval =
        disableTimerForTesting_ ? 0 : kAnimationDelay;
    animationDelayTimer_ =
        [NSTimer timerWithTimeInterval:delayInterval
                                target:self
                              selector:@selector(animateAndResetTimer)
                              userInfo:nil
                               repeats:NO];
    [[NSRunLoop currentRunLoop] addTimer:animationDelayTimer_
                                 forMode:NSRunLoopCommonModes];
  }
}

@end

@implementation AppToolbarButton (ExposedForTest)

- (AnimatedIcon*)animatedIcon {
  return animatedIcon_.get();
}

- (void)setAnimatedIcon:(AnimatedIcon*)icon {
  animatedIcon_.reset(icon);
}

- (NSTimer*)animationDelayTimer {
  return animationDelayTimer_;
}

- (void)setDisableTimerForTest:(BOOL)disable {
  disableTimerForTesting_ = disable;
}

@end
