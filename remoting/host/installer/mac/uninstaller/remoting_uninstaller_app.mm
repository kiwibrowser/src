// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/installer/mac/uninstaller/remoting_uninstaller_app.h"

#import <Cocoa/Cocoa.h>

#include "remoting/host/installer/mac/uninstaller/remoting_uninstaller.h"

@implementation RemotingUninstallerAppDelegate

- (void)dealloc {
  [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
}

- (void)showSuccess:(bool)success withMessage:(NSString*) message {
  NSString* summary = success ? @"Uninstall succeeded" : @"Uninstall failed";
  NSAlert* alert = [NSAlert alertWithMessageText:summary
                                   defaultButton:@"OK"
                                 alternateButton:nil
                                     otherButton:nil
                       informativeTextWithFormat:@"%@", message];
  [alert setAlertStyle:
       (success ? NSInformationalAlertStyle : NSCriticalAlertStyle)];
  [alert runModal];
}

- (IBAction)uninstall:(NSButton*)sender {
  @try {
    NSLog(@"Chrome Remote Desktop uninstall starting.");

    RemotingUninstaller* uninstaller =
        [[[RemotingUninstaller alloc] init] autorelease];
    OSStatus status = [uninstaller remotingUninstall];

    NSLog(@"Chrome Remote Desktop Host uninstall complete.");

    bool success = false;
    NSString* message = nullptr;
    if (status == errAuthorizationSuccess) {
      success = true;
      message = @"Chrome Remote Desktop Host successfully uninstalled.";
    } else if (status == errAuthorizationCanceled) {
      message = @"Chrome Remote Desktop Host uninstall canceled.";
    } else if (status == errAuthorizationDenied) {
      message = @"Chrome Remote Desktop Host uninstall authorization denied.";
    } else {
      [NSException raise:@"AuthorizationCopyRights Failure"
                  format:@"Error during AuthorizationCopyRights status=%d",
                             static_cast<int>(status)];
    }
    if (message != nullptr) {
      NSLog(@"Uninstall %s: %@", success ? "succeeded" : "failed", message);
      [self showSuccess:success withMessage:message];
    }
  }
  @catch (NSException* exception) {
    NSLog(@"Exception %@ %@", [exception name], [exception reason]);
    NSString* message =
        @"Error! Unable to uninstall Chrome Remote Desktop Host.";
    [self showSuccess:false withMessage:message];
  }

  [NSApp terminate:self];
}

- (IBAction)cancel:(id)sender {
  [NSApp terminate:self];
}

- (IBAction)handleMenuClose:(NSMenuItem*)sender {
  [NSApp terminate:self];
}

@end

int main(int argc, char* argv[])
{
  // The no-ui option skips the UI confirmation dialogs. This is provided as
  // a convenience for our automated testing.
  // There will still be an elevation prompt unless the command is run as root.
  if (argc == 2 && !strcmp(argv[1], "--no-ui")) {
    @autoreleasepool {
      NSLog(@"Chrome Remote Desktop uninstall starting.");
      NSLog(@"--no-ui : Suppressing UI");

      RemotingUninstaller* uninstaller =
          [[[RemotingUninstaller alloc] init] autorelease];
      OSStatus status = [uninstaller remotingUninstall];

      NSLog(@"Chrome Remote Desktop Host uninstall complete.");
      NSLog(@"Status = %d", static_cast<int>(status));
      return status != errAuthorizationSuccess;
    }
  } else {
    return NSApplicationMain(argc, (const char**)argv);
  }
}

