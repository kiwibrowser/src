// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/translate/ios/browser/js_translate_manager.h"

#import <Foundation/Foundation.h>

#include <memory>

#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation JsTranslateManager {
  NSString* _translationScript;
}

- (NSString*)script {
  return _translationScript;
}

- (void)setScript:(NSString*)script {
  // The translation script uses performance.now() for metrics, which is not
  // supported except on iOS 8.0. To make the translation script work on these
  // iOS versions, add some JavaScript to |script| that defines an
  // implementation of performance.now().
  NSString* const kPerformancePlaceholder =
      @"var performance = window['performance'] || {};"
      @"performance.now = performance['now'] ||"
      @"(function () { return Date.now(); });\n";
  script = [kPerformancePlaceholder stringByAppendingString:script];
  NSString* path =
      [base::mac::FrameworkBundle() pathForResource:@"translate_ios"
                                             ofType:@"js"];
  DCHECK(path);
  NSError* error = nil;
  NSString* content = [NSString stringWithContentsOfFile:path
                                                encoding:NSUTF8StringEncoding
                                                   error:&error];
  DCHECK(!error && [content length]);
  script = [script stringByAppendingString:content];
  _translationScript = [script copy];
}

- (void)injectWaitUntilTranslateReadyScript {
  [self.receiver executeJavaScript:@"__gCrWeb.translate.checkTranslateReady()"
                 completionHandler:nil];
}

- (void)injectTranslateStatusScript {
  [self.receiver executeJavaScript:@"__gCrWeb.translate.checkTranslateStatus()"
                 completionHandler:nil];
}

- (void)startTranslationFrom:(const std::string&)source
                          to:(const std::string&)target {
  NSString* script =
      [NSString stringWithFormat:@"cr.googleTranslate.translate('%s','%s')",
                                 source.c_str(), target.c_str()];
  [self.receiver executeJavaScript:script completionHandler:nil];
}

- (void)revertTranslation {
  DCHECK([self hasBeenInjected]);
  [self.receiver executeJavaScript:@"cr.googleTranslate.revert()"
                 completionHandler:nil];
}

#pragma mark -
#pragma mark CRWJSInjectionManager methods

- (NSString*)injectionContent {
  DCHECK(_translationScript);
  NSString* translationScript = _translationScript;
  _translationScript = nil;
  return translationScript;
}

@end
