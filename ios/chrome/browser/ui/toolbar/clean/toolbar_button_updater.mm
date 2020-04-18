// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button_updater.h"

#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_factory.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/voice/voice_search_notification_names.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
enum ToolbarButtonMode {
  ToolbarButtonModeNormal,
  ToolbarButtonModeReversed,
};
}  // namespace

@interface ToolbarButtonUpdater ()

// Keeps track of whether or not the back button's images have been reversed.
@property(nonatomic, assign) ToolbarButtonMode backButtonMode;

// Keeps track of whether or not the forward button's images have been
// reversed.
@property(nonatomic, assign) ToolbarButtonMode forwardButtonMode;

@end

@implementation ToolbarButtonUpdater

@synthesize backButton = _backButton;
@synthesize forwardButton = _forwardButton;
@synthesize backButtonMode = _backButtonMode;
@synthesize forwardButtonMode = _forwardButtonMode;
@synthesize factory = _factory;
@synthesize TTSPlaying = _TTSPlaying;
@synthesize voiceSearchButton = _voiceSearchButton;

#pragma mark - Public

- (instancetype)init {
  self = [super init];
  if (self) {
    _backButtonMode = ToolbarButtonModeNormal;
    _forwardButtonMode = ToolbarButtonModeNormal;
  }
  return self;
}

- (BOOL)canStartPlayingTTS {
  return !self.voiceSearchButton.hidden;
}

- (void)updateIsTTSPlaying:(NSNotification*)notify {
  BOOL wasTTSPlaying = self.TTSPlaying;
  self.TTSPlaying =
      [notify.name isEqualToString:kTTSWillStartPlayingNotification];
  if (wasTTSPlaying != self.TTSPlaying && IsIPadIdiom()) {
    NSArray<UIImage*>* buttonImages;
    if (self.TTSPlaying) {
      buttonImages = [self.factory TTSImages];
    } else {
      buttonImages = [self.factory voiceSearchImages];
    }
    [self.voiceSearchButton setImage:buttonImages[0]
                            forState:UIControlStateNormal];
    [self.voiceSearchButton setImage:buttonImages[1]
                            forState:UIControlStateHighlighted];
  }
  if (self.TTSPlaying && UIAccessibilityIsVoiceOverRunning()) {
    // Moving VoiceOver without delay results in VoiceOver not staying on
    // |voiceSearchButton| and instead moving to views inside the WebView. Use
    // |voiceSearchButton| in the block to prevent |self| from being retained.
    UIButton* voiceSearchButton = self.voiceSearchButton;
    dispatch_async(dispatch_get_main_queue(), ^{
      UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification,
                                      voiceSearchButton);
    });
  }
}

- (void)moveVoiceOverToVoiceSearchButton {
  UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification,
                                  self.voiceSearchButton);
}

#pragma mark - TabHistoryUIUpdater

- (void)updateUIForTabHistoryPresentationFrom:(ToolbarButtonType)button {
  UIButton* historyButton = button ? self.backButton : self.forwardButton;
  // Keep the button pressed by swapping the normal and highlighted images.
  [self setImagesForNavButton:historyButton withTabHistoryVisible:YES];
}

- (void)updateUIForTabHistoryWasDismissed {
  [self setImagesForNavButton:self.backButton withTabHistoryVisible:NO];
  [self setImagesForNavButton:self.forwardButton withTabHistoryVisible:NO];
}

#pragma mark - Private

- (void)setImagesForNavButton:(UIButton*)button
        withTabHistoryVisible:(BOOL)tabHistoryVisible {
  BOOL isBackButton = button == self.backButton;
  ToolbarButtonMode newMode =
      tabHistoryVisible ? ToolbarButtonModeReversed : ToolbarButtonModeNormal;
  if (isBackButton && newMode == self.backButtonMode)
    return;
  if (!isBackButton && newMode == self.forwardButtonMode)
    return;

  UIImage* normalImage = [button imageForState:UIControlStateNormal];
  UIImage* highlightedImage = [button imageForState:UIControlStateHighlighted];
  [button setImage:highlightedImage forState:UIControlStateNormal];
  [button setImage:normalImage forState:UIControlStateHighlighted];
  if (isBackButton)
    self.backButtonMode = newMode;
  else
    self.forwardButtonMode = newMode;
}

@end
