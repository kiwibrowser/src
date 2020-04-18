// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_MD_DOWNLOAD_ITEM_PROGRESS_INDICATOR_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_MD_DOWNLOAD_ITEM_PROGRESS_INDICATOR_H_

#import <AppKit/AppKit.h>

enum class MDDownloadItemProgressIndicatorState {
  kNotStarted,
  kInProgress,
  kComplete,
};

// MDDownloadItemProgressIndicator is the animated, circular progress bar
// that's part of each item in the download shelf.
@interface MDDownloadItemProgressIndicator : NSView

// Freezes the indeterminate animation if YES, resumes if NO.
@property(nonatomic) BOOL paused;

// Sets the desired state and progress of the progress indicator. Animated.
// - state: Currently only expected to advance or stay the same.
// - progress: A number <= 1. < 0 means "unknown".
// - animations: A block of animations to run alongside the progress
//   indicator's animation. May run after a delay if transition is already in
//   progress.
// - completion: A block to be run once the animation is complete.
- (void)setState:(MDDownloadItemProgressIndicatorState)state
        progress:(CGFloat)progress
      animations:(void (^)())animations
      completion:(void (^)())completion;
@end

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_MD_DOWNLOAD_ITEM_PROGRESS_INDICATOR_H_
