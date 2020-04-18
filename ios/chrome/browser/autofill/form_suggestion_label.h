// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTOFILL_FORM_SUGGESTION_LABEL_H_
#define IOS_CHROME_BROWSER_AUTOFILL_FORM_SUGGESTION_LABEL_H_

#import <UIKit/UIKit.h>


@class FormSuggestion;
@protocol FormSuggestionViewClient;

// Class for Autofill suggestion in the customized keyboard.
@interface FormSuggestionLabel : UIView

// Designated initializer. Initializes with |proposedFrame| and |client| for
// |suggestion|. Its width will be adjusted according to the length of
// |suggestion| and width in |proposedFrame| is ignored.
- (id)initWithSuggestion:(FormSuggestion*)suggestion
           proposedFrame:(CGRect)proposedFrame
                   index:(NSUInteger)index
          numSuggestions:(NSUInteger)numSuggestions
                  client:(id<FormSuggestionViewClient>)client;

@end

#endif  // IOS_CHROME_BROWSER_AUTOFILL_FORM_SUGGESTION_LABEL_H_
