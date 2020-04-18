// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/autofill/ios/browser/js_autofill_manager.h"

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/format_macros.h"
#include "base/json/string_escape.h"
#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/ios/browser/autofill_switches.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation JsAutofillManager {
  // The injection receiver used to evaluate JavaScript.
  CRWJSInjectionReceiver* _receiver;
}

- (instancetype)initWithReceiver:(CRWJSInjectionReceiver*)receiver {
  DCHECK(receiver);
  self = [super init];
  if (self) {
    _receiver = receiver;
  }
  return self;
}

- (void)addJSDelay {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(
          autofill::switches::kAutofillIOSDelayBetweenFields)) {
    std::string delayString = command_line->GetSwitchValueASCII(
        autofill::switches::kAutofillIOSDelayBetweenFields);
    int commandLineDelay = 0;
    if (base::StringToInt(delayString, &commandLineDelay)) {
      NSString* setDelayJS =
          [NSString stringWithFormat:@"__gCrWeb.autofill.setDelay(%d);",
                                     commandLineDelay];
      [_receiver executeJavaScript:setDelayJS completionHandler:nil];
    }
  }
}

- (void)fetchFormsWithMinimumRequiredFieldsCount:(NSUInteger)requiredFieldsCount
                               completionHandler:
                                   (void (^)(NSString*))completionHandler {
  DCHECK(completionHandler);

  NSString* restrictUnownedFieldsToFormlessCheckout =
      base::FeatureList::IsEnabled(
          autofill::features::kAutofillRestrictUnownedFieldsToFormlessCheckout)
          ? @"true"
          : @"false";
  NSString* extractFormsJS = [NSString
      stringWithFormat:@"__gCrWeb.autofill.extractForms(%" PRIuNS ", %@);",
                       requiredFieldsCount,
                       restrictUnownedFieldsToFormlessCheckout];
  [_receiver executeJavaScript:extractFormsJS
             completionHandler:^(id result, NSError*) {
               completionHandler(base::mac::ObjCCastStrict<NSString>(result));
             }];
}

#pragma mark -
#pragma mark ProtectedMethods

- (void)fillActiveFormField:(NSString*)dataString
          completionHandler:(ProceduralBlock)completionHandler {
  NSString* script =
      [NSString stringWithFormat:@"__gCrWeb.autofill.fillActiveFormField(%@);",
                                 dataString];
  [_receiver executeJavaScript:script
             completionHandler:^(id, NSError*) {
               completionHandler();
             }];
}

- (void)toggleTrackingFormMutations:(BOOL)state {
  NSString* script =
      [NSString stringWithFormat:@"__gCrWeb.form.trackFormMutations(%d);",
                                 state ? 200 : 0];
  [_receiver executeJavaScript:script completionHandler:nil];
}

- (void)fillForm:(NSString*)dataString
    forceFillFieldIdentifier:(NSString*)forceFillFieldIdentifier
           completionHandler:(ProceduralBlock)completionHandler {
  DCHECK(completionHandler);
  std::string fieldIdentifier =
      forceFillFieldIdentifier
          ? base::GetQuotedJSONString(
                base::SysNSStringToUTF8(forceFillFieldIdentifier))
          : "null";
  NSString* fillFormJS =
      [NSString stringWithFormat:@"__gCrWeb.autofill.fillForm(%@, %s);",
                                 dataString, fieldIdentifier.c_str()];
  [_receiver executeJavaScript:fillFormJS
             completionHandler:^(id, NSError*) {
               completionHandler();
             }];
}

- (void)clearAutofilledFieldsForFormNamed:(NSString*)formName
                        completionHandler:(ProceduralBlock)completionHandler {
  DCHECK(completionHandler);
  NSString* script =
      [NSString stringWithFormat:
                    @"__gCrWeb.autofill.clearAutofilledFields(%s);",
                    base::GetQuotedJSONString([formName UTF8String]).c_str()];
  [_receiver executeJavaScript:script
             completionHandler:^(id, NSError*) {
               completionHandler();
             }];
}

- (void)fillPredictionData:(NSString*)dataString {
  NSString* script =
      [NSString stringWithFormat:@"__gCrWeb.autofill.fillPredictionData(%@);",
                                 dataString];
  [_receiver executeJavaScript:script completionHandler:nil];
}

@end
