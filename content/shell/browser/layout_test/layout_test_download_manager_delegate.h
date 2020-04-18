// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_DOWNLOAD_MANAGER_DELEGATE_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_DOWNLOAD_MANAGER_DELEGATE_H_

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/download_manager_delegate.h"
#include "content/shell/browser/shell_download_manager_delegate.h"

namespace download {
class DownloadItem;
}

namespace content {

class LayoutTestDownloadManagerDelegate : public ShellDownloadManagerDelegate {
 public:
  LayoutTestDownloadManagerDelegate();
  ~LayoutTestDownloadManagerDelegate() override;

  // ShellDownloadManagerDelegate implementation.
  bool ShouldOpenDownload(download::DownloadItem* item,
                          const DownloadOpenDelayedCallback& callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LayoutTestDownloadManagerDelegate);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_DOWNLOAD_MANAGER_DELEGATE_H_
