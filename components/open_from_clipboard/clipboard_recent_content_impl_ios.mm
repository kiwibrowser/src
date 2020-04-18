// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/open_from_clipboard/clipboard_recent_content_impl_ios.h"

#import <CommonCrypto/CommonDigest.h>
#import <UIKit/UIKit.h>

#import "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/sys_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Key used to store the pasteboard's current change count. If when resuming
// chrome the pasteboard's change count is different from the stored one, then
// it means that the pasteboard's content has changed.
NSString* const kPasteboardChangeCountKey = @"PasteboardChangeCount";
// Key used to store the last date at which it was detected that the pasteboard
// changed. It is used to evaluate the age of the pasteboard's content.
NSString* const kPasteboardChangeDateKey = @"PasteboardChangeDate";
// Key used to store the hash of the content of the pasteboard. Whenever the
// hash changed, the pasteboard content is considered to have changed.
NSString* const kPasteboardEntryMD5Key = @"PasteboardEntryMD5";

// Compute a hash consisting of the first 4 bytes of the MD5 hash of |string|.
// This value is used to detect pasteboard content change. Keeping only 4 bytes
// is a privacy requirement to introduce collision and allow deniability of
// having copied a given string.
NSData* WeakMD5FromNSString(NSString* string) {
  unsigned char hash[CC_MD5_DIGEST_LENGTH];
  const std::string clipboard = base::SysNSStringToUTF8(string);
  const char* c_string = clipboard.c_str();
  CC_MD5(c_string, strlen(c_string), hash);
  NSData* data = [NSData dataWithBytes:hash length:4];
  return data;
}

}  // namespace

@interface ClipboardRecentContentImplIOS ()

// The user defaults from the app group used to optimize the pasteboard change
// detection.
@property(nonatomic, strong) NSUserDefaults* sharedUserDefaults;
// The pasteboard's change count. Increases everytime the pasteboard changes.
@property(nonatomic) NSInteger lastPasteboardChangeCount;
// MD5 hash of the last registered pasteboard entry.
@property(nonatomic, strong) NSData* lastPasteboardEntryMD5;
// Contains the authorized schemes for URLs.
@property(nonatomic, readonly) NSSet* authorizedSchemes;
// Delegate for metrics.
@property(nonatomic, strong) id<ClipboardRecentContentDelegate> delegate;
// Maximum age of clipboard in seconds.
@property(nonatomic, readonly) NSTimeInterval maximumAgeOfClipboard;

// If the content of the pasteboard has changed, updates the change count,
// change date, and md5 of the latest pasteboard entry if necessary.
- (void)updateIfNeeded;

// Returns whether the pasteboard changed since the last time a pasteboard
// change was detected.
- (BOOL)hasPasteboardChanged;

// Loads information from the user defaults about the latest pasteboard entry.
- (void)loadFromUserDefaults;

// Returns the URL contained in the clipboard (if any).
- (NSURL*)URLFromPasteboard;

// Returns the uptime.
- (NSTimeInterval)uptime;

@end

@implementation ClipboardRecentContentImplIOS

@synthesize lastPasteboardChangeCount = _lastPasteboardChangeCount;
@synthesize lastPasteboardChangeDate = _lastPasteboardChangeDate;
@synthesize lastPasteboardEntryMD5 = _lastPasteboardEntryMD5;
@synthesize sharedUserDefaults = _sharedUserDefaults;
@synthesize authorizedSchemes = _authorizedSchemes;
@synthesize delegate = _delegate;
@synthesize maximumAgeOfClipboard = _maximumAgeOfClipboard;

- (instancetype)initWithMaxAge:(NSTimeInterval)maxAge
             authorizedSchemes:(NSSet<NSString*>*)authorizedSchemes
                  userDefaults:(NSUserDefaults*)groupUserDefaults
                      delegate:(id<ClipboardRecentContentDelegate>)delegate {
  self = [super init];
  if (self) {
    _maximumAgeOfClipboard = maxAge;
    _delegate = delegate;
    _authorizedSchemes = authorizedSchemes;
    _sharedUserDefaults = groupUserDefaults;

    _lastPasteboardChangeCount = NSIntegerMax;
    [self loadFromUserDefaults];
    [self updateIfNeeded];

    // Makes sure |last_pasteboard_change_count_| was properly initialized.
    DCHECK_NE(_lastPasteboardChangeCount, NSIntegerMax);
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(didBecomeActive:)
               name:UIApplicationDidBecomeActiveNotification
             object:nil];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)didBecomeActive:(NSNotification*)notification {
  [self loadFromUserDefaults];
  [self updateIfNeeded];
}

