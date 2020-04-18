// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_BAR_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_BAR_H_

#import <Foundation/Foundation.h>

@protocol VoiceSearchBarDelegate;

// TODO(crbug.com/800266): Check if those protocols are still relevant with the
// adaptive toolbar.

// Protocol used by bottom toolbars containing a button to launch VoiceSearch.
@protocol VoiceSearchBar<NSObject>

// The VoiceSearchBar's delegate.
@property(nonatomic, assign) id<VoiceSearchBarDelegate> voiceSearchBarDelegate;

// Animates the view up from the bottom of the its superview so that it's
// visible or down so that it's no longer visible.
- (void)animateToBecomeVisible:(BOOL)visible;

// Sets necessary state to prepare for presenting voice search.
- (void)prepareToPresentVoiceSearch;

@end

// The delegate protocol for VoiceSearchBar.
@protocol VoiceSearchBarDelegate

// Returns whether Text To Speech is enabled for |voiceSearchBar|.
- (BOOL)isTTSEnabledForVoiceSearchBar:(id<VoiceSearchBar>)voiceSearchBar;

// Called to notify the delegate that the state of |voiceSearchBar|'s
// VoiceSearch button appearance has been updated.
- (void)voiceSearchBarDidUpdateButtonState:(id<VoiceSearchBar>)voiceSearchBar;

@end

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_BAR_H_
