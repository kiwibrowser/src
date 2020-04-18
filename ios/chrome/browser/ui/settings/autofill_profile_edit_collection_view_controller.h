// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_AUTOFILL_PROFILE_EDIT_COLLECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_AUTOFILL_PROFILE_EDIT_COLLECTION_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/settings/autofill_edit_collection_view_controller.h"

namespace autofill {
class AutofillProfile;
class PersonalDataManager;
}  // namespace autofill

extern NSString* const kAutofillProfileEditCollectionViewId;

// The collection view for the Autofill settings.
@interface AutofillProfileEditCollectionViewController
    : AutofillEditCollectionViewController

// Creates a controller for |profile| and |dataManager| that cannot be null.
+ (instancetype)controllerWithProfile:(const autofill::AutofillProfile&)profile
                  personalDataManager:
                      (autofill::PersonalDataManager*)dataManager;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_AUTOFILL_PROFILE_EDIT_COLLECTION_VIEW_CONTROLLER_H_
