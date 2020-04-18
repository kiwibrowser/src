// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "Unpacker.h"

#import <AppKit/AppKit.h>
#include <DiskArbitration/DiskArbitration.h>
#include <dispatch/dispatch.h>

#import "Downloader.h"

@interface Unpacker () {
  NSURL* temporaryDirectoryURL_;
  NSString* mountPath_;

  NSTask* __weak mountTask_;

  DASessionRef session_;
  dispatch_queue_t unpack_dq_;
}
- (void)didFinishEjectingDisk:(DADiskRef)disk
                withDissenter:(DADissenterRef)dissenter;
@end

static void eject_callback(DADiskRef disk,
                           DADissenterRef dissenter,
                           void* context) {
  Unpacker* unpacker = (__bridge_transfer Unpacker*)context;
  [unpacker didFinishEjectingDisk:disk withDissenter:dissenter];
}

static void unmount_callback(DADiskRef disk,
                             DADissenterRef dissenter,
                             void* context) {
  if (dissenter) {
    Unpacker* unpacker = (__bridge Unpacker*)context;
    [unpacker didFinishEjectingDisk:disk withDissenter:dissenter];
  } else {
    DADiskEject(disk, kDADiskEjectOptionDefault, eject_callback, context);
  }
}

@implementation Unpacker

@synthesize delegate = delegate_;
@synthesize appPath = appPath_;

- (void)cleanUp {
  [mountTask_ terminate];
  // It's not the end of the world if this temporary directory is not removed
  // here. The directory will be deleted when the operating system itself
  // decides to anyway.
  [[NSFileManager defaultManager] removeItemAtURL:temporaryDirectoryURL_
                                            error:nil];
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

// TODO: The failure delegate methods need to be revised to meaningfully deal
// with the errors (pipe in stderr / stdout to handle the error according to
// what the error was).
- (void)mountDMGFromURL:(NSURL*)fileURL {
  NSError* error = nil;
  temporaryDirectoryURL_ = [[NSFileManager defaultManager]
        URLForDirectory:NSItemReplacementDirectory
               inDomain:NSUserDomainMask
      appropriateForURL:[NSURL fileURLWithPath:@"/" isDirectory:YES]
                 create:YES
                  error:&error];
  if (error) {
    [delegate_ unpacker:self onMountFailure:error];
    return;
  }

  NSURL* temporaryDiskImageURL =
      [temporaryDirectoryURL_ URLByAppendingPathComponent:@"GoogleChrome.dmg"];
  mountPath_ = [[temporaryDirectoryURL_ URLByAppendingPathComponent:@"mnt"
                                                        isDirectory:YES] path];
  [[NSFileManager defaultManager] createDirectoryAtPath:mountPath_
                            withIntermediateDirectories:YES
                                             attributes:nil
                                                  error:&error];
  if (error) {
    [delegate_ unpacker:self onMountFailure:error];
    return;
  }

  // If the user closes the app at any time, we make sure that the cleanUp
  // function deletes the temporary folder we just created.
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(cleanUp)
             name:NSApplicationWillTerminateNotification
           object:nil];

  [[NSFileManager defaultManager] moveItemAtURL:fileURL
                                          toURL:temporaryDiskImageURL
                                          error:nil];

  NSString* path = @"/usr/bin/hdiutil";
  NSArray* args = @[
    @"attach", temporaryDiskImageURL, @"-nobrowse", @"-noverify",
    @"-mountpoint", mountPath_
  ];

  NSTask* mountTask = [[NSTask alloc] init];
  mountTask.launchPath = path;
  mountTask.arguments = args;
  mountTask.terminationHandler = ^void(NSTask* task) {
    NSError* error = nil;
    NSString* diskAppPath =
        [NSString pathWithComponents:@[ mountPath_, @"Google Chrome.app" ]];
    NSString* tempAppPath = [[temporaryDirectoryURL_
        URLByAppendingPathComponent:@"Google Chrome.app"] path];
    [[NSFileManager defaultManager] copyItemAtPath:diskAppPath
                                            toPath:tempAppPath
                                             error:&error];
    if (error) {
      [delegate_ unpacker:self onMountFailure:error];
    } else {
      [delegate_ unpacker:self onMountSuccess:tempAppPath];
    }
  };
  mountTask_ = mountTask;
  [mountTask launch];
}

- (void)unmountDMG {
  session_ = DASessionCreate(nil);
  unpack_dq_ =
      dispatch_queue_create("com.google.chrome.unpack", DISPATCH_QUEUE_SERIAL);
  DASessionSetDispatchQueue(session_, unpack_dq_);
  DADiskRef child_disk = DADiskCreateFromVolumePath(
      nil, session_,
      (__bridge CFURLRef)[NSURL fileURLWithPath:mountPath_ isDirectory:YES]);
  DADiskRef whole_disk = DADiskCopyWholeDisk(child_disk);

  DADiskUnmount(whole_disk,
                kDADiskUnmountOptionWhole | kDADiskUnmountOptionForce,
                unmount_callback, (__bridge_retained void*)self);

  CFRelease(whole_disk);
  CFRelease(child_disk);
}

- (void)didFinishEjectingDisk:(DADiskRef)disk
                withDissenter:(DADissenterRef)dissenter {
  DASessionSetDispatchQueue(session_, NULL);
  CFRelease(session_);
  NSError* error = nil;
  if (dissenter) {
    DAReturn status = DADissenterGetStatus(dissenter);
    error = [NSError
        errorWithDomain:@"ChromeErrorDomain"
                   code:err_get_code(status)
               userInfo:@{
                 NSLocalizedDescriptionKey :
                     (__bridge NSString*)DADissenterGetStatusString(dissenter)
               }];
    [delegate_ unpacker:self onUnmountFailure:error];
  } else {
    [self cleanUp];
    [delegate_ unpacker:self onUnmountSuccess:mountPath_];
  }
}

@end
