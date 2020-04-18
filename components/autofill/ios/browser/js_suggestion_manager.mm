// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/autofill/ios/browser/js_suggestion_manager.h"

#include "base/format_macros.h"
#include "base/json/string_escape.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Santizies |str| and wraps it in quotes so it can be injected safely in
// JavaScript.
NSString* JSONEscape(NSString* str) {
  return base::SysUTF8ToNSString(
      base::GetQuotedJSONString(base::SysNSStringToUTF8(str)));
}
}  // namespace

@implementation JsSuggestionManager

#pragma mark -
#pragma mark ProtectedMethods

- (NSString*)scriptPath {
  return @"suggestion_controller";
}

- (void)selectNextElement {
  [self selectElementAfterForm:@"" field:@""];
}

- (void)selectElementAfterForm:(NSString*)formName field:(NSString*)fieldName {
  NSString* selectNextElementJS = [NSString
      stringWithFormat:@"__gCrWeb.suggestion.selectNextElement(%@, %@)",
                       JSONEscape(formName), JSONEscape(fieldName)];
  [self executeJavaScript:selectNextElementJS completionHandler:nil];
}

- (void)selectPreviousElement {
  [self selectElementBeforeForm:@"" field:@""];
}

- (void)selectElementBeforeForm:(NSString*)formName field:(NSString*)fieldName {
  NSString* selectPreviousElementJS = [NSString
      stringWithFormat:@"__gCrWeb.suggestion.selectPreviousElement(%@, %@)",
                       JSONEscape(formName), JSONEscape(fieldName)];
  [self executeJavaScript:selectPreviousElementJS completionHandler:nil];
}

- (void)fetchPreviousAndNextElementsPresenceWithCompletionHandler:
        (void (^)(BOOL, BOOL))completionHandler {
  [self fetchPreviousAndNextElementsPresenceForForm:@""
                                              field:@""
                                  completionHandler:completionHandler];
}

- (void)fetchPreviousAndNextElementsPresenceForForm:(NSString*)formName
                                              field:(NSString*)fieldName
                                  completionHandler:
                                      (void (^)(BOOL, BOOL))completionHandler {
  DCHECK(completionHandler);
  DCHECK([self hasBeenInjected]);
  NSString* escapedFormName = JSONEscape(formName);
  NSString* escapedFieldName = JSONEscape(fieldName);
  NSString* JS = [NSString
      stringWithFormat:@"[__gCrWeb.suggestion.hasPreviousElement(%@, %@),"
                       @"__gCrWeb.suggestion.hasNextElement(%@, %@)]"
                       @".toString()",
                       escapedFormName, escapedFieldName, escapedFormName,
                       escapedFieldName];
  [self executeJavaScript:JS completionHandler:^(id result, NSError* error) {
    // The result maybe an empty string here due to 2 reasons:
    // 1) When there is an exception running the JS
    // 2) There is a race when the page is changing due to which
    // JSSuggestionManager has not yet injected __gCrWeb.suggestion object
    // Handle this case gracefully.
    // If a page has overridden Array.toString, the string returned may not
    // contain a ",", hence this is a defensive measure to early return.
    NSArray* components = [result componentsSeparatedByString:@","];
    if (components.count != 2) {
      completionHandler(NO, NO);
      return;
    }

    DCHECK([components[0] isEqualToString:@"true"] ||
           [components[0] isEqualToString:@"false"]);
    BOOL hasPreviousElement = [components[0] isEqualToString:@"true"];
    DCHECK([components[1] isEqualToString:@"true"] ||
           [components[1] isEqualToString:@"false"]);
    BOOL hasNextElement = [components[1] isEqualToString:@"true"];
    completionHandler(hasPreviousElement, hasNextElement);
  }];
}

- (void)closeKeyboard {
  [self executeJavaScript:@"document.activeElement.blur()"
        completionHandler:nil];
}

@end
