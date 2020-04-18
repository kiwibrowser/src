// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/settings_utils.h"

#import "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/commands/open_url_command.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

ProceduralBlockWithURL BlockToOpenURL(UIResponder* responder,
                                      id<ApplicationCommands> dispatcher) {
  __weak UIResponder* weakResponder = responder;
  __weak id<ApplicationCommands> weakDispatcher = dispatcher;
  ProceduralBlockWithURL blockToOpenURL = ^(const GURL& url) {
    UIResponder* strongResponder = weakResponder;
    if (!strongResponder)
      return;
    OpenUrlCommand* command =
        [[OpenUrlCommand alloc] initWithURLFromChrome:url];
    [weakDispatcher closeSettingsUIAndOpenURL:command];
  };
  return [blockToOpenURL copy];
}
