// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_DIRECTORY_UTIL_H_
#define IOS_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_DIRECTORY_UTIL_H_

namespace base {
class FilePath;
}

// Fills |directory_path| with the FilePath to the downloads directory. Returns
// true if this is successful. This method does not create the directory, it
// just returns the path.
bool GetDownloadsDirectory(base::FilePath* directory_path);

// Asynchronously deletes downloads directory.
void DeleteDownloadsDirectory();

#endif  // IOS_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_DIRECTORY_UTIL_H_
