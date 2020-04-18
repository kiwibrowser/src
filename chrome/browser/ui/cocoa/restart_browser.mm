// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/restart_browser.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

// Helper to clean up after the notification that the alert was dismissed.
@interface RestartHelper : NSObject {
 @private
  NSAlert* alert_;
}
- (NSAlert*)alert;
- (void)alertDidEnd:(NSAlert*)alert
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo;
@end

@implementation RestartHelper

- (NSAlert*)alert {
  alert_ = [[NSAlert alloc] init];
  return alert_;
}

- (void)dealloc {
  [alert_ release];
  [super dealloc];
}

- (void)alertDidEnd:(NSAlert*)alert
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo {
  if (returnCode == NSAlertFirstButtonReturn) {
    chrome::AttemptRestart();
  } else if (returnCode == NSAlertSecondButtonReturn) {
    // Nothing to do. User will restart later.
  } else {
    NOTREACHED();
  }
  [self autorelease];
}

@end

namespace restart_browser {

void RequestRestart(NSWindow* parent) {
  NSString* title =
      l10n_util::GetNSStringFWithFixup(IDS_PLEASE_RELAUNCH_BROWSER,
          l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));
  NSString* text =
      l10n_util::GetNSStringFWithFixup(IDS_UPDATE_RECOMMENDED,
          l10n_util::GetStringUTF16(IDS_PRODUCT_NAME));
  NSString* notNowButton = l10n_util::GetNSStringWithFixup(IDS_NOT_NOW);
  NSString* restartButton =
      l10n_util::GetNSStringWithFixup(IDS_RELAUNCH_AND_UPDATE);

  RestartHelper* helper = [[RestartHelper alloc] init];

  NSAlert* alert = [helper alert];
  [alert setAlertStyle:NSInformationalAlertStyle];
  [alert setMessageText:title];
  [alert setInformativeText:text];
  [alert addButtonWithTitle:restartButton];
  [alert addButtonWithTitle:notNowButton];

  if (parent) {
    [alert beginSheetModalForWindow:parent
                      modalDelegate:helper
                     didEndSelector:@selector(alertDidEnd:
                                               returnCode:
                                              contextInfo:)
                        contextInfo:nil];
  } else {
    NSInteger returnCode = [alert runModal];
    [helper alertDidEnd:alert
             returnCode:returnCode
            contextInfo:NULL];
  }
}

}  // namespace restart_browser
