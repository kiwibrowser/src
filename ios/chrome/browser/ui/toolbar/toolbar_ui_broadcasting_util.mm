// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/toolbar_ui_broadcasting_util.h"

#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_ui.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void StartBroadcastingToolbarUI(id<ToolbarUI> toolbar,
                                ChromeBroadcaster* broadcaster) {
  [broadcaster broadcastValue:@"toolbarHeight"
                     ofObject:toolbar
                     selector:@selector(broadcastToolbarHeight:)];
}

void StopBroadcastingToolbarUI(ChromeBroadcaster* broadcaster) {
  [broadcaster stopBroadcastingForSelector:@selector(broadcastToolbarHeight:)];
}
