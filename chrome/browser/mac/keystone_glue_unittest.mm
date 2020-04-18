// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>
#import <objc/objc-class.h>

#include "base/test/scoped_task_environment.h"
#import "chrome/browser/mac/keystone_glue.h"
#import "chrome/browser/mac/keystone_registration.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace ksr = keystone_registration;


@interface FakeKeystoneRegistration : KSRegistration
@end


// This unit test implements FakeKeystoneRegistration as a KSRegistration
// subclass. It won't be linked against KSRegistration, so provide a stub
// KSRegistration class on which to base FakeKeystoneRegistration.
@implementation KSRegistration

+ (id)registrationWithProductID:(NSString*)productID {
  return nil;
}

- (BOOL)registerWithParameters:(NSDictionary*)args {
  return NO;
}

- (BOOL)promoteWithParameters:(NSDictionary*)args
                authorization:(AuthorizationRef)authorization {
  return NO;
}

- (BOOL)setActive {
  return NO;
}

- (BOOL)setActiveWithReportingAttributes:(NSArray*)reportingAttributes
                                   error:(NSError**)error {
  return NO;
}

- (void)checkForUpdateWasUserInitiated:(BOOL)userInitiated {
}

- (void)startUpdate {
}

- (ksr::KSRegistrationTicketType)ticketType {
  return ksr::kKSRegistrationDontKnowWhatKindOfTicket;
}

@end


@implementation FakeKeystoneRegistration

// Send the notifications that a real KeystoneGlue object would send.

- (void)checkForUpdateWasUserInitiated:(BOOL)userInitiated {
  NSNumber* yesNumber = [NSNumber numberWithBool:YES];
  NSString* statusKey = @"Status";
  NSDictionary* dictionary = [NSDictionary dictionaryWithObject:yesNumber
                                                         forKey:statusKey];
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center postNotificationName:ksr::KSRegistrationCheckForUpdateNotification
                        object:nil
                      userInfo:dictionary];
}

- (void)startUpdate {
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center postNotificationName:ksr::KSRegistrationStartUpdateNotification
                        object:nil];
}

@end


@interface FakeKeystoneGlue : KeystoneGlue {
 @public
  BOOL upToDate_;
  NSString *latestVersion_;
  BOOL successful_;
  int installs_;
}

- (void)fakeAboutWindowCallback:(NSNotification*)notification;
@end


@implementation FakeKeystoneGlue

