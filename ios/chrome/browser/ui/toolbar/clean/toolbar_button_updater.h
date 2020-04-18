// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_BUTTON_UPDATER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_BUTTON_UPDATER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/history_popup/requirements/tab_history_ui_updater.h"

@class ToolbarButtonFactory;

// Updater for the toolbar buttons.
@interface ToolbarButtonUpdater : NSObject<TabHistoryUIUpdater>

// Back button of the toolbar.
@property(nonatomic, strong) UIButton* backButton;
// Forward button of the toolbar.
@property(nonatomic, strong) UIButton* forwardButton;
// Voice search button of the toolbar.
@property(nonatomic, strong) UIButton* voiceSearchButton;
// Whether Text-To-Speech is playing and the voice search icon should indicate
// so.
@property(nonatomic, assign, getter=isTTSPlaying) BOOL TTSPlaying;
// Button factory to get images for TTS and VoiceSearch.
@property(nonatomic, strong) ToolbarButtonFactory* factory;

// Whether the Text-To-Speech playback can start.
- (BOOL)canStartPlayingTTS;
// Updates the TTS button depending on whether or not TTS is currently playing.
- (void)updateIsTTSPlaying:(NSNotification*)notify;
// Moves VoiceOver to the button used to perform a voice search.
- (void)moveVoiceOverToVoiceSearchButton;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_BUTTON_UPDATER_H_
