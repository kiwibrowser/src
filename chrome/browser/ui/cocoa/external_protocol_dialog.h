// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTERNAL_PROTOCOL_DIALOG_H_
#define CHROME_BROWSER_UI_COCOA_EXTERNAL_PROTOCOL_DIALOG_H_

#import <Cocoa/Cocoa.h>

#include "base/time/time.h"
#include "url/gurl.h"

@interface ExternalProtocolDialogController : NSObject {
 @private
  NSAlert* alert_;
  GURL url_;
  int render_process_host_id_;
  int routing_id_;
  base::Time creation_time_;
};

- (id)initWithGURL:(const GURL*)url
    renderProcessHostId:(int)renderProcessHostId
    routingId:(int)routingId;

@end

#endif  // CHROME_BROWSER_UI_COCOA_EXTERNAL_PROTOCOL_DIALOG_H_
