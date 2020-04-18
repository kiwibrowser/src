// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_IOS_BROWSER_JS_SUGGESTION_MANAGER_H_
#define COMPONENTS_AUTOFILL_IOS_BROWSER_JS_SUGGESTION_MANAGER_H_

#import "ios/web/public/web_state/js/crw_js_injection_manager.h"

@class CRWJSInjectionReceiver;

// Loads the JavaScript file, suggestion_manager.js, which contains form parsing
// and autofill functions.
@interface JsSuggestionManager : CRWJSInjectionManager

// Focuses the next focusable element in tab order. No action if there is no
// such element.
- (void)selectNextElement;

// Focuses the next focusable element in tab order after the element specified
// by |formName| and |fieldName|. No action if there is no such element.
- (void)selectElementAfterForm:(NSString*)formName field:(NSString*)fieldName;

// Focuses the previous focusable element in tab order. No action if there is
// no such element.
- (void)selectPreviousElement;

// Focuses the previous focusable element in tab order from the element
// specified by |formName| and |fieldName|. No action if there is no such
// element.
- (void)selectElementBeforeForm:(NSString*)formName field:(NSString*)fieldName;

// Injects JS to check if the page contains a next and previous element.
// |completionHandler| is called with 2 BOOLs, the first indicating if a
// previous element was found, and the second indicating if a next element was
// found. |completionHandler| cannot be nil.
- (void)fetchPreviousAndNextElementsPresenceWithCompletionHandler:
        (void (^)(BOOL, BOOL))completionHandler;

// Injects JS to check if the page contains a next and previous element
// starting from the field specified by |formName| and |fieldName|.
// |completionHandler| is called with 2 BOOLs, the first indicating if a
// previous element was found, and the second indicating if a next element was
// found. |completionHandler| cannot be nil.
- (void)fetchPreviousAndNextElementsPresenceForForm:(NSString*)formName
                                              field:(NSString*)fieldName
                                  completionHandler:
                                      (void (^)(BOOL, BOOL))completionHandler;

// Closes the keyboard and defocuses the active input element.
- (void)closeKeyboard;

@end

#endif  // COMPONENTS_AUTOFILL_IOS_BROWSER_JS_SUGGESTION_MANAGER_H_
