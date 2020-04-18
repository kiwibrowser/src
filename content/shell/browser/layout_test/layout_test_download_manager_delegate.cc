// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/layout_test_download_manager_delegate.h"

#if defined(OS_WIN)
#include <windows.h>
#include <commdlg.h>
#endif

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/download_manager.h"
#include "content/shell/browser/layout_test/blink_test_controller.h"
#include "net/base/filename_util.h"

#if defined(OS_WIN)
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#endif

namespace content {

LayoutTestDownloadManagerDelegate::LayoutTestDownloadManagerDelegate()
    : ShellDownloadManagerDelegate() {}

LayoutTestDownloadManagerDelegate::~LayoutTestDownloadManagerDelegate(){
}

bool LayoutTestDownloadManagerDelegate::ShouldOpenDownload(
    download::DownloadItem* item,
    const DownloadOpenDelayedCallback& callback) {
  if (BlinkTestController::Get() &&
      BlinkTestController::Get()->IsMainWindow(
          DownloadItemUtils::GetWebContents(item)) &&
      item->GetMimeType() == "text/html") {
    BlinkTestController::Get()->OpenURL(
        net::FilePathToFileURL(item->GetFullPath()));
  }
  return true;
}

}  // namespace content
