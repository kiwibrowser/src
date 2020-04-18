// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/common/mac/app_mode_chrome_locator.h"

#import <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/mac/app_mode_common.h"

namespace app_mode {

bool FindBundleById(NSString* bundle_id, base::FilePath* out_bundle) {
  NSWorkspace* ws = [NSWorkspace sharedWorkspace];
  NSString *bundlePath = [ws absolutePathForAppBundleWithIdentifier:bundle_id];
  if (!bundlePath)
    return false;

  *out_bundle = base::mac::NSStringToFilePath(bundlePath);
  return true;
}

NSString* GetVersionedPath(NSString* bundle_path, NSString* version) {
  return [NSString
      pathWithComponents:@[ bundle_path, @"Contents", @"Versions", version ]];
}

bool GetChromeBundleInfo(const base::FilePath& chrome_bundle,
                         const std::string& version_str,
                         base::FilePath* executable_path,
                         base::FilePath* version_path,
                         base::FilePath* framework_shlib_path) {
  using base::mac::ObjCCast;

  NSString* cr_bundle_path = base::mac::FilePathToNSString(chrome_bundle);
  NSBundle* cr_bundle = [NSBundle bundleWithPath:cr_bundle_path];

  if (!cr_bundle)
    return false;

  // Get versioned directory.
  NSString* cr_versioned_path;
  if (!version_str.empty()) {
    cr_versioned_path =
        GetVersionedPath(cr_bundle_path, base::SysUTF8ToNSString(version_str));
  }

  if (version_str.empty() ||
      !base::PathExists(base::mac::NSStringToFilePath(cr_versioned_path))) {
    // Read version string.
    NSString* cr_version = ObjCCast<NSString>(
        [cr_bundle objectForInfoDictionaryKey:app_mode::kCrBundleVersionKey]);
    if (!cr_version) {
      // Older bundles have the Chrome version in the following key.
      cr_version = ObjCCast<NSString>([cr_bundle
          objectForInfoDictionaryKey:app_mode::kCFBundleShortVersionStringKey]);
    }
    if (!cr_version)
      return false;

    cr_versioned_path = GetVersionedPath(cr_bundle_path, cr_version);
  }

  // Get the framework path.
  NSString* cr_bundle_exe =
      ObjCCast<NSString>(
          [cr_bundle objectForInfoDictionaryKey:@"CFBundleExecutable"]);
  // Essentially we want chrome::kFrameworkName which looks like
  // "$PRODUCT_STRING Framework.framework". The library itself is at
  // "$PRODUCT_STRING Framework.framework/$PRODUCT_STRING Framework". Note that
  // $PRODUCT_STRING is not |cr_bundle_exe| because in Canary the framework is
  // still called "Google Chrome Framework".
  // However, we want the shims to be agnostic to distribution and operate based
  // on the data in their plist, so encode the framework names here.
  NSDictionary* framework_for_exe = @{
    @"Chromium": @"Chromium",
    @"Google Chrome": @"Google Chrome",
    @"Google Chrome Canary": @"Google Chrome",
  };
  NSString* framework_name = [framework_for_exe objectForKey:cr_bundle_exe];
  NSString* cr_framework_shlib_path =
      [cr_versioned_path stringByAppendingPathComponent:
          [framework_name stringByAppendingString:@" Framework.framework"]];
  cr_framework_shlib_path =
      [cr_framework_shlib_path stringByAppendingPathComponent:
          [framework_name stringByAppendingString:@" Framework"]];
  if (!cr_bundle_exe || !cr_framework_shlib_path)
    return false;

  // A few more sanity checks.
  BOOL is_directory;
  BOOL exists = [[NSFileManager defaultManager]
                    fileExistsAtPath:cr_framework_shlib_path
                         isDirectory:&is_directory];
  if (!exists || is_directory)
    return false;

  // Everything OK, copy output parameters.
  *executable_path = base::mac::NSStringToFilePath([cr_bundle executablePath]);
  *version_path = base::mac::NSStringToFilePath(cr_versioned_path);
  *framework_shlib_path =
      base::mac::NSStringToFilePath(cr_framework_shlib_path);
  return true;
}

}  // namespace app_mode
