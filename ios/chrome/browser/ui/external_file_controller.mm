// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_file_controller.h"

#import <WebKit/WebKit.h>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/web/public/web_view_creation_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// The path relative to the <Application_Home>/Documents/ directory where the
// files received from other applications are saved.
NSString* const kInboxPath = @"Inbox";

// Conversion factor to turn number of days to number of seconds.
const CFTimeInterval kSecondsPerDay = 60 * 60 * 24;

}  // namespace

@interface ExternalFileController ()
// Returns the path to the Inbox directory.
+ (NSString*)inboxDirectoryPath;
@end

@implementation ExternalFileController {
  WKWebView* _webView;
}

- (instancetype)initWithURL:(const GURL&)URL
               browserState:(web::BrowserState*)browserState {
  self = [super initWithURL:URL];
  if (self) {
    _webView = web::BuildWKWebView(CGRectZero, browserState);
    [_webView setBackgroundColor:[UIColor whiteColor]];
    [_webView setAutoresizingMask:(UIViewAutoresizingFlexibleWidth |
                                   UIViewAutoresizingFlexibleHeight)];
    [_webView setUserInteractionEnabled:YES];
    UIView* view = [[UIView alloc] initWithFrame:CGRectZero];
    self.view = view;
    [self.view addSubview:_webView];
    [self.view setAutoresizingMask:(UIViewAutoresizingFlexibleWidth |
                                    UIViewAutoresizingFlexibleHeight)];
    NSString* filePath = [ExternalFileController pathForExternalFileURL:URL];
    DCHECK([[NSFileManager defaultManager] fileExistsAtPath:filePath]);
    // TODO(cgrigoruta): Currently only PDFs are supported. Change it to support
    // other file types as well.
    NSURL* fileURL = [NSURL fileURLWithPath:filePath];
    DCHECK(fileURL);
    [_webView loadFileURL:fileURL allowingReadAccessToURL:fileURL];
  }
  return self;
}

+ (NSString*)inboxDirectoryPath {
  NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                       NSUserDomainMask, YES);
  if ([paths count] < 1)
    return nil;

  NSString* documentsDirectoryPath = [paths objectAtIndex:0];
  return [documentsDirectoryPath stringByAppendingPathComponent:kInboxPath];
}

+ (NSString*)pathForExternalFileURL:(const GURL&)url {
  NSString* fileName = base::SysUTF8ToNSString(url.ExtractFileName());
  return [[ExternalFileController inboxDirectoryPath]
      stringByAppendingPathComponent:[fileName
                                         stringByRemovingPercentEncoding]];
}

+ (void)removeFilesExcluding:(NSSet*)filesToKeep
                   olderThan:(NSInteger)ageInDays {
  base::AssertBlockingAllowed();
  NSFileManager* fileManager = [NSFileManager defaultManager];
  NSString* inboxDirectory = [ExternalFileController inboxDirectoryPath];
  NSArray* externalFiles =
      [fileManager contentsOfDirectoryAtPath:inboxDirectory error:nil];
  for (NSString* filename in externalFiles) {
    NSString* filePath =
        [inboxDirectory stringByAppendingPathComponent:filename];
    if ([filesToKeep containsObject:filename])
      continue;
    // Checks the age of the file and do not remove files that are too recent.
    // Under normal circumstances, e.g. when file purge is not initiated by
    // user action, leave recently downloaded files around to avoid users
    // using history back or recent tabs to reach an external file that was
    // pre-maturely purged.
    NSError* error = nil;
    NSDictionary* attributesDictionary =
        [fileManager attributesOfItemAtPath:filePath error:&error];
    if (error) {
      DLOG(ERROR) << "Failed to retrieve attributes for " << filePath << ": "
                  << base::SysNSStringToUTF8([error description]);
      continue;
    }
    NSDate* date = [attributesDictionary objectForKey:NSFileCreationDate];
    if (-[date timeIntervalSinceNow] <= (ageInDays * kSecondsPerDay))
      continue;
    // Removes the file.
    [fileManager removeItemAtPath:filePath error:&error];
    if (error) {
      DLOG(ERROR) << "Failed to remove file " << filePath << ": "
                  << base::SysNSStringToUTF8([error description]);
      continue;
    }
  }
}

- (void)dealloc {
  [_webView removeFromSuperview];
}

@end
