// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_MAC_APP_DOWNLOADER_H_
#define CHROME_INSTALLER_MAC_APP_DOWNLOADER_H_

#import <Foundation/Foundation.h>

@class Downloader;
@protocol DownloaderDelegate
- (void)downloader:(Downloader*)download percentProgress:(double)percentage;
- (void)downloader:(Downloader*)download onSuccess:(NSURL*)diskImageURL;
- (void)downloader:(Downloader*)download onFailure:(NSError*)error;
@end

@interface Downloader : NSObject<NSURLSessionDownloadDelegate>

@property(nonatomic, assign) id<DownloaderDelegate> delegate;

// Downloads Chrome from |chromeImageURL| to the local hard drive.
- (void)downloadChromeImageFrom:(NSURL*)chromeImageURL;

@end

#endif  // CHROME_INSTALLER_MAC_APP_DOWNLOADER_H_
