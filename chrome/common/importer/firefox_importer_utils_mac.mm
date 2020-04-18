// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Cocoa/Cocoa.h>
#include <sys/param.h>

#include "chrome/common/importer/firefox_importer_utils.h"

#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"
#include "base/path_service.h"

base::FilePath GetProfilesINI() {
  base::FilePath app_data_path;
  if (!base::PathService::Get(base::DIR_APP_DATA, &app_data_path)) {
    return base::FilePath();
  }
  base::FilePath ini_file =
      app_data_path.Append("Firefox").Append("profiles.ini");
  if (!base::PathExists(ini_file)) {
    return base::FilePath();
  }
  return ini_file;
}

base::FilePath GetFirefoxDylibPath() {
  CFURLRef appURL = nil;
  if (LSFindApplicationForInfo(kLSUnknownCreator,
                              CFSTR("org.mozilla.firefox"),
                              NULL,
                              NULL,
                              &appURL) != noErr) {
    return base::FilePath();
  }
  NSBundle *ff_bundle =
      [NSBundle bundleWithPath:[base::mac::CFToNSCast(appURL) path]];
  CFRelease(appURL);
  NSString *ff_library_path =
      [[ff_bundle executablePath] stringByDeletingLastPathComponent];
  char buf[MAXPATHLEN];
  if (![ff_library_path getFileSystemRepresentation:buf maxLength:sizeof(buf)])
    return base::FilePath();
  return base::FilePath(buf);
}
