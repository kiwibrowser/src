// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/commands/command_dispatcher.h"

#import <Foundation/Foundation.h>

#include "base/macros.h"
#import "ios/chrome/browser/ui/metrics/metrics_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - Test handlers

@protocol ShowProtocol<NSObject>
- (void)show;
- (void)showMore;
@end

// A handler with methods that take no arguments.
@interface CommandDispatcherTestSimpleTarget : NSObject<ShowProtocol>

// Will be set to YES when the |-show| method is called.
@property(nonatomic, assign) BOOL showCalled;

// Will be set to YES when the |-showMore| method is called.
@property(nonatomic, assign) BOOL showMoreCalled;

// Will be set to YES when the |-hide| method is called.
@property(nonatomic, assign) BOOL hideCalled;

// Resets the above properties to NO.
- (void)resetProperties;

// Handler methods.
- (void)hide;

@end

@implementation CommandDispatcherTestSimpleTarget

@synthesize showCalled = _showCalled;
@synthesize showMoreCalled = _showMoreCalled;
@synthesize hideCalled = _hideCalled;

- (void)resetProperties {
  self.showCalled = NO;
  self.showMoreCalled = NO;
  self.hideCalled = NO;
}

- (void)show {
  self.showCalled = YES;
}

- (void)showMore {
  self.showMoreCalled = YES;
}

- (void)hide {
  self.hideCalled = YES;
}

@end

// A handler with methods that take various types of arguments.
@interface CommandDispatcherTestTargetWithArguments : NSObject

// Set to YES when |-methodWithInt:| is called.
@property(nonatomic, assign) BOOL intMethodCalled;

// The argument passed to the most recent call of |-methodWithInt:|.
@property(nonatomic, assign) int intArgument;

// Set to YES when |-methodWithObject:| is called.
@property(nonatomic, assign) BOOL objectMethodCalled;

// The argument passed to the most recent call of |-methodWithObject:|.
@property(nonatomic, strong) NSObject* objectArgument;

// Resets the above properties to NO or nil.
- (void)resetProperties;

// Handler methods.
- (void)methodWithInt:(int)arg;
- (void)methodWithObject:(NSObject*)arg;
- (int)methodToAddFirstArgument:(int)first toSecond:(int)second;

@end

@implementation CommandDispatcherTestTargetWithArguments

@synthesize intMethodCalled = _intMethodCalled;
@synthesize intArgument = _intArgument;
@synthesize objectMethodCalled = _objectMethodCalled;
@synthesize objectArgument = _objectArgument;

- (void)resetProperties {
  self.intMethodCalled = NO;
  self.intArgument = 0;
  self.objectMethodCalled = NO;
  self.objectArgument = nil;
}

- (void)methodWithInt:(int)arg {
  self.intMethodCalled = YES;
  self.intArgument = arg;
}

- (void)methodWithObject:(NSObject*)arg {
  self.objectMethodCalled = YES;
  self.objectArgument = arg;
}

- (int)methodToAddFirstArgument:(int)first toSecond:(int)second {
  return first + second;
}

@end

#pragma mark - TestMetricsRecorder

// A MetricsRecorder that provides information about calls to
// |recordMetricForInvocation:|.
@interface TestMetricsRecorder : NSObject<MetricsRecorder>

// Number of times |recordMetricForInvocation:| was called.
@property(nonatomic, assign) int callCount;

// The NSInvocation from the most recent call to |recordMetricForInvocation:|.
@property(nonatomic, strong) NSInvocation* mostRecentInvocation;

@end

@implementation TestMetricsRecorder

@synthesize callCount = _callCount;
@synthesize mostRecentInvocation = _mostRecentInvocation;

- (void)recordMetricForInvocation:(NSInvocation*)anInvocation {
  self.callCount += 1;

  [anInvocation retainArguments];
  self.mostRecentInvocation = anInvocation;
}

@end

#pragma mark - Tests

using CommandDispatcherTest = PlatformTest;

// Tests handler methods with no arguments.
TEST_F(CommandDispatcherTest, SimpleTarget) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestSimpleTarget* target =
      [[CommandDispatcherTestSimpleTarget alloc] init];

  [dispatcher startDispatchingToTarget:target forSelector:@selector(show)];
  [dispatcher startDispatchingToTarget:target forSelector:@selector(hide)];

  [dispatcher show];
  EXPECT_TRUE(target.showCalled);
  EXPECT_FALSE(target.hideCalled);

  [target resetProperties];
  [dispatcher hide];
  EXPECT_FALSE(target.showCalled);
  EXPECT_TRUE(target.hideCalled);
}

