// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_TEST_SCOPED_KEY_WINDOW_H_
#define IOS_CHROME_TEST_SCOPED_KEY_WINDOW_H_

#import <UIKit/UIKit.h>

// Sets temporary key window and returns it via Get method. Saves the current
// key window and restores it on destruction.
class ScopedKeyWindow {
 public:
  explicit ScopedKeyWindow()
      : current_key_window_(
            [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds]),
        original_key_window_([UIApplication sharedApplication].keyWindow) {
    [current_key_window_ makeKeyAndVisible];
  }
  ~ScopedKeyWindow() { [original_key_window_ makeKeyAndVisible]; }
  UIWindow* Get() { return current_key_window_; }

 private:
  __strong UIWindow* current_key_window_;
  __strong UIWindow* original_key_window_;
};

#endif  // IOS_CHROME_TEST_SCOPED_KEY_WINDOW_H_
