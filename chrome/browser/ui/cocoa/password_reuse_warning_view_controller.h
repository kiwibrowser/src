// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_VIEW_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_VIEW_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

class PasswordReuseWarningDialogCocoa;

// Controller for the password reuse dialog view.
@interface PasswordReuseWarningViewController : NSViewController

- (instancetype)initWithOwner:(PasswordReuseWarningDialogCocoa*)owner;

@end

#endif  // CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_DIALOG_CONTROLLER_H_
