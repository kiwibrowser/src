// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_BAR_OWNER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_BAR_OWNER_H_

#import <Foundation/Foundation.h>

@protocol VoiceSearchBar;

// Protocol implemented by UIViewControllers that own a VoiceSearchBar.
@protocol VoiceSearchBarOwner<NSObject>

// The VoiceSearchBar.
@property(nonatomic, readonly) id<VoiceSearchBar> voiceSearchBar;

@end

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_BAR_OWNER_H_
