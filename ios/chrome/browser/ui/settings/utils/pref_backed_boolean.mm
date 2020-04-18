// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/utils/pref_backed_boolean.h"

#include "base/bind.h"
#include "components/prefs/pref_member.h"
#include "components/prefs/pref_service.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Adaptor function to allow invoking onChange as a base::Closure.
void OnChange(id<ObservableBoolean> object) {
  [object.observer booleanDidChange:object];
}

}  // namespace

@implementation PrefBackedBoolean {
  BooleanPrefMember _pref;
}

@synthesize observer = _observer;

- (id)initWithPrefService:(PrefService*)prefs prefName:(const char*)prefName {
  self = [super init];
  if (self) {
    // Create a base::Closure that calls onChange.
    // Bind expects a refcounted object but allows us to pass a raw pointer
    // with Unretained.  Since the closure will be deleted when |_pref| is
    // deleted, which happens when |self| is deleted, we know |self| will still
    // be valid when the closure is invoked.
    base::Closure onChangeClosure =
        base::Bind(&OnChange, base::Unretained(self));
    _pref.Init(prefName, prefs, onChangeClosure);
  }
  return self;
}

- (BOOL)value {
  return _pref.GetValue();
}

- (void)setValue:(BOOL)value {
  _pref.SetValue(value == YES);
}

@end
