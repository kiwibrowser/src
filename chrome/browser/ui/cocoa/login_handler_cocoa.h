// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOGIN_HANDLER_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_LOGIN_HANDLER_COCOA_H_

#import <Cocoa/Cocoa.h>

class LoginHandlerMac;

// Controller of the sheet used by LoginHandlerMac. Interface Builder wants
// this to be in a .h file.
@interface LoginHandlerSheet : NSWindowController {
 @private
  IBOutlet NSTextField* nameField_;
  IBOutlet NSSecureTextField* passwordField_;
  IBOutlet NSTextField* authorityField_;
  IBOutlet NSTextField* explanationField_;
  IBOutlet NSButton* loginButton_;
  IBOutlet NSButton* cancelButton_;
  LoginHandlerMac* handler_;  // weak, owns us
}
- (id)initWithLoginHandler:(LoginHandlerMac*)handler;
- (IBAction)loginPressed:(id)sender;
- (IBAction)cancelPressed:(id)sender;
- (void)autofillLogin:(NSString*)login password:(NSString*)password;
- (void)setAuthority:(NSString*)authority;
- (void)setExplanation:(NSString*)explanation;
@end

#endif  // CHROME_BROWSER_UI_COCOA_LOGIN_HANDLER_COCOA_H_
