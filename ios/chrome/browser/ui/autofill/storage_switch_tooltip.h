// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_AUTOFILL_STORAGE_SWITCH_TOOLTIP_H_
#define IOS_CHROME_BROWSER_UI_AUTOFILL_STORAGE_SWITCH_TOOLTIP_H_

#import <UIKit/UIKit.h>

// A label that displays the storage setting tooltip text with appropriate
// layout and styling.
@interface StorageSwitchTooltip : UILabel

- (instancetype)initWithFrame:(CGRect)frame NS_DESIGNATED_INITIALIZER;

- (instancetype)init;

- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_AUTOFILL_STORAGE_SWITCH_TOOLTIP_H_