- (NSData*)getCurrentMD5 {
  NSString* pasteboardString = [UIPasteboard generalPasteboard].string;
  NSData* md5 = WeakMD5FromNSString(pasteboardString);

  return md5;
}

- (BOOL)hasPasteboardChanged {
  // If |MD5Changed|, we know for sure there has been at least one pasteboard
  // copy since last time it was checked.
  // If the pasteboard content is still the same but the device was not
  // rebooted, the change count can be checked to see if it changed.
  // Note: due to a mismatch between the actual behavior and documentation, and
  // lack of consistency on different reboot scenarios, the change count cannot
  // be checked after a reboot.
  // See radar://21833556 for more information.
  BOOL deviceRebooted = [self clipboardContentAge] >= [self uptime];
  if (!deviceRebooted) {
    NSInteger changeCount = [UIPasteboard generalPasteboard].changeCount;
    bool changeCountChanged = changeCount != self.lastPasteboardChangeCount;
    return changeCountChanged;
  }

  BOOL md5Changed =
      ![[self getCurrentMD5] isEqualToData:self.lastPasteboardEntryMD5];
  return md5Changed;
}

- (NSURL*)recentURLFromClipboard {
  [self updateIfNeeded];
  if ([self clipboardContentAge] > self.maximumAgeOfClipboard) {
    return nil;
  }
  return [self URLFromPasteboard];
}

- (NSTimeInterval)clipboardContentAge {
  return -[self.lastPasteboardChangeDate timeIntervalSinceNow];
}

- (void)suppressClipboardContent {
  // User cleared the user data. The pasteboard entry must be removed from the
  // omnibox list. Force entry expiration by setting copy date to 1970.
  self.lastPasteboardChangeDate =
      [[NSDate alloc] initWithTimeIntervalSince1970:0];
  [self saveToUserDefaults];
}

- (void)updateIfNeeded {
  if (![self hasPasteboardChanged]) {
    return;
  }

  [self.delegate onClipboardChanged];

  self.lastPasteboardChangeDate = [NSDate date];
  self.lastPasteboardChangeCount = [UIPasteboard generalPasteboard].changeCount;
  self.lastPasteboardEntryMD5 = [self getCurrentMD5];

  [self saveToUserDefaults];
}

- (NSURL*)URLFromPasteboard {
  NSString* clipboardString = [UIPasteboard generalPasteboard].string;

  NSURL* url = [NSURL URLWithString:clipboardString];
  if (![self.authorizedSchemes containsObject:url.scheme]) {
    return nil;
  }
  return url;
}

- (void)loadFromUserDefaults {
  self.lastPasteboardChangeCount =
      [self.sharedUserDefaults integerForKey:kPasteboardChangeCountKey];
  self.lastPasteboardChangeDate = base::mac::ObjCCastStrict<NSDate>(
      [self.sharedUserDefaults objectForKey:kPasteboardChangeDateKey]);
  self.lastPasteboardEntryMD5 = base::mac::ObjCCastStrict<NSData>(
      [self.sharedUserDefaults objectForKey:kPasteboardEntryMD5Key]);
}

- (void)saveToUserDefaults {
  [self.sharedUserDefaults setInteger:self.lastPasteboardChangeCount
                               forKey:kPasteboardChangeCountKey];
  [self.sharedUserDefaults setObject:self.lastPasteboardChangeDate
                              forKey:kPasteboardChangeDateKey];
  [self.sharedUserDefaults setObject:self.lastPasteboardEntryMD5
                              forKey:kPasteboardEntryMD5Key];
}

- (NSTimeInterval)uptime {
  return base::SysInfo::Uptime().InSecondsF();
}

@end
