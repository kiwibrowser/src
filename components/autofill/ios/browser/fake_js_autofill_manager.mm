// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/autofill/ios/browser/fake_js_autofill_manager.h"

#import "base/mac/bind_objc_block.h"
#include "ios/web/public/web_thread.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation FakeJSAutofillManager

@synthesize lastClearedFormName = _lastClearedFormName;

- (void)clearAutofilledFieldsForFormNamed:(NSString*)formName
                        completionHandler:(ProceduralBlock)completionHandler {
  web::WebThread::PostTask(web::WebThread::UI, FROM_HERE, base::BindBlockArc(^{
                             _lastClearedFormName = [formName copy];
                             completionHandler();
                           }));
}

@end
