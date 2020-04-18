// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/metrics/metrics_test_util.h"

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSInvocation* GetInvocationForProtocolInstanceMethod(Protocol* protocol,
                                                     SEL selector,
                                                     BOOL isRequiredMethod) {
  // Get the NSMethodSignature required to create NSInvocation.
  struct objc_method_description methodDesc = protocol_getMethodDescription(
      protocol, selector, isRequiredMethod, YES /* an instance method */);
  DCHECK(methodDesc.types);
  NSMethodSignature* method =
      [NSMethodSignature signatureWithObjCTypes:methodDesc.types];
  DCHECK(method);

  NSInvocation* invocation =
      [NSInvocation invocationWithMethodSignature:method];
  invocation.selector = selector;
  return invocation;
}
