// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTOFILL_FORM_SUGGESTION_VIEW_H_
#define IOS_CHROME_BROWSER_AUTOFILL_FORM_SUGGESTION_VIEW_H_

#import <UIKit/UIKit.h>

@protocol FormSuggestionViewClient;

// A scrollable view for displaying user-selectable autofill form suggestions.
@interface FormSuggestionView : UIScrollView<UIInputViewAudioFeedback>

// Initializes with |frame| and |client| to show |suggestions|.
- (instancetype)initWithFrame:(CGRect)frame
                       client:(id<FormSuggestionViewClient>)client
                  suggestions:(NSArray*)suggestions;

@end

@interface FormSuggestionView (ForTesting)
@property(nonatomic, readonly) NSArray* suggestions;
@end

#endif  // IOS_CHROME_BROWSER_AUTOFILL_FORM_SUGGESTION_VIEW_H_
