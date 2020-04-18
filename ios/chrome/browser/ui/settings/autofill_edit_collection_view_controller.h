// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_AUTOFILL_EDIT_COLLECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_AUTOFILL_EDIT_COLLECTION_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/settings/settings_root_collection_view_controller.h"

// The collection view for an Autofill edit entry menu.
@interface AutofillEditCollectionViewController
    : SettingsRootCollectionViewController<UITextFieldDelegate>
@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_AUTOFILL_EDIT_COLLECTION_VIEW_CONTROLLER_H_
