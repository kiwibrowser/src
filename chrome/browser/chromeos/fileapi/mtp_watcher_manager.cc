// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/mtp_watcher_manager.h"

#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace chromeos {

MTPWatcherManager::MTPWatcherManager(
    DeviceMediaAsyncFileUtil* device_media_async_file_util)
    : device_media_async_file_util_(device_media_async_file_util) {
  DCHECK(device_media_async_file_util != NULL);
}

MTPWatcherManager::~MTPWatcherManager() {
}

void MTPWatcherManager::AddWatcher(
    const storage::FileSystemURL& url,
    bool recursive,
    const StatusCallback& callback,
    const NotificationCallback& notification_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  device_media_async_file_util_->AddWatcher(url, recursive, callback,
                                            notification_callback);
}

void MTPWatcherManager::RemoveWatcher(const storage::FileSystemURL& url,
                                      bool recursive,
                                      const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  device_media_async_file_util_->RemoveWatcher(url, recursive, callback);
}

}  // namespace chromeos
