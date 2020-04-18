// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_CONSUMER_H_

#import <UIKit/UIKit.h>

@protocol OmniboxConsumer<NSObject>

// Notifies the consumer to update the autocomplete icon for the currently
// highlighted autocomplete result.
- (void)updateAutocompleteIcon:(UIImage*)icon;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_CONSUMER_H_