// Tests handler methods that take arguments.
TEST_F(CommandDispatcherTest, TargetWithArguments) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestTargetWithArguments* target =
      [[CommandDispatcherTestTargetWithArguments alloc] init];

  [dispatcher startDispatchingToTarget:target
                           forSelector:@selector(methodWithInt:)];
  [dispatcher startDispatchingToTarget:target
                           forSelector:@selector(methodWithObject:)];
  [dispatcher
      startDispatchingToTarget:target
                   forSelector:@selector(methodToAddFirstArgument:toSecond:)];

  const int int_argument = 4;
  [dispatcher methodWithInt:int_argument];
  EXPECT_TRUE(target.intMethodCalled);
  EXPECT_FALSE(target.objectMethodCalled);
  EXPECT_EQ(int_argument, target.intArgument);

  [target resetProperties];
  NSObject* object_argument = [[NSObject alloc] init];
  [dispatcher methodWithObject:object_argument];
  EXPECT_FALSE(target.intMethodCalled);
  EXPECT_TRUE(target.objectMethodCalled);
  EXPECT_EQ(object_argument, target.objectArgument);

  [target resetProperties];
  EXPECT_EQ(13, [dispatcher methodToAddFirstArgument:7 toSecond:6]);
  EXPECT_FALSE(target.intMethodCalled);
  EXPECT_FALSE(target.objectMethodCalled);
}

// Tests that messages are routed to the proper handler when multiple targets
// are registered.
TEST_F(CommandDispatcherTest, MultipleTargets) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestSimpleTarget* showTarget =
      [[CommandDispatcherTestSimpleTarget alloc] init];
  CommandDispatcherTestSimpleTarget* hideTarget =
      [[CommandDispatcherTestSimpleTarget alloc] init];

  [dispatcher startDispatchingToTarget:showTarget forSelector:@selector(show)];
  [dispatcher startDispatchingToTarget:hideTarget forSelector:@selector(hide)];

  [dispatcher show];
  EXPECT_TRUE(showTarget.showCalled);
  EXPECT_FALSE(hideTarget.showCalled);

  [showTarget resetProperties];
  [dispatcher hide];
  EXPECT_FALSE(showTarget.hideCalled);
  EXPECT_TRUE(hideTarget.hideCalled);
}

// Tests handlers registered via protocols.
TEST_F(CommandDispatcherTest, ProtocolRegistration) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestSimpleTarget* target =
      [[CommandDispatcherTestSimpleTarget alloc] init];

  [dispatcher startDispatchingToTarget:target
                           forProtocol:@protocol(ShowProtocol)];

  [dispatcher show];
  EXPECT_TRUE(target.showCalled);
  [dispatcher showMore];
  EXPECT_TRUE(target.showCalled);
}

// Tests that handlers are no longer forwarded messages after selector
// deregistration.
TEST_F(CommandDispatcherTest, SelectorDeregistration) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestSimpleTarget* target =
      [[CommandDispatcherTestSimpleTarget alloc] init];

  [dispatcher startDispatchingToTarget:target forSelector:@selector(show)];
  [dispatcher startDispatchingToTarget:target forSelector:@selector(hide)];

  [dispatcher show];
  EXPECT_TRUE(target.showCalled);
  EXPECT_FALSE(target.hideCalled);

  [target resetProperties];
  [dispatcher stopDispatchingForSelector:@selector(show)];
  bool exception_caught = false;
  @try {
    [dispatcher show];
  } @catch (NSException* exception) {
    EXPECT_EQ(NSInvalidArgumentException, [exception name]);
    exception_caught = true;
  }
  EXPECT_TRUE(exception_caught);

  [dispatcher hide];
  EXPECT_FALSE(target.showCalled);
  EXPECT_TRUE(target.hideCalled);
}

// Tests that handlers are no longer forwarded messages after protocol
// deregistration.
TEST_F(CommandDispatcherTest, ProtocolDeregistration) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestSimpleTarget* target =
      [[CommandDispatcherTestSimpleTarget alloc] init];

  [dispatcher startDispatchingToTarget:target
                           forProtocol:@protocol(ShowProtocol)];
  [dispatcher startDispatchingToTarget:target forSelector:@selector(hide)];

  [dispatcher show];
  EXPECT_TRUE(target.showCalled);
  EXPECT_FALSE(target.showMoreCalled);
  EXPECT_FALSE(target.hideCalled);
  [target resetProperties];
  [dispatcher showMore];
  EXPECT_FALSE(target.showCalled);
  EXPECT_TRUE(target.showMoreCalled);
  EXPECT_FALSE(target.hideCalled);

  [target resetProperties];
  [dispatcher stopDispatchingForProtocol:@protocol(ShowProtocol)];
  bool exception_caught = false;
  @try {
    [dispatcher show];
  } @catch (NSException* exception) {
    EXPECT_EQ(NSInvalidArgumentException, [exception name]);
    exception_caught = true;
  }
  EXPECT_TRUE(exception_caught);
  exception_caught = false;
  @try {
    [dispatcher showMore];
  } @catch (NSException* exception) {
    EXPECT_EQ(NSInvalidArgumentException, [exception name]);
    exception_caught = true;
  }
  EXPECT_TRUE(exception_caught);

  [dispatcher hide];
  EXPECT_FALSE(target.showCalled);
  EXPECT_FALSE(target.showMoreCalled);
  EXPECT_TRUE(target.hideCalled);
}

