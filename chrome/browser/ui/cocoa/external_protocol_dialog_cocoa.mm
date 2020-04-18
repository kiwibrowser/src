// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/external_protocol_dialog.h"

#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/tab_contents/tab_util.h"
#import "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/gfx/text_elider.h"

///////////////////////////////////////////////////////////////////////////////
// ExternalProtocolDialogController

@interface ExternalProtocolDialogController(Private)
- (void)alertEnded:(NSAlert *)alert
        returnCode:(int)returnCode
       contextInfo:(void*)contextInfo;
@end

@implementation ExternalProtocolDialogController
- (id)initWithGURL:(const GURL*)url
    renderProcessHostId:(int)renderProcessHostId
    routingId:(int)routingId {
  DCHECK(base::MessageLoopForUI::IsCurrent());

  if (!(self = [super init]))
    return nil;

  url_ = *url;
  render_process_host_id_ = renderProcessHostId;
  routing_id_ = routingId;
  creation_time_ = base::Time::Now();

  base::string16 appName =
      shell_integration::GetApplicationNameForProtocol(url_);
  if (appName.length() == 0) {
    // No registered apps for this protocol; give up and go home.
    [self autorelease];
    return nil;
  }

  alert_ = [[NSAlert alloc] init];

  [alert_ setMessageText:
      l10n_util::GetNSStringFWithFixup(IDS_EXTERNAL_PROTOCOL_TITLE, appName)];

  NSButton* allowButton = [alert_
      addButtonWithTitle:l10n_util::GetNSStringFWithFixup(
                   IDS_EXTERNAL_PROTOCOL_OK_BUTTON_TEXT, appName)];
  [allowButton setKeyEquivalent:kKeyEquivalentReturn];  // set as default
  [alert_ addButtonWithTitle:
      l10n_util::GetNSStringWithFixup(
                   IDS_EXTERNAL_PROTOCOL_CANCEL_BUTTON_TEXT)];

  [alert_ setShowsSuppressionButton:YES];
  [[alert_ suppressionButton]
      setTitle:l10n_util::GetNSStringWithFixup(
                   IDS_EXTERNAL_PROTOCOL_CHECKBOX_TEXT)];

  [[alert_ window] setInitialFirstResponder:allowButton];
  [alert_ beginSheetModalForWindow:nil  // nil here makes it app-modal
                     modalDelegate:self
                    didEndSelector:@selector(alertEnded:returnCode:contextInfo:)
                       contextInfo:nil];

  return self;
}

- (void)dealloc {
  [alert_ release];

  [super dealloc];
}

- (void)alertEnded:(NSAlert *)alert
        returnCode:(int)returnCode
       contextInfo:(void*)contextInfo {
  ExternalProtocolHandler::BlockState blockState =
      ExternalProtocolHandler::UNKNOWN;
  switch (returnCode) {
    case NSAlertFirstButtonReturn:
      blockState = ExternalProtocolHandler::DONT_BLOCK;
      break;
    case NSAlertSecondButtonReturn:
      blockState = ExternalProtocolHandler::BLOCK;
      break;
    default:
      NOTREACHED();
  }

  content::WebContents* web_contents =
      tab_util::GetWebContentsByID(render_process_host_id_, routing_id_);

  if (web_contents) {
    Profile* profile =
        Profile::FromBrowserContext(web_contents->GetBrowserContext());
    bool isChecked = [[alert_ suppressionButton] state] == NSOnState &&
                     blockState == ExternalProtocolHandler::DONT_BLOCK;
    // Set the "don't warn me again" info.
    if (isChecked) {
      ExternalProtocolHandler::SetBlockState(url_.scheme(), blockState,
                                             profile);
    }

    ExternalProtocolHandler::RecordHandleStateMetrics(isChecked, blockState);

    if (blockState == ExternalProtocolHandler::DONT_BLOCK) {
      UMA_HISTOGRAM_LONG_TIMES("clickjacking.launch_url",
                               base::Time::Now() - creation_time_);

      ExternalProtocolHandler::LaunchUrlWithoutSecurityCheck(url_,
                                                             web_contents);
    }
  }

  [self autorelease];
}

@end
