// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_MANAGER_DELEGATE_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_MANAGER_DELEGATE_H_

#include <stdint.h>
#include <string>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/public/browser/download_manager_delegate.h"

namespace content {

class DownloadManager;

namespace protocol {

class CONTENT_EXPORT DevToolsDownloadManagerDelegate
    : public content::DownloadManagerDelegate,
      public base::RefCounted<DevToolsDownloadManagerDelegate> {
 public:
  // Takes over a |download_manager|. If the |download_manager| owns a
  // |DownloadManagerDelegate| it will be stored as a proxy delegate.
  // When the proxy is set, this delegate will use the proxy's |GetNextId|
  // function to ensure compatibility. It will also call its |Shutdown| method
  // when sutting down and it will fallback to the proxy if it cannot find any
  // DevToolsDownloadManagerHelper associated with the download.
  static scoped_refptr<DevToolsDownloadManagerDelegate> TakeOver(
      content::DownloadManager* download_manager);

  // DownloadManagerDelegate overrides.
  void Shutdown() override;
  bool DetermineDownloadTarget(
      download::DownloadItem* download,
      const content::DownloadTargetCallback& callback) override;
  bool ShouldOpenDownload(
      download::DownloadItem* item,
      const content::DownloadOpenDelayedCallback& callback) override;
  void GetNextId(const content::DownloadIdCallback& callback) override;

 private:
  friend class base::RefCounted<DevToolsDownloadManagerDelegate>;

  DevToolsDownloadManagerDelegate();
  static DevToolsDownloadManagerDelegate* GetInstance();
  ~DevToolsDownloadManagerDelegate() override;

  typedef base::Callback<void(const base::FilePath&)>
      FilenameDeterminedCallback;

  static void GenerateFilename(const GURL& url,
                               const std::string& content_disposition,
                               const std::string& suggested_filename,
                               const std::string& mime_type,
                               const base::FilePath& suggested_directory,
                               const FilenameDeterminedCallback& callback);

  void OnDownloadPathGenerated(uint32_t download_id,
                               const content::DownloadTargetCallback& callback,
                               const base::FilePath& suggested_path);

  content::DownloadManager* download_manager_;
  content::DownloadManagerDelegate* proxy_download_delegate_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsDownloadManagerDelegate);
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_MANAGER_DELEGATE_H_
