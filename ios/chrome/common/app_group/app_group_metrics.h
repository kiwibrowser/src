// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_COMMON_APP_GROUP_APP_GROUP_METRICS_H_
#define IOS_CHROME_COMMON_APP_GROUP_APP_GROUP_METRICS_H_

#import <Foundation/Foundation.h>

#include "base/mac/scoped_block.h"
#include "ios/chrome/common/app_group/app_group_constants.h"

namespace app_group {

// Suffix to the name of file containing logs ready for upload.
extern NSString* const kPendingLogFileSuffix;

// Directory containing the logs produced by extensions that are ready for
// upload.
extern NSString* const kPendingLogFileDirectory;

// An app_group key to the number of times Search Extension was displayed since
// last Chrome launch.
extern NSString* const kSearchExtensionDisplayCount;

// An app_group key to the number of times Content Extension was displayed since
// last Chrome launch.
extern NSString* const kContentExtensionDisplayCount;

// Offsets the sessionID to avoid collision. The sessionID is limited to 1<<23.
int AppGroupSessionID(int sessionID, AppGroupApplications application);

}  // namespace app_group

#endif  // IOS_CHROME_COMMON_APP_GROUP_APP_GROUP_METRICS_H_
