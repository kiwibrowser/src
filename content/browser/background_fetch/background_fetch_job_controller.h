// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_JOB_CONTROLLER_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_JOB_CONTROLLER_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/background_fetch/background_fetch_delegate_proxy.h"
#include "content/browser/background_fetch/background_fetch_registration_id.h"
#include "content/browser/background_fetch/background_fetch_request_info.h"
#include "content/browser/background_fetch/background_fetch_scheduler.h"
#include "content/common/background_fetch/background_fetch_types.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace content {

class BackgroundFetchRequestManager;

// The JobController will be responsible for coordinating communication with the
// DownloadManager. It will get requests from the RequestManager and dispatch
// them to the DownloadService. It lives entirely on the IO thread.
//
// Lifetime: It is created lazily only once a Background Fetch registration
// starts downloading, and it is destroyed once no more communication with the
// DownloadService or Offline Items Collection is necessary (i.e. once the
// registration has been aborted, or once it has completed/failed and the
// waitUntil promise has been resolved so UpdateUI can no longer be called).
class CONTENT_EXPORT BackgroundFetchJobController final
    : public BackgroundFetchDelegateProxy::Controller,
      public BackgroundFetchScheduler::Controller {
 public:
  using FinishedCallback =
      base::OnceCallback<void(const BackgroundFetchRegistrationId&,
                              BackgroundFetchReasonToAbort)>;
  using ProgressCallback =
      base::RepeatingCallback<void(const std::string& /* unique_id */,
                                   uint64_t /* download_total */,
                                   uint64_t /* downloaded */)>;
  BackgroundFetchJobController(
      BackgroundFetchDelegateProxy* delegate_proxy,
      const BackgroundFetchRegistrationId& registration_id,
      const BackgroundFetchOptions& options,
      const SkBitmap& icon,
      const BackgroundFetchRegistration& registration,
      BackgroundFetchRequestManager* request_manager,
      ProgressCallback progress_callback,
      BackgroundFetchScheduler::FinishedCallback finished_callback);
  ~BackgroundFetchJobController() override;

  // Initializes the job controller with the status of the active and completed
  // downloads. Only called when this has been loaded from the database.
  void InitializeRequestStatus(
      int completed_downloads,
      int total_downloads,
      const std::vector<std::string>& outstanding_guids);

  // Gets the number of bytes downloaded for jobs that are currently running.
  uint64_t GetInProgressDownloadedBytes();

  // Updates the UI (currently only job title) that's shown to the user as part
  // of a notification for instance.
  void UpdateUI(const std::string& title);

  // Returns the options with which this job is fetching data.
  const BackgroundFetchOptions& options() const { return options_; }

  base::WeakPtr<BackgroundFetchJobController> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  // BackgroundFetchDelegateProxy::Controller implementation:
  void DidStartRequest(
      const scoped_refptr<BackgroundFetchRequestInfo>& request) override;
  void DidUpdateRequest(
      const scoped_refptr<BackgroundFetchRequestInfo>& request,
      uint64_t bytes_downloaded) override;
  void DidCompleteRequest(
      const scoped_refptr<BackgroundFetchRequestInfo>& request) override;

  // BackgroundFetchScheduler::Controller implementation:
  bool HasMoreRequests() override;
  void StartRequest(scoped_refptr<BackgroundFetchRequestInfo> request) override;
  void Abort(BackgroundFetchReasonToAbort reason_to_abort) override;

 private:

  // Options for the represented background fetch registration.
  BackgroundFetchOptions options_;

  // Icon for the represented background fetch registration.
  SkBitmap icon_;

  // Map from in-progress |download_guid|s to number of bytes downloaded.
  base::flat_map<std::string, uint64_t> active_request_download_bytes_;

  // Cache of downloaded byte count stored by the DataManager, to enable
  // delivering progress events without having to read from the database.
  uint64_t complete_requests_downloaded_bytes_cache_;

  // The RequestManager's lifetime is controlled by the BackgroundFetchContext
  // and will be kept alive until after the JobController is destroyed.
  BackgroundFetchRequestManager* request_manager_;

  // Proxy for interacting with the BackgroundFetchDelegate across thread
  // boundaries. It is owned by the BackgroundFetchContext.
  BackgroundFetchDelegateProxy* delegate_proxy_;

  // Callback run each time download progress updates.
  ProgressCallback progress_callback_;

  // Number of requests that comprise the whole job.
  int total_downloads_ = 0;

  // Number of the requests that have been completed so far.
  int completed_downloads_ = 0;

  base::WeakPtrFactory<BackgroundFetchJobController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchJobController);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_JOB_CONTROLLER_H_
