// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_VIEW_H_
#define IOS_CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_VIEW_H_

#import <UIKit/UIKit.h>

@protocol BrowserCommands;

// The a11y ID of the text input field in the find-in-page bar.
extern NSString* const kFindInPageInputFieldId;

// The a11y ID of the "next" button in the find-in-page bar.
extern NSString* const kFindInPageNextButtonId;

// The a11y ID of the "previous" button in the find-in-page bar.
extern NSString* const kFindInPagePreviousButtonId;

// The a11y ID of the "close" button in the find-in-page bar.
extern NSString* const kFindInPageCloseButtonId;

// Find bar view.
// It shows a textfield that hosts the search term, a label with results count
// in format of "1 of 13", and next/previous/close buttons.
@interface FindBarView : UIView

// Designated initializer.
// |darkAppearance| makes the background to dark color and changes font colors
// to lighter colors.
- (instancetype)initWithDarkAppearance:(BOOL)darkAppearance
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Updates |resultsLabel| with |text|. Updates |inputField| layout so that input
// text does not overlap with results count. |text| can be nil.
- (void)updateResultsLabelWithText:(NSString*)text;

// The textfield with search term.
@property(nonatomic, weak) UITextField* inputField;
// Button to go to previous search result.
@property(nonatomic, weak) UIButton* previousButton;
// Button to go to next search result.
@property(nonatomic, weak) UIButton* nextButton;
// Button to dismiss Find in Page.
@property(nonatomic, weak) UIButton* closeButton;

@end

#endif  // IOS_CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_VIEW_H_