// Tests that handlers are no longer forwarded messages after target
// deregistration.
TEST_F(CommandDispatcherTest, TargetDeregistration) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestSimpleTarget* showTarget =
      [[CommandDispatcherTestSimpleTarget alloc] init];
  CommandDispatcherTestSimpleTarget* hideTarget =
      [[CommandDispatcherTestSimpleTarget alloc] init];

  [dispatcher startDispatchingToTarget:showTarget forSelector:@selector(show)];
  [dispatcher startDispatchingToTarget:hideTarget forSelector:@selector(hide)];

  [dispatcher show];
  EXPECT_TRUE(showTarget.showCalled);
  EXPECT_FALSE(hideTarget.showCalled);

  [dispatcher stopDispatchingToTarget:showTarget];
  bool exception_caught = false;
  @try {
    [dispatcher show];
  } @catch (NSException* exception) {
    EXPECT_EQ(NSInvalidArgumentException, [exception name]);
    exception_caught = true;
  }
  EXPECT_TRUE(exception_caught);

  [dispatcher hide];
  EXPECT_FALSE(showTarget.hideCalled);
  EXPECT_TRUE(hideTarget.hideCalled);
}

// Tests that an exception is thrown when there is no registered handler for a
// given selector.
TEST_F(CommandDispatcherTest, NoTargetRegisteredForSelector) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestSimpleTarget* target =
      [[CommandDispatcherTestSimpleTarget alloc] init];

  [dispatcher startDispatchingToTarget:target forSelector:@selector(show)];

  bool exception_caught = false;
  @try {
    [dispatcher hide];
  } @catch (NSException* exception) {
    EXPECT_EQ(NSInvalidArgumentException, [exception name]);
    exception_caught = true;
  }

  EXPECT_TRUE(exception_caught);
}

// Tests that -respondsToSelector returns YES for methods once they are
// dispatched for.
// Tests handler methods with no arguments.
TEST_F(CommandDispatcherTest, RespondsToSelector) {
  id dispatcher = [[CommandDispatcher alloc] init];

  EXPECT_FALSE([dispatcher respondsToSelector:@selector(show)]);
  CommandDispatcherTestSimpleTarget* target =
      [[CommandDispatcherTestSimpleTarget alloc] init];

  [dispatcher startDispatchingToTarget:target forSelector:@selector(show)];
  EXPECT_TRUE([dispatcher respondsToSelector:@selector(show)]);

  [dispatcher stopDispatchingForSelector:@selector(show)];
  EXPECT_FALSE([dispatcher respondsToSelector:@selector(show)]);

  // Actual dispatcher methods should still always advertise that they are
  // responded to.
  EXPECT_TRUE([dispatcher
      respondsToSelector:@selector(startDispatchingToTarget:forSelector:)]);
  EXPECT_TRUE(
      [dispatcher respondsToSelector:@selector(stopDispatchingForSelector:)]);
}

// Tests that a registered MetricsRecorder is successfully
// notified when commands with no arguments are invoked on the dispatcher.
TEST_F(CommandDispatcherTest, MetricsRecorderNoArguments) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestSimpleTarget* target =
      [[CommandDispatcherTestSimpleTarget alloc] init];
  TestMetricsRecorder* recorder = [[TestMetricsRecorder alloc] init];

  [dispatcher startDispatchingToTarget:target forSelector:@selector(show)];
  [dispatcher registerMetricsRecorder:recorder forSelector:@selector(show)];

  [dispatcher startDispatchingToTarget:target forSelector:@selector(hide)];
  [dispatcher registerMetricsRecorder:recorder forSelector:@selector(hide)];

  [dispatcher show];
  EXPECT_EQ(1, recorder.callCount);
  EXPECT_EQ(@selector(show), recorder.mostRecentInvocation.selector);

  [dispatcher hide];
  EXPECT_EQ(2, recorder.callCount);
  EXPECT_EQ(@selector(hide), recorder.mostRecentInvocation.selector);
}

