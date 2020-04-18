// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_LOCATION_DIALOG_RESULT_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_LOCATION_DIALOG_RESULT_H_

// Result of download location dialog.
// Recorded in histogram, so do not delete or reuse entries. The values must
// match DownloadLocationDialogResult in enums.xml.
enum class DownloadLocationDialogResult {
  USER_CONFIRMED = 0,    // User confirmed a file path.
  USER_CANCELED = 1,     // User canceled file path selection.
  DUPLICATE_DIALOG = 2,  // Dialog is already showing.
  kMaxValue = DUPLICATE_DIALOG
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_LOCATION_DIALOG_RESULT_H_
