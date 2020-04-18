// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_VIEW_CONTROLLER_AUDIENCE_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_VIEW_CONTROLLER_AUDIENCE_H_

// Audience for the ContentSuggestions, getting informations from it.
@protocol ContentSuggestionsViewControllerAudience

// Notifies the audience that the content suggestions collection's content
// offset has changed, for example after scrolling or adding/removing an item.
- (void)contentOffsetDidChange;
// Notifies the audience that the promo has been shown.
- (void)promoShown;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_VIEW_CONTROLLER_AUDIENCE_H_
