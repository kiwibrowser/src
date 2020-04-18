// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/download/download_util_mac.h"

#include "base/files/file_path.h"
#include "base/strings/sys_string_conversions.h"

namespace download_util {

void AddFileToPasteboard(NSPasteboard* pasteboard, const base::FilePath& path) {
  // Write information about the file being dragged to the pasteboard.
  NSString* file = base::SysUTF8ToNSString(path.value());
  NSArray* fileList = [NSArray arrayWithObject:file];
  [pasteboard declareTypes:[NSArray arrayWithObject:NSFilenamesPboardType]
                     owner:nil];
  [pasteboard setPropertyList:fileList forType:NSFilenamesPboardType];
}

}  // namespace download_util
