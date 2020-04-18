// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_MEDIATOR_H_

#import <Foundation/Foundation.h>
#include <string>

@protocol LanguageSelectionConsumer;
@class LanguageSelectionContext;

// Mediator object to configure and provide data for language selection.
@interface LanguageSelectionMediator : NSObject

// Designated initializer. |context| is the context object provided for language
// selection.
- (instancetype)initWithContext:(LanguageSelectionContext*)context
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

// Consumer for this mediator.
@property(nonatomic, weak) id<LanguageSelectionConsumer> consumer;

// Utility method for the coordinator to map a selected language index to a
// language name.
- (std::string)languageCodeForLanguageAtIndex:(int)index;

@end

#endif  // IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_MEDIATOR_H_
