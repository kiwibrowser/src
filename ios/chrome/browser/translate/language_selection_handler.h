// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TRANSLATE_LANGUAGE_SELECTION_HANDLER_H_
#define IOS_CHROME_BROWSER_TRANSLATE_LANGUAGE_SELECTION_HANDLER_H_

#import <Foundation/Foundation.h>

@class LanguageSelectionContext;
@protocol LanguageSelectionDelegate;

// Protocol adopted by an object that can provide an interface for a user to
// select a language from the languages exposed by a translate_infobar_delegate.
@protocol LanguageSelectionHandler

// Tells the handler to display a language selector using the language
// information in |languages| and telling |delegate| the results of the
// selection.
- (void)showLanguageSelectorWithContext:(LanguageSelectionContext*)context
                               delegate:(id<LanguageSelectionDelegate>)delegate;

// Tells the handler to stop displaying the language selector, telling the
// delegate no selection was made.
- (void)dismissLanguageSelector;

@end

#endif  // IOS_CHROME_BROWSER_TRANSLATE_LANGUAGE_SELECTION_HANDLER_H_
