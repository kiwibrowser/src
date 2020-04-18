// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_MAC_APP_UNPACKER_H_
#define CHROME_INSTALLER_MAC_APP_UNPACKER_H_

#import <Foundation/Foundation.h>

@class Unpacker;
@protocol UnpackDelegate<NSObject>
- (void)unpacker:(Unpacker*)unpacker onMountSuccess:(NSString*)tempAppPath;
- (void)unpacker:(Unpacker*)unpacker onMountFailure:(NSError*)error;
- (void)unpacker:(Unpacker*)unpacker onUnmountSuccess:(NSString*)mountpath;
- (void)unpacker:(Unpacker*)unpacker onUnmountFailure:(NSError*)error;
@end

@interface Unpacker : NSObject

@property(nonatomic, assign) id<UnpackDelegate> delegate;
@property(nonatomic, copy) NSString* appPath;

// Mount a disk image at |fileURL|.
- (void)mountDMGFromURL:(NSURL*)fileURL;
// Unmount that same disk image.
- (void)unmountDMG;

@end

#endif  // CHROME_INSTALLER_MAC_APP_UNPACKER_H_
