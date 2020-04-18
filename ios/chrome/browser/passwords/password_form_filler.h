// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_PASSWORD_FORM_FILLER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_PASSWORD_FORM_FILLER_H_

#import <Foundation/Foundation.h>

@protocol PasswordFormFiller<NSObject>

// Finds password forms in the page and fills them with the |username| and
// |password|. If not nil, |completionHandler| is called once per form filled.
- (void)findAndFillPasswordForms:(NSString*)username
                        password:(NSString*)password
               completionHandler:(void (^)(BOOL))completionHandler;

@end

#endif  // IOS_CHROME_BROWSER_PASSWORDS_PASSWORD_FORM_FILLER_H_
