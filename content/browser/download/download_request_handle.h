// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_REQUEST_HANDLE_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_REQUEST_HANDLE_H_

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "components/download/public/common/download_request_handle_interface.h"
#include "content/browser/download/download_resource_handler.h"
#include "content/common/content_export.h"
#include "content/public/browser/resource_request_info.h"

namespace content {
class WebContents;

// A handle used by the download system for operations on the URLRequest
// or objects conditional on it (e.g. WebContentsImpl).
// This class needs to be copyable, so we can pass it across threads and not
// worry about lifetime or const-ness.
class CONTENT_EXPORT DownloadRequestHandle
    : public download::DownloadRequestHandleInterface {
 public:
  DownloadRequestHandle(const DownloadRequestHandle& other);
  ~DownloadRequestHandle() override;

  // Create a null DownloadRequestHandle: getters will return null, and
  // all actions are no-ops.
  // TODO(rdsmith): Ideally, actions would be forbidden rather than
  // no-ops, to confirm that no non-testing code actually uses
  // a null DownloadRequestHandle.  But for now, we need the no-op
  // behavior for unit tests.  Long-term, this should be fixed by
  // allowing mocking of ResourceDispatcherHost in unit tests.
  DownloadRequestHandle();

  DownloadRequestHandle(const base::WeakPtr<DownloadResourceHandler>& handler,
                        const content::ResourceRequestInfo::WebContentsGetter&
                            web_contents_getter);

  WebContents* GetWebContents() const;

  // download::DownloadRequestHandleInterface interface.
  void PauseRequest() override;
  void ResumeRequest() override;
  void CancelRequest(bool user_cancel) override;

 private:
  base::WeakPtr<DownloadResourceHandler> handler_;
  content::ResourceRequestInfo::WebContentsGetter web_contents_getter_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_REQUEST_HANDLE_H_
