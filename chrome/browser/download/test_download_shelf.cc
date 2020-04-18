// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/test_download_shelf.h"

#include "content/public/browser/download_manager.h"

TestDownloadShelf::TestDownloadShelf()
    : is_showing_(false),
      did_add_download_(false),
      download_manager_(NULL) {
}

TestDownloadShelf::~TestDownloadShelf() {
  if (download_manager_)
    download_manager_->RemoveObserver(this);
}

bool TestDownloadShelf::IsShowing() const {
  return is_showing_;
}

bool TestDownloadShelf::IsClosing() const {
  return false;
}

Browser* TestDownloadShelf::browser() const {
  return NULL;
}

void TestDownloadShelf::set_download_manager(
    content::DownloadManager* download_manager) {
  if (download_manager_)
    download_manager_->RemoveObserver(this);
  download_manager_ = download_manager;
  if (download_manager_)
    download_manager_->AddObserver(this);
}

void TestDownloadShelf::ManagerGoingDown(content::DownloadManager* manager) {
  DCHECK_EQ(manager, download_manager_);
  download_manager_ = NULL;
}

void TestDownloadShelf::DoAddDownload(download::DownloadItem* download) {
  did_add_download_ = true;
}

void TestDownloadShelf::DoOpen() {
  is_showing_ = true;
}

void TestDownloadShelf::DoClose(CloseReason reason) {
  is_showing_ = false;
}

void TestDownloadShelf::DoHide() {
  is_showing_ = false;
}

void TestDownloadShelf::DoUnhide() {
  is_showing_ = true;
}

base::TimeDelta TestDownloadShelf::GetTransientDownloadShowDelay() {
  return base::TimeDelta();
}

content::DownloadManager* TestDownloadShelf::GetDownloadManager() {
  return download_manager_;
}
