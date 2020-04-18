// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_EXTERNAL_FILE_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_EXTERNAL_FILE_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/native_content_controller.h"

namespace web {
class BrowserState;
}

// Native content controller for presenting external files received from
// other applications. These files are saved in the
// <Application_Home>/Documents/Inbox directory by the iOS framework.
@interface ExternalFileController : NativeContentController
// Initialize with an |url| of the form "chrome://external-file/<file_name>"
// where |file_name| is the name of the file received from another application.
- (instancetype)initWithURL:(const GURL&)URL
               browserState:(web::BrowserState*)browserState;

// Returns the path in the application sandbox of an external file from the
// URL received for that file.
+ (NSString*)pathForExternalFileURL:(const GURL&)url;

// Removes all the files in the Inbox directory that are not in
// |filesToKeep| and that are older than |ageInDays| days.
// |filesToKeep| may be nil if all files should be removed.
+ (void)removeFilesExcluding:(NSSet*)filesToKeep olderThan:(NSInteger)ageInDays;

@end

#endif  // IOS_CHROME_BROWSER_UI_EXTERNAL_FILE_CONTROLLER_H_
