// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/commands/command_dispatcher.h"

#include <objc/runtime.h>
#include <unordered_map>
#include <vector>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/metrics/metrics_recorder.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation CommandDispatcher {
  // Stores which target to forward to for a given selector.
  std::unordered_map<SEL, __weak id> _forwardingTargets;

  // Stores which MetricsRecorder to notify for a given selector.
  std::unordered_map<SEL, __weak id<MetricsRecorder>> _metricsRecorders;
}

- (void)startDispatchingToTarget:(id)target forSelector:(SEL)selector {
  DCHECK(![self targetForSelector:selector]);

  _forwardingTargets[selector] = target;
}

- (void)startDispatchingToTarget:(id)target forProtocol:(Protocol*)protocol {
  unsigned int methodCount;
  objc_method_description* requiredInstanceMethods =
      protocol_copyMethodDescriptionList(protocol, YES /* isRequiredMethod */,
                                         YES /* isInstanceMethod */,
                                         &methodCount);
  for (unsigned int i = 0; i < methodCount; i++) {
    [self startDispatchingToTarget:target
                       forSelector:requiredInstanceMethods[i].name];
  }
  free(requiredInstanceMethods);
}

- (void)stopDispatchingForSelector:(SEL)selector {
  _forwardingTargets.erase(selector);
}

- (void)stopDispatchingForProtocol:(Protocol*)protocol {
  unsigned int methodCount;
  objc_method_description* requiredInstanceMethods =
      protocol_copyMethodDescriptionList(protocol, YES /* isRequiredMethod */,
                                         YES /* isInstanceMethod */,
                                         &methodCount);
  for (unsigned int i = 0; i < methodCount; i++) {
    [self stopDispatchingForSelector:requiredInstanceMethods[i].name];
  }
  free(requiredInstanceMethods);
}

// |-stopDispatchingToTarget| should be called much less often than
// |-forwardingTargetForSelector|, so removal is intentionally O(n) in order
// to prioritize the speed of lookups.
- (void)stopDispatchingToTarget:(id)target {
  std::vector<SEL> selectorsToErase;
  for (auto& kv : _forwardingTargets) {
    if (kv.second == target) {
      selectorsToErase.push_back(kv.first);
    }
  }

  for (auto* selector : selectorsToErase) {
    [self stopDispatchingForSelector:selector];
  }
}

- (void)registerMetricsRecorder:(id<MetricsRecorder>)recorder
                    forSelector:(SEL)selector {
  DCHECK(![self metricsRecorderForSelector:selector]);

  _metricsRecorders[selector] = recorder;
}

- (void)deregisterMetricsRecordingForSelector:(SEL)selector {
  _metricsRecorders.erase(selector);
}

#pragma mark - NSObject

// Overridden to forward messages to registered handlers.
- (id)forwardingTargetForSelector:(SEL)selector {
  // If the selector is registered with a MetricsRecorder, return nil to force
  // |forwardInvocation| to handle message forwarding. |forwardInvocation|
  // provides an NSInvocation that is required by the MetricsRecorders.
  if ([self metricsRecorderForSelector:selector])
    return nil;

  id target = [self targetForSelector:selector];
  if (target)
    return target;

  return [super forwardingTargetForSelector:selector];
}

// Overriden to return YES for any registered method.
- (BOOL)respondsToSelector:(SEL)selector {
  if ([self targetForSelector:selector])
    return YES;
  return [super respondsToSelector:selector];
}

// Overriden to forward messages to registered handlers when an NSInvocation is
// required.
- (void)forwardInvocation:(NSInvocation*)anInvocation {
  SEL selector = anInvocation.selector;

  id<MetricsRecorder> recorder = [self metricsRecorderForSelector:selector];
  if (recorder) {
    [recorder recordMetricForInvocation:anInvocation];
  }

  id target = [self targetForSelector:selector];
  if ([target respondsToSelector:selector]) {
    [anInvocation invokeWithTarget:target];
    return;
  }

  [super forwardInvocation:anInvocation];
}

// Overriden because overrides of |forwardInvocation| also require an override
// of |methodSignatureForSelector|, as the method signature is needed to
// construct NSInvocations.
- (NSMethodSignature*)methodSignatureForSelector:(SEL)aSelector {
  NSMethodSignature* signature = [super methodSignatureForSelector:aSelector];
  if (signature)
    return signature;

  id target = [self targetForSelector:aSelector];
  return [target methodSignatureForSelector:aSelector];
}

#pragma mark - Private

// Returns the target registered to receive messeages for |selector|.
- (id)targetForSelector:(SEL)selector {
  auto target = _forwardingTargets.find(selector);
  if (target == _forwardingTargets.end())
    return nil;
  return target->second;
}

// Returns the MetricsRecorder registered to be notified when |selector| is
// invoked on the dispatcher.
- (id<MetricsRecorder>)metricsRecorderForSelector:(SEL)selector {
  auto recorder = _metricsRecorders.find(selector);
  if (recorder == _metricsRecorders.end())
    return nil;
  return recorder->second;
}

@end
