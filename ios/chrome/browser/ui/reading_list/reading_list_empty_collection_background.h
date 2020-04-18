// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_EMPTY_COLLECTION_BACKGROUND_H_
#define IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_EMPTY_COLLECTION_BACKGROUND_H_

#import <UIKit/UIKit.h>

@interface ReadingListEmptyCollectionBackground : UIView

- (instancetype)init NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;

+ (NSString*)accessibilityIdentifier;

@end

#endif  // IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_EMPTY_COLLECTION_BACKGROUND_H_
