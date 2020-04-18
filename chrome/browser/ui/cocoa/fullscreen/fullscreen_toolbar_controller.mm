// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_controller.h"

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_menubar_tracker.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_animation_controller.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_mouse_tracker.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_visibility_lock_controller.h"
#import "chrome/browser/ui/cocoa/fullscreen/immersive_fullscreen_controller.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"

namespace {

// Visibility fractions for the menubar and toolbar.
const CGFloat kHideFraction = 0.0;
const CGFloat kShowFraction = 1.0;

void RecordToolbarStyle(FullscreenToolbarStyle style) {
  UMA_HISTOGRAM_ENUMERATION("OSX.Fullscreen.ToolbarStyle", style,
                            kFullscreenToolbarStyleCount);
}

}  // namespace

@interface FullscreenToolbarController ()
// Updates |toolbarStyle_|.
- (void)updateToolbarStyle:(BOOL)isExitingTabFullscreen;
@end

@implementation FullscreenToolbarController

- (id)initWithBrowserController:(BrowserWindowController*)controller {
  if ((self = [super init])) {
    browserController_ = controller;
    animationController_.reset(new FullscreenToolbarAnimationController(self));
    visibilityLockController_.reset(
        [[FullscreenToolbarVisibilityLockController alloc]
            initWithFullscreenToolbarController:self
                            animationController:animationController_.get()]);
  }

  return self;
}

- (void)dealloc {
  DCHECK(!inFullscreenMode_);
  [super dealloc];
}

- (void)enterFullscreenMode {
  DCHECK(!inFullscreenMode_);
  inFullscreenMode_ = YES;

  [self updateToolbarStyle:NO];
  RecordToolbarStyle(toolbarStyle_);

  if ([browserController_ isInImmersiveFullscreen]) {
    immersiveFullscreenController_.reset([[ImmersiveFullscreenController alloc]
        initWithBrowserController:browserController_]);
    [immersiveFullscreenController_ updateMenuBarAndDockVisibility];
  } else {
    menubarTracker_.reset([[FullscreenMenubarTracker alloc]
        initWithFullscreenToolbarController:self]);
    mouseTracker_.reset([[FullscreenToolbarMouseTracker alloc]
        initWithFullscreenToolbarController:self]);
  }
}

- (void)exitFullscreenMode {
  DCHECK(inFullscreenMode_);
  inFullscreenMode_ = NO;

  animationController_->StopAnimationAndTimer();
  [[NSNotificationCenter defaultCenter] removeObserver:self];

  menubarTracker_.reset();
  mouseTracker_.reset();
  immersiveFullscreenController_.reset();

  // No more calls back up to the BWC.
  browserController_ = nil;
}

// Cancels any running animation and timers.
- (void)cancelAnimationAndTimer {
  animationController_->StopAnimationAndTimer();
}

- (void)revealToolbarForWebContents:(content::WebContents*)contents
                       inForeground:(BOOL)inForeground {
  if (!base::FeatureList::IsEnabled(features::kFullscreenToolbarReveal))
    return;

  animationController_->AnimateToolbarForTabstripChanges(contents,
                                                         inForeground);
}

- (CGFloat)toolbarFraction {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kKioskMode))
    return kHideFraction;

  switch (toolbarStyle_) {
    case FullscreenToolbarStyle::TOOLBAR_PRESENT:
      return kShowFraction;
    case FullscreenToolbarStyle::TOOLBAR_NONE:
      return kHideFraction;
    case FullscreenToolbarStyle::TOOLBAR_HIDDEN:
      if (animationController_->IsAnimationRunning())
        return animationController_->GetToolbarFractionFromProgress();

      if ([self mustShowFullscreenToolbar])
        return kShowFraction;

      return [menubarTracker_ menubarFraction];
  }
}

- (FullscreenToolbarStyle)toolbarStyle {
  return toolbarStyle_;
}

- (FullscreenToolbarLayout)computeLayout {
  FullscreenToolbarLayout layout;
  layout.toolbarStyle = toolbarStyle_;
  layout.toolbarFraction = [self toolbarFraction];

  // Calculate how much the toolbar is offset downwards to avoid the menu.
  if ([browserController_ isInAppKitFullscreen]) {
    layout.menubarOffset = [menubarTracker_ menubarFraction];
  } else {
    layout.menubarOffset =
        [immersiveFullscreenController_ shouldShowMenubar] ? 1 : 0;
  }
  layout.menubarOffset *= -[browserController_ menubarHeight];

  return layout;
}

- (BOOL)mustShowFullscreenToolbar {
  if (!inFullscreenMode_)
    return NO;

  if (toolbarStyle_ == FullscreenToolbarStyle::TOOLBAR_PRESENT)
    return YES;

  if (toolbarStyle_ == FullscreenToolbarStyle::TOOLBAR_NONE)
    return NO;

  FullscreenMenubarState menubarState = [menubarTracker_ state];
  return menubarState == FullscreenMenubarState::SHOWN ||
         [visibilityLockController_ isToolbarVisibilityLocked];
}

- (void)updateToolbarFrame:(NSRect)frame {
  if (mouseTracker_.get())
    [mouseTracker_ updateToolbarFrame:frame];
}

- (void)updateToolbarStyle:(BOOL)isExitingTabFullscreen {
  if ([browserController_ isFullscreenForTabContentOrExtension] &&
      !isExitingTabFullscreen) {
    toolbarStyle_ = FullscreenToolbarStyle::TOOLBAR_NONE;
  } else {
    PrefService* prefs = [browserController_ profile]->GetPrefs();
    toolbarStyle_ = prefs->GetBoolean(prefs::kShowFullscreenToolbar)
                        ? FullscreenToolbarStyle::TOOLBAR_PRESENT
                        : FullscreenToolbarStyle::TOOLBAR_HIDDEN;
  }
}

- (void)layoutToolbarStyleIsExitingTabFullscreen:(BOOL)isExitingTabFullscreen {
  FullscreenToolbarStyle oldStyle = toolbarStyle_;
  [self updateToolbarStyle:isExitingTabFullscreen];

  if (oldStyle != toolbarStyle_) {
    [self layoutToolbar];
    RecordToolbarStyle(toolbarStyle_);
  }
}

- (void)layoutToolbar {
  [browserController_ layoutSubviews];
  animationController_->ToolbarDidUpdate();
  [mouseTracker_ updateTrackingArea];
}

- (BOOL)isInFullscreen {
  return inFullscreenMode_;
}

- (BrowserWindowController*)browserWindowController {
  return browserController_;
}

- (FullscreenToolbarVisibilityLockController*)visibilityLockController {
  return visibilityLockController_.get();
}

@end

@implementation FullscreenToolbarController (ExposedForTesting)

- (FullscreenToolbarAnimationController*)animationController {
  return animationController_.get();
}

- (void)setMenubarTracker:(FullscreenMenubarTracker*)tracker {
  menubarTracker_.reset([tracker retain]);
}

- (void)setMouseTracker:(FullscreenToolbarMouseTracker*)tracker {
  mouseTracker_.reset([tracker retain]);
}

- (void)setToolbarStyle:(FullscreenToolbarStyle)style {
  toolbarStyle_ = style;
}

- (void)setTestFullscreenMode:(BOOL)isInFullscreen {
  inFullscreenMode_ = isInFullscreen;
}

@end
