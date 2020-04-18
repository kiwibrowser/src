// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OPEN_FROM_CLIPBOARD_CLIPBOARD_RECENT_CONTENT_IMPL_IOS_H_
#define COMPONENTS_OPEN_FROM_CLIPBOARD_CLIPBOARD_RECENT_CONTENT_IMPL_IOS_H_

#import <Foundation/Foundation.h>

// A protocol implemented by delegates to handle clipboard changes.
@protocol ClipboardRecentContentDelegate<NSObject>

- (void)onClipboardChanged;

@end

// Helper class returning a URL if the content of the clipboard can be turned
// into a URL, and if it estimates that the content of the clipboard is not too
// old.
@interface ClipboardRecentContentImplIOS : NSObject

// |delegate| is used for metrics logging and can be nil. |authorizedSchemes|
// should contain all schemes considered valid. |groupUserDefaults| is the
// NSUserDefaults used to store information on pasteboard entry expiration. This
// information will be shared with other applications in the application group.
- (instancetype)initWithMaxAge:(NSTimeInterval)maxAge
             authorizedSchemes:(NSSet<NSString*>*)authorizedSchemes
                  userDefaults:(NSUserDefaults*)groupUserDefaults
                      delegate:(id<ClipboardRecentContentDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Returns the copied URL if the clipboard contains a recent URL that has not
// been supressed. Otherwise, returns nil.
- (NSURL*)recentURLFromClipboard;

// Returns how old the content of the clipboard is.
- (NSTimeInterval)clipboardContentAge;

// Prevents GetRecentURLFromClipboard from returning anything until the
// clipboard's content changes.
- (void)suppressClipboardContent;

// Methods below are exposed for testing purposes.

// Estimation of the date when the pasteboard changed.
@property(nonatomic, strong) NSDate* lastPasteboardChangeDate;

// Saves information to the user defaults about the latest pasteboard entry.
- (void)saveToUserDefaults;

@end

#endif  // COMPONENTS_OPEN_FROM_CLIPBOARD_CLIPBOARD_RECENT_CONTENT_IMPL_IOS_H_
