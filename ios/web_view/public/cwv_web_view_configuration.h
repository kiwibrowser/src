// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_CONFIGURATION_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_CONFIGURATION_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

@class CWVPreferences;
@class CWVUserContentController;
@class CWVWebsiteDataStore;

// Configuration used for creation of a CWVWebView.
CWV_EXPORT
@interface CWVWebViewConfiguration : NSObject

// Configuration with persistent data store which stores all data on disk.
+ (instancetype)defaultConfiguration;

// Configuration with ephemeral data store that neven stores data on disk.
+ (instancetype)incognitoConfiguration;

- (instancetype)init NS_UNAVAILABLE;

// The preferences object associated with this web view configuration.
@property(nonatomic, readonly) CWVPreferences* preferences;

// The user content controller to associate with web views created using this
// configuration.
@property(nonatomic, readonly) CWVUserContentController* userContentController;

// YES if it is a configuration with persistent data store which stores all data
// on disk.
@property(nonatomic, readonly, getter=isPersistent) BOOL persistent;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_CONFIGURATION_H_
