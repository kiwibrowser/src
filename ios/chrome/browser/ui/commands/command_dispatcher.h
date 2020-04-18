// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_COMMAND_DISPATCHER_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_COMMAND_DISPATCHER_H_

#import <Foundation/Foundation.h>

@protocol MetricsRecorder;

// CommandDispatcher allows coordinators to register as command handlers for
// specific selectors.  Other objects can call these methods on the dispatcher,
// which in turn will forward the call to the registered handler. In addition,
// coordinators can register MetricsRecorders with selectors so that when a
// a selector is invoked on the dispatcher, the MetricsRecorder is also
// notified.
@interface CommandDispatcher : NSObject

// Registers the given |target| to receive forwarded messages for the given
// |selector|.
- (void)startDispatchingToTarget:(id)target forSelector:(SEL)selector;

// Removes forwarding registration for the given |selector|.
- (void)stopDispatchingForSelector:(SEL)selector;

// Registers the given |target| to receive forwarded messages for the methods of
// the given |protocol|. Only required instance methods are registered. The
// other definitions in the protocol are ignored.
- (void)startDispatchingToTarget:(id)target forProtocol:(Protocol*)protocol;

// Removes forwarding registration for the given |selector|. Only dispatching to
// required instance methods is removed. The other definitions in the protocol
// are ignored.
- (void)stopDispatchingForProtocol:(Protocol*)protocol;

// Removes all forwarding registrations for the given |target|.
- (void)stopDispatchingToTarget:(id)target;

// Registers the given |recorder| to be notified when |selector| is invoked
// on the dispatcher.
- (void)registerMetricsRecorder:(id<MetricsRecorder>)recorder
                    forSelector:(SEL)selector;

// Deregisters |selector| from notifying its associated MetricsRecorder when
// |selector| is invoked on the dispatcher.
- (void)deregisterMetricsRecordingForSelector:(SEL)selector;

@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_COMMAND_DISPATCHER_H_
