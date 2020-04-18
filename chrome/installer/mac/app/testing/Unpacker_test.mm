// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/installer/mac/app/Unpacker.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

#import "chrome/installer/mac/app/Downloader.h"

@interface TestDelegate : NSObject<UnpackDelegate>
@property(nonatomic) BOOL pass;
@property(nonatomic) dispatch_semaphore_t test_semaphore;
- (void)fail;
- (void)succeed;
- (void)wait;
@end

@implementation TestDelegate
@synthesize pass = pass_;
@synthesize test_semaphore = test_semaphore_;

- (id)init {
  if ((self = [super init])) {
    test_semaphore_ = dispatch_semaphore_create(0);
    pass_ = NO;
  }
  return self;
}

- (void)succeed {
  pass_ = YES;
  dispatch_semaphore_signal(test_semaphore_);
}
- (void)fail {
  pass_ = NO;
  dispatch_semaphore_signal(test_semaphore_);
}
- (void)wait {
  dispatch_semaphore_wait(test_semaphore_, DISPATCH_TIME_FOREVER);
}

- (void)unpacker:(Unpacker*)unpacker onMountSuccess:(NSString*)tempAppPath {
  if ([[NSFileManager defaultManager] fileExistsAtPath:tempAppPath]) {
    [self succeed];
  } else {
    [self fail];
  }
}
- (void)unpacker:(Unpacker*)unpacker onMountFailure:(NSError*)error {
  [self fail];
}

- (void)unpacker:(Unpacker*)unpacker onUnmountSuccess:(NSString*)mountpath {
  if (![[NSFileManager defaultManager]
          fileExistsAtPath:[NSString pathWithComponents:@[
            mountpath, @"Google Chrome.app"
          ]]]) {
    [self succeed];
  } else {
    [self fail];
  }
}
- (void)unpacker:(Unpacker*)unpacker onUnmountFailure:(NSError*)error {
  [self fail];
}

@end

namespace {

TEST(UnpackerTest, IntegrationTest) {
  // Create objects and semaphore
  Unpacker* unpack = [[Unpacker alloc] init];
  TestDelegate* test_delegate = [[TestDelegate alloc] init];
  unpack.delegate = test_delegate;

  // Get a disk image to use to test
  base::FilePath originalPath;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &originalPath);
  originalPath = originalPath.AppendASCII("chrome/test/data/mac_installer/");
  base::FilePath copiedPath = base::FilePath(originalPath);
  NSString* diskImageOriginalPath = base::SysUTF8ToNSString(
      (originalPath.AppendASCII("test-dmg.dmg")).value());
  NSString* diskImageCopiedPath = base::SysUTF8ToNSString(
      (originalPath.AppendASCII("test-dmg2.dmg")).value());
  // The unpacker moves (not copies) a downloaded disk image directly into its
  // own temporary directory, so if the below copy didn't happen, `test-dmg.dmg`
  // would disappear every time this test was run
  [[NSFileManager defaultManager] copyItemAtPath:diskImageOriginalPath
                                          toPath:diskImageCopiedPath
                                           error:nil];
  NSURL* dmgURL = [NSURL fileURLWithPath:diskImageCopiedPath isDirectory:NO];
  // Start mount step
  [unpack mountDMGFromURL:dmgURL];
  [test_delegate wait];

  // Is the disk image mounted?
  ASSERT_TRUE([test_delegate pass]);

  // Start unmount step
  [unpack unmountDMG];
  [test_delegate wait];

  // Is the disk image gone?
  EXPECT_TRUE([test_delegate pass]);
}

}  // namespace
