// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_BACKGROUND_FETCH_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_BACKGROUND_FETCH_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"

class GURL;

namespace gfx {
class Size;
}

namespace net {
class HttpRequestHeaders;
struct NetworkTrafficAnnotationTag;
}  // namespace net

namespace content {
struct BackgroundFetchResponse;
struct BackgroundFetchResult;
struct BackgroundFetchDescription;

// Various reasons a Background Fetch can get aborted.
enum class BackgroundFetchReasonToAbort {
  NONE,
  CANCELLED_FROM_UI,
  ABORTED_BY_DEVELOPER,
  TOTAL_DOWNLOAD_SIZE_EXCEEDED,
};

// Interface for launching background fetches. Implementing classes would
// generally interface with the DownloadService or DownloadManager.
// Must only be used on the UI thread and generally expected to be called by the
// BackgroundFetchDelegateProxy.
class CONTENT_EXPORT BackgroundFetchDelegate {
 public:
  using GetIconDisplaySizeCallback = base::OnceCallback<void(const gfx::Size&)>;

  // Client interface that a BackgroundFetchDelegate would use to signal the
  // progress of a background fetch.
  class Client {
   public:
    virtual ~Client() {}

    // Called when the entire download job has been cancelled by the delegate,
    // e.g. because the user clicked cancel on a notification.
    virtual void OnJobCancelled(
        const std::string& job_unique_id,
        BackgroundFetchReasonToAbort reason_to_abort) = 0;

    // Called after the download has started with the initial response
    // (including headers and URL chain). Always called on the UI thread.
    virtual void OnDownloadStarted(
        const std::string& job_unique_id,
        const std::string& download_guid,
        std::unique_ptr<content::BackgroundFetchResponse> response) = 0;

    // Called during the download to indicate the current progress. Always
    // called on the UI thread.
    virtual void OnDownloadUpdated(const std::string& job_unique_id,
                                   const std::string& download_guid,
                                   uint64_t bytes_downloaded) = 0;

    // Called after the download has completed giving the result including the
    // path to the downloaded file and its size. Always called on the UI thread.
    virtual void OnDownloadComplete(
        const std::string& job_unique_id,
        const std::string& download_guid,
        std::unique_ptr<BackgroundFetchResult> result) = 0;

    // Called by the delegate when it's shutting down to signal that the
    // delegate is no longer valid.
    virtual void OnDelegateShutdown() = 0;
  };

  BackgroundFetchDelegate();

  virtual ~BackgroundFetchDelegate();

  // Gets size of the icon to display with the Background Fetch UI.
  virtual void GetIconDisplaySize(GetIconDisplaySizeCallback callback) = 0;

  // Creates a new download grouping identified by |job_unique_id|. Further
  // downloads started by DownloadUrl will also use this |job_unique_id| so that
  // a notification can be updated with the current status. If the download was
  // already started in a previous browser session, then |current_guids| should
  // contain the GUIDs of in progress downloads, while completed downloads are
  // recorded in |completed_parts|.
  virtual void CreateDownloadJob(
      std::unique_ptr<BackgroundFetchDescription> fetch_description) = 0;

  // Creates a new download identified by |download_guid| in the download job
  // identified by |job_unique_id|.
  virtual void DownloadUrl(
      const std::string& job_unique_id,
      const std::string& download_guid,
      const std::string& method,
      const GURL& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      const net::HttpRequestHeaders& headers) = 0;

  // Aborts any downloads associated with |job_unique_id|.
  virtual void Abort(const std::string& job_unique_id) = 0;

  // Set the client that the delegate should communicate changes to.
  void SetDelegateClient(base::WeakPtr<Client> client) { client_ = client; }

  base::WeakPtr<Client> client() { return client_; }

 private:
  base::WeakPtr<Client> client_;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_BACKGROUND_FETCH_DELEGATE_H_
