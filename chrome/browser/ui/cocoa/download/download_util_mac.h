// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_UTIL_MAC_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_UTIL_MAC_H_

#import <Cocoa/Cocoa.h>

namespace base {
class FilePath;
}

namespace download_util {

void AddFileToPasteboard(NSPasteboard* pasteboard, const base::FilePath& path);

}  // namespace download_util

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_UTIL_MAC_H_
