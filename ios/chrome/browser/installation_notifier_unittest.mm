// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/installation_notifier.h"

#include <stdint.h>
#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"
#include "base/message_loop/message_loop.h"
#include "base/test/histogram_tester.h"
#include "ios/web/public/test/test_web_thread.h"
#include "net/base/backoff_entry.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FakeDispatcher : NSObject<DispatcherProtocol>
- (int64_t)lastDelayInNSec;
@end

@implementation FakeDispatcher {
  int _dispatchCount;
  int64_t _lastDelayInNSec;
  NSMutableDictionary* _blocks;
}

- (instancetype)init {
  if ((self = [super init]))
    _blocks = [[NSMutableDictionary alloc] init];
  return self;
}

#pragma mark -
#pragma mark Testing methods

- (void)executeAfter:(int)dispatchCount block:(ProceduralBlock)block {
  [_blocks setObject:[block copy]
              forKey:[NSNumber numberWithInt:dispatchCount]];
}

- (int64_t)lastDelayInNSec {
  return _lastDelayInNSec;
}

#pragma mark -
#pragma mark DispatcherProtocol

- (void)dispatchAfter:(int64_t)delayInNSec withBlock:(dispatch_block_t)block {
  _lastDelayInNSec = delayInNSec;
  void (^blockToCallForThisIteration)(void) =
      [_blocks objectForKey:[NSNumber numberWithInt:_dispatchCount]];
  if (blockToCallForThisIteration)
    blockToCallForThisIteration();
  _dispatchCount++;
  block();
}

@end

@interface MockNotificationReceiver : NSObject
@end

@implementation MockNotificationReceiver {
  int notificationCount_;
}

- (int)notificationCount {
  return notificationCount_;
}

- (void)receivedNotification {
  notificationCount_++;
}

@end

@interface InstallationNotifier (Testing)
- (void)setDispatcher:(id<DispatcherProtocol>)dispatcher;
- (void)resetDispatcher;
- (void)dispatchInstallationNotifierBlock;
- (void)registerForInstallationNotifications:(id)observer
                                withSelector:(SEL)notificationSelector
                                   forScheme:(NSString*)scheme
                                startPolling:(BOOL)poll;
- (net::BackoffEntry::Policy const*)backOffPolicy;
@end

namespace {

class InstallationNotifierTest : public PlatformTest {
 public:
  InstallationNotifierTest() : ui_thread_(web::WebThread::UI, &message_loop_) {}

 protected:
  void SetUp() override {
    installationNotifier_ = [InstallationNotifier sharedInstance];
    FakeDispatcher* dispatcher = [[FakeDispatcher alloc] init];
    dispatcher_ = dispatcher;
    notificationReceiver1_ = ([[MockNotificationReceiver alloc] init]);
    notificationReceiver2_ = ([[MockNotificationReceiver alloc] init]);
    application_ = OCMClassMock([UIApplication class]);
    OCMStub([application_ sharedApplication]).andReturn(application_);
    [installationNotifier_ setDispatcher:dispatcher_];
    histogramTester_.reset(new base::HistogramTester());
  }

  ~InstallationNotifierTest() override {
    [installationNotifier_ resetDispatcher];
  }

  void VerifyHistogramValidity(int expectedYes, int expectedNo) {
    histogramTester_->ExpectTotalCount("NativeAppLauncher.InstallationDetected",
                                       expectedYes + expectedNo);
    histogramTester_->ExpectBucketCount(
        "NativeAppLauncher.InstallationDetected", YES, expectedYes);
    histogramTester_->ExpectBucketCount(
        "NativeAppLauncher.InstallationDetected", NO, expectedNo);
  }

  void VerifyDelay(int pollingIteration) {
    double delayInMSec = [dispatcher_ lastDelayInNSec] / NSEC_PER_MSEC;
    double initialDelayInMSec =
        [installationNotifier_ backOffPolicy]->initial_delay_ms;
    double multiplyFactor =
        [installationNotifier_ backOffPolicy]->multiply_factor;
    double expectedDelayInMSec =
        initialDelayInMSec * pow(multiplyFactor, pollingIteration);
    double jitter = [installationNotifier_ backOffPolicy]->jitter_factor;
    EXPECT_NEAR(delayInMSec, expectedDelayInMSec,
                50 + jitter * expectedDelayInMSec);
  }

