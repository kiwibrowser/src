// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/translate/language_selection_consumer.h"

// The accessibility identifier of the cancel button on language picker view.
// NOTE: this should not be used on iOS 9 for testing.
extern NSString* const kLanguagePickerCancelButtonId;

// The accessibility identifier of the done button on language picker view.
// NOTE: this should not be used on iOS 9 for testing.
extern NSString* const kLanguagePickerDoneButtonId;

// A delegate for a LanguageSelectionViewController instance, which the view
// controller tells about selection events.
@protocol LanguageSelectionViewControllerDelegate

// Tells the delegate that a language was selected. |index| will be an index
// in the range provided to the view controller over via the consumer protocol.
- (void)languageSelectedAtIndex:(int)index;

// Tells the delegate that language selection was cancelled.
- (void)languageSelectionCanceled;

@end

// A view controller that displays a picker view for selecting a language from
// a list of provided languages.
@interface LanguageSelectionViewController
    : UIViewController<LanguageSelectionConsumer>

// The delegate for this view controller.
@property(nonatomic, weak) id<LanguageSelectionViewControllerDelegate> delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_VIEW_CONTROLLER_H_
