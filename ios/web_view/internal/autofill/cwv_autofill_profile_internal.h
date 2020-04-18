// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_PROFILE_INTERNAL_H_
#define IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_PROFILE_INTERNAL_H_

#import "ios/web_view/public/cwv_autofill_profile.h"

namespace autofill {
class AutofillProfile;
}  // namespace autofill

@interface CWVAutofillProfile ()

// The internal autofill profile that is wrapped by this object.
@property(nonatomic, readonly) autofill::AutofillProfile* internalProfile;

- (instancetype)initWithProfile:(const autofill::AutofillProfile&)profile
    NS_DESIGNATED_INITIALIZER;

@end

#endif  // IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_PROFILE_INTERNAL_H_