  base::MessageLoopForUI message_loop_;
  web::TestWebThread ui_thread_;
  __weak InstallationNotifier* installationNotifier_;
  __weak FakeDispatcher* dispatcher_;
  MockNotificationReceiver* notificationReceiver1_;
  MockNotificationReceiver* notificationReceiver2_;
  id application_;
  std::unique_ptr<base::HistogramTester> histogramTester_;
};

TEST_F(InstallationNotifierTest, RegisterWithAppAlreadyInstalled) {
  OCMStub([application_ canOpenURL:[OCMArg any]]).andReturn(YES);
  [installationNotifier_
      registerForInstallationNotifications:notificationReceiver1_
                              withSelector:@selector(receivedNotification)
                                 forScheme:@"foo-scheme"];
  EXPECT_EQ(1, [notificationReceiver1_ notificationCount]);
  [installationNotifier_
      registerForInstallationNotifications:notificationReceiver1_
                              withSelector:@selector(receivedNotification)
                                 forScheme:@"foo-scheme"];
  EXPECT_EQ(2, [notificationReceiver1_ notificationCount]);
  VerifyHistogramValidity(2, 0);
}

TEST_F(InstallationNotifierTest, RegisterWithAppInstalledAfterSomeTime) {
  [dispatcher_
      executeAfter:10
             block:^{
               OCMStub([application_ canOpenURL:[OCMArg any]]).andReturn(YES);
             }];
  [installationNotifier_
      registerForInstallationNotifications:notificationReceiver1_
                              withSelector:@selector(receivedNotification)
                                 forScheme:@"foo-scheme"];
  EXPECT_EQ(1, [notificationReceiver1_ notificationCount]);
  VerifyHistogramValidity(1, 0);
}

TEST_F(InstallationNotifierTest, RegisterForTwoInstallations) {
  [dispatcher_
      executeAfter:10
             block:^{
               OCMStub([application_ canOpenURL:[OCMArg any]]).andReturn(YES);
             }];
  [installationNotifier_
      registerForInstallationNotifications:notificationReceiver1_
                              withSelector:@selector(receivedNotification)
                                 forScheme:@"foo-scheme"
                              startPolling:NO];
  [installationNotifier_
      registerForInstallationNotifications:notificationReceiver2_
                              withSelector:@selector(receivedNotification)
                                 forScheme:@"foo-scheme"
                              startPolling:NO];
  [installationNotifier_
      registerForInstallationNotifications:notificationReceiver2_
                              withSelector:@selector(receivedNotification)
                                 forScheme:@"bar-scheme"
                              startPolling:NO];
  [installationNotifier_ dispatchInstallationNotifierBlock];
  EXPECT_EQ(1, [notificationReceiver1_ notificationCount]);
  EXPECT_EQ(2, [notificationReceiver2_ notificationCount]);
  VerifyHistogramValidity(2, 0);
}

TEST_F(InstallationNotifierTest, RegisterAndThenUnregister) {
  OCMStub([application_ canOpenURL:[OCMArg any]]).andReturn(NO);
  [dispatcher_ executeAfter:10
                      block:^{
                        [installationNotifier_
                            unregisterForNotifications:notificationReceiver1_];
                      }];
  [installationNotifier_
      registerForInstallationNotifications:notificationReceiver1_
                              withSelector:@selector(receivedNotification)
                                 forScheme:@"foo-scheme"];
  EXPECT_EQ(0, [notificationReceiver1_ notificationCount]);
  VerifyHistogramValidity(0, 1);
}

TEST_F(InstallationNotifierTest, TestExponentialBackoff) {
  OCMStub([application_ canOpenURL:[OCMArg any]]).andReturn(NO);
  // Making sure that delay is multiplied by |multiplyFactor| every time.
  [dispatcher_ executeAfter:0
                      block:^{
                        VerifyDelay(0);
                      }];
  [dispatcher_ executeAfter:1
                      block:^{
                        VerifyDelay(1);
                      }];
  [dispatcher_ executeAfter:2
                      block:^{
                        VerifyDelay(2);
                      }];
  // Registering for the installation of another application and making sure
  // that the delay is reset to the initial delay.
  [dispatcher_ executeAfter:
                   3 block:^{
    VerifyDelay(3);
    [installationNotifier_
        registerForInstallationNotifications:notificationReceiver1_
                                withSelector:@selector(receivedNotification)
                                   forScheme:@"bar-scheme"
                                startPolling:NO];
  }];
  [dispatcher_ executeAfter:4
                      block:^{
                        VerifyDelay(0);
                      }];
  [dispatcher_ executeAfter:5
                      block:^{
                        VerifyDelay(1);
                        [installationNotifier_
                            unregisterForNotifications:notificationReceiver1_];
                      }];

  [installationNotifier_
      registerForInstallationNotifications:notificationReceiver1_
                              withSelector:@selector(receivedNotification)
                                 forScheme:@"foo-scheme"];
  VerifyHistogramValidity(0, 2);
}

TEST_F(InstallationNotifierTest, TestThatEmptySchemeDoesntCrashChrome) {
  [installationNotifier_
      registerForInstallationNotifications:notificationReceiver1_
                              withSelector:@selector(receivedNotification)
                                 forScheme:nil];
  [installationNotifier_ unregisterForNotifications:notificationReceiver1_];
}

}  // namespace