- (id)init {
  if ((self = [super init])) {
    // some lies
    upToDate_ = YES;
    latestVersion_ = @"foo bar";
    successful_ = YES;
    installs_ = 1010101010;

    // Set up an observer that takes the notification that the About window
    // listens for.
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(fakeAboutWindowCallback:)
                   name:kAutoupdateStatusNotification
                 object:nil];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

// For mocking
- (NSDictionary*)infoDictionary {
  NSDictionary* dict = [NSDictionary dictionaryWithObjectsAndKeys:
                                     @"http://foo.bar", @"KSUpdateURL",
                                     @"com.google.whatever", @"KSProductID",
                                     @"0.0.0.1", @"KSVersion",
                                     nil];
  return dict;
}

// For mocking
- (BOOL)loadKeystoneRegistration {
  return YES;
}

// Confirms certain things are happy
- (BOOL)dictReadCorrectly {
  return ([url_ isEqual:@"http://foo.bar"] &&
          [productID_ isEqual:@"com.google.whatever"] &&
          [version_ isEqual:@"0.0.0.1"]);
}

// Confirms certain things are happy
- (BOOL)hasATimer {
  return timer_ ? YES : NO;
}

- (void)addFakeRegistration {
  registration_ = [[FakeKeystoneRegistration alloc] init];
}

- (void)fakeAboutWindowCallback:(NSNotification*)notification {
  NSDictionary* dictionary = [notification userInfo];
  AutoupdateStatus status = static_cast<AutoupdateStatus>(
      [[dictionary objectForKey:kAutoupdateStatusStatus] intValue]);

  if (status == kAutoupdateAvailable) {
    upToDate_ = NO;
    latestVersion_ = [dictionary objectForKey:kAutoupdateStatusVersion];
  } else if (status == kAutoupdateInstallFailed) {
    successful_ = NO;
    installs_ = 0;
  }
}

// Confirm we look like callbacks with nil NSNotifications
- (BOOL)confirmCallbacks {
  return (!upToDate_ &&
          (latestVersion_ == nil) &&
          !successful_ &&
          (installs_ == 0));
}

@end

@interface KeystoneGlue (PrivateMethods)

+ (BOOL)isValidSystemKeystone:(NSDictionary*)systemKeystonePlistContents
            comparedToBundled:(NSDictionary*)bundledKeystonePlistContents;

@end

namespace {

class KeystoneGlueTest : public PlatformTest {
 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

// DISABLED because the mocking isn't currently working.
TEST_F(KeystoneGlueTest, DISABLED_BasicGlobalCreate) {
  // Allow creation of a KeystoneGlue by mocking out a few calls
  SEL ids = @selector(infoDictionary);
  IMP oldInfoImp_ = [[KeystoneGlue class] instanceMethodForSelector:ids];
  IMP newInfoImp_ = [[FakeKeystoneGlue class] instanceMethodForSelector:ids];
  Method infoMethod_ = class_getInstanceMethod([KeystoneGlue class], ids);
  method_setImplementation(infoMethod_, newInfoImp_);

  SEL lks = @selector(loadKeystoneRegistration);
  IMP oldLoadImp_ = [[KeystoneGlue class] instanceMethodForSelector:lks];
  IMP newLoadImp_ = [[FakeKeystoneGlue class] instanceMethodForSelector:lks];
  Method loadMethod_ = class_getInstanceMethod([KeystoneGlue class], lks);
  method_setImplementation(loadMethod_, newLoadImp_);

  KeystoneGlue *glue = [KeystoneGlue defaultKeystoneGlue];
  ASSERT_TRUE(glue);

  // Fix back up the class to the way we found it.
  method_setImplementation(infoMethod_, oldInfoImp_);
  method_setImplementation(loadMethod_, oldLoadImp_);
}

// DISABLED because the mocking isn't currently working.
TEST_F(KeystoneGlueTest, DISABLED_BasicUse) {
  FakeKeystoneGlue* glue = [[[FakeKeystoneGlue alloc] init] autorelease];
  [glue loadParameters];
  ASSERT_TRUE([glue dictReadCorrectly]);

  // Likely returns NO in the unit test, but call it anyway to make
  // sure it doesn't crash.
  [glue loadKeystoneRegistration];

  // Confirm we start up an active timer
  [glue registerWithKeystone];
  ASSERT_TRUE([glue hasATimer]);
  [glue stopTimer];

  // Brief exercise of callbacks
  [glue addFakeRegistration];
  [glue checkForUpdate];
  [glue installUpdate];
  ASSERT_TRUE([glue confirmCallbacks]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Nils) {
  ASSERT_TRUE([KeystoneGlue isValidSystemKeystone:nil comparedToBundled:nil]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Nil_Bundled) {
  ASSERT_TRUE([KeystoneGlue isValidSystemKeystone:@{} comparedToBundled:nil]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Empty_Bundled) {
  ASSERT_TRUE([KeystoneGlue isValidSystemKeystone:@{} comparedToBundled:@{}]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Bad_Bundled) {
  ASSERT_TRUE(
      [KeystoneGlue isValidSystemKeystone:@{} comparedToBundled:@{
        @[] : @2
      }]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Bad_Bundled_Version) {
  ASSERT_TRUE([KeystoneGlue isValidSystemKeystone:@{}
      comparedToBundled:@{
        @"CFBundleVersion" : @1
      }]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Bad_Bundled_Version_String) {
  ASSERT_TRUE([KeystoneGlue
      isValidSystemKeystone:@{}
          comparedToBundled:@{@"CFBundleVersion" : @"Hi how are you?"}]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Nil_System_Keystone) {
  ASSERT_FALSE([KeystoneGlue
      isValidSystemKeystone:nil
          comparedToBundled:@{@"CFBundleVersion" : @"1.2.3"}]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Empty_System_Keystone) {
  ASSERT_FALSE([KeystoneGlue
      isValidSystemKeystone:@{}
          comparedToBundled:@{@"CFBundleVersion" : @"1.2.3"}]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Bad_System_Keystone) {
  ASSERT_FALSE([KeystoneGlue
      isValidSystemKeystone:@{@"foo" : @"bar"}
          comparedToBundled:@{@"CFBundleVersion" : @"1.2.3"}]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Bad_System_Keystone_Version) {
  ASSERT_FALSE([KeystoneGlue
      isValidSystemKeystone:@{
        @"CFBundleVersion" : @[]
      }
          comparedToBundled:@{@"CFBundleVersion" : @"1.2.3"}]);
}

TEST_F(KeystoneGlueTest,
       isValidSystemKeystone_Bad_System_Keystone_Version_String) {
  ASSERT_FALSE([KeystoneGlue
      isValidSystemKeystone:@{@"CFBundleVersion" : @"I am baddy."}
          comparedToBundled:@{@"CFBundleVersion" : @"1.2.3"}]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Bad_System_Keystone_Outdated) {
  ASSERT_FALSE([KeystoneGlue
      isValidSystemKeystone:@{@"CFBundleVersion" : @"1.2.2.15"}
          comparedToBundled:@{@"CFBundleVersion" : @"1.2.3"}]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Bad_System_Keystone_Same) {
  ASSERT_TRUE([KeystoneGlue
      isValidSystemKeystone:@{@"CFBundleVersion" : @"1.2.3.4"}
          comparedToBundled:@{@"CFBundleVersion" : @"1.2.3.4"}]);
}

TEST_F(KeystoneGlueTest, isValidSystemKeystone_Bad_System_Keystone_Newer) {
  ASSERT_TRUE([KeystoneGlue
      isValidSystemKeystone:@{@"CFBundleVersion" : @"1.2.4.1"}
          comparedToBundled:@{@"CFBundleVersion" : @"1.2.3.4"}]);
}

}  // namespace