// Tests that a registered MetricsRecorder is successfully
// notified when commands with arguments are invoked on the dispatcher.
TEST_F(CommandDispatcherTest, MetricsRecorderWithArguments) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestTargetWithArguments* target =
      [[CommandDispatcherTestTargetWithArguments alloc] init];
  TestMetricsRecorder* recorder = [[TestMetricsRecorder alloc] init];

  [dispatcher startDispatchingToTarget:target
                           forSelector:@selector(methodWithInt:)];
  [dispatcher registerMetricsRecorder:recorder
                          forSelector:@selector(methodWithInt:)];
  [dispatcher startDispatchingToTarget:target
                           forSelector:@selector(methodWithObject:)];
  [dispatcher registerMetricsRecorder:recorder
                          forSelector:@selector(methodWithObject:)];

  const int int_argument = 4;
  [dispatcher methodWithInt:int_argument];

  EXPECT_EQ(1, recorder.callCount);
  EXPECT_EQ(@selector(methodWithInt:), recorder.mostRecentInvocation.selector);
  int received_int_argument = 0;
  // The index of the int argument is 2, as indices 0 and 1 are reserved for
  // hidden arguments.
  [recorder.mostRecentInvocation getArgument:&received_int_argument atIndex:2];
  EXPECT_EQ(int_argument, received_int_argument);

  NSObject* object_argument = [[NSObject alloc] init];
  [dispatcher methodWithObject:object_argument];

  EXPECT_EQ(2, recorder.callCount);
  EXPECT_EQ(@selector(methodWithObject:),
            recorder.mostRecentInvocation.selector);
  __unsafe_unretained NSObject* received_object_argument = nil;
  // The index of the object argument is 2, as indices 0 and 1 are reserved for
  // hidden arguments.
  [recorder.mostRecentInvocation getArgument:&received_object_argument
                                     atIndex:2];
  EXPECT_NSEQ(object_argument, received_object_argument);
}

// Tests that the correct MetricsRecorders are notified for an invocation
// when multiple recorders are registered.
TEST_F(CommandDispatcherTest, MetricsRecorderMultipleRecorders) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestSimpleTarget* showTarget =
      [[CommandDispatcherTestSimpleTarget alloc] init];
  TestMetricsRecorder* showRecorder = [[TestMetricsRecorder alloc] init];
  CommandDispatcherTestSimpleTarget* hideTarget =
      [[CommandDispatcherTestSimpleTarget alloc] init];
  TestMetricsRecorder* hideRecorder = [[TestMetricsRecorder alloc] init];

  [dispatcher startDispatchingToTarget:showTarget forSelector:@selector(show)];
  [dispatcher registerMetricsRecorder:showRecorder forSelector:@selector(show)];
  [dispatcher startDispatchingToTarget:hideTarget forSelector:@selector(hide)];
  [dispatcher registerMetricsRecorder:hideRecorder forSelector:@selector(hide)];

  [dispatcher show];
  EXPECT_EQ(1, showRecorder.callCount);
  EXPECT_EQ(@selector(show), showRecorder.mostRecentInvocation.selector);
  EXPECT_EQ(0, hideRecorder.callCount);
  EXPECT_NSEQ(nil, hideRecorder.mostRecentInvocation);

  [dispatcher hide];
  EXPECT_EQ(1, hideRecorder.callCount);
  EXPECT_EQ(@selector(hide), hideRecorder.mostRecentInvocation.selector);
  EXPECT_EQ(1, showRecorder.callCount);
  EXPECT_EQ(@selector(show), showRecorder.mostRecentInvocation.selector);
}

// Tests that if a selector registered to a MetricsRecorder is deregistered,
// the MetricsRecorder is no longer notified when the selector is invoked on the
// dispatcher.
TEST_F(CommandDispatcherTest, DeregisterMetricsRecorder) {
  id dispatcher = [[CommandDispatcher alloc] init];
  CommandDispatcherTestSimpleTarget* target =
      [[CommandDispatcherTestSimpleTarget alloc] init];
  TestMetricsRecorder* recorder = [[TestMetricsRecorder alloc] init];

  [dispatcher startDispatchingToTarget:target forSelector:@selector(show)];
  [dispatcher registerMetricsRecorder:recorder forSelector:@selector(show)];

  [dispatcher show];
  EXPECT_EQ(1, recorder.callCount);
  EXPECT_EQ(@selector(show), recorder.mostRecentInvocation.selector);

  [dispatcher deregisterMetricsRecordingForSelector:@selector(show)];

  [dispatcher show];
  EXPECT_EQ(1, recorder.callCount);
}
