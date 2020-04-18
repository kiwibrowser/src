// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "Downloader.h"

#include <assert.h>

@implementation Downloader

@synthesize delegate = delegate_;

// Downloads contents of chromeURL to downloads folders and delegates the work
// to the DownloadDelegate class.
- (void)downloadChromeImageFrom:(NSURL*)chromeImageURL {
  NSURLSession* session =
      [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration
                                                 defaultSessionConfiguration]
                                    delegate:self
                               delegateQueue:nil];
  [[session downloadTaskWithURL:chromeImageURL] resume];
  [session finishTasksAndInvalidate];
}

// Provides updates to download progress.
- (void)URLSession:(NSURLSession*)session
                 downloadTask:(NSURLSessionDownloadTask*)downloadTask
                 didWriteData:(int64_t)bytesWritten
            totalBytesWritten:(int64_t)totalBytesWritten
    totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite {
  double downloadProgressPercentage =
      (double)totalBytesWritten / totalBytesExpectedToWrite * 100.0;
  [delegate_ downloader:self percentProgress:downloadProgressPercentage];
}

- (void)URLSession:(NSURLSession*)session
                 downloadTask:(NSURLSessionDownloadTask*)downloadTask
    didFinishDownloadingToURL:(NSURL*)location {
  assert([location isFileURL]);
  [delegate_ downloader:self onSuccess:location];
}

- (void)URLSession:(NSURLSession*)session
                    task:(NSURLSessionTask*)task
    didCompleteWithError:(NSError*)error {
  if (error) {
    [delegate_ downloader:self onFailure:error];
  }
}

@end
