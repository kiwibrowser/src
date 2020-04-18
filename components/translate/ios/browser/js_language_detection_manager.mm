// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/translate/ios/browser/js_language_detection_manager.h"

#include "base/callback.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace language_detection {
// Note: This should stay in sync with the constant in language_detection.js.
const size_t kMaxIndexChars = 65535;
}  // namespace language_detection

@implementation JsLanguageDetectionManager

#pragma mark - Protected methods

- (NSString*)scriptPath {
  return @"language_detection";
}

#pragma mark - Public methods

- (void)startLanguageDetection {
  [self executeJavaScript:@"__gCrWeb.languageDetection.detectLanguage()"
        completionHandler:nil];
}

- (void)retrieveBufferedTextContent:
        (const language_detection::BufferedTextCallback&)callback {
  DCHECK(!callback.is_null());
  // Copy the completion handler so that the block does not capture a reference.
  __block language_detection::BufferedTextCallback blockCallback = callback;
  NSString* JS = @"__gCrWeb.languageDetection.retrieveBufferedTextContent()";
  [self executeJavaScript:JS completionHandler:^(id result, NSError*) {
    blockCallback.Run(base::SysNSStringToUTF16(result));
  }];
}

@end
