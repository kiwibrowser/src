// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/crash_report/crash_report_multi_parameter.h"

#include <memory>

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#import "ios/chrome/browser/crash_report/breakpad_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Maximum size of a breakpad parameter. The length of the dictionary serialized
// into JSON cannot exceed this length. See declaration in (BreakPad.h) for
// details.
const int kMaximumBreakpadValueSize = 255;
}

@implementation CrashReportMultiParameter {
  NSString* crashReportKey_;
  std::unique_ptr<base::DictionaryValue> dictionary_;
}

- (instancetype)initWithKey:(NSString*)key {
  if ((self = [super init])) {
    DCHECK([key length] && ([key length] <= kMaximumBreakpadValueSize));
    dictionary_.reset(new base::DictionaryValue());
    crashReportKey_ = [key copy];
  }
  return self;
}

- (void)removeValue:(NSString*)key {
  dictionary_->Remove(base::SysNSStringToUTF8(key).c_str(), nullptr);
  [self updateCrashReport];
}

- (void)setValue:(NSString*)key withValue:(int)value {
  dictionary_->SetInteger(base::SysNSStringToUTF8(key).c_str(), value);
  [self updateCrashReport];
}

- (void)incrementValue:(NSString*)key {
  int value;
  std::string utf8_string = base::SysNSStringToUTF8(key);
  if (dictionary_->GetInteger(utf8_string.c_str(), &value)) {
    dictionary_->SetInteger(utf8_string.c_str(), value + 1);
  } else {
    dictionary_->SetInteger(utf8_string.c_str(), 1);
  }
  [self updateCrashReport];
}

- (void)decrementValue:(NSString*)key {
  int value;
  std::string utf8_string = base::SysNSStringToUTF8(key);
  if (dictionary_->GetInteger(utf8_string.c_str(), &value)) {
    if (value <= 1) {
      dictionary_->Remove(utf8_string.c_str(), nullptr);
    } else {
      dictionary_->SetInteger(utf8_string.c_str(), value - 1);
    }
    [self updateCrashReport];
  }
}

- (void)updateCrashReport {
  std::string stateAsJson;
  base::JSONWriter::Write(*dictionary_.get(), &stateAsJson);
  if (stateAsJson.length() > kMaximumBreakpadValueSize) {
    NOTREACHED();
    return;
  }
  breakpad_helper::AddReportParameter(
      crashReportKey_,
      [NSString stringWithCString:stateAsJson.c_str()
                         encoding:[NSString defaultCStringEncoding]],
      true);
}

@end
