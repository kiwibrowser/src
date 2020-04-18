// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_CONTROLLER_DELEGATE_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_CONTROLLER_DELEGATE_H_

// A component that receives voice search results.
@protocol VoiceSearchControllerDelegate
// Receives the best voice search result.
- (void)receiveVoiceSearchResult:(NSString*)voiceResult;
@end

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_CONTROLLER_DELEGATE_H_
