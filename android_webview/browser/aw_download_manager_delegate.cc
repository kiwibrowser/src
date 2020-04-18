// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_download_manager_delegate.h"

#include "base/files/file_path.h"
#include "components/download/public/common/download_danger_type.h"
#include "components/download/public/common/download_item.h"

namespace android_webview {

AwDownloadManagerDelegate::~AwDownloadManagerDelegate() {}

bool AwDownloadManagerDelegate::DetermineDownloadTarget(
    download::DownloadItem* item,
    const content::DownloadTargetCallback& callback) {
  // Note this cancel is independent of the URLRequest cancel in
  // AwResourceDispatcherHostDelegate::DownloadStarting. The request
  // could have already finished by the time DownloadStarting is called.
  callback.Run(base::FilePath() /* Empty file path for cancel */,
               download::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
               download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, base::FilePath(),
               download::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED);
  return true;
}

bool AwDownloadManagerDelegate::ShouldCompleteDownload(
    download::DownloadItem* item,
    const base::Closure& complete_callback) {
  NOTREACHED();
  return true;
}

bool AwDownloadManagerDelegate::ShouldOpenDownload(
    download::DownloadItem* item,
    const content::DownloadOpenDelayedCallback& callback) {
  NOTREACHED();
  return true;
}

void AwDownloadManagerDelegate::GetNextId(
    const content::DownloadIdCallback& callback) {
  static uint32_t next_id = download::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

}  // namespace android_webview
